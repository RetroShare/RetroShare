/*
 * "$Id: xpgpcert.cc,v 1.18 2007-04-15 18:45:18 rmf24 Exp $"
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




#include "xpgpcert.h"
#include "pqipacket.h"

#include "pqinetwork.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <sstream>
#include <iomanip>

#include "pqidebug.h"

const int pqisslrootzone = 1211;

/** XPGP keyring interface ************
int     XPGP_add_certificate(XPGP_KEYRING *kr, XPGP *x);
int     XPGP_auth_certficate(XPGP_KEYRING *kr, XPGP *x);
int     XPGP_sign_certificate(XPGP_KEYRING *kr, XPGP *subj, XPGP *issuer);

int     XPGP_check_valid_certificate(XPGP *x);

int     XPGP_signer_trusted(XPGP_KEYRING *kr, XPGP *trusted);
int     XPGP_signer_untrusted(XPGP_KEYRING *kr, XPGP *untrusted);
 
 int     XPGP_copy_known_signatures(XPGP_KEYRING *kr, XPGP *dest, XPGP *src);

 *
 *
 */

unsigned char convertHexToChar(unsigned char a, unsigned char b);


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
			const char *passwd)
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


	// XXX TODO
	// actions_to_seed_PRNG();

	std::cerr << "SSL Library Init!" << std::endl;

	// setup connection method
	sslctx = SSL_CTX_new(PGPv1_method());

	// setup cipher lists.
	SSL_CTX_set_cipher_list(sslctx, "DEFAULT");

	// certificates (Set Local Server Certificate).
	FILE *ownfp = fopen(cert_file, "r");
	if (ownfp == NULL)
	{
		std::cerr << "Couldn't open Own Certificate!" << std::endl;
		return -1;
	}



	// get xPGP certificate.
	XPGP *xpgp = PEM_read_XPGP(ownfp, NULL, NULL, NULL);
	fclose(ownfp);

	if (xpgp == NULL)
	{
		return -1;
	}
	SSL_CTX_use_pgp_certificate(sslctx, xpgp);

	// get private key
	FILE *pkfp = fopen(priv_key_file, "rb");
	if (pkfp == NULL)
	{
		std::cerr << "Couldn't Open PrivKey File!" << std::endl;
		closessl();
		return -1;
	}

	pkey = PEM_read_PrivateKey(pkfp, NULL, NULL, (void *) passwd);
	fclose(pkfp);

	if (pkey == NULL)
	{
		return -1;
	}
	SSL_CTX_use_pgp_PrivateKey(sslctx, pkey);

	if (1 != SSL_CTX_check_pgp_private_key(sslctx))
	{
		std::cerr << "Issues With Private Key! - Doesn't match your Cert" << std::endl;
		std::cerr << "Check your input key/certificate:" << std::endl;
		std::cerr << priv_key_file << " & " << cert_file;
		std::cerr << std::endl;
		closessl();
		return -1;
	}

	// make keyring.
	pgp_keyring = createPGPContext(xpgp, pkey);
	SSL_CTX_set_XPGP_KEYRING(sslctx, pgp_keyring);


	// Setup the certificate. (after keyring is made!).

	own_cert = makeCertificateXPGP(xpgp);
	if (own_cert == NULL)
	{
		std::cerr << "Failed to Make Own Cert!" << std::endl;
		return -1;
	}
	addCertificate(own_cert);

	// enable verification of certificates (PEER)
	SSL_CTX_set_verify(sslctx, SSL_VERIFY_PEER | 
			SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

	std::cerr << "SSL Verification Set" << std::endl;



	/* configure basics on the certificate. */
	own_cert -> Name(getX509CNString(own_cert -> certificate -> subject -> subject));

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
		std::cerr << std::endl;
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
		std::cerr << std::endl;
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
		std::cerr << "EVP_SignInit Failure!" << std::endl;
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
	}


	if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
	{
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
	}

	std::cerr << "Conf Signature is(" << signlen << "): ";
	for(i = 0; i < signlen; i++) 
	{
		fprintf(stderr, "%02x", signature[i]);
		conftxt += signature[i];
	}
	std::cerr << std::endl;

	FILE *cfd = fopen(configname.c_str(), "wb");
	int wrec;
	if (1 != (wrec = fwrite(conftxt.c_str(), conftxt.length(), 1, cfd)))
	{
		std::cerr << "Error writing: " << configname << std::endl;
		std::cerr << "Wrote: " << wrec << "/" << 1 << " Records" << std::endl;
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
		std::cerr << "Unable to Load Configuration File!" << std::endl;
		std::cerr << "File: " << configname << std::endl;
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
				std::cerr << "Error Reading Signature of: ";
				std::cerr << fname;
				std::cerr << std::endl;
				std::cerr << "ABorting Load!";
				std::cerr << std::endl;
				return -1;
			}
			unsigned char uc = (unsigned char) c;
			signature += (unsigned char) uc;
		}
		if ('\n' != (c = fgetc(cfd)))
		{
			std::cerr << "Warning Mising seperator" << std::endl;
		}

		std::cerr << "Read fname:" << fname << std::endl;
		std::cerr << "Signature:" << std::endl;
		for(i = 0; i < signlen; i++) 
		{
			fprintf(stderr, "%02x", (unsigned char) signature[i]);
		}
		std::cerr << std::endl;
		std::cerr << std::endl;

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
				std::cerr << "Error Reading Value of: ";
				std::cerr << opt;
				std::cerr << std::endl;
				std::cerr << "ABorting Load!";
				std::cerr << std::endl;
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
			std::cerr << "Warning Mising seperator" << std::endl;
		}

		std::cerr << "Read OPT:" << opt;
		std::cerr << " Val:" << val << std::endl;

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
				std::cerr << "Error Reading Conf Signature:";
				std::cerr << std::endl;
				return 1;
			}
			unsigned char uc = (unsigned char) c;
			name[i] = uc;
		}
	}

	std::cerr << "Configuration File Signature: " << std::endl;
	for(i = 0; i < signlen; i++) 
	{
		fprintf(stderr, "%02x", (unsigned char) name[i]);
	}
	std::cerr << std::endl;


	// when we get here - should have the final signature in the buffer.
	// check.
	//
	// compare signatures.
	// instead of verifying with the public key....
	// we'll sign it again - and compare .... FIX LATER...
	
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	if (0 == EVP_SignInit(mdctx, EVP_sha1()))
	{
		std::cerr << "EVP_SignInit Failure!" << std::endl;
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
	}

	if (0 == EVP_SignFinal(mdctx, conf_signature, &signlen, pkey))
	{
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
	}

	EVP_MD_CTX_destroy(mdctx);
	fclose(cfd);

	std::cerr << "Recalced File Signature: " << std::endl;
	for(i = 0; i < signlen; i++) 
	{
		fprintf(stderr, "%02x", conf_signature[i]);
	}
	std::cerr << std::endl;

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
		std::cerr << "ERROR VALIDATING CONFIGURATION!" << std::endl;
		std::cerr << "PLEASE FIX!" << std::endl;
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
				std::cerr << "Updated Certificate....but no";
				std::cerr << " need for addition";
				std::cerr << std::endl;
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


const int PQI_SSLROOT_CERT_CONFIG_SIZE = 1024;

int     sslroot::savecertificate(cert *c, const char *fname)
{
	// load certificates from file.
	FILE *setfp = fopen(fname, "wb");
	if (setfp == NULL)
	{
		std::cerr << "sslroot::savecertificate() Bad File: " << fname;
		std::cerr << " Cannot be Written!" << std::endl;
		return -1;
	}

	std::cerr << "Writing out Cert...:" << c -> Name() << std::endl;
	PEM_write_XPGP(setfp, c -> certificate);

	// writing out details....
	
	// read in a line.....
	int size = PQI_SSLROOT_CERT_CONFIG_SIZE;
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
	int *ncts = (int *) &(line[pos_ncts]);
	int *ncvl = (int *) &(line[pos_ncvl]);
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

	/* now convert it to hex */
	char config_hex[2 * size];
	for(i = 0; i < size; i++)
	{
		sprintf(&(config_hex[i * 2]), "%02x", 
			(unsigned int) ((unsigned char *) line)[i]);
	}

	if (1 != fwrite(config_hex, size * 2,1, setfp))
	{
		std::cerr << "Error Writing Peer Record!" << std::endl;
		return -1;
	}
	fclose(setfp);

	// then reopen to generate hash.
	setfp = fopen(fname, "rb");
	if (setfp == NULL)
	{
		std::cerr << "sslroot::savecertificate() Bad File: " << fname;
		std::cerr << " Opened for ReHash!" << std::endl;
		return -1;
	}

	unsigned int signlen = EVP_PKEY_size(pkey);
	unsigned char signature[signlen];

	int maxsize = 20480;
	int rbytes;
	char inall[maxsize];
	if (0 == (rbytes = fread(inall, 1, maxsize, setfp)))
	{
		std::cerr << "Error Writing Peer Record!" << std::endl;
		return -1;
	}
	std::cerr << "Read " << rbytes << std::endl;

	// so we read rbytes.
	// hash.
	//OpenSSL_add_all_digests();

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
	{
		std::cerr << "EVP_SignInit Failure!" << std::endl;
	}

	if (0 == EVP_SignUpdate(mdctx, inall, rbytes))
	{
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
	}

	if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
	{
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
	}

	std::cerr << "Saved Cert: " << c -> Name() << " Status: ";
	std::cerr << std::hex << (unsigned int) c->Status() << std::dec << std::endl;

	std::cerr << "Cert + Setting Signature is(" << signlen << "): ";
	std::string signstr;
	for(i = 0; i < (signed) signlen; i++) 
	{
		fprintf(stderr, "%02x", signature[i]);
		signstr += signature[i];
	}
	std::cerr << std::endl;

	c -> Hash(signstr);
	std::cerr << "Stored Hash Length: " << (c -> Hash()).length() << std::endl;
	std::cerr << "Real Hash Length: " << signlen << std::endl;

	fclose(setfp);

	EVP_MD_CTX_destroy(mdctx);

	return 1;
}

cert *sslroot::loadcertificate(const char *fname, std::string hash)
{
	// if there is a hash - check that the file matches it before loading.
	FILE *pcertfp;

	/* We only check a signature's hash if
	 * we are loading from a configuration file.
	 * Therefore we saved the file and it should be identical. 
	 * and a direct load + verify will work.
	 *
	 * If however it has been transported by email....
	 * Then we might have to correct the data (strip out crap)
	 * from the configuration at the end. (XPGP load should work!)
	 */

	if (hash.length() > 1)
	{
		pcertfp = fopen(fname, "rb");
		// load certificates from file.
		if (pcertfp == NULL)
		{
			std::cerr << "sslroot::loadcertificate() Bad File: " << fname;
			std::cerr << " Cannot be Hashed!" << std::endl;
			return NULL;
		}

		unsigned int signlen = EVP_PKEY_size(pkey);
		unsigned char signature[signlen];

		int maxsize = 20480; /* should be enough for about 50 signatures */
		int rbytes;
		char inall[maxsize];
		if (0 == (rbytes = fread(inall, 1, maxsize, pcertfp)))
		{
			std::cerr << "Error Reading Peer Record!" << std::endl;
			return NULL;
		}
		//std::cerr << "Read " << rbytes << std::endl;


		EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
		if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
		{
			std::cerr << "EVP_SignInit Failure!" << std::endl;
		}
	
		if (0 == EVP_SignUpdate(mdctx, inall, rbytes))
		{
			std::cerr << "EVP_SignUpdate Failure!" << std::endl;
		}
	
		if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
		{
			std::cerr << "EVP_SignFinal Failure!" << std::endl;
		}

		fclose(pcertfp);
		EVP_MD_CTX_destroy(mdctx);
	
		bool same = true;
		if (signlen != hash.length())
		{
				std::cerr << "Different Length Signatures... ";
				std::cerr << "Cannot Load Certificate!" << std::endl;
				return NULL;
		}

		for(int i = 0; i < (signed) signlen; i++) 
		{
			if (signature[i] != (unsigned char) hash[i])
			{
				same = false;
				std::cerr << "Invalid Signature... ";
				std::cerr << "Cannot Load Certificate!" << std::endl;
				return NULL;
			}
		}
		std::cerr << "Verified Signature for: " << fname;
		std::cerr << std::endl;
	
	
	}
	else
	{
		std::cerr << "Not checking cert signature" << std::endl;
	}

	pcertfp = fopen(fname, "rb");

	// load certificates from file.
	if (pcertfp == NULL)
	{
		std::cerr << "sslroot::loadcertificate() Bad File: " << fname;
		std::cerr << " Cannot be Read!" << std::endl;
		return NULL;
	}


	XPGP *pc;
	cert *npc = NULL;

	if ((pc = PEM_read_XPGP(pcertfp, NULL, NULL, NULL)) != NULL)
	{
		// read a certificate.
		std::cerr << "Loaded Certificate: ";
		std::cerr << pc -> name << std::endl;

		npc = makeCertificateXPGP(pc);
		if (npc == NULL)
		{
			std::cerr << "Failed to Create Cert!" << std::endl;
			return NULL;
		}
	}
	else // (pc == NULL)
	{
		unsigned long err = ERR_get_error();
		std::cerr << "Read Failed .... CODE(" << err << ")" << std::endl;
		std::cerr << ERR_error_string(err, NULL) << std::endl;
		return NULL;
	}

	// Now we try to read in 1024 bytes.....
	// if successful, then have settings!

	// read in a line.....
	int size = PQI_SSLROOT_CERT_CONFIG_SIZE;
	char config_hex[PQI_SSLROOT_CERT_CONFIG_SIZE * 4]; /* double for extra space */
	char line[PQI_SSLROOT_CERT_CONFIG_SIZE];

	/* load as much as possible into the config_hex.
	 */

	int rbytes = fread(config_hex, 1, size * 4, pcertfp);
	bool configLoaded = false;
	int i, j;

	if (rbytes < size * 2)
	{
		if ((hash.size() > 1) && (rbytes >= size))
		{
			/* old format certificate (already verified) */
			std::cerr << "Loading Old Style Cert Config" << std::endl;
			memcpy(line, config_hex, size);
			configLoaded = true;
		}
		else
		{
			std::cerr << "Error Reading Setting: Only Cert Retrieved" << std::endl;
			return npc;
		}
	}

	/* if there was no hash then we need to check it */
	if (hash.size() <= 1)
	{
		std::cerr << "Checking Cert Configuration for spam char" << std::endl;
		for(i = 0, j = 0; i < rbytes; i++)
		{
			if (isxdigit(config_hex[i]))
			{
				config_hex[j++] = config_hex[i];
			}
			else
			{
				std::cerr << "Stripped out:" << config_hex[i] << " or "
					<< (int) config_hex[i] << "@" << i
					<< " j:" << j << std::endl;
			}
				
		}
		if (j < size * 2)
		{
			std::cerr << "Error Cert Config wrong size" << std::endl;
			return npc;
		}
		std::cerr << "Stripped out " << i - j << " spam chars" << std::endl;
	}

	/* now convert the hex into binary */
	if (!configLoaded)
	{
	  for(i = 0; i < size; i++)
	  {
		((unsigned char *) line)[i] = convertHexToChar(
			config_hex[2 * i], config_hex[2 * i + 1]);
	  }
	  configLoaded = true;
	}

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
	int *ncts = (int *) &(line[pos_ncts]);
	int *ncvl = (int *) &(line[pos_ncvl]);
	char *name = &(line[pos_name]);
	//char *end = &(line[pos_end]);

	// end of data structures....



	// fill in the data.
	cert *c = npc;
	c -> Status(*status);

	std::cerr << "Loaded Cert: " << c -> Name() << " Prev Status: ";
	std::cerr << std::hex << (unsigned int) c->Status() << std::dec << std::endl;

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

        // for sending stuff as text
	// cert *      loadCertFromString(std::string pem);
	// std::string saveCertAsString(cert *c);
	//

std::string   sslroot::saveCertAsString(cert *c)
{
	// save certificate to a string, 
	// must use a BIO.
	std::string certstr;
	BIO *bp = BIO_new(BIO_s_mem());

	std::cerr << "saveCertAsString:" << c -> Name() << std::endl;
	PEM_write_bio_XPGP(bp, c -> certificate);

	/* translate the bp data to a string */
	char *data;
	int len = BIO_get_mem_data(bp, &data);
	for(int i = 0; i < len; i++)
	{
		certstr += data[i];
	}

	BIO_free(bp);

	return certstr;
}

cert *sslroot::loadCertFromString(std::string pem)
{
	/* Put the data into a mem BIO */
	char *certstr = strdup(pem.c_str());

	BIO *bp = BIO_new_mem_buf(certstr, -1);

	XPGP *pc;
	cert *npc = NULL;

	pc = PEM_read_bio_XPGP(bp, NULL, NULL, NULL);

	BIO_free(bp);
	free(certstr);

	if (pc != NULL)
	{
		// read a certificate.
		std::cerr << "loadCertFromString: ";
		std::cerr << pc -> name << std::endl;

		npc = makeCertificateXPGP(pc);
		if (npc == NULL)
		{
			std::cerr << "Failed to Create Cert!" << std::endl;
			return NULL;
		}
	}
	else // (pc == NULL)
	{
		unsigned long err = ERR_get_error();
		std::cerr << "Read Failed .... CODE(" << err << ")" << std::endl;
		std::cerr << ERR_error_string(err, NULL) << std::endl;
		return NULL;
	}

	// small hack - as the timestamps seem wrong.....
	// could be a saving thing - or a bug....
	npc -> lc_timestamp = 0;
	npc -> lr_timestamp = 0;

	// reset these. as well.
	npc -> nc_timestamp = 0;
	npc -> nc_timeintvl = 5;

	return npc;
}


unsigned char convertHexToChar(unsigned char a, unsigned char b)
{
	int num1 = 0;
	int num2 = 0;
	if (('0' <= a) && ('9' >= a))
	{
		num1 = a - '0';
	}
	else if (('a' <= a) && ('f' >= a))
	{
		num1 = 10 + a - 'a';
	}
	else if (('A' <= a) && ('F' >= a))
	{
		num1 = 10 + a - 'A';
	}

	if (('0' <= b) && ('9' >= b))
	{
		num2 = b - '0';
	}
	else if (('a' <= b) && ('f' >= b))
	{
		num2 = 10 + b - 'a';
	}
	else if (('A' <= b) && ('F' >= b))
	{
		num2 = 10 + b - 'A';
	}

	num1 *= 16;
	num1 += num2;

	return (unsigned char) num1;
}

int     sslroot::printCertificate(cert *c, std::ostream &out)
{
	out << "Cert Name:" << (c -> certificate) -> name << std::endl;
	//X509_print_fp(stderr, c -> certificate);
	return 1;
}

// This function will clean up X509 *c if necessary.
// This fn will also collate the signatures....
// that are received via p3disc. 
// (connections -> registerCertificate, which does similar sign merging)
//
cert    *sslroot::makeCertificateXPGP(XPGP *c)
{
	if (c == NULL)
	{
		return NULL;
	}

	// At this point we check to see if there is a duplicate.
	cert    *dup = checkDuplicateXPGP(c);
	cert *npc = NULL;
	if (dup == NULL)
	{
		npc = new cert();
		npc -> certificate = c;
		if (!addtosignmap(npc)) // only allow the cert if no dup
		{
			std::cerr << "sslroot::makeCertificate()";
			std::cerr << "Failed to Get Signature - Not Allowed!";
			std::cerr << std::endl;

			// failed!... cannot add it!.
			delete npc;
			return NULL;
		}

		/* setup the defaults */
		npc -> Status(PERSON_STATUS_MANUAL);
		npc -> trustLvl = -1;
		// set Tag to be their X509CN.
		npc -> Name(getX509CNString(npc->certificate->
						subject->subject));

		allcerts.push_back(npc);
		std::cerr << "sslroot::makeCertificate() For " << c -> name;
		std::cerr << " A-Okay!" << std::endl;
		// at this point we need to add to the signaturelist....
	
	}
	else if (c == dup -> certificate)
	{
		// identical - so okay.
		npc = dup;
		std::cerr << "sslroot::makeCertificate() For " << c -> name;
		std::cerr << " Found Identical - A-Okay!" << std::endl;
	}
	else
	{
		std::cerr << "sslroot::makeCertificate() For " << c -> name;
		std::cerr << " Cleaning up other XPGP!" << std::endl;
		std::cerr << " Also moving new signatures ... " << std::endl;
		// clean up c.
		XPGP_copy_known_signatures(pgp_keyring, dup -> certificate, c);
		XPGP_free(c);
		npc = dup;
	}
	return npc;
}


cert    *sslroot::checkDuplicateXPGP(XPGP *x)
{
	if (x == NULL)
		return NULL;

	// loop through and print - then check.
	std::list<cert *>::iterator it;
	for(it = allcerts.begin(); it != allcerts.end(); it++)
	{
		if (0 == XPGP_cmp((*it) -> certificate, x))
		{
			return (*it);
		}
	}
	return NULL;
}


cert    *sslroot::checkPeerXPGP(XPGP *x)
{
	if (x == NULL)
		return NULL;

	// loop through and print - then check.
	std::list<cert *>::iterator it;
	for(it = peercerts.begin(); it != peercerts.end(); it++)
	{
		if (0 == XPGP_cmp((*it) -> certificate, x))
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
	std::cerr << "sslroot::findpeercert() Failed!" << std::endl;
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
	
	int val = XPGP_cmp(a -> certificate, b -> certificate);

	std::cerr << "Certificate Comparison Returned: " << val << std::endl;
	
	return val;
}

cert *	sslroot::registerCertificateXPGP(XPGP *nc, struct sockaddr_in raddr, bool in)
{
	if (nc == NULL)
		return NULL;

	// shoud check all certs.
	cert *c = checkDuplicateXPGP(nc);
	if (c != NULL)
	{
		if (c -> certificate == nc)
		{
			std::cerr << "sslroot::registerCertificate()";
			std::cerr << " Found Identical XPGP cert";
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "sslroot::registerCertificate()";
			std::cerr << " Found Same XPGP cert/diff mem - Clean";
			std::cerr << std::endl;
			std::cerr << "sslroot::registerCertificate()";
			std::cerr << " Copying New Signatures before deleting";
			std::cerr << std::endl;
			/* copy across the signatures -> if necessary */
			XPGP_copy_known_signatures(pgp_keyring, c->certificate, nc);
			XPGP_free(nc);
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
				  std::cerr << "Guessing Default Server Addr!";
				  std::cerr << std::endl;
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
			std::cerr << "WARNING: attempt to reg CONNECTED Cert!";
			std::cerr << std::endl;
		}
		return c;
	}
	
	std::cerr << "sslroot::registerCertificate() Certificate Not Found!" << std::endl;
	std::cerr << "Saving :" << nc -> name << std::endl;
	std::cerr << std::endl;
	
	cert *npc = makeCertificateXPGP(nc);
	if (npc == NULL)
	{
		std::cerr << "Failed to Make Certificate";
		std::cerr << std::endl;
		return NULL; 
	}

	npc -> Name(nc -> name);

	npc -> lastaddr = raddr;
	if (in == true)
	{
		npc -> lr_timestamp = time(NULL);
		// likely to be server address (with default port)
		std::cerr << "Guessing Default Server Addr!";
		std::cerr << std::endl;
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
		std::cerr << "sslroot::removeCertificate() Failed" << std::endl;
		std::cerr << "\t a cert is in use." << std::endl;
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

			std::cerr << "sslroot::removeCertificate() ";
			std::cerr << "Success!" << std::endl;
			std::cerr << "\tMoved to Collected Certs" << std::endl;
	
			/* remove from the keyring */
			XPGP_remove_certificate(pgp_keyring, c->certificate);

			collectedcerts.push_back(c);

			certsChanged.IndicateChanged();
			certsMajorChanged.IndicateChanged();
			return 1;
		}
	}
	std::cerr << "sslroot::removeCertificate() ";
	std::cerr << "Failed to Match Cert!" << std::endl;

	return 0;
}


int	sslroot::addCertificate(cert *c)
{
	std::cerr << "sslroot::addCertificate()" << std::endl;
	c -> InUse(false);
	// let most flags through.
	//c -> Accepted(false);
	//c -> WillConnect(false);
	if (c -> certificate == NULL)
	{
		std::cerr << "sslroot::addCertificate() certificate==NULL" << std::endl;
		std::cerr << "\tNot Adding Certificate!" << std::endl;
		return 0;
	}

	cert *dup = checkPeerXPGP(c -> certificate);
	if (dup != NULL)
	{
		std::cerr << "sslroot::addCertificate() Not Adding";
		std::cerr << "Certificate with duplicate...." << std::endl;
		std::cerr << "\t\tTry RegisterCertificate() " << std::endl;

		return -1;
	}

	// else put in in the list.
	peercerts.push_back(c);

	/* add to keyring */
	XPGP_add_certificate(pgp_keyring, c->certificate);

	/* if this should be a trusted cert... setup */
	if (c-> Trusted())
	{
		if (XPGP_signer_trusted(pgp_keyring, c -> certificate))
		{
			c -> Trusted(true);
		}
		else
		{
			c -> Trusted(false);
		}
	}
		
	c -> trustLvl = XPGP_auth_certificate(pgp_keyring, c->certificate);

	certsChanged.IndicateChanged();
	certsMajorChanged.IndicateChanged();

	return 1;
}


int	sslroot::addUntrustedCertificate(cert *c)
{
	// blank it all.
	c -> Status(PERSON_STATUS_MANUAL);
	// set Tag to be their X509CN.
	c -> Name(getX509CNString(c -> certificate -> subject -> subject));

	return addCertificate(c);
}


int	sslroot::addCollectedCertificate(cert *c)
{
	// blank it all.
	c -> Status(PERSON_STATUS_MANUAL);
	// set Tag to be their X509CN.
	c -> Name(getX509CNString(c -> certificate -> subject -> subject));

	// put in the collected certs ...
	collectedcerts.push_back(c);
	return 1;
}



int	sslroot::validateCertificateXPGP(cert *c)
{
	std::cerr << "sslroot::validateCertificate() Why Not!" << std::endl;
	if (XPGP_check_valid_certificate(c->certificate))
	{
		c -> Valid(true);
	}
	else
	{
		c -> Valid(false);
	}
	std::cerr << "Cert Status: " << c -> Status() << std::endl;
	return 1;
}


/* this redoes the trust calculations */
int	sslroot::checkAuthCertificate(cert *xpgp)
{
	std::cerr << "sslroot::checkAuthCertificate()" << std::endl;
	if ((xpgp == NULL) || (xpgp -> certificate == NULL))
	{
		return -1;
	}

	/* reevaluate the auth of the xpgp */
	xpgp -> trustLvl = XPGP_auth_certificate(pgp_keyring, xpgp->certificate);

	/* this also merges the signature into the keyring */
	certsChanged.IndicateChanged();
	certsMajorChanged.IndicateChanged();
	return 1;
}


int	sslroot::signCertificate(cert *xpgp)
{
	std::cerr << "sslroot::signCertificate()" << std::endl;
	cert *own = getOwnCert();

	pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
		"sslroot::signCertificate()");

	/* check that cert is suitable */
	/* sign it */
	XPGP_sign_certificate(pgp_keyring, xpgp -> certificate, own -> certificate);

	/* reevaluate the auth of the xpgp */
	xpgp -> trustLvl = XPGP_auth_certificate(pgp_keyring, xpgp->certificate);

	/* this also merges the signature into the keyring */
	certsChanged.IndicateChanged();
	certsMajorChanged.IndicateChanged();
	return 1;
}

int	sslroot::trustCertificate(cert *c, bool totrust)
{
	std::cerr << "sslroot::trustCertificate()" << std::endl;
	/* check auth status of certificate */
	pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
		"sslroot::trustCertificate()");

	/* if trusted -> untrust */
	if (!totrust)
	{
		XPGP_signer_untrusted(pgp_keyring, c -> certificate);
		c -> Trusted(false);
	}
	else
	{
		/* if auth then we can trust them */
		if (XPGP_signer_trusted(pgp_keyring, c -> certificate))
		{
			c -> Trusted(true);
		}
	}

	/* reevaluate the auth of the xpgp */
	c -> trustLvl = XPGP_auth_certificate(pgp_keyring, c->certificate);

	certsChanged.IndicateChanged();
	certsMajorChanged.IndicateChanged();
	
	return 1;
}

int 	sslroot::superNodeMode()
{
#
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // UNIX only.

	XPGP_supernode(pgp_keyring);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
	return 1;
}

/***** REMOVED!!!
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
 *
 */


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
		std::cerr << "sslroot::getSetting(" << opt << ") = ";
		std::cerr << it -> second << std::endl;
		return it -> second;
	}
	// else return empty string.
	std::cerr << "sslroot::getSetting(" << opt;
	std::cerr << ") Not There!" << std::endl;

	std::string empty("");
	return empty;
}

void sslroot::setSetting(std::string opt, std::string val)
{
	// check settings..
	std::cerr << "sslroot::saveSetting(" << opt << ", ";
	std::cerr << val << ")" << std::endl;

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
		std::ostringstream out;
        	certsign cs;
		getSSLRoot() -> getcertsign(this, cs);

        	out << std::hex;
		for(int i = 0; i < CERTSIGNLEN; i++)
		{

			unsigned char n = cs.data[i];
                	out << std::hex << std::setw(2) << std::setfill('0')
		                    << std::setprecision(2) << (unsigned int) n;
		}
		return out.str();
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
	if (0 > memcmp(data, ref.data, CERTSIGNLEN))
		return true;
	return false;
}


bool certsign::operator==(const certsign &ref) const
{
	//compare the signature.
	return (0 == memcmp(data, ref.data, CERTSIGNLEN));
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
	pqioutput(PQL_WARNING, pqisslrootzone, 
		"sslroot::findcertsign() ERROR: No Matching Entry");
	return NULL;
}

int   sslroot::getcertsign(cert *c, certsign &sign)
{
	// bug ... segv a couple of times here!
	if ((c == NULL) || (c->certificate == NULL))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"sslroot::getcertsign() ERROR: NULL c || c->certificate");
		return 0;
	}
		
	// a Bit of a hack here.....
	// get the first signature....
	if (sk_XPGP_SIGNATURE_num(c->certificate->signs) < 1)
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"sslroot::getcertsign() ERROR: No Signatures");
		return 0;
	}
	XPGP_SIGNATURE *xpgpsign = sk_XPGP_SIGNATURE_value(c->certificate->signs, 0);

	// get the signature from the cert, and copy to the array.
	ASN1_BIT_STRING *signature = xpgpsign->signature;
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
		std::cerr << "EVP_DigestInit Failure!" << std::endl;
		return -1;
	}
	
	if (0 == EVP_DigestUpdate(mdctx, data, dlen))
	{
		std::cerr << "EVP_DigestUpdate Failure!" << std::endl;
		return -1;
	}

	unsigned int signlen = hlen;
	if (0 == EVP_DigestFinal_ex(mdctx, hash, &signlen))
	{
		std::cerr << "EVP_DigestFinal Failure!" << std::endl;
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
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"sslroot::signDigest() Sign Length too short");
		return -1;
	}

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
	if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"sslroot::signDigest() EVP_SignInit Failure!");
		return -1;
	}
	
	if (0 == EVP_SignUpdate(mdctx, data, dlen))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"sslroot::signDigest() EVP_SignUpdate Failure!");
		return -1;
	}

	signlen = slen;
	if (0 == EVP_SignFinal(mdctx, sign, &signlen, key))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"sslroot::signDigest() EVP_SignFinal Failure!");
		return -1;
	}

	{
		// display signed data
		std::ostringstream out;
		out << "sslroot::signDigest() Data Display" << std::endl;
		out << "Data To Sign (" << dlen << "):::::::::::::" << std::hex;
		for(unsigned int i = 0; i < dlen; i++)
		{
			if (i % 16 == 0)
			{
				out << std::endl;
				out << std::setw(4) << i << " : ";
			}
			out << std::setw(2) << (unsigned int) ((unsigned char *) data)[i] << " ";
		}
		out << std::endl;
		out << "Signature  (" << std::dec << slen << "):::::::::::::" << std::hex;
		for(unsigned int i = 0; i < slen; i++)
		{
			if (i % 16 == 0)
			{
				out << std::endl;
				out << std::setw(4) << i << " : ";
			}
			out << std::setw(2) << (unsigned int) ((unsigned char *) sign)[i] << " ";
		}
		out << std::endl;
		out << "::::::::::::::::::::::::::::::::::::::::::::::" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, out.str());

	}


	EVP_MD_CTX_destroy(mdctx);
	return signlen;
}


int   sslroot::verifyDigest(EVP_PKEY *key, char *data, unsigned int dlen, 
					unsigned char *sign, unsigned int slen)
{
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
	if (0 == EVP_VerifyInit_ex(mdctx, EVP_sha1(), NULL))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"sslroot::verifyDigest() EVP_VerifyInit Failure!");
		return -1;
	}
	
	if (0 == EVP_VerifyUpdate(mdctx, data, dlen))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"sslroot::verifyDigest() EVP_VerifyUpdate Failure!");
		return -1;
	}

	int vv;
	if (0 > (vv = EVP_VerifyFinal(mdctx, sign, slen, key)))
	{
		pqioutput(PQL_ALERT, pqisslrootzone, 
			"sslroot::verifyDigest() EVP_VerifyFinale Failure!");
		return -1;
	}
	if (vv == 1)
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslrootzone, 
			"sslroot::verifyDigest() Verified Signature OKAY");
	}
	else
	{
		std::ostringstream out;
		out << "sslroot::verifyDigest() Failed Verification!" << std::endl;
		out << "Data To Verify (" << dlen << "):::::::::::::" << std::hex;
		for(unsigned int i = 0; i < dlen; i++)
		{
			if (i % 16 == 0)
			{
				out << std::endl;
				out << std::setw(4) << i << " : ";
			}
			out << std::setw(2) << (unsigned int) ((unsigned char *) data)[i] << " ";
		}
		out << std::endl;
		out << "Signature  (" << std::dec << slen << "):::::::::::::" << std::hex;
		for(unsigned int i = 0; i < slen; i++)
		{
			if (i % 16 == 0)
			{
				out << std::endl;
				out << std::setw(4) << i << " : ";
			}
			out << std::setw(2) << (unsigned int) ((unsigned char *) sign)[i] << " ";
		}
		out << std::endl;
		out << "::::::::::::::::::::::::::::::::::::::::::::::" << std::endl;
		out << "sslroot::verifyDigest() Should Clear SSL Error!";

		pqioutput(PQL_ALERT, pqisslrootzone, out.str());
	}

	EVP_MD_CTX_destroy(mdctx);
	return vv;
}

// Think both will fit in the one Structure.
int   sslroot::generateKeyPair(EVP_PKEY *keypair, unsigned int keylen)
{
	RSA *rsa = RSA_generate_key(2048, 65537, NULL, NULL);
	EVP_PKEY_assign_RSA(keypair, rsa);
	std::cerr << "sslroot::generateKeyPair()" << std::endl;
	return 1;
}

// Extra features for XPGP..... (for login window)



// This fn installs and signs a trusted peer.
// It is limited to only working just after certificate creation.
// this is done by checking the timestamps.
//
// It should be called before the pqi handler is initiated, 
// otherwise the connection will not be automatically started.

int     sslroot::loadInitialTrustedPeer(std::string tp_file)
{
	/* we will only do this if various conditions are met.
	 * (1) check validity of certificate
	 * (2) check that we don't have any other certificates loaded.
	 * (3) check that our certificate has just been created (timestamp) and only has one signature.
	 */

	bool canLoad = true;

	std::string userName;

	/* check (1) valid cert */
	if (!LoadCheckXPGPandGetName(tp_file.c_str(),userName))
	{
		std::cerr << "sslroot::loadInitialTrustedPeer() Failed TrustedPeer Checks!(1)";
		std::cerr << std::endl;
		canLoad = false;
		return 0;
	}

	/* check (2) no other certificates loaded */
	if (peercerts.size() != 1)
	{
		/* too many certs loaded! */
		std::cerr << "sslroot::loadInitialTrustedPeer() Failed TrustedPeer Checks!(2a)";
		std::cerr << std::endl;
		canLoad = false;
		return 0;
	}

	/* that one must be our own */
	cert *ourcert = getOwnCert();
	if ((!ourcert) || (ourcert != *(peercerts.begin())) || (!ourcert->certificate))
	{
		/* too many certs loaded! */
		std::cerr << "sslroot::loadInitialTrustedPeer() Failed TrustedPeer Checks!(2b)";
		std::cerr << std::endl;
		canLoad = false;
		return 0;
	}

	XPGP *xpgp = ourcert->certificate;
	if (sk_XPGP_SIGNATURE_num(xpgp->signs) != 1)
	{
		/* too many certs loaded! */
		std::cerr << "sslroot::loadInitialTrustedPeer() Failed TrustedPeer Checks!(3a)";
		std::cerr << std::endl;
		canLoad = false;
		return 0;
	}

	/* check own certificate timestamps */
	time_t cts = time(NULL);
	X509_VAL *certv = xpgp->key->validity;
	XPGP_SIGNATURE *ownsign = sk_XPGP_SIGNATURE_value(xpgp->signs, 0);
	ASN1_TIME     *signts = ownsign->timestamp;
	ASN1_TIME     *createts = certv->notBefore;

	/* compare timestamps 
	 * Certificate timestamp should have been generated 
	 * within the last 5 seconds, */
	time_t max_initts = cts - 5;
	if ((0 > X509_cmp_time(createts, &max_initts)) || (0 > X509_cmp_time(signts, &max_initts)))
	{
		std::cerr << "sslroot::loadInitialTrustedPeer() Failed TrustedPeer Checks!(3b)";
		std::cerr << std::endl;
		canLoad = false;
		return 0;
	}

	/* or if in the future! */
	if ((0 < X509_cmp_current_time(createts)) || (0 < X509_cmp_current_time(signts)))
	{
		std::cerr << "sslroot::loadInitialTrustedPeer() Failed TrustedPeer Checks!(3c)";
		std::cerr << std::endl;
		canLoad = false;
		return 0;
	}

	/* if we get here - it has passed the tests, and we can sign it, and install it */
	cert *trusted_cert = loadcertificate(tp_file.c_str(), ""); /* no Hash! */
	if (!trusted_cert)
	{
		std::cerr << "sslroot::loadInitialTrustedPeer() Failed TrustedPeer Checks!(4a)";
		std::cerr << std::endl;
		canLoad = false;
		return 0;
	}

	/* now add it */
	if (1 != addCertificate(trusted_cert))
	{
		std::cerr << "sslroot::loadInitialTrustedPeer() Failed TrustedPeer Checks!(4b)";
		std::cerr << std::endl;
		canLoad = false;
		return 0;
	}

	/* must set these flags completely - as they aren't changed */
	trusted_cert->Accepted(true);
	trusted_cert->Manual(false);
	trusted_cert->WillConnect(true);
	trusted_cert->WillListen(true);
	/* use existing firewall/forwarded flags */

	/* sign it! (must be after add) */
	if (!signCertificate(trusted_cert))
	{
		std::cerr << "sslroot::loadInitialTrustedPeer() Failed TrustedPeer Checks!(4c)";
		std::cerr << std::endl;
		canLoad = false;
		return 0;
	}

	if (canLoad)
	{
		std::cerr << "sslroot::loadInitialTrustedPeer() Loaded: " << userName;
		std::cerr << std::endl;
		return 1;
	}
	return 0;
}





// Not dependent on sslroot. load, and detroys the XPGP memory.

int	LoadCheckXPGPandGetName(const char *cert_file, std::string &userName)
{
	/* This function loads the XPGP certificate from the file, 
	 * and checks the certificate 
	 */

	FILE *tmpfp = fopen(cert_file, "r");
	if (tmpfp == NULL)
	{
		std::cerr << "sslroot::LoadCheckAndGetXPGPName()";
		std::cerr << " Failed to open Certificate File:" << cert_file;
		std::cerr << std::endl;
		return 0;
	}

	// get xPGP certificate.
	XPGP *xpgp = PEM_read_XPGP(tmpfp, NULL, NULL, NULL);
	fclose(tmpfp);

	// check the certificate.
	bool valid = false;
	if (xpgp)
	{
		valid = XPGP_check_valid_certificate(xpgp);
	}

	if (valid)
	{
		// extract the name.
		userName = getX509CNString(xpgp->subject->subject);
	}

	// clean up.
	XPGP_free(xpgp);

	if (valid)
	{
		// happy!
		return 1;
	}
	else
	{
		// something went wrong!
		return 0;
	}
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
		id << std::hex << std::setw(2) << std::setfill('0') << (uint16_t) (((uint8_t *) (sign.data))[i]);
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


