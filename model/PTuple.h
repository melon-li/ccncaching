#ifndef PTUPLE_H_
#define PTUPLE_H_

#include <vector>
#include "ns3/core-module.h"
#include "ns3/local_app.h"
#include <ns3/network-module.h>

using std::vector;

namespace ns3 {

class LocalApp;
class Bloomfilter;

class PTuple : public Object {
public:
    static int COUNT_TUPLES;

    PTuple();
    PTuple(vector<Ptr<NetDevice> >* devices );
    PTuple( vector<Ptr<LocalApp> >* apps);

    virtual ~PTuple();
    virtual void DoDispose(void);

    bool addLocalApp(Ptr<LocalApp> app);
    bool addDevice(Ptr<NetDevice> device);

    bool removeLocalApp(Ptr<LocalApp>);
    bool removeDevice(Ptr<NetDevice>);

    vector<Ptr<LocalApp> >& getLocalApps() {return r;}//apps that asked for data
    vector<Ptr<NetDevice> >& getDevices() {return d;}//devices that asked for data

private:
    vector<Ptr<LocalApp> > r;
    vector<Ptr<NetDevice> > d;
    vector<Ptr<LocalApp> >::iterator find(Ptr<LocalApp>);
    vector<Ptr<NetDevice> >::iterator find(Ptr<NetDevice>);
};

}
#endif
