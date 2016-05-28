#include "CCN_Name.h"
#include "utils.h"
#include <sstream>
#include <iostream>

using std::stringstream;
using std::cout;
using std::endl;

namespace ns3 {

int CCN_Name::CCN_NAMES = 0;

//operators
bool operator< (const Ptr<PtrString> lhs, const Ptr<PtrString> rhs){
    return lhs->getContent() < rhs->getContent();
}

bool operator<= (const Ptr<PtrString> lhs, const Ptr<PtrString> rhs){
    return lhs->getContent() <= rhs->getContent();
}

bool operator> (const Ptr<PtrString> lhs, const Ptr<PtrString> rhs){
    return lhs->getContent() > rhs->getContent();
}

bool operator>= (const Ptr<PtrString> lhs, const Ptr<PtrString> rhs){
    return lhs->getContent() >= rhs->getContent();
}

bool operator== (const PtrString &lhs, const PtrString &rhs){
    return lhs.getContent() == rhs.getContent();
}

bool operator!= (const PtrString &lhs, const PtrString &rhs){
    return lhs.getContent() != rhs.getContent();
}

map<string, Ptr<PtrString> > CCN_Name::cachedTokens = map<string, Ptr<PtrString> >();

Ptr<PtrString> CCN_Name::getTokenPtr(string &str){
    Ptr<PtrString> ptr = CCN_Name::cachedTokens[str];
    if (ptr == 0){
        ptr = CreateObject<PtrString>(str);
        CCN_Name::cachedTokens[str] = ptr;
    }

    return ptr;
}

CCN_Name::CCN_Name(vector<string>& names) {
    CCN_NAMES++;
    for(uint32_t i=0; i<names.size(); i++){
        Ptr<PtrString> ptrString = CCN_Name::getTokenPtr(names[i]);
        tokens.push_back(ptrString);
    }
}

CCN_Name::~CCN_Name() {
    tokens.clear();
}

vector<string> CCN_Name::getTokens() const{
    vector<string> v;
    for(uint32_t i=0; i<tokens.size(); i++){
        v.push_back(tokens[i]->getContent());
    }
    return v;
}

void CCN_Name::DoDispose(void) {
    tokens.clear();

    CCN_NAMES--;

    //if(CCN_NAMES==0)
    //{
        //std::cout<<"NAMES over."<<std::endl;
    //}
}

string CCN_Name::toString() const{
    stringstream sstream;
    for (uint8_t i=0; i<tokens.size(); i++){
        sstream << "/" << tokens[i]->getContent();
    }
    return sstream.str();
}


/**
 * returns a string of the prefix of name (all domains except the last one)
 * Exclussivly implemented for cache-module.
 * DOES NOT check the number of tokens!!
 */
string CCN_Name::getPrefix() const{
    stringstream sstream;
    for (uint8_t i=0; i<(tokens.size()-1); i++){
        sstream << "/" << tokens[i]->getContent();
    }
    return sstream.str();
}

/**
 * returns an int of the packets ID (last domain)
 * Exclussivly implemented for cache-module.
 * DOES NOT check the number of tokens!!
 */
string CCN_Name::getID() const{
    return tokens[tokens.size()-1]->getContent();
}


uint32_t CCN_Name::serializedSize() const {
    uint32_t length = 0;
    for (uint32_t i=0; i<tokens.size(); i++){
        length += 1 + tokens[i]->getContentLength();
    }
    return sizeof(uint16_t) + length*sizeof(char);
}

uint32_t CCN_Name::serializeToBuffer(uint8_t *buffer) const {
    string str = toString();
    uint16_t length = str.length();
    memcpy(buffer, &length, sizeof(uint16_t));
    memcpy((void*)(buffer + sizeof(uint16_t)), (void*)str.c_str(), length*sizeof(char));

    return sizeof(uint16_t) + length*sizeof(char);
}


pair<Ptr<CCN_Name>, uint32_t> CCN_Name::deserializeFromBuffer(uint8_t *buffer){

    uint16_t length = 0;
    memcpy((void*)&length, buffer, sizeof(uint16_t));

    char *str_buff = (char*)malloc(sizeof(char)*(length+1));
    memcpy((void *)str_buff, (void*)(buffer + sizeof(uint16_t)), sizeof(char)*length);

    //terminate with null
    str_buff[length] = '\0';
    //ignore starting '/'
    char *starting_pointer = str_buff[0] == '/'? str_buff + sizeof(char): str_buff;
    string str = string(starting_pointer);
    free(str_buff);

    vector<string> elems;
    Utils::split(str, '/', elems);
    Ptr<CCN_Name> name = CreateObject<CCN_Name>(elems);
    uint32_t readBytes = sizeof(uint16_t) + sizeof(char)*length;

    return pair<Ptr<CCN_Name>, uint32_t >(name, readBytes);
}

bool operator< (const Ptr<CCN_Name>& lhs, const Ptr<CCN_Name>& rhs){
    if (lhs->size() < rhs->size()){
        return true;
    }else if (lhs->size() > rhs->size()){
        return false;
    }else{
        for (uint32_t i=0; i<lhs->size(); i++){
            string lstr = lhs->getToken(i)->getContent();
            string rstr = rhs->getToken(i)->getContent();
            int diff = lstr.compare(rstr);
            if (diff < 0){
                return true;
            }else if (diff > 0){
                return false;
            }
        }
        return false;
    }
}

bool operator== (const Ptr<CCN_Name>& lhs, const Ptr<CCN_Name>& rhs){
    if (lhs->size() != rhs->size()){
        return false;
    }else{
        for (uint32_t i=0; i<lhs->size(); i++){
            if (lhs->getToken(i)->getContent().compare(rhs->getToken(i)->getContent()) != 0){
                return false;
            }
        }
        return true;
    }
}

bool operator!= (const Ptr<CCN_Name>& lhs, const Ptr<CCN_Name>& rhs){
    if (lhs->size() != rhs->size()){
        return true;
    }else{
        for (uint32_t i=0; i<lhs->size(); i++){
            if (lhs->getToken(i)->getContent().compare(rhs->getToken(i)->getContent()) != 0){
                return true;
            }
        }
        return false;
    }
}

}
