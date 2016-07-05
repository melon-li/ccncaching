#ifndef PARSER_H_
#define PARSER_H_


#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/Graph.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/utils.h"
#include "ns3/experiment_globals.h"


using std::vector;
using std::map;
using std::set;
using std::string;


namespace  ns3 {

class Parser : public Object
{
        map<uint32_t, set<uint32_t> > matrix_map;
        // used also in boostrapping for creating ccnmodules

        vector <uint32_t> active_nodes;
        uint32_t sender_id;
        
        map<uint32_t, double> graph_node_id_to_betw;
        
        vector <Ptr <Node> > all_nodes;
        
    public:
    
        Parser();
        ~Parser();
        virtual void DoDispose(void);
        // should make these private, but they are used a lot in Bootstrapper
        map<uint32_t, Ptr<Node> > idToNode;
        map<uint32_t, uint32_t > nodeToId;
        
        double getNode_Betw(const uint32_t& node_graph_id ){
            return graph_node_id_to_betw[node_graph_id];
        }


        Ptr<Node> getNodeById(uint32_t);
        uint32_t findId(Ptr<Node>);
        vector<Ptr<Node> > getNeighbors(uint32_t);
        void parse(string &, uint32_t group_size);
        Graph topology;
        Graph getGraph();
        
        vector <uint32_t> getActiveNodes(){
            return active_nodes;
        }
        
        uint32_t getSenderId(){
            return sender_id;
        }
            
        vector <Ptr <Node> > get_all_nodes(){
            return all_nodes;
            }
};

}  // namespace  ns3
#endif /* PARSER_H_ */
