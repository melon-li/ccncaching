#include "Sender.h"
#include <vector>
#include <string>
#include "stdlib.h"

using std::string;

namespace ns3 {

int Sender::COUNT_SENDERS = 0;

Sender::Sender(Ptr<CcnModule> ccnmIn) {
	COUNT_SENDERS++;
	ccnm = ccnmIn;
	localApp = CreateObject<LocalApp>();

	Callback<void, Ptr<CCN_Name> > interestCb = MakeCallback(&Sender::handleInterest, this);
	localApp->setInterestCallback(interestCb);

	Callback<void, Ptr<CCN_Name>, uint8_t*, uint32_t> dataCb = MakeCallback(&Sender::handleData, this);
	localApp->setDataCallback(dataCb);

	interests = 0;
}

Sender::~Sender() {
	ccnm = 0;
	localApp = 0;
	data.clear();
}

void Sender::DoDispose(void){
	ccnm = 0;
	localApp = 0;
	data.clear();

	COUNT_SENDERS--;

	if(COUNT_SENDERS==0)
	{
		std::cout<<"SENDERS over."<<std::endl;
	}
}

uint32_t Sender::getInterests()
{
	return interests;
}

Ptr<LocalApp> Sender::getLocalApp()
{
	return localApp;
}

void Sender::handleData(Ptr<CCN_Name>, uint8_t*, uint32_t){
	NS_ASSERT_MSG(false, "Sender should not receive any data.");
}

void Sender::insertData(Ptr< CCN_Name > name, Ptr<Packet> p)
{
	data[name] = p;
}

void Sender::AnnounceName(Ptr<CCN_Name> name) {
	//this->ccnm->announceName(name, this);
}

void Sender::handleInterest(Ptr<CCN_Name> ccnn) {
	interests++;
	//Time t = Seconds(this->waitingTime);
	Ptr<Packet> data = findData(ccnn);
	Simulator::Schedule(PicoSeconds(0), &Sender::SendData, this, ccnn, data);
}

void Sender::SendData(Ptr<CCN_Name> name, Ptr<Packet> data) {
	//std::cout<<Now().ToInteger(Time::MS)<<" Sender sending: "<<name->toString()<<std::endl;
	uint8_t *newBuff = (uint8_t*)malloc(data->GetSize());
	data->CopyData(newBuff, data->GetSize());
	this->ccnm->sendData(name, newBuff, data->GetSize());
	free(newBuff);
}

Ptr<Packet> Sender::findData(Ptr<CCN_Name> ccnn){
	Ptr<Packet> packet = data[ccnn];
	if (packet == 0){
		string nameString = ccnn->toString();
		packet = Create<Packet>((uint8_t*)nameString.c_str(), nameString.size());
		data[ccnn] = packet;
	}

	return packet;
}

TypeId Sender::GetTypeId(void) {
	static TypeId t = TypeId("SENDER");
	t.SetParent<Object>();
	return t;
}

TypeId Sender::GetInstanceTypeId(void) const {
	return GetTypeId();
}
}
