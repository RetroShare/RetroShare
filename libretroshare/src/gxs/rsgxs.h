#ifndef RSGXS_H
#define RSGXS_H

/*
 * libretroshare/src/gxs   : rsgxs.h
 *
 * GXS  interface for RetroShare.
 * Convenience header
 *
 * Copyright 2011 Christopher Evi-Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 * This is *THE* auth manager. It provides the web-of-trust via
 * gpgme, and authenticates the certificates that are managed
 * by the sublayer AuthSSL.
 *
 */

#include <string>
#include <inttypes.h>

#include "serialiser/rsbaseitems.h"
#include "rsgnp.h"
#include "rsgdp.h"
#include "rsgixs.h"

/*!
 *
 * Brief explanation of how RsGeneralExchange service would work \n
 *
 * Resposibilities : Giving access to its RsGeneralDataService and RsNetworktExchangeService
 * instances \n
 *
 * Features: \n
 *
 *  Internal Message Flow: \n\n
 *
 *    - Storage of user generated messages : stores are made to RsGeneralDataService \n
 *    - Outbound Requests are made via RsNetworktExchangeService from here, and delivered to the service \n
 *      A consideration is allow the RsNetworktExchangeService x-ref GDP and GNP \n
 *
 *    - Inbound request from peers are made by RsNetworktExchangeService to RsGeneralDataService \n
 *      Again x-ref between GNP and GDP (Services don't see implementation so they don't know this) \n
 *
 *    - RsNetworktExchangeService stores requested message \n
 *
 *  Permissions: \n\n
 *    - Permission are defined within the service, and mirror RsGroups feature set \n
 *    - These are resolved in RsGeneralDataService \n
 *
 *  Storage: \n\n
 *
 *   - Storage mechnaism used by service can be whatever they want with current RsGeneralStorageService
 *     interface \n
 *   - But a partial (non-pure virtual) implementation may be done to enforce use of Retrodb
 *     and efficiency \n
 *
 * Security:
 *   - This accessible at all levels but generally the service and storage. \n
 *   - Security feature can be used or not, for RsIdentityExchangeService this is not used obviously\n
 *
 *
 ************************************/
class RsGeneralExchangeService {

public:

    /*!
     * get the RsGeneralDataService instance serving this \n
     * RsGeneralExchangeService
     * @return data service
     */
    virtual RsGeneralDataService* getDataService() = 0;

    /*!
     * get the RsNetworktExchangeService instance serving this \n
     * RsGeneralExchangeService
     * @return network exchange service
     */
    virtual RsNetworktExchangeService* getNetworkService() = 0;

};


#endif // RSGXS_H
