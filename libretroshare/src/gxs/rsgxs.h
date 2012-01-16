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
#include "util/rsthreads.h"

/*!
 * Retroshare general exchange service
 * This forms the basic interface that classes need to inherit
 * in order to use the general exchange service
 * Generally GNP drives the service.
 * GDP deals with exporting and importing msgs to and from the concrete service
 * data from the derived class
 * GXIP is used to maintain
 *
 *
 * The maiin transport mechanism
 */
class RsGxs : public RsThread
{
public:
    RsGxs(const std::string& serviceName);

public:

    /*!
     * These are messages, that have been pushed to you
     * This will be called by RsGdp whenever a new msg(s) has arrived
     * the list contains ids which may be queried from the external db
     *
     * @param msgIds the ids of the new msgs received
     */
    virtual void receiveMessage(std::set<std::string>& msgIds) = 0;

    /*!
     * Push a set of messages which have been written to your service
     * database
     * @param msgIds
     */
    void push(std::set<std::string>& msgIds) = 0;


    /*!
     * allows for non blocking pushes, receives, and configuration changes.
     */
    void run();


    /*!
     * cache a message
     * @param msg message to cache
     *
     */
    void cache(RsGxsSignedMessage* msg);

    /*!
     * Queries if message with message id is cached
     * @param msgId the id of the message to query
     */
    bool cached(std::string& msgId);

    /*!
     * Use to retrieve cached msgs
     * If message id of messages not cached are requested, it is simply ignored
     * @param requestIds The ids of messages to retrieve
     * @param msgs the retrieved cached messages
     */
    void retrieveCache(std::set<std::string>& requestIds, std::set<RsGxsSignedMessage*>& msgs);



};

#endif // RSGXS_H
