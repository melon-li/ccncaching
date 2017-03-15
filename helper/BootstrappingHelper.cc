#include "BootstrappingHelper.h"


using std::ofstream;
using std::stringstream;
using std::vector;
using std::string;
using std::set;
using std::map;
using std::endl;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Bootstrapper");

BootstrappingHelper::BootstrappingHelper(string filename,string output,uint32_t gsize, uint32_t seed, 
                                         pair<pair<char,bool>, double> _cache_mode, 
                                         uint64_t _caching_cap, uint64_t _fast_cap)
{
    NS_LOG_INFO("Constructing BootstrappingHelper");
    this->seed=seed;
    this->filename=filename;
    this->output=output;
    this->group_size=gsize;
    cache_cap = _caching_cap;
    fast_cap = _fast_cap;
    cache_mode = _cache_mode.first.first;
    enable_opt =  _cache_mode.first.second;
    fp = _cache_mode.second;
    if(fp == 0) fp = 0.001;
    ExperimentGlobals::RANDOM_VAR = CreateObject<UniformRandomVariable>();
    RngSeedManager::SetSeed (seed);
    finished = false;
}

BootstrappingHelper::~BootstrappingHelper()
{
    NS_LOG_INFO("Destructing BootstrappingHelper (not updated..)");
    parser=0;
    vector<Ptr<CcnModule> >::iterator iter;
    initializer=0;
}

/**
 * @nodes_size is the number of host nodes that will be inserted in each access node
 * 
 * Intially reads topology file and creates NS-3 network, then adds a CCN_module at each
 * NS-3 network node
 */
void BootstrappingHelper::parseTopology(uint32_t nodes_size)
{
    parser=CreateObject<Parser>();
    parser->parse(filename, nodes_size); // creates the NS-3 network (nodes and links)
    
    //ns3 nodeID <--> topologyID
    map<uint32_t, uint32_t> id_binder;
    // add a ccnmodule for every vertex and assign a unique ID
    for (map<uint32_t, Ptr<Node> >::iterator iter=parser->idToNode.begin(); iter!=parser->idToNode.end(); ++iter){
        NS_ASSERT_MSG(iter->second!=NULL, "FATAL ERROR, node "<<iter->first<<" is NULL");
        Ptr<CcnModule> m = CreateObject<CcnModule>(iter->second);
        m->setBetw(parser->getNode_Betw(iter->first));
        nsNodeIdToModule[iter->second->GetId()]=m;
        //std::cout<<"Created and added new CcnModule with id:"<<iter->second->GetId()<<" topology_id:"<<iter->first<<std::endl;
        id_binder[iter->second->GetId()]=iter->first;
        //std::cout<<"Inserted "<<iter->first<<" "<<iter->second->GetId()<<std::endl;
    }
    //here we set the neighbors of every CcnModule
    for (map<uint32_t, Ptr<CcnModule> >::iterator iter2=nsNodeIdToModule.begin(); iter2!=nsNodeIdToModule.end(); ++iter2 ){
        
        vector<Ptr<Node> > tmp_neighbors = parser->getNeighbors(id_binder[iter2->first]);
        //NS_LOG_INFO("Node "<<iter2->first<<" ("<<id_binder[iter2->first]<<") has "<<tmp_neighbors.size()<<" neighbors");

        for(unsigned j=0;j<tmp_neighbors.size();j++){
            Ptr<Node> n1=tmp_neighbors.at(j);//this is the neighbor
            Ptr<CcnModule> ccn_module=nsNodeIdToModule[n1->GetId()];//neighbor's module
            Ptr<NetDevice> net_device = ndfinder(n1, iter2->second->getNode());
            NS_ASSERT_MSG(net_device!=NULL,"Net_device "<<n1->GetId()<<"<->"<<iter2->second->getNode()->GetId()<<" about to be inserted in the map is null");
            NS_ASSERT_MSG(ccn_module!=NULL, "ccn_module about to be inserted in the map is null, id:"<<n1->GetId());
            iter2->second->setNeighbor(net_device, ccn_module);
            //NS_LOG_INFO("Added CcnModule link from "<<iter2->first<<" to "<<n1->GetId());
        }
    }
    NS_LOG_INFO("CCN nodes: "<<nsNodeIdToModule.size());
}

/**
 * epistrefei to net device tou deksiou me to opoio o deksis syndeetai ston aristero
 */
Ptr<NetDevice> BootstrappingHelper::ndfinder(Ptr<Node> n1,Ptr<Node> n2)
{
    //std::cout<<"Bootstrapping ndfinder"<<std::endl;
    if(n1==0) NS_LOG_WARN("In ndfinder in initializer n1 is null");

    if(n2==0) NS_LOG_WARN("In ndfinder in initializer n2 is null");

    for(unsigned i=0;i<n2->GetNDevices();i++)
    {
            if(n2->GetDevice(i)->GetChannel()->GetDevice(0)->GetNode()==n1)
                    return n2->GetDevice(i);

            if(n2->GetDevice(i)->GetChannel()->GetDevice(1)->GetNode()==n1)
                    return n2->GetDevice(i);
    }

    NS_LOG_INFO("Bootstrapping helper ndfinder returning 0");
    return 0;
}


/**
 * Starts the experiment
 * Topology is already created in method parseTopology()
 * */
void BootstrappingHelper::startExperiment(){
        uint8_t c=0;
    
        NS_LOG_INFO("Initializing experiment");
        Graph topology=parser->getGraph(); //load the topology. P is object of type Parser. Returns a Topology object
        string type_of_node;
        // cannot understand what is this for..            
        stringstream st;
        st << seed+c;
        this->seedString=st.str();

        // read receiver_nodes and 
        vector <uint32_t> receiver_nodes = parser->getActiveNodes(); 
        uint32_t publisher_id= parser->getSenderId();
    
        //receiver apps on the receiver nodes
        vector < Ptr< Receiver > > receiver_apps;
        for (vector<uint32_t>::iterator it=receiver_nodes.begin(); it!=receiver_nodes.end(); ++it){
            receiver_apps.push_back(CreateObject<Receiver>(nsNodeIdToModule[*it]));
        }
        Ptr<Sender> publisher_app=CreateObject<Sender>( nsNodeIdToModule[publisher_id] );

        NS_LOG_INFO("Initializing FIBs");
        //initialize FIBs, and Catalog
        this->initializer=CreateObject<Initializer>(parser, publisher_id, nsNodeIdToModule);
        map <string, uint32_t> file_map = this->initializer->parseFileMap();//updates the map of files <filename, filesize>
        this->initializer->initializeFIBs();//initialize forwarding rules for network routers
        this->initializer->initialize_FIBs_for_publisher_app(publisher_app);//initialize forwarding rules for publisher app

        NS_LOG_INFO("Initializing caches");
        //setup caching nodes
        vector < Ptr< Node > > cache_nodes;
        vector < Ptr< CcnModule > > cache_modules;
        if (cache_mode != 0){ //cache enable (0=no cache,1=LRU,2=OPC)
            if (ExperimentGlobals::CACHE_PLACEMENT == 0 ){ //caching at edge nodes only
                //start caching nodes
                for (vector<uint32_t>::iterator it=receiver_nodes.begin(); it!=receiver_nodes.end(); ++it){
                    Ptr <Node> access_node_i = parser->getNeighbors(*it)[0];
                    if (std::find(cache_nodes.begin(), cache_nodes.end(), access_node_i)==cache_nodes.end()){
                        cache_nodes.push_back(access_node_i); //there will be only one neighbor
                    
                        cache_modules.push_back(nsNodeIdToModule[access_node_i->GetId()]);
                        nsNodeIdToModule[access_node_i->GetId()]->enableCache(cache_mode, cache_cap, fast_cap, 
                                                                             &file_map, fp, enable_opt);
                        NS_LOG_INFO("Added cache app at "<< type_of_node<<" "<<  access_node_i->GetId()<<" mode:"<<(unsigned)cache_mode<<
                                " capacity:"<<cache_cap<<" betw "<<nsNodeIdToModule[access_node_i->GetId()]->getBetweenness());
                    }
                }
            }else if (ExperimentGlobals::CACHE_PLACEMENT > 0 ){/*either betw or NDN*/
                NS_LOG_INFO("Caching less for more");
                vector <Ptr <Node> > all_nodes=parser->get_all_nodes();
                for (unsigned l=0;l<all_nodes.size(); l++){
                    cache_nodes.push_back(all_nodes[l]); //there will be only one neighbor                
                    cache_modules.push_back(nsNodeIdToModule[all_nodes[l]->GetId()]);
                    nsNodeIdToModule[all_nodes[l]->GetId()]->enableCache(cache_mode, cache_cap, fast_cap, 
                                                                         &file_map, fp, enable_opt);
                    NS_LOG_UNCOND("Added cache app at node "<<  all_nodes[l]->GetId()<<
                                " mode:"<<(unsigned)cache_mode<<" capacity:"<<cache_cap<<
                                " betw "<<nsNodeIdToModule[all_nodes[l]->GetId()]->getBetweenness());                
                }
                
            }
        }
            
        NS_LOG_INFO("Initializing "<<receiver_nodes.size()<<" receivers");
        //start receivers - for every user give the worklad and the same file_map / catalog
        vector<pair<string, uint32_t> >  full_workload =  this->initializer->parseWorkload(seed);
        vector<pair<string, uint32_t> >::iterator start, end;
        uint32_t segment_size = (full_workload.size()/receiver_nodes.size()> 1) ? full_workload.size()/receiver_nodes.size() : 1 ;

        for(uint32_t node_i=0;node_i<receiver_nodes.size();node_i++){        
            //std::cout<<node_i<<" ";
            if (node_i*segment_size+segment_size > full_workload.size()) 
                break;
            start = full_workload.begin() + node_i*segment_size;
            end = start + segment_size;
            if(node_i == (receiver_nodes.size() -1)) end = full_workload.end();
            vector <pair<string, uint32_t> > sub_workload(start, end);

            /*for(vector <pair <string, uint32_t> >::iterator it=sub_workload.begin(); it!=sub_workload.end(); it++){
                std::cout<<"vec="<<it->first<<" "<<it->second<<std::endl;
            }*/

            receiver_apps.at(node_i)->workload = sub_workload;
            receiver_apps.at(node_i)->start();
            NS_LOG_INFO("Receiver at "<<node_i<<" workload "<<sub_workload.at(0).first<<", "<<sub_workload.size() );
            //break;
        }

        //NS_LOG_WARN("print vec finished!!");
        NS_LOG_INFO("Starting simulation");
        finished = false;
        //if (cache_mode!=0) cache_check(cache_nodes);
        uint32_t starttime = Now().ToInteger(Time::MS);

        Simulator::Run();//start the simulator ekteleitai h start toy reiceiver
        Simulator::Stop(Seconds(10));

        uint64_t dtime = Now().ToInteger(Time::MS);
        finished = true;
        NS_LOG_UNCOND("Overall service time: ms:"<<Now().ToInteger(Time::MS)<<" ps:"<<Now().ToInteger(Time::PS)<<" ("<<(starttime)<<") "); 
        
        /**
        * print caching results
        */
        if (cache_mode != 0){
            uint64_t reqs = 0;
            uint64_t hits = 0;
            uint64_t r_evictions = 0;
            uint64_t r_fetchings = 0;
            uint64_t r_insertions = 0;
            
            uint64_t false_positive_cnt = 0;
            uint64_t read_dram_cnt = 0;
            uint64_t readcache_rmlru = 0;
            uint64_t writecache_rmlru = 0;
            uint64_t stored_packets = 0;
            uint64_t w_storings = 0;
            uint64_t write_outoforder = 0;
            uint64_t false_positive_cnt_w = 0;
            uint64_t false_positive_cnt_test = 0; // for-test
            uint64_t false_positive_cnt_w_test = 0; // for-test
            uint64_t total_stored_packets = 0;
            uint64_t sram_stored_packets = 0;
            uint64_t dramcache_pcks=0;
            uint64_t dramcache_outoforder = 0;
            uint64_t dramcache_rmlru = 0;
            uint64_t ssd_rmlru = 0;
            uint64_t miss = 0;
            uint64_t fast_memory_hit = 0;
            uint64_t slow_memory_hit = 0;

            for (uint8_t n = 0; n< cache_nodes.size(); n++){
                Ptr<Node> nd = cache_nodes[n];
                reqs+=nsNodeIdToModule[nd->GetId()]->cache->requests;
                hits+=nsNodeIdToModule[nd->GetId()]->cache->hits;
                r_evictions+=nsNodeIdToModule[nd->GetId()]->cache->reads_for_evictions;
                r_fetchings+=nsNodeIdToModule[nd->GetId()]->cache->reads_for_fetchings;
                w_storings+=nsNodeIdToModule[nd->GetId()]->cache->write_for_storings;
                r_insertions+=nsNodeIdToModule[nd->GetId()]->cache->reads_for_insertions;

                //NS_LOG_UNCOND(nsNodeIdToModule[nd->GetId()]->cache->get_chunk_id_hits());

                false_positive_cnt += nsNodeIdToModule[nd->GetId()]->cache->false_positive_cnt;
                read_dram_cnt += nsNodeIdToModule[nd->GetId()]->cache->read_dram_cnt;
                readcache_rmlru += nsNodeIdToModule[nd->GetId()]->cache->readcache_rmlru;
                writecache_rmlru += nsNodeIdToModule[nd->GetId()]->cache->writecache_rmlru;
                stored_packets += nsNodeIdToModule[nd->GetId()]->cache->stored_packets;
                write_outoforder += nsNodeIdToModule[nd->GetId()]->cache->write_outoforder;
                false_positive_cnt_w += nsNodeIdToModule[nd->GetId()]->cache->false_positive_cnt_w;
                false_positive_cnt_w_test += nsNodeIdToModule[nd->GetId()]->cache->false_positive_cnt_w_test;
                false_positive_cnt_test += nsNodeIdToModule[nd->GetId()]->cache->false_positive_cnt_test;
                total_stored_packets += nsNodeIdToModule[nd->GetId()]->cache->total_stored_packets;
                sram_stored_packets += nsNodeIdToModule[nd->GetId()]->cache->sram_stored_packets;
                dramcache_pcks += nsNodeIdToModule[nd->GetId()]->cache->dramcache_pcks;
                dramcache_outoforder += nsNodeIdToModule[nd->GetId()]->cache->dramcache_outoforder;
                dramcache_rmlru += nsNodeIdToModule[nd->GetId()]->cache->dramcache_rmlru;
                ssd_rmlru += nsNodeIdToModule[nd->GetId()]->cache->ssd_rmlru;
                miss += nsNodeIdToModule[nd->GetId()]->cache->miss;
                fast_memory_hit += nsNodeIdToModule[nd->GetId()]->cache->fast_memory_hit;
                slow_memory_hit += nsNodeIdToModule[nd->GetId()]->cache->slow_memory_hit;
                 
            }
            NS_LOG_UNCOND("Cache requests: "<<reqs<<" hits: "<<hits<<" stored_packets: "<<
                      stored_packets<<" r_evictions: "<<r_evictions<<" r_fetchings: "<<
                                    r_fetchings<<" r_insertions: "<<r_insertions<<
                           " w_storings: "<<w_storings);
            NS_LOG_UNCOND("miss: "<<miss<<
                          ", fast_memory_hit: "<<fast_memory_hit<<
                          ", slow_memory_hit: "<<slow_memory_hit);
            NS_LOG_UNCOND("dramcache_pcks:"<<dramcache_pcks<<
                          ", dramcache_outoforder:"<<dramcache_outoforder<<
                          ", dramcache_rmlru:"<<dramcache_rmlru<<
                          ", ssd_rmlru:"<<ssd_rmlru);          
            NS_LOG_UNCOND("false_positive_cnt: "<<false_positive_cnt
                          <<" false_positive_cnt_test: " << false_positive_cnt_test
                         <<" read_dram_cnt: "<<read_dram_cnt
                        <<" readcache_rmlru: "<<readcache_rmlru
                        <<" writecache_rmlru: "<<writecache_rmlru);
            NS_LOG_UNCOND(" false_positive_cnt_w: "<<false_positive_cnt_w
                          <<" false_positive_cnt_w_test: " << false_positive_cnt_w_test
                          <<" write_outoforder_cnt: "<< write_outoforder
                          <<" total_stored_packets: "<<total_stored_packets
                          <<" sram_stored_packets: "<<sram_stored_packets);
        }
        
        NS_LOG_UNCOND("HITS="<<CcnModule::HITS);
        //print results and initialize structures again
        //----------------------------------------------------
        Ptr<ResultPrinter> rp=CreateObject<ResultPrinter>(nsNodeIdToModule,group_size,c,publisher_app,receiver_apps,output);

        //print to file
        //----------------------------------------------------
        ofstream file;
        string tempPath=output+"/results.txt";
        file.open (tempPath.c_str(),std::ios::app);
        file << "experiment "<<c<<" nodessize "<<group_size<<" participants ["<<nsNodeIdToModule[publisher_id]->getNodeId()<<",";  

        for(unsigned i=0;i<receiver_apps.size();i++)    {
            file<<receiver_apps.at(i)->getModule()->getNodeId();
            if(i!=group_size-1)
                file<<",";
        }    

        file<<"] interests "<<CcnModule::RX_INTERESTS<<" data "<<CcnModule::RX_DATA<<" server_interests "<<publisher_app->getInterests()<<" time: "<<dtime<<"("<<(dtime-starttime)<<")\n";
        file.close();
        //----------------------------------------------------
        CcnModule::RX_INTERESTS=0;
        CcnModule::RX_DATA=0;
        //for(unsigned i=0;i<module.size();i++){
            //module.at(i)->reInit();
        //}
        //----------------------------------------------------
    
}

// CAREFULL this runs for ever!!
void BootstrappingHelper::cache_check(vector < Ptr< Node > > cache_nodes){
    if (finished)
        return;
    for (uint8_t n = 0; n < cache_nodes.size(); n++){
        Ptr<Node> nd = cache_nodes[n];
        std::cout<<nd->GetId()<<"(stored_items): betw:"<<nsNodeIdToModule[nd->GetId()]->getBetweenness()<<" "<<nsNodeIdToModule[nd->GetId()]->cache->get_state();    
        std::cout<<nd->GetId()<<"(file_hits): betw:"<<nsNodeIdToModule[nd->GetId()]->getBetweenness()<<" "<<nsNodeIdToModule[nd->GetId()]->cache->get_file_hits();    
        std::cout<<nd->GetId()<<"(chunkID_hits): betw:"<<nsNodeIdToModule[nd->GetId()]->getBetweenness()<<" "<<nsNodeIdToModule[nd->GetId()]->cache->get_chunk_id_hits();    
     }
    Time t=ns3::Seconds(50);
    Simulator::Schedule(t,&BootstrappingHelper::cache_check,this,cache_nodes);
}


void BootstrappingHelper::PITCheck(int gs,int exp,vector<uint32_t> nodes,Graph topology,uint32_t owner){
    string nodes_nodes="";
    for (vector<uint32_t>::iterator it=nodes.begin(); it!=nodes.end(); ++it){
        std::stringstream s;
        s << *it;
        nodes_nodes=nodes_nodes+","+s.str();
    }

    for (map <uint32_t , Ptr < CcnModule > >::iterator iter=nsNodeIdToModule.begin(); iter!= nsNodeIdToModule.end(); ++iter)
    {
        if (iter->second->getPIT()->getSize()==0)
            continue;
        stringstream st;
        st << gs;
        stringstream st2;
        st2 << exp;
        ofstream file;
        string secondPartPath="CCN/pit_stats/gs-"+st.str()+"-experiment-"+st2.str()+"-nodes_nodes-"+nodes_nodes+"-seed-"+seedString+".txt";
        string tempPath=output+secondPartPath;
        const char* tempPath2=tempPath.c_str();
        file.open (tempPath2,std::ios::app);
        file<<"router "<<iter->second->getNodeId()<<" pit_entries "<<iter->second->getPIT()->getSize();
        if(topology.isItCoreNode(iter->second->getNodeId()+1))//+1 because the implementation of our graph used to calculate the degree here ,counts from 1
            file<<" core";
        else
            file<<" access";
        file<<endl;
        file.close();    
    }
}
