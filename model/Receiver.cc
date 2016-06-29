#include "Receiver.h"
#include <vector>
#include <string>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include "boost/lexical_cast.hpp"

using std::stringstream;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Receiver");

int Receiver::COUNT_RECEIVERS = 0;

Receiver::Receiver(Ptr<CcnModule> ccnmIn) {
    COUNT_RECEIVERS++;
    askedfor = 0;
    asked = set<Ptr<CCN_Name> >();

    returned = 0;
    ccnm = ccnmIn;
    localApp = CreateObject<LocalApp>();

    Callback<void, Ptr<CCN_Name> > interestCb = MakeCallback(&Receiver::handleInterest, this);
    localApp->setInterestCallback(interestCb);

    Callback<void, Ptr<CCN_Name>, uint8_t*, uint32_t> dataCb = MakeCallback(&Receiver::handleData, this);
    localApp->setDataCallback(dataCb);
    
    aborted_chunks=0;
}

Receiver::~Receiver() {
    ccnm = 0;
    localApp = 0;
    dataName = 0;
    asked.clear();
}

void Receiver::DoDispose(void){
    ccnm = 0;
    localApp = 0;
    dataName = 0;
    asked.clear();

    COUNT_RECEIVERS--;

    if(COUNT_RECEIVERS==0)
    {
        std::cout<<"RECEIVERS over."<<std::endl;
    }
}

TypeId Receiver::GetTypeId(void) {
    static TypeId t = TypeId("RECEIVER");
    t.SetParent<Object>();
    //t.AddConstructor<CCNPacketSizeHeader>();

    return t;
}

TypeId Receiver::GetInstanceTypeId(void) const {
    return GetTypeId();
}

uint32_t Receiver::getReturned()
{
    return returned;
}

uint32_t Receiver::getAskedFor()
{
    return askedfor;
}

Ptr<CcnModule> Receiver::getModule()
{
    return ccnm;
}


void Receiver::handleInterest(Ptr<CCN_Name>){
    NS_ASSERT_MSG(false, "Receiver should not receive any Interests");
}

void Receiver::handleData(Ptr<CCN_Name> name, uint8_t*, uint32_t){
    if (asked.find(name) == asked.end()){
        NS_ASSERT_MSG(false, "Got a Data for interest not asked " << name->toString());
    }else{
        returned++;
        vector<string> tokens = name->getTokens();
        string id_str = tokens.back();
        unsigned id = atoi(id_str.c_str());
        
        tokens.pop_back();//vgale to teleutaio item
        uint32_t num_of_packets = current_filesize;
        if (id>=num_of_packets){
            //std::cout<<ccnm->getNodeId()<<" Got "<<id<<" packets of file "<<name->getTokens()[1]<<std::endl;
            start();//zitaei to prwto tou epomenoy arxeioy
            return;
        }
        //simulate download abortions
        /*if (id%10==0 &&  ExperimentGlobals::RANDOM_VAR->GetInteger(0,5000)<10){
            aborted_chunks+=num_of_packets-id;
            NS_LOG_INFO("Receiver: "<<ccnm->getNodeId()<<" aborted chunks: "<<aborted_chunks);
            start();
            return;
        }*/
        
        id++;
        stringstream sstream;
        sstream << id;
        string numstr = sstream.str();
        tokens.push_back(numstr);
        Ptr<CCN_Name> theName = CreateObject<CCN_Name>(tokens);
        Simulator::Schedule(PicoSeconds(0), &Receiver::doSendInterest, this, theName);
    }
}

/* //workload is the vector with requests
 * this method always requests the first packet of the last file in the workload, which
 * is then dropped out of the workload vector
 * */
void Receiver::start() {
        if (workload.empty()){
            std::cout<<"Receiver at node "<<ccnm->getNodeId()<<" finnished "<<Now().ToInteger(Time::MS)<<" askedfor="<<askedfor<<std::endl;
            return;
        }
        if (workload.size()%100==0){
            NS_LOG_INFO(Now().ToInteger(Time::MS)<<" Receiver: "<<ccnm->getNodeId()<<" remaining downloads: "<<workload.size());
            //std::cout<<"Receiver at node "<<ccnm->getNodeId()<<" finnished "<<Now().ToInteger(Time::MS)<<std::endl;
            //return;
        }
        //NS_LOG_INFO("Receiver: "<<ccnm->getNodeId()<<" requested file: "<<workload.at(workload.size()-1).first);
        //workload is a vector with strings
        current_filename = workload.at(workload.size()-1).first;
        current_filesize = workload.at(workload.size()-1).second;
        vector<string> tokens;
        tokens.push_back(ROOT_DOMAIN);//eg. domain1
        tokens.push_back(current_filename);
        tokens.push_back("1");
        Ptr<CCN_Name> theName = CreateObject<CCN_Name>(tokens);        
        Simulator::Schedule(PicoSeconds(0), &Receiver::doSendInterest, this, theName);
        workload.pop_back();
        // if I dont clear it, sim does not scale...
        asked.clear();
}

void Receiver::doSendInterest(Ptr<CCN_Name> name){
    askedfor++;
    asked.insert(name);
//    std::cout<<Now().ToInteger(Time::MS)<<" "<<ccnm->getNodeId()<<" asking for: "<<name->toString()<<std::endl;
    this->ccnm->sendInterest(name, localApp);
}

}
