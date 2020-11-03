/*******************************************************************************
 * libretroshare/src/upnp: UPnPBase.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (c) 2004-2009 Marcelo Roberto Jimenez ( phoenix@amule.org )       *
 * Copyright (c) 2006-2009 aMule Team ( admin@amule.org / http://www.amule.org)*
 * Copyright (c) 2009-2010 Retroshare Team                                     *
 * Copyright (C) 2020      Gioacchino Mazzurco <gio@eigenlab.org>              *
 * Copyright (C) 2020      Asociación Civil Altermundi <info@altermundi.net>   *
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

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <iostream>
#include <cstdint>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>
#include <upnp/upnpdebug.h>

#include "util/rsthreads.h"
#include "util/rsdebuglevel2.h"
#include "util/rsmemory.h"
#include "util/rsdeprecate.h"
#include "util/rsnet.h"


#ifdef UPNP_C
	std::string stdEmptyString;
#else // UPNP_C
	extern std::string stdEmptyString;
#endif // UPNP_C

//#define UPNP_DEBUG 1

/**
 * Case insensitive std::string comparison
 */
bool stdStringIsEqualCI(
	const std::string &s1,
	const std::string &s2);


struct CUPnPPortMapping
{
	std::string m_ex_port;
	std::string m_in_port;
	std::string m_protocol;
	std::string m_enabled;
	std::string m_description;
	std::string m_key;
	
	CUPnPPortMapping(
	        uint16_t ex_port, uint16_t in_port, const std::string& protocol,
	        bool enabled, const std::string& description ):
	    m_ex_port(std::to_string(ex_port)), m_in_port(std::to_string(in_port)),
	    m_protocol(protocol), m_enabled(enabled ? ENABLED : DISABLED),
	    m_description(description), m_key(m_protocol + m_ex_port) {}

	const std::string& getKey() const{ return m_key; }

	static constexpr char PROTO_TCP[] = "TCP";
	static constexpr char PROTO_UDP[] = "UDP";

	static constexpr char ENABLED[] = "1";
	static constexpr char DISABLED[] = "0";

	RS_DEPRECATED const std::string& getExPort() const { return m_ex_port; }
	RS_DEPRECATED const std::string& getInPort() const { return m_in_port; }
	RS_DEPRECATED const std::string& getProtocol() const { return m_protocol; }
	RS_DEPRECATED const std::string& getDescription() const { return m_description; }
};

namespace std
{
template<> class default_delete<IXML_Document>
{
public:
	void operator()(IXML_Document* doc) { ixmlDocument_free(doc); }
};
}

struct RsLibUpnpXml
{
	// Convenience function so we don't have to write explicit calls 
	// to char2unicode every time
	std::string GetUPnPErrorMessage(int code) const;
	
	/// Convenience function to avoid repetitive processing of error messages
	static std::string processUPnPErrorMessage(
	        const std::string& messsage, int code, const DOMString errorString,
	        const IXML_Document* doc );

	// Processing response to actions
	void ProcessActionResponse(
	        const IXML_Document* RespDoc,
	        const std::string& actionName ) const;
	
	static rs_view_ptr<IXML_Element> GetRootElement(IXML_Document& doc);
	static rs_view_ptr<IXML_Element> GetFirstChild(IXML_Element& parent);
	static rs_view_ptr<IXML_Element> GetNextSibling(IXML_Element& sib);
	static const DOMString GetTag(IXML_Element& element);
	static const DOMString GetTextValue(IXML_Element& element);
	static const DOMString GetChildValueByTag(
	        IXML_Element& element, const DOMString tag);
	static rs_view_ptr<IXML_Element> GetFirstChildByTag(
	        IXML_Element& element, const DOMString tag);
	static rs_view_ptr<IXML_Element> GetNextSiblingByTag(
	        IXML_Element& element, const DOMString tag);
	static const DOMString GetAttributeByTag(
	        IXML_Element& element, const DOMString tag );

	static std::unique_ptr<IXML_Document> DownloadXmlDoc(const char* url);

	static constexpr char UPNP_DEVICE_IGW_VAL[] =
	        "urn:schemas-upnp-org:device:InternetGatewayDevice:1";

	static constexpr char UPNP_DEVICE_WAN[] =
	        "urn:schemas-upnp-org:device:WANDevice:1";

	static constexpr char UPNP_DEVICE_WAN_CONNECTION[] =
	        "urn:schemas-upnp-org:device:WANConnectionDevice:1";

	static constexpr char UPNP_DEVICE_LAN[] =
	        "urn:schemas-upnp-org:device:LANDevice:1";

	static constexpr char UPNP_SERVICE_LAYER3_FORWARDING[] =
	        "urn:schemas-upnp-org:service:Layer3Forwarding:1";

	static constexpr char UPNP_SERVICE_WAN_COMMON_INTERFACE_CONFIG[] =
	        "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1";

	static constexpr char UPNP_SERVICE_WAN_IP_CONNECTION[] =
	        "urn:schemas-upnp-org:service:WANIPConnection:1";

	static constexpr char UPNP_SERVICE_WAN_PPP_CONNECTION[] =
	        "urn:schemas-upnp-org:service:WANPPPConnection:1";

	static constexpr char EXTERNAL_ADDRESS_STATE_VAR_NAME[] =
	        "NewExternalIPAddress";

	static constexpr char GET_EXTERNAL_IP_ADDRESS_ACTION[] =
	        "GetExternalIPAddress";
};

RS_DEPRECATED_FOR(RsLibUpnpXml)
typedef RsLibUpnpXml CUPnPLib;

class CUPnPControlPoint;

/*
 * Even though we can retrieve the upnpLib handler from the upnpControlPoint,
 * we must pass it separetly at this point, because the class CUPnPControlPoint
 * must be declared after.
 *
 * CUPnPLib can only be removed from the constructor once we agree to link to
 * UPnPLib explicitly, making this dlopen() stuff unnecessary.
 */
template <typename T, char const *XML_ELEMENT_NAME, char const *XML_LIST_NAME>
struct CXML_List : std::map<const std::string, std::unique_ptr<T>>
{
	CXML_List(IXML_Element& parent, const std::string& url);
};


template <typename T, char const *XML_ELEMENT_NAME, char const *XML_LIST_NAME>
CXML_List<T, XML_ELEMENT_NAME, XML_LIST_NAME>::CXML_List(
        IXML_Element& parent, const std::string& url )
{
	rs_view_ptr<IXML_Element> elementList =
	        RsLibUpnpXml::GetFirstChildByTag(parent, XML_LIST_NAME);
	if(!elementList) return;

	for( IXML_Element* element =
	     RsLibUpnpXml::GetFirstChildByTag(*elementList, XML_ELEMENT_NAME);
	     element;
	     element = RsLibUpnpXml::GetNextSiblingByTag(*element, XML_ELEMENT_NAME) )
	{
		// Add a new element to the element map
		std::unique_ptr<T> upnpElement(new T(*element, url));
		(*this)[upnpElement->GetKey()] = std::move(upnpElement);
	}
}

extern const char s_argument[];
extern const char s_argumentList[];
extern const char s_action[];
extern const char s_actionList[];
extern const char s_allowedValue[];
extern const char s_allowedValueList[];
extern const char s_stateVariable[];
extern const char s_serviceStateTable[];
extern const char s_service[];
extern const char s_serviceList[];
extern const char s_device[];
extern const char s_deviceList[];

#ifdef UPNP_C
const char s_argument[] = "argument";
const char s_argumentList[] = "argumentList";
const char s_action[] = "action";
const char s_actionList[] = "actionList";
const char s_allowedValue[] = "allowedValue";
const char s_allowedValueList[] = "allowedValueList";
const char s_stateVariable[] = "stateVariable";
const char s_serviceStateTable[] = "serviceStateTable";
const char s_service[] = "service";
const char s_serviceList[] = "serviceList";
const char s_device[] = "device";
const char s_deviceList[] = "deviceList";
#endif // UPNP_C

class CUPnPArgument;
typedef CXML_List<CUPnPArgument, s_argument, s_argumentList> ArgumentList;
class CUPnPAction;
typedef CXML_List<CUPnPAction, s_action, s_actionList> ActionList;
class CUPnPStateVariable;
typedef CXML_List<CUPnPStateVariable, s_stateVariable, s_serviceStateTable> ServiceStateTable;
class CUPnPAllowedValue;
typedef CXML_List<CUPnPAllowedValue, s_allowedValue, s_allowedValueList> AllowedValueList;
class CUPnPService;
typedef CXML_List<CUPnPService, s_service, s_serviceList> ServiceList;
class CUPnPDevice;
typedef CXML_List<CUPnPDevice, s_device, s_deviceList> DeviceList;

struct CUPnPArgument
{
	CUPnPArgument(IXML_Element& argument, const std::string& /*SCPDURL*/):
	    m_name(RsLibUpnpXml::GetChildValueByTag(argument, "name")),
	    m_direction(RsLibUpnpXml::GetChildValueByTag(argument, "direction")),
	    m_retval(RsLibUpnpXml::GetFirstChildByTag(argument, "retval")),
	    m_relatedStateVariable(
	        RsLibUpnpXml::GetChildValueByTag(argument, "relatedStateVariable") )
	{}

	const std::string &GetName() const
		{ return m_name; }
	const std::string &GetDirection() const
		{ return m_direction; }
	bool GetRetVal() const
		{ return m_retval; }
	const std::string &GetRelatedStateVariable() const
		{ return m_relatedStateVariable; }
	const std::string &GetKey() const
		{ return m_name; }

	const std::string m_name;
	const std::string m_direction;
	bool m_retval;
	const std::string m_relatedStateVariable;
};


struct CUPnPAction
{
	CUPnPAction(IXML_Element& action, const std::string& SCPDURL):
	    mName(RsLibUpnpXml::GetChildValueByTag(action, "name")),
	    mArgumentList(action, SCPDURL) {}

	const std::string mName;
	ArgumentList mArgumentList;

	const std::string& GetKey() const { return mName; }

	RS_DEPRECATED
	const std::string &GetName() const
	    { return mName; }
	RS_DEPRECATED
	const ArgumentList &GetArgumentList() const
	    { return mArgumentList; }
};


struct CUPnPAllowedValue
{
	CUPnPAllowedValue(
	        IXML_Element& allowedValue, const std::string& /*SCPDURL*/ ):
	    m_allowedValue(RsLibUpnpXml::GetTextValue(allowedValue)) {}

	const std::string m_allowedValue;

	const std::string& GetKey() const
	{ return m_allowedValue; }
};


struct CUPnPStateVariable
{
	CUPnPStateVariable(
	        IXML_Element& stateVariable, const std::string& SCPDURL ):
	    m_AllowedValueList(stateVariable, SCPDURL),
	    m_name        (RsLibUpnpXml::GetChildValueByTag(stateVariable, "name")),
	    m_dataType    (RsLibUpnpXml::GetChildValueByTag(stateVariable, "dataType")),
	    m_defaultValue(RsLibUpnpXml::GetChildValueByTag(stateVariable, "defaultValue")),
	    m_sendEvents  (RsLibUpnpXml::GetAttributeByTag (stateVariable, "sendEvents"))
	{}

	AllowedValueList m_AllowedValueList;
	const std::string m_name;
	const std::string m_dataType;
	const std::string m_defaultValue;
	const std::string m_sendEvents;

	const std::string&GetKey() const { return m_name; }

	RS_DEPRECATED const std::string &GetNname() const { return m_name; }
	RS_DEPRECATED const std::string &GetDataType() const { return m_dataType; }
	RS_DEPRECATED const std::string &GetDefaultValue() const
	{ return m_defaultValue; }
	RS_DEPRECATED const AllowedValueList &GetAllowedValueList() const
	{ return m_AllowedValueList; }
};


struct CUPnPSCPD
{
	CUPnPSCPD(IXML_Element& scpd, const std::string& SCPDURL):
	    m_ActionList(scpd, SCPDURL), m_ServiceStateTable(scpd, SCPDURL),
	    m_SCPDURL(SCPDURL) {}

	ActionList m_ActionList;
	ServiceStateTable m_ServiceStateTable;
	const std::string m_SCPDURL;

	RS_DEPRECATED const ActionList &GetActionList() const
	{ return m_ActionList; }
	RS_DEPRECATED const ServiceStateTable &GetServiceStateTable() const
	{ return m_ServiceStateTable; }
};


struct CUPnPArgumentValue
{
	CUPnPArgumentValue() = default;
	CUPnPArgumentValue(const std::string& argument, const std::string& value);

	std::string mArgument;
	std::string mValue;

	RS_DEPRECATED const std::string &GetArgument() const { return mArgument; }
	RS_DEPRECATED const std::string &GetValue() const { return mValue; }
	RS_DEPRECATED const std::string &SetArgument(const std::string& argument)
	{ return mArgument = argument; }
	RS_DEPRECATED const std::string &SetValue(const std::string &value)
	{ return mValue = value; }
};


struct CUPnPService
{
	CUPnPService(IXML_Element& service, const std::string& URLBase);

	const std::string &GetServiceType() const
		{ return m_serviceType; }
	const std::string &GetServiceId() const
		{ return m_serviceId; }
	const std::string &GetSCPDURL() const
		{ return m_SCPDURL; }
	const std::string &GetAbsSCPDURL() const
		{ return m_absSCPDURL; }
	const std::string &GetControlURL() const
		{ return m_controlURL; }
	const std::string &GetEventSubURL() const
		{ return m_eventSubURL; }
	const std::string &GetAbsControlURL() const
		{ return m_absControlURL; }
	const std::string &GetAbsEventSubURL() const
		{ return m_absEventSubURL; }
	int GetTimeout() const
		{ return m_timeout; }
	void SetTimeout(int t)
		{ m_timeout = t; }
	int *GetTimeoutAddr()
		{ return &m_timeout; }
	char* GetSID() { return m_SID; }
	const char* GetSID() const { return m_SID; }
	void SetSID(const char *s)
		{ memcpy(m_SID, s, sizeof(Upnp_SID)); }
	const std::string &GetKey() const
		{ return m_serviceId; }
	bool IsSubscribed() const
		{ return m_SCPD.get() != NULL; }
	void SetSCPD(CUPnPSCPD *SCPD)
		{ m_SCPD.reset(SCPD); }

	const std::string m_serviceType;
	const std::string m_serviceId;
	const std::string m_SCPDURL;
	const std::string m_controlURL;
	const std::string m_eventSubURL;
	std::string m_absSCPDURL;
	std::string m_absControlURL;
	std::string m_absEventSubURL;
	int m_timeout;
	Upnp_SID m_SID;
	std::unique_ptr<CUPnPSCPD> m_SCPD;
	std::map<std::string, std::string> mPropertiesMap;
};


struct CUPnPDevice
{
	CUPnPDevice(IXML_Element& device, const std::string& URLBase);

	static constexpr char DEVICE_TYPE_TAG[] = "deviceType";

	const std::string &GetUDN() const
	    { return m_UDN; }
	const std::string &GetDeviceType() const
	    { return m_deviceType; }
	const std::string &GetFriendlyName() const
	    { return m_friendlyName; }
	const std::string &GetPresentationURL() const
	    { return m_presentationURL; }
	const std::string &GetKey() const
	    { return m_UDN; }

	// Please, lock these lists before use
	DeviceList m_DeviceList;
	ServiceList m_ServiceList;
	
	const std::string m_deviceType;
	const std::string m_friendlyName;
	const std::string m_manufacturer;
	const std::string m_manufacturerURL;
	const std::string m_modelDescription;
	const std::string m_modelName;
	const std::string m_modelNumber;
	const std::string m_modelURL;
	const std::string m_serialNumber;
	const std::string m_UDN;
	const std::string m_UPC;
	std::string m_presentationURL;
};


struct CUPnPRootDevice: CUPnPDevice
{
	const std::string m_URLBase;
	const std::string m_location;
	int m_expires;

	CUPnPRootDevice(
	        IXML_Element& rootDevice, const std::string& OriginalURLBase,
	        const std::string& FixedURLBase, const char* location, int expires );

	const std::string &GetURLBase() const
		{ return m_URLBase; }
	const std::string &GetLocation() const
		{ return m_location; }
	int GetExpires() const
		{ return m_expires; }
	void SetExpires(int expires)
		{ m_expires = expires; }
};


RS_DEPRECATED
typedef std::map<std::string, CUPnPRootDevice*> RootDeviceMap;

RS_DEPRECATED
typedef std::map<std::string, CUPnPService*>  ServiceMap;

RS_DEPRECATED
typedef std::map<std::string, CUPnPPortMapping> PortMappingMap;


class CUPnPControlPoint
{
public:
	explicit CUPnPControlPoint(uint16_t upnpUdpBroadcastPort);
	~CUPnPControlPoint();

	/** Used to get a pointer to an instance of the class.
	 * In CUPnPControlPoint::Callback, use this instead of passing the cookie
	 * for better type safety.
	 * Used also in service costructor to register the service.
	 */
	static rs_view_ptr<CUPnPControlPoint> instance;

	rs_view_ptr<const char> getInternalIpAddress();
	bool getExternalAddress(sockaddr_storage& addr);

	void Subscribe(CUPnPService &service);
	void Unsubscribe(const CUPnPService& service);
	bool RequestPortsForwarding(
		std::vector<CUPnPPortMapping> &upnpPortMapping);
	bool checkPortMapping(std::vector<CUPnPPortMapping> &upnpPortMapping)
	{
		// TODO
		return false;
	}
	bool DeletePortMappings(
		std::vector<CUPnPPortMapping> &upnpPortMapping);

	bool Execute(
	        CUPnPService& service, const std::string& ActionName,
	        const std::vector<CUPnPArgumentValue>& argValue ) const;

	const std::string GetStateVariable(
	        CUPnPService& service, const std::string& stateVariableName );

	UpnpClient_Handle GetUPnPClientHandle()	const
		{ return m_UPnPClientHandle; }

	bool GetIGWDeviceDetected() const { return !m_RootDeviceMap.empty(); }
	bool WanServiceDetected() const { return !m_ServiceMap.empty(); }
	void SetWanService(CUPnPService *service) { m_WanService = service; }

	// Callback function
	static int Callback(
	        Upnp_EventType eventType, const void* event, void* cookie );
	void OnEventReceived(const std::string &Sid,
	    int EventKey,
	    const IXML_Document* ChangedVariables);

	RS_DEPRECATED_FOR(m_ServiceMap)
	CUPnPService *m_WanService;


private:
	static constexpr char ADD_PORT_MAPPING_ACTION[] = "AddPortMapping";

	static constexpr char PORT_MAPPING_NUMBER_OF_ENTIES_KEY[] =
	        "PortMappingNumberOfEntries";

	static constexpr char URL_BASE_TAG[] = "URLBase";
	static constexpr char DEVICE_TAG[] = "device";

	void AddRootDevice(
	        IXML_Element& rootDevice, const std::string& urlBase,
	        const char* location, int expires );
	void RemoveRootDevice(const char* udn);
	void RefreshPortMappings();
	bool PrivateAddPortMapping(const CUPnPPortMapping& upnpPortMapping);
	bool PrivateDeletePortMapping(CUPnPPortMapping &upnpPortMapping);

	CUPnPLib m_upnpLib;
	UpnpClient_Handle m_UPnPClientHandle;
	std::map<std::string, CUPnPRootDevice*> m_RootDeviceMap;

	/** Store Internet Gateway and similar services so we can request port
	 * forwarding
	 * TODO: Is this class in charge of cleaning the memory?
	 */
	std::map<std::string, CUPnPService*> m_ServiceMap;

	std::map<std::string, CUPnPPortMapping> m_ActivePortMappingsMap;
	RsMutex mMutex; /// Protect internal data
	bool m_IGWDeviceDetected;

	RS_DEPRECATED
	RsMutex mWaitForSearchMutex;
};

// File_checked_for_headers
