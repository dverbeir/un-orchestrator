#include <string.h>

#include "implementation.h"

Implementation* Implementation::create(const char* type, xmlNodePtr xmlDetails)
{
	if(strcmp(type, "dpdk") == 0) {
		return new DPDKImplementation(DPDK, xmlDetails);
	}
	else if(strcmp(type, "docker") == 0) {
		return new DockerImplementation(DOCKER, xmlDetails);
	}
	else if(strcmp(type, "kvm") == 0) {
		return new KVMImplementation(KVM, xmlDetails);
	}
	return NULL;
}

Implementation::Implementation(nf_t type, xmlNodePtr xmlDetails) :
	type(type)
{
	xmlChar* attr_uri = xmlGetProp(xmlDetails, (const xmlChar*)URI_ATTRIBUTE);
	if (attr_uri == NULL)
		throw string("Missing URI for NF implementation");

	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\tURI:%s", attr_uri);

	uri = (const char*)attr_uri;
}

void Implementation::toJSON(Object& impl)
{
	impl["uri"]  = uri;
}

DPDKImplementation::DPDKImplementation(nf_t type, xmlNodePtr xmlDetails) : Implementation(type, xmlDetails)
{
	xmlChar* attr_cores = xmlGetProp(xmlDetails, (const xmlChar*)CORES_ATTRIBUTE);
	xmlChar* attr_location = xmlGetProp(xmlDetails, (const xmlChar*)LOCATION_ATTRIBUTE);

	//the attributes "cores" and "location" must be present
	if (attr_cores == NULL)
		throw string("DPDK implementation missing 'cores' attribute");
	if (attr_location == NULL)
		throw string("DPDK implementation missing 'location' attribute");

	cores = (char *)attr_cores;
	location = (char *)attr_location;

	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t\t\tcores: %s", cores.c_str());
	logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "\t\t\tlocation: %s", location.c_str());
}

void DPDKImplementation::toJSON(Object& impl)
{
	Implementation::toJSON(impl);

	impl["type"] = "dpdk";

	impl["cores"] = cores;
	impl["location"] = location;
}

DockerImplementation::DockerImplementation(nf_t type, xmlNodePtr xmlDetails) : Implementation(type, xmlDetails)
{
}

void DockerImplementation::toJSON(Object& impl)
{
	Implementation::toJSON(impl);

	impl["type"] = "docker";
}

KVMImplementation::KVMImplementation(nf_t type, xmlNodePtr xmlDetails) : Implementation(type, xmlDetails)
{
}

void KVMImplementation::toJSON(Object& impl)
{
	Implementation::toJSON(impl);

	impl["type"] = "kvm";
}
