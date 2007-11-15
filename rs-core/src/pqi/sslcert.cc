/*
 * Core PQI networking: sslcert.cc
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


#include "sslcert.h"

#include "pqi.h"
#include "pqinetwork.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <sstream>
#include <iomanip>

#include "pqidebug.h"

const int pqisslrootzone = 1211;


// other fns
std::string getCertName(cert *c)
{
	std::string name = c -> certificate -> name;
	// strip out bad chars.
	for(int i = 0; i < (signed) name.length(); i++)
	{
		if ((name[i] == '/') || (name[i] == ' ') || (name[i] == '=') ||
			(name[i] == '\\') || (name[i] == '\t') || (name[i] == '\n'))
		{
			name[i] = '_';
		}
	}
	return name;
}



int pem_passwd_cb(char *buf, int size, int rwflag, void *password)
{
	strncpy(buf, (char *)(password), size);
	buf[size - 1] = '\0';
	return(strlen(buf));
}


/* This class handles openssl library init/destruct.
 * only one of these... handles 
 * the CTX and setup?
 *
 * it will also handle the certificates.....
 * mantaining a library of recieved certs, 
 * and ip addresses that the connections come from...
 *
 */

// the single instance of this.
static sslroot instance_sslroot;

sslroot *getSSLRoot()
{
	return &instance_sslroot;
}

sslroot::sslroot()
	:sslctx(NULL), init(0), certsChanged(1), 
	certsMajorChanged(1), pkey(NULL)
{
}

int sslroot::active()
{
	return init;
}

// args: server cert, server private key, trusted certificates.

int	sslroot::initssl(const char *cert_file, const char *priv_key_file, 
		const char *CA_FILE, const char *passwd)
{
static  int initLib = 0;
	if (!initLib)
	{
		initLib = 1;
		SSL_load_error_strings();
		SSL_library_init();
	}
		
		
	if (init == 1)
	{
		return 1;
	}
		
	if ((cert_file == NULL) ||
		(priv_key_file == NULL) ||
		(passwd == NULL))
	{
		fprintf(stderr, "sslroot::initssl() missing parameters!\n");
		return 0;
	}
		
		
	SSL_load_error_strings();
	SSL_library_init();
	// XXX TODO
	// actions_to_seed_PRNG();

	pqioutput(PQL_WARNING, pqisslrootzone, "SSL Library Init!");

	// setup connection method
	sslctx = SSL_CTX_new(SSLv23_method());

	// setup cipher lists.
	SSL_CTX_set_cipher_list(sslctx, "DEFAULT");

	// certificates (Set Local Server Certificate).
	FILE *ownfp = fopen(cert_file, "r");
	if (ownfp == NULL)
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"Couldn't open Own Certificate!");
		return -1;
	}

	X509 *x509 = PEM_read_X509(ownfp, NULL, NULL, NULL);
	fclose(ownfp);
	if (x509 != NULL)
	{
		SSL_CTX_use_certificate(sslctx, x509);
		own_cert = makeCertificate(x509);
		if (own_cert == NULL)
		{
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"Failed to Make Own Cert!");
			return -1;
		}
		addCertificate(own_cert);
	}
	else
	{
		return -1;
	}


	// SSL_CTX_use_certificate_chain_file(sslctx, cert_file_chain);

	pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, "SSL Set Chain File");

	SSL_CTX_load_verify_locations(sslctx, CA_FILE, 0);

	// enable verification of certificates (PEER)
	SSL_CTX_set_verify(sslctx, SSL_VERIFY_PEER | 
			SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

	pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, "SSL Verification Set");

	// setup private key
	FILE *pkfp = fopen(priv_key_file, "rb");
	if (pkfp == NULL)
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "Couldn't Open PrivKey File!");
		closessl();
		return -1;
	}

	pkey = PEM_read_PrivateKey(pkfp, NULL, NULL, (void *) passwd);

	SSL_CTX_use_PrivateKey(sslctx, pkey);

	if (1 != SSL_CTX_check_private_key(sslctx))
	{
		std::ostringstream out;
		out << "Issues With Private Key! - Doesn't match your Cert" << std::endl;
		out << "Check your input key/certificate:" << std::endl;
		out << priv_key_file << " & " << cert_file;
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());

		closessl();
		return -1;
	}


	// Load CA for clients.
	STACK_OF(X509_NAME) *cert_names;
	cert_names = SSL_load_client_CA_file(CA_FILE);

	if (cert_names != NULL)
	{
		SSL_CTX_set_client_CA_list(sslctx, cert_names);
	}
	else
	{
		std::ostringstream out;
		out << "Couldn't Load Client CA files!" << std::endl;
		out << "Check That (" << CA_FILE << ") is valid";
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		closessl();
		return -1;
	}

	/* configure basics on the certificate. */
	std::string tagname; // = "LOCL:";
	own_cert -> Name(tagname + getX509CNString(own_cert -> certificate -> cert_info -> subject));

	init = 1;
	return 1;
}



int	sslroot::closessl()
{
	SSL_CTX_free(sslctx);

	// clean up private key....
	// remove certificates etc -> opposite of initssl.
	init = 0;
	return 1;
}

/* Context handling  */
SSL_CTX *sslroot::getCTX()
{
	return sslctx;
}

int     sslroot::setConfigDirs(const char *cdir, const char *ndir)
{
	certdir = cdir;
	neighbourdir = ndir;
	return 1;
}

static const unsigned int OPT_LEN = 16;
static const unsigned int VAL_LEN = 1000;

int     sslroot::saveCertificates()
{
	if (certfile.length() > 1)
		return saveCertificates(certfile.c_str());
	return -1;
}

	
int     sslroot::saveCertificates(const char *fname)
{
	// construct file name.
	//
	// create the file in memory - hash + sign.
	// write out data to a file.
	
	std::string neighdir = certdir + "/" + neighbourdir + "/";
	std::string configname = certdir + "/";
	configname += fname;

	std::map<std::string, std::string>::iterator mit;


	std::string conftxt;
	std::string empty("");
	unsigned int i;

	std::list<cert *>::iterator it;
	for(it = peercerts.begin(); it != peercerts.end(); it++)
	{
		std::string neighfile = neighdir + getCertName(*it) + ".pqi";
		savecertificate((*it), neighfile.c_str());
		conftxt += "CERT ";
		conftxt += getCertName(*it);
		conftxt += "\n";
		conftxt += (*it) -> Hash();
		conftxt += "\n";
	}

	// Now add the options.
	for(mit = settings.begin(); mit != settings.end(); mit++)
	{
		// only save the nonempty settings.
	      if (mit -> second != empty) {
		conftxt += "OPT ";
		for(i = 0; (i < OPT_LEN) && (i < mit -> first.length()); i++)
		{
			conftxt += mit -> first[i];
		}
		conftxt += "\n";
		for(i = 0; i < VAL_LEN; i++)
		{
			if (i < mit -> second.length())
			{
				conftxt += mit -> second[i];
			}
			else
			{
				conftxt += '\0';
			}
		}
		conftxt += "\n";
	      }
	}

	// now work out signature of it all. This relies on the 
	// EVP library of openSSL..... We are going to use signing
	// for the moment.

	unsigned int signlen = EVP_PKEY_size(pkey);
	unsigned char signature[signlen];

	//OpenSSL_add_all_digests();

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
			"EVP_SignInit Failure!");
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
			"EVP_SignUpdate Failure!");
	}


	if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
			"EVP_SignFinal Failure!");
	}

	{
	  std::ostringstream out;
	  out << "Conf Signature is(" << signlen << "): ";
	  for(i = 0; i < signlen; i++) 
	  {
		out << std::hex << std::setw(2) <<  (int) signature[i];
		conftxt += signature[i];
	  }
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}

	FILE *cfd = fopen(configname.c_str(), "wb");
	int wrec;
	if (1 != (wrec = fwrite(conftxt.c_str(), conftxt.length(), 1, cfd)))
	{
		std::ostringstream out;
		out << "Error writing: " << configname << std::endl;
		out << "Wrote: " << wrec << "/" << 1 << " Records" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}

	EVP_MD_CTX_destroy(mdctx);
	fclose(cfd);

	return 1;
}

int     sslroot::loadCertificates(const char *conf_fname)
{
	// open the configuration file.
	//
	// read in CERT + Hash.

	// construct file name.
	//
	// create the file in memory - hash + sign.
	// write out data to a file.
	
	std::string neighdir = certdir + "/" + neighbourdir + "/";
	std::string configname = certdir + "/";
	configname += conf_fname;

	// save name for later save attempts.
	certfile = conf_fname;

	std::string conftxt;

	unsigned int maxnamesize = 1024;
	char name[maxnamesize];

	int c;
	unsigned int i;

	FILE *cfd = fopen(configname.c_str(), "rb");
	if (cfd == NULL)
	{
		std::ostringstream out;
		out << "Unable to Load Configuration File!" << std::endl;
		out << "File: " << configname << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		return -1;
	}

	std::list<std::string> fnames;
	std::list<std::string> hashes;
	std::map<std::string, std::string>::iterator mit;
	std::map<std::string, std::string> tmpsettings;

	unsigned int signlen = EVP_PKEY_size(pkey);
	unsigned char conf_signature[signlen];
	char *ret = NULL;

	for(ret = fgets(name, maxnamesize, cfd); 
			((ret != NULL) && (!strncmp(name, "CERT ", 5)));
				ret = fgets(name, maxnamesize, cfd))
	{
		for(i = 5; (name[i] != '\n') && (i < (unsigned) maxnamesize); i++);

		if (name[i] == '\n')
		{
			name[i] = '\0';
		}

		// so the name is first....
		std::string fname = &(name[5]);

		// now read the 
		std::string hash;
		std::string signature;

		for(i = 0; i < signlen; i++)
		{
			if (EOF == (c = fgetc(cfd)))
			{
				std::ostringstream out;
				out << "Error Reading Signature of: ";
				out << fname;
				out << std::endl;
				out << "ABorting Load!";
				out << std::endl;
				pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
				return -1;
			}
			unsigned char uc = (unsigned char) c;
			signature += (unsigned char) uc;
		}
		if ('\n' != (c = fgetc(cfd)))
		{
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"Warning Mising seperator");
		}

		{
		  std::ostringstream out;
		  out << "Read fname:" << fname << std::endl;
		  out << "Signature:" << std::endl;
		  for(i = 0; i < signlen; i++) 
		  {
			out << std::hex << std::setw(2) << (int) signature[i];
		  }
		  out << std::endl;
		  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		}

		// push back.....
		fnames.push_back(fname);
		hashes.push_back(signature);

		conftxt += "CERT ";
		conftxt += fname;
		conftxt += "\n";
		conftxt += signature;
		conftxt += "\n";

		// be sure to write over a bit...
		name[0] = 'N';
		name[1] = 'O';
	}

	// string already waiting!
	for(; ((ret != NULL) && (!strncmp(name, "OPT ", 4)));
				ret = fgets(name, maxnamesize, cfd))
	{
		for(i = 4; (name[i] != '\n') && (i < OPT_LEN); i++);
		// terminate the string.
		name[i] = '\0';

		// so the name is first....
		std::string opt = &(name[4]);

		// now read the 
		std::string val;     // cleaned up value.
		std::string valsign; // value in the file.

		for(i = 0; i < VAL_LEN; i++)
		{
			if (EOF == (c = fgetc(cfd)))
			{
		  		std::ostringstream out;
				out << "Error Reading Value of: ";
				out << opt;
				out << std::endl;
				out << "ABorting Load!";
				out << std::endl;
				pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
				return -1;
			}
			// remove zeros on strings...
			if (c != '\0')
			{
				val += (unsigned char) c;
			}
			valsign += (unsigned char) c;
		}
		if ('\n' != (c = fgetc(cfd)))
		{
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"Warning Mising seperator");
		}

		{
		  std::ostringstream out;
		  out << "Read OPT:" << opt;
		  out << " Val:" << val << std::endl;
		  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		}

		// push back.....
		tmpsettings[opt] = val;

		conftxt += "OPT ";
		conftxt += opt;
		conftxt += "\n";
		conftxt += valsign;
		conftxt += "\n";

		// be sure to write over a bit...
		name[0] = 'N';
		name[1] = 'O';
	}

	// only read up to the first newline symbol....
	// continue...
	for(i = 0; (name[i] != '\n') && (i < signlen); i++);

		//printf("Stepping over [%d] %0x\n", i, name[i]);


	if (i != signlen)
	{
		for(i++; i < signlen; i++)
		{
			c = fgetc(cfd);
			if (c == EOF)
			{
				pqioutput(PQL_ALERT, pqisslrootzone,
					"Error Reading Conf Signature:");
				return 1;
			}
			unsigned char uc = (unsigned char) c;
			name[i] = uc;
		}
	}

	{
	  std::ostringstream out;
	  out << "Configuration File Signature: " << std::endl;
	  for(i = 0; i < signlen; i++) 
	  {
		out << std::hex << std::setw(2) << (int) name[i];
	  }
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}


	// when we get here - should have the final signature in the buffer.
	// check.
	//
	// compare signatures.
	// instead of verifying with the public key....
	// we'll sign it again - and compare .... FIX LATER...
	
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	if (0 == EVP_SignInit(mdctx, EVP_sha1()))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignInit Failure!");
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignUpdate Failure!");
	}

	if (0 == EVP_SignFinal(mdctx, conf_signature, &signlen, pkey))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignFinal Failure!");
	}

	EVP_MD_CTX_destroy(mdctx);
	fclose(cfd);

	{
	  std::ostringstream out;
	  out << "Recalced File Signature: " << std::endl;
	  for(i = 0; i < signlen; i++) 
	  {
		out << std::hex << std::setw(2) << (int) conf_signature[i];
	  }
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}

	bool same = true;
	for(i = 0; i < signlen; i++)
	{
		if ((unsigned char) name[i] != conf_signature[i])
		{
			same = false;
		}
	}

	if (same == false)
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"ERROR VALIDATING CONFIGURATION! -- PLEASE FIX!");
		return -1;
	}
	std::list<std::string>::iterator it;
	std::list<std::string>::iterator it2;
	for(it = fnames.begin(), it2 = hashes.begin(); it != fnames.end(); it++, it2++)
	{
		std::string neighfile = neighdir + (*it) + ".pqi";
		cert *nc = loadcertificate(neighfile.c_str(), (*it2));
		if (nc != NULL)
		{
			if (0 > addCertificate(nc))
			{
				// cleanup.
				pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
					"Updated Certificate....but no need for addition");
				// X509_free(nc -> certificate);
				//delete nc;
			}
		}
	}
	for(mit = tmpsettings.begin(); mit != tmpsettings.end(); mit++)
	{
		settings[mit -> first] = mit -> second;
	}
	return 1;
}


int     sslroot::savecertificate(cert *c, const char *fname)
{
	// load certificates from file.
	FILE *setfp = fopen(fname, "wb");
	if (setfp == NULL)
	{
		std::ostringstream out;
		out << "sslroot::savecertificate() Bad File: " << fname;
		out << " Cannot be Written!" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		return -1;
	}

	{
	  std::ostringstream out;
	  out << "Writing out Cert...:" << c -> Name() << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}

	PEM_write_X509(setfp, c -> certificate);

	// writing out details....
	
	// read in a line.....
	int size = 1024;
	char line[size];
	std::list<cert *>::iterator it;

	int i;

	// This will need to be made portable.....

	struct sockaddr_in *addr_inet;
	struct sockaddr_in *addr_inet2;
	struct sockaddr_in *addr_inet3;

	int pos_status = 0;
	int pos_addr = sizeof(int);
	int pos_addr2 = pos_addr + sizeof(*addr_inet);
	int pos_addr3 = pos_addr2 + sizeof(*addr_inet2);

	int pos_lcts = pos_addr3 + sizeof(*addr_inet3);
	int pos_lrts = pos_lcts + sizeof(int);
	int pos_ncts = pos_lrts + sizeof(int);
	int pos_ncvl = pos_ncts + sizeof(int);
	int pos_name = pos_ncvl + sizeof(int);
	int pos_end = pos_name + 20; // \n. for readability.

	int *status = (int *) &(line[pos_status]);
	addr_inet = (struct sockaddr_in *) &(line[pos_addr]);
	addr_inet2 = (struct sockaddr_in *) &(line[pos_addr2]);
	addr_inet3 = (struct sockaddr_in *) &(line[pos_addr3]);
	int *lcts = (int *) &(line[pos_lcts]);
	int *lrts = (int *) &(line[pos_lrts]);
	char *ncts = &(line[pos_ncts]);
	char *ncvl = &(line[pos_ncvl]);
	char *name = &(line[pos_name]);
	char *end = &(line[pos_end]);

	for(i = 0; i < 1024; i++) 
		line[i] = 0; 

	*status = c -> Status();
	*addr_inet = c -> lastaddr;
	*addr_inet2 = c -> localaddr;
	*addr_inet3 = c -> serveraddr;

	*lcts = c -> lc_timestamp;
	*lrts = c -> lr_timestamp;
	*ncts = c -> nc_timestamp;
	*ncvl = c -> nc_timeintvl;

	std::string tmpname = c -> Name();
	for(i = 0; (i < (signed) tmpname.length()) && (i < 20 - 1); i++)
	{
		name[i] = tmpname[i];
	}
	name[20 - 1] = '\0';
	end[0] = '\n';


	if (1 != fwrite(line, size,1, setfp))
	{
		pqioutput(PQL_ALERT, pqisslrootzone,
			"Error Writing Peer Record!");
		return -1;
	}
	fclose(setfp);

	// then reopen to generate hash.
	setfp = fopen(fname, "rb");
	if (setfp == NULL)
	{
		std::ostringstream out;
		out << "sslroot::savecertificate() Bad File: " << fname;
		out << " Opened for ReHash!" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		return -1;
	}

	unsigned int signlen = EVP_PKEY_size(pkey);
	unsigned char signature[signlen];

	int maxsize = 10240;
	int rbytes;
	char inall[maxsize];
	if (0 == (rbytes = fread(inall, 1, maxsize, setfp)))
	{
		pqioutput(PQL_ALERT, pqisslrootzone,
			"Error Writing Peer Record!");
		return -1;
	}

	{
	  std::ostringstream out;
	  out << "Read " << rbytes << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}

	// so we read rbytes.
	// hash.
	//OpenSSL_add_all_digests();

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignInit Failure!");
	}

	if (0 == EVP_SignUpdate(mdctx, inall, rbytes))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignUpdate Failure!");
	}

	if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignFinal Failure!");
	}

	std::string signstr;
	{
	  std::ostringstream out;
	  out << "Cert + Setting Signature is(" << signlen << "): ";
	  for(i = 0; i < (signed) signlen; i++) 
	  {
		out << std::hex << std::setw(2) << (int) signature[i];
		signstr += signature[i];
	  }
	  out << std::dec << std::endl;

	  c -> Hash(signstr);
	  out << "Stored Hash Length: " << (c -> Hash()).length() << std::endl;
	  out << "Real Hash Length: " << signlen << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}

	fclose(setfp);

	EVP_MD_CTX_destroy(mdctx);

	return 1;
}

cert *sslroot::loadcertificate(const char *fname, std::string hash)
{
	// if there is a hash - check that the file matches it before loading.
	FILE *pcertfp;
	if (hash.length() > 1)
	{
		pcertfp = fopen(fname, "rb");
		// load certificates from file.
		if (pcertfp == NULL)
		{
			std::ostringstream out;
			out << "sslroot::loadcertificate() Bad File: " << fname;
			out << " Cannot be Hashed!" << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
			return NULL;
		}

		unsigned int signlen = EVP_PKEY_size(pkey);
		unsigned char signature[signlen];

		int maxsize = 10240;
		int rbytes;
		char inall[maxsize];
		if (0 == (rbytes = fread(inall, 1, maxsize, pcertfp)))
		{
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"Error Reading Peer Record!");
			return NULL;
		}

		EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
		if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
		{
			pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignInit Failure!");
		}
	
		if (0 == EVP_SignUpdate(mdctx, inall, rbytes))
		{
			pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignUpdate Failure!");
		}
	
		if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
		{
			pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignFinal Failure!");
		}

		fclose(pcertfp);
		EVP_MD_CTX_destroy(mdctx);
	
		bool same = true;
		if (signlen != hash.length())
		{
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"Different Length Signatures... Cannot Load Cert!");
			return NULL;
		}

		for(int i = 0; i < (signed) signlen; i++) 
		{
			if (signature[i] != (unsigned char) hash[i])
			{
				same = false;
				pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
					"Invalid Signature... Cannot Load Certificate!");
				return NULL;
			}
		}

		{
		  std::ostringstream out;
		  out << "Verified Signature for: " << fname;
		  out << std::endl;
		  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		}
	
	
	}
	else
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "Not checking cert signature");
	}

	pcertfp = fopen(fname, "rb");

	// load certificates from file.
	if (pcertfp == NULL)
	{
		std::ostringstream out;
		out << "sslroot::loadcertificate() Bad File: " << fname;
		out << " Cannot be Read!" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		return NULL;
	}


	X509 *pc;
	cert *npc = NULL;

	if ((pc = PEM_read_X509(pcertfp, NULL, NULL, NULL)) != NULL)
	{
		// read a certificate.
		std::ostringstream out;
		out << "Loaded Certificate: ";
		out << pc -> name << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());

		npc = makeCertificate(pc);
		if (npc == NULL)
		{
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, "Failed to Create Cert!");
			return NULL;
		}
	}
	else // (pc == NULL)
	{
		unsigned long err = ERR_get_error();
		std::ostringstream out;
		out << "Read Failed .... CODE(" << err << ")" << std::endl;
		out << ERR_error_string(err, NULL) << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		return NULL;
	}

	// Now we try to read in 1024 bytes.....
	// if successful, then have settings!

	// read in a line.....
	int size = 1024;
	char line[size];

	// Data arrangment.
	// so far
	// ------------
	// 4 - (int) status
	// 8 - sockaddr
	// 8 - sockaddr
	// 8 - sockaddr
	// 4 - lc_timestamp
	// 4 - lr_timestamp
	// 4 - nc_timestamp
	// 4 - nc_timeintvl
	// 20 - name.
	// 1 - end
	
	// This will need to be made portable.....

	struct sockaddr_in *addr_inet;
	struct sockaddr_in *addr_inet2;
	struct sockaddr_in *addr_inet3;

	//int pos_status = 0;
	int pos_addr = sizeof(int);
	int pos_addr2 = pos_addr + sizeof(*addr_inet);
	int pos_addr3 = pos_addr2 + sizeof(*addr_inet2);
	int pos_lcts = pos_addr3 + sizeof(*addr_inet3);
	
	int pos_lrts = pos_lcts + sizeof(int);
	int pos_ncts = pos_lrts + sizeof(int);
	int pos_ncvl = pos_ncts + sizeof(int);
	int pos_name = pos_ncvl + sizeof(int);
	//int pos_end = pos_name + 20; // \n. for readability.

	int *status = (int *) line;
	addr_inet = (struct sockaddr_in *) &(line[pos_addr]);
	addr_inet2 = (struct sockaddr_in *) &(line[pos_addr2]);
	addr_inet3 = (struct sockaddr_in *) &(line[pos_addr3]);
	int *lcts = (int *) &(line[pos_lcts]);
	int *lrts = (int *) &(line[pos_lrts]);
	char *ncts = &(line[pos_ncts]);
	char *ncvl = &(line[pos_ncvl]);
	char *name = &(line[pos_name]);
	//char *end = &(line[pos_end]);

	// end of data structures....

	if (1 != (signed) fread(line, size,1, pcertfp))
	{
		pqioutput(PQL_WARNING, pqisslrootzone, 
			"Error Reading Setting: Only Cert Retrieved");
		return npc;
	}


	// fill in the data.
	cert *c = npc;
	c -> Status(*status);
	// but ensure that inUse is not set.
	c -> InUse(false);
			
	c -> lastaddr = *addr_inet;
	c -> localaddr = *addr_inet2;
	c -> serveraddr = *addr_inet3;

	c -> lc_timestamp = *lcts;
	c -> lr_timestamp = *lrts;
	c -> nc_timestamp = *ncts;
	c -> nc_timeintvl = *ncvl;


	name[20 - 1] = '\0';
	c -> Name(std::string(name));

	// save the hash.
	c -> Hash(hash);

	fclose(pcertfp);

	// small hack - as the timestamps seem wrong.....
	// could be a saving thing - or a bug....
	c -> lc_timestamp = 0;
	c -> lr_timestamp = 0;

	// reset these. as well.
	c -> nc_timestamp = 0;
	c -> nc_timeintvl = 5;

	return c;
}

	
int     sslroot::printCertificate(cert *c, std::ostream &out)
{
	out << "Cert Name:" << (c -> certificate) -> name << std::endl;
	//X509_print_fp(stderr, c -> certificate);
	return 1;
}

// This function will clean up X509 *c if necessary.

cert    *sslroot::makeCertificate(X509 *c)
{
	if (c == NULL)
	{
		return NULL;
	}

	// At this point we check to see if there is a duplicate.
	cert    *dup = checkDuplicateX509(c);
	cert *npc = NULL;
	if (dup == NULL)
	{
		npc = new cert();
		npc -> certificate = c;
		if (!addtosignmap(npc)) // only allow the cert if no dup
		{

			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"sslroot::makeCertificate() Failed to Get Signature - Not Allowed!");

			// failed!... cannot add it!.
			delete npc;
			return NULL;
		}

		allcerts.push_back(npc);
		{
		  std::ostringstream out;
		  out << "sslroot::makeCertificate() For " << c -> name;
		  out << " A-Okay!" << std::endl;
		  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		}
		// at this point we need to add to the signaturelist....
	
	}
	else if (c == dup -> certificate)
	{
		// identical - so okay.
		npc = dup;
		std::ostringstream out;
		out << "sslroot::makeCertificate() For " << c -> name;
		out << " Found Identical - A-Okay!" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}
	else
	{
		std::ostringstream out;
		out << "sslroot::makeCertificate() For " << c -> name;
		out << " Cleaning up other X509!" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		// clean up c.
		X509_free(c);
		npc = dup;
	}
	return npc;
}


cert    *sslroot::checkDuplicateX509(X509 *x)
{
	if (x == NULL)
		return NULL;

	// loop through and print - then check.
	std::list<cert *>::iterator it;
	for(it = allcerts.begin(); it != allcerts.end(); it++)
	{
		if (0 == X509_cmp((*it) -> certificate, x))
		{
			return (*it);
		}
	}
	return NULL;
}


cert    *sslroot::checkPeerX509(X509 *x)
{
	if (x == NULL)
		return NULL;

	// loop through and print - then check.
	std::list<cert *>::iterator it;
	for(it = peercerts.begin(); it != peercerts.end(); it++)
	{
		if (0 == X509_cmp((*it) -> certificate, x))
		{
			return (*it);
		}
	}
	return NULL;
}



cert    *sslroot::findpeercert(const char *name)
{
	// loop through and print - then check.
	//std::cerr << "Checking Certs for: " << name << std::endl;
	std::list<cert *>::iterator it;
	for(it = peercerts.begin(); it != peercerts.end(); it++)
	{
		char *certname = ((*it) -> certificate) -> name;
		//std::cerr << "Cert Name:" << certname << std::endl;
		if (strstr(certname, name) != NULL)
		{
			//std::cerr << "Matches!" << std::endl;
			return (*it);
		}
	}
	pqioutput(PQL_DEBUG_BASIC, pqisslrootzone,
		"sslroot::findpeercert() Failed!");
	return NULL;
}

// returns zero for the same.
int	sslroot::compareCerts(cert *a, cert *b)
{
	// std::cerr << "Comparing Certificates:" << std::endl;
	//printCertificate(a);
	//printCertificate(b);
	//X509_print_fp(stderr, a -> certificate);
	//X509_print_fp(stderr, b -> certificate);
	
	int val = X509_cmp(a -> certificate, b -> certificate);

	{
	  std::ostringstream out;
	  out << "Certificate Comparison Returned: " << val << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}
	
	return val;
}

cert *	sslroot::registerCertificate(X509 *nc, struct sockaddr_in raddr, bool in)
{
	if (nc == NULL)
		return NULL;

	// shoud check all certs.
	cert *c = checkDuplicateX509(nc);
	if (c != NULL)
	{
		if (c -> certificate == nc)
		{
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"sslroot::registerCertificate() Found Identical X509 cert");
		}
		else
		{
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"sslroot::registerCertificate() Found Same X509 cert/diff mem - Clean");
			X509_free(nc);
		}

		if (!c -> Connected())
		{
			c -> lastaddr = raddr;

			if (in == true)
			{
				c -> lr_timestamp = time(NULL);
				// likely to be server address 
				// (with default port)
				// if null!
				if (!isValidNet(&(c -> serveraddr.sin_addr)))
				{
				  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				  	"Guessing Default Server Addr!");

				  c -> serveraddr = raddr;
				  c -> serveraddr.sin_port = 
					htons(PQI_DEFAULT_PORT);
				}
			}
			else
			{
				c -> lc_timestamp = time(NULL);
				// also likely to be servera address,
				// but we can check and see if its local.
				// can flag local 
				if (0 == inaddr_cmp(c -> localaddr, raddr))
				{
					c -> Local(true);
					// don't set serveraddr -> just ignore
				}
				else
				{
					c -> serveraddr = raddr;
					c -> Firewalled(false);
				}
			}
		}
		else
		{
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"WARNING: attempt to reg CONNECTED Cert!");
		}
		return c;
	}

	{
	  std::ostringstream out;
	  out << "sslroot::registerCertificate() Certificate Not Found!" << std::endl;
	  out << "Saving :" << nc -> name << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}
	
	cert *npc = makeCertificate(nc);
	if (npc == NULL)
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
			"Failed to Make Certificate");
		return NULL; 
	}

	npc -> Name(nc -> name);

	npc -> lastaddr = raddr;
	if (in == true)
	{
		npc -> lr_timestamp = time(NULL);
		// likely to be server address (with default port)
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, "Guessing Default Server Addr!");
		npc -> serveraddr = raddr;
		npc -> serveraddr.sin_port = htons(PQI_DEFAULT_PORT);
	}
	else
	{
		npc -> lc_timestamp = time(NULL);

		// as it is a new cert... all fields are
		// null and the earlier tests must be 
		// delayed until the discovery packets.

		// also likely to be server.
		npc -> serveraddr = raddr;
	}

	// push back onto collected.
	npc -> nc_timestamp = 0;
	collectedcerts.push_back(npc);

	// return NULL to indicate that it dosen't yet exist in dbase.
	return NULL;
}

cert *  sslroot::getCollectedCert()
{
	if (collectedcerts.size() < 1)
		return NULL;

	cert *c = collectedcerts.front();
	collectedcerts.pop_front();
	return c;
}

bool    sslroot::collectedCerts()
{
	return (collectedcerts.size() > 0);
}


int	sslroot::removeCertificate(cert *c)
{
	if (c -> InUse())
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone,
			"sslroot::removeCertificate() Failed: cert is in use.");
		return -1;
	}

	std::list<cert *>::iterator it;
	for(it = peercerts.begin(); it != peercerts.end(); it++)
	{
		if (c == (*it))
		{
			peercerts.erase(it);

			c -> InUse(false);
			c -> Accepted(false);

			
			pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
				"sslroot::removeCertificate() Success! Moved to Coll Certs");

			collectedcerts.push_back(c);

			certsChanged.IndicateChanged();
			certsMajorChanged.IndicateChanged();
			return 1;
		}
	}
	pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
		"sslroot::removeCertificate() Failed to Match Cert!");

	return 0;
}


int	sslroot::addCertificate(cert *c)
{
	c -> InUse(false);
	// let most flags through.
	//c -> Accepted(false);
	//c -> WillConnect(false);
	if (c -> certificate == NULL)
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
			"sslroot::addCertificate() certificate==NULL, Not Adding");
		return 0;
	}

	cert *dup = checkPeerX509(c -> certificate);
	if (dup != NULL)
	{
		std::ostringstream out;
		out << "sslroot::addCertificate() Not Adding";
		out << "Certificate with duplicate...." << std::endl;
		out << "\t\tTry RegisterCertificate() " << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());

		return -1;
	}

	// else put in in the list.
	peercerts.push_back(c);

	certsChanged.IndicateChanged();
	certsMajorChanged.IndicateChanged();

	return 1;
}


int	sslroot::addUntrustedCertificate(cert *c)
{
	// blank it all.
	c -> Status(PERSON_STATUS_MANUAL);
	// set Tag to be their X509CN.
	c -> Name(getX509CNString(c -> certificate -> cert_info -> subject));

	return addCertificate(c);
}



int	sslroot::validateCertificate(cert *c)
{
	std::ostringstream out;
	out << "sslroot::validateCertificate() Why Not!" << std::endl;
	c -> Valid(true);
	out << "Cert Status: " << c -> Status() << std::endl;
	pqioutput(PQL_ALERT, pqisslrootzone, out.str());
	return 1;
}

/***** REMOVED!
 *
 *
std::list<std::string> sslroot::listCertificates()
{
	std::list<std::string> names;
	std::list<cert *>::iterator it;
	for(it = peercerts.begin(); it != peercerts.end(); it++)
	{
		names.push_back(((*it) -> certificate) -> name);
	}
	return names;
}
 *
 *
 *****/


bool sslroot::CertsChanged()
{
	return certsChanged.Changed(0);
}

bool sslroot::CertsMajorChanged()
{
	return certsMajorChanged.Changed(0);
}

void sslroot::IndicateCertsChanged()
{
	certsChanged.IndicateChanged();
}


std::list<cert *> &sslroot::getCertList()
{
	return peercerts;
}

std::string sslroot::getSetting(std::string opt)
{
	std::map<std::string, std::string>::iterator it;
	if (settings.end() != (it = settings.find(opt)))
	{
		// found setting.
	  	std::ostringstream out;
		out << "sslroot::getSetting(" << opt << ") = ";
		out << it -> second << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
		return it -> second;
	}
	// else return empty string.
	
	{
	  std::ostringstream out;
	  out << "sslroot::getSetting(" << opt;
	  out << ") Not There!" << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}

	std::string empty("");
	return empty;
}

void sslroot::setSetting(std::string opt, std::string val)
{
	// check settings..
	std::ostringstream out;
	out << "sslroot::saveSetting(" << opt << ", ";
	out << val << ")" << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());

	settings[opt] = val;
	return;
}

cert *sslroot::getOwnCert()
{
	return own_cert;
}

int     sslroot::checkNetAddress()
{
	std::list<std::string> addrs = getLocalInterfaces();
	std::list<std::string>::iterator it;

	bool found = false;
	for(it = addrs.begin(); (!found) && (it != addrs.end()); it++)
	{
		if ((*it) == inet_ntoa(own_cert -> localaddr.sin_addr))
		{
			found = true;
		}
	}
        /* check that we didn't catch 0.0.0.0 - if so go for prefered */
        if ((found) && (own_cert -> localaddr.sin_addr.s_addr == 0))
        {
                found = false;
        }

	if (!found)
	{
		own_cert -> localaddr.sin_addr = getPreferredInterface();
	}
	if ((isPrivateNet(&(own_cert -> localaddr.sin_addr))) ||
		(isLoopbackNet(&(own_cert -> localaddr.sin_addr))))
	{
		own_cert -> Firewalled(true);
	}
	else
	{
		//own_cert -> Firewalled(false);
	}

	int port = ntohs(own_cert -> localaddr.sin_port);
	if ((port < PQI_MIN_PORT) || (port > PQI_MAX_PORT))
	{
		own_cert -> localaddr.sin_port = htons(PQI_DEFAULT_PORT);
	}

       /* if localaddr = serveraddr, then ensure that the ports
	* are the same (modify server)... this mismatch can
	* occur when the local port is changed....
	*/

        if (own_cert -> localaddr.sin_addr.s_addr == 
			own_cert -> serveraddr.sin_addr.s_addr)
	{
		own_cert -> serveraddr.sin_port =     
			own_cert -> localaddr.sin_port; 
	}

        // ensure that address family is set, otherwise windows Barfs.
        own_cert -> localaddr.sin_family = AF_INET;
        own_cert -> serveraddr.sin_family = AF_INET;
        own_cert -> lastaddr.sin_family = AF_INET;

	return 1;
}



		
/********** SSL ERROR STUFF ******************************************/

int printSSLError(SSL *ssl, int retval, int err, unsigned long err2, 
		std::ostream &out)
{
	std::string reason;

	std::string mainreason = std::string("UNKNOWN ERROR CODE");
	if (err == SSL_ERROR_NONE)
	{
		mainreason =  std::string("SSL_ERROR_NONE");
	}
	else if (err == SSL_ERROR_ZERO_RETURN)
	{
		mainreason =  std::string("SSL_ERROR_ZERO_RETURN");
	}
	else if (err == SSL_ERROR_WANT_READ)
	{
		mainreason =  std::string("SSL_ERROR_WANT_READ");
	}
	else if (err == SSL_ERROR_WANT_WRITE)
	{
		mainreason =  std::string("SSL_ERROR_WANT_WRITE");
	}
	else if (err == SSL_ERROR_WANT_CONNECT)
	{
		mainreason =  std::string("SSL_ERROR_WANT_CONNECT");
	}
	else if (err == SSL_ERROR_WANT_ACCEPT)
	{
		mainreason =  std::string("SSL_ERROR_WANT_ACCEPT");
	}
	else if (err == SSL_ERROR_WANT_X509_LOOKUP)
	{
		mainreason =  std::string("SSL_ERROR_WANT_X509_LOOKUP");
	}
	else if (err == SSL_ERROR_SYSCALL)
	{
		mainreason =  std::string("SSL_ERROR_SYSCALL");
	}
	else if (err == SSL_ERROR_SSL)
	{
		mainreason =  std::string("SSL_ERROR_SSL");
	}
	out << "RetVal(" << retval;
	out << ") -> SSL Error: " << mainreason << std::endl;
	out << "\t + ERR Error: " << ERR_error_string(err2, NULL) << std::endl;
	return 1;
}

cert::cert()
	:certificate(NULL), hash("")
{
	return;
}

cert::~cert()
{
	return;
}

std::string cert::Signature()
{
	if (certificate == NULL)
	{
		return Name();
	}
	else
	{
		// get signature from cert....
		return Name();
	}
}


std::string cert::Hash()
{
	return hash;
}


void cert::Hash(std::string h)
{
	hash = h;
	return;
}



/********************* signature stuff *********************/

bool certsign::operator<(const certsign &ref) const
{
	//compare the signature.
	if (0 > strncmp(data, ref.data, CERTSIGNLEN))
		return true;
	return false;
}


bool certsign::operator==(const certsign &ref) const
{
	//compare the signature.
	return (0 == strncmp(data, ref.data, CERTSIGNLEN));
}


/* Fns for relating cert signatures to structures */
cert *sslroot::findcertsign(certsign &sign)
{
	std::map<certsign, cert *>::iterator it;

	std::ostringstream out;
	out << "sslroot::findcertsign()" << std::endl;
	for (it = signmap.begin(); it != signmap.end(); it++)
	{
		out << "Checking Vs " << it -> second -> Name();
		if (sign == it -> first)
		{
			out << "Match!";
		}
		out << std::endl;
	}
	pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());

	if (signmap.end() != (it = signmap.find(sign)))
	{
		return it -> second;
	}
	return NULL;
}

int   sslroot::getcertsign(cert *c, certsign &sign)
{
	if ((c == NULL) || (c->certificate == NULL))
	{
		pqioutput(PQL_ALERT, pqisslrootzone,
			"sslroot::getcertsign() ERROR: NULL c || c->certificate");
		return 0;
	}

	// get the signature from the cert, and copy to the array.
	ASN1_BIT_STRING *signature = c -> certificate -> signature;
	int signlen = ASN1_STRING_length(signature);
	if (signlen < CERTSIGNLEN)
	{
		pqioutput(PQL_ALERT, pqisslrootzone,
			"sslroot::getcertsign() ERROR: short Signature");
		return 0;
	}
	// else copy in the first CERTSIGNLEN.
	unsigned char *signdata = ASN1_STRING_data(signature);
	memcpy(sign.data, signdata, CERTSIGNLEN);

	return 1;
}

int   sslroot::addtosignmap(cert *c)
{
	certsign cs;
	if (!getcertsign(c, cs))
	{
		// error.
                pqioutput(PQL_ALERT, pqisslrootzone,
			"sslroot::addsigntomap() ERROR: Fail to getcertsign()");
		return 0;
	}
	cert *c2 = findcertsign(cs);
	if (c2 == NULL)
	{
		// add, and return okay.
		signmap[cs] = c;
		return 1;
	}
	if (c2 != c)
	{
		// error.
                pqioutput(PQL_ALERT, pqisslrootzone,
			"sslroot::addsigntomap() ERROR: Duplicate Entry()");
		return 0;
	}

	// else already exists.
	return 1;
}




int   sslroot::hashFile(std::string fname, unsigned char *hash, unsigned int hlen)
{
	// open the file.
	// setup the hash.
	
	// pipe the file through.
	
	
	return 1;
}

int   sslroot::hashDigest(char *data, unsigned int dlen, 
		unsigned char *hash, unsigned int hlen)
{
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
	if (0 == EVP_DigestInit_ex(mdctx, EVP_sha1(), NULL))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_DigestInit Failure!");
		return -1;
	}
	
	if (0 == EVP_DigestUpdate(mdctx, data, dlen))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_DigestUpdate Failure!");
		return -1;
	}

	unsigned int signlen = hlen;
	if (0 == EVP_DigestFinal_ex(mdctx, hash, &signlen))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_DigestFinal Failure!");
		return -1;
	}

	EVP_MD_CTX_destroy(mdctx);
	return signlen;
}



int   sslroot::signDigest(EVP_PKEY *key, char *data, unsigned int dlen, 
			unsigned char *sign, unsigned int slen)
{
	unsigned int signlen = EVP_PKEY_size(key);

	{
	  std::ostringstream out;
	  out << "sslroot::signDigest(" << (void *) key;
	  out << ", " << (void *) data << ", " << dlen << ", ";
	  out << (void *) sign << ", " << slen << ")" << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());
	}

	if (signlen > slen)
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
			"sslroot::signDigest() Sign Length too short");
		return -1;
	}

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
	if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignInit Failure!");
		return -1;
	}
	
	if (0 == EVP_SignUpdate(mdctx, data, dlen))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignUpdate Failure!");
		return -1;
	}

	signlen = slen;
	if (0 == EVP_SignFinal(mdctx, sign, &signlen, key))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_SignFinal Failure!");
		return -1;
	}

	EVP_MD_CTX_destroy(mdctx);
	return signlen;
}


int   sslroot::verifyDigest(EVP_PKEY *key, char *data, unsigned int dlen, 
					unsigned char *enc, unsigned int elen)
{
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
	if (0 == EVP_VerifyInit_ex(mdctx, EVP_sha1(), NULL))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_VerifyInit Failure!");
		return -1;
	}
	
	if (0 == EVP_VerifyUpdate(mdctx, data, dlen))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_VerifyUpdate Failure!");
		return -1;
	}

	int vv;
	if (0 > (vv = EVP_VerifyFinal(mdctx, enc, elen, key)))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, "EVP_VerifyFinal Failure!");
		return -1;
	}
	if (vv == 1)
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, "Verified Signature OKAY");
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, "Failed Verification!");
	}

	EVP_MD_CTX_destroy(mdctx);
	return vv;
}

// Think both will fit in the one Structure.
int   sslroot::generateKeyPair(EVP_PKEY *keypair, unsigned int keylen)
{
	RSA *rsa = RSA_generate_key(2048, 65537, NULL, NULL);
	EVP_PKEY_assign_RSA(keypair, rsa);
	pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, "sslroot::generateKeyPair()");
	return 1;
}


std::string getX509NameString(X509_NAME *name)
{
	std::string namestr;
	for(int i = 0; i < X509_NAME_entry_count(name); i++)
	{
		X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, i);
		ASN1_STRING *entry_data = X509_NAME_ENTRY_get_data(entry);
		ASN1_OBJECT *entry_obj = X509_NAME_ENTRY_get_object(entry);

		namestr += "\t";
		namestr += OBJ_nid2ln(OBJ_obj2nid(entry_obj));
		namestr += " : ";

		//namestr += entry_obj -> flags;
		//namestr += entry_data -> length;
		//namestr += entry_data -> type;

		//namestr += entry_data -> flags;
		//entry -> set; 

		if (entry_data -> data != NULL)
		{
			namestr += (char *) entry_data -> data;
		}
		else
		{
			namestr += "NULL";
		}

		if (i + 1 < X509_NAME_entry_count(name))
		{
			namestr += "\n";
		}

	}
	return namestr;
}


std::string getX509CNString(X509_NAME *name)
{
	std::string namestr;
	for(int i = 0; i < X509_NAME_entry_count(name); i++)
	{
		X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, i);
		ASN1_STRING *entry_data = X509_NAME_ENTRY_get_data(entry);
		ASN1_OBJECT *entry_obj = X509_NAME_ENTRY_get_object(entry);

		if (0 == strncmp("CN", OBJ_nid2sn(OBJ_obj2nid(entry_obj)), 2))
		{
			if (entry_data -> data != NULL)
			{
				namestr += (char *) entry_data -> data;
			}
			else
			{
				namestr += "Unknown";
			}
			return namestr;
		}
	}
	return namestr;
}

	
std::string getX509TypeString(X509_NAME *name, char *type, int len)
{
	std::string namestr;
	for(int i = 0; i < X509_NAME_entry_count(name); i++)
	{
		X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, i);
		ASN1_STRING *entry_data = X509_NAME_ENTRY_get_data(entry);
		ASN1_OBJECT *entry_obj = X509_NAME_ENTRY_get_object(entry);
	
		if (0 == strncmp(type, OBJ_nid2sn(OBJ_obj2nid(entry_obj)), len))
		{
			if (entry_data -> data != NULL)
			{
				namestr += (char *) entry_data -> data;
			}
			else
			{
				namestr += "Unknown";
			}
			return namestr;
		}
	}
	return namestr;
}

std::string getX509LocString(X509_NAME *name)
{
        return getX509TypeString(name, "L", 2);
}

std::string getX509OrgString(X509_NAME *name)
{
        return getX509TypeString(name, "O", 2);
}


std::string getX509CountryString(X509_NAME *name)
{
        return getX509TypeString(name, "C", 2);
}

	
std::string convert_to_str(certsign &sign)
{
	std::ostringstream id;
	for(int i = 0; i < CERTSIGNLEN; i++)
	{
		id << std::hex << std::setw(2) << std::setfill('0') 
			<< (uint16_t) (((uint8_t *) (sign.data))[i]);
	}
	return id.str();
}
	
bool convert_to_certsign(std::string id, certsign &sign)
{
	char num[3];
	if (id.length() < CERTSIGNLEN * 2)
	{
		return false;
	}
	
	for(int i = 0; i < CERTSIGNLEN; i++)
	{
		num[0] = id[i * 2];
		num[1] = id[i * 2 + 1];
		num[2] = '\0';
		int32_t val;
		if (1 != sscanf(num, "%x", &val))
		{
			return false;
		}
		sign.data[i] = (uint8_t) val;
	}
	return true;
}
	
	
