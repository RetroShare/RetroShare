/*
 * libretroshare/src/services: p3circles.h
 *
 * Identity interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#ifndef P3_CIRCLES_SERVICE_HEADER
#define P3_CIRCLES_SERVICE_HEADER


#include "retroshare/rscircles.h"	// External Interfaces.
#include "gxs/rsgenexchange.h"		// GXS service.
#include "gxs/rsgixs.h"			// Internal Interfaces.

#include <map>
#include <string>

/* 
 * Circles Identity Service
 *
 * A collection of notes:
 *
 * We want to be able to express the following types of Circles.
 *
 *   - Public 
 *       - Groups & Messages can be passed onto anyone. ( No Restrictions. )
 *       - GXS Notes:
 * 		- This is what we have currently.
 *
 *   - External Circle
 *       - List of Identities that can receive the Group / Messages.
 * 	 - This list will be defined via a set of RsIdentities - which have PGPHashes set.
 * 	 	- We need the PGPHashes to be able to identify which peers can receive msgs.
 *  		- Messages are passed to the Intersection of (Identified PGPHashes & Friends)
 *       - Distribution of Circle Definitions can be also be restricted via circles.
 * 		- You can have Public External Groups, or Groups that only the Members know about.
 *	 - Control of these External Groups is determined by Admin / Publish Keys.
 *       - The Danger with External Groups, is your ID wll be associated with other people...
 * 		- Leaking information!!!
 *       - GXS Notes:
 * 		- p3Circles will provide a distrib list for a given Circle Group.
 *
 *   - Personal Circle or "Your Eyes Only".
 *       - Same as an Internal Circle Definition. (What will be used for File Sharing initially)
 *       - Each peer will have a bunch of these, Friends, Family, etc.
 *
 *       - The list is not publically shared, only the originator of the message will distribute.
 *       - You can communicate back to the originator, who will share with the other members.
 *         but you mustn't discuss / share content with anyone else.
 *       - This is quite a Weak / Fragile Group, as there is only one distributor.
 *       - GXS NOTES:
 *           - TO make this work, we need GXS or RsCircles to maintain extra info:
 *           - GXS stores the original source, so communications can go back there.
 * 	     - If Originator, GXS store a REFERENCE, Circles turn this into a distrib list of peers.
 *
 *
 *
 * Like RsIdentities are used to validation messages, 
 * RsCircles will be used to determine if a peer can receive a group / messages.
 *
 *        bool RsCircles::canSend(RsCircleId, RsPeerId)
 *        bool RsCircles::canSend(RsCircleInternalId, RsPeerId)
 *
 * or maybe just:
 *
 *        bool RsCircles::recipients(GxsPermission &perms, std::list<RsPeerId> friendlist);
 *
 */


/* Permissions is part of GroupMetaData 
 */

#define GXS_PERM_TYPE_PUBLIC		0x0001
#define GXS_PERM_TYPE_EXTERNAL		0x0002
#define GXS_PERM_TYPE_YOUREYESONLY	0x0003

class GxsPermissions
{
public:
	uint32_t mType; // PUBLIC, EXTERNAL or YOUREYESONLY, Mutually exclusive.	
	RsCircleId mCircleId; // If EXTERNAL, otherwise Blank.

	// BELOW IS NOT SERIALISED - BUT MUST BE STORED LOCALLY BY GXS. (If YOUREYESONLY)
	RsPeerId mOriginator;	
	RsCircleInternalId mInternalCircle; // if Originator == ownId, otherwise blank.
};


class RsGxsCircleGroup
{
	public:
	GroupMetaData mMeta; // includes GxsPermissions, for control of group distribution.

	std::list<RsGxsId> mMembers;
	std::list<RsCircleId> mCircleMembers;


	// Not Serialised.
	// Internally inside rsCircles, this will be turned into:
	// std::list<RsPeerId> mAllowedFriends;
};





class rsCircle
{
public:

	/* GXS Interface - for working out who can receive */

       bool canSend(RsCircleId, RsPeerId)
       bool canSend(RsCircleInternalId, RsPeerId)
       bool recipients(GxsPermission &perms, std::list<RsPeerId> friendlist);

	/* Functions to handle Local / Internal Circles == Same as for file permissions. */
	createLocalCircle()
	addToLocalCircle()
	removeFromLocalCircle()
	getLocalCirclePeers()
	getListOfLocalCircles()

	/* similar functions for External Groups */
	virtual bool createGroup(uint32_t& token, RsGxsCircleGroup &group);
	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsCircleGroup> &groups);



}






/*** IGNORE BELOW HERE *****/

class p3Circles: public RsGxsCircleExchange, public RsCircles
{
	public:
	p3Circles(RsGeneralDataService* gds, RsNetworkExchangeService* nes);

	virtual void service_tick(); // needed for background processing.


	/* General Interface is provided by RsIdentity / RsGxsIfaceImpl. */

	/* Data Specific Interface */

	// These are exposed via RsIdentity.
virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsIdGroup> &groups);

	// These are local - and not exposed via RsIdentity.
virtual bool getMsgData(const uint32_t &token, std::vector<RsGxsIdOpinion> &opinions);
virtual bool createGroup(uint32_t& token, RsGxsIdGroup &group);
virtual bool createMsg(uint32_t& token, RsGxsIdOpinion &opinion);

	/**************** RsIdentity External Interface.
	 * Notes:
	 */

	/**************** RsGixs Implementation 
	 * Notes:
	 *   Interface is only suggestion at the moment, will be changed as necessary.
	 *   Results should be cached / preloaded for maximum speed.
	 *
	 */

	/**************** RsGixsReputation Implementation 
	 * Notes:
	 *   Again should be cached if possible.
	 */


	protected:

    /** Notifications **/
    virtual void notifyChanges(std::vector<RsGxsNotify*>& changes);

	private:

};

#endif // P3_IDENTITY_SERVICE_HEADER



