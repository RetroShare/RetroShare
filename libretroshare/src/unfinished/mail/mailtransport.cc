/*
 * libretroshare/src/services/mail mailtransport.cc
 *
 * Services for RetroShare.
 *
 * Copyright 2014 by Robert Fernie.
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

#include "services/mail/mailtransport.h"

using namespace Rs::Mail;

MailTransport::MailTransport(uint32_t max_size, uint16_t transport_type)
:mEnabled(true), mMaxSize(max_size), mTransportType(transport_type) 
{
	return;
}

bool MailTransport::isEnabled()
{
	return mEnabled;
}

void MailTransport::setEnabled(bool enabled)
{
	mEnabled = enabled;
}

uint32_t MailTransport::getMaxSize()
{
	return mMaxSize;
}

uint16_t MailTransport::getTransportType()
{
	return mTransportType;
}


