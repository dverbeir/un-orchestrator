#include "ovsdb_manager.h"
#include "ovsdb_constants.h"

#include <algorithm>

struct in_addr sIPaddr1;/* struct IP addr server*/
char ErrBuf[BUFFER_SIZE];

int rnumber = 1;
uint64_t dnumber = 1;
int pnumber = 1, nfnumber = 0;

/* Transaction ID */
static int tid = 0;

/*map use to obtain name of switch from id*/
map<uint64_t, string> switch_id;
/*map use to obtain uuid of switch from name*/
map<uint64_t, string> switch_uuid;
/*map use to obtain vport peer of switch from vport name*/
map<string, string> peer_n;
/*map use to obtain list of ports from bridge-id*/
map<uint64_t, list<string> > port_l;
/*map use to obtain list of vports from bridge-id*/
map<uint64_t, list<string> > vport_l;
/*map use to obtain list of port uuid from bridge-id*/
map<uint64_t, list<string> > port_uuid;
/*map use to obtain list of vport uuid from bridge-id*/
map<uint64_t, list<string> > vport_uuid;
/*map use to obtain virtual link id from bridge name*/
map<string, list<uint64_t> > virtual_link_id;
/*map use to obtain name of port from port_id*/
map<uint64_t, string> port_id;
/*map use to id of switch from vport_id*/
map<uint64_t, uint64_t> vl_id;
/*map use to obtain id of peer vlink from local vlink*/
map<uint64_t, uint64_t> vl_p;

/**
 * Build name that is valid as UUID: no '-', no '_' ...
 */
string build_port_uuid_name(const string& port_name, uint64_t bridge_no)
{
	string p_lc = port_name;

	std::transform(p_lc.begin(), p_lc.end(), p_lc.begin(), ::tolower);

 	stringstream ss;
 	ss << p_lc << 'b' << dnumber;
	string uuid_name = ss.str();

	std::replace(uuid_name.begin(), uuid_name.end(), '_', 'p');
	std::replace(uuid_name.begin(), uuid_name.end(), '-', 's');
	return uuid_name;
}

//Constructor
commands::commands(){
}

//Destroyer
commands::~commands(){
}

/*connect to a ovs server using Socket*/
int commands::cmd_connect() {
	uint16_t tport_h;
	int	result, s;
	
	struct addrinfo Hints;
	struct addrinfo *AddrInfo;
	
	/*Read ip and port by the server*/
	result = inet_aton(SOCKET_IP, &sIPaddr1);
	if (!result){
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Invalid IP address.");
		throw commandsException();
	}
	
	if (sscanf(SOCKET_PORT, "%" SCNu16, &tport_h)!=1){
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Invalid number of port.");
		throw commandsException();
	}
	
	memset(&Hints, 0, sizeof(struct addrinfo));

	Hints.ai_family= AF_INET;
	Hints.ai_socktype= SOCK_STREAM;
	
	if (sock_initaddress (SOCKET_IP, SOCKET_PORT, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error resolving given address/port (%s/%s): %s",  SOCKET_IP, SOCKET_PORT, ErrBuf);
		throw commandsException();
	}

	if ( (s=sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Connection failure!");
		throw commandsException();
	}
	
	return s;
}

int commands::cmd_disconnect(int s){
	char ErrBuf[BUFFER_SIZE];

	if(sock_close(s, ErrBuf, sizeof(ErrBuf)) != 0){
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Closing socket failed.");
		throw commandsException();
	}
	
	return EXIT_SUCCESS;
}

/*
*	@cli = struct CreateLsiIn
*/
CreateLsiOut* commands::cmd_editconfig_lsi (CreateLsiIn cli, int s){

    unsigned int i=0;	
    int nwritten = 0;

    ssize_t r = 0;
	char read_buf[4096] = "";

	CreateLsiOut *clo = NULL;
	map<string,unsigned int> physical_ports;
	map<string,map<string, unsigned int> >  network_functions_ports;
	list<pair<unsigned int, unsigned int> > virtual_links;
	
    int dnumber_new = 0, nfnumber_old = 0;

    //list of ports
	list<string> ports = cli.getPhysicalPortsName();

	//list of nf	
	set<string> nfs = cli.getNetworkFunctionsName();

	//list of nft	
	map<string,nf_t> nf_type = cli.getNetworkFunctionsType(); 	

	//list of remote LSI
	list<uint64_t> vport = cli.getVirtualLinksRemoteLSI();
	
	//list of remote LSI
	char rp[2048][64];
	
	//Local variables
	const char *peer_name = "";

	char sw[64] = "Bridge", tcp_s[64] = "tcp:", ctr[64] = "ctrl", vrt[64] = "VirtualPort", trv[64] = "vport";
	char temp[64] = "", tmp[64] = "", of_version[64] = "";

    /*force to use OpenFlow12*/
    strcpy(of_version, "OpenFlow12");

    //connect socket
    s = cmd_connect();

    /*root object contained three object [method, params, id]*/
    Object root;
    root["method"] = "transact";

    Array params;
    params.push_back("Open_vSwitch");

    Object first_obj;
    Object row;
    Array iface;
    Array iface1;
    Array iface2;
	
	//create Bridge
    /*create current name of a bridge "Bridge+dnumber"*/
	sprintf(temp, "%" PRIu64, dnumber);
	strcat(sw, temp);
	
	/*fill the map switch_id*/
	switch_id[dnumber] = string(sw);
	
	Array peer;
    Array peer1;
    Array peer2;

    int l = 0;

    //Create controller
    /*Create the current target of a controller*/
	strcat(tcp_s, cli.getControllerAddress().c_str());
	strcat(tcp_s, ":");
	strcat(tcp_s, cli.getControllerPort().c_str());
	strcpy(temp, tcp_s);
	
	first_obj["op"] = "insert";
    first_obj["table"] = "Controller";

    /*insert a Controller*/
    row["target"] = temp;
    row["local_ip"] = "127.0.0.1";
    row["connection_mode"] = "out-of-band";
    row["is_connected"] = true;

    first_obj["row"] = row;

    //create the current name of a controller --> ctrl+dnumber
    sprintf(temp, "%s%" PRIu64, ctr, dnumber);

    first_obj["uuid-name"] = temp;

    params.push_back(first_obj);

    row.clear();
    first_obj.clear();

	first_obj["op"] = "insert";
    first_obj["table"] = "Bridge";

    /*insert a bridge*/
    row["name"] = sw;

    Array port;
	Array port1;
	Array port2;
	
	port.push_back("set");

	port.push_back(port1);
	
	row["ports"] = port;
	
	port1.clear();
	port.clear();

    Array ctrl;
    ctrl.push_back("set");

	Array ctrl1;
	Array ctrl2;

	//create the current name of a controller
	sprintf(tmp, "%s%" PRIu64, ctr, dnumber);
	
	ctrl2.push_back("named-uuid");
	ctrl2.push_back(tmp);
	
	ctrl1.push_back(ctrl2);
	
	ctrl.push_back(ctrl1);

    row["controller"] = ctrl;

	peer.push_back("map");
	
	peer2.push_back("disable-in-band");
	peer2.push_back("true");
			
	peer1.push_back(peer2);
	peer.push_back(peer1);

    row["other_config"] = peer;

    row["protocols"] = of_version;

    first_obj["row"] = row;

    first_obj["uuid-name"] = sw;

    params.push_back(first_obj);

    row.clear();
    first_obj.clear();
    port.clear();
    port1.clear();
    port2.clear();
    ctrl.clear();
    ctrl1.clear();
    ctrl2.clear();
    peer.clear();
    peer1.clear();
    peer2.clear();

    dnumber_new = dnumber;

    /*Object with four items [op, table, where, mutations]*/
    Object second_obj;
    second_obj["op"] = "mutate";
    second_obj["table"] = "Open_vSwitch";

    /*Empty array [where]*/
    Array where;
    second_obj["where"] = where;

    /*Array with one element*/
    Array w_array;

    /*Array  with three elements*/
    Array m_array;
    m_array.push_back("bridges");
    m_array.push_back("insert");

    /*Array with two elements*/
    Array i_array;
    i_array.push_back("set");

    /*Array with one element*/
    Array s_array;

    /*Array with two element*/
    Array a_array;
    a_array.push_back("named-uuid");
    a_array.push_back(sw);

    s_array.push_back(a_array);

    i_array.push_back(s_array);

    m_array.push_back(i_array);

    w_array.push_back(m_array);

    second_obj["mutations"] = w_array;

    params.push_back(second_obj);

    root["params"] = params;
	root["id"] = tid;

	w_array.clear();
	m_array.clear();
	i_array.clear();
	s_array.clear();
	a_array.clear();
	
	//Increment transaction id
	tid++;

    string *strr = new string[256];

    stringstream ss;
 	write_formatted(root, ss);

    nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}

 	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
    logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
    logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
    logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

	//parse json response
	Value value;
    read( read_buf, value );
    Object rootNode = value.getObject();

    for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
    {
    	const string name = (*it).first;
        const Value &node = (*it).second;

        if (name == "result")
        {
     		const Array &result = node.getArray();
     	
     		for(i=0;i<result.size();i++){
		 		Object uuidNode = result[i].getObject();
		 		
		 		for (Object::const_iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
				{
					const string name1 = (*it1).first;
		    		const Value &node1 = (*it1).second;
		    			
					if(name1 == "uuid"){
						const Array &stuff1 = node1.getArray();
		 				strr[i] = stuff1[1].getString();
					} else if(name1 == "details"){
						logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "%s", node1.getString().c_str());
						throw commandsException();
					}
				}	
     		}	
        }
	}

	//store the switch-uuid
    switch_uuid[dnumber] = strr[i-2];

	/*create physical ports ports*/
	if(ports.size() !=0){
		for(list<string>::iterator p = ports.begin(); p != ports.end(); p++)
		{
			add_port((*p), dnumber, false, s);
			
			port_l[dnumber].push_back((*p).c_str());
			physical_ports[(*p)] = rnumber-1;
		}
	}
	
	/*Create interfaces related to the NFs*/
	map<string,list<string> > out_nf_ports_name_on_switch;
    if(nfs.size() != 0) {
        		
		/*for each network function port in the list of nfs*/
		for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
		{
			list<struct nf_port_info> nfs_ports = cli.getNetworkFunctionsPortsInfo(*nf);
			
			list<string> port_name_on_switch;
			
			map<string,unsigned int> n_ports_1;
			
			/*for each network function port in the list of nfs_ports*/
			for(list<struct nf_port_info>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++){
				add_port(nfp->port_name, dnumber, true, s, nfp->port_type);

				port2.push_back("named-uuid");

				//insert this port name into port_l
				string uuid_name = build_port_uuid_name(nfp->port_name, dnumber);
				port_l[dnumber].push_back(uuid_name);
				
				/*fill the map ports*/
				n_ports_1[nfp->port_name] = rnumber-1;
				
				stringstream pnos;
				pnos << dnumber << "_" << nfp->port_name;
				port_name_on_switch.push_back(pnos.str());
				
			}
			
			/*fill the network_functions_ports*/
			network_functions_ports[(*nf)] = n_ports_1;
			out_nf_ports_name_on_switch[*nf] = port_name_on_switch;
		}
	}
	
	/*Create interfaces related by the vlink ports*/
	if(vport.size() != 0)
	{	
		nfnumber_old = pnumber;
	
		for(list<uint64_t>::iterator nf = vport.begin(); nf != vport.end(); nf++)
		{
			strcpy(vrt, "vport");
			strcpy(trv, "vport");
	    	char ifac[64] = "iface";
	    	
			sprintf(temp, "%d", pnumber);
			strcat(vrt, temp);
			
			pnumber++;
			
			sprintf(temp, "%d", pnumber);
			strcat(trv, temp);

			peer_n[trv] = vrt;
				
			strcpy(rp[l], trv);	
			
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
		
			cmd_add_virtual_link(vrt, trv, ifac, dnumber, s);
			
			port_id[rnumber-1] = vrt;
			port_id[rnumber+vport.size()-1] = trv;
			
			virtual_link_id[sw].push_back(rnumber-1);
			
			vport_l[dnumber].push_back(vrt);
				
			vl_id[rnumber-1] = dnumber;
			vl_id[rnumber+vport.size()-1] = (*nf);
				
			vl_p[rnumber-1] = rnumber+vport.size()-1;	
				
			pnumber++;
			
			l++;
			
			virtual_links.push_back(make_pair(rnumber-1, rnumber+vport.size()-1));
		}
	}
	
    //increment switch number
    dnumber++;

    root.clear();
    params.clear();
    row.clear();
    first_obj.clear();
    port2.clear();
    port1.clear();
    port.clear();
    peer.clear();
    peer1.clear();
    peer2.clear();

    pnumber = nfnumber_old;

    uint64_t pi = 0;

    Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;
	
	//disconnect socket
    cmd_disconnect(s);
	
	l = 0;
	
    if(vport.size() != 0)
	{
		for(list<uint64_t>::iterator nf = vport.begin(); nf != vport.end(); nf++)
		{
			//connect socket
			s = cmd_connect();
		
			root["method"] = "transact";
			params.push_back("Open_vSwitch");
		
			pi = (*nf);
			
			strcpy(vrt, "vport");
			strcpy(trv, "vport");
			char ifac[64] = "iface";
				
			sprintf(temp, "%d", pnumber);
			strcat(vrt, temp);
				
			pnumber++;
				
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
		
			//store this vport
			vport_l[pi].push_back(rp[l]);
				
			peer_name = peer_n[rp[l]].c_str();
		
			cmd_add_virtual_link(rp[l], peer_name, ifac, pi, s);
				
			pnumber++;
			
			root.clear();
			params.clear();
			
			//increment transaction id
			tid++;
			
			l++;
			
			//disconnect socket
			cmd_disconnect(s);
		}
	}

    clo = new CreateLsiOut(dnumber_new, physical_ports, network_functions_ports, out_nf_ports_name_on_switch, virtual_links);
    	
	return clo;
}

void commands::add_port(string p, uint64_t dnumber, bool is_nf_port, int s, PortType port_type){
	
	string uuid_name;
	
    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	map<string, unsigned int> ports;
	
	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;

	logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "add_port(%s,type=%s)", p.c_str(), portTypeToString(port_type).c_str());

	//connect socket
    s = cmd_connect();

	char ifac[64];
    string port_name;
		
	//Create the current name of a interface
	sprintf(ifac, "iface%d", rnumber);
	if (!is_nf_port) {
		uuid_name = p;
		port_name = p;
	} else {
		uuid_name = build_port_uuid_name(p, dnumber);
		
		/*create name of port --> lsiId_portName*/
		stringstream ss;
		ss << dnumber << "_" << p.c_str();
		port_name = ss.str();
	}
	
	root["method"] = "transact";

	params.push_back("Open_vSwitch");
		
	first_obj["op"] = "insert";
	first_obj["table"] = "Interface";
		
	/*Insert an Interface*/
	if (is_nf_port) {
		switch (port_type) {
		case IVSHMEM_PORT:
		case DPDKR_PORT:
			row["type"] = "dpdkr";
			
			
			//XXX the next rows have to be removed when this plugin with support OvS-DPDK
			logger(ORCH_WARNING, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Currently supported by the OvS-DPDK plugin");
			assert(0 && "Currently supported by the OvS-DPDK plugin");
			throw OVSDBManagerException();
			
			break;
		case USVHOST_PORT:
			row["type"] = "dpdkvhostuser";

			// DPDK TODO: Delete socket to be sure ovs can create it!
			
			//XXX the next rows have to be removed when this plugin with support OvS-DPDK
			logger(ORCH_WARNING, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Currently supported by the OvS-DPDK plugin");
			assert(0 && "Currently supported by the OvS-DPDK plugin");
			throw OVSDBManagerException();
			
			break;
		case VETH_PORT:
		{		
			//In this case the veth pair needs to be created!
			stringstream peer_port_name;
			peer_port_name << port_name << ".lxc";
			stringstream cmd_create_veth_pair;
			cmd_create_veth_pair << CREATE_VETH_PAIR << " " << port_name << " " << peer_port_name.str();
			logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"", cmd_create_veth_pair.str().c_str());

			int retVal = system(cmd_create_veth_pair.str().c_str());
			retVal = retVal >> 8;
			if(retVal == 0) {
				logger(ORCH_WARNING, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Failed to create 'veth' port");
				throw OVSDBManagerException();
			}
			
			//	The veth pair peer given to the container is the one with the port_name as derived from the NF-FG.
			//	As a result, the veth pair peer we add to OVS is the one with the decorated name: port_name.lxc
			port_name = peer_port_name.str();
			break;
		}
		default:
			//We are here in case of type "vhost"
			assert(port_type == VHOST_PORT);
			row["type"] = "internal";
			break;
		}
	}
	else { // External ports
		if (p.compare(0, 4, "dpdk") == 0)
			row["type"] = "dpdk";
	}

	row["name"] = port_name.c_str();
	
	row["admin_state"] = "up";
	row["link_state"] = "up";
	row["ofport"] = rnumber;
	row["ofport_request"] = rnumber;
		
	first_obj["row"] = row;
		
	first_obj["uuid-name"] = ifac;
		
	params.push_back(first_obj);
		
	row.clear();
	first_obj.clear();

	/*Insert a port*/
	first_obj["op"] = "insert";
	first_obj["table"] = "Port";
		
	/*Insert a port*/
	row["name"] = port_name.c_str();
		
	iface.push_back("set");
	
	iface2.push_back("named-uuid");
	iface2.push_back(ifac);
	
	iface1.push_back(iface2);
	iface.push_back(iface1);
		
	row["interfaces"] = iface;
		
	first_obj["row"] = row;
		
	first_obj["uuid-name"] = uuid_name.c_str();

	params.push_back(first_obj);
		
	//insert this port name into port_l
	port_l[dnumber].push_back(uuid_name);
		
	row.clear();
	first_obj.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
		
	first_obj["op"] = "update";
	first_obj["table"] = "Bridge";
			
	third_object.push_back("_uuid");
	third_object.push_back("==");
		
	fourth_object.push_back("uuid");
	fourth_object.push_back(switch_uuid[dnumber].c_str());
		
	third_object.push_back(fourth_object);
	where.push_back(third_object);
		
	first_obj["where"] = where;
	
	where.clear();
	
	for(list<string>::iterator u = port_uuid[dnumber].begin(); u != port_uuid[dnumber].end(); u++)
	{
		port2.push_back("uuid");
			
		port2.push_back((*u));
	
		port1.push_back(port2);
	
		port2.clear();
	}
		
	for(list<string>::iterator u = vport_uuid[dnumber].begin(); u != vport_uuid[dnumber].end(); u++)
	{
		port2.push_back("uuid");
			
		port2.push_back((*u));
	
		port1.push_back(port2);
	
		port2.clear();
	}
		
	port2.push_back("named-uuid");
	port2.push_back(uuid_name.c_str());
	
	port1.push_back(port2);
	
	port2.clear();
		
	/*Array with two elements*/
	i_array.push_back("set");
		
	i_array.push_back(port1);
		
	row["ports"] = i_array;
		
	first_obj["row"] = row;
		
	params.push_back(first_obj);
		
	second_obj.clear();
		
	/*Object with four items [op, table, where, mutations]*/
	second_obj["op"] = "mutate";
	second_obj["table"] = "Open_vSwitch";
		
	/*Empty array [where]*/
	second_obj["where"] = where;
		
	/*Array with two element*/
	maa.push_back("next_cfg");
	maa.push_back("+=");
	maa.push_back(1);
			
	ma.push_back(maa);
		
	second_obj["mutations"] = ma;
			
	params.push_back(second_obj);
		
	ma.clear();
	maa.clear();
	row.clear();
	where.clear();
	first_obj.clear();
	second_obj.clear();
	third_object.clear();
	fourth_object.clear();
	i_array.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
	port1.clear();
	port2.clear();
	root["params"] = params;

	root["id"] = tid;

	stringstream ss;
 	write_formatted(root, ss );
		
	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}
		
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);
		
	Value value;
    read( read_buf, value );
    Object rootNode = value.getObject();

	for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{	
		const std::string name = (*it).first;
		const Value &node = (*it).second;
			
		if (name == "result")
		{
			const Array &result = node.getArray();
				 	
			for(unsigned i=0;i<result.size();i++)
			{
				Object uuidNode = result[i].getObject();
							 		
				for (Object::const_iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
				{
					std::string name1 = (*it1).first;
					const Value &node1 = (*it1).second;
							
					if(name1 == "uuid"){
						const Array &stuff1 = node1.getArray();
							 	
						if(i==1){
							port_uuid[dnumber].push_back(stuff1[1].getString());
						}
					} else if(name1 == "details"){
						logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "%s", node1.getString().c_str());
						throw commandsException();
					}
				}		
			}
		}
	}
			
	root.clear();
	params.clear();
			
	//Increment transaction id
	tid++;
	
	rnumber++;

	//disconnect socket
    cmd_disconnect(s);

#if 0
    //XXX: this code is a trick that activates a VNF ports through ifconfig. In fact, we noted that on some system
    //this operation has not done by OVSDB
    if(is_nf_port && (port_type != USVHOST_PORT) && (port_type != IVSHMEM_PORT))
    {
    	stringstream command;
		command << ACTIVATE_INTERFACE << " " << port_name;
		logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Executing command \"%s\"",command.str().c_str());
		int retVal = system(command.str().c_str());
		retVal = retVal >> 8;
		
		assert(retVal == 0);

		if(retVal != 0)
			logger(ORCH_DEBUG_INFO, OVSDB_MODULE_NAME, __FILE__, __LINE__, "This cannot happen. It is here just for the compiler.");
	}
#endif

}

void commands::cmd_editconfig_lsi_delete(uint64_t dpi, int s){
	
    ssize_t nwritten = 0;
	
	int r = 0;
	
	char read_buf[4096] = "";
	
	map<string, unsigned int> ports;
	
	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;

	/*for all virtual link ports, destroy it*/
	for(list<uint64_t>::iterator i = virtual_link_id[switch_id[dpi]].begin(); i != virtual_link_id[switch_id[dpi]].end(); i++){
		cmd_delete_virtual_link(vl_id[(*i)], (*i), s);
		cmd_delete_virtual_link(vl_id[vl_p[(*i)]], vl_p[(*i)], s);
	}

	port_l.erase(dpi);
	vport_l.erase(dpi);
	port_uuid.erase(dpi);
	vport_uuid.erase(dpi);
	virtual_link_id[switch_id[dpi]].clear();

	//connect socket
    s = cmd_connect();

	root["method"] = "transact";

	params.push_back("Open_vSwitch");
				
	first_obj["op"] = "update";
	first_obj["table"] = "Open_vSwitch";
			
	first_obj["where"] = where;
	
	switch_uuid.erase(dpi);
	
	for(map<uint64_t, string>::iterator sww = switch_uuid.begin(); sww != switch_uuid.end(); sww++)
	{
		port2.push_back("uuid");	
		port2.push_back(sww->second);
		port1.push_back(port2);

		port2.clear();
	}
		
	/*Array with two elements*/
	i_array.push_back("set");
		
	i_array.push_back(port1);
		
	row["bridges"] = i_array;
		
	first_obj["row"] = row;
		
	params.push_back(first_obj);
		
	second_obj.clear();
		
	/*Object with four items [op, table, where, mutations]*/
	second_obj["op"] = "mutate";
	second_obj["table"] = "Open_vSwitch";
		
	/*Empty array [where]*/
	second_obj["where"] = where;
			
	/*Array with two element*/
	maa.push_back("next_cfg");
	maa.push_back("+=");
	maa.push_back(1);
			
	ma.push_back(maa);
		
	second_obj["mutations"] = ma;
			
	params.push_back(second_obj);
	
	ma.clear();
	maa.clear();
	row.clear();
	where.clear();
	first_obj.clear();
	second_obj.clear();
	third_object.clear();
	fourth_object.clear();
	i_array.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
	port1.clear();
	port2.clear();
	root["params"] = params;

	root["id"] = tid;

	stringstream ss;
 	write_formatted(root, ss );	
	
	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}
			
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);

	root.clear();
	params.clear();
			
	//Increment transaction id
	tid++;
	
	//disconnect socket
    cmd_disconnect(s);
}

AddNFportsOut *commands::cmd_editconfig_NFPorts(AddNFportsIn anpi, int s){

	AddNFportsOut *anf = NULL;
	
    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	map<string, unsigned int> ports;
	list<string> ports_name_on_switch;

	list<struct nf_port_info> nf_ports = anpi.getNetworkFunctionsPorts();
	
	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;

	//connect socket
    s = cmd_connect();

	if(nf_ports.size() != 0){
		
		root["method"] = "transact";

		params.push_back("Open_vSwitch");
		
		/*for each port in the list*/
		for(list<struct nf_port_info>::iterator nfp = nf_ports.begin(); nfp != nf_ports.end(); nfp++)
		{
			char ifac[64];
			string port_name;
				
			//Create the current name of a interface
			sprintf(ifac, "iface%d", rnumber);
			
			string uuid_name = build_port_uuid_name(nfp->port_name, anpi.getDpid());
				
			first_obj["op"] = "insert";
			first_obj["table"] = "Interface";
		
			stringstream ss;
			ss << anpi.getDpid() << '_' << nfp->port_name;
			port_name = ss.str();

			row["name"] = port_name.c_str();
			row["type"] = "internal";
			
			row["admin_state"] = "up";
			row["link_state"] = "up";
			row["ofport"] = rnumber;
			row["ofport_request"] = rnumber;

			first_obj["row"] = row;

			first_obj["uuid-name"] = ifac;

			params.push_back(first_obj);

			row.clear();
			first_obj.clear();

			first_obj["op"] = "insert";
			first_obj["table"] = "Port";

			row["name"] = port_name.c_str();

			iface.push_back("set");

			iface2.push_back("named-uuid");
			iface2.push_back(ifac);

			iface1.push_back(iface2);
			iface.push_back(iface1);

			row["interfaces"] = iface;

			first_obj["row"] = row;

			first_obj["uuid-name"] = uuid_name.c_str();

			params.push_back(first_obj);

			row.clear();
			first_obj.clear();
			iface.clear();
			iface1.clear();
			iface2.clear();

			//insert this port name into port_n
			port_l[anpi.getDpid()].push_back(uuid_name);

			ports[nfp->port_name] = rnumber;

			stringstream pnos;
			pnos << anpi.getDpid() << "_" << nfp->port_name;
			ports_name_on_switch.push_back(pnos.str());

			rnumber++;
		}

		first_obj["op"] = "update";
		first_obj["table"] = "Bridge";

		third_object.push_back("_uuid");
		third_object.push_back("==");

		fourth_object.push_back("uuid");
		fourth_object.push_back(switch_uuid[anpi.getDpid()].c_str());
		
		third_object.push_back(fourth_object);
		where.push_back(third_object);
		
		first_obj["where"] = where;
	
		where.clear();
	
		for(list<string>::iterator u = port_uuid[anpi.getDpid()].begin(); u != port_uuid[anpi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");
			port2.push_back((*u));
			port1.push_back(port2);
			port2.clear();
		}
		
		for(list<string>::iterator u = vport_uuid[anpi.getDpid()].begin(); u != vport_uuid[anpi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");		
			port2.push_back((*u));
			port1.push_back(port2);
			port2.clear();
		}

		list<string> ports_name_on_switch;
		for(list<struct nf_port_info>::iterator nfp = nf_ports.begin(); nfp != nf_ports.end(); nfp++)
		{
			string uuid_name = build_port_uuid_name(nfp->port_name, anpi.getDpid());
		
			port2.push_back("named-uuid");
			port2.push_back(uuid_name);
			port1.push_back(port2);
			port2.clear();
		}
		
		/*Array with two elements*/
		i_array.push_back("set");		
		i_array.push_back(port1);
		row["ports"] = i_array;
		first_obj["row"] = row;
		params.push_back(first_obj);
		
		second_obj.clear();
		
		/*Object with four items [op, table, where, mutations]*/
		second_obj["op"] = "mutate";
		second_obj["table"] = "Open_vSwitch";
		
		/*Empty array [where]*/
		second_obj["where"] = where;
			
		/*Array with two element*/
		maa.push_back("next_cfg");
		maa.push_back("+=");
		maa.push_back(1);
			
		ma.push_back(maa);
		
		second_obj["mutations"] = ma;
			
		params.push_back(second_obj);
		
		ma.clear();
		maa.clear();
		row.clear();
		where.clear();
		first_obj.clear();
		second_obj.clear();
		third_object.clear();
		fourth_object.clear();
		i_array.clear();
		iface.clear();
		iface1.clear();
		iface2.clear();
		port1.clear();
		port2.clear();
		root["params"] = params;

		root["id"] = tid;

		stringstream ss;
 		write_formatted(root, ss );
		
		nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
		if (nwritten == sockFAILURE)
		{
			logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			throw commandsException();
		}

    	r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (r == sockFAILURE)
		{
			logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			throw commandsException();
		}
		
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);
		
		Value value;
    	read( read_buf, value );
    	Object rootNode = value.getObject();

		for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
		{
			std::string name = (*it).first;
			const Value &node = it->second;
			if (name == "result")
			{
				const Array &result = node.getArray();
				
				for(unsigned i=0;i<result.size();i++)
				{
					Object uuidNode = result[i].getObject();
								 		
					for (Object::const_iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
					{
						std::string name1 = (*it1).first;
						const Value &node1 = it1->second;
							
						if(name1 == "uuid")
						{
							const Array &stuff1 = node1.getArray();
									
							if(i==1){
								port_uuid[dnumber].push_back(stuff1[i].getString());
							}
						} else if(name1 == "details"){
							logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "%s", node1.getString().c_str());
							throw commandsException();
						}
					}		
				}
			}
		}
			
		root.clear();
		params.clear();
			
		//Increment transaction id
		tid++;
	}

	//disconnect socket
    cmd_disconnect(s);

    anf = new AddNFportsOut(anpi.getNFname(), ports, ports_name_on_switch);

	return anf;
}

void commands::cmd_editconfig_NFPorts_delete(DestroyNFportsIn dnpi, int s){
	
    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	map<string, unsigned int> ports;

	set<string> nfp = dnpi.getNFports();
	
	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;

	//connect socket
    s = cmd_connect();

	if(nfp.size() != 0){
		
		root["method"] = "transact";

		params.push_back("Open_vSwitch");
		
		first_obj["op"] = "update";
		first_obj["table"] = "Bridge";
			
		third_object.push_back("_uuid");
		third_object.push_back("==");
		
		fourth_object.push_back("uuid");
		fourth_object.push_back(switch_uuid[dnpi.getDpid()].c_str());
		
		third_object.push_back(fourth_object);
		where.push_back(third_object);
		
		first_obj["where"] = where;
	
		where.clear();
		
		list<string>::iterator uu = port_uuid[dnpi.getDpid()].begin();
		
		/*for each port in the list of the getNetworkFunctionsPorts*/
		for(set<string>::iterator p = nfp.begin(); p != nfp.end(); p++){
			string sss = (*p);
			uu = port_uuid[dnpi.getDpid()].begin();
			//should be search in port_l, p....if find it take the index and remove it from the set port-uuid[pi]
			for(list<string>::iterator u = port_l[dnpi.getDpid()].begin(); u != port_l[dnpi.getDpid()].end(); u++){
				string s = (*u);
				if(s.compare(sss) == 0){
					port_uuid[dnpi.getDpid()].remove((*uu));
					break;	
				}
				uu++;
			}
		}
	
		for(list<string>::iterator u = port_uuid[dnpi.getDpid()].begin(); u != port_uuid[dnpi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");
				
			port2.push_back((*u));
	
			port1.push_back(port2);
	
			port2.clear();
		}
		
		for(list<string>::iterator u = vport_uuid[dnpi.getDpid()].begin(); u != vport_uuid[dnpi.getDpid()].end(); u++)
		{
			port2.push_back("uuid");
			
			port2.push_back((*u));
	
			port1.push_back(port2);
	
			port2.clear();
		}
		
		/*Array with two elements*/
		i_array.push_back("set");
		
		i_array.push_back(port1);
		
		row["ports"] = i_array;
		
		first_obj["row"] = row;
		
		params.push_back(first_obj);
		
		second_obj.clear();
		
		/*Object with four items [op, table, where, mutations]*/
		second_obj["op"] = "mutate";
		second_obj["table"] = "Open_vSwitch";
		
		/*Empty array [where]*/
		second_obj["where"] = where;
			
		/*Array with two element*/
		maa.push_back("next_cfg");
		maa.push_back("+=");
		maa.push_back(1);
			
		ma.push_back(maa);
		
		second_obj["mutations"] = ma;
			
		params.push_back(second_obj);
		
		ma.clear();
		maa.clear();
		row.clear();
		where.clear();
		first_obj.clear();
		second_obj.clear();
		third_object.clear();
		fourth_object.clear();
		i_array.clear();
		iface.clear();
		iface1.clear();
		iface2.clear();
		port1.clear();
		port2.clear();
		root["params"] = params;

		root["id"] = tid;

		stringstream ss;
 		write_formatted(root, ss );
		
		nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
		if (nwritten == sockFAILURE)
		{
			logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			throw commandsException();
		}

    	r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (r == sockFAILURE)
		{
			logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			throw commandsException();
		}
			
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
		logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);
			
		root.clear();
		params.clear();
			
		//Increment transaction id
		tid++;
		
		cmd_disconnect(s);
	}
}

AddVirtualLinkOut *commands::cmd_addVirtualLink(AddVirtualLinkIn avli, int s){
	/*struct to return*/
	AddVirtualLinkOut *avlo = NULL;
	
	char temp[64] = "", vrt[64] = "", trv[64] = "";
	
	list<pair<unsigned int, unsigned int> > virtual_links;
	
	strcpy(vrt, "vport");
	char ifac[64] = "iface";
	strcpy(trv, "vport");
	    	
	/*create virtual link*/
	sprintf(temp, "%d", pnumber);
	strcat(vrt, temp);
		
	pnumber++;
			
	/*create virtual link*/
	sprintf(temp, "%d", pnumber);
	strcat(trv, temp);
		
	//Create the current name of a interface
	sprintf(temp, "%d", rnumber);
	strcat(ifac, temp);

	//create first endpoint
	cmd_add_virtual_link(vrt, trv, ifac, avli.getDpidA(), s);
	
	//Create the current name of a interface
	strcpy(ifac, "iface");
	sprintf(temp, "%d", rnumber);
	strcat(ifac, temp);
	
	//create second endpoint
	cmd_add_virtual_link(trv, vrt, ifac, avli.getDpidB(), s);

	/*store the information [bridge name, list of peer port]*/
	virtual_link_id[switch_id[avli.getDpidA()]].push_back(rnumber-2);
	
	/*store the information [bridge name, list of peer port]*/
	virtual_link_id[switch_id[avli.getDpidB()]].push_back(rnumber-1);

	port_id[rnumber-2] = vrt;
	port_id[rnumber-1] = trv;

	/*store the information [switch_id, port_id]*/
	vl_id[rnumber-2] = avli.getDpidA();
	vl_id[rnumber-1] = avli.getDpidB();
	
	//insert this port name into port_n
	vport_l[avli.getDpidA()].push_back(vrt);
	//insert this port name into port_n
	vport_l[avli.getDpidB()].push_back(trv);
	
	/*peer name of the bridge name vrt is trv*/
	peer_n[trv] = vrt;
	
	/*peer name of the bridge name trv is vrt*/
	peer_n[vrt] = trv;

	avlo = new AddVirtualLinkOut(rnumber-2, rnumber-1);

	return avlo;
}

void commands::cmd_add_virtual_link(string vrt, string trv, char ifac[64], uint64_t dpi, int s){
	
    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	map<string, unsigned int> ports;
	
	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;

	//connect socket
    s = cmd_connect();

	root["method"] = "transact";

	params.push_back("Open_vSwitch");
			
	first_obj["op"] = "insert";
	first_obj["table"] = "Interface";
		
	/*Insert an Interface*/
	row["name"] = vrt;
	row["type"] = "patch";
	
	row["admin_state"] = "up";
	row["link_state"] = "up";
	row["ofport"] = rnumber;
	row["ofport_request"] = rnumber;
		
	/*Add options peer*/
	peer.push_back("map");
	
	peer2.push_back("peer");
	peer2.push_back(trv);
			
	peer1.push_back(peer2);
	peer.push_back(peer1);

    row["options"] = peer;
		
	first_obj["row"] = row;
		
	first_obj["uuid-name"] = ifac;
		
	params.push_back(first_obj);
		
	row.clear();
	first_obj.clear();
				
	first_obj["op"] = "insert";
	first_obj["table"] = "Port";
		
	/*Insert a port*/
	row["name"] = vrt;
		
	iface.push_back("set");
	
	iface2.push_back("named-uuid");
	iface2.push_back(ifac);
	
	iface1.push_back(iface2);
	iface.push_back(iface1);
		
	row["interfaces"] = iface;
		
	first_obj["row"] = row;
		
	first_obj["uuid-name"] = vrt;
		
	params.push_back(first_obj);
		
	row.clear();
	first_obj.clear();
	peer.clear();
	peer1.clear();
	peer2.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
		
	first_obj["op"] = "update";
	first_obj["table"] = "Bridge";
			
	third_object.push_back("_uuid");
	third_object.push_back("==");
		
	fourth_object.push_back("uuid");
	fourth_object.push_back(switch_uuid[dpi].c_str());
		
	third_object.push_back(fourth_object);
	where.push_back(third_object);
		
	first_obj["where"] = where;
	
	where.clear();
	
	for(list<string>::iterator u = port_uuid[dpi].begin(); u != port_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");
			
		port2.push_back((*u));
	
		port1.push_back(port2);
	
		port2.clear();
	}
	
	for(list<string>::iterator u = vport_uuid[dpi].begin(); u != vport_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");
			
		port2.push_back((*u));
	
		port1.push_back(port2);
	
		port2.clear();
	}
	
	port2.push_back("named-uuid");
	port2.push_back(vrt);
	
	port1.push_back(port2);
	
	port2.clear();
		
	/*Array with two elements*/
	i_array.push_back("set");
		
	i_array.push_back(port1);
		
	row["ports"] = i_array;
		
	first_obj["row"] = row;
		
	params.push_back(first_obj);
		
	second_obj.clear();
		
	/*Object with four items [op, table, where, mutations]*/
	second_obj["op"] = "mutate";
	second_obj["table"] = "Open_vSwitch";
		
	/*Empty array [where]*/
	second_obj["where"] = where;
			
	/*Array with two element*/
	maa.push_back("next_cfg");
	maa.push_back("+=");
	maa.push_back(1);
			
	ma.push_back(maa);
		
	second_obj["mutations"] = ma;
			
	params.push_back(second_obj);
	
	ma.clear();
	maa.clear();
	row.clear();
	where.clear();
	first_obj.clear();
	second_obj.clear();
	third_object.clear();
	fourth_object.clear();
	i_array.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
	port1.clear();
	port2.clear();
	root["params"] = params;

	root["id"] = tid;

	stringstream ss;
 	write_formatted(root, ss );
	
	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}
		
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);	
		
	Value value;
    read( read_buf, value );
    Object rootNode = value.getObject();

	for (Object::const_iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{
		std::string name = (*it).first;
		const Value &node = it->second;
		if (name == "result")
		{
			const Array &result = node.getArray();
				
			for(unsigned i=0;i<result.size();i++)
			{
				Object uuidNode = result[i].getObject();
						 		
				for (Object::iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
				{
					std::string name1 = (*it1).first;
					Value &node1 = (*it1).second;
							
					if(name1 == "uuid")
					{
						const Array &stuff1 = node1.getArray();
									
						if(i==1){
							vport_uuid[dpi].push_back(stuff1[i].getString());
						}
					} else if(name1 == "details"){
						logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "%s", node1.getString().c_str());
						throw commandsException();
					}
				}		
			}
		}
	}
			
	root.clear();
	params.clear();
			
	//Increment transaction id
	tid++;
	
	rnumber++;
	
	//disconnect socket
    cmd_disconnect(s);
}

void commands::cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli, int s){
	
	char vrt[64] = "", trv[64] = "";
	
	list<pair<unsigned int, unsigned int> > virtual_links;

	//destroy first endpoint
	cmd_delete_virtual_link(dvli.getDpidA(), dvli.getIdA(), s);
	
	//destroy second endpoint
	cmd_delete_virtual_link(dvli.getDpidB(), dvli.getIdB(), s);

	/*erase information*/
	vl_id.erase(dvli.getIdA());
	vl_id.erase(dvli.getIdB());
	
	//remove this port name from port_n
	vport_l[dvli.getDpidA()].remove(vrt);
	vport_l[dvli.getDpidB()].remove(trv);
	
	//remove this port id from vport id list
	virtual_link_id[switch_id[dvli.getDpidA()]].remove(dvli.getIdA());
	virtual_link_id[switch_id[dvli.getDpidB()]].remove(dvli.getIdB());
	
}

void commands::cmd_delete_virtual_link(uint64_t dpi, uint64_t idp, int s){

    ssize_t nwritten;
	
	char read_buf[4096] = "";
	
	int r = 0;
	
	map<string, unsigned int> ports;
	
	Object root, first_obj, second_obj, row;
	Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Array ma;
	Array maa;
	
	Array third_object;
	Array fourth_object;

	//connect socket
    s = cmd_connect();

	root["method"] = "transact";

	params.push_back("Open_vSwitch");
				
	first_obj["op"] = "update";
	first_obj["table"] = "Bridge";
			
	third_object.push_back("_uuid");
	third_object.push_back("==");
		
	fourth_object.push_back("uuid");
	fourth_object.push_back(switch_uuid[dpi].c_str());
		
	third_object.push_back(fourth_object);
	where.push_back(third_object);
		
	first_obj["where"] = where;
	
	where.clear();
	
	//search the port-id and after erase this port-uuid
	list<string>::iterator uu = vport_uuid[dpi].begin();
	
	//should be search in port_l, p....if find it take the index and remove it from the set port-uuid[pi]
	for(list<string>::iterator u = vport_l[dpi].begin(); u != vport_l[dpi].end(); u++){
		string sss = (*u);
		if(port_id[idp].compare(sss) == 0){
			vport_uuid[dpi].remove(*uu);
			vport_l[dpi].remove(*u);
			break;	
		}
		uu++;
	}
	
	//insert normal ports
	for(list<string>::iterator u = port_uuid[dpi].begin(); u != port_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");
			
		port2.push_back((*u));
	
		port1.push_back(port2);
	
		port2.clear();
	}
	
	//insert vports
	for(list<string>::iterator u = vport_uuid[dpi].begin(); u != vport_uuid[dpi].end(); u++)
	{
		port2.push_back("uuid");
			
		port2.push_back((*u));
	
		port1.push_back(port2);
	
		port2.clear();
	}
		
	/*Array with two elements*/
	i_array.push_back("set");
		
	i_array.push_back(port1);
		
	row["ports"] = i_array;
		
	first_obj["row"] = row;
		
	params.push_back(first_obj);
		
	second_obj.clear();
		
	/*Object with four items [op, table, where, mutations]*/
	second_obj["op"] = "mutate";
	second_obj["table"] = "Open_vSwitch";
		
	/*Empty array [where]*/
	second_obj["where"] = where;
			
	/*Array with two element*/
	maa.push_back("next_cfg");
	maa.push_back("+=");
	maa.push_back(1);
			
	ma.push_back(maa);
		
	second_obj["mutations"] = ma;
			
	params.push_back(second_obj);
	
	ma.clear();
	maa.clear();
	row.clear();
	where.clear();
	first_obj.clear();
	second_obj.clear();
	third_object.clear();
	fourth_object.clear();
	i_array.clear();
	iface.clear();
	iface1.clear();
	iface2.clear();
	port1.clear();
	port2.clear();
	root["params"] = params;

	root["id"] = tid;

	stringstream ss;
 	write_formatted(root, ss );
	
	nwritten = sock_send(s, ss.str().c_str(), strlen(ss.str().c_str()), ErrBuf, sizeof(ErrBuf));
	if (nwritten == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
		throw commandsException();
	}

    r = sock_recv(s, read_buf, sizeof(read_buf), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
	if (r == sockFAILURE)
	{
		logger(ORCH_ERROR, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
		throw commandsException();
	}
			
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Message sent to ovs: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, ss.str().c_str());
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, "Answer: ");
	logger(ORCH_DEBUG, OVSDB_MODULE_NAME, __FILE__, __LINE__, read_buf);
			
	root.clear();
	params.clear();
			
	//Increment transaction id
	tid++;
	
	//disconnect socket
    cmd_disconnect(s);
}
