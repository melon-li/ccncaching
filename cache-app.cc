#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/BootstrappingHelper.h"


using namespace ns3;

int main(int argc ,char *argv[])
{
        string tp;
        uint16_t seed;
        uint16_t gs;
        uint16_t cpl;
        uint16_t cpo;
	uint64_t cache_cap = 0;
	uint64_t fast_cap = 0;
        
	double cache_size = 0;
	double fast_size = 0;
        
        double fp;
        bool enable_opt = false;
	CommandLine cmd;

	//Topology path
        cmd.AddValue("tp", "Topology path", tp);

	//Seed
        cmd.AddValue("seed", "Seed", seed);

	//Group size
        cmd.AddValue("gs", "Group size", gs);

	/*Cache placement 0 access-nodes
          >0 all: 1 betweenness. !=1 all-nodes*/
        cmd.AddValue("cpl", "The Cache placement(0: caching at edge nodes only, >0: betweenness or NDN)", cpl);

	//caching policy: 0 for no cache, 1 for packet_level, 2 for object level
        cmd.AddValue("cpo", "The Cache policy(0:none, 1:LRU, 2:OPC, 3:HCaching, 4:DRAM-SSD)", cpo);

        //DRAM capacity, S
        cmd.AddValue("dc", "Total DRAM capacity(uint64_t Bytes)", cache_cap);
        cmd.AddValue("sc", "Total SRAM capacity(uint64_t Bytes)", fast_cap);

 
        cmd.AddValue("ds", 
        "Total DRAM size(double GB. if the cpo = 3, the actual DRAM size is twice over this value ),prior than dc.", cache_size);
        cmd.AddValue("ss", "Total SRAM size(double MB), prior than sc.", fast_size);

        cmd.AddValue("fp", "The desired false-positive rate", fp);
        cmd.AddValue("opt", "Bool value,true: enable optimized reading cache. Otherwise, disable", enable_opt);

	cmd.Parse (argc, argv);

	ExperimentGlobals::RANDOM_VAR =CreateObject<UniformRandomVariable>();

	uint8_t arg =1;
	RngSeedManager::SetSeed (seed);
	ExperimentGlobals::CACHE_PLACEMENT = cpl;
	char caching = cpo;
        if(cache_size) cache_cap = uint64_t(cache_size*1024*1024*1024);
        if(fast_size) fast_cap = uint64_t(fast_size*1024*1024);

	//print experiment info
	NS_LOG_UNCOND("Topology: "<<tp);
	NS_LOG_UNCOND("Seed: "<<seed);
	NS_LOG_UNCOND("Group size: "<<(uint16_t)gs);
	NS_LOG_UNCOND("Cache placement: "<<(uint16_t)ExperimentGlobals::CACHE_PLACEMENT);
	NS_LOG_UNCOND("Caching policy: "<<(unsigned)caching<<" \nCache capacity DRAM = "<<cache_cap<<
                   " bytes, SRAM = "<<fast_cap<<" bytes");	
        if(enable_opt){
            NS_LOG_UNCOND("enable_opt = true");
        }else{
            NS_LOG_UNCOND("enable_opt = false");

        }

	Ptr<BootstrappingHelper> bh = 
        CreateObject<BootstrappingHelper>(tp, "/tmp/", gs, seed, 
            std::make_pair(std::make_pair(caching, enable_opt), fp),
            cache_cap, fast_cap);
	bh->parseTopology(gs);//ftiaxnei ccnModules kai nodes
	bh->startExperiment();

	std::cout<<"Application ending."<<std::endl;
	Simulator::Destroy();
	return 0;
}
