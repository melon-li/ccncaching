#include <map>
#include "PIT.h"

using std::pair;

namespace ns3 {

int PIT::COUNT_PITS = 0;

PIT::PIT() {
    COUNT_PITS++;
    table = map<Ptr<CCN_Name>, Ptr<PTuple> >();
}

PIT::~PIT() {
    table.clear();
}

/*
 * checks if named content is stored in PI
 */
Ptr<PTuple> PIT::check(Ptr<CCN_Name> key)
{
    map<Ptr<CCN_Name>, Ptr<PTuple> >::iterator find = table.find(key);

    if (find != table.end())
    {
        return find->second;
    }
    else
    {
        return 0;
    }
}

void PIT::DoDispose(void) {
    table.clear();

    COUNT_PITS--;

    //if(COUNT_PITS==0)
    //{
        //std::cout<<"PITS over."<<std::endl;
    //}
}

void PIT::update(Ptr<CCN_Name> key, Ptr<PTuple> re) {
    table[key] = re;
}

void PIT::erase(Ptr<CCN_Name> key) {
    table.erase(key);
}

uint32_t PIT::getSize() {
    return table.size();
}
}
