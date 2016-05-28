/*
 * Trie.h
 *
 *  Created on: Sep 17, 2013
 *      Author: Coaxial
 */

#ifndef TRIE_H_
#define TRIE_H_

#include "TrieNode.h"
#include "CCN_Name.h"

namespace ns3 {

class Trie: public Object {
public:
    static int COUNT_TRIES;
    Trie();
    ~Trie();
    virtual void DoDispose(void);

    Ptr<TrieNode> longestPrefixMatch(Ptr<CCN_Name> name);
    bool put(Ptr<CCN_Name> name, Ptr<NetDevice> device);
    bool put(Ptr<CCN_Name> name, Ptr<LocalApp> localApp);

    bool hasData();

private:
    Ptr<TrieNode> root;
    Ptr<TrieNode> findNode(Ptr<CCN_Name>);

};

}

#endif /* TRIE_H_ */
