#include "ResultPrinter.h"

#include <iostream>
#include <sstream>

using std::stringstream;
using std::ofstream;

using namespace ns3;

ResultPrinter::ResultPrinter(map<uint32_t, Ptr<CcnModule> > nsNodeToModule, uint32_t gs, uint32_t exp, Ptr<Sender> s, vector<Ptr<Receiver> > receiver,std::string output)
{

    NS_LOG_UNCOND("-----------------Interest and data count-----------------");

    NS_LOG_UNCOND(CcnModule::RX_INTERESTS << " interests appeared." );
    NS_LOG_UNCOND(CcnModule::RX_DATA << " data appeared.");

    NS_LOG_UNCOND("-----------------Interest and data count-----------------");
    
        stringstream st;
        st << gs;

        stringstream st2;
        st2 << exp;

        ofstream file;

        file.open((output+"/gs-" + st.str() + "-experiment-" + 
                  st2.str()+ ".txt").c_str(), std::ios::app);

    for (map<uint32_t, Ptr<CcnModule> >::iterator it=nsNodeToModule.begin(); 
         it!= nsNodeToModule.end(); ++it){
    
        if ( it->second->getTXData()!=0)
            file << "router " << it->second->getNodeId() <<
                 " data_count " << it->second->getTXData() << std::endl;
    }
        file.close();

    NS_LOG_UNCOND("Sender got " << s->getInterests() << " interests.");
}

ResultPrinter::~ResultPrinter() {

}

void ResultPrinter::DoDispose(void) {

}
