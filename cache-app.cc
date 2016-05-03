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
        
	CommandLine cmd;

	//Topology path
        cmd.AddValue("tp", "Topology path", tp);

	//Seed
        cmd.AddValue("seed", "Seed", seed);

	//Group size
        cmd.AddValue("gs", "Group size", gs);

	//Cache placement 0/1/2 access-nodes/betweenness/all-nodes
        cmd.AddValue("cpl", "Cache placement", cpl);

	//caching policy: 0 for no cache, 1 for packet_level, 2 for object level
        cmd.AddValue("cpo", "Cache policy", cpo);

	cmd.Parse (argc, argv);

	ExperimentGlobals::RANDOM_VAR =CreateObject<UniformRandomVariable>();

	uint8_t arg =1;
	//--------------------------------------------------
	//--------------------------------------------------
	//--------------------------------------------------
	//seed = std::atoi(argv[arg++]);
	RngSeedManager::SetSeed (seed);
	//--------------------------------------------------
	//--------------------------------------------------
	//gs=std::atoi(argv[arg++]);
	//--------------------------------------------------
	//--------------------------------------------------
	ExperimentGlobals::CACHE_PLACEMENT = cpl;
	char caching = cpo;
	uint32_t cache_cap=0;
	uint32_t fast_cap=0;
	if (caching!=0) {
		cache_cap=std::atoi(argv[arg++]);
        if (argc>arg)
            fast_cap=std::atoi(argv[arg++]);
	}
	
	//print experiment info
	std::cout<<"Topology: "<<tp<<std::endl;
	std::cout<<"Seed: "<<seed<<std::endl;
	std::cout<<"Group size: "<<(uint16_t)gs<<std::endl;
	std::cout<<"Cache placement: "<<(uint16_t)ExperimentGlobals::CACHE_PLACEMENT<<std::endl;
	std::cout<<"Caching policy: "<<(unsigned)caching<<" \nCache capacity (DRAM-SRAM): "<<cache_cap<<"-"<<fast_cap<<std::endl;	

	Ptr<BootstrappingHelper> bh=CreateObject<BootstrappingHelper>(tp, "/tmp/", gs, seed,
                                                                      caching, cache_cap, fast_cap);
	bh->parseTopology(gs);//ftiaxnei ccnModules kai nodes
	bh->startExperiment();

	std::cout<<"Application ending."<<std::endl;
	Simulator::Destroy();
	return 0;
}
