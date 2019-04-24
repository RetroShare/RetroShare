/*******************************************************************************
 * libupnp-1.8.x -> libupnp-1.6.x  retrocompatibility header                   *
 *                                                                             *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#pragma once

#include "upnp/upnp.h"


#if UPNP_VERSION < 10800

using UpnpDiscovery        = Upnp_Discovery;
using UpnpEvent            = Upnp_Event;
using UpnpEventSubscribe   = Upnp_Event_Subscribe;
using UpnpActionComplete   = Upnp_Action_Complete;
using UpnpStateVarComplete = Upnp_State_Var_Complete;

#endif // UPNP_VERSION < 10800

#if UPNP_VERSION < 10624

static inline int UpnpDiscovery_get_Expires(const Upnp_Discovery* disc) noexcept
{ return disc->Expires; }

static inline const char* UpnpDiscovery_get_DeviceID_cstr(
        const Upnp_Discovery* disc ) noexcept
{ return disc->DeviceId; }

static inline const char* UpnpDiscovery_get_DeviceType_cstr(
        const Upnp_Discovery* disc ) noexcept
{ return disc->DeviceType; }

static inline const char* UpnpDiscovery_get_Location_cstr(
        const Upnp_Discovery* disc ) noexcept
{ return disc->Location; }

static inline const char* UpnpEvent_get_SID_cstr(const UpnpEvent* ev) noexcept
{ return ev->Sid; }

static inline int UpnpEvent_get_EventKey(const UpnpEvent* ev) noexcept
{ return ev->EventKey; }

static inline const IXML_Document* UpnpEvent_get_ChangedVariables(
        const UpnpEvent* ev) noexcept { return ev->ChangedVariables; }

static inline int UpnpEventSubscribe_get_ErrCode(const UpnpEventSubscribe* evs)
noexcept { return evs->ErrCode; }

static inline const char* UpnpEventSubscribe_get_PublisherUrl_cstr(
        const UpnpEventSubscribe* evs ) noexcept { return evs->PublisherUrl; }

static inline int UpnpActionComplete_get_ErrCode(const UpnpActionComplete* evc)
noexcept { return evc->ErrCode; }

static inline const IXML_Document* UpnpActionComplete_get_ActionResult(
        const UpnpActionComplete* evc ) noexcept { return evc->ActionResult; }

static inline int UpnpStateVarComplete_get_ErrCode(
        const UpnpStateVarComplete* esvc) noexcept { return esvc->ErrCode; }

static inline const char* UpnpStateVarComplete_get_StateVarName_cstr(
        const UpnpStateVarComplete* esvc) noexcept { return esvc->StateVarName; }

static inline const char* UpnpStateVarComplete_get_CurrentVal_cstr(
        const UpnpStateVarComplete* esvc) noexcept { return esvc->CurrentVal; }

#endif // UPNP_VERSION < 10624
