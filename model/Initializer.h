#ifndef INITIALIZER_H_
#define INITIALIZER_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "ns3/core-module.h"
#include "ns3/CcnModule.h"
#include "ns3/Parser.h"
#include <ns3/net-device.h>
#include "ns3/CcnModule.h"
#include "ns3/Sender.h"

using std::map;
using std::vector;

namespace ns3 {
class CcnModule;

class Initializer: public Object {
public:
    Initializer( Ptr<Parser>, int, map< uint32_t, Ptr <CcnModule> > );
    ~Initializer();
    void initializeFIBs();
    map<string, uint32_t> file_map;
    vector<string> create_workload(map<string, uint32_t> files, uint8_t seed);
    map<string, uint32_t> parseFileMap();
    vector<pair <string, uint32_t> > parseWorkload(uint32_t sseed);
    void initialize_FIBs_for_publisher_app( Ptr<Sender> _publisher_app);

private:
    void doesEveryModuleHaveANode();
    int dataOwner;
    uint32_t dataNum;
    Ptr<CcnModule> firstUnvisitedChild(Ptr<CcnModule>);
    Ptr<Parser> parser;
    map<uint32_t, bool> visited;
    Ptr<NetDevice> ndfinder(Ptr<Node> n1, Ptr<Node> n2);
    map<uint32_t, Ptr <CcnModule> > nsNodeIdToModule;

};
}

#endif
