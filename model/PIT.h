#ifndef PIT_H_
#define PIT_H_

#include <map>
#include "CCN_Name.h"
#include "PTuple.h"
#include "local_app.h"

using std::map;

namespace ns3 {
class PIT_Key;
class PTuple;
class Bloomfilter;

class PIT: public Object {
public:
    static int COUNT_PITS;
    PIT();
    ~PIT();
    virtual void DoDispose(void);

    void update(Ptr<CCN_Name> key, Ptr<PTuple> re);

    Ptr<PTuple> check(Ptr<CCN_Name> key);

    void erase(Ptr<CCN_Name> key);

    uint32_t getSize();

//private:
    map<Ptr<CCN_Name>, Ptr<PTuple> > table;
};
}

#endif
