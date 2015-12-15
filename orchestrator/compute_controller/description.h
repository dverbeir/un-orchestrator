#ifndef DESCRIPTION_H_
#define DESCRIPTION_H_ 1

#include "nf_type.h"
#include "nfs_manager.h"

#pragma once

#include "../utils/logger.h"
#include "../utils/constants.h"

#include <string>
#include <vector>
#include <assert.h>

using namespace std;

class NFsManager;

enum PortType {
	UNDEFINED_PORT,
	USVHOST_PORT,
	IVSHMEM_PORT,
};

PortType portTypeFromString(const std::string& s);


class Description
{
friend NFsManager;

public:

private:
	nf_t type;
	string uri;
	//The next attribute are meningful only for DPDK VNFs
	//FIXME: this is bad.. The same description should be valid for all the NFs. Then, it is up to the proper
	//plugin to decide wheter an information has to be used or not.
	string cores;
	string location;
	std::vector<PortType> port_types;
	
public:
	Description(nf_t type, string uri, string cores, string location, std::vector<PortType>& port_types);
	Description(string type, string uri, string cores, string location, std::vector<PortType>& port_types);
	
	string getURI();
	string getLocation();
	nf_t getType();
	const std::vector<PortType>& getPortTypes() { return port_types; }
	
protected:
	string getCores();
};

#endif //DESCRIPTION_H_
