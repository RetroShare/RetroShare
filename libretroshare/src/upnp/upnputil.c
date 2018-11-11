/*******************************************************************************
 * libretroshare/src/upnp: upnputil.c                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 * From MiniUPnPc, re-licensed with permission                                 *
 *                                                                             *
 * Copyright (c) 2005-2016, Thomas BERNARD                                     *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
//this file uses miniupnp
//From https://github.com/miniupnp/miniupnp/blob/master/miniupnpc/upnpc.c

#include "upnp/upnputil.h"

#if MINIUPNPC_API_VERSION >= -4//1.0 2008/02/18
#include "util/rstime.h"
#endif

/* protofix() checks if protocol is "UDP" or "TCP"
 * returns NULL if not */
const char * protofix(const char * proto)
{
	static const char proto_tcp[4] = { 'T', 'C', 'P', 0};
	static const char proto_udp[4] = { 'U', 'D', 'P', 0};
	int i, b;
	for(i=0, b=1; i<4; i++)
		b = b && (   (proto[i] == proto_tcp[i])
		             || (proto[i] == (proto_tcp[i] | 32)) );
	if(b)
		return proto_tcp;
	for(i=0, b=1; i<4; i++)
		b = b && (   (proto[i] == proto_udp[i])
		             || (proto[i] == (proto_udp[i] | 32)) );
	if(b)
		return proto_udp;
	return 0;
}

void DisplayInfos(struct UPNPUrls * urls,
                         struct IGDdatas * data)
{
	char externalIPAddress[40];
	char connectionType[64];
	char status[64];
	char lastconnerr[64];
	unsigned int uptime;
	unsigned int brUp, brDown;
	time_t timenow, timestarted;  // Don't use rstime_t here or ctime break on windows
	int r;
#if MINIUPNPC_API_VERSION >= -2//1.4 2010/12/09
	const char * servicetype = data->first.servicetype;
	const char * servicetype_CIF = data->CIF.servicetype;
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	const char * servicetype = data->servicetype;
	const char * servicetype_CIF = data->servicetype_CIF;
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
#endif//>=-7
#endif//>=-2

#if MINIUPNPC_API_VERSION >= -3//1.0 2009/04/17
	if(UPNP_GetConnectionTypeInfo(urls->controlURL,
	                              servicetype,
	                              connectionType) != UPNPCOMMAND_SUCCESS)
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	UPNP_GetConnectionTypeInfo(urls->controlURL, servicetype,
	                           connectionType);
	if(connectionType[0])
#endif//>=-7
#endif//>=-3
		printf("GetConnectionTypeInfo failed.\n");
	else
		printf("Connection Type : %s\n", connectionType);

#if MINIUPNPC_API_VERSION >= -4//1.0 2008/02/18
	if(UPNP_GetStatusInfo(urls->controlURL, servicetype,
	                      status, &uptime, lastconnerr) != UPNPCOMMAND_SUCCESS)
		printf("GetStatusInfo failed.\n");
	else
		printf("Status : %s, uptime=%us, LastConnectionError : %s\n",
		       status, uptime, lastconnerr);
	timenow = time(NULL);
	timestarted = timenow - uptime;
	printf("  Time started : %s", ctime(&timestarted));
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	UPNP_GetStatusInfo(urls->controlURL, servicetype,
	                   status, &uptime, lastconnerr);
	printf("Status : %s, uptime=%u, LastConnectionError : %s\n",
	       status, uptime, lastconnerr);
#endif//>=-7
#endif//>=-4

#if MINIUPNPC_API_VERSION >= -4//1.0 2008/02/18
	if(UPNP_GetLinkLayerMaxBitRates(urls->controlURL_CIF, servicetype_CIF,
	                                &brDown, &brUp) != UPNPCOMMAND_SUCCESS) {
		printf("GetLinkLayerMaxBitRates failed.\n");
	} else {
		printf("MaxBitRateDown : %u bps", brDown);
		if(brDown >= 1000000) {
			printf(" (%u.%u Mbps)", brDown / 1000000, (brDown / 100000) % 10);
		} else if(brDown >= 1000) {
			printf(" (%u Kbps)", brDown / 1000);
		}
		printf("   MaxBitRateUp %u bps", brUp);
		if(brUp >= 1000000) {
			printf(" (%u.%u Mbps)", brUp / 1000000, (brUp / 100000) % 10);
		} else if(brUp >= 1000) {
			printf(" (%u Kbps)", brUp / 1000);
		}
		printf("\n");
	}
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	UPNP_GetLinkLayerMaxBitRates(urls->controlURL_CIF,
	                             servicetype_CIF,
	                             &brDown, &brUp);
	printf("MaxBitRateDown : %u bps   MaxBitRateUp %u bps\n", brDown, brUp);
#endif//>=-7
#endif//>=-4

#if MINIUPNPC_API_VERSION >= -5//1.0 2007/12/19
	r = UPNP_GetExternalIPAddress(urls->controlURL,
	                              servicetype,
	                              externalIPAddress);
	if(r != UPNPCOMMAND_SUCCESS) {
		printf("GetExternalIPAddress failed. (errorcode=%d)\n", r);
	} else {
		printf("ExternalIPAddress = %s\n", externalIPAddress);
	}
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	UPNP_GetExternalIPAddress(urls->controlURL,
	                          servicetype,
	                          externalIPAddress);
	if(externalIPAddress[0])
		printf("ExternalIPAddress = %s\n", externalIPAddress);
	else
		printf("GetExternalIPAddress failed.\n");
#endif//>=-7
#endif//>=-4
}

void GetConnectionStatus(struct UPNPUrls * urls,
                                struct IGDdatas * data)
{
	unsigned int bytessent, bytesreceived, packetsreceived, packetssent;
#if MINIUPNPC_API_VERSION >= -2//1.4 2010/12/09
	const char * servicetype_CIF = data->CIF.servicetype;
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	const char * servicetype_CIF = data->servicetype_CIF;
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
#endif//>=-7
#endif//>=-2

	DisplayInfos(urls, data);
	bytessent = UPNP_GetTotalBytesSent(urls->controlURL_CIF, servicetype_CIF);
	bytesreceived = UPNP_GetTotalBytesReceived(urls->controlURL_CIF, servicetype_CIF);
	packetssent = UPNP_GetTotalPacketsSent(urls->controlURL_CIF, servicetype_CIF);
	packetsreceived = UPNP_GetTotalPacketsReceived(urls->controlURL_CIF, servicetype_CIF);
	printf("Bytes:   Sent: %8u\tRecv: %8u\n", bytessent, bytesreceived);
	printf("Packets: Sent: %8u\tRecv: %8u\n", packetssent, packetsreceived);
}

void ListRedirections(struct UPNPUrls * urls,
                             struct IGDdatas * data)
{
	int r;
	int i = 0;
	char index[6];
	char intClient[40];
	char intPort[6];
	char extPort[6];
	char protocol[4];
	char desc[80];
	char enabled[6];
	char rHost[64];
	char duration[16];
#if MINIUPNPC_API_VERSION >= -2//1.4 2010/12/09
	const char * servicetype = data->first.servicetype;
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	const char * servicetype = data->servicetype;
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
#endif//>=-7
#endif//>=-2

	/*unsigned int num=0;
	UPNP_GetPortMappingNumberOfEntries(urls->controlURL, data->servicetype, &num);
	printf("PortMappingNumberOfEntries : %u\n", num);*/
	do {
		snprintf(index, 6, "%d", i);
		rHost[0] = '\0'; enabled[0] = '\0';
		duration[0] = '\0'; desc[0] = '\0';
		extPort[0] = '\0'; intPort[0] = '\0'; intClient[0] = '\0';
		r = UPNP_GetGenericPortMappingEntry(urls->controlURL,
		                                    servicetype,
		                                    index,
		                                    extPort, intClient, intPort,
		                                    protocol, desc, enabled,
		                                    rHost, duration);
		if(r==0)
			printf("%02d - %s %5s->%s:%-5s"
			       "\tenabled=%s leaseDuration=%s\n"
			       "     desc='%s' rHost='%s'\n",
			       i, protocol, extPort, intClient, intPort,
			       enabled, duration,
			       desc, rHost);
		else
			printf("GetGenericPortMappingEntry() returned %d (%s)\n",
			       r, strupnperror(r));
		i++;
	} while(r==0);
}

/* Test function 
 * 1 - get connection type
 * 2 - get extenal ip address
 * 3 - Add port mapping
 * 4 - get this port mapping from the IGD */
int SetRedirectAndTest(struct UPNPUrls * urls,
                               struct IGDdatas * data,
                               const char * iaddr,
                               const char * iport,
                               const char * eport,
                               const char * proto,
                               const char * leaseDuration,
                               const char * description,
                               int addAny)
{
	char externalIPAddress[40];
	char intClient[40];
	char intPort[6];
	char reservedPort[6];
	char duration[16];
	int r;
	int ok = 1;
#if MINIUPNPC_API_VERSION >= -2//1.4 2010/12/09
	const char * servicetype = data->first.servicetype;
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	const char * servicetype = data->servicetype;
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
#endif
#endif

	if(!iaddr || !iport || !eport || !proto)
	{
		fprintf(stderr, "Wrong arguments\n");
		return 0;
	}
	proto = protofix(proto);
	if(!proto)
	{
		fprintf(stderr, "invalid protocol\n");
		return 0;
	}

#if MINIUPNPC_API_VERSION >= -5//1.0 2007/12/19
	r = UPNP_GetExternalIPAddress(urls->controlURL,
	                              servicetype,
	                              externalIPAddress);
	if(r != UPNPCOMMAND_SUCCESS)
		printf("GetExternalIPAddress failed. (errorcode=%d)\n", r);
	else
		printf("ExternalIPAddress = %s\n", externalIPAddress);
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	UPNP_GetExternalIPAddress(urls->controlURL,
	                          servicetype,
	                          externalIPAddress);
	if(externalIPAddress[0])
		printf("ExternalIPAddress = %s\n", externalIPAddress);
	else
		printf("GetExternalIPAddress failed.\n");
#endif//>=-7
#endif//>=-4

#if MINIUPNPC_API_VERSION >= 11
	if (addAny) {
		r = UPNP_AddAnyPortMapping(urls->controlURL, data->first.servicetype,
					   eport, iport, iaddr, description,
					   proto, 0, leaseDuration, reservedPort);
		if(r==UPNPCOMMAND_SUCCESS)
			eport = reservedPort;
		else
			printf("AddAnyPortMapping(%s, %s, %s) failed with code %d (%s)\n",
			       eport, iport, iaddr, r, strupnperror(r));
	} else
#endif
	{
#if MINIUPNPC_API_VERSION >= -1
		/* $Id: upnpcommands.h,v 1.30 2015/07/15 12:21:28 nanard Exp $ */
		/* $Id: upnpcommands.h,v 1.20 2011/02/15 11:13:22 nanard Exp $ */
		//UPNP_AddPortMapping(const char * controlURL, const char * servicetype,
		//                    const char * extPort,
		//                    const char * inPort,
		//                    const char * inClient,
		//                    const char * desc,
		//                    const char * proto,
		//                    const char * remoteHost,
		//                    const char * leaseDuration);
		r = UPNP_AddPortMapping(urls->controlURL, servicetype,
		                        eport, iport, iaddr, description, proto, NULL, NULL);
#else
#if MINIUPNPC_API_VERSION >= -3 //1.0 2009/04/17
		/* $Id: upnpcommands.h,v 1.18 2010/06/09 10:59:09 nanard Exp $ */
		/* $Id: upnpcommands.h,v 1.17 2009/04/17 21:21:19 nanard Exp $ */
		//UPNP_AddPortMapping(const char * controlURL, const char * servicetype,
		//                    const char * extPort,
		//                    const char * inPort,
		//                    const char * inClient,
		//                    const char * desc,
		//                    const char * proto,
		//                    const char * remoteHost);
		r = UPNP_AddPortMapping(urls->controlURL, servicetype,
		                        eport, iport, iaddr, description, proto, NULL);
#else
#if MINIUPNPC_API_VERSION >= -7//Before 1.0
		/* $Id: upnpcommands.h,v 1.14 2008/09/25 18:02:50 nanard Exp $ */
		/* $Id: upnpcommands.h,v 1.7 2006/07/09 12:00:54 nanard Exp $ */
		//UPNP_AddPortMapping(const char * controlURL, const char * servicetype,
		//                    const char * extPort,
		//                    const char * inPort,
		//                    const char * inClient,
		//                    const char * desc,
		//                    const char * proto);
		r = UPNP_AddPortMapping(urls->controlURL, servicetype,
		                        eport, iport, iaddr, description, proto);
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
#endif//>=-7
#endif//>=-3
#endif//>=-1

#if MINIUPNPC_API_VERSION >= -5//2007/12/19
		if(r!=UPNPCOMMAND_SUCCESS){
#else
#if MINIUPNPC_API_VERSION >= -7//Before 1.0
		if(r==0){
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
#endif//>=-7
#endif//>=-5
			printf("AddPortMapping(%s, %s, %s) failed and returns %d\n", eport, iport, iaddr, r);
			//this seems to trigger for unknown reasons sometimes.
			//rely on Checking it afterwards...
			//should check IP address then!
			ok = 0;
		}
	}

#if MINIUPNPC_API_VERSION >= 10//1.0 2006/09/04
	/* $Id: upnpcommands.h,v 1.30 2015/07/15 12:21:28 nanard Exp $ */
	/* $Id: upnpcommands.h,v 1.26 2014/01/31 13:18:26 nanard Exp $ */
	//UPNP_GetSpecificPortMappingEntry(const char * controlURL,
	//                                 const char * servicetype,
	//                                 const char * extPort,
	//                                 const char * proto,
	//                                 const char * remoteHost,
	//                                 char * intClient,
	//                                 char * intPort,
	//                                 char * desc,
	//                                 char * enabled,
	//                                 char * leaseDuration);
	r = UPNP_GetSpecificPortMappingEntry(urls->controlURL,
	                                     data->first.servicetype,
	                                     eport, proto, NULL/*remoteHost*/,
	                                     intClient, intPort, NULL/*desc*/,
	                                     NULL/*enabled*/, duration);
#else
#if MINIUPNPC_API_VERSION >= 6//1.0 2006/09/04
	/* $Id: upnpcommands.h,v 1.24 2012/03/05 19:42:47 nanard Exp $ */
	/* $Id: upnpcommands.h,v 1.22 2011/03/14 13:36:01 nanard Exp $ */
	//UPNP_GetSpecificPortMappingEntry(const char * controlURL,
	//                                 const char * servicetype,
	//                                 const char * extPort,
	//                                 const char * proto,
	//                                 char * intClient,
	//                                 char * intPort,
	//                                 char * desc,
	//                                 char * enabled,
	//                                 char * leaseDuration);
	r = UPNP_GetSpecificPortMappingEntry(urls->controlURL,
	                                     data->first.servicetype,
	                                     eport, proto,
	                                     intClient, intPort, NULL/*desc*/,
	                                     NULL/*enabled*/, duration);
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	/* $Id: upnpcommands.h,v 1.20 2011/02/15 11:13:22 nanard Exp $ */
	/* $Id: upnpcommands.h,v 1.7 2006/07/09 12:00:54 nanard Exp $ */
	//UPNP_GetSpecificPortMappingEntry(const char * controlURL,
	//                                 const char * servicetype,
	//                                 const char * extPort,
	//                                 const char * proto,
	//                                 char * intClient,
	//                                 char * intPort);
	UPNP_GetSpecificPortMappingEntry(urls->controlURL,
	                                 servicetype,
	                                 eport,
	                                 proto,
	                                 intClient,
	                                 intPort);
	if(intClient[0]) r = UPNPCOMMAND_SUCCESS;
#endif//>=-7
#endif//>=6
#endif//>=10

	if(r!=UPNPCOMMAND_SUCCESS) {
		printf("GetSpecificPortMappingEntry() failed with code %d (%s)\n",
		       r, strupnperror(r));
		ok = 0;
	} else if(intClient[0]) {
		printf("InternalIP:Port = %s:%s\n", intClient, intPort);
		printf("external %s:%s %s is redirected to internal %s:%s (duration=%s)\n",
		       externalIPAddress, eport, proto, intClient, intPort, duration);
	}

	if ((strcmp(iaddr, intClient) != 0) || (strcmp(iport, intPort) != 0))
	{
		printf("PortMappingEntry to wrong location! FAILED\n");
		printf("IP1:\"%s\"\n", iaddr);
		printf("IP2:\"%s\"\n", intClient);
		printf("PORT1:\"%s\"\n", iport);
		printf("PORT2:\"%s\"\n", intPort);
		ok = 0;
	}

	if (ok)
	{
		printf("uPnP Forward/Mapping Succeeded\n");
	}
	else
	{
		printf("uPnP Forward/Mapping Failed\n");
	}

	return ok;
}

int TestRedirect(struct UPNPUrls * urls,
                 struct IGDdatas * data,
                 const char * iaddr,
                 const char * iport,
                 const char * eport,
                 const char * proto)
{
	char externalIPAddress[40];
	char intClient[40];
	char intPort[6];
	char duration[16];
	int r = 0;
	int ok = 1;

	if(!iaddr || !iport || !eport || !proto)
	{
		fprintf(stderr, "Wrong arguments\n");
		return 0;
	}
	proto = protofix(proto);
	if(!proto)
	{
		fprintf(stderr, "invalid protocol\n");
		return 0;
	}

#if MINIUPNPC_API_VERSION >= -2//1.4 2010/12/09
	const char * servicetype = data->first.servicetype;
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	const char * servicetype = data->servicetype;
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
#endif
#endif

#if MINIUPNPC_API_VERSION >= 10//1.0 2006/09/04
	/* $Id: upnpcommands.h,v 1.30 2015/07/15 12:21:28 nanard Exp $ */
	/* $Id: upnpcommands.h,v 1.26 2014/01/31 13:18:26 nanard Exp $ */
	//UPNP_GetSpecificPortMappingEntry(const char * controlURL,
	//                                 const char * servicetype,
	//                                 const char * extPort,
	//                                 const char * proto,
	//                                 const char * remoteHost,
	//                                 char * intClient,
	//                                 char * intPort,
	//                                 char * desc,
	//                                 char * enabled,
	//                                 char * leaseDuration);
	r = UPNP_GetSpecificPortMappingEntry(urls->controlURL,
	                                     servicetype,
	                                     eport, proto, NULL/*remoteHost*/,
	                                     intClient, intPort, NULL/*desc*/,
	                                     NULL/*enabled*/, duration);
#else
#if MINIUPNPC_API_VERSION >= 6//1.0 2006/09/04
	/* $Id: upnpcommands.h,v 1.24 2012/03/05 19:42:47 nanard Exp $ */
	/* $Id: upnpcommands.h,v 1.22 2011/03/14 13:36:01 nanard Exp $ */
	//UPNP_GetSpecificPortMappingEntry(const char * controlURL,
	//                                 const char * servicetype,
	//                                 const char * extPort,
	//                                 const char * proto,
	//                                 char * intClient,
	//                                 char * intPort,
	//                                 char * desc,
	//                                 char * enabled,
	//                                 char * leaseDuration);
	r = UPNP_GetSpecificPortMappingEntry(urls->controlURL,
	                                     servicetype,
	                                     eport, proto,
	                                     intClient, intPort, NULL/*desc*/,
	                                     NULL/*enabled*/, duration);
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	/* $Id: upnpcommands.h,v 1.20 2011/02/15 11:13:22 nanard Exp $ */
	/* $Id: upnpcommands.h,v 1.7 2006/07/09 12:00:54 nanard Exp $ */
	//UPNP_GetSpecificPortMappingEntry(const char * controlURL,
	//                                 const char * servicetype,
	//                                 const char * extPort,
	//                                 const char * proto,
	//                                 char * intClient,
	//                                 char * intPort);
	UPNP_GetSpecificPortMappingEntry(urls->controlURL,
	                                 servicetype,
	                                 eport,
	                                 proto,
	                                 intClient,
	                                 intPort);
	if(intClient[0]) r = UPNPCOMMAND_SUCCESS;
#endif//>=-7
#endif//>=6
#endif//>=10

	if(r!=UPNPCOMMAND_SUCCESS) {
		printf("GetSpecificPortMappingEntry() failed with code %d (%s)\n",
		       r, strupnperror(r));
		ok = 0;
	} else if(intClient[0]) {
		printf("uPnP Check: InternalIP:Port = %s:%s\n", intClient, intPort);
		printf("external %s:%s %s is redirected to internal %s:%s (duration=%s)\n",
		       externalIPAddress, eport, proto, intClient, intPort, duration);
	}

	if (ok)
	{
		printf("uPnP Check: uPnP Forward/Mapping still Active\n");
	}
	else
	{
		printf("uPnP Check: Forward/Mapping has been Dropped\n");
	}

	return ok;
}

int
RemoveRedirect(struct UPNPUrls * urls,
               struct IGDdatas * data,
               const char * eport,
               const char * proto)
{
	if(!proto || !eport)
	{
		fprintf(stderr, "invalid arguments\n");
		return 0;
	}
	proto = protofix(proto);
	if(!proto)
	{
		fprintf(stderr, "protocol invalid\n");
		return 0;
	}
#if MINIUPNPC_API_VERSION >= -2//1.4 2010/12/09
	UPNP_DeletePortMapping(urls->controlURL, data->first.servicetype, eport, proto, NULL);
#else
#if MINIUPNPC_API_VERSION >= -3//1.3 2009/04/17
	UPNP_DeletePortMapping(urls->controlURL, data->servicetype, eport, proto, NULL);
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	UPNP_DeletePortMapping(urls->controlURL, data->servicetype, eport, proto);
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
#endif//>= -7
#endif//>= -3
#endif//>= -2

	return 1;
}


/* EOF */
