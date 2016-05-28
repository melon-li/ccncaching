
#ifndef PIT_KEY_H_
#define PIT_KEY_H_

#include "ns3/core-module.h"
#include <ns3/network-module.h>
#include "ns3/local_app.h"

namespace ns3 {

class LocalApp;
class Bloomfilter;

class PIT_Key : public Object {
public:

    PIT_Key(Ptr<CCN_Name>, Ptr<NetDevice> );
    PIT_Key(Ptr<CCN_Name>, Ptr<LocalApp> );
    virtual ~PIT_Key();
    virtual void DoDispose(void);
    bool addLocalApp(Ptr<LocalApp> app);
    bool removeLocalApp(Ptr<LocalApp>);
    Ptr<LocalApp> & getLocalApp() {return app;}
    Ptr<NetDevice> & getDevice() {return device;}
    Ptr<CCN_Name> getName(){return name;};
    std::string toString();
    friend bool operator< (const Ptr<PIT_Key>&, const Ptr<PIT_Key>&);

private:
    Ptr<CCN_Name> name;
    Ptr<NetDevice>  device;
    Ptr<LocalApp>  app;
};

}


#endif
