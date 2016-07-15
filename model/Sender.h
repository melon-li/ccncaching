/*
 * Sender.h
 *
 *  Created on: Oct 21, 2013
 *      Author: Coaxial
 */

#ifndef SENDER_H_
#define SENDER_H_

#include "ns3/core-module.h"
#include "ns3/CcnModule.h"
#include "ns3/ccn-packets.h"
#include "ns3/local_app.h"
#include "ns3/CCN_Name.h"
#include <map>

using std::map;

namespace ns3{

class CcnModule;
class CCN_Name;

class Sender: public ns3::Application {

public:
    static int COUNT_SENDERS;
    Sender(Ptr<CcnModule>);
    virtual ~Sender();
    virtual void DoDispose(void);

    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    uint32_t getInterests();
    Ptr<LocalApp> getLocalApp();

    void MapTest(int l);

    void SendData(Ptr<CCN_Name> data, Ptr<Packet>);

    void AnnounceName(Ptr<CCN_Name> n);
    void insertData(Ptr< CCN_Name >, Ptr<Packet> );

private:
    void handleInterest(Ptr<CCN_Name>);
    void handleData(Ptr<CCN_Name>, uint8_t*, uint32_t);
    Ptr<Packet> findData(Ptr<CCN_Name>);

    Ptr<CcnModule> ccnm;
    Ptr<LocalApp> localApp;

    uint32_t interests;
    static uint8_t buf[PKT_SIZE];
    map<Ptr<CCN_Name>, Ptr<Packet> > data;
};

}
#endif
