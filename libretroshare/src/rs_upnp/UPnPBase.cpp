/*******************************************************************************
 * libretroshare/src/upnp: UPnPBase.cpp                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (c) 2004-2009 Marcelo Roberto Jimenez ( phoenix@amule.org )       *
 * Copyright (c) 2006-2009 aMule Team ( admin@amule.org / http://www.amule.org)*
 * Copyright (c) 2009-2010 Retroshare Team                                     *
 * Copyright (C) 2019-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2020       Asociación Civil Altermundi <info@altermundi.net>  *
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

#define UPNP_C

#include "UPnPBase.h"

#include <stdio.h>
#include <string.h>
#include <sstream> // for std::istringstream
#include <algorithm>		// For transform()

#include "util/rsstring.h"
#include "rs_upnp/upnp18_retrocompat.h"
#include "util/rstime.h"
#include "util/cxx17retrocompat.h"
#include "util/rsnet.h"

#ifdef __GNUC__
	#if __GNUC__ >= 4
		#define REINTERPRET_CAST(x) reinterpret_cast<x>
	#endif
#endif
#ifndef REINTERPRET_CAST
	// Let's hope that function pointers are equal in size to data pointers
	#define REINTERPRET_CAST(x) (x)
#endif


#if __cplusplus < 201703L
/* Solve weird undefined reference error with C++ < 17 see:
 * https://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char
 */

/*static*/ constexpr char RsLibUpnpXml::UPNP_DEVICE_IGW_VAL[];

/*static*/ constexpr char RsLibUpnpXml::UPNP_DEVICE_WAN[];

/*static*/ constexpr char RsLibUpnpXml::UPNP_DEVICE_WAN_CONNECTION[];

/*static*/ constexpr char RsLibUpnpXml::UPNP_DEVICE_LAN[];

/*static*/ constexpr char RsLibUpnpXml::UPNP_SERVICE_LAYER3_FORWARDING[];

/*static*/ constexpr char RsLibUpnpXml::UPNP_SERVICE_WAN_COMMON_INTERFACE_CONFIG[];

/*static*/ constexpr char RsLibUpnpXml::UPNP_SERVICE_WAN_IP_CONNECTION[];

/*static*/ constexpr char RsLibUpnpXml::UPNP_SERVICE_WAN_PPP_CONNECTION[];

/*static*/ constexpr char CUPnPControlPoint::ADD_PORT_MAPPING_ACTION[];

/*static*/ constexpr char CUPnPControlPoint::PORT_MAPPING_NUMBER_OF_ENTIES_KEY[];

/*static*/ constexpr char CUPnPControlPoint::URL_BASE_TAG[];

/*static*/ constexpr char CUPnPControlPoint::DEVICE_TAG[];

/*static*/ constexpr char CUPnPDevice::DEVICE_TYPE_TAG[];
#endif


/**
 * Case insensitive std::string comparison
 */
bool stdStringIsEqualCI(const std::string &s1, const std::string &s2)
{
	std::string ns1(s1);
	std::string ns2(s2);
	std::transform(ns1.begin(), ns1.end(), ns1.begin(), tolower);
	std::transform(ns2.begin(), ns2.end(), ns2.begin(), tolower);
	return ns1 == ns2;
}




std::string CUPnPLib::GetUPnPErrorMessage(int code) const
{ return UpnpGetErrorMessage(code); }


/*static*/ std::string RsLibUpnpXml::processUPnPErrorMessage(
        const std::string& message, int errorCode,
        const DOMString errorString, const IXML_Document* doc)
{
	/* remove unused parameter warnings */
	(void) message;
	(void) errorCode;
	(void) errorString;
	(void) doc;

	std::string msg;
#ifdef UPNP_DEBUG
	if (errorString == NULL || *errorString == 0) {
		errorString = "Not available";
	}
	if (errorCode > 0) {
		std::cerr << "CUPnPLib::processUPnPErrorMessage() Error: " <<
			message <<
			": Error code :'";
		if (doc) {
			CUPnPError e(*this, doc);
			std::cerr << e.getErrorCode() <<
				"', Error description :'" <<
				e.getErrorDescription() <<
				"'.";
		} else {
			std::cerr << errorCode <<
				"', Error description :'" <<
				errorString <<
				"'.";
		}
		std::cerr << std::endl;
	} else {
		std::cerr << "CUPnPLib::processUPnPErrorMessage() Error: " <<
			message <<
			": UPnP SDK error: " <<
			GetUPnPErrorMessage(errorCode) << 
			" (" << errorCode << ").";
		std::cerr << std::endl;
	}
#endif
	return msg;
}


/*!
 * \brief Returns the root node of a given document.
 */
/*static*/ rs_view_ptr<IXML_Element> RsLibUpnpXml::GetRootElement(
        IXML_Document& doc )
{
	IXML_Element* root = REINTERPRET_CAST(IXML_Element *)(
	            ixmlNode_getFirstChild(REINTERPRET_CAST(IXML_Node *)(&doc)) );
	return root;
}


/*!
 * \brief Returns the first child of a given element.
 */
/*static*/ rs_view_ptr<IXML_Element> RsLibUpnpXml::GetFirstChild(
        IXML_Element& parent )
{
	IXML_Node *node = REINTERPRET_CAST(IXML_Node *)(&parent);
	IXML_Node *child = ixmlNode_getFirstChild(node);
	return REINTERPRET_CAST(IXML_Element *)(child);
}


/*!
 * \brief Returns the next sibling of a given child.
 */
/*static*/ rs_view_ptr<IXML_Element> RsLibUpnpXml::GetNextSibling(
        IXML_Element& sib )
{
	IXML_Node* node = REINTERPRET_CAST(IXML_Node *)(&sib);
	IXML_Node* sibling = ixmlNode_getNextSibling(node);
	return REINTERPRET_CAST(IXML_Element *)(sibling);
}


/*!
 * \brief Returns the element tag (name)
 */
/*static*/ const DOMString RsLibUpnpXml::GetTag(IXML_Element& element)
{
	IXML_Node *node = REINTERPRET_CAST(IXML_Node *)(&element);
	const DOMString tag = ixmlNode_getNodeName(node);
	return tag;
}


/*!
 * \brief Returns the TEXT node value of the current node.
 */
/*static*/ const DOMString RsLibUpnpXml::GetTextValue(IXML_Element& element)
{
	IXML_Node *text = ixmlNode_getFirstChild(
	            REINTERPRET_CAST(IXML_Node*)(&element) );
	return ixmlNode_getNodeValue(text);
}


/*!
 * \brief Returns the TEXT node value of the first child matching tag.
 */
/*static*/ const DOMString RsLibUpnpXml::GetChildValueByTag(
        IXML_Element& element, const DOMString tag)
{
	IXML_Element* child = GetFirstChildByTag(element, tag);
	if(child) return GetTextValue(*child);
	return nullptr;
}


/*!
 * \brief Returns the first child element that matches the requested tag or
 * NULL if not found.
 */
/*static*/ rs_view_ptr<IXML_Element> RsLibUpnpXml::GetFirstChildByTag(
        IXML_Element& element, const DOMString tag )
{
	if(!tag) return nullptr;

	IXML_Node *node = REINTERPRET_CAST(IXML_Node *)(&element);
	IXML_Node *child = ixmlNode_getFirstChild(node);
	const DOMString childTag = ixmlNode_getNodeName(child);
	while(child && childTag && strcmp(tag, childTag)) {
		child = ixmlNode_getNextSibling(child);
		childTag = ixmlNode_getNodeName(child);
	}

	return REINTERPRET_CAST(IXML_Element *)(child);
}


/*!
 * \brief Returns the next sibling element that matches the requested tag. Should be
 * used with the return value of Element_GetFirstChildByTag().
 */
/*static*/ rs_view_ptr<IXML_Element> RsLibUpnpXml::GetNextSiblingByTag(
        IXML_Element& element, const DOMString tag )
{
	if(!tag) return nullptr;

	IXML_Node *child = REINTERPRET_CAST(IXML_Node *)(&element);
	const DOMString childTag = nullptr;
	do
	{
		child = ixmlNode_getNextSibling(child);
		childTag = ixmlNode_getNodeName(child);
	}
	while(child && childTag && strcmp(tag, childTag));

	return REINTERPRET_CAST(IXML_Element *)(child);
}

/*static*/ const DOMString RsLibUpnpXml::GetAttributeByTag(
        IXML_Element& element, const DOMString tag)
{
	rs_owner_ptr<IXML_NamedNodeMap> aMap =
	        ixmlNode_getAttributes(REINTERPRET_CAST(IXML_Node *)(&element));

	rs_view_ptr<IXML_Node> attribute = ixmlNamedNodeMap_getNamedItem(aMap, tag);
	ixmlNamedNodeMap_free(aMap);

	return ixmlNode_getNodeValue(attribute);
}

std::unique_ptr<IXML_Document> RsLibUpnpXml::DownloadXmlDoc(const char* url)
{
	rs_owner_ptr<IXML_Document> tmpDoc = nullptr;
	int ret = UpnpDownloadXmlDoc(url, &tmpDoc);

	if(!tmpDoc || ret != UPNP_E_SUCCESS)
	{
		RsErr() << __PRETTY_FUNCTION__ << " UpnpDownloadXmlDoc ret: "
		        << ret << " tmpDoc: " << tmpDoc << std::endl;
		return nullptr;
	}

	return std::unique_ptr<IXML_Document>(tmpDoc);
}



CUPnPArgumentValue::CUPnPArgumentValue(
        const std::string& argument, const std::string& value ):
    mArgument(argument), mValue(value) {}

CUPnPService::CUPnPService(IXML_Element& service, const std::string& URLBase):
    m_serviceType(RsLibUpnpXml::GetChildValueByTag(service, "serviceType")),
    m_serviceId  (RsLibUpnpXml::GetChildValueByTag(service, "serviceId")),
    m_SCPDURL    (RsLibUpnpXml::GetChildValueByTag(service, "SCPDURL")),
    m_controlURL (RsLibUpnpXml::GetChildValueByTag(service, "controlURL")),
    m_eventSubURL(RsLibUpnpXml::GetChildValueByTag(service, "eventSubURL")),
    m_timeout(1801), m_SID{0}
{
	m_absSCPDURL.resize(URLBase.length() + m_SCPDURL.length() + 1, 0);
	int errcode =
	        UpnpResolveURL(URLBase.c_str(), m_SCPDURL.c_str(), &m_absSCPDURL[0]);
	if( errcode != UPNP_E_SUCCESS )
	{
		RsErr() << __PRETTY_FUNCTION__ << " Error resolving scpdURL from |"
		        << URLBase << "|" << m_SCPDURL << "|" << std::endl;
		m_absSCPDURL.clear();
	}

	m_absControlURL.resize(URLBase.length() + m_controlURL.length() + 1, 0);
	errcode = UpnpResolveURL(
	            URLBase.c_str(), m_controlURL.c_str(), &m_absControlURL[0] );
	if( errcode != UPNP_E_SUCCESS )
	{
		RsErr() << __PRETTY_FUNCTION__ << " Error resolving controlURL from "
		        << "|" << URLBase << "|" << m_controlURL << "|" << std::endl;
		m_absControlURL.clear();
	}

	m_absEventSubURL.resize(URLBase.length() + m_eventSubURL.length() + 1, 0);
	errcode = UpnpResolveURL(
	            URLBase.c_str(), m_eventSubURL.c_str(), &m_absEventSubURL[0]);
	if( errcode != UPNP_E_SUCCESS )
	{
		RsErr() << __PRETTY_FUNCTION__ << " Error generating eventURL from "
		        << "|" << URLBase << "|" << m_eventSubURL << "|" << std::endl;
		m_absEventSubURL.clear();
	}

	if( m_serviceType == RsLibUpnpXml::UPNP_SERVICE_WAN_IP_CONNECTION ||
	    m_serviceType == RsLibUpnpXml::UPNP_SERVICE_WAN_PPP_CONNECTION )
	{
		CUPnPControlPoint::instance->Subscribe(*this);
#if 0
//#warning Delete this code on release.
		} else {
#ifdef UPNP_DEBUG
			std::cerr << "WAN service detected again: '" <<
				m_serviceType <<
				"'. Will only use the first instance.";
			std::cerr << std::endl;
#endif
		}
#endif
	} else {
#ifdef UPNP_DEBUG
		std::cerr << "CUPnPService::CUPnPService() Uninteresting service detected: '" <<
			m_serviceType << "'. Ignoring.";
		std::cerr << std::endl;
#endif
	}
}

bool CUPnPControlPoint::Execute(
        CUPnPService& service, const std::string& ActionName,
        const std::vector<CUPnPArgumentValue>& ArgValue ) const
{
	RS_DBG1(ActionName);

	if(!service.m_SCPD.get())
	{
		RsErr() << __PRETTY_FUNCTION__ << " Service without SCPD Document, "
		        << "cannot execute action '" << ActionName << " for service "
		        << service.GetServiceType() << std::endl;
		print_stacktrace();
		return false;
	}

	// Check for correct action name
	ActionList::const_iterator itAction =
	        service.m_SCPD->m_ActionList.find(ActionName);
	if(itAction == service.m_SCPD->m_ActionList.end())
	{
		RsErr() << __PRETTY_FUNCTION__ << " invalid action name "
		        << ActionName << " for service " << service.GetServiceType()
		        << std::endl;
		print_stacktrace();
		return false;
	}

	// Check for correct Argument/Value pairs
	const CUPnPAction& action = *(itAction->second);
	for (unsigned int i = 0; i < ArgValue.size(); ++i)
	{
		ArgumentList::const_iterator itArg =
		        action.mArgumentList.find(ArgValue[i].mArgument);
		if (itArg == action.mArgumentList.end())
		{
			RsErr() << __PRETTY_FUNCTION__ << " Invalid argument name "
			        << ArgValue[i].mArgument << " for action "
			        << action.GetName() << " for service "
			        << service.GetServiceType() << std::endl;
			print_stacktrace();
			return false;
		}

		const CUPnPArgument& argument = *(itArg->second);
		if( tolower(argument.GetDirection()[0]) != 'i' ||
		    tolower(argument.GetDirection()[1]) != 'n' )
		{
			RsErr() << __PRETTY_FUNCTION__ << " Invalid direction for argument "
			        << ArgValue[i].mArgument
			        << " for action " << action.GetName()
			        << " for service " << service.m_serviceType << std::endl;
			print_stacktrace();
			return false;
		}

		const std::string& relatedStateVariableName =
			argument.GetRelatedStateVariable();
		if (!relatedStateVariableName.empty())
		{
			auto itSVT = std::as_const(service.m_SCPD->m_ServiceStateTable)
			        .find(relatedStateVariableName);

			if (itSVT == service.m_SCPD->m_ServiceStateTable.end())
			{
				RsErr() << __PRETTY_FUNCTION__ << " Inconsistent Service State "
				        << "Table, did not find " << relatedStateVariableName
				        << " for argument " << argument.GetName()
				        << " for action " << action.GetName()
				        << " for service " << service.m_serviceType << std::endl;
				print_stacktrace();
				return false;
			}

			const CUPnPStateVariable& stateVariable = *(itSVT->second);
			if( !stateVariable.GetAllowedValueList().empty() &&
			        stateVariable.GetAllowedValueList().find(ArgValue[i].mValue)
			        == stateVariable.GetAllowedValueList().end() )
			{
				RsErr() << __PRETTY_FUNCTION__
				        << " Value not allowed " << ArgValue[i].mValue
				        << " for state variable " << relatedStateVariableName
				        << " for argument " << argument.GetName()
				        << " for action " << action.GetName()
				        << " for service " << service.m_serviceType << std::endl;
				print_stacktrace();
				return false;
			}
		}
	}

	// Everything is ok, make the action
	IXML_Document* ActionDoc = nullptr; // G10h4ck do we own this?
	if(ArgValue.size())
	{
		for (unsigned int i = 0; i < ArgValue.size(); ++i)
		{
			int ret = UpnpAddToAction(
			            &ActionDoc,
			            action.GetName().c_str(), service.m_serviceType.c_str(),
			            ArgValue[i].mArgument.c_str(),
			            ArgValue[i].mValue.c_str() );

			if(ret != UPNP_E_SUCCESS)
			{
				RsErr() << __PRETTY_FUNCTION__
				        << " UpnpAddToAction return error: " << ret << std::endl;
				print_stacktrace();
				return false;
			}
		}
	}
	else
	{
		ActionDoc = UpnpMakeAction(
		            action.GetName().c_str(), service.m_serviceType.c_str(),
		            0, nullptr );
		if (!ActionDoc)
		{
			RsErr() << __PRETTY_FUNCTION__ <<  "UpnpMakeAction returned nullptr"
			        << std::endl;
			print_stacktrace();
			return false;
		}
	}

	// Send the action asynchronously
	UpnpSendActionAsync(
	            GetUPnPClientHandle(),
	            service.GetAbsControlURL().c_str(),
	            service.GetServiceType().c_str(),
	            nullptr, ActionDoc,
	            reinterpret_cast<Upnp_FunPtr>(&CUPnPControlPoint::Callback),
	            nullptr );
	return true;
}


const std::string CUPnPControlPoint::GetStateVariable(
        CUPnPService& service,
        const std::string& variableName )
{
	{
		RS_STACK_MUTEX(mMutex);
		auto it = std::as_const(service.mPropertiesMap).find(variableName);
		if(it != service.mPropertiesMap.end()) return (*it).second;
	}

	/* property map is not populated with the specified value.
	 * try to get it with an event */

	DOMString tVarVal;
	UpnpGetServiceVarStatus(
	            GetUPnPClientHandle(), service.GetAbsControlURL().c_str(),
	            variableName.c_str(), &tVarVal);

	if(tVarVal) return tVarVal;

	/* If var hasn't been retrieved yet let's wait for the event */
	rstime_t timeout = time(nullptr) + 7;
	while(time(nullptr) < timeout)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		{
			RS_STACK_MUTEX(mMutex);
			auto it = std::as_const(service.mPropertiesMap).find(variableName);
			if(it != service.mPropertiesMap.end()) return it->second;
		}
	}

	return "";
}


CUPnPDevice::CUPnPDevice(IXML_Element& device, const std::string& URLBase):
    m_DeviceList(device, URLBase),
    m_ServiceList(device, URLBase),
    m_deviceType(RsLibUpnpXml::GetChildValueByTag(device, DEVICE_TYPE_TAG)),
    m_friendlyName(RsLibUpnpXml::GetChildValueByTag(device, "friendlyName")),
    m_manufacturer(RsLibUpnpXml::GetChildValueByTag(device, "manufacturer")),
    m_manufacturerURL(RsLibUpnpXml::GetChildValueByTag(device, "manufacturerURL")),
    m_modelDescription(RsLibUpnpXml::GetChildValueByTag(device, "modelDescription")),
    m_modelName(RsLibUpnpXml::GetChildValueByTag(device, "modelName")),
    m_modelNumber(RsLibUpnpXml::GetChildValueByTag(device, "modelNumber")),
    m_modelURL(RsLibUpnpXml::GetChildValueByTag(device, "modelURL")),
    m_serialNumber(RsLibUpnpXml::GetChildValueByTag(device, "serialNumber")),
    m_UDN(RsLibUpnpXml::GetChildValueByTag(device, "UDN")),
    m_UPC(RsLibUpnpXml::GetChildValueByTag(device, "UPC")),
    m_presentationURL(RsLibUpnpXml::GetChildValueByTag(device, "presentationURL"))
{
	int presURLlen = strlen(URLBase.c_str()) +
		strlen(m_presentationURL.c_str()) + 2;
	std::vector<char> vpresURL(presURLlen);
	char* presURL = &vpresURL[0];
	int errcode = UpnpResolveURL(
		URLBase.c_str(),
		m_presentationURL.c_str(),
		presURL);
	if (errcode != UPNP_E_SUCCESS) {
#ifdef UPNP_DEBUG
		std::cerr << "CUPnPDevice::CUPnPDevice() Error generating presentationURL from " <<
			"|" << URLBase << "|" <<
			m_presentationURL << "|.";
		std::cerr << std::endl;
#endif
	} else {
		m_presentationURL = presURL;
	}
	
#ifdef UPNP_DEBUG
	std::cerr <<	"CUPnPDevice::CUPnPDevice() \n    Device: "                <<
		"\n        friendlyName: "      << m_friendlyName <<
		"\n        deviceType: "        << m_deviceType <<
		"\n        manufacturer: "      << m_manufacturer <<
		"\n        manufacturerURL: "   << m_manufacturerURL <<
		"\n        modelDescription: "  << m_modelDescription <<
		"\n        modelName: "         << m_modelName <<
		"\n        modelNumber: "       << m_modelNumber <<
		"\n        modelURL: "          << m_modelURL <<
		"\n        serialNumber: "      << m_serialNumber <<
		"\n        UDN: "               << m_UDN <<
		"\n        UPC: "               << m_UPC <<
		"\n        presentationURL: "   << m_presentationURL
		<< std::endl;
#endif
}


CUPnPRootDevice::CUPnPRootDevice(
        IXML_Element& rootDevice, const std::string& OriginalURLBase,
        const std::string& FixedURLBase, const char* location, int expires ):
    CUPnPDevice(rootDevice, FixedURLBase), m_URLBase(OriginalURLBase),
    m_location(location), m_expires(expires) {}


CUPnPControlPoint* CUPnPControlPoint::instance = nullptr;

CUPnPControlPoint::CUPnPControlPoint(unsigned short udpPort):
    m_WanService(nullptr), m_UPnPClientHandle(),
    mMutex("UPnPControlPoint"), m_IGWDeviceDetected(false),
    mWaitForSearchMutex("UPnPControlPoint-WaitForSearchTimeout")
{
	RS_DBG2("udpPort: ", udpPort);

	// Pointer to self
	instance = this;
	
	// Start UPnP
	int ret = UpnpInit(nullptr, udpPort);
	if (ret != UPNP_E_SUCCESS && ret != UPNP_E_INIT)
	{
		RsErr() << __PRETTY_FUNCTION__ << " UpnpInit returned : " << ret
		        << std::endl;
		goto error;
	}

	RS_DBG1("bound to ", UpnpGetServerIpAddress(), UpnpGetServerPort());

	ret = UpnpRegisterClient(
	            reinterpret_cast<Upnp_FunPtr>(&CUPnPControlPoint::Callback),
	            &m_UPnPClientHandle, &m_UPnPClientHandle );
	if (ret != UPNP_E_SUCCESS)
	{
		RsErr() << __PRETTY_FUNCTION__ << " UpnpRegisterClient returned: "
		        << ret << std::endl;
		goto error;
	}

	// We could ask for just the right device here. If the root device
	// contains the device we want, it will respond with the full XML doc,
	// including the root device and every sub-device it has.
	// 
	// But lets find out what we have in our network by calling UPNP_ROOT_DEVICE.
	//
	// We should not search twice, because this will produce two
	// UPNP_DISCOVERY_SEARCH_TIMEOUT events, and we might end with problems
	// on the mutex.
//	UpnpSetContentLength(m_UPnPClientHandle, 5000000);
//	UpnpSetMaxContentLength(5000000);
	ret = UpnpSearchAsync(
	            m_UPnPClientHandle, 20, m_upnpLib.UPNP_DEVICE_IGW_VAL, nullptr);
	//ret = UpnpSearchAsync(m_UPnPClientHandle, 3, m_upnpLib.UPNP_DEVICE_IGW.c_str(), this);
	//ret = UpnpSearchAsync(m_UPnPClientHandle, 3, m_upnpLib.UPNP_DEVICE_LAN.c_str(), this);
	//ret = UpnpSearchAsync(m_UPnPClientHandle, 3, m_upnpLib.UPNP_DEVICE_WAN_CONNECTION.c_str(), this);
	if (ret != UPNP_E_SUCCESS) {
#ifdef UPNP_DEBUG
		std::cerr << "UPnPControlPoint::CUPnPControlPoint() error(UpnpSearchAsync): Error sending search request: ";
#endif
		goto error;
	}

	/* Wait for the UPnP initialization to complete.
	 * Lock TWICE on purpose so that we block until an internet gateway is found
	 * or search for them times out @see ::Callback(...)
	 * at UPNP_DISCOVERY_SEARCH_RESULT and UPNP_DISCOVERY_SEARCH_TIMEOUT */
	mWaitForSearchMutex.lock();
	RS_DBG2("Double lock m_WaitForSearchTimeoutMutex");
	mWaitForSearchMutex.lock();
	RS_DBG2("m_WaitForSearchTimeoutMutex blocking finished");

#ifndef RS_UPNP_FRENCH_NEUFBOX_WORKAROUND
#	define RS_UPNP_FRENCH_NEUFBOX_WORKAROUND 0
#endif
#if RS_UPNP_FRENCH_NEUFBOX_WORKAROUND
	/* Clean the PortMappingNumberOfEntries as it is erroneus on the first event
	 * with the french neufbox. */
	/* G10h4ck: Last model of this router seems to have been produced long ago
	 * (2009) https://openwrt.org/toh/sfr/nb4 still Phenom suggests other models
	 * might be affected too so enable it by default ATM.*/
	if(WanServiceDetected())
	{
		constexpr auto&& pkk = PORT_MAPPING_NUMBER_OF_ENTIES_KEY;
		if(m_WanService->hasPropertyK(pkk))
		{
			RS_DBG0( "Appling Neufbox workaround erasing ",
			         pkk, m_WanService->getPropertyV(pkk) );
			m_WanService->delPropertyK(pkk);
		}
	}
#endif

	RS_DBG1("Costructor finished successfully");
	return;

error:
	RS_DBG0("Costructor finishing with error: ", ret, " ",
	        m_upnpLib.GetUPnPErrorMessage(ret) );
	UpnpFinish();
}


rs_view_ptr<const char> CUPnPControlPoint::getInternalIpAddress()
{ return UpnpGetServerIpAddress(); } // libupnp own the pointed memory

CUPnPControlPoint::~CUPnPControlPoint()
{
	for(auto&& it : m_RootDeviceMap ) delete it.second;
	UpnpUnRegisterClient(m_UPnPClientHandle);
	UpnpFinish();
}


bool CUPnPControlPoint::RequestPortsForwarding(
            std::vector<CUPnPPortMapping>& upnpPortMapping )
{
	if(!WanServiceDetected())
	{
		RsErr() << __PRETTY_FUNCTION__ << " WAN Service not detected!"
		        << std::endl;
		return false;
	}

	// Add the enabled port mappings
	for(auto& pm: upnpPortMapping)
	{
		if(pm.m_enabled == "1")
		{
			// Add the mapping to the control point active mappings list
			m_ActivePortMappingsMap[pm.getKey()] = pm;
			
			// Add the port mapping
			PrivateAddPortMapping(pm);
		}
	}

	return true;
}

bool CUPnPControlPoint::getExternalAddress(sockaddr_storage& addr)
{
	bool twice = false;
retry:
	{
		RS_STACK_MUTEX(mMutex);
		for(auto& it: m_ServiceMap)
		{
			const std::string&& addrStr =
			        GetStateVariable(
			            *it.second, RsLibUpnpXml::EXTERNAL_ADDRESS_STATE_VAR_NAME );
			if(sockaddr_storage_inet_pton(addr, addrStr) &&
			        sockaddr_storage_isExternalNet(addr) )
				return true;
		}
	}

	// Do not attempt more then twice
	if(twice) return false;

	/* If we don't have the information already try requesting it via UPnP and
	 * wait just a bit */

	{
		RS_STACK_MUTEX(mMutex);
		for(auto& it: m_ServiceMap)
			Execute( *it.second,
			         RsLibUpnpXml::GET_EXTERNAL_IP_ADDRESS_ACTION,
			         std::vector<CUPnPArgumentValue>(0) );
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	goto retry;
}

void CUPnPControlPoint::RefreshPortMappings()
{
	for (	PortMappingMap::iterator it = m_ActivePortMappingsMap.begin();
		it != m_ActivePortMappingsMap.end();
		++it) {
		PrivateAddPortMapping(it->second);
	}
}


bool CUPnPControlPoint::PrivateAddPortMapping(
            const CUPnPPortMapping& upnpPortMapping )
{
	auto ipaddrPtr = getInternalIpAddress();
	if(!ipaddrPtr)
	{
		RsErr() << __PRETTY_FUNCTION__ << " UpnpGetServerIpAddress return null"
		        << std::endl;
	}
	
	// Start building the action
	std::vector<CUPnPArgumentValue> argval(8);
	
	// Action parameters
	argval[0].SetArgument("NewRemoteHost");
	argval[0].SetValue("");
	argval[1].SetArgument("NewExternalPort");
	argval[1].SetValue(upnpPortMapping.getExPort());
	argval[2].SetArgument("NewProtocol");
	argval[2].SetValue(upnpPortMapping.getProtocol());
	argval[3].SetArgument("NewInternalPort");
	argval[3].SetValue(upnpPortMapping.getInPort());
	argval[4].SetArgument("NewInternalClient");
	argval[4].SetValue(ipaddrPtr);
	argval[5].SetArgument("NewEnabled");
	argval[5].SetValue("1");
	argval[6].SetArgument("NewPortMappingDescription");
	argval[6].SetValue(upnpPortMapping.getDescription());
	argval[7].SetArgument("NewLeaseDuration");
	argval[7].SetValue("0");
	
	bool ret = true;
	{
		// Execute
		RS_STACK_MUTEX(mMutex);
		for( auto& mSit : m_ServiceMap )
			ret &= Execute(*mSit.second, ADD_PORT_MAPPING_ACTION, argval);
	}
	return ret;
}


bool CUPnPControlPoint::DeletePortMappings(
	std::vector<CUPnPPortMapping> &upnpPortMapping)
{
#ifdef UPNP_DEBUG
	std::cerr << "CUPnPControlPoint::DeletePortMappings() called." << std::endl;
#endif
	if (!WanServiceDetected()) {
#ifdef UPNP_DEBUG
		std::cerr  <<  "UPnP Error: "
			"CUPnPControlPoint::DeletePortMapping: "
			"WAN Service not detected." << std::endl;
#endif
		return false;
	}
	
	int n = upnpPortMapping.size();
	
	// Delete the enabled port mappings
	for (int i = 0; i < n; ++i) {
		if (upnpPortMapping[i].m_enabled == "1") {
			// Delete the mapping from the control point 
			// active mappings list
			PortMappingMap::iterator it =
				m_ActivePortMappingsMap.find(
					upnpPortMapping[i].getKey());
			if (it != m_ActivePortMappingsMap.end()) {
				m_ActivePortMappingsMap.erase(it);
			} else {
#ifdef UPNP_DEBUG
				std::cerr   <<  "CUPnPControlPoint::DeletePortMappings() UPnP Error: "
					"CUPnPControlPoint::DeletePortMapping: "
					"Mapping was not found in the active "
					"mapping map." << std::endl;
#endif
			}
			
			// Delete the port mapping
			PrivateDeletePortMapping(upnpPortMapping[i]);
		}
	}
		
	return true;
}


bool CUPnPControlPoint::PrivateDeletePortMapping(
	CUPnPPortMapping &upnpPortMapping)
{
	// Start building the action
	std::string actionName("DeletePortMapping");
	std::vector<CUPnPArgumentValue> argval(3);
	
	// Action parameters
	argval[0].SetArgument("NewRemoteHost");
	argval[0].SetValue("");
	argval[1].SetArgument("NewExternalPort");
	argval[1].SetValue(upnpPortMapping.getExPort());
	argval[2].SetArgument("NewProtocol");
	argval[2].SetValue(upnpPortMapping.getProtocol());
	
	// Execute
	bool ret = true;
	{
		RS_STACK_MUTEX(mMutex);
		for( auto&& mSit : m_ServiceMap)
		{
			bool success = mSit.second->Execute(actionName, argval);
			success &= ret;
			RS_DBG1("Deleting port mapping action finished with: ", success);
		}
	}

	return ret;
}

// This function is static
int CUPnPControlPoint::Callback(
            Upnp_EventType eventType, const void* event, void* /*cookie*/ )
{
	RS_DBG1("eventType: ", eventType,  " event: ", event);

	/* if(!Event) return 0; Event might be null legitimately depending on
	 * EventType */

	if (!CUPnPControlPoint::instance)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Callback called before control "
		        << "point initialization, must never happen!" << std::endl;
		print_stacktrace();
		return 0;
	}

	std::string msg;
	std::string msg2;

	switch (eventType)
	{
	case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE: /*fallthrough*/
	case UPNP_DISCOVERY_SEARCH_RESULT:
	{
		const UpnpDiscovery* d_event = static_cast<const UpnpDiscovery*>(event);

		// This way we are sure it is deleted even in case of errors
		using doc_ptr = std::unique_ptr<IXML_Document, decltype(ixmlDocument_free)>;
		doc_ptr doc(nullptr, ixmlDocument_free);

		{
			rs_owner_ptr<IXML_Document> tmpDoc = nullptr;

			// Get the XML tree device description in doc
			int ret = UpnpDownloadXmlDoc(
			            UpnpDiscovery_get_Location_cstr(d_event), &tmpDoc);

			if (tmpDoc) doc.reset(tmpDoc);
			else
			{
				RsErr() << __PRETTY_FUNCTION__ << " UpnpDownloadXmlDoc ret: "
				        << ret << std::endl;
				break;
			}
		}

		rs_view_ptr<IXML_Element> root =
		        RsLibUpnpXml::GetRootElement(*doc);
		if(!root)
		{
			RsErr() << __PRETTY_FUNCTION__ << " failure getting document root"
			        << std::endl;
			break;
		}

		// Extract the URLBase
		auto urlPtr = RsLibUpnpXml::GetChildValueByTag(*root, URL_BASE_TAG);
		if(!urlPtr)
		{
			RsErr() << __PRETTY_FUNCTION__ << " failure getting UPnP url"
			        << std::endl;
			break;
		}
		const std::string urlBase(urlPtr);

		// Get the root device
		rs_view_ptr<IXML_Element> rootDevice =
		        RsLibUpnpXml::GetFirstChildByTag(*root, DEVICE_TAG);
		if(!rootDevice)
		{
			RsErr() << __PRETTY_FUNCTION__ << " failure getting root device"
			        << std::endl;
			break;
		}

		auto devTypePtr = RsLibUpnpXml::GetChildValueByTag(
		            *rootDevice, CUPnPDevice::DEVICE_TYPE_TAG );
		if(!devTypePtr)
		{
			RsErr() << __PRETTY_FUNCTION__ << " failure getting device type"
			        << std::endl;
			break;
		}
		const std::string devType(devTypePtr);

		// Only add device if it is an InternetGatewayDevice
		if(stdStringIsEqualCI(devType, RsLibUpnpXml::UPNP_DEVICE_IGW_VAL))
		{
			/* Only internet gateway gets here */
			CUPnPControlPoint::instance->AddRootDevice(
			            rootDevice, urlBase,
			            UpnpDiscovery_get_Location_cstr(d_event),
			            UpnpDiscovery_get_Expires(d_event) );

			/* We discovered at least one Internet Gateway so we can continue
			 * UPnP setup */
			CUPnPControlPoint::instance->mWaitForSearchMutex.unlock();
		}
		break;
	}
	case UPNP_DISCOVERY_SEARCH_TIMEOUT:
		/* Didn't find any UPnP device, we can continue UPnP setup */
		CUPnPControlPoint::instance->mWaitForSearchMutex.unlock();
		break;
	case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE: // UPnP Device Removed
	{
		const UpnpDiscovery* dab_event = static_cast<const UpnpDiscovery*>(event);
		if(!dab_event) break;

		std::string devType = UpnpDiscovery_get_DeviceType_cstr(dab_event);
		std::transform(devType.begin(), devType.end(), devType.begin(), tolower);

		// Check for an InternetGatewayDevice and removes it from the list
		if(stdStringIsEqualCI(devType, RsLibUpnpXml::UPNP_DEVICE_IGW_VAL))
			CUPnPControlPoint::instance->RemoveRootDevice(
			            UpnpDiscovery_get_DeviceID_cstr(dab_event) );
		break;
	}
	case UPNP_EVENT_RECEIVED:
	{
		// Event reveived
		const UpnpEvent* e_event = static_cast<const UpnpEvent*>(event);

		const std::string Sid = UpnpEvent_get_SID_cstr(e_event);

		// Parses the event
		CUPnPControlPoint::instance->OnEventReceived(
		            Sid, UpnpEvent_get_EventKey(e_event),
		            UpnpEvent_get_ChangedVariables(e_event) );
		break;
	}
	case UPNP_EVENT_SUBSCRIBE_COMPLETE:
		msg += "error(UPNP_EVENT_SUBSCRIBE_COMPLETE): ";
		goto upnpEventRenewalComplete;
	case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
		msg += "error(UPNP_EVENT_UNSUBSCRIBE_COMPLETE): ";
		goto upnpEventRenewalComplete;
	case UPNP_EVENT_RENEWAL_COMPLETE:
	{
		msg += "error(UPNP_EVENT_RENEWAL_COMPLETE): ";
upnpEventRenewalComplete:
		const UpnpEventSubscribe* es_event =
		        static_cast<const UpnpEventSubscribe*>(event);

		if (UpnpEventSubscribe_get_ErrCode(es_event) != UPNP_E_SUCCESS)
		{
			msg += "Error in Event Subscribe Callback";
			RsLibUpnpXml::processUPnPErrorMessage(
			            msg, UpnpEventSubscribe_get_ErrCode(es_event),
			            nullptr, nullptr );
		}
		
		break;
	}
	case UPNP_EVENT_AUTORENEWAL_FAILED:
		msg += "CUPnPControlPoint::Callback() error(UPNP_EVENT_AUTORENEWAL_FAILED): ";
		msg2 += "UPNP_EVENT_AUTORENEWAL_FAILED: ";
		goto upnpEventSubscriptionExpired;
	case UPNP_EVENT_SUBSCRIPTION_EXPIRED: {
		msg += "CUPnPControlPoint::Callback() error(UPNP_EVENT_SUBSCRIPTION_EXPIRED): ";
		msg2 += "UPNP_EVENT_SUBSCRIPTION_EXPIRED: ";
upnpEventSubscriptionExpired:
		const UpnpEventSubscribe* es_event =
		        static_cast<const UpnpEventSubscribe*>(event);

#ifdef PATCHED_LIBUPNP
		auto&& pubUrlStr = UpnpString_get_String(es_event->PublisherUrl);
#else
		auto&& pubUrlStr = UpnpEventSubscribe_get_PublisherUrl_cstr(es_event);
#endif

		Upnp_SID newSID;
		int TimeOut = 1801;
		int ret = UpnpSubscribe(
		            CUPnPControlPoint::instance->m_UPnPClientHandle,
		            pubUrlStr, &TimeOut, newSID );
		if (ret != UPNP_E_SUCCESS)
		{
			msg += "Error Subscribing to EventURL";
			RsLibUpnpXml::processUPnPErrorMessage(
			            msg, UpnpEventSubscribe_get_ErrCode(es_event),
			            nullptr, nullptr );
		}
		else
		{
			CUPnPControlPoint::instance->mMutex.lock();

			ServiceMap::iterator it = CUPnPControlPoint::instance->
			        m_ServiceMap.find(pubUrlStr);
			if(it != CUPnPControlPoint::instance->m_ServiceMap.end())
			{
				CUPnPService& service = *(it->second);
				service.SetTimeout(TimeOut);
				service.SetSID(newSID);
				RS_DBG1( "Re-subscribed to EventURL ", pubUrlStr, " with SID ",
				         newSID );

				/* TODO: we need to refresh port mappings just for this device
				 * not for all*/
				CUPnPControlPoint::instance->RefreshPortMappings();
			}
			else
			{
				RsErr() << __PRETTY_FUNCTION__ << " Could not find service "
				        << newSID << " in the service map." << std::endl;
				print_stacktrace();
			}

			CUPnPControlPoint::instance->mMutex.unlock();
		}
		break;
	}
	case UPNP_CONTROL_ACTION_COMPLETE:
	{
		// This is here if we choose to do this asynchronously
		const UpnpActionComplete* a_event =
		        static_cast<const UpnpActionComplete*>(event);
		if(UpnpActionComplete_get_ErrCode(a_event) != UPNP_E_SUCCESS)
		{
			RsLibUpnpXml::processUPnPErrorMessage(
			            "UpnpSendActionAsync",
			            UpnpActionComplete_get_ErrCode(a_event), nullptr,
			            UpnpActionComplete_get_ActionResult(a_event) );
			break;
		}

		rs_view_ptr<IXML_Document> respDoc =
		        UpnpActionComplete_get_ActionResult(a_event);
		if(!respDoc)
		{
			RsErr() << __PRETTY_FUNCTION__ << " null RespDoc" << std::endl;
			break;
		}

		RS_DBG0( "UPNP_CONTROL_ACTION_COMPLETE ixmlPrintDocument(respDoc) ",
		         ixmlPrintDocument(respDoc) );

		rs_view_ptr<IXML_Element> root = RsLibUpnpXml::GetRootElement(
		            *const_cast<IXML_Document*>(respDoc) );
		if(!root)
		{
			RsErr() << __PRETTY_FUNCTION__ << " null root" << std::endl;
			break;
		}

		rs_view_ptr<IXML_Element> child = RsLibUpnpXml::GetFirstChild(*root);
		while(child)
		{
			const DOMString childTag = RsLibUpnpXml::GetTag(*child);
			const DOMString childValue = RsLibUpnpXml::GetTextValue(*child);

			//add the variable to the wanservice property map
			CUPnPControlPoint::instance->m_WanService
			        ->setPropertyKV(childTag, childValue);
			child = RsLibUpnpXml::GetNextSibling(*child);
		}

		/* No need for any processing here.
		 * Service state table updates are handled by events. */
		break;
	}
	case UPNP_CONTROL_GET_VAR_COMPLETE:
	{
		msg += "CUPnPControlPoint::Callback() error(UPNP_CONTROL_GET_VAR_COMPLETE): ";
		const UpnpStateVarComplete* sv_event =
		        static_cast<const UpnpStateVarComplete*>(event);
		if (UpnpStateVarComplete_get_ErrCode(sv_event) != UPNP_E_SUCCESS)
		{
			msg += "m_UpnpGetServiceVarStatusAsync";
			RsLibUpnpXml::processUPnPErrorMessage(
			            msg, UpnpStateVarComplete_get_ErrCode(sv_event),
			            nullptr, nullptr );
		}
		else
		{
			//add the variable to the wanservice property map
			CUPnPControlPoint::instance->m_WanService->setPropertyKV(
			            UpnpStateVarComplete_get_StateVarName_cstr(sv_event),
			            UpnpStateVarComplete_get_CurrentVal_cstr(sv_event) );
		}
		break;
	}
	// ignore these cases, since this is not a device 
	case UPNP_CONTROL_GET_VAR_REQUEST: // [[fallthrough]]
	case UPNP_CONTROL_ACTION_REQUEST: // [[fallthrough]]
	case UPNP_EVENT_SUBSCRIPTION_REQUEST:
		break;
	default:
		RsErr() << __PRETTY_FUNCTION__ << " unknown event: " << event
		        << " eventType: " << eventType << std::endl;
		print_stacktrace();
		break;
	}
	
	return 0;
}


void CUPnPControlPoint::OnEventReceived(
        const std::string& Sid,
		int EventKey,
        const IXML_Document* changedVariablesDocPtr )
{
	RS_DBG2( "Sid: ", Sid, " EventKey: ", EventKey);

	/* If the received data is of a wan device store the properties */

	if(!changedVariablesDocPtr)
	{
		RsErr() << __PRETTY_FUNCTION__ << " got null xml document" << std::endl;
		return;
	}
	IXML_Document& changedVariablesDoc(
	            *const_cast<IXML_Document*>(changedVariablesDocPtr) );

	RS_STACK_MUTEX(mMutex);

	rs_view_ptr<CUPnPService> tWanService = nullptr;
	for(auto&& it: m_ServiceMap)
	{
		if(it.second->GetSID() == Sid)
		{
			tWanService = it.second;
			break;
		}
	}
	if(!tWanService) return;


	rs_view_ptr<IXML_Element> root =
	        RsLibUpnpXml::GetRootElement(changedVariablesDoc);
	if(!root)
	{
		RsErr() << __PRETTY_FUNCTION__ << " invalid xml document " << std::endl;
		return;
	}

	rs_view_ptr<IXML_Element> child = m_upnpLib.GetFirstChild(*root);
	if(!child) RS_DBG2("Empty property list");

	while(child)
	{
		rs_view_ptr<IXML_Element> prop = RsLibUpnpXml::GetFirstChild(*child);
		if(!prop) continue;

		const DOMString childTag = RsLibUpnpXml::GetTag(*prop);
		const DOMString childVal = RsLibUpnpXml::GetTextValue(*prop);
		RS_DBG3(childTag, childVal);
		tWanService->mPropertiesMap.insert(childTag, childVal);
		child = RsLibUpnpXml::GetNextSibling(*child);
	}

	// Freeing that doc segfaults. Probably should not be freed.
	//ixmlDocument_free(ChangedVariablesDoc);
}


void CUPnPControlPoint::AddRootDevice(
    IXML_Element& rootDevice, const std::string &urlBase,
	const char *location, int expires)
{
	RS_STACK_MUTEX(mMutex);
	
	// Root node's URLBase
	std::string OriginalURLBase(urlBase);
	std::string FixedURLBase(OriginalURLBase.empty() ?
		location :
		OriginalURLBase);

	// Get the UDN (Unique Device Name)
	std::string UDN(
	    RsLibUpnpXml::GetChildValueByTag(rootDevice, "UDN"));
	RootDeviceMap::iterator it = m_RootDeviceMap.find(UDN);
	bool alreadyAdded = it != m_RootDeviceMap.end();
	if (alreadyAdded) {
		// Just set the expires field
		it->second->SetExpires(expires);
	} else {
		// Add a new root device to the root device list
		CUPnPRootDevice *upnpRootDevice = new CUPnPRootDevice(
			*this, m_upnpLib, rootDevice,
			OriginalURLBase, FixedURLBase,
			location, expires);
		m_RootDeviceMap[upnpRootDevice->GetUDN()] = upnpRootDevice;
	}
}


void CUPnPControlPoint::RemoveRootDevice(const char *udn)
{
	RS_STACK_MUTEX(mMutex);

	// Remove
	std::string UDN(udn);
	RootDeviceMap::iterator it = m_RootDeviceMap.find(UDN);
	if (it != m_RootDeviceMap.end()) {
		delete it->second;
		m_RootDeviceMap.erase(UDN);
	}
}


void CUPnPControlPoint::Subscribe(CUPnPService& service)
{
	auto scpdDoc = RsLibUpnpXml::DownloadXmlDoc(service.GetAbsSCPDURL().c_str());
	if(!scpdDoc)
	{
		RsErr() << __PRETTY_FUNCTION__ << " DownloadXmlDoc failed" << std::endl;
		return;
	}

	rs_view_ptr<IXML_Element> scpdRoot = RsLibUpnpXml::GetRootElement(*scpdDoc);
	if(!scpdRoot)
	{
		RsErr() << __PRETTY_FUNCTION__ << " GetRootElement failed" << std::endl;
		return;
	}

	CUPnPSCPD *scpd = new CUPnPSCPD(*this, *scpdRoot, service.GetAbsSCPDURL());
	service.SetSCPD(scpd);

	{
		RS_STACK_MUTEX(mMutex);
		m_ServiceMap[service.GetAbsEventSubURL()] = &service;
	}

	RS_DBG1( "Successfully retrieved SCPD Document for service ",
	         service.GetServiceType(), " absEventSubURL: ",
	         service.GetAbsEventSubURL() );

	/* Now try to subscribe to this service. If the subscription
	 * is not successfull, we will not be notified with events, but it may be
	 * possible to use the service anyway. */

	int errcode = UpnpSubscribe(
	            m_UPnPClientHandle, service.GetAbsEventSubURL().c_str(),
	            service.GetTimeoutAddr(), service.GetSID() );
	if (errcode != UPNP_E_SUCCESS)
	{
		RsErr() << __PRETTY_FUNCTION__ << " UpnpSubscribe failued for: "
		        << service.GetServiceType() << " absEventSubURL: "
		        << service.GetAbsEventSubURL() << " err: " << errcode
		        << std::endl;
		return;
	}
}


void CUPnPControlPoint::Unsubscribe(const CUPnPService& service)
{
	RS_STACK_MUTEX(mMutex);
	auto&& sit = m_ServiceMap.find(service.GetAbsEventSubURL());
	if(sit != m_ServiceMap.end())
	{
		m_ServiceMap.erase(sit);
		UpnpUnSubscribe(m_UPnPClientHandle, service.GetSID());
	}
}


// File_checked_for_headers
