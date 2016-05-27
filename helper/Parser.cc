#include "Parser.h"

using std::ifstream;
using std::istringstream;
using std::stringstream;
using std::getline;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Parser");


Parser::Parser() {
	matrix_map = map<uint32_t, set<uint32_t> >(); // Node id, set of nodes which is connected
	idToNode = map<uint32_t, Ptr<Node> >();
	nodeToId = map<uint32_t, uint32_t>();
}

Parser::~Parser() {
	//TODO implement correctly
}

void Parser::DoDispose(void) {
	idToNode.clear();
}

Ptr<Node> Parser::getNodeById(uint32_t nodeid) {
	return idToNode[nodeid];
}

uint32_t Parser::findId(Ptr<Node> node) {
	return nodeToId[node->GetId()];
}

vector<Ptr<Node> > Parser::getNeighbors(uint32_t graphNodeId) {

	set < uint32_t > neighbors = matrix_map[graphNodeId];
	vector < Ptr<Node> > v;
	set<uint32_t>::iterator iter;
	for (iter = neighbors.begin(); iter != neighbors.end(); iter++) {
		uint32_t n_id = *iter;
		Ptr < Node > nodePtr = idToNode[n_id];
		v.push_back(nodePtr);
	}
	return v;
}

/*
 * parses topology file (usualy placed in ./ccncaching/topology dir)
 * and updates some maps that contain vertexes and their associates
 */
void Parser::parse(string& filepath, uint8_t group_size) {
	matrix_map.clear();

	ifstream f;
	string line;
	f.open(filepath.c_str());
	uint16_t num_of_access_nodes=0;
	
	NS_LOG_INFO("Parsing topology: "<<filepath);

    // read topology file and update neighbor matrices
    NS_ASSERT_MSG(f.is_open(), "Could not open file "<<filepath);
	
		// for all vertex connections
	while (getline(f, line)) {
		bool hasBetw = false;
		bool hasNeighbors = false;
		vector <string> tokens;
		
		size_t found = line.find("_");
		if (found!=string::npos)
			hasBetw = true;
		size_t found2 = line.find("<");
		if 	(found2!=string::npos)
			hasNeighbors = true;
		else
			continue;
		
		double betw=0;
		uint16_t sourceNode;
		if (hasBetw){
			NS_ASSERT_MSG( line.find("_", found+1)!=string::npos, "Invalid betweenness markers at line "<<line);
			betw = std::atof(line.substr(found+1, line.find("_", found+1)-found-1).c_str());
			sourceNode = atoi(line.substr(0, found-1).c_str());
			line = line.substr(line.find("_", found+1)+1);
			}
		else if (hasNeighbors){
			sourceNode = atoi(line.substr(0, found2-1).c_str());
			line = line.substr(found2+1);	
			}
		 
		
		graph_node_id_to_betw[sourceNode] = betw;
		NS_LOG_INFO("Node "<<sourceNode<<" has betweenneess "<<betw);
		while (line.find(">")!=string::npos){
			tokens.push_back(line.substr(line.find("<")+1, line.find(">")-(line.find("<")+1)));
			line = line.substr(line.find(">")+1);
		}
		//calculate access nodes
		if (tokens.size()==1){
			num_of_access_nodes++;
		}
			
		NS_ASSERT_MSG(tokens.size()!=0, "Node "<<sourceNode<<" is isolated, remove it from topology file.");
			
		set < uint32_t > neighbors;
		for (uint32_t i = 0; i < tokens.size(); i++) {
			uint32_t neighbor = atoi(tokens[i].c_str());
			matrix_map[sourceNode].insert(neighbor);
			matrix_map[neighbor].insert(sourceNode);
			NS_LOG_INFO("Added Graph Ids "<<sourceNode<<"->"<<neighbor);
		}
			
	}
	NS_LOG_INFO("Access nodes: "<<num_of_access_nodes);

	// create a NS-3 node object for each vertex
	NodeContainer n;
	for (map<uint32_t, set<uint32_t> >::iterator iter = matrix_map.begin();
			iter != matrix_map.end(); iter++) {
		uint32_t topology_id = iter->first;
		Ptr < Node > nodePtr = CreateObject<Node>(topology_id);
		n.Add(nodePtr);
		all_nodes.push_back(nodePtr);
		idToNode[topology_id] = nodePtr;
		nodeToId[nodePtr->GetId()] = topology_id;
		NS_LOG_DEBUG("Created node with ns-3 id:"<<nodePtr->GetId()<<", graph id:"<<topology_id);
	}

	//create NS-3 links between the Nodes
	set < uint32_t > nodes;
	map<uint32_t, map<uint32_t, uint32_t> > alreadyConnected;
	for (map<uint32_t, set<uint32_t> >::iterator iter = matrix_map.begin();
			iter != matrix_map.end(); iter++) {
		Ptr < Node > sourceNode = idToNode[iter->first];
		vector < Ptr<Node> > neighbors = getNeighbors(iter->first);
		for (vector<Ptr<Node> >::iterator neighborsIter = neighbors.begin();
				neighborsIter != neighbors.end(); neighborsIter++) {
			uint32_t sourceNodeGraphId = nodeToId[sourceNode->GetId()];
			uint32_t neighborNodeGraphId = nodeToId[(*neighborsIter)->GetId()];

			if (alreadyConnected[sourceNodeGraphId][neighborNodeGraphId] == 1)
				continue;

			NodeContainer n;
			n.Add(sourceNode);
			n.Add(*neighborsIter);

			PointToPointHelper pph;
			pph.SetQueue(string("ns3::DropTailQueue"), string("MaxPackets"),
					ns3::UintegerValue(10000), string("MaxBytes"),
					ns3::UintegerValue(10000));
			pph.SetDeviceAttribute("DataRate", StringValue(LINK_CAPACITY));
			pph.SetChannelAttribute("Delay", StringValue(LINK_DELAY));
			NetDeviceContainer ndc = pph.Install(n);

			nodes.insert(sourceNodeGraphId);
			nodes.insert(neighborNodeGraphId);

			alreadyConnected[sourceNodeGraphId][neighborNodeGraphId] = 1;
			alreadyConnected[neighborNodeGraphId][sourceNodeGraphId] = 1;
			
			NS_LOG_DEBUG("Added ns3 link "<<sourceNodeGraphId<<"<->"<<neighborNodeGraphId);
		}
	}

	/* 
     * we create group of users connecting to the access_nodes (those with degree=1)
	 * IMPROVE: create a method "create_link"
	 */	
	uint8_t limit=255;
	uint16_t sender_position = ExperimentGlobals::RANDOM_VAR->GetInteger(0,num_of_access_nodes-1)%limit;
	map<uint32_t, set<uint32_t> > matrix_map_tmp = matrix_map;
	uint32_t host_node_name = matrix_map.size()-1;
	uint8_t added_groups = 0;

	for (map<uint32_t, set<uint32_t> >::iterator iter = matrix_map.begin();
			iter != matrix_map.end(); iter++) {
		vector < Ptr<Node> > neighbors = getNeighbors(iter->first);
		if (neighbors.size() == 1) {//if they are access nodes
			if (added_groups==sender_position) {
				NS_LOG_INFO("Added sender at position "<<sender_position<<" ("<<iter->first <<")");
				sender_id = idToNode[iter->first]->GetId();
				added_groups++;
				continue;
			}
			
			if (added_groups++==limit)
				break;
			
			std::ostringstream message;
			message << "Group " <<(unsigned)added_groups<<" (Access node: "<< iter->first<<"): ";
			Ptr < Node > sourceNode = getNodeById(iter->first);
			uint32_t sourceNodeGraphId = nodeToId[sourceNode->GetId()];

			for (unsigned gs = 0; gs < group_size; gs++) {
				// add the new host node
				Ptr < Node > newNode = CreateObject<Node>();
				n.Add(newNode);

				idToNode[++host_node_name] = newNode;
				nodeToId[newNode->GetId()] = host_node_name;

				NodeContainer n;
				n.Add(sourceNode);
				n.Add(newNode);

				// add the new link
				PointToPointHelper pph;
				pph.SetQueue(string("ns3::DropTailQueue"), string("MaxPackets"),
						ns3::UintegerValue(10000), string("MaxBytes"),
						ns3::UintegerValue(10000));
				pph.SetDeviceAttribute("DataRate", StringValue(LINK_CAPACITY));
				pph.SetChannelAttribute("Delay", StringValue(LINK_DELAY));
				NetDeviceContainer ndc = pph.Install(n);

				nodes.insert(host_node_name);

				alreadyConnected[sourceNodeGraphId][host_node_name] = 1;
				alreadyConnected[host_node_name][sourceNodeGraphId] = 1;

				message << host_node_name<<", ";
				// update the temporary matrix map
				matrix_map_tmp[iter->first].insert(host_node_name);
				matrix_map_tmp[host_node_name].insert(iter->first);
				
				active_nodes.push_back(newNode->GetId());
			}
			
			NS_LOG_INFO(message.str());
			message.clear();
		}
	}
	matrix_map = matrix_map_tmp;
	topology.setGraph(matrix_map);
	
}

Graph Parser::getGraph() {
	return topology;
}

}