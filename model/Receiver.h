/*
 * Receiver.h
 *
 *  Created on: Oct 21, 2013
 *      Author: Coaxial
 */

#ifndef RECEIVER_H_
#define RECEIVER_H_

#include "ns3/core-module.h"
#include "ns3/CcnModule.h"
#include "ns3/local_app.h"

#include <set>

using std::set;

namespace ns3 {

class CcnModule;
class CCN_Name;

class Receiver: public Application {

public:
    static int COUNT_RECEIVERS;
    Receiver(Ptr<CcnModule> ccnm);
    ~Receiver();
    virtual void DoDispose(void);

    static TypeId GetTypeId(void);
    Ptr<CcnModule> getModule();
    virtual TypeId GetInstanceTypeId(void) const;
    uint32_t getAskedFor();
    uint32_t getReturned();

    void start();
    
    vector<pair<string, uint32_t> > workload;
    //map<string, uint32_t> file_map;

private:
    void handleInterest(Ptr<CCN_Name>);
    void handleData(Ptr<CCN_Name>, uint8_t*, uint32_t);
    void doSendInterest(Ptr<CCN_Name>);

    Ptr<CcnModule> ccnm;
    Ptr<LocalApp> localApp;

    uint8_t* data;
    uint32_t length;
    Ptr<CCN_Name> dataName;
    uint32_t askedfor;
    set<Ptr<CCN_Name> > asked;
    uint32_t returned;
    uint32_t aborted_chunks;


    string current_filename;
    uint32_t current_filesize;
};
}
#endif
