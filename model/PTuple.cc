#include "PTuple.h"

namespace ns3 {

int PTuple::COUNT_TUPLES = 0;

PTuple::PTuple()
{
    COUNT_TUPLES++;

    r = vector<Ptr<LocalApp> >();
    d = vector<Ptr<NetDevice> >();
}

PTuple::PTuple( vector<Ptr<LocalApp> >* apps)
{
    COUNT_TUPLES++;

    r = vector<Ptr<LocalApp> >();
    vector<Ptr<LocalApp> >::iterator iter;
    for(iter=apps->begin(); iter!=apps->end(); iter++){
        r.push_back(*iter);
    }

    d = vector<Ptr<NetDevice> >();
}

PTuple::PTuple( vector<Ptr<NetDevice> >* devices)
{
    COUNT_TUPLES++;

    d = vector<Ptr<NetDevice> >();
    vector<Ptr<NetDevice> >::iterator iter;
    for(iter=devices->begin(); iter!=devices->end(); iter++){
        d.push_back(*iter);
    }

    r = vector<Ptr<LocalApp> >();
}

PTuple::~PTuple() {



}

void PTuple::DoDispose(void)
{
    d.clear();
    r.clear();

    COUNT_TUPLES--;

    //if(COUNT_TUPLES==0)
    //{
        //std::cout<<"TUPLES over."<<std::endl;
    //}
}

bool PTuple::addLocalApp(Ptr<LocalApp> app){
    bool added = false;
    if (find(app) == r.end()){
        r.push_back(app);
        added = true;
    }
    return added;
}

bool PTuple::addDevice(Ptr<NetDevice> device){
    bool added = false;
    if (find(device) == d.end()){
        d.push_back(device);
        added = true;
    }
    return added;
}

bool PTuple::removeLocalApp(Ptr<LocalApp> app){
    bool found = false;
    vector<Ptr<LocalApp> >::iterator iter = find(app);
    if(iter != r.end()){
        r.erase(iter);
        found = true;
    }

    return found;
}

bool PTuple::removeDevice(Ptr<NetDevice> device){
    bool found = false;
    vector<Ptr<NetDevice> >::iterator iter = find(device);
    if(iter != d.end()){
        d.erase(iter);
        found = true;
    }

    return found;
}

vector<Ptr<LocalApp> >::iterator PTuple::find(Ptr<LocalApp> app){
    vector<Ptr<LocalApp> >::iterator iter;
    for(iter=r.begin(); iter!=r.end(); iter++){
        if (*iter == app){
            break;
        }
    }
    return iter;
}

vector<Ptr<NetDevice> >::iterator PTuple::find(Ptr<NetDevice> device){
    vector<Ptr<NetDevice> >::iterator iter;
    for(iter=d.begin(); iter!=d.end(); iter++){
        if (*iter == device){
            break;
        }
    }
    return iter;
}
}
