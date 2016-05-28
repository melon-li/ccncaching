#ifndef CCN_NAME_H_
#define CCN_NAME_H_

#include <string>
#include <vector>
#include <map>
#include <ns3/core-module.h>

using std::string;
using std::vector;
using std::map;
using std::pair;

namespace ns3 {

class PtrString: public Object {
public:
    PtrString(const char *str){ content = string(str); }
    PtrString(string &str){ content = str; }
    string getContent() const { return content; }
    uint32_t getContentLength()const { return content.length();}

    //operators
    friend bool operator< (const Ptr<PtrString> lhs, const Ptr<PtrString> rhs);
    friend bool operator<= (const Ptr<PtrString> lhs, const Ptr<PtrString> rhs);
    friend bool operator> (const Ptr<PtrString> lhs, const Ptr<PtrString> rhs);
    friend bool operator>= (const Ptr<PtrString> lhs, const Ptr<PtrString> rhs);
    friend bool operator== (const PtrString &lhs, const PtrString &rhs);
    friend bool operator!= (const PtrString &lhs, const PtrString &rhs);

private:
    string content;

};

class CCN_Name: public Object {
public:
    static int CCN_NAMES;
    CCN_Name(vector<string>&);
    ~CCN_Name();
    virtual void DoDispose(void);

    uint32_t size() const {return tokens.size();}
    Ptr<PtrString> getToken (uint32_t i) const {return tokens[i];}
    vector<string> getTokens() const;
    string getPrefix() const;
    string getID() const;
    string toString() const ;

    uint32_t serializedSize() const ;
    uint32_t serializeToBuffer(uint8_t *) const;
    static pair<Ptr<CCN_Name>, uint32_t> deserializeFromBuffer(uint8_t*);
    static void clearCachedNames(){cachedTokens.clear();}

    friend bool operator< (const Ptr<CCN_Name>& lhs, const Ptr<CCN_Name>& rhs);
    friend bool operator== (const Ptr<CCN_Name>& lhs, const Ptr<CCN_Name>& rhs);
    friend bool operator!= (const Ptr<CCN_Name>& lhs, const Ptr<CCN_Name>& rhs);

private:
    static map<string, Ptr<PtrString> > cachedTokens;
    static Ptr<PtrString> getTokenPtr(string &);
    vector<Ptr<PtrString> > tokens;

};

}

#endif /* CCN_NAME_H_ */
