#ifndef CCNMODULE_H
#define CCNMODULE_H

#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <queue>
#include <map>
#include "ns3/core-module.h"
#include "ns3/data-rate.h"
#include "ns3/network-module.h"
#include "ns3/Trie.h"
#include "ns3/PIT.h"
#include "ns3/CCN_Name.h"
#include "ns3/ccn-packets.h"
#include "ns3/Cache.h"
#include "ns3/experiment_globals.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ptr.h"

using std::string;


namespace ns3 {

class Trie;

class PIT;
class CCN_Data;
class CCN_Interest;
class CCN_Name;
class CacheModule;


enum CSState
{
        READY,   /**< The transmitter is ready to begin transmission of a packet */
        BUSY     /**< The transmitter is busy transmitting a packet */
};

// The Content Storage poll the recQueue and sendQueue alternately.
enum PollState{
    WRITE,
    READ
};

class CcnModule: public Object {
public:

    static uint32_t RX_INTERESTS;
    static uint32_t RX_DATA;
    static int COUNT;
    static uint32_t HITS;

    CcnModule(Ptr<Node> );
    ~CcnModule();
    void reset();


    virtual void DoDispose(void);

    uint8_t extract_packet_type(Ptr<const Packet> p);
    void reInit();

    void sendThroughDevice(Ptr<const Packet> p, Ptr<NetDevice> nd);
    int decideTtl();

    uint32_t getTXData(); //const {return p_RX_Data;}
    uint32_t getNodeId(); //const {return nodePtr->GetId(); }
    Ptr<Trie> getFIB();
    void setFIB(Ptr<Trie> F);

    Ptr<PIT> getPIT();
    Ptr<Node> getNode();
    map<Ptr<NetDevice>, Ptr<CcnModule> > getNeighborModules();
    void setNeighbor(Ptr<NetDevice> dev, Ptr<CcnModule> module){ neighborModules[dev] = module;}

    //for LocalApp
    void sendInterest(Ptr<CCN_Name> name, Ptr<LocalApp>);
    void sendData(Ptr<CCN_Name>, uint8_t *buff, uint32_t bufflen);

    bool handlePacket(Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t a, const Address& ad);
    void handleIncomingData(Ptr<const Packet> p, Ptr<NetDevice> nd);
    void dohandleIncomingData(Ptr<const Packet> p, Ptr<NetDevice> nd);
    void handleIncomingInterest(Ptr<const Packet> p, Ptr<NetDevice> nd);
    void dohandleIncomingInterest(Ptr<const Packet> p, Ptr<NetDevice> nd);
    void cacheSend(Ptr<const Packet> p, Ptr<NetDevice> nd);
    bool cacheTransmitStart();
    void cacheTransmitComplete();
    void getCachedSend(Ptr<const Packet> p, Ptr<NetDevice> nd);
    bool getCachedTransmitStart();
    void getCachedTransmitComplete();
    void senderTransmitData(Ptr<NetDevice> nd);
    void senderTransmitComplete(Ptr<NetDevice> nd);

    friend bool operator< (const Ptr<NetDevice>&, const Ptr<NetDevice>&);
    char enableCache(char _mode, uint64_t _cache_cap, uint64_t _cache_fast_cap, 
                     map<string, uint32_t> * file_map_p, double _fp, bool enable_opt);
    
    Ptr<CacheModule> cache;

    void setBetw(const double& betw){
        betweenness = betw;
        }
        
    double getBetweenness(){
        return betweenness;
        }

private:

    uint32_t p_RX_Data;
    Ptr<Trie> FIB;
    Ptr<PIT> thePIT;
    Ptr<Node> nodePtr;
    map<Ptr<NetDevice>, Ptr<CcnModule> > neighborModules;

    map<Ptr<NetDevice>, Address> addresses;

    void doSendInterest(Ptr<CCN_Name> name, Ptr<LocalApp>);
    void doSendData(Ptr<CCN_Name>, uint8_t *buff, uint32_t bufflen);

     
    uint32_t buf_cnt;
    //map<Ptr<NetDevice>, int64_t> prev_sendtimes;
    bool congestionFlag;
    CSState m_CSState;
    PollState m_PollState;
    uint64_t minCompletedTime;
    //uint64_t sendQueue;
    //uint64_t recQueue;
    std::queue<std::pair<Ptr<const Packet>, Ptr<NetDevice> > > recQueue;
    std::queue<std::pair<Ptr<const Packet>, Ptr<NetDevice> > > sendQueue;
    std::map<Ptr<NetDevice>, std::queue<Ptr<const Packet> > > sendDataQueues;
    std::map<Ptr<NetDevice>, CSState> m_senderStates;
    /*
     *paragram: cache delay, PicoSeconds number
     *return: PicoSeconds number after current time
     */
    //uint64_t get_sendtime(Ptr<NetDevice> nd, uint64_t cache_delay);
    uint64_t pushSendQueue(Ptr<NetDevice> nd, uint64_t cache_delay);
    uint64_t popRecQueue(Ptr<NetDevice> nd, uint64_t cache_delay);

    static Time ONE_NS;
    uint32_t terminator;
    
    double betweenness;
    map <Ptr<CCN_Name>, float > nameToBetweenness;
    map <Ptr<CCN_Name>, float >::iterator betw_iter;
    
};
}
#endif

