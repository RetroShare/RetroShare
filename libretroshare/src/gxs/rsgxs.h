#ifndef RSGXS_H
#define RSGXS_H

/*
 * libretroshare/src/gxs   : rsgxs.h
 *
 * GXS  interface for RetroShare.
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

#include "rsgdp.h"

/*!
 * Retroshare general exchange service
 * This forms the basic interface that classes need to inherit
 * in order to use the general exchange service
 * General GNP drives the service.
 * GDP deals with exporting and importing
 * data from the derived class
 * GXIP is used to maintain
 */
class RsGxs
{
public:
    RsGxs();

public:

    virtual void receiveMessage(RsGxsMessage*) = 0;
    void sendMessage(RsGxsMessage*) = 0;

    /*!
     * drives synchronisation between peers
     */
    void tick();
};

#endif // RSGXS_H
