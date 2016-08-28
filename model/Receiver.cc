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
    current_filename = "";
    current_filesize = 0;
    current_fileseq = 0;
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
        NS_ASSERT_MSG(false, "Got a Data for interest not asked " << name->toString());
    }else{
        returned++;
        asked.erase(name);
        //simulate download abortions
        /*if (current_sequence%10==0 &&  ExperimentGlobals::RANDOM_VAR->GetInteger(0,5000)<10){
            aborted_chunks += current_filesize - current_sequence;
            NS_LOG_INFO("Receiver: "<<ccnm->getNodeId()<<" aborted chunks: "<<aborted_chunks);
            start();
            return;
         }*/
        sendInterests();
    }
}

void Receiver::start() {
        //NS_LOG_INFO("Receiver: "<<ccnm->getNodeId()<<" requested file: "<<workload.at(workload.size()-1).first);
        sendInterests();
}

void Receiver::sendInterests(){
        uint32_t i = 0;
        vector<string> tokens;

        while(1){
            if(current_fileseq > current_filesize or current_filesize == 0){
                if(workload.empty()){
                    if(asked.size()) return;
                    std::cout<<"Receiver at node "<<ccnm->getNodeId()<<" finnished "
                         <<Now().ToInteger(Time::MS)<<" askedfor="<<askedfor<<std::endl;
                    return;
                }

                if (workload.size()%100==0){
                    NS_LOG_INFO(Now().ToInteger(Time::MS)<<" Receiver: "<<ccnm->getNodeId()
                                <<" remaining downloads: "<<workload.size());
                }

                std::cout<<"Receiver: "<<ccnm->getNodeId()
                          <<" requested file: "<<workload.at(workload.size()-1).first<<std::endl;
                current_filename = workload.at(workload.size()-1).first;
                current_filesize = workload.at(workload.size()-1).second;
                current_fileseq =  1;
                workload.pop_back();
            }

            tokens.clear();
            tokens.push_back(ROOT_DOMAIN);//eg. domain1
            tokens.push_back(current_filename);
            Ptr<CCN_Name> theName;        
            for(i=current_fileseq; i<=current_filesize; i++){
                if(asked.size() >= WIN_MAX) break;
                tokens.push_back(int2str(i));
                theName = CreateObject<CCN_Name>(tokens);        
                tokens.pop_back();
                if (asked.find(theName) != asked.end())  continue;
                asked.insert(theName);
                std::cout<<"rec send interest "<<theName->toString()<<std::endl;
                Simulator::Schedule(PicoSeconds(0), &Receiver::doSendInterest, this, theName);
            }
            current_fileseq = i;
            if(asked.size() >= WIN_MAX) break;
        }
}


void Receiver::doSendInterest(Ptr<CCN_Name> name){
//    std::cout<<Now().ToInteger(Time::MS)<<" "<<ccnm->getNodeId()<<" asking for: "<<name->toString()<<std::endl;
    askedfor++;
    this->ccnm->sendInterest(name, localApp);
}

}//end ns3
