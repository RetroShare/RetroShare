/*
 * libretroshare/src    gpgauthmgr.cc
 *
 * GPG  interface for RetroShare.
 *
 * Copyright 2008-2009 by Raghu Dev R
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the termsf the GNU Library General Public
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
 * o
 */
#include <iostream>
#include "gpgauthmgr.h"


GPGAuthMgr::GPGAuthMgr()
	:gpgmeInit(false) 
{

	setlocale(LC_ALL, "");
	gpgme_check_version(NULL);
	gpgme_set_locale(NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));

	#ifndef HAVE_W32_SYSTEM
		gpgme_set_locale(NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
	#endif
 
	if (GPG_ERR_NO_ERROR != gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP))
	{
		std::cerr << "Error check engine version";
		std::cerr << std::endl;
		return;
	}

	if (GPG_ERR_NO_ERROR != gpgme_get_engine_info(&INFO))
	{
		std::cerr << "Error getting engine info";
		std::cerr << std::endl;
		return;
	}

	/* Create New Contexts */
	if (GPG_ERR_NO_ERROR != gpgme_new(&CTX))
	{
		std::cerr << "Error creating GPGME Context";
		std::cerr << std::endl;
		return;
	}

	/* setup the protocol */
	if (GPG_ERR_NO_ERROR != gpgme_set_protocol(CTX, GPGME_PROTOCOL_OpenPGP))
	{
		std::cerr << "Error creating Setting Protocol";
		std::cerr << std::endl;
		return;
	}

	/* if we get to here -> we have inited okay */
	gpgmeInit = true;

//	return storeAllKeys();
}

 GPGAuthMgr::~GPGAuthMgr()
{
}

// store all keys in map mKeyList to avoid callin gpgme exe repeatedly
bool   GPGAuthMgr::storeAllKeys()
{
	gpg_error_t ERR;
	if (!gpgmeInit)
	{
		std::cerr << "Error since GPG is not initialised";
		std::cerr << std::endl;
		return false;
	}

         /* store keys */
	gpgme_key_t KEY = NULL;

	/* Initiates a key listing */
	if (GPG_ERR_NO_ERROR != gpgme_op_keylist_start (CTX, "", 1))
	{
		std::cerr << "Error iterating through KeyList";
		std::cerr << std::endl;
		return false;
	} 

	/* Loop until end of key */
	for(int i = 0;(GPG_ERR_NO_ERROR == (ERR = gpgme_op_keylist_next (CTX, &KEY))); i++)
	{
		/* store in pqiAuthDetails */
		pqiAuthDetails entryDetails;
		entryDetails.id = (KEY->subkeys) ? KEY->subkeys->keyid : NULL;
        	entryDetails.fpr= (KEY->subkeys) ? KEY->subkeys->fpr : NULL;
        	entryDetails.name = (KEY->uids) ? KEY->uids->name : NULL;
        	entryDetails.email = (KEY->uids) ? KEY->uids->email : NULL;		
	//	entryDetails.location = "here"; 
	//	entryDetails.org = "me.com"; 

		entryDetails.trustLvl = KEY->owner_trust; 
		entryDetails.ownsign = KEY->can_sign;   
		entryDetails.trusted = KEY->can_certify; 

		/* store in map */
		mKeyList.insert(std::make_pair(entryDetails.id,entryDetails));

		/* release key */
		gpgme_key_release (KEY);
	}
	return true;

}

	
bool   GPGAuthMgr:: active()
{
	return gpgmeInit;
}

int     GPGAuthMgr::InitAuth(const char *srvr_cert, const char *priv_key, 
                                        const char *passwd)
{
	return 1;
}

bool    GPGAuthMgr::CloseAuth()
{
	return true;
}

int     GPGAuthMgr::setConfigDirectories(std::string confFile, std::string neighDir)
{
	return 1;
}

std::string GPGAuthMgr::OwnId()
{
	return mOwnId;
}

bool	GPGAuthMgr::getAllList(std::list<std::string> &ids)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		ids.push_back(it->first);
	}
	return true;
}

bool	GPGAuthMgr::getAuthenticatedList(std::list<std::string> &ids)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		if (it->second.trustLvl > 3)
		{
			ids.push_back(it->first);
		}
	}
	return true;
}

bool	GPGAuthMgr::getUnknownList(std::list<std::string> &ids)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		if (it->second.trustLvl <= 3)
		{
			ids.push_back(it->first);
		}
	}
	return true;
}

bool	GPGAuthMgr::isValid(std::string id)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	return (mKeyList.end() != mKeyList.find(id));
}


bool	GPGAuthMgr::isAuthenticated(std::string id)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	if (mKeyList.end() != (it = mKeyList.find(id)))
	{
		return (it->second.trustLvl > 3);
	}
	return false;
}

std::string GPGAuthMgr::getName(std::string id)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	if (mKeyList.end() != (it = mKeyList.find(id)))
	{
		return it->second.name;
	}
	std::string empty("");
	return empty;
}

bool	GPGAuthMgr::getDetails(std::string id, pqiAuthDetails &details)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	if (mKeyList.end() != (it = mKeyList.find(id)))
	{
		details = it->second;
		return true;
	}
	return false;
}

bool GPGAuthMgr::FinalSaveCertificates()
{
	return false;
}

bool GPGAuthMgr::CheckSaveCertificates()
{
	return false;
}

bool GPGAuthMgr::saveCertificates()
{
	return false;
}

bool GPGAuthMgr::loadCertificates()
{
	return false;
}

bool GPGAuthMgr::LoadCertificateFromString(std::string pem, std::string &id)
{
	return false;
}

std::string GPGAuthMgr::SaveCertificateToString(std::string id)
{
	std::string dummy("CERT STRING");
	return dummy;
}

bool GPGAuthMgr::LoadCertificateFromFile(std::string filename, std::string &id)
{
	return false;
}

bool GPGAuthMgr::SaveCertificateToFile(std::string id, std::string filename)
{
	return false;
}
bool GPGAuthMgr::LoadCertificateFromBinary(const uint8_t *ptr, uint32_t len, std::string &id)
{
	return false;
}

bool GPGAuthMgr::SaveCertificateToBinary(std::string id, uint8_t **ptr, uint32_t *len)
{
	return false;
}

		/* Signatures */
bool GPGAuthMgr::AuthCertificate(std::string id)
{
	return false;
}

bool GPGAuthMgr::SignCertificate(std::string id)
{
	return false;
}

bool GPGAuthMgr::RevokeCertificate(std::string id)
{
	return false;
}

bool GPGAuthMgr::TrustCertificate(std::string id, bool trust)
{
	return false;
}

bool GPGAuthMgr::SignData(std::string input, std::string &sign)
{
	return false;
}

bool GPGAuthMgr::SignData(const void *data, const uint32_t len, std::string &sign)
{
	return false;
}


