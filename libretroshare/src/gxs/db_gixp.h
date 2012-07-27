/*
 * libretroshare/src/gxp: gxp.h
 *
 * General Exchange Protocol interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#ifndef RS_GIXP_H
#define RS_GIXP_H


/************************************************************************
 * GIXP: General Identity Exchange Protocol.
 *
 * As we're always running into troubles with GPG signatures... we are going to 
 * create a layer of RSA Keys for the following properties:
 *
 * 1) RSA Keys can be Anonymous, Self-Signed with Pseudonym, Signed by GPG Key.
 *	- Anonymous & Pseudonym Keys will be shared network-wide (Hop by Hop).
	- GPG signed Keys will only be shared if we can validate the signature 
		(providing similar behaviour to existing GPG Keys).
	- GPG signed Keys can optionally be marked for Network-wide sharing.
 * 2) These keys can be used anywhere, specifically in the protocols described below.
 * 3) These keys can be used to sign, encrypt, verify & decrypt
 * 4) Keys will never need to be directly accessed - stored in this class.
 * 5) They will be cached locally and exchanged p2p, by pull request.
 * 6) This class will use the generalised packet storage for efficient caching & loading.
 * 7) Data will be stored encrypted.
 *
 *
 *****/


class gixp::key
{
	gxip::keyref mKeyId;

	PubKey *pubKey;
	PrivateKey *privKey; /* NULL if non-existant */
};


/* As we want GPG signed profiles, to be usable as PSEUDONYMS further afield,
 * we will split them out, and distribute them seperately.
 *
 * So a key can have multiple profiles associated with it.
 * - They should always have a self-signed one.
 * - optionally have a gpg-signed one.
 */

class gixp::profile
{
	public:

	gxip::keyref mKeyId;

	std::string mName;
	time_t      mTimestamp;   /* superseded by newer timestamps */
	uint32_t    mProfileType; /* ANONYMOUS (no name, self-signed), PSEUDONYM (self-signed), GPG (name=gpgid, gpgsigned), REVOCATION?? */ 
	gpp::permissions mPermissions;

	gxip::signature mSignature;
};


class  gxip::keyref
{
	std::string keyId;
}

class gxip::keyrefset
{
	std::list<gxip::keyref> keyIds;
}

class gxip::signature
{
	gxip::keyref signer;
	std::string signature;
}

class gxip::signset
{
	std::list<gxip::signature> signs;
};


/*
 * We will pinch an idea from Amplify & Briar... of using reputations to decide
 * if we display or distribute messages.
 *
 * The Reputation will be associated with the Profile - It has to be stored
 * independently of the messages - which will not be touched.
 *
 * This part of the code will have to be worked out.
 */

class gxip::reputation
{
	gxip::keyref;

	int16_t score; /* -1000 => 1000 ??? */
	std::string comment;

	gxip::signature sign;
};


/*******
 * NOTES:
 * 1) much of this is already implemented in libretroshare/src/distrib/distribsecurity.cc
 * 2) Data types will need to be shoehorned into gxmp::signedmsg format for transport.
 * 3) Likewise this class will sit above a gdp/gnp/gsp data handling.
 */

class p3gixp
{
	bool createKey(gixp::profile, bool doGpgAlso); /* fills in mKeyId, and signature */

	bool haveKey(keyId);
	bool havePrivateKey(keyId);
	bool requestKey(keyId);

	bool getProfile(keyId, gixp::profile &prof);		/* generic profile - that can be distributed far and wide */
	bool getGpgProfile(keyId, gixp::profile &prof);		/* personal profile - (normally) only shared with friends */

	bool getReputations(keyId, std::list<gxip::reputation> &reps);
	int16_t getRepScore(keyId, uint32_t flags);

	/*** process data ***/
	bool sign(KeyId, Data, signature); 	
	bool verify(KeyId, Data, signature);
	bool decrypt(KeyId, Data, decryptedData);
	bool encrypt(KeyId, Data, decryptedData);

};

#endif /* RS_GIXP_H */


