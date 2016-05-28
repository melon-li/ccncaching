#include "TrieNode.h"

namespace ns3 {

int TrieNode::COUNT_TRIENODES = 0;

TrieNode::TrieNode(Ptr<PtrString> incoming_word) {
    COUNT_TRIENODES++;
    children = map<Ptr<PtrString>, Ptr<TrieNode> >();
    this->word = incoming_word;
}

TrieNode::~TrieNode() {
    children.clear();
    if (devices.size()!=0){
        devices.clear();
    }

    if (localApps.size()!=0){
        localApps.clear();
    }
}

void TrieNode::DoDispose(void){
    children.clear();
    if (devices.size()!=0){
        devices.clear();
    }

    if (localApps.size()!=0){
        localApps.clear();
    }

    COUNT_TRIENODES--;

    if(COUNT_TRIENODES==0)
    {
        std::cout<<"TRIENODES over."<<std::endl;
    }
}

bool TrieNode::addDevice(Ptr<NetDevice> d){
    /*if (!devices){
        devices = new vector<Ptr<NetDevice> >();
    }*/

    vector<Ptr<NetDevice> >::iterator iter;
    for(iter=devices.begin(); iter!=devices.end(); iter++){
        if ( *iter == d){
            return false;
        }
    }

    devices.push_back(d);
    return true;
}

bool TrieNode::addLocalApp(Ptr<LocalApp> app){
    /*if (!localApps){
        localApps = new vector<Ptr<LocalApp> >();
    }*/

    vector<Ptr<LocalApp> >::iterator iter;
    for(iter=localApps.begin(); iter!=localApps.end(); iter++){
        if ( *iter == app){
            return false;
        }
    }

    localApps.push_back(app);
    return true;
}

Ptr<TrieNode> TrieNode::setAndGetChildren(Ptr<PtrString> word){
    Ptr<TrieNode> retValue = 0;
    map<Ptr<PtrString>, Ptr<TrieNode> >::iterator find = children.find(word);
    if (find != children.end()){
        retValue = find->second;
    }else{
        Ptr<TrieNode> newTrieNode = CreateObject<TrieNode>(word);
        children[word] = newTrieNode;
        retValue = newTrieNode;
    }

    return retValue;
}

Ptr<TrieNode> TrieNode::getChild(Ptr<PtrString> word){
    map<Ptr<PtrString>, Ptr<TrieNode> >::iterator find = children.find(word);
    if (find == children.end()){
        return 0;
    }else{
        return find->second;
    }
}

bool TrieNode::hasData(){
    return (devices.size() > 0)
            || (localApps.size() > 0);
}

}
