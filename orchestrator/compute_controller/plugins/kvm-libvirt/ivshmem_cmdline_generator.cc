#include "ivshmem_cmdline_generator.h"

#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>

#include <rte_config.h>
#include <rte_eal.h>
#include <rte_ivshmem.h>
#include <rte_mempool.h>
#include <rte_ring.h>

#define DPDKR_FORMAT "dpdkr%d"
#define DPDKR_TX_FORMAT DPDKR_FORMAT"_tx"
#define DPDKR_RX_FORMAT DPDKR_FORMAT"_rx"

#define MEMPOOL_METADATA_NAME "OVSMEMPOOL"

bool IvshmemCmdLineGenerator::init = false;
pthread_mutex_t IvshmemCmdLineGenerator::IvshmemCmdLineGenerator_mutex = PTHREAD_MUTEX_INITIALIZER;

bool IvshmemCmdLineGenerator::memorypool_generated = false;
pthread_mutex_t IvshmemCmdLineGenerator::memory_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

IvshmemCmdLineGenerator::IvshmemCmdLineGenerator()
{
	//TODO: handle error in initialization
	dpdk_init();
}


bool IvshmemCmdLineGenerator::dpdk_init(void)
{
	cpu_set_t *c;
	int nCores = 0;

	pthread_mutex_lock(&IvshmemCmdLineGenerator_mutex);

	/* does not exist a nicer way to do it? */
	/* XXX: why -n is not required? */
	char * arg[] =
	{
		"./something",
		"--proc-type=secondary",
		"-c",
		"0x01",
		"--",
		NULL
	};

	int argv = sizeof(arg)/sizeof(*arg) - 1;

	if(init)
		goto generated;

	/*
	 * XXX: DPDK versions before bb7c5ab does not reset the getopt library before
	 * using it, so if such library has been used before calling rte_eal_init it
	 * could fail.
	 * bb7c5ab solves the issue and should be included in dpdk 2.2.0
	 */
	optind = 1;
	if(rte_eal_init(argv, (char**)arg) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "DPDK can not be initialized");
		pthread_mutex_unlock(&IvshmemCmdLineGenerator_mutex);
		return false;
	}

	init = true;

	/*
	 * rte_eal_init changes the core mask of the thread.
	 * Change it back to all cores
	 */
	nCores = sysconf(_SC_NPROCESSORS_ONLN);

	c = CPU_ALLOC(nCores);
	for(int i = 0;  i < nCores; i++)
		CPU_SET(i, c);

	sched_setaffinity(0, nCores, c);

generated:
	pthread_mutex_unlock(&IvshmemCmdLineGenerator_mutex);
	return true;
}

/*
 * Share global packet's mempool with a given metadata name
 */
#define MAX_MEMPOOLS 100
struct mempool_list {
	const struct rte_mempool *mp[MAX_MEMPOOLS];
	int num_mp;
};
static void mempool_check_func(const struct rte_mempool *mp, void *arg)
{
	const char ovs_mp_prefix[] = "ovs_mp_";
	const char *mp_exact_match[] = { "pool_direct", "pool_indirect", NULL };
	int i;

	struct mempool_list* mpl = (struct mempool_list*)arg;
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Considering mempool '%s'\n", mp->name);
	if (strncmp(mp->name, ovs_mp_prefix,
				sizeof(ovs_mp_prefix)/sizeof(*ovs_mp_prefix) - 1) == 0) {
		if (mpl->num_mp < MAX_MEMPOOLS) {
			mpl->mp[mpl->num_mp] = mp;
			mpl->num_mp++;
		}
	}

	for (i = 0; mp_exact_match[i]; i++) {
		if (strncmp(mp->name, mp_exact_match[i], strlen(mp_exact_match[i])) == 0) {
			if (mpl->num_mp < MAX_MEMPOOLS) {
				mpl->mp[mpl->num_mp] = mp;
				mpl->num_mp++;
			}
		}
	}
}

static int share_mempools(const char *metadata_name)
{
        struct mempool_list mpl;
        int i;

        logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Adding mempools to metadata '%s'\n", metadata_name);

        /* Walk the mempools and stores the ones that hold OVS mbufs in mp */
        mpl.num_mp = 0;
        rte_mempool_walk(mempool_check_func, (void *)&mpl);
        if (mpl.num_mp >= MAX_MEMPOOLS) {
        	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Too many mempool to share to metadata '%s'\n",
                                                metadata_name);
			return -1;
        }

        for (i = 0; i < mpl.num_mp; i++) {
			if (rte_ivshmem_metadata_add_mempool(mpl.mp[i], metadata_name) < 0) {
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Failed adding mempool '%s' to metadata '%s'\n",
													mpl.mp[i]->name, metadata_name);
					return -1;
			}
        }

        return 0;
}


bool IvshmemCmdLineGenerator::get_single_cmdline(char * cmdline, int size, const std::string& vnf_name, std::vector<std::string>& port_names)
{
#if 0
	struct rte_mempool * packets_pool = NULL;
#endif

	if (rte_ivshmem_metadata_create(vnf_name.c_str()) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Metadata file can not be created");
		goto error;
	}
#if 1
	share_mempools(vnf_name.c_str());
#else
	packets_pool = mempool_lookup();
	if (packets_pool == NULL)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"OVS packets mempool can not be found");
		goto error;
	}
	if(rte_ivshmem_metadata_add_mempool(packets_pool, vnf_name.c_str()) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"OVS packets mempool can not be added to metadatafile");
		goto error;
	}
#endif

	for (std::vector<std::string>::iterator it = port_names.begin(); it != port_names.end(); ++it) {
		int port_no;
		struct rte_ring * rx;
		struct rte_ring * tx;
		char ring_name[20];
		const char* port_name = (*it).c_str();

		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Adding data for port '%s'", port_name);

		/* it has to read just one integer that is the port name */
		if(sscanf(port_name, DPDKR_FORMAT, &port_no) != 1)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
					"Port (%s) has bad port name format", port_name);
			goto error;
		}

		/* look for the transmission ring */
		snprintf(ring_name, 20, DPDKR_TX_FORMAT, port_no);
		tx = rte_ring_lookup(ring_name);
		if(tx == NULL)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
					"port (%s) can not be found", port_name);
			goto error;
		}

		/* look fot the reception ring */
		snprintf(ring_name, 20, DPDKR_RX_FORMAT, port_no);
		rx = rte_ring_lookup(ring_name);
		if(rx == NULL)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
					"port (%s) can not be found", port_name);
			goto error;
		}

		if(rte_ivshmem_metadata_add_ring(tx, vnf_name.c_str()) < 0)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
					"Port (%s) can not be added to metadata_file", port_name);
			goto error;
		}

		if(rte_ivshmem_metadata_add_ring(rx, vnf_name.c_str()) < 0)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
					"Port (%s) can not be added to metadata_file", port_name);
			goto error;
		}
	}

	if (rte_ivshmem_metadata_cmdline_generate(cmdline, size, vnf_name.c_str()) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Command line for mempool can not be generated");
		goto error;
	}
	pthread_mutex_unlock(&memory_pool_mutex);
	return true;

error:
	pthread_mutex_unlock(&memory_pool_mutex);;
	return false;
}


/* look for the memory pool */
/*
* XXX: improve the wasy the memory pool is looked, the name could not
* always be the same
*/
struct rte_mempool * IvshmemCmdLineGenerator::mempool_lookup()
{
	return rte_mempool_lookup("ovs_mp_1500_0_262144");
}

bool IvshmemCmdLineGenerator::get_mempool_cmdline(char * cmdline, int size)
{
	struct rte_mempool * packets_pool;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
			"Generating command line for memory pool");

	/*lazy dpdk initialization */
	if(!init)
		return false;

	pthread_mutex_lock(&memory_pool_mutex);
	if(memorypool_generated)
		goto generate;

	packets_pool = mempool_lookup();
	if (packets_pool == NULL)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"OVS packets mempool can not be found");
		goto error;
	}

	if (rte_ivshmem_metadata_create(MEMPOOL_METADATA_NAME) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Metadata file can not be created");
		goto error;
	}

	if(rte_ivshmem_metadata_add_mempool(packets_pool, MEMPOOL_METADATA_NAME) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"OVS packets mempool can not be added to metadatafile");
		goto error;
	}

generate:
	if (rte_ivshmem_metadata_cmdline_generate(cmdline, size, MEMPOOL_METADATA_NAME) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Command line for mempool can not be generated");
		goto error;
	}
	memorypool_generated = true;
	pthread_mutex_unlock(&memory_pool_mutex);
	return true;

error:
	pthread_mutex_unlock(&memory_pool_mutex);;
	return false;
}

bool
IvshmemCmdLineGenerator::get_port_cmdline(const char * port_name, char * cmdline, int size)
{
	char ring_name[20];
	int port_no;
	struct rte_ring * rx;
	struct rte_ring * tx;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
			"Generating command line for port '%s'", port_name);

	/*lazy dpdk initialization */
	if(!init)
		return false;

	/* it has to read just one integer that is the port name */
	if(sscanf(port_name, DPDKR_FORMAT, &port_no) != 1)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Port (%s) has bad port name format", port_name);
		return false;
	}

	/* look for the transmission ring */
	snprintf(ring_name, 20, DPDKR_TX_FORMAT, port_no);
	tx = rte_ring_lookup(ring_name);
	if(tx == NULL)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"port (%s) can not be found", port_name);
		return false;
	}

	/* look fot the reception ring */
	snprintf(ring_name, 20, DPDKR_RX_FORMAT, port_no);
	rx = rte_ring_lookup(ring_name);
	if(rx == NULL)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"port (%s) can not be found", port_name);
		return false;
	}

	if (rte_ivshmem_metadata_create(port_name) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Metadata file can not be created");
		return false;
	}

	if(rte_ivshmem_metadata_add_ring(tx, port_name) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Port (%s) can not be added to metadata_file", port_name);
		return false;
	}

	if(rte_ivshmem_metadata_add_ring(rx, port_name) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Port (%s) can not be added to metadata_file", port_name);
		return false;
	}

	if (rte_ivshmem_metadata_cmdline_generate(cmdline, size, port_name) < 0)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
				"Command line can not be generated", port_name);
		return false;
	}

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__,
			"Command line: '%s'", cmdline);

	return true;
}
