#include "p3photoserviceV2.h"
#include "serialiser/rsphotov2items.h"

p3PhotoServiceV2::p3PhotoServiceV2(RsGeneralDataService* gds, RsNetworkExchangeService* nes)
	: RsGenExchange(gds, nes, new RsGxsPhotoSerialiser())
{

}
