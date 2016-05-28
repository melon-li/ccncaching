/*
 * local_app.cc
 *
 *  Created on: Dec 24, 2013
 *      Author: tsilochr
 */

#include "ns3/local_app.h"
#include <time.h>

namespace ns3{

int LocalApp::COUNT_APPS = 0;

Time LocalApp::ONE_NS = NanoSeconds(1);

void LocalApp::deliverInterest(Ptr<CCN_Name> name){
    Simulator::Schedule(ONE_NS, &LocalApp::doDeliverInterest, this, name);
}

void LocalApp::deliverData(Ptr<CCN_Name> name, uint8_t* buffer, uint32_t bufflen){
    Simulator::Schedule(ONE_NS, &LocalApp::doDeliverData, this, name, buffer, bufflen);
}

bool operator< (const Ptr<LocalApp>& first, const Ptr<LocalApp>& second)
{
    if(PeekPointer(first)<PeekPointer(second))
    {
        return true;
    }

    return false;
}
}


