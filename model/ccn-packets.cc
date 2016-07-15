#include "ccn-packets.h"

#include <cstdlib>
#include <ostream>
#include <map>

using std::ostringstream;

namespace ns3 {

uint8_t CCN_Packets::INTEREST = 0;
uint8_t CCN_Packets::DATA = 1;
uint8_t CCN_Data::buf[PKT_SIZE] = {0};


CCN_Interest::CCN_Interest(Ptr<CCN_Name> nameIn, float _betweenness) {
    name = nameIn;
    betweenness = _betweenness;
}

CCN_Interest::~CCN_Interest() {
    name = 0;
    betweenness = 0;
}

void CCN_Interest::DoDispose(void) {
    name = 0;
    betweenness = 0;
}

Ptr<Packet> CCN_Interest::serializeToPacket() const {
    uint8_t bufflen = sizeof(uint8_t) + name->serializedSize() + sizeof(float);// for betweenneess
    uint8_t *packetBuff = (uint8_t*) malloc(bufflen * sizeof(uint8_t));

    //write interest label
    memcpy(packetBuff, &CCN_Packets::INTEREST, sizeof(uint8_t));
    uint32_t offset = sizeof(uint8_t);
    //write betweenneess
    memcpy(packetBuff + offset, &betweenness, sizeof(float)* sizeof(uint8_t));
    offset += sizeof(float)* sizeof(uint8_t);
    //write name
    name->serializeToBuffer((uint8_t*) (packetBuff + offset));

    Ptr<Packet> p = Create<Packet>(packetBuff, bufflen);
    free(packetBuff);

    return p;
}

Ptr<CCN_Interest> CCN_Interest::deserializeFromPacket(
        const Ptr<Packet> packet) {
            
    uint32_t packetSize = packet->GetSize();
    
    uint8_t *buff = (uint8_t*) malloc(packetSize * sizeof(uint8_t));
    packet->CopyData(buff, packetSize);
    //interest label is implicitly read
    uint32_t offset = sizeof(uint8_t);
    //read betweenness
    float betw;
    memcpy(&betw, buff + offset,sizeof (float) * sizeof(uint8_t));
    offset+= sizeof (float) * sizeof(uint8_t);
    pair<Ptr<CCN_Name>, uint32_t> name = CCN_Name::deserializeFromBuffer(
            buff + offset);

    free(buff);

    Ptr<CCN_Interest> interest = CreateObject<CCN_Interest>(name.first, betw);
    //std::cout<<"Got interest "<<name.first->getID()<<" betw "<<betw<<std::endl;
    return interest;
}

bool operator==(const Ptr<CCN_Interest>& lhs, const Ptr<CCN_Interest>& rhs) {
    return lhs->getName() == rhs->getName();
}

CCN_Data::CCN_Data( Ptr<CCN_Name> nameIn,uint8_t* buffer, uint32_t buffsize, float _betweenness)
{
    name = nameIn;
    betweenness = _betweenness;
    if(buffer == NULL){
        uint32_t headerLength = sizeof(CCN_Packets::DATA)+ name->serializedSize()
                              + sizeof(float) /*betweenness*/+ sizeof(uint32_t);
        dataLength = PKT_SIZE - headerLength;
        data = buf;
        return;
    }
    dataLength = buffsize;
    data = (uint8_t*) malloc(buffsize * sizeof(uint8_t));
    memcpy(data, buffer, buffsize * sizeof(uint8_t));
}

CCN_Data::~CCN_Data() {

    name = 0;
    if (data != buf) {
        free(data);
        data = 0;
    }
}

void CCN_Data::DoDispose(void) {

    name = 0;
    betweenness = 0;
    if (data != buf) {
        free(data);
        data = 0;
    }
}

Ptr<Packet> CCN_Data::serializeToPacket() const {
    uint32_t packetBuffLen = sizeof(CCN_Packets::DATA)
             + name->serializedSize() + sizeof(float) /*betweenness*/+ sizeof(dataLength)
            + dataLength;

    //std::cout<< "packetBuffLen " << (int)packetBuffLen << std::endl;
    uint8_t *packetBuff = (uint8_t*) malloc(packetBuffLen);

    //CCN_Packets::DATA
    memcpy(packetBuff, &CCN_Packets::DATA, sizeof(uint8_t));
    uint32_t offset = sizeof(uint8_t);
    
    //betweenness
    memcpy(packetBuff + offset, &betweenness, sizeof(float)* sizeof(uint8_t));
    offset += sizeof(float)* sizeof(uint8_t);

    //name
    offset += name->serializeToBuffer((uint8_t*) (packetBuff + offset));
    //data
    memcpy((void*) (packetBuff + offset), &dataLength, sizeof(dataLength));
    offset += sizeof(dataLength);

    memcpy((void*) (packetBuff + offset), data, dataLength);

    Ptr<Packet> p = Create<Packet>(packetBuff, packetBuffLen);
    free(packetBuff);
    return p;
}

Ptr<CCN_Data> CCN_Data::deserializeFromPacket(const Ptr<Packet> packet) {
    uint8_t* buff = (uint8_t*) malloc(packet->GetSize() * sizeof(uint8_t));
    packet->CopyData(buff, packet->GetSize());

    //DATA label
    uint32_t offset = sizeof(uint8_t); //ignore CCN_Packets::DATA
    //read betweenness
    float betw;
    memcpy(&betw, buff + offset,sizeof (float) * sizeof(uint8_t));
    offset+= sizeof (float) * sizeof(uint8_t);
    //std::cout << "read name " << ccnNamePair.first->toString() << std::endl;
    pair<Ptr<CCN_Name>, uint32_t> ccnNamePair = CCN_Name::deserializeFromBuffer(
            buff + offset);
    offset += ccnNamePair.second;

    uint32_t datalen = 0;
    memcpy(&datalen, (void*) (buff + offset), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    //std::cout<< "d offset " << (int)offset << std::endl;

    Ptr<CCN_Data> data = CreateObject<CCN_Data>(
            ccnNamePair.first, (uint8_t*) (buff + offset), datalen, betw);
    free(buff);
    return data;
}

bool buffEquals(uint8_t *rhs, uint8_t* lhs, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        if (rhs[i] != lhs[i]) {
            return false;
        }
    }
    return true;
}

bool operator==(const Ptr<CCN_Data>& lhs, const Ptr<CCN_Data>& rhs) {
    return lhs->getName() == rhs->getName()

            && lhs->getLength() == rhs->getLength()
            && buffEquals(lhs->getData(), rhs->getData(), lhs->getLength());
}

}  // namespace ns3
