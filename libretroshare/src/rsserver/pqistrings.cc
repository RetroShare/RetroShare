
/*
 * "$Id: pqistrings.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
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


#include "pqi/pqi.h"
#include "pqi/pqinetwork.h"

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "rsserver/pqistrings.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <time.h>

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
std::string getXPGPInfo(XPGP *cert);
#endif /* XPGP Certificates */
/**************** PQI_USE_XPGP ******************/

std::string get_status_string(int status)
{
	std::string sstr("");
	if (status & PERSON_STATUS_CONNECTED)
	{
		sstr += "Online";
		return sstr;
	}
	if (!(status & PERSON_STATUS_ACCEPTED))
	{
		sstr += "Denied Access";
		return sstr;
	}

	if (status & PERSON_STATUS_INUSE)
	{
		sstr += "Connecting";
		/*
		if (status & PERSON_STATUS_WILL_LISTEN)
		{
			sstr += "Listening";
		}
		sstr += "/";
		if (status & PERSON_STATUS_WILL_CONNECT)
		{
			sstr += "Connecting";
		}
		sstr += "]";
		*/
		return sstr;
	}
	sstr += "Unknown";
	return sstr;
}


std::string get_neighbourstatus_string(Person *p)
{
	// if connected - show how long
	// if !autoconnected - tick to connect.
	//
	// if connecting.
	// 	show time to next connect attempt.
	// else show the last connect time.

	std::ostringstream connstr;

	connstr << "Last Conn/Recv: ";
	int lct = time(NULL) - p -> lc_timestamp;
	int lrt = time(NULL) - p -> lr_timestamp;
	if (lct < 100000000)
	{
		connstr << get_timeperiod_string(lct);
	}
	else
	{
		connstr << "Never";
	}
	connstr << "/";
	if (lrt < 100000000)
	{
		connstr << get_timeperiod_string(lrt);
	}
	else
	{
		connstr << "Never";
	}

	return connstr.str();
}


int get_lastconnecttime(Person *p)
{
	std::ostringstream connstr;

	int lct = time(NULL) - p -> lc_timestamp;
	int lrt = time(NULL) - p -> lr_timestamp;
	if (lrt < lct)
	{
		lct = lrt;
	}
	return lct;
}

std::string get_lastconnecttime_string(Person *p)
{
	int lct = get_lastconnecttime(p);
	if (lct < 32000000)
	{
		return get_timeperiod_string(lct);
	}
	else
	{
		return std::string("Never");
	}
}


std::string get_autoconnect_string(Person *p)
{
	// if connected - show how long
	// if !autoconnected - tick to connect.
	//
	// if connecting.
	// 	show time to next connect attempt.
	// else show the last connect time.

	std::ostringstream connstr;

	Person *own = getSSLRoot() -> getOwnCert();
	if (p == own)
	{
		connstr << "Yourself"; 
		return connstr.str();
	}

	if (p -> Connected())
	{
		/*
		long ct = p->lc_timestamp;
		if (ct < p->lr_timestamp)
			ct = p->lr_timestamp;

		connstr << "Online: " << get_timeperiod_string(time(NULL) - ct);
		*/
		connstr << "Online";
	}
	else if (p -> Manual())
	{
		if (p->trustLvl < TRUST_SIGN_AUTHEN)
		{
			connstr << "Please Authenticate";
		}
		else
		{
			connstr << "Tick to Connect";
		}
	}
	else
	{
		connstr << "Offline";
	}

	/*
	else if (p -> WillConnect())
	{
		connstr << "Connect in:";
		connstr << get_timeperiod_string(p->nc_timestamp - time(NULL));
	}
	else
	{
		connstr << "Last Conn:";
		long ct = p->lc_timestamp;
		if (ct < p->lr_timestamp)
		{
			ct = p->lr_timestamp;
			connstr << "(I):";
		}
		else
		{
			connstr << "(O):";
		}
		connstr << get_timeperiod_string(time(NULL) - ct);
	}
	*/

	return connstr.str();
}


std::string get_trust_string(Person *p)
{
	std::ostringstream srvstr;

	/* This is now changing to display 2 things. 
	 *
	 * (1) - Proxy 
	 * (2) - Auth Level.
	 */

	Person *own = getSSLRoot() -> getOwnCert();
	if (p == own)
	{
		srvstr << "Yourself"; // Certificate";
		return srvstr.str();
	}

	switch(p -> trustLvl)
	{
		case TRUST_SIGN_OWN:
			srvstr << "Auth (S)"; //Good: Own Signature";
			break;
		case TRUST_SIGN_TRSTED:
			srvstr << "Trusted (ST)"; //Good: Trusted Signer";
			break;
		case TRUST_SIGN_AUTHEN:
			srvstr << "Auth"; //Good: Authenticated";
			break;
		case TRUST_SIGN_BASIC:
			srvstr << "Untrusted"; // : Acquaintance ";
			break;
		case TRUST_SIGN_UNTRUSTED:
			srvstr << "Unknown (2)";
			break;
		case TRUST_SIGN_UNKNOWN:
			srvstr << "Unknown (1)";
			break;
		case TRUST_SIGN_NONE:
			srvstr << "Unknown (0)";
			break;
		case TRUST_SIGN_BAD: /* not checked yet */ 
			srvstr << "Not Avail";
			break;
		default:
			srvstr << "UNKNOWN";
			break;
	}
	return srvstr.str();
}

		

std::string get_server_string(Person *p)
{
	std::ostringstream srvstr;

	if ((0 == inaddr_cmp(p -> serveraddr, 0)) || (p -> Local()))
	{
		if (0 == inaddr_cmp(p -> localaddr, 0))
		{
			srvstr << "Unknown Addr";
		}
		else
		{
			srvstr << "L: " << inet_ntoa(p -> localaddr.sin_addr);
			srvstr << ":" << ntohs(p -> localaddr.sin_port);
		}
	}
	else
	{
		srvstr << "S: " << inet_ntoa(p -> serveraddr.sin_addr);
		srvstr << ":" << ntohs(p -> serveraddr.sin_port);
	}

	// need own cert!
	Person *own = getSSLRoot() -> getOwnCert();

        if ((isValidNet(&(p->serveraddr.sin_addr))) &&
          (!isPrivateNet(&(p->serveraddr.sin_addr))) &&
          (!sameNet(&(own->localaddr.sin_addr), &(p->serveraddr.sin_addr))))
	{
		srvstr << " (Proxy-S) ";
	}
        else if ((!p->Firewalled()) && (isValidNet(&(p->localaddr.sin_addr))) &&
                (!isPrivateNet(&(p->localaddr.sin_addr))) &&
                (!sameNet(&(own->localaddr.sin_addr), &(p->localaddr.sin_addr))))
	{
		srvstr << " (Proxy-L) ";
	}
	return srvstr.str();
}

const int sec_per_min = 60;
const int sec_per_hour = 3600;
const int sec_per_day = 3600 * 24;

std::string get_timeperiod_string(int secs)
{

	int days = secs / sec_per_day;
	secs -= days * sec_per_day;
	int hours = secs / sec_per_hour;
	secs -= hours * sec_per_hour;
	int mins = secs / sec_per_min;
	secs -= mins * sec_per_min;

	std::ostringstream srvstr;

	if (days > 0)
	{
		srvstr << days << " days ";
	}
	else if (hours > 0)
	{
		srvstr << hours << ":" << mins << " hours";
	}
	else
	{
		srvstr << mins << ":" << secs << " min";
	}
	return srvstr.str();
}

std::string get_neighbour_info(cert *c)
{
	std::ostringstream ostr;

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	ostr << getX509CNString(c -> certificate -> subject -> subject);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	ostr << getX509CNString(c -> certificate -> cert_info -> subject);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/


	ostr << "\t" << get_neighbourstatus_string(c);
	return ostr.str();
}

std::string get_cert_info(cert *c)
{
	std::ostringstream ostr;
	ostr << "************ Certificate **************" << std::endl;
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	ostr << getXPGPInfo(c -> certificate);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	ostr << getX509Info(c -> certificate);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	ostr << "********** Connection Info ************" << std::endl;
	ostr << "Local Addr       : " << inet_ntoa(c -> localaddr.sin_addr);
	ostr << ":" << ntohs(c -> localaddr.sin_port) << std::endl;
	ostr << "Server Addr      : " << inet_ntoa(c -> serveraddr.sin_addr);
	ostr << ":" << ntohs(c -> serveraddr.sin_port) << std::endl;
	ostr << "FLAGS: ";
	if (!c -> Firewalled())
	{
		ostr << "Not ";
	}
	ostr << "Firewalled, ";
	if (!c -> Forwarded())
	{
		ostr << "Not ";
	}
	ostr << "Forwarded" << std::endl;
	ostr << std::endl;


	ostr << "Last Connect Addr: " << inet_ntoa(c -> lastaddr.sin_addr);
	ostr << ":" << ntohs(c -> lastaddr.sin_port) << std::endl;

	ostr << "Last Connect Time: ";
	ostr << get_timeperiod_string(time(NULL) - c -> lc_timestamp);
	ostr << std::endl;
	ostr << "Last Receive Time: ";
	ostr << get_timeperiod_string(time(NULL) - c -> lr_timestamp);
	ostr << std::endl;
	ostr << std::endl;

	ostr << "Next Connect in  : ";
	ostr << get_timeperiod_string(c->nc_timestamp - time(NULL));
	ostr << std::endl;

	ostr << "AutoConnect: " << get_autoconnect_string(c);
	ostr << std::endl;

	ostr << "***************************************" << std::endl;

	return ostr.str();
}


std::string getX509Info(X509 *cert)
{
	// Details from the structure
	// cert_info (X509_CINF *)
	// sig_alg (X509_ALGOR *)
	// signature (ASN1_BIT_STRING *)
	// valid (int)
	// references (int)
	// name (char *)
	//
	// random flags
	//
	// skid (ASN1_OCTET_STRING *)
	// akid (AUTHORITY_KEYID *)
	// aux (X509_CERT_AUX *)
	std::string certstr;
	char numstr[1000];

	sprintf(numstr, "%ld", ASN1_INTEGER_get(cert -> cert_info -> version));
	certstr += "Version: ";
	certstr += numstr;

	sprintf(numstr, "%ld", ASN1_INTEGER_get(cert -> 
					cert_info -> serialNumber));
	certstr += "\nSerial Number: ";
	certstr += numstr;

//	switch(cert -> cert_info -> signature)
//	{
//		case STANDRD:
//			certstr += "\nSig Algorithm: Standard";
//			break;
//		default:
//			certstr += "\nSig Algorithm: Unknown";
//			break;
//	}
//

	
	certstr += "\nSubject:\n";
	certstr += getX509NameString(cert -> cert_info -> subject);

	// Validity in Here.  cert -> cert_info -> validity;

	certstr += "\nIssuer:\n";
	certstr += getX509NameString(cert -> cert_info -> issuer);

	// Key cert -> cert_info -> key;
	//
	// IDS + extensions. cert -> cert_info -> issuerUID/subjectUID;
	// cert -> cert_info -> extensions;
	
	// END OF INFO.	
	
	// Next sigalg again?
	// next sig... 
	certstr += "\nSignature:";
	for(int i = 0; i < cert -> signature -> length;)
	{
		if (i % 128 == 0)
		{
			certstr += "\n\t";
		}
		char hbyte = 0;
		for(int j = 0; (j < 4) && (i < cert -> signature -> length);
				j++, i++)
		{
			hbyte = hbyte << 1;
			if (ASN1_BIT_STRING_get_bit(cert -> signature, i) == 1)
			{
				hbyte++;
				//std::cerr << "1";
			}
			else
			{
				//std::cerr << "0";
			}
		}
		if (hbyte > 9)
		{
			certstr += ('a' + (hbyte - 10));
		}
		else
		{
			certstr += ('0' + hbyte);
		}
		//std::cerr << " " << i << " " << (char) ('0' + hbyte);
		//std::cerr << " " << (int) hbyte << std::endl;
	}

	
	sprintf(numstr, "%d/%d", cert -> valid, cert -> references);
	certstr += "\nValid/References: ";
	certstr += numstr;
	certstr += "\n";

	// That will do for now.
	return certstr;
}

/* Helper function to convert a Epoch Timestamp
 * into a string.
 */

static const char *TIME_FORMAT_STR_BRIEF = "%H:%M:%S";
static const char *TIME_FORMAT_STR_OLDVAGUE_NOW = "%H:%M:%S";
static const char *TIME_FORMAT_STR_OLDVAGUE_WEEK = "%a %H:%M";
static const char *TIME_FORMAT_STR_OLDVAGUE_OLD = "%a, %d %b";
static const char *TIME_FORMAT_STR_LONG = "%c";
static const char *TIME_FORMAT_STR_NORMAL = "%a, %H:%M:%S";

std::string timeFormat(int epoch, int format)
{
	time_t ctime = epoch;
        struct tm *stime = localtime(&ctime);

	size_t msize = 1024;
	char space[msize];
	const char *fmtstr = NULL;

	if (format == TIME_FORMAT_OLDVAGUE)
	{
		int itime = time(NULL);
		int delta = abs(itime - ctime);
		if (delta < 12 * 3600)
		{
			format = TIME_FORMAT_OLDVAGUE_NOW;
		}
		else if (delta < 3 * 24 * 3600)
		{
			format = TIME_FORMAT_OLDVAGUE_WEEK;
		}
		else 
		{
			format = TIME_FORMAT_OLDVAGUE_OLD;
		}
	}

	switch(format)
	{
		case TIME_FORMAT_BRIEF:
			fmtstr = TIME_FORMAT_STR_BRIEF;
			break;
		case TIME_FORMAT_OLDVAGUE_NOW:
			fmtstr = TIME_FORMAT_STR_OLDVAGUE_NOW;
			break;
		case TIME_FORMAT_OLDVAGUE_WEEK:
			fmtstr = TIME_FORMAT_STR_OLDVAGUE_WEEK;
			break;
		case TIME_FORMAT_OLDVAGUE_OLD:
			fmtstr = TIME_FORMAT_STR_OLDVAGUE_OLD;
			break;
		case TIME_FORMAT_LONG:
			fmtstr = TIME_FORMAT_STR_LONG;
			break;
		case TIME_FORMAT_NORMAL:
		default:
			fmtstr = TIME_FORMAT_STR_NORMAL;
			break;
	}

	if (fmtstr != NULL) // Short -> Only Time.
	{
            int usize = strftime(space, msize, fmtstr, stime);
	    if (usize > 0)
	    	return std::string(space);
	}
	return std::string("");
}

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)


std::string getXPGPInfo(XPGP *cert)
{
	std::stringstream out;
	long l;
	int i,j;

	out << "XPGP Certificate:" << std::endl;
	l=XPGP_get_version(cert);
	out << "     Version: " << l+1 << "(0x" << l << ")" << std::endl;
	out << "     Subject: " << std::endl;
	out << "  " << getX509NameString(cert -> subject -> subject);
	out << std::endl;
	out << std::endl;
	out << "     Signatures:" << std::endl;

	for(i = 0; i < sk_XPGP_SIGNATURE_num(cert->signs); i++)
	{
		out << "Sign[" << i << "] -> [";

		XPGP_SIGNATURE *sig = sk_XPGP_SIGNATURE_value(cert->signs,i);
	        ASN1_BIT_STRING *signature = sig->signature;
	        int signlen = ASN1_STRING_length(signature);
	        unsigned char *signdata = ASN1_STRING_data(signature);

		/* only show the first 8 bytes */
		if (signlen > 8)
			signlen = 8;
		for(j=0;j<signlen;j++)
		{
			out << std::hex << std::setw(2) << (int) (signdata[j]);
			if ((j+1)%16==0)
			{
				out << std::endl;
			}
			else
			{
				out << ":";
			}
		}
		out << "] by:";
		out << std::endl;
		out << getX509NameString(sig->issuer);
		out << std::endl;
		out << std::endl;
	}

	return out.str();
}



std::string getXPGPAuthCode(XPGP *xpgp)
{
	/* get the self signature -> the first signature */

	std::stringstream out;
	if (1 >  sk_XPGP_SIGNATURE_num(xpgp->signs))
	{
		out.str();
	}

	XPGP_SIGNATURE *sig = sk_XPGP_SIGNATURE_value(xpgp->signs,0);
	ASN1_BIT_STRING *signature = sig->signature;
	int signlen = ASN1_STRING_length(signature);
	unsigned char *signdata = ASN1_STRING_data(signature);

	/* extract the authcode from the signature */
	/* convert it to a string, inverse of 2 bytes of signdata */
	if (signlen > 2)
		signlen = 2;
	int j;
	for(j=0;j<signlen;j++)
	{
		out << std::hex << std::setprecision(2) << std::setw(2) 
		<< std::setfill('0') << (unsigned int) (signdata[j]);
	}
	return out.str();
}

std::list<std::string> getXPGPsigners(XPGP *cert)
{
	std::list<std::string> signers;
	int i;

	for(i = 0; i < sk_XPGP_SIGNATURE_num(cert->signs); i++)
	{
		XPGP_SIGNATURE *sig = sk_XPGP_SIGNATURE_value(cert->signs,i);
		std::string str = getX509CNString(sig->issuer);
		signers.push_back(str);
		std::cerr << "XPGPsigners(" << i << ")" << str << std::endl;
	}
	return signers;
}


#endif /* XPGP Certificates */
/**************** PQI_USE_XPGP ******************/


std::string get_cert_name(cert *c)
{
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	return getX509CNString(c->certificate->subject -> subject);
#else
	return getX509CNString(c->certificate->cert_info -> subject);
#endif /* XPGP Certificates */
/**************** PQI_USE_XPGP ******************/
}

std::string get_cert_org(cert *c)
{
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	return getX509OrgString(c->certificate->subject -> subject);
#else
	return getX509OrgString(c->certificate->cert_info -> subject);
#endif /* XPGP Certificates */
/**************** PQI_USE_XPGP ******************/
}

std::string get_cert_loc(cert *c)
{
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	return getX509LocString(c->certificate->subject -> subject);
#else
	return getX509LocString(c->certificate->cert_info -> subject);
#endif /* XPGP Certificates */
/**************** PQI_USE_XPGP ******************/
}

std::string get_cert_country(cert *c)
{
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	return getX509CountryString(c->certificate->subject -> subject);
#else
	return getX509CountryString(c->certificate->cert_info -> subject);
#endif /* XPGP Certificates */
/**************** PQI_USE_XPGP ******************/
}


	


