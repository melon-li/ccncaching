/*
 * ResultPrinter.h
 *
 *  Created on: Oct 8, 2013
 *      Author: Coaxial
 */

#ifndef RESULTPRINTER_H_
#define RESULTPRINTER_H_

#include "ns3/core-module.h"
#include "ns3/CcnModule.h"
#include "ns3/Sender.h"
#include "ns3/local_app.h"
#include "ns3/Receiver.h"

#include <vector>

class Receiver;

using std::vector;

namespace ns3 {

class Sender;
class CcnModule;

class ResultPrinter: public ns3::Object {

public:
    ResultPrinter(map<uint32_t, Ptr<CcnModule> > nsNodeToModule, uint32_t gs, uint32_t exp, Ptr<Sender> s,vector < Ptr < Receiver > > receiver,std::string output);
    ~ResultPrinter();
    virtual void DoDispose(void);

private:
    map<uint32_t, Ptr<CcnModule> > nsNodeToModule;
};

}

#endif /* RESULTPRINTER_H_ */
