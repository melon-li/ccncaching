#include "CcnModule.h"


using std::string;
using std::stringstream;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("CcnModule");

uint32_t CcnModule::RX_INTERESTS = 0;
uint32_t CcnModule::RX_DATA = 0;
int CcnModule::COUNT = 0;
uint32_t CcnModule::HITS = 0;

Time CcnModule::ONE_NS = NanoSeconds(1);

CcnModule::CcnModule(Ptr<Node> node) {
        
    COUNT++;
    nodePtr = node;    

    thePIT = CreateObject<PIT>();
    FIB = CreateObject<Trie>();
    p_RX_Data = 0;
    addresses = map<Ptr<NetDevice>, Address>();

    for (uint32_t i = 0; i < nodePtr->GetNDevices(); i++) {

        Ptr<NetDevice> device = nodePtr->GetDevice(i);
        prev_sendtimes[device] = Simulator::Now().GetPicoSeconds();
        device->SetReceiveCallback(
                MakeCallback(&CcnModule::handlePacket, this));

        Ptr<Channel> channel = device->GetChannel();
        Address adr;
        if (device == channel->GetDevice(0)) {
            adr = channel->GetDevice(1)->GetAddress();
        } else {
            adr = channel->GetDevice(0)->GetAddress();
        }
        addresses[device] = adr;
    }
    
    buf_cnt = 0;
    terminator=0;
    betweenness=0;
}
/**
 * enables and initializes the cache module of the CcnModule
 */
char CcnModule::enableCache(char _mode, uint64_t _cache_cap, uint64_t _cache_fast_cap,
                            map<string, uint32_t> *_file_map_p, double _fp, bool enable_opt){
    char mode = _mode;
    uint64_t capacity = _cache_cap/PKT_SIZE;

    if (mode == PACKET_CACHE_MODE){
        //In  Since LRU algorithm, DRAM entries to SRAM entries is one-to-one,
        //the _cache_fast_cap is not employed.
        cache = new P_Cache(capacity, _cache_fast_cap/LRU_ENTRY_SIZE);
    }else if(mode == OBJECT_CACHE_MODE){
        cache = new O_Cache(capacity, _cache_fast_cap/OPC_ENTRY_SIZE);
    }else{
        //In HCaching, L2 index is realized by A^2 bloom filter, 
        //the size of L2 index is determined by DRAM size.
        //We can calculate this size as follows:
        size_t ka = std::floor(-std::log(1 - std::sqrt(1 - _fp)) / std::log(2));
        size_t cells = ka*(capacity/PKT_NUM)/std::log(2); //bits
        uint64_t writing_cache_size;
        NS_LOG_UNCOND("Enable Cache,ka = "<<ka<<" cells = "<<cells<<
                      " size = "<<cells/1024/1024<<" Mb");

        //Get cache_size which is equit to (reading_cache_size + writing _cache_size) 
        uint64_t cache_size = _cache_fast_cap - cells/8;
        //Achieve the writing cache size
        if(enable_opt){
            writing_cache_size = uint64_t(cache_size/(1 + OPT_RATIO));
        }else{
            writing_cache_size = uint64_t(cache_size/2);
        }
        cache = new S_Cache(capacity, writing_cache_size/PKT_SIZE, _file_map_p, _fp, enable_opt);
    }
    return 0;
}

CcnModule::~CcnModule() {
    thePIT = 0;
    FIB = 0;
    nodePtr = 0;

    addresses.clear();
}

void CcnModule::reset()
{
    neighborModules.clear();
    nameToBetweenness.clear();
    addresses.clear();
}

void CcnModule::DoDispose(void) {
    thePIT = 0;
    FIB = 0;
    nodePtr = 0;

    addresses.clear();
    COUNT--;
}

void CcnModule::reInit() {
    thePIT = 0;
    thePIT = CreateObject<PIT>();
    p_RX_Data = 0;

    FIB = 0;
    FIB = CreateObject<Trie>();
}

void CcnModule::sendInterest(Ptr<CCN_Name> name, Ptr<LocalApp> localApp){
    Simulator::Schedule(PicoSeconds(1), &CcnModule::doSendInterest, this, name, localApp);//name and localApp are parameters
}

void CcnModule::sendData(Ptr<CCN_Name> name, uint8_t *buff, uint32_t bufflen){
    uint8_t *newBuff = (uint8_t*)malloc(bufflen);
    memcpy(newBuff, buff, bufflen);
    Simulator::Schedule(PicoSeconds(1), &CcnModule::doSendData, this, name, newBuff, bufflen);
}


void CcnModule::sendThroughDevice(Ptr<const Packet> p, Ptr<NetDevice> nd) {
    uint8_t* b = new uint8_t[p->GetSize()];
    p->CopyData(b, p->GetSize());
    Ptr<Packet> p2 = Create<Packet>(b, p->GetSize());
    delete [] b;

    //bool sent = nd->Send(p2, addresses[nd], 0x88DD);
    bool sent = nd->Send(p2, addresses[nd], CCN_PROTO);

    if (!sent) {
        uint8_t type = extract_packet_type(p);
        if (type == CCN_Packets::INTEREST) {
             Ptr<CCN_Interest> interest = CCN_Interest::deserializeFromPacket(p->Copy());
             NS_LOG_UNCOND(Simulator::Now ().GetPicoSeconds()<<" "<<nodePtr->GetId()<<
                           " dropped interest packet:" <<interest->getName()->toString());
        } else if (type == CCN_Packets::DATA) {
            Ptr<CCN_Data> data = CCN_Data::deserializeFromPacket(p->Copy());
            NS_LOG_UNCOND( Simulator::Now ().GetPicoSeconds()<<" "<<nodePtr->GetId()<<
                         " dropped data packet:"<<data->getName()->toString());
        }else{
            NS_LOG_UNCOND(nodePtr->GetId()<<" node dropped packet");
       }
    }
}

bool CcnModule::handlePacket(Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t a,
        const Address& ad) {
    uint8_t type = extract_packet_type(p);
    if (type == CCN_Packets::INTEREST) {
        RX_INTERESTS++;
        handleIncomingInterest(p, nd);
    } else if (type == CCN_Packets::DATA) {
        RX_DATA++;
        p_RX_Data++;
        handleIncomingData(p, nd);
    }

    return true;
}

uint8_t CcnModule::extract_packet_type(Ptr<const Packet> p) {
    uint8_t b2 = 0;
    p->CopyData(&b2, sizeof(uint8_t));
    return b2;
}


uint8_t cnt = 0;
void CcnModule::handleIncomingInterest(Ptr<const Packet> p, Ptr<NetDevice> nd){
    //struct timespec time_start={0, 0},time_end={0, 0};
    //clock_gettime(CLOCK_REALTIME, &time_start);
    //NS_LOG_UNCOND (Simulator::Now ().GetPicoSeconds () << "\t<<<<<<<<<<<");

    Ptr<CCN_Interest> interest = CCN_Interest::deserializeFromPacket(p->Copy());
    NS_LOG_DEBUG(Simulator::Now ().GetPicoSeconds()<<" "<<nodePtr->GetId()<<
                " got interest "<<interest->getName()->toString() <<" packet size="<<p->GetSize());
    //cache is enabled, look for stored response
    float betw = interest->getBetweenness();
    if (ExperimentGlobals::CACHE_PLACEMENT == 1 && betw < this->getBetweenness()){
        interest->setBetweenness(this->getBetweenness());
        NS_LOG_INFO("Router "<<getNode()->GetId()<<" updated btw from " << betw<<" to "<<this->getBetweenness());
    }
    if (cache != NULL){
        string pref = interest->getName()->getPrefix();
        string _id = interest->getName()->getID();
        int64_t lt = cache->get_cached_packet(pref, _id);
        //int64_t lt = -1;
        if (lt >= 0)    {//if >=0 then is found
            uint64_t lookup_time = get_sendtime(nd, SRAM_ACCESS_TIME+uint64_t(lt));
            //lookup_time = 0;
            //NS_LOG_DEBUG("getcache time for interest="<<lookup_time);
            HITS++;
            uint8_t *tmp = NULL;
            //std::cout<<getNode()->GetId()<<" hit,"<<pref<<" "<<_id<<std::endl;
            Ptr<CCN_Data> data = CreateObject<CCN_Data>(interest->getName(), tmp, 0, interest->getBetweenness());
            Ptr<const Packet> dataP=data->serializeToPacket();
            // simulate SRAM and DRAM delay 
            Simulator::Schedule(PicoSeconds(lookup_time), &CcnModule::sendThroughDevice, this, dataP, nd);
            //sendData(interest->getName(), NULL, 0);
            NS_LOG_INFO("Router "<<getNode()->GetId()<<" found packet cached");
            return;
         }
   
        //if out-of-order, ingore and drop it
        //if(lookup_time == 0) return;

        // lookup failed -> simulate SRAM delay
        Simulator::Schedule(PicoSeconds(SRAM_ACCESS_TIME), &CcnModule::dohandleIncomingInterest, this, p, nd);
    }// cache is not enabled
    else dohandleIncomingInterest(p, nd);

   //NS_LOG_UNCOND (Simulator::Now ().GetPicoSeconds () << "\t>>>>>>>>");
   //clock_gettime(CLOCK_REALTIME, &time_end);
   //printf("duration:%llus %lluns\n", time_end.tv_sec-time_start.tv_sec, time_end.tv_nsec-time_start.tv_nsec);
    
}
void CcnModule::dohandleIncomingInterest(Ptr<const Packet> p, Ptr<NetDevice> nd) {
    Ptr<CCN_Interest> interest = CCN_Interest::deserializeFromPacket(p->Copy());
        
    //checks if pit contains interest
    Ptr<PTuple> pt = this->thePIT->check(interest->getName()); 
    
    if (pt != 0){
        pt->addDevice(nd);
        return;
    }    

    //PIT tuple not found
    Ptr<TrieNode> tn = this->FIB->longestPrefixMatch(interest->getName());    
    if(tn->hasLocalApps()){
        Ptr<LocalApp> publisher = tn->getLocalApps().at(0);

        //save to PIT first, next hop is publisher
        Ptr<PTuple> tuple=CreateObject<PTuple>();
        tuple->addDevice(nd);
        this->thePIT->update(interest->getName(),tuple);
        publisher->deliverInterest(interest->getName());
        
        nameToBetweenness[interest->getName()] = interest->getBetweenness();
        return;
    }
    NS_ASSERT_MSG(tn->hasDevices(),"router " + nodePtr->GetId() << 
                  "does not know how to forward " << interest->getName()->toString());
            
    //interest will go to the first netdevice
    Ptr<NetDevice> outport = tn->getDevices().at(0);

    //must store in PIT
    Ptr<PTuple> ptuple = CreateObject<PTuple>();
    ptuple->addDevice(nd);
    thePIT->update(interest->getName(), ptuple);

    Ptr<Packet> packet = interest->serializeToPacket();
    NS_LOG_DEBUG(Simulator::Now ().GetPicoSeconds()<<" transfer, "<< nodePtr->GetId()<<
                " node, transfer "<<interest->getName()->toString());
    sendThroughDevice(packet, outport);
}

void CcnModule::handleIncomingData(Ptr<const Packet> p, Ptr<NetDevice> nd){
    
    Ptr<CCN_Data> data = CCN_Data::deserializeFromPacket(p->Copy());
    //cache is enabled, cache data packet
    float betw = data->getBetweenness();
    bool cache_packet = true;
    
    // Betw implementation
    if (ExperimentGlobals::CACHE_PLACEMENT == 1 && betw > this->getBetweenness() && this->getBetweenness()!=0){
        cache_packet = false;
    }
    
    //uint8_t *buffer = (uint8_t *)malloc((p->GetSize()+1)*sizeof(uint8_t));
   // p->Serialize(buffer, p->GetSize());
   // buffer[p->GetSize()+1] = '\0';
    NS_LOG_DEBUG(Simulator::Now ().GetPicoSeconds()<<" "<<nodePtr->GetId()
             << " got data"<<data->getName()->toString() 
            <<" packet size="<<p->GetSize());
    
    //free(buffer);
    // if caching is allowed at this node
    if (cache_packet && cache!=NULL){
        string pref = data->getName()->getPrefix();
        string _id = data->getName()->getID();
        NS_LOG_DEBUG(nodePtr->GetId()<<" get data to cache");

        int64_t lt = cache->cache_packet(pref, _id, NULL);
        //int64_t lt = 0;
        uint64_t lookup_time = get_sendtime(nd, SRAM_ACCESS_TIME +lt);
        //lookup_time = 0;
        //std::cout<<"data cache time="<<lookup_time<<std::endl;
       /* if(lookup_time>0){
            std::cout<<nodePtr->GetId() << " got data "<<data->getName()->toString()<<" lookup_time="<<lookup_time<<"\n";
        }*/
        NS_LOG_INFO("Cached packet "<<pref<<"/"<<_id<<" at router "<<getNode()->GetId()<<" router betw "<<this->getBetweenness()<<" packet "<<betw);
        Simulator::Schedule(PicoSeconds(lookup_time), &CcnModule::dohandleIncomingData, this, p, nd);
        //Simulator::Schedule(Seconds(0.1), &CcnModule::dohandleIncomingData, this, p, nd);
    }    
    else{
        dohandleIncomingData(p, nd);
    }
}

void CcnModule::dohandleIncomingData(Ptr<const Packet> p, Ptr<NetDevice> nd)
{
    Ptr<CCN_Data> data = CCN_Data::deserializeFromPacket(p->Copy());

    //always check PIT
    Ptr<PTuple> pt = this->thePIT->check(data->getName());

    if (pt != 0)
    {
        thePIT->erase(data->getName());

        //give data to any local app
        vector<Ptr<LocalApp> >& localApps = pt->getLocalApps();
        for (uint32_t i = 0; i < localApps.size(); i++)
        {
            localApps[i]->deliverData(data->getName(), data->getData(),    data->getLength());
        }

        //give data to any device
        vector<Ptr<NetDevice> >& devices = pt->getDevices();
        for (uint32_t i = 0; i < devices.size(); i++)
        {
            Ptr<const Packet> dataP=data->serializeToPacket();

            sendThroughDevice(dataP,devices[i]);
        }
    }
    else
    {
        if(ENABLE_AGGREGATION) 
            NS_LOG_WARN("Does not know how to forward data.");
    }
}



void CcnModule::doSendInterest(Ptr<CCN_Name> name, Ptr<LocalApp> localApp) {
    Ptr<PTuple> pt = thePIT->check(name);
    if (pt != 0 && ENABLE_AGGREGATION) {
        bool added = pt->addLocalApp(localApp);
        if (!added) {
            NS_LOG_WARN("local app has already requested "<<name->toString());
        }
        return;
    }

    Ptr<CCN_Interest> interest = CreateObject<CCN_Interest>(name);
    Ptr<TrieNode> fibLookup = this->FIB->longestPrefixMatch(name);
    if (fibLookup == 0) { //an den to vrike- den yparxei 
        stringstream sstr;
        sstr << "router " << nodePtr->GetId() << " cannot forward Interests for "
                << name->toString();
        string mesg = sstr.str();
        NS_ASSERT_MSG(fibLookup != 0, mesg);
    }

    pt = CreateObject<PTuple>();//apothikeuei ti exei zitisei kai pou to exei steilei
    pt->addLocalApp(localApp);
    
    thePIT->update(name, pt);

    if (fibLookup->hasLocalApps()) {
        Ptr<LocalApp> publisher = fibLookup->getLocalApps().at(0);
        publisher->deliverInterest(name);
    } else if (fibLookup->hasDevices()) {
        Ptr<NetDevice> netdevice = fibLookup->getDevices().at(0);
        Ptr<Packet> p = interest->serializeToPacket();
        sendThroughDevice(p, netdevice);
    } else {
        stringstream sstr;
        sstr << "router " << nodePtr->GetId() << "cannot forward Interests for "
                << name->toString();
        string mesg = sstr.str();
        NS_ASSERT_MSG(fibLookup != 0, mesg);
        NS_ASSERT_MSG(false, mesg);
    }
}



Ptr<Trie> CcnModule::getFIB() {
    return FIB;
}

void CcnModule::setFIB(Ptr<Trie> F) {
    FIB=F;
}


Ptr<Node> CcnModule::getNode() {
    return nodePtr;
}

map<Ptr<NetDevice>, Ptr<CcnModule> > CcnModule::getNeighborModules() {
    return neighborModules;
}

Ptr<PIT> CcnModule::getPIT() {
    return thePIT;
}

uint64_t CcnModule::get_sendtime(const Ptr<NetDevice> nd, uint64_t cache_delay){
    uint64_t cur = Simulator::Now().GetPicoSeconds();
    DataRate m_bps(LINK_CAPACITY);
    uint64_t v = m_bps.GetBitRate();
    uint64_t pt = PKT_SIZE*8*pow(10,12)/v; // tranfering data time
    map<Ptr<NetDevice>, int64_t>::iterator it =  prev_sendtimes.find(nd);
   
    //NS_LOG_DEBUG("cache_delay ="<<cache_delay<<",pt="<<pt);
    cache_delay = cache_delay > pt?cache_delay-pt:0; 

    //there are not other packets in queue of nd device
    if(it == prev_sendtimes.end()){
        it->second = cur + cache_delay;
        return cache_delay;
    }
    if(cur >= (it->second+pt)){
        it->second = cur + cache_delay;
        return cache_delay;
    }

    //there are other packets in queue of nd device
    cache_delay = cache_delay + (it->second + pt - cur);
    it->second = cur + cache_delay;
    return cache_delay;
}

void CcnModule::doSendData(Ptr<CCN_Name> name, uint8_t* buff, uint32_t bufflen) {

    Ptr<PTuple> ptuple = thePIT->check(name);

    if (ptuple == 0) {
        stringstream sstr;
        sstr << "router " << nodePtr->GetId() << " has no pit record for "
                << name->toString();
        string mesg = sstr.str();
        NS_ASSERT_MSG(ptuple != 0, mesg);
    }

    uint32_t deliverd = 0;
    vector<Ptr<LocalApp> >::iterator iter;
    for (iter = ptuple->getLocalApps().begin();
            iter != ptuple->getLocalApps().end(); iter++) {
        deliverd++;
        (*iter)->deliverData(name, buff, bufflen);
    }
    uint32_t fwded=0;
    vector<Ptr<NetDevice> >::iterator iter2;
    float betw = 9999999;
    betw_iter = nameToBetweenness.find(name);
    if (betw_iter!= nameToBetweenness.end()){
        betw = betw_iter->second;
        //NS_LOG_INFO("Found betweenness for packet "<< betw);
        nameToBetweenness.erase(betw_iter);
    }else
        {
        NS_LOG_INFO("Did not find betweenness for packet");
            }
    for (iter2 = ptuple->getDevices().begin();iter2 != ptuple->getDevices().end(); iter2++)
    {
        Ptr<CCN_Data> data = CreateObject<CCN_Data>(name, buff, bufflen, betw);
        Ptr<const Packet> dataP=data->serializeToPacket();

        fwded++;
        sendThroughDevice(dataP,*iter2);
    }

    // remove entry
    
    thePIT->erase(name);
    NS_ASSERT_MSG(fwded > 0 || deliverd > 0, "data went nowhere");

    free(buff);
}

bool operator<(const Ptr<NetDevice>& lhs, const Ptr<NetDevice>& rhs) {
    if(lhs==0)
    {
        NS_LOG_WARN("first was null");
    }

    if(rhs==0)
    {
        NS_LOG_WARN("second was null");
    }
    return lhs->GetAddress() < rhs->GetAddress();
}

uint32_t CcnModule::getNodeId()
{
    return nodePtr->GetId();
}

uint32_t CcnModule::getTXData()
{
    return p_RX_Data;
}

}
