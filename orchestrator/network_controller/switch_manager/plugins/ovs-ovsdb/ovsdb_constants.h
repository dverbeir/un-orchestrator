#ifndef OVS_CONSTANTS_H_
#define OVS_CONSTANTS_H_ 1

#define OVSDB_MODULE_NAME			"OvS-OVSDB-Manager"

/**
*	@brief: paths of the bash scripts exploited by the plugin
*/
#define CREATE_VETH_PAIR			"./network_controller/switch_manager/plugins/ovs-ovsdb/scripts/create_veth_pair.sh"
#ifdef ENABLE_OVSDB_DPDK
#define PREP_USVHOST_PORT			"./network_controller/switch_manager/plugins/ovs-ovsdb/scripts/prep_usvhost.sh"
#endif
#if 0
#define ACTIVATE_INTERFACE			"./network_controller/switch_manager/plugins/ovs-ovsdb/scripts/activate_interface.sh"
#endif

#define SOCKET_IP					"127.0.0.1"
#define SOCKET_PORT					"6632"
#define NUM_UUID					3

#endif //OVS_CONSTANTS_H_
