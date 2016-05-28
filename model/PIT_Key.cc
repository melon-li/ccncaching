#include "PIT_Key.h"
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "boost/lexical_cast.hpp"

namespace ns3 {

PIT_Key::PIT_Key(Ptr<CCN_Name> name,Ptr<NetDevice>  device)
{
    this->name=name;
    this->device=device;
    this->app=0;
}

PIT_Key::PIT_Key(Ptr<CCN_Name> name,Ptr<LocalApp>  app)
{
    this->name=name;
    this->device=0;
    this->app=app;
}

PIT_Key::~PIT_Key() {    
}

void PIT_Key::DoDispose(void) {
    name = 0;
    device = 0;
    app = 0;
}

/*bool operator< (const Ptr<PIT_Key>& first, const Ptr<PIT_Key>& second)
{
    //first compare names ,the overloading needed is already implemented in CCN_Name.cc
    if(first->getName()<second->getName())
    {
        return true;
    }


    if(first->getName()>second->getName())
    {
        return false;
    }


    //then compare total size of vectors
    if(first->getLocalApps().size()+first->getDevices().size() < second->getLocalApps().size()+second->getDevices().size())
    {
        return true;
    }

    if(first->getLocalApps().size()+first->getDevices().size() > second->getLocalApps().size()+second->getDevices().size())
    {
        return false;
    }

    //If total size is also equal ,then compare the first NetDevice of each object
    //or else compare the first LocalApp of each object ,or else consider smaller the
    //object that has just NetDevices.
    //(There must be at least one NetDevice or LocalApp in every PIT_Key because everything
    //gets forwarded somewhere.)
    //Operator < for NetDevices is implemented in CcnModule.cc and operator <
    //for LoacApps is implemented in local_app.cc

    if(first->getDevices()!=0&&second->getDevices()!=0)
    {
        if(first->getDevices().at(0)<second->getDevices().at(0))
        {
            return true;
        }

        if(first->getDevices().at(0)>second->getDevices().at(0))
        {
            return false;
        }
    }

    if(first->getLocalApps()!=0&&second->getLocalApps()!=0)
    {
        if(first->getLocalApps().at(0)<second->getLocalApps().at(0))
        {
            return true;
        }

        if(first->getLocalApps().at(0)>second->getLocalApps().at(0))
        {
            return false;
        }
    }

    //if one of the PIT_Keys has just NetDevices and the other has just LocalApps then
    //consider smaller the object that has just NetDevices

    if(first->getDevices().size()!=0)
    {
        return true;
    }

    if(second->getDevices().size()!=0)
    {
        return false;
    }

    return true;
}*/

bool operator< (const Ptr<PIT_Key>& first, const Ptr<PIT_Key>& second)
{
    //first compare names ,the overloading needed is already implemented in CCN_Name.cc
    if(first->getName()<second->getName())
    {
        return true;
    }

    if(first->getName()>second->getName())
    {
        return false;
    }

    //Every object has at least a device or an app because
    //everything gets forwarded.

    if(first->getDevice()!=0 && second->getDevice()!=0)//if both have a device
    {
        return first->getDevice() < second->getDevice();//overloading of < for NetDevices is implemented in CcnModule.cc.
    }

    if(first->getLocalApp()!=0 && second->getLocalApp()!=0)//if both have an app
    {
        return first->getLocalApp() < second->getLocalApp();//overloading of < for LocalApps is implemented in local_app.cc.
    }

    //if one has an app and the other has a device consider smaller the one that has a device
    if(first->getDevice()!=0 && second->getDevice()==0)
    {
        return true;
    }

    if(first->getDevice()==0 && second->getDevice()!=0)
    {
        return false;
    }

    return false;
}

/*bool operator< (const Ptr<PIT_Key>& first, const Ptr<PIT_Key>& second)
{
    if(PeekPointer(first)<PeekPointer(second))
    {
        return true;
    }

    return false;
}*/

}
