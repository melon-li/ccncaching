#include "Trie.h"
#include "CCN_Name.h"
#include <vector>
#include <map>
#include <string>

using std::vector;
using std::string;
using std::map;

namespace ns3 {

int Trie::COUNT_TRIES = 0;

Trie::Trie() {
    COUNT_TRIES++;
    root = CreateObject<TrieNode>(CreateObject<PtrString>(""));
}

Trie::~Trie() {
    root = 0;
}

void Trie::DoDispose(void){
    root = 0;

    COUNT_TRIES--;

    if(COUNT_TRIES==0)
    {
        std::cout<<"COUNT_TRIES over."<<std::endl;
    }
}


Ptr<TrieNode> Trie::findNode(Ptr<CCN_Name> name){
    Ptr<TrieNode> handle = root;
    Ptr<TrieNode> currentNode = root;
    uint32_t currentPosition = 0;
    while(currentPosition < name->size()){
        Ptr<PtrString> token = name->getToken(currentPosition);
        Ptr<TrieNode> child = currentNode->setAndGetChildren(token);
        currentNode = child;
        currentPosition++;
    }

    return currentNode;
}

bool Trie::put(Ptr<CCN_Name> name, Ptr<NetDevice> device) {
    Ptr<TrieNode> node = findNode(name);
    return node->addDevice(device);
}

bool Trie::put(Ptr<CCN_Name> name, Ptr<LocalApp> localApp){
    Ptr<TrieNode> node = findNode(name);
    return node->addLocalApp(localApp);
}

Ptr<TrieNode> Trie::longestPrefixMatch(Ptr<CCN_Name> name) {
    Ptr<TrieNode> currentNode = root;
    Ptr<TrieNode> lastNodeWithData = 0;

    uint32_t currentPosition = 0;
    while(currentPosition < name->size()){
        Ptr<PtrString> token = name->getToken(currentPosition);
        Ptr<TrieNode> child = currentNode->getChild(token);
        if (child == 0){
            break;
        }

        if (child->hasData()){
            lastNodeWithData = child;
        }

        currentNode = child;
        currentPosition++;
    }

    if(lastNodeWithData==0)
    {
        std::cout<<"Trie::longestPrefixMatch | null before return"<<std::endl;
    }
    return lastNodeWithData;
}
}
