#include "ns3/Initializer.h"

using std::stringstream;
using std::vector;
using std::queue;

NS_LOG_COMPONENT_DEFINE("Initializer");


namespace ns3 {

class CcnModule;

Initializer::Initializer(Ptr<Parser> parser,
        int dataOwner, map<uint32_t, Ptr <CcnModule> > nsNodeIdToModule) {
    this->parser = parser;
    this->dataOwner = dataOwner;
    this->dataNum = dataNum;
    this->nsNodeIdToModule = nsNodeIdToModule;


}

Initializer::~Initializer() {
    //delete connection;
}

void Initializer::doesEveryModuleHaveANode() {
    for (map<uint32_t, Ptr<CcnModule> >::iterator it=nsNodeIdToModule.begin(); it!= nsNodeIdToModule.end(); ++it) {
        if (it->second->getNode() == 0) {
            NS_LOG_INFO ("Module " << it->first<< " does not have a node.");
        }
    }
}

/*
 * initialize forwarding rules!!
 * Should be done at file-level, because packet-level requires too much time. DONE!
 */
void Initializer::initializeFIBs() {
    queue < Ptr<CcnModule> > q = queue<Ptr<CcnModule> >(); //queue construction
    Ptr<CcnModule> ccnm = nsNodeIdToModule[dataOwner];
    NS_ASSERT_MSG(ccnm!=NULL, "Could not find CcnModule for node id "<<dataOwner);
    q.push(ccnm);
    visited[ccnm->getNodeId()] = true;
    NS_LOG_INFO("Node "<<ccnm->getNodeId()<<" is now visited");
    //add FIBs from every network node to the publisher - inefficient
    vector < string > nameVector;
    nameVector.push_back(ROOT_DOMAIN);
    while (q.size() != 0) {
        Ptr<CcnModule> front_module = q.front();
        q.pop();

        map<Ptr<NetDevice>, Ptr<CcnModule> > neighbors = front_module->getNeighborModules();
        map<Ptr<NetDevice>, Ptr<CcnModule> >::iterator niter;
        NS_LOG_INFO("Node "<<front_module->getNodeId()<<" has "<<neighbors.size()<<" neighbors");
        for (niter = neighbors.begin(); niter != neighbors.end(); ++niter) {
            
            Ptr<CcnModule> c = niter->second;
            if (visited.find(c->getNodeId())!=visited.end()) {
                continue;
            }
            visited[c->getNodeId()] = true;
                NS_LOG_INFO("Node "<<c->getNodeId()<<" is now visited");

            Ptr<CCN_Name> name = CreateObject < CCN_Name > (nameVector);
            c->getFIB()->put(name, ndfinder(front_module->getNode(), c->getNode()));
            NS_LOG_INFO("Added FIB entry for "<<ROOT_DOMAIN<<" from "<<front_module->getNodeId()<<" to "<< c->getNodeId() );
            q.push(c);
        }        
    }
    NS_LOG_INFO("BFS visited " << visited.size() << " nodes");
}

/**
 * Parses workload and creates the map of files <filename, filesize> 
 * */
map<string, uint32_t> Initializer::parseFileMap(){
    map <string, uint32_t> map;
    std::ifstream myfile (WORKLOAD_FILE);
    string file,line;
    uint32_t packets=0;
    uint64_t sum_packets=0;
    NS_ASSERT_MSG(myfile.is_open(), "Unable to open file_map file:"<<WORKLOAD_FILE); 

    
    while ( getline (myfile,line) )    {
        file=(line.substr(0, line.find(" ")).c_str());
        if (map.find(file)==map.end()){
            packets = atoi(line.substr(line.find(" ")+1).c_str());
            map[file] = packets;
            sum_packets+=packets;
        }
    }
    myfile.close();
        
    NS_LOG_INFO("Num_of_files: "<<map.size()<<" data: " << sum_packets);
    file_map = map;
    return map;
}
/**
 * Creates a shuffled download catalog
 * */
vector<pair <string, uint32_t> > Initializer::parseWorkload(uint32_t sseed){
    vector <pair <string, uint32_t> > vec;
    std::ifstream myfile (WORKLOAD_FILE);
    NS_LOG_INFO("Workload path: "<<WORKLOAD_FILE);
    string line; 
    if (myfile.is_open()){
        while ( getline (myfile,line) )    
          vec.push_back (std::make_pair (line.substr(0, line.find(" ")).c_str(), file_map[line.substr(0, line.find(" ")).c_str()]));
        myfile.close();
      }    
    std::srand(sseed);
    std::random_shuffle(vec.begin(), vec.end()); // shuffle
    NS_LOG_INFO("workload size: "<<vec.size()<<" - items: first: "<< vec[0].first<<" middle: "<< vec[vec.size()/2].first<<" last: "<< vec[vec.size()-1].first);
    return vec;    
    }

void Initializer::initialize_FIBs_for_publisher_app( Ptr<Sender> _publisher_app){

        NS_ASSERT_MSG(file_map.size()>0, "Error no files found in catalog.. Exiting");
        vector<string> tokens;
        tokens.push_back(ROOT_DOMAIN);//tokens are names/prefixes
        for(map <string, uint32_t>::iterator it=file_map.begin();it!=file_map.end();++it)
        {        
            tokens.push_back(it->first);
            Ptr<CCN_Name> theName = CreateObject<CCN_Name>(tokens);
            NS_ASSERT_MSG(nsNodeIdToModule[dataOwner]->getFIB()->put(theName,_publisher_app->getLocalApp()),
                    "Could not add FIB entry :" <<theName->toString());
           // NS_LOG_INFO("Added pit for publisher "<<dataOwner<<" for file "<<it->first);
            tokens.pop_back(); 
        }    
        NS_LOG_INFO("Created CCN_names for publisher \n");
}

Ptr<CcnModule> Initializer::firstUnvisitedChild(Ptr<CcnModule> ccn) {
    map<Ptr<NetDevice>, Ptr<CcnModule> >::iterator it;
    for (it = ccn->getNeighborModules().begin();
            it != ccn->getNeighborModules().end(); it++) {
        Ptr<CcnModule> neighborModule = it->second;

        if (!visited[neighborModule->getNodeId()]) {
            return neighborModule;
        }
    }

    return 0;
}
/**
 * epistrefei to net device tou deksiou me to opoio o deksis syndeetai ston aristero
 * */
Ptr<NetDevice> Initializer::ndfinder(Ptr<Node> n1, Ptr<Node> n2) 
        {

    NS_ASSERT_MSG(n1 != 0,
        std::cout << "In ndfinder in initializer n1 is null" );
        
    NS_ASSERT_MSG(n2 != 0,
        std::cout << "In ndfinder in initializer n2 is null" );

    for (unsigned i = 0; i < n2->GetNDevices(); i++) {
        if (n2->GetDevice(i)->GetChannel()->GetDevice(0)->GetNode() == n1) {
            return n2->GetDevice(i);
        }

        if (n2->GetDevice(i)->GetChannel()->GetDevice(1)->GetNode() == n1) {
            return n2->GetDevice(i);
        }
    }

    return 0;
}

/*
 * files
 * @int: seedfor random workloads
 * @vector: files to be downloaded
 */
vector<string> Initializer::create_workload(map<string, uint32_t> files,
        uint8_t seed) {
    vector < string > _workload;
    double * probabilities = new double[files.size()];
    uint8_t j = 0;
    uint16_t num_of_files = files.size();
    // weight will normalize probabilities to 1 for the least popular
    unsigned weight = ceil(pow(num_of_files, ZIPF_A));
    for (map<string, uint32_t>::iterator it = files.begin(); it != files.end();
            ++it) {
        probabilities[j] = floor(weight * 1 / pow(j + 1, ZIPF_A));
        for (unsigned k = 0; k < probabilities[j]; k++)
            _workload.push_back(it->first);

        j++;
    }

    //randomly swap the downloads

    for (unsigned f = 0; f < _workload.size() / 2; f++) {
        for (unsigned i = 0; i < j; i++) {
            uint16_t A = i;
            uint16_t B = ExperimentGlobals::RANDOM_VAR->GetInteger(0,_workload.size()-1);

            string tmp = _workload.at(A);
            _workload.at(A) = _workload.at(B);
            _workload.at(B) = tmp;
        }
    }

    //std::cout<<"Overall there will be "<<_workload"
    delete[] probabilities;
    return _workload;
}

}
