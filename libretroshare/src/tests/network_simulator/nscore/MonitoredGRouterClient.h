#pragma once

#include <grouter/grouterclientservice.h>

class MonitoredGRouterClient: public GRouterClientService
{
	public:
		static const uint32_t GROUTER_CLIENT_SERVICE_ID_00 ;

		// Derived from grouterclientservice.h
		//
		virtual void connectToGlobalRouter(p3GRouter *p) { _grouter = p ; p->registerClientService(GROUTER_CLIENT_SERVICE_ID_00,this) ; }
		virtual void receiveGRouterData(const GRouterKeyId& destination_key,const RsGRouterGenericDataItem *item);

		// Own functionality
		//
		void sendMessage(const GRouterKeyId& destination_key) const ;
		void provideKey(const GRouterKeyId& key) ;

	private:
		p3GRouter *_grouter ;
};

