# Universal Node Repository Summary
This repository contains the current implementation of the Universal Node and is divided in three sub-portions.
Please check individual README's in each sub-package.

## Universal Node orchestrator
The Universal Node orchestrator (un-orchestrator) is a module
that, given a Network Function - Forwarding Graph (NF-FG), deploys it on
the current physical server. To this purpose, it interacts with a virtual
switch in order to configure the paths among the NFs, and with an hypervisor
in order to properly start the required NFs.

## Name Resolver
The Name Resolver is a module that returns a set of implementations for a
given NF. It is exploited by the un-orchestrator each time that a NF must
be started in order to translate the 'abstract' name into the proper
suitable software image.

## NFs
This folder contains some examples of virtual network functions.
