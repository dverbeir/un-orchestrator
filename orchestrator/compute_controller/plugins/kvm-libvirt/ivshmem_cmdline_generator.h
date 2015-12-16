#ifndef IVSHMEM_CMDLINE_GENERATOR_H
#define IVSHMEM_CMDLINE_GENERATOR_H

#pragma once

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <string>

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"

class IvshmemCmdLineGenerator
{
private:
	/**
	*	@brief: mutex to protect the initialization of DPDK
	*/
	static pthread_mutex_t IvshmemCmdLineGenerator_mutex;

	static bool init;
	static bool memorypool_generated;
	static pthread_mutex_t memory_pool_mutex;

	static bool dpdk_init(void);

public:
	IvshmemCmdLineGenerator();

	/*
	* @brief:	generates the command line for adding a port to a guest
	*
	* @param:	port_name	Name of the port related to the command line to be generated
	* @param:	cmdline		Buffer where the command line will be saved
	* @param:	size		Size of the buffer for the command line
	*/
	bool get_port_cmdline(const char * port_name, char * cmdline, int size);

	/*
	* @brief:	generates the command line for map ovs mempool into a guest
	*
	* @param:	cmdline		Buffer where the command line will be saved
	* @param:	size		Size of the buffer for the command line
	*/
	bool get_mempool_cmdline(char * cmdline, int size);

	bool get_single_cmdline(char * cmdline, int size, const std::string& vnf_name, std::vector<std::string>& port_names);

protected:
	struct rte_mempool * mempool_lookup();
};

#endif //IVSHMEM_CMDLINE_GENERATOR_H
