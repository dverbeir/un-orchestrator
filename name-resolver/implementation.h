#ifndef IMPLEMENTATION_H_
#define IMPLEMENTATION_H_ 1

#pragma once

#include <string>
#include <assert.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>

#include "constants.h"

using namespace std;
using namespace json_spirit;

typedef enum { DPDK, DOCKER, KVM } nf_t;

class Implementation
{
protected:
	nf_t type;
	string uri;
	
public:
	static Implementation* create(const char* type, xmlNodePtr xmlDetails);

protected:
	Implementation(nf_t type, xmlNodePtr xmlDetails);

public:
	virtual void toJSON(Object& impl);
};

class DPDKImplementation : public Implementation
{
public:
	DPDKImplementation(nf_t type, xmlNodePtr xmlDetails);

	virtual void toJSON(Object& impl);

private:
	string cores;
	string location;
};

class KVMImplementation : public Implementation
{
public:
	KVMImplementation(nf_t type, xmlNodePtr xmlDetails);

	virtual void toJSON(Object& impl);

private:
};

class DockerImplementation : public Implementation
{
public:
	DockerImplementation(nf_t type, xmlNodePtr xmlDetails);

	virtual void toJSON(Object& impl);

private:
};

#endif //IMPLEMENTATION_H_
