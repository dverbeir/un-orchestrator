#include "nf.h"


NF::NF(string name, int nports, string summary) :
	name(name),nports(nports),summary(summary)
{

}

void NF::addImplementation(Implementation *implementation)
{
	implementations.push_back(implementation);
}

string NF::getName()
{
	return name;
}

Object NF::toJSON()
{
	Object nf;	
	
	nf["name"]  = name;
	nf["nports"]  = nports;
	nf["summary"] = summary;
	
	Array impl_ary;
	for(list<Implementation*>::iterator i = implementations.begin(); i != implementations.end();i++)
	{
		Object impl;
		(*i)->toJSON(impl);
		impl_ary.push_back(impl);
	}
	
	nf["implementations"] = impl_ary;
	
	return nf;
}
