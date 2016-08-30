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

   /* workload is the vector with requests
    * this method always requests the first packet of the last file in the workload, which
    * is then dropped out of the workload vector
    */
    void start();
    
    vector<pair<string, uint32_t> > workload;
    //map<string, uint32_t> file_map;

private:
    void handleInterest(Ptr<CCN_Name>);
    void handleData(Ptr<CCN_Name>, uint8_t*, uint32_t);
    void doSendInterest(Ptr<CCN_Name>);
    void sendInterests();
    string int2str(int n);

    Ptr<CcnModule> ccnm;
    Ptr<LocalApp> localApp;

    uint8_t* data;
    uint32_t length;
    Ptr<CCN_Name> dataName;
    uint32_t askedfor;
    map<Ptr<CCN_Name>, uint64_t> asked;
    uint64_t asking;
    uint32_t returned;
    uint32_t aborted_chunks;

    // current_filename, current_filesize,current_sequence
    string current_filename;
    uint32_t current_filesize;
    uint32_t current_fileseq;
    //typedef Info pair<uint32_t, uint32_t>;
    //map<string, Info> requested_files;
};
}
#endif
