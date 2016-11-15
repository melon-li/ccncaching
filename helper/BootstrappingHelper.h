
#ifndef BOOTSTRAPPINGHELPER_H_
#define BOOTSTRAPPINGHELPER_H_

#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>    // std::random_shuffle
#include <vector>
#include <ctime>
#include <cstdlib>
#include <stdlib.h>     /* srand, rand */
//#include "ns3/mod-module.h"
#include "ns3/CcnModule.h"
#include "ns3/Initializer.h"
#include "ns3/Receiver.h"
#include "ns3/Sender.h"
#include "ns3/ResultPrinter.h"


using std::string;
using std::vector;
using std::set;

namespace ns3
{
class Text;
class Initializer;
class CCN_Name;

class BootstrappingHelper : public Object
{
    bool finished;
    public:

    Ptr<Parser> parser;
    map <uint32_t , Ptr < CcnModule > > nsNodeIdToModule;

    Ptr<CCN_Name> name2;
    Ptr<Initializer> initializer;
    Ptr<NetDevice> ndfinder(Ptr<Node> n1,Ptr<Node> n2);
    vector< Ptr < Object > > vec3;
    
    void PITCheck(int ,int , vector<uint32_t>,Graph,uint32_t);
    uint32_t group_size;
    uint32_t seed;
    string seedString;
    string output;
    void specificData();
    std::string filename;

    int length;

    BootstrappingHelper(string filename, string output, uint32_t gsize, uint32_t seed,
                        pair<pair<char,bool>, double> _cache_mode, 
                        uint64_t _caching_cap, uint64_t _fast_cap);
    
    char cache_mode;
    uint64_t cache_cap;
    uint64_t fast_cap;
    double   fp;
    bool enable_opt;
    
    ~BootstrappingHelper();
    void parseTopology( uint32_t group_size);

    void startExperiment();
    
    void cache_check(vector < Ptr< Node > > cache_nodes);

    
};
}


#endif
