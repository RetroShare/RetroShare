#include "MonitoredGRouterClient.h"

const uint32_t MonitoredGRouterClient::GROUTER_CLIENT_SERVICE_ID_00 = 0x0111 ;

void MonitoredGRouterClient::receiveGRouterData(const GRouterKeyId& destination_key,const RsGRouterGenericDataItem *item)
{
	std::cerr << "received one global grouter item for key " << destination_key << std::endl;
}

void MonitoredGRouterClient::provideKey(const GRouterKeyId& key_id)
{
	_grouter->registerKey(key_id,GROUTER_CLIENT_SERVICE_ID_00,"test grouter address") ;

	std::cerr << "Registered new key " << key_id << " for service " << std::hex << GROUTER_CLIENT_SERVICE_ID_00 << std::dec << std::endl;
}

void MonitoredGRouterClient::sendMessage(const GRouterKeyId& destination_key_id) const
{
	RsGRouterGenericDataItem *item = new RsGRouterGenericDataItem ;
	item->data_size = 1000 + (RSRandom::random_u32()%1000) ;
	item->data_bytes = (unsigned char *)malloc(item->data_size) ;

	RSRandom::random_bytes(item->data_bytes,item->data_size) ;
	GRouterMsgPropagationId propagation_id ;

	_grouter->sendData(destination_key_id,GROUTER_CLIENT_SERVICE_ID_00,item,propagation_id) ;
}
