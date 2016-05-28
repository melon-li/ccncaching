#include <vector>
#include <istream>
#include "Text.h"

using std::vector;
using std::pair;
using std::istringstream;

namespace ns3{

Ptr<Text> Text::text=0;

Text::Text()
{
    textContainer=new map<string,string*>();
}


void Text::DoDispose(void){
    if (textContainer){
        delete textContainer;
    }
    textContainer = 0;
}
Text::~Text()
{
    if (textContainer){
        delete textContainer;
    }
    textContainer = 0;
}

Ptr<Text> Text::getPtr()
{
    if(!text)
    {
        text=new Text;
    }

    return text;
}

Ptr<CCN_Name> Text::giveText(string* text)
{
    //std::cout<<"getting value "<<*text<<std::endl;
    vector<string*>* res=new vector<string*>();

    vector < string >* name=new vector < string >();
    istringstream s(*text);
    string piece;

    while(getline(s,piece,'/'))
    {
        name->push_back(piece);
    }

    for(unsigned i=0;i<name->size();i++)
    {
        if(textContainer->find(name->at(i))!=textContainer->end())
        {
            std::string* r=textContainer->find(name->at(i))->second;
            res->push_back(r);
        }
        else
        {
            pair<string,string* > pa (name->at(i),&(name->at(i)));//to & edo einai ok?
            textContainer->insert(pa);

            string* r2=textContainer->find(name->at(i))->second;
            res->push_back(r2);
        }
    }

    Ptr<CCN_Name> result=CreateObject<CCN_Name>(*res);
    //std::cout<<"QWEQWE"<<result->getValue()<<std::endl;
    return (result);

    return 0;
}

void Text::removeText(string* text)
{
    (*textContainer).erase(*text);
}
}
