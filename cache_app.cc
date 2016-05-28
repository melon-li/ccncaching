#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/BootstrappingHelper.h"


using namespace ns3;

int main(int argc ,char *argv[])
{
    CommandLine c;
    c.Parse (argc, argv);

    ExperimentGlobals::RANDOM_VAR =CreateObject<UniformRandomVariable>();

    uint8_t arg =1;
    //Topology path
    //--------------------------------------------------
    string topology = *(new std::string(argv[arg++]));
    //--------------------------------------------------
    //Seed
    //--------------------------------------------------
    uint16_t seed = std::atoi(argv[arg++]);
    RngSeedManager::SetSeed (seed);
    //--------------------------------------------------
    //Group size
    //--------------------------------------------------
    uint16_t gs=std::atoi(argv[arg++]);
    //--------------------------------------------------
    //Cache placement 0/1/2 access-nodes/betweenness/all-nodes
    //--------------------------------------------------
    ExperimentGlobals::CACHE_PLACEMENT = std::atoi(argv[arg++]);
    //caching policy: 0 for no cache, 1 for packet_level, 2 for object level
    char caching=std::atoi(argv[arg++]);
    uint32_t cache_cap=0;
    uint32_t fast_cap=0;
    if (caching!=0) {
        cache_cap=std::atoi(argv[arg++]);
        if (argc>arg)
            fast_cap=std::atoi(argv[arg++]);
    }
    
    //print experiment info
    std::cout<<"Topology: "<<topology<<std::endl;
    std::cout<<"Seed: "<<seed<<std::endl;
    std::cout<<"Group size: "<<(uint16_t)gs<<std::endl;
    std::cout<<"Cache placement: "<<(uint16_t)ExperimentGlobals::CACHE_PLACEMENT<<std::endl;
    std::cout<<"Caching policy: "<<(unsigned)caching<<" \nCache capacity (DRAM-SRAM): "<<cache_cap<<"-"<<fast_cap<<std::endl;    

    Ptr<BootstrappingHelper> bh=CreateObject<BootstrappingHelper>(topology,"/tmp/",gs,std::atoi(argv[3]), caching, cache_cap, fast_cap);
    bh->parseTopology(gs);//ftiaxnei ccnModules kai nodes
    bh->startExperiment();

    std::cout<<"Application ending."<<std::endl;
    Simulator::Destroy();
    return 0;
}
