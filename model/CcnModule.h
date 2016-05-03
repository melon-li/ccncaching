#ifndef CCNMODULE_H
#define CCNMODULE_H

#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/Trie.h"
#include "ns3/PIT.h"
#include "ns3/CCN_Name.h"
#include "ns3/ccn-packets.h"
#include "ns3/Cache.h"
#include "ns3/experiment_globals.h"

using std::string;


namespace ns3 {

class Trie;

class PIT;
class CCN_Data;
class CCN_Interest;
class CCN_Name;
class CacheModule;


class CcnModule: public Object {
public:

	static uint32_t RX_INTERESTS;
	static uint32_t RX_DATA;
	static int COUNT;

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

	friend bool operator< (const Ptr<NetDevice>&, const Ptr<NetDevice>&);
	char enableCache(char _mode, uint32_t _cache_cap, uint32_t _cache_fast_cap);
	
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

	static Time ONE_NS;
	uint32_t terminator;
	
	double betweenness;
	map <Ptr<CCN_Name>, float > nameToBetweenness;
	map <Ptr<CCN_Name>, float >::iterator betw_iter;
	
};
}
#endif

