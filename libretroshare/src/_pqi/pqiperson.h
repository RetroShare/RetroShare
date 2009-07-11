/*
 * libretroshare/src/pqi pqiperson.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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
 */



#ifndef PQIPERSON_H
#define PQIPERSON_H


#include "_pqi/pqipersongrp.h"
#include "_pqi/pqi.h"
#include "_util/rsdebug.h"

#include <sstream>


#include <list>

class pqiperson;

static const int CONNECT_RECEIVED     = 1;
static const int CONNECT_SUCCESS      = 2;
static const int CONNECT_UNREACHABLE  = 3;
static const int CONNECT_FIREWALLED   = 4;
static const int CONNECT_FAILED       = 5;

#include "pqi/pqistreamer.h"

class pqiconnect: public pqistreamer, public NetInterface
{
public:
    pqiconnect(RsSerialiser *rss, NetBinInterface *ni_in);
    virtual ~pqiconnect() {
        return;
    }

    // presents a virtual NetInterface -> passes to ni.
    virtual int	connect(struct sockaddr_in raddr);
    virtual int	listen();
    virtual int	stoplistening();
    virtual int reset();
    virtual int disconnect();
    virtual bool connect_parameter(uint32_t type, uint32_t value);

    // get the contact from the net side!
    virtual std::string PeerId();

    // to check if our interface.
    virtual bool thisNetInterface(NetInterface *ni_in);
    //protected:
    NetBinInterface *ni;
protected:
};


class pqipersongrp;

class pqiperson: public PQInterface
{
public:
    pqiperson(std::string id, pqipersongrp *ppg);
    virtual ~pqiperson(); // must clean up children.

    // control of the connection.
    int 	reset();
    int 	listen();
    int 	stoplistening();
    int	connect(uint32_t type, struct sockaddr_in raddr, uint32_t delay, uint32_t period, uint32_t timeout);

    // add in connection method.
    int	addChildInterface(uint32_t type, pqiconnect *pqi);

    // The PQInterface interface.
    virtual int     SendItem(RsItem *);
    virtual RsItem *GetItem();

    virtual int 	status();
    virtual int	tick();

    // overloaded callback function for the child - notify of a change.
    int 	notifyEvent(NetInterface *ni, int event);

    // PQInterface for rate control overloaded....
    virtual float   getRate(bool in);
    virtual void    setMaxRate(bool in, float val);

private:

    std::map<uint32_t, pqiconnect *> kids;
    bool active;
    pqiconnect *activepqi;
    bool inConnectAttempt;
    int waittimes;

private: /* Helper functions */
    pqipersongrp *pqipg; /* parent for callback */
};



#endif // PQIPERSON_H

