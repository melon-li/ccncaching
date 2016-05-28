#ifndef TEXT_H_
#define TEXT_H_

#include <map>
#include <string>

#include "ns3/object.h"
#include "ns3/mod-module.h"

using std::map;
using std::string;

namespace ns3{

class Text  : public Object
{
    private:

    Text();
    static Ptr<Text> text;
    map<string, string*>* textContainer;

    public:
    ~Text();
    virtual void DoDispose(void);
    Ptr<CCN_Name> giveText(string* text);
    void removeText(string* text);
    static Ptr<Text> getPtr();
};
}
#endif
