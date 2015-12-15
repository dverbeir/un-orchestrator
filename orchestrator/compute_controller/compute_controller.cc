#include "compute_controller.h"

pthread_mutex_t ComputeController::nfs_manager_mutex = PTHREAD_MUTEX_INITIALIZER;

map<int,uint64_t> ComputeController::cores;
int ComputeController::nextCore = 0;

ComputeController::ComputeController()
{
}

ComputeController::~ComputeController()
{
}

void ComputeController::setCoreMask(uint64_t core_mask)
{
	uint64_t mask = 1;
	for(uint64_t i = 0; i < 64; i++)
	{
		if(core_mask & mask)
		{
			cores[nextCore] = mask;
			nextCore++;
		}
			
		mask = mask << 1;
	}
	nextCore = 0;
	
	for(unsigned int i = 0; i < cores.size(); i++)
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Mask of an available core: \"%d\"",cores[i]);
}

nf_manager_ret_t ComputeController::retrieveDescription(string nf)
{
	try
 	{
 		string translation;

 		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Considering the NF \"%s\"",nf.c_str());

		char ErrBuf[BUFFER_SIZE];
		struct addrinfo Hints;
		struct addrinfo *AddrInfo;
		int socket;							// keeps the socket ID for this connection
		int WrittenBytes;					// Number of bytes written on the socket
		int ReadBytes;						// Number of bytes received from the socket
		char DataBuffer[DATA_BUFFER_SIZE];	// Buffer containing data received from the socket

		memset(&Hints, 0, sizeof(struct addrinfo));

		Hints.ai_family= AF_INET;
		Hints.ai_socktype= SOCK_STREAM;

		if (sock_initaddress (NAME_RESOLVER_ADDRESS, NAME_RESOLVER_PORT, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error resolving given address/port (%s/%s): %s",  NAME_RESOLVER_ADDRESS, NAME_RESOLVER_PORT, ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		stringstream tmp;
		tmp << "GET " << NAME_RESOLVER_BASE_URL << nf << " HTTP/1.1\r\n";
		tmp << "Host: :" << NAME_RESOLVER_ADDRESS << ":" << NAME_RESOLVER_PORT << "\r\n";
		tmp << "Connection: close\r\n";
		tmp << "Accept: */*\r\n\r\n";
		string message = tmp.str();

		char command[message.size()+1];
		command[message.size()]=0;
		memcpy(command,message.c_str(),message.size());

		if ( (socket= sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
		{
			// AddrInfo is no longer required
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot contact the name resolver at \"%s:%s\"", NAME_RESOLVER_ADDRESS, NAME_RESOLVER_PORT);
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "%s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		WrittenBytes = sock_send(socket, command, strlen(command), ErrBuf, sizeof(ErrBuf));
		if (WrittenBytes == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;

		}

		ReadBytes= sock_recv(socket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (ReadBytes == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		// Terminate buffer, just for printing purposes
		// Warning: this can originate a buffer overflow
		DataBuffer[ReadBytes]= 0;

		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Data received: ");
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",DataBuffer);

		shutdown(socket,SHUT_WR);
		sock_close(socket,ErrBuf,sizeof(ErrBuf));

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_METHOD_NOT_ALLLOWED,3) == 0)
			return NFManager_NO_NF;

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_OK,3) != 0)
			return NFManager_SERVER_ERROR;

		//the HTTP headers must be removed
		int i = 0;
		for(; i < ReadBytes; i++)
		{
			if((i+4) <= ReadBytes)
			{
				if((DataBuffer[i] == '\r') && (DataBuffer[i+1] == '\n') && (DataBuffer[i+2] == '\r') && (DataBuffer[i+3] == '\n'))
				{
					i += 4;
					break;
				}
			}
		}

		translation.assign(&DataBuffer[i]);

		if(!parseAnswer(translation,nf))
		{
			//ERROR IN THE SERVER
			return NFManager_SERVER_ERROR;
		}
 	}
	catch (std::exception& e)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Exception: %s",e.what());
		return NFManager_SERVER_ERROR;
	}

	return NFManager_OK;
}

bool ComputeController::parseAnswer(string answer, string nf)
{
	try
	{
		Value value;
		read(answer, value);
		Object obj = value.getObject();

		bool foundName = false;
		bool foundImplementations = false;

		list<Description*> possibleDescriptions;
		string nf_name;
#ifdef UNIFY_NFFG
		unsigned int numports = 0;
		string text_description;
		
		bool foundNports = false, foundTextDescription = false;
#endif

		for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
		{
	 	    const string& name  = i->first;
		    const Value&  value = i->second;

		    if(name == "name")
		    {
		    	foundName = true;
		    	nf_name = value.getString();
		    	if(nf_name != nf)
		    	{
			    	logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Required NF \"%s\", received info for NF \"%s\"",nf.c_str(),nf_name.c_str());
					return false;
		    	}
		    }
		    else if(name == "nports")
		    {
#ifdef UNIFY_NFFG
				foundNports = true;
		    	numports = value.getInt();
#endif
		    }
		    else if(name == "description")
		    {
#ifdef UNIFY_NFFG
				foundTextDescription = true;
		    	text_description = value.getString();
#endif
		    }
		    else if(name == "implementations")
		    {
		    	foundImplementations = true;
		    	const Array& impl_array = value.getArray();
		    	if(impl_array.size() == 0)
		    	{
			    	logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"implementations\" without descriptions");
					return false;
		    	}

				bool next = false;
		    	//Itearate on the implementations
		    	for( unsigned int impl = 0; impl < impl_array.size(); ++impl)
				{
					//This is an implementation, with a type and an URI
					Object implementation = impl_array[impl].getObject();
					bool foundURI = false;
					bool foundType = false;
					bool foundCores = false;
					bool foundLocation = false;

					string type;
			    	string uri;
					string cores;
					string location;
					std::vector<PortType> port_types;

					for( Object::const_iterator impl_el = implementation.begin(); impl_el != implementation.end(); ++impl_el )
					{
				 	    const string& el_name  = impl_el->first;
						const Value&  el_value = impl_el->second;

						if(el_name == "type")
						{
							foundType = true;
							type = el_value.getString();
							if(!NFType::isValid(type))
							{
								logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Invalid implementation type \"%s\". Skip it.", type.c_str());
								//return false;
								next = true;
								break;
							}
						}
						else if(el_name == "uri")
						{
							foundURI = true;
							uri = el_value.getString();
						}
						else if(el_name == "cores")
						{
							foundCores = true;
							cores = el_value.getString();
						}
						else if(el_name == "location")
						{
							foundLocation = true;
							location = el_value.getString();
						}
						else if(el_name == "ports")
						{
					    	const Array& ports_array = el_value.getArray();
					    	logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "%d ports in implementation", ports_array.size());

					    	if (ports_array.size() == 0)
					    	{
						    	logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Empty ports list in implementation");
								return false;
					    	}
					    	for( unsigned int p = 0; p < ports_array.size(); ++p)
							{
								Object port = ports_array[p].getObject();
								int port_id = -1;
								PortType port_type = UNDEFINED_PORT;

								for( Object::const_iterator port_el = port.begin(); port_el != port.end(); ++port_el )
								{
							 	    const string& pel_name  = port_el->first;
									const Value&  pel_value = port_el->second;
									if (pel_name == "id") {
										port_id = pel_value.getInt();
									}
									else if (pel_name == "type") {
										port_type = portTypeFromString(pel_value.getString());
									}
									else {
										logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" within an implementation port", pel_name.c_str());
										return false;
									}
								}
								if (port_id == -1) {
									logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Missing port \"ïd\" attribute for implementation");
									return false;
								}
								if (port_type == UNDEFINED_PORT) {
									logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Missing port \"type\" attribute for implementation");
									return false;
								}
								logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Port %d id=%d type=%d", p, port_id, port_type);

								if ((size_t)port_id >= port_types.size())
									port_types.resize(port_id + 1);
								port_types[port_id] = port_type;
							}
						}
						else
						{
							logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\" within an implementation", el_name.c_str());
							return false;
						}
					}
					
					if(next)
					{
						//The current network function is of a type not supported by the orchestator
						next = false;
						continue;
					}
					
					if(!foundURI || !foundType)
					{
						logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"uri\", key \"type\", or both are not found into an implementation description");
						return false;
					}
					if(type == "dpdk")
					{
						if(!foundCores || !foundLocation)
						{
							logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Description of a NF of type \"%s\" received without the \"cores\" attribute, \"location\" attribute, or both",type.c_str());
							return false;
						}
					}
					else if(foundCores || foundLocation)
					{
						logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Description of a NF of type \"%s\" received with a wrong attribute (\"cores\", \"location\", or both)",type.c_str());
						return false;
					}

					Description* descr = new Description(type, uri, cores, location, port_types);
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Description has %d PortTypes (ref %d)", descr->getPortTypes().size(), port_types.size());

					possibleDescriptions.push_back(descr);
				}
		    } //end if(name == "implementations")
		    else
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\"",name.c_str());
				return false;
			}
		}//end iteration on the answer

		if(!foundName || !foundImplementations
#ifdef UNIFY_NFFG
			|| !foundNports || !foundTextDescription
#endif		
		)
		{
#ifdef UNIFY_NFFG
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"name\", and/or key \"implementations\", and/or key \"num-ports\", and/or key \"description\" not found in the answer");
#else
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"name\", or key \"implementations\", or both not found in the answer");
#endif
			return false;
		}

		NF *new_nf = new NF(nf_name
#ifdef UNIFY_NFFG		
			,numports, text_description
#endif	
		);
		assert(possibleDescriptions.size() != 0);
		
		if(possibleDescriptions.size() == 0)
		{
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Cannot find a supported implementation for the network function \"%s\"",nf.c_str());
			return false;
		}		
		
		for(list<Description*>::iterator impl = possibleDescriptions.begin(); impl != possibleDescriptions.end(); impl++)
			new_nf->addDescription(*impl);

		nfs[nf_name] = new_nf;

	}
	catch (std::runtime_error& e) {
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "JSON parse error: %s", e.what());
		return false;
	}
	catch(...)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The content does not respect the JSON syntax");
		return false;
	}

	return true;
}

bool ComputeController::selectImplementation()
{
	NFsManager *manager = NULL;

#ifdef ENABLE_DOCKER
	//Manage Docker execution environment

	manager = new Docker();
	if(manager->isSupported())
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Select a Docker implementation if exists.");
		selectImplementation(DOCKER);
		
		if(allSelected())
			return true;
	}
	else
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Docker deamon is not running (at least, it is not running with the LXC implementation).");
#endif

#ifdef ENABLE_DPDK_PROCESSES
	//Manage DPDK execution environment

	manager = new Dpdk();
	if(manager->isSupported())
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Select DPDK implementation if exists.");
	
		selectImplementation(DPDK);
		if(allSelected())
			return true;
	}
	else
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "DPDK is not supported.");
#endif

#ifdef ENABLE_KVM
	//Manage QEMU/KVM execution environment through libvirt
	
	manager = new Libvirt();
	
	if(manager->isSupported())
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Select QEMU/KVM implementation if exists.");
		selectImplementation(KVM);
		
		if(allSelected())
			return true;
	}
	else
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "QEMU/KVM is not supported.");

#endif

	//[+] Add here other implementations for the execution environment

	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Some network functions do not have a supported implementation!");
	return false;
}

void ComputeController::selectImplementation(nf_t desiredType)
{
	//Select an implmentation of the NF
	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		NF *current = nf->second;
		
		//An descritpion is selected only for those functions that do not have a description yet
		if(current->getSelectedDescription() == NULL)
		{
			list<Description*> descriptions = current->getAvailableDescriptions();

			list<Description*>::iterator impl;
			for(impl = descriptions.begin(); impl != descriptions.end(); impl++)
			{
				if((*impl)->getType() == desiredType)
				{
					NFsManager *manager = NULL;

					//Create the proper NFsManager according to the selected type
					switch(desiredType)
					{
#ifdef ENABLE_DPDK_PROCESSES
						case DPDK:
							manager = new Dpdk();
							break;
#endif
#ifdef ENABLE_DOCKER
						case DOCKER:
							manager = new Docker();
							break;
#endif
#ifdef ENABLE_KVM
						case KVM:
							manager = new Libvirt();
							break;
#endif
						//[+] Add here other implementations for the execution environment
						default:
							assert(0);
					}
					
					manager->setDescription(*impl);					
					current->setSelectedDescription(manager);
										
					logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s description has been selected for NF \"%s\".",NFType::toString(desiredType).c_str(),nf->first.c_str());
					break;
				}
			}
		}
	}
}

bool ComputeController::allSelected()
{
	bool retVal = true;

	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		NF *current = nf->second;
		if(current->getSelectedDescription() == NULL)
		{
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "The NF \"%s\" has not been selected yet.",nf->first.c_str());
			retVal = false;
		}
	}
	return retVal;
}

#ifdef UNIFY_NFFG
unsigned int ComputeController::getNumPorts(string name)
{
	assert(nfs.count(name) != 0);

	NF *nf = nfs[name];
	unsigned int np = nf->getNumPorts();
	
	assert(np != 0);
	
	return np;
}
#endif

nf_t ComputeController::getNFType(string name)
{
	assert(nfs.count(name) != 0);

	NF *nf = nfs[name];
	NFsManager *impl = nf->getSelectedDescription();

	assert(impl != NULL);

	return impl->getNFType();
}

void ComputeController::setLsiID(uint64_t lsiID)
{
	this->lsiID = lsiID;
}

//FIXME: I'm assuming that the first element of namesOfPortsOnTheSwitch corresponds to the first port of the network function,
//the second element corresponds to the second port of the network function, and so on...
bool ComputeController::startNF(string nf_name, list<string> namesOfPortsOnTheSwitch)
{
	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Starting the NF \"%s\"",nf_name.c_str());
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Ports of the NF connected to the switch:");
	for(list<string>::iterator it = namesOfPortsOnTheSwitch.begin(); it != namesOfPortsOnTheSwitch.end(); it++)
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\t%s",(*it).c_str());

	if(nfs.count(nf_name) == 0)
	{
		assert(0);
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Unknown NF with name \"%s\"",nf_name.c_str());
		return false;
	}

	NF *nf = nfs[nf_name];
	NFsManager *nfsManager = nf->getSelectedDescription();
	
	
	StartNFIn sni(lsiID, nf_name, namesOfPortsOnTheSwitch, calculateCoreMask(nfsManager->getCores()));

	if(!nfsManager->startNF(sni))
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while starting the NF \"%s\"",nf_name.c_str());
		return false;
	}

	nf->setRunning(true);

	return true;
}

void ComputeController::stopAll()
{
	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
		stopNF(nf->first);
}

bool ComputeController::stopNF(string nf_name)
{
	//FIXME: remove the NF from the map?
	//FIXME: if not, remove at least the selected description?

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Stopping the NF \"%s\"",nf_name.c_str());

	if(nfs.count(nf_name) == 0)
	{
		logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Unknown NF with name \"%s\"",nf_name.c_str());
		return false;
	}

	NF *nf = nfs[nf_name];
	NFsManager *nfsManager = nf->getSelectedDescription();
	
	StopNFIn sni(lsiID,nf_name);
	
	if(!nfsManager->stopNF(sni))
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while stopping the NF \"%s\"",nf_name.c_str());
		return false;
	}
	
	nf->setRunning(false);

	return true;
}

uint64_t ComputeController::calculateCoreMask(string coresRequried)
{
	if(coresRequried == "")
		return 0x0;

	int requiredCores;
	sscanf(coresRequried.c_str(),"%d",&requiredCores);

	pthread_mutex_lock(&nfs_manager_mutex);
	uint64_t mask = 0;
	for(int i = 0; i < requiredCores; i++)
	{
		mask |= cores[nextCore];
		nextCore = (nextCore+1)%cores.size();
	}
	pthread_mutex_unlock(&nfs_manager_mutex);

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "The NF requires %d cores. Its core mask is  \"%x\"",requiredCores,mask);

	return mask;
}

void ComputeController::printInfo(int graph_id)
{
	for(map<string, NF*>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
	{
		nf_t type = nf->second->getSelectedDescription()->getNFType();
		string str = NFType::toString(type);
		if(graph_id == 2)
			coloredLogger(ANSI_COLOR_BLUE,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tType: %s\t-\tStatus: %s",nf->first.c_str(),(nf->first.length()<=7)? "\t" : "", str.c_str(),(nf->second->getRunning())?"running":"stopped");
		else if(graph_id == 3)
			coloredLogger(ANSI_COLOR_RED,ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tType: %s\t-\tStatus: %s",nf->first.c_str(),(nf->first.length()<=7)? "\t" : "", str.c_str(),(nf->second->getRunning())?"running":"stopped");
		else
			logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "\t\tName: '%s'%s\t-\tType: %s\t-\tStatus: %s",nf->first.c_str(),(nf->first.length()<=7)? "\t" : "", str.c_str(),(nf->second->getRunning())?"running":"stopped");
	}
}

#ifdef UNIFY_NFFG	
nf_manager_ret_t ComputeController::retrieveAllAvailableNFs()
{
	set<string> NFsNames;

	try
 	{
 		string translation;

 		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Retrieving information on all the available VNFs");

		char ErrBuf[BUFFER_SIZE];
		struct addrinfo Hints;
		struct addrinfo *AddrInfo;
		int socket;				// keeps the socket ID for this connection
		int WrittenBytes;			// Number of bytes written on the socket
		int ReadBytes;				// Number of bytes received from the socket
		char DataBuffer[DATA_BUFFER_SIZE];	// Buffer containing data received from the socket

		memset(&Hints, 0, sizeof(struct addrinfo));

		Hints.ai_family= AF_INET;
		Hints.ai_socktype= SOCK_STREAM;

		if (sock_initaddress (NAME_RESOLVER_ADDRESS, NAME_RESOLVER_PORT, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error resolving given address/port (%s/%s): %s",  NAME_RESOLVER_ADDRESS, NAME_RESOLVER_PORT, ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		stringstream tmp;
		tmp << "GET " << NAME_RESOLVER_BASE_URL << NAME_RESOLVER_DIGEST_URL << " HTTP/1.1\r\n";
		tmp << "Host: :" << NAME_RESOLVER_ADDRESS << ":" << NAME_RESOLVER_PORT << "\r\n";
		tmp << "Connection: close\r\n";
		tmp << "Accept: */*\r\n\r\n";
		string message = tmp.str();

		char command[message.size()+1];
		command[message.size()]=0;
		memcpy(command,message.c_str(),message.size());

		if ( (socket= sock_open(AddrInfo, 0, 0,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
		{
			// AddrInfo is no longer required
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot contact the name resolver at \"%s:%s\"", NAME_RESOLVER_ADDRESS, NAME_RESOLVER_PORT);
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "%s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		WrittenBytes = sock_send(socket, command, strlen(command), ErrBuf, sizeof(ErrBuf));
		if (WrittenBytes == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error sending data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;

		}

		ReadBytes= sock_recv(socket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (ReadBytes == sockFAILURE)
		{
			logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error reading data: %s", ErrBuf);
			return NFManager_SERVER_ERROR;
		}

		// Terminate buffer, just for printing purposes
		// Warning: this can originate a buffer overflow
		DataBuffer[ReadBytes]= 0;

		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Data received: ");
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",DataBuffer);

		shutdown(socket,SHUT_WR);
		sock_close(socket,ErrBuf,sizeof(ErrBuf));

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_METHOD_NOT_ALLLOWED,3) == 0)
			return NFManager_NO_NF;

		if(strncmp(&DataBuffer[CODE_POSITION],CODE_OK,3) != 0)
			return NFManager_SERVER_ERROR;

		//the HTTP headers must be removed
		int i = 0;
		for(; i < ReadBytes; i++)
		{
			if((i+4) <= ReadBytes)
			{
				if((DataBuffer[i] == '\r') && (DataBuffer[i+1] == '\n') && (DataBuffer[i+2] == '\r') && (DataBuffer[i+3] == '\n'))
				{
					i += 4;
					break;
				}
			}
		}

		translation.assign(&DataBuffer[i]);
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Information on NFs:");
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "%s",translation.c_str());

		//Parse the answer		
		try
		{
			Value value;
			read(translation, value);
			Object obj = value.getObject();

			bool foundNFs = false;

			for( Object::const_iterator i = obj.begin(); i != obj.end(); ++i )
			{
		 	    const string& name  = i->first;
				const Value&  value = i->second;

				if(name == "network-functions")
				{
					foundNFs = true;
					
					const Array& names_array = value.getArray();
					if(names_array.size() == 0)
					{
						logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"network-functions\" without descriptions");
						return NFManager_NO_NF;
					}

					//Itearate on the VNFs
					for( unsigned int i = 0; i < names_array.size(); ++i)
					{
						//This is an implementation, with a type and an URI
						Object nf_name = names_array[i].getObject();
						
						for( Object::const_iterator nfn = nf_name.begin(); nfn != nf_name.end(); ++nfn )
						{
							const string& the_name  = nfn->first;
							const Value&  the_value = nfn->second;
												
							if(the_name == "name")
							{
								string vnfName = the_value.getString();
								NFsNames.insert(vnfName);
							}
							else
							{
								logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\"",the_name.c_str());
								return NFManager_SERVER_ERROR;
							}
						}
					}

				}//end if(name == "network-functions")
				else
				{
					logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Invalid key \"%s\"",name.c_str());
					return NFManager_SERVER_ERROR;
				}
			}
			if(!foundNFs)
			{
				logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "Key \"network-functions\" not found in the answer");
				return NFManager_SERVER_ERROR;
			}
		}catch(...)
		{
			logger(ORCH_WARNING, MODULE_NAME, __FILE__, __LINE__, "The content does not respect the JSON syntax");
			return NFManager_SERVER_ERROR;
		}
 	}
	catch (std::exception& e)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Exception: %s",e.what());
		return NFManager_SERVER_ERROR;
	}

	//For each VNF, download its description
	ComputeController tmpManager;
	for(set<string>::iterator name = NFsNames.begin(); name != NFsNames.end(); name++)
	{
		nf_manager_ret_t retval = tmpManager.retrieveDescription(*name);
		if(retval !=NFManager_OK)
			return retval;
	}
	//Provide the dfescriptions to the virtualizer
	set<NF*> vnfs;
	for(map<string, NF*>::iterator it = tmpManager.nfs.begin(); it != tmpManager.nfs.end(); it++)
		vnfs.insert(it->second);

	Virtualizer::addSupportedVNFs(vnfs);

	return NFManager_OK;
}
#endif
