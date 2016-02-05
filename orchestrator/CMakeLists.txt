CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(node-orchestrator)

IF(CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings )
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
IF(CMAKE_COMPILER_IS_GNUCC)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings )
ENDIF(CMAKE_COMPILER_IS_GNUCC)


# Rather complicated CMake code for setting the logging level
SET(LOGGING_LEVEL "ORCH_WARNING" CACHE STRING "Orchestrator logging levels: ORCH_DEBUG, ORCH_DEBUG_INFO, ORCH_WARNING, ORCH_ERROR, ORCH_INFO")
SET(LOGGING_LEVEL_VALUES "ORCH_DEBUG" "ORCH_DEBUG_INFO" "ORCH_WARNING" "ORCH_ERROR" "ORCH_INFO")
SET_PROPERTY(CACHE LOGGING_LEVEL PROPERTY STRINGS ${LOGGING_LEVEL_VALUES})
LIST(FIND LOGGING_LEVEL_VALUES ${LOGGING_LEVEL} LOGGING_LEVEL_INDEX)

IF(${LOGGING_LEVEL_INDEX} EQUAL -1)
	message(FATAL_ERROR "Option ${LOGGING_LEVEL} not supported, valid entries are ${LOGGING_LEVEL_VALUES}")
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 0)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_DEBUG)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 1)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_DEBUG_INFO)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 2)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_WARNING)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 3)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_ERROR)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 4)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_INFO)
ENDIF()
# End of the rather complicated CMake code for setting the logging level

# Rather complicated CMake code for selecting the virtual switch implementation
SET(VSWITCH_IMPLEMENTATION "OVS-OVSDB" CACHE STRING "vSwitch implementation: XDPD, OVS-OFCONFIG, OVS-DPDK, OVS-OVSDB, ERFS")
SET(VSWITCH_IMPLEMENTATION_VALUES "XDPD" "OVS-OFCONFIG" "OVS-DPDK" "OVS-OVSDB" "ERFS") #Add here other implementations
SET_PROPERTY(CACHE VSWITCH_IMPLEMENTATION PROPERTY STRINGS ${VSWITCH_IMPLEMENTATION_VALUES})
LIST(FIND VSWITCH_IMPLEMENTATION_VALUES ${VSWITCH_IMPLEMENTATION} VSWITCH_IMPLEMENTATION_INDEX)

IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL -1)
	message(FATAL_ERROR "Option ${VSWITCH_IMPLEMENTATION} not supported, valid entries are ${VSWITCH_IMPLEMENTATION_VALUES}")
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 0)
	ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_XDPD)
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 1)
	ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_OFCONFIG)
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 2)
	ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_OVSDPDK)
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 3)
	ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_OVSDB)
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 4)
        ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_ERFS)
ENDIF()

OPTION(
	STARTVNF_SINGLE_THREAD
	"Turn on to start the VNF sequentially"
	OFF
)

IF(STARTVNF_SINGLE_THREAD)
	ADD_DEFINITIONS(-DSTARTVNF_SINGLE_THREAD)
ENDIF(STARTVNF_SINGLE_THREAD)

OPTION(
	ENABLE_KVM
	"Turn on to support NFs implemented in KVM-based virtual machines"
	ON
)
IF(ENABLE_KVM)
	ADD_DEFINITIONS(-DENABLE_KVM)
ENDIF(ENABLE_KVM)

OPTION(
	ENABLE-DPDK-CMDLINE-GENERATOR
	"Turn on to compile the code to generate the command line needed to add ivshmem device in the VM"
	OFF
)

OPTION(
	ENABLE_OVSDB_DPDK
	"Turn on to support OVS with DPDK netdev using the OVS-OVSDB netwrok controller plugin"
	OFF
)
IF(ENABLE_OVSDB_DPDK)
	ADD_DEFINITIONS(-DENABLE_OVSDB_DPDK)
ENDIF(ENABLE_OVSDB_DPDK)
#################################################################################################################

OPTION(
	RUN_NFS
	"Turn on the actually run the NFs"
	ON
)
IF(RUN_NFS)
	ADD_DEFINITIONS(-DRUN_NFS)
ENDIF(RUN_NFS)

OPTION(
	ENABLE_DOCKER
	"Turn on the support of NFs implemented in Docker containers"
	OFF
)
IF(ENABLE_DOCKER)
	ADD_DEFINITIONS(-DENABLE_DOCKER)
ENDIF(ENABLE_DOCKER)

OPTION(
	ENABLE_NATIVE
	"Turn on the support of NFs implemented in native form"
	OFF
)
IF(ENABLE_NATIVE)
	ADD_DEFINITIONS(-DENABLE_NATIVE)
ENDIF(ENABLE_NATIVE)


OPTION(ENABLE_DPDK_PROCESSES
	"Turn on the support of NFs implemented in as DPDK processes"
	OFF
)
IF(ENABLE_DPDK_PROCESSES)
	ADD_DEFINITIONS(-DENABLE_DPDK_PROCESSES)
ENDIF(ENABLE_DPDK_PROCESSES)


OPTION(DEBUG_KVM
	"Turn on the debug of the KVM-libvirt compute plugin"
	OFF
)

IF(DEBUG_KVM)
	ADD_DEFINITIONS(-DDEBUG_KVM)
ENDIF(DEBUG_KVM)

OPTION(
	POLITO_MESSAGE
	"Turn on to support a slightly different JSON message describing the graph"
	OFF
)
IF(POLITO_MESSAGE)
	ADD_DEFINITIONS(-DPOLITO_MESSAGE)
ENDIF(POLITO_MESSAGE)

OPTION(
	LOG_1024X768
	"Turn on to print the log for a monitor of 1024 X 768"
	OFF
)
IF(LOG_1024X768)
	ADD_DEFINITIONS(-DLOG_1024X768)
ENDIF(LOG_1024X768)

OPTION(
	DEBUG_OPENFLOW
	"Turn on to enable the debug of Openflow messages"
	OFF
)
IF(DEBUG_OPENFLOW)
	ADD_DEFINITIONS(-DDEBUG_OPENFLOW)
ENDIF(DEBUG_OPENFLOW)


OPTION(
	VIRTUALIZER_NFFG
	"Turn on to support the virtualizer NF-FG model defined in WP3"
	OFF
)
IF(VIRTUALIZER_NFFG)
	ADD_DEFINITIONS(-DUNIFY_NFFG)
ENDIF(VIRTUALIZER_NFFG)

OPTION(
	LOG_ON_FILE
	"Turn on to print log messages on the file 'node-orchestrator.log'. If disabled, stdout is used for the log."
	OFF
)
IF(LOG_ON_FILE)
	ADD_DEFINITIONS(-DLOG_ON_FILE)
ENDIF(LOG_ON_FILE)

OPTION(
	IVSHMEM_RING_ALIAS
	"Use IVSHMEM Ring Aliases to expose localized ring names to VMs (requires DPDK patch)"
	OFF
)
IF(IVSHMEM_RING_ALIAS)
	ADD_DEFINITIONS(-DIVSHMEM_RING_ALIAS)
ENDIF(IVSHMEM_RING_ALIAS)


MARK_AS_ADVANCED(
	DEBUG_KVM DEBUG_OPENFLOW POLITO_MESSAGE LOG_1024X768 RUN_NFS STARTVNF_SINGLE_THREAD IVSHMEM_RING_ALIAS
)

# Set source files
SET(SOURCES
	node_orchestrator.cc

	node_resource_manager/graph_manager/graph_manager.h
	node_resource_manager/graph_manager/graph_manager.cc
	node_resource_manager/graph_manager/graph_info.h
	node_resource_manager/graph_manager/graph_info.cc
	node_resource_manager/graph_manager/graph_translator.h
	node_resource_manager/graph_manager/graph_translator.cc
	node_resource_manager/graph_manager/rule_removed_info.h
	node_resource_manager/graph_manager/lsi.h
	node_resource_manager/graph_manager/lsi.cc
	node_resource_manager/graph_manager/virtual_link.h
	node_resource_manager/graph_manager/virtual_link.cc
	node_resource_manager/graph_manager/fileParser.h
	node_resource_manager/graph_manager/fileParser.cc

	node_resource_manager/graph/match.h
	node_resource_manager/graph/match.cc
	node_resource_manager/graph/generic_action.h
	node_resource_manager/graph/generic_action.cc
	node_resource_manager/graph/vlan_action.h
	node_resource_manager/graph/vlan_action.cc

	node_resource_manager/graph/low_level_graph/output_action.h
	node_resource_manager/graph/low_level_graph/output_action.cc
	node_resource_manager/graph/low_level_graph/graph.h
	node_resource_manager/graph/low_level_graph/graph.cc
	node_resource_manager/graph/low_level_graph/low_level_match.h
	node_resource_manager/graph/low_level_graph/low_level_match.cc
	node_resource_manager/graph/low_level_graph/rule.h
	node_resource_manager/graph/low_level_graph/rule.cc

	node_resource_manager/graph/high_level_graph/high_level_output_action.h
	node_resource_manager/graph/high_level_graph/high_level_output_action.cc
	node_resource_manager/graph/high_level_graph/high_level_output_action_nf.h
	node_resource_manager/graph/high_level_graph/high_level_output_action_nf.cc
	node_resource_manager/graph/high_level_graph/high_level_output_action_port.h
	node_resource_manager/graph/high_level_graph/high_level_output_action_port.cc
	node_resource_manager/graph/high_level_graph/high_level_output_action_endpoint.h
	node_resource_manager/graph/high_level_graph/high_level_output_action_endpoint.cc
	node_resource_manager/graph/high_level_graph/high_level_rule.h
	node_resource_manager/graph/high_level_graph/high_level_rule.cc
	node_resource_manager/graph/high_level_graph/high_level_graph.h
	node_resource_manager/graph/high_level_graph/high_level_graph.cc
	node_resource_manager/graph/high_level_graph/high_level_match.h
	node_resource_manager/graph/high_level_graph/high_level_match.cc

	network_controller/switch_manager/switch_manager.h
	network_controller/switch_manager/addNFports_in.h
	network_controller/switch_manager/addNFports_out.h
	network_controller/switch_manager/addVirtualLink_in.h
	network_controller/switch_manager/addVirtualLink_out.h
	network_controller/switch_manager/createLSIout.h
	network_controller/switch_manager/destroyNFports_in.h
	network_controller/switch_manager/destroyVirtualLink_in.h
	network_controller/switch_manager/checkPhysicalPorts_in.h

	network_controller/openflow_controller/controller.h
	network_controller/openflow_controller/controller.cc

	compute_controller/nfs_manager.h
	compute_controller/nfs_manager.cc
	compute_controller/compute_controller.h
	compute_controller/compute_controller.cc
	compute_controller/nf.h
	compute_controller/nf.cc
	compute_controller/nf_type.h
	compute_controller/description.h
	compute_controller/description.cc
	compute_controller/startNF_in.h
	compute_controller/stopNF_in.h

	node_resource_manager/rest_server/rest_server.h
	node_resource_manager/rest_server/rest_server.cc
	node_resource_manager/rest_server/match_parser.h
	node_resource_manager/rest_server/match_parser.cc

	utils/logger.h
	utils/logger.c
	utils/constants.h
	utils/sockutils.h
	utils/sockutils.c
)

IF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 0)
    SET(SOURCES2
    ${SOURCES}
	network_controller/switch_manager/plugins/xdpd/xdpd_manager.h
	network_controller/switch_manager/plugins/xdpd/xdpd_manager.cc
	network_controller/switch_manager/plugins/xdpd/xdpd_constants.h
    )
ENDIF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 0)

IF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 1)
    SET(SOURCES2
    ${SOURCES}
	network_controller/switch_manager/plugins/ovs-ofconfig/commands.h
	network_controller/switch_manager/plugins/ovs-ofconfig/commands.cc
	network_controller/switch_manager/plugins/ovs-ofconfig/ofconfig_constants.h
	network_controller/switch_manager/plugins/ovs-ofconfig/ofconfig_manager.h
	network_controller/switch_manager/plugins/ovs-ofconfig/ofconfig_manager.cc
    )
ENDIF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 1)

IF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 2)
    SET(SOURCES2
    ${SOURCES}
	network_controller/switch_manager/plugins/ovs-dpdk/ovsdpdk_manager.h
	network_controller/switch_manager/plugins/ovs-dpdk/ovsdpdk_manager.cc
	network_controller/switch_manager/plugins/ovs-dpdk/ovsdpdk_constants.h
    )
ENDIF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 2)

IF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 3)
    SET(SOURCES2
    ${SOURCES}
	network_controller/switch_manager/plugins/ovs-ovsdb/commands.h
	network_controller/switch_manager/plugins/ovs-ovsdb/commands.cc
	network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_constants.h
	network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_manager.h
    network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_manager.cc
    )
ENDIF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 3)

IF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 4)
    SET(SOURCES2
    ${SOURCES}
	network_controller/switch_manager/plugins/erfs/manager.h
	network_controller/switch_manager/plugins/erfs/manager.cc
	network_controller/switch_manager/plugins/erfs/constants.h
    )
ENDIF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 4)

IF(ENABLE_KVM)
    SET(SOURCES3
    ${SOURCES2}
	compute_controller/plugins/kvm-libvirt/libvirt.cc
	compute_controller/plugins/kvm-libvirt/libvirt.h	
	compute_controller/plugins/kvm-libvirt/ivshmem_cmdline_generator.cc
	compute_controller/plugins/kvm-libvirt/ivshmem_cmdline_generator.h
	)
ELSE()
     SET(SOURCES3
         ${SOURCES2}
    	)
ENDIF(ENABLE_KVM)

IF(ENABLE_DOCKER)
    SET(SOURCES4
    ${SOURCES3}
	compute_controller/plugins/docker/docker.cc
	compute_controller/plugins/docker/docker.h
	compute_controller/plugins/docker/docker_constants.h
	)
ELSE()
     SET(SOURCES4
         ${SOURCES3}
    	)
ENDIF(ENABLE_DOCKER)


IF(VIRTUALIZER_NFFG)
	SET(SOURCES5
    ${SOURCES4}
    node_resource_manager/virtualizer/virtualizer.cc
    node_resource_manager/virtualizer/virtualizer.h
	)
ELSE()
	SET(SOURCES5
    	${SOURCES4}
	)
ENDIF(VIRTUALIZER_NFFG)

IF(ENABLE_DPDK_PROCESSES)
    SET(SOURCES6
    ${SOURCES5}
	compute_controller/plugins/dpdk/dpdk.h
	compute_controller/plugins/dpdk/dpdk.cc
	compute_controller/plugins/dpdk/dpdk_constants.h
	compute_controller/plugins/dpdk/dpdk_description.cc
	compute_controller/plugins/dpdk/dpdk_description.h
	)
ELSE()
     SET(SOURCES6
         ${SOURCES5}
    	)
ENDIF(ENABLE_DPDK_PROCESSES)

IF(ENABLE_NATIVE)
    SET(SOURCES7
    ${SOURCES6}
	compute_controller/plugins/native/native.cc
	compute_controller/plugins/native/native.h
	compute_controller/plugins/native/native_constants.h
	compute_controller/plugins/native/capability.h
	compute_controller/plugins/native/capability.cc
	compute_controller/plugins/native/native_description.h
	compute_controller/plugins/native/native_description.cc
	)
ELSE()
     SET(SOURCES7
         ${SOURCES6}
    	)
ENDIF(ENABLE_NATIVE)


INCLUDE_DIRECTORIES (
	/usr/include/boost
	/usr/include/libxml2
)

#Include specific for the virtualizer NF-FG model defined in WP3
IF(VIRTUALIZER_NFFG)
	find_package(PythonLibs REQUIRED)
	INCLUDE_DIRECTORIES (
		${PYTHON_INCLUDE_DIRS}
	)
ENDIF(VIRTUALIZER_NFFG)

# Create the executable
ADD_EXECUTABLE(
	node-orchestrator
	${SOURCES7}
)

LINK_DIRECTORIES(
	/usr/lib/
	/usr/local/lib/
	/usr/x86_64-linux-gnu/
	/usr/lib/x86_64-linux-gnu/
	/usr/lib/python2.7/config-x86_64-linux-gnu/
)

TARGET_LINK_LIBRARIES( node-orchestrator
	libpthread.so
	/usr/local/lib/librofl_common.so
	libjson_spirit.so
	libmicrohttpd.so
	libboost_system.so
	libxml2.so
	-lrt
)

#Links specific for ovs
IF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 1)
	TARGET_LINK_LIBRARIES( node-orchestrator
		libnetconf.so
	)
ENDIF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 1)

#Links specific for KVM
IF(ENABLE_KVM)
	TARGET_LINK_LIBRARIES( node-orchestrator
		libvirt.so
	)
ENDIF(ENABLE_KVM)

#Links specific for the virtualizer NF-FG model defined in WP3
IF(VIRTUALIZER_NFFG)
    TARGET_LINK_LIBRARIES( node-orchestrator
		libpython2.7.so
		${PYTHON_LIBRARIES}
	)
ENDIF(VIRTUALIZER_NFFG)

# Add configuration rules to compile command line generator
IF(ENABLE-DPDK-CMDLINE-GENERATOR)

	IF(NOT DEFINED ENV{RTE_SDK})
		message(FATAL_ERROR "Please define RTE_SDK environment variable (it must point to the DPDK folder)")
	ENDIF(NOT DEFINED ENV{RTE_SDK})
	
	IF(NOT DEFINED ENV{RTE_TARGET})
		message(FATAL_ERROR "Please define RTE_TARGET environment variable as follows: 'export RTE_TARGET=x86_64-ivshmem-linuxapp-gcc'")
	ENDIF(NOT DEFINED ENV{RTE_TARGET})

	ADD_DEPENDENCIES(node-orchestrator cmd_line_generator)

	IF (IVSHMEM_RING_ALIAS)
		SET(CMDLINEGEN_MAKE_PREARGS EXTRA_CFLAGS="-DIVSHMEM_RING_ALIAS")
	ENDIF(IVSHMEM_RING_ALIAS)
	  
	ADD_CUSTOM_TARGET(
		cmd_line_generator
		COMMAND ${CMDLINEGEN_MAKE_PREARGS} make
		WORKING_DIRECTORY compute_controller/plugins/kvm-libvirt/cmdline_generator/
		)

ENDIF(ENABLE-DPDK-CMDLINE-GENERATOR)

###############################################################################

#Check unsupported configurations

IF(ENABLE_DPDK_PROCESSES AND ( (${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 1) OR (${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 3) ) )
	#This configuration (ovs (ofconfig/ovsdb) + dpdk processes) is not supported
	message(FATAL_ERROR "OvS not supported with Dpdk processes")
ENDIF(ENABLE_DPDK_PROCESSES AND ( (${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 1) OR (${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 3) ) )

IF(ENABLE-DPDK-CMDLINE-GENERATOR AND (${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 1) )
	#This configuration (ovs-ofconfig + kvm-ivshmem) is not supported
	message(FATAL_ERROR "OVS-OFCONFIG not supported with KVM-IVSHMEM")
ENDIF(ENABLE-DPDK-CMDLINE-GENERATOR AND (${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 1) )

IF( (NOT ENABLE_DOCKER) AND (NOT ENABLE_NATIVE) AND (NOT ENABLE_DPDK_PROCESSES) AND (NOT ENABLE_KVM) )
	#No execution environment is selected
	message(FATAL_ERROR "At least one execution environmnet must be enabled")
ENDIF( (NOT ENABLE_DOCKER) AND (NOT ENABLE_NATIVE) AND (NOT ENABLE_DPDK_PROCESSES) AND (NOT ENABLE_KVM) )

