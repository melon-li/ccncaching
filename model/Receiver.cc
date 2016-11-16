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
    maxRate = DataRate(LINK_THROUGHTPUT).GetBitRate();
    maxLen = maxRate/(DataRate(USER_EXPERIENCED_RATE).GetBitRate());
    // the max rate for requesting data
    maxRate = maxRate*REQ_SIZE/PKT_SIZE;
    sendRate = maxRate;
    offSet = 0;
    // the max number of requesting packets for a TTL time.
    asked_size = (maxRate*TTL)/(REQ_SIZE);
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
        NS_LOG_UNCOND("RECEIVERS over.");
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

string Receiver::int2str(int n){
    stringstream ss;
    ss<<n;
    return ss.str();
}

void Receiver::handleInterest(Ptr<CCN_Name>){
    NS_ASSERT_MSG(false, "Receiver should not receive any Interests");
}

void Receiver::handleData(Ptr<CCN_Name> name, uint8_t*, uint32_t){
    if (asked.find(name) == asked.end()){
        if(ENABLE_AGGREGATION == false) return; 
        NS_ASSERT_MSG(false, "Got a Data for interest not asked " << name->toString());
    }else{
        returned++;
        asked.erase(name);
        //std::cout<<"asking="<<asked.size()<<std::endl;
        //std::cout<<"asked_size="<<asked_size<<std::endl;
        
        //simulate download abortions
        /*if (current_sequence%10==0 &&  ExperimentGlobals::RANDOM_VAR->GetInteger(0,5000)<10){
            aborted_chunks += current_filesize - current_sequence;
            NS_LOG_INFO("Receiver: "<<ccnm->getNodeId()<<" aborted chunks: "<<aborted_chunks);
            start();
            return;
         }*/
        //sendInterests();

        NS_LOG_DEBUG("Data packet arrives at receiver at node "<<ccnm->getNodeId());
        if(workload.empty()){
            if(asked.size()) return;
            NS_LOG_UNCOND(Simulator::Now ().GetPicoSeconds()<<" "
                 <<"Receiver at node "<<ccnm->getNodeId()<<" finnished "
                 <<" askedfor="<<askedfor<<",returned="<<returned);
            return;
        }
    }
}

void Receiver::start() {
        //NS_LOG_INFO("Receiver: "<<ccnm->getNodeId()<<" requested file: "<<workload.at(workload.size()-1).first);
        sendInterests();
}

void Receiver::sendInterests(){
        uint64_t tNext = 0;
        Ptr<CCN_Name> theName = NULL;        

        theName = nextRequestName();
        if(!theName)return; 
        asked.insert(theName);
        NS_LOG_DEBUG("rec send interest "<<theName->toString());
        
        //simple congestion control algorithms
        if(asked.size() > asked_size*2){
            sendRate = sendRate/2;
            //NS_LOG_UNCOND("asked_size > asked_size*2,descrease sendRate to "<<
            //              float(sendRate)/1024/1024<<" Mb"); 
        }else if(asked.size() < asked_size/2){
            sendRate = sendRate*2;
            if(sendRate >= maxRate) sendRate = maxRate;
            //NS_LOG_UNCOND("asked_size < asked_size/2,increase sendRate to "<<
            //              float(sendRate)/1024/1024<<" Mb"); 
        }

        tNext = REQ_SIZE*8*pow(10, 12)/sendRate;
        Simulator::Schedule(PicoSeconds(tNext), &Receiver::doSendInterest, this, theName);
}

void Receiver::doSendInterest(Ptr<CCN_Name> name){
//    std::cout<<Now().ToInteger(Time::MS)<<" "<<ccnm->getNodeId()<<
       //" asking for: "<<name->toString()<<std::endl;
    askedfor++;
    this->ccnm->sendInterest(name, localApp);
    sendInterests();
}


Ptr<CCN_Name> Receiver::nextRequestName(){
        Ptr<CCN_Name> theName = NULL;
        while(!theName){
            theName = doNextRequestName();
            if(!theName && sendFiles.size() == 0
               && workload.empty()) return 0;
        }
       return theName;
}

Ptr<CCN_Name> Receiver::doNextRequestName(){
        vector<string> tokens;
        Ptr<CCN_Name> theName = NULL;
        std::pair<string, std::pair<uint32_t, uint32_t>> p;

        //assemble sendFiles to MaxLen
        while(sendFiles.size() < maxLen){

            if(workload.empty()) break;

            if (workload.size()%100==0){
                NS_LOG_INFO(Now().ToInteger(Time::MS)<<
                                   " Receiver: "<<ccnm->getNodeId()<<
                             " remaining downloads: "<<workload.size());
            }

            //std::cout<<"Receiver: "<<ccnm->getNodeId()
            //          <<" requested file: "<<workload.at(workload.size()-1).first<<std::endl;
            p.first = workload.at(workload.size()-1).first;
            p.second.first = workload.at(workload.size()-1).second;
            p.second.second = 1;
            sendFiles.push_back(p);
            workload.pop_back();
        }
        
        //achieve the name of requesting packet
        do{
            if(sendFiles.size()==0) return NULL;
            if(offSet >= sendFiles.size()) offSet = 0;

            tokens.clear();
            tokens.push_back(ROOT_DOMAIN);//eg. domain1
            tokens.push_back(sendFiles[offSet].first);
            tokens.push_back(int2str(sendFiles[offSet].second.second));
            theName = CreateObject<CCN_Name>(tokens);        

            if((sendFiles[offSet].second.first - sendFiles[offSet].second.second) <= 0){
                sendFiles.erase(sendFiles.begin() + offSet);
            }else{
                sendFiles[offSet].second.second++;
                offSet++;
            }
        }while(ENABLE_AGGREGATION && asked.find(theName) != asked.end());

        return theName;
}

}//end ns3
