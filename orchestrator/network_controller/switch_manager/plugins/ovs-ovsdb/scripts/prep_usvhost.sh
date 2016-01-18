#!/bin/bash

#Author: UNIFY Consortium
#Date: 2015-01-18
#Brief: Prepare for a usvhost port creation

#$1 full port name as will be created in OVS

socket_path="/usr/local/var/run/openvswitch/"

port="$1"

if (( $EUID != 0 ))
then
    echo "[$0] This script must be executed with ROOT privileges"
    exit 0
fi

echo "Cleaning up socket for USVHOST port $port"

rm -f "$socket_path/$port"
exit 1
