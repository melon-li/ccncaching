#ifndef LOCAL_APP_H_
#define LOCAL_APP_H_

#include "ns3/core-module.h"
#include "CCN_Name.h"
#include <time.h>

namespace ns3{


class LocalApp : public Object{
public:
        static int COUNT_APPS;
        LocalApp(){COUNT_APPS++;};
        virtual ~LocalApp(){ }
        virtual void DoDispose()
        {
            COUNT_APPS--;

            if(COUNT_APPS==0)
            {
                std::cout<<"APPS over."<<std::endl;
            }
        }

        //called from CCN_Module
        void deliverInterest(Ptr<CCN_Name>);
        void deliverData(Ptr<CCN_Name>, uint8_t* , uint32_t );

        //callbacks set from applications
        void setInterestCallback(Callback<void, Ptr<CCN_Name> >& cb){ onInterestCallback = cb; }
        void setDataCallback(Callback<void, Ptr<CCN_Name>, uint8_t*, uint32_t>& cb){ onDataCallback = cb;}
        friend bool operator< (const Ptr<LocalApp>&, const Ptr<LocalApp>&);

private:
        //Ptr<CCN_Module> ccnModule;
        Callback<void, Ptr<CCN_Name> > onInterestCallback;
        Callback<void, Ptr<CCN_Name>, uint8_t*, uint32_t> onDataCallback;

        void doDeliverInterest(Ptr<CCN_Name> name){ onInterestCallback(name); }
        void doDeliverData(Ptr<CCN_Name> name, uint8_t* buffer, uint32_t buffLen){onDataCallback(name, buffer, buffLen); }

        static Time ONE_NS;

};

}

#endif /* LOCAL_APP_H_ */
