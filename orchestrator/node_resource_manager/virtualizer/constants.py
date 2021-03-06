'''
	File used by the orchestrator to maintain the state (i.e., rules deployed,
	VNF instantiated
'''

#TMP file use by the orchestrator to maintain the current configuration of the node
CONFIGURATION_FILE = './node_resource_manager/virtualizer/.universalnode.xml'

#TMP file used by the orchestrator and representing the deployed graph,
#in the JSON syntax internally used by the orchestrator itself
GRAPH_FILE = './node_resource_manager/virtualizer/.graph.json'

#File containing the new piece of graph to be deployed, in the JSON
#syntax internally used by the orchestrator
NEW_GRAPH_FILE = './node_resource_manager/virtualizer/.new_graph.json'
#File containing the IDs of the rules to be removed from the graph
REMOVE_GRAPH_FILE = './node_resource_manager/virtualizer/.remove_graph.json'

'''
	Information to be exported
'''
INFRASTRUCTURE_NAME = 'Single node'
INFRASTRUCTURE_ID = 'UUID001'
NODE_NAME = 'Universal Node'
NODE_ID = 'UUID11'
NODE_TYPE = 'BisBis'

'''
	Supported matches.
	For each key, it indicates the equivalent match in the NF-FG supported natively
	by the node orchestrator.
'''
supported_matches = {
	"dl_vlan" : "vlan_id"
}

'''
	Supported actions.
	For each key, it indicates the number of parameters required (0 or 1).
'''
#XXX: for each key, there must be an handler for that key in actionsParser.py
supported_actions = {
	"strip_vlan" : 0,
	"push_vlan" : 1
}

'''
	Equivalent actions.
	For each key, it indicates the equivalent action in the NF-FG supported natively
	by the node orchestrator.
'''
#XXX: for each key, there must be an handler for that key in actionsParser.py
equivalent_actions = {
	"strip_vlan" : "vlan",
	"push_vlan" : "vlan"
}

