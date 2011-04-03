/*
 * libretroshare/src/pqi: sslfns.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

/* Functions in this file are SSL only, 
 * and have no dependence on SSLRoot() etc.
 * might need SSL_Init() to be called - thats it!
 */

/******************** notify of new Cert **************************/

#include "pqi/sslfns.h"
#include "pqi/pqi_base.h"
#include "util/rsdir.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string.h>

/****
 * #define AUTHSSL_DEBUG 1
 ***/

/********************************************************************************/

#if defined(SSLFNS_ADD_CIPHER_CTX_RAND_KEY)

int EVP_CIPHER_CTX_rand_key(EVP_CIPHER_CTX *ctx, unsigned char *key)
{
	//if (ctx->cipher->flags & EVP_CIPH_RAND_KEY)
	//	return EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_RAND_KEY, 0, key);
	if (RAND_bytes(key, ctx->key_len) <= 0)
		return 0;
	return 1;
}

#endif

/********************************************************************************/
/********************************************************************************/

X509_REQ *GenerateX509Req(
		std::string pkey_file, std::string passwd,
		std::string name, std::string email, std::string org, 
		std::string loc, std::string state, std::string country, 
		int nbits_in, std::string &errString)
{
	/* generate request */
	X509_REQ *req=X509_REQ_new();

        // setup output.
        BIO *bio_out = NULL;
        bio_out = BIO_new(BIO_s_file());
        BIO_set_fp(bio_out,stdout,BIO_NOCLOSE);

        EVP_PKEY *pkey = NULL;

        // first generate a key....
        if ((pkey=EVP_PKEY_new()) == NULL)
        {
                fprintf(stderr,"GenerateX509Req: Couldn't Create Key\n");
		errString = "Couldn't Create Key";
                return 0;
        }

        int nbits = 2048;
        unsigned long e = 0x10001;

        if ((nbits_in >= 512) && (nbits_in <= 4096))
        {
                nbits = nbits_in;
        }
        else
        {
                fprintf(stderr,"GenerateX509Req: strange num of nbits: %d\n", nbits_in);
                fprintf(stderr,"GenerateX509Req: reverting to %d\n", nbits);
        }


        RSA *rsa = RSA_generate_key(nbits, e, NULL, NULL);
        if ((rsa == NULL) || !EVP_PKEY_assign_RSA(pkey, rsa))
        {
                if(rsa) RSA_free(rsa);
                fprintf(stderr,"GenerateX509Req: Couldn't Generate RSA Key!\n");
		errString = "Couldn't generate RSA Key";
                return 0;
        }


        // open the file.
        FILE *out;
        if (NULL == (out = RsDirUtil::rs_fopen(pkey_file.c_str(), "w")))
        {
                fprintf(stderr,"GenerateX509Req: Couldn't Create Key File!");
                fprintf(stderr," : %s\n", pkey_file.c_str());

		errString = "Couldn't Create Key File";
                return 0;
        }

        const EVP_CIPHER *cipher = EVP_des_ede3_cbc();

        if (!PEM_write_PrivateKey(out,pkey,cipher,
                        NULL,0,NULL,(void *) passwd.c_str()))
        {
                fprintf(stderr,"GenerateX509Req() Couldn't Save Private Key");
                fprintf(stderr," : %s\n", pkey_file.c_str());

		errString = "Couldn't Save Private Key File";
                return 0;
        }
        fclose(out);

        // We have now created a private key....
        fprintf(stderr,"GenerateX509Req() Saved Private Key");
        fprintf(stderr," : %s\n", pkey_file.c_str());

        /********** Test Loading the private Key.... ************/
        FILE *tst_in = NULL;
        EVP_PKEY *tst_pkey = NULL;
        if (NULL == (tst_in = RsDirUtil::rs_fopen(pkey_file.c_str(), "rb")))
        {
                fprintf(stderr,"GenerateX509Req() Couldn't Open Private Key");
                fprintf(stderr," : %s\n", pkey_file.c_str());

		errString = "Couldn't Open Private Key";
                return 0;
        }

        if (NULL == (tst_pkey =
                PEM_read_PrivateKey(tst_in,NULL,NULL,(void *) passwd.c_str())))
        {
                fprintf(stderr,"GenerateX509Req() Couldn't Read Private Key");
                fprintf(stderr," : %s\n", pkey_file.c_str());

		errString = "Couldn't Read Private Key";
                return 0;
        }
        fclose(tst_in);
        EVP_PKEY_free(tst_pkey);
        /********** Test Loading the private Key.... ************/

	/* Fill in details: fields. 
	req->req_info;
	req->req_info->enc;
	req->req_info->version;
	req->req_info->subject;
	req->req_info->pubkey;
	 ****************************/

	long version = 0x00;
        unsigned long chtype = MBSTRING_ASC;
	X509_NAME *x509_name = X509_NAME_new();

        // fill in the request.

        /**** X509_REQ -> Version ********************************/
        if (!X509_REQ_set_version(req,version)) /* version 1 */
        {
                fprintf(stderr,"GenerateX509Req(): Couldn't Set Version!\n");

		errString = "Couldn't Set Version";
                return 0;
        }
        /**** X509_REQ -> Version ********************************/
	/**** X509_REQ -> Key     ********************************/

	if (!X509_REQ_set_pubkey(req,pkey)) 
	{
                fprintf(stderr,"GenerateX509Req() Couldn't Set PUBKEY !\n");

		errString = "Couldn't Set PubKey";
		return 0;
	}

	/**** SUBJECT         ********************************/
        // create the name.

        // fields to add.
        // commonName CN
        // emailAddress (none)
        // organizationName O
        // localityName L
        // stateOrProvinceName ST
        // countryName C

        if (0 < strlen(name.c_str()))
        {
                X509_NAME_add_entry_by_txt(x509_name, "CN", chtype,
                        (unsigned char *) name.c_str(), -1, -1, 0);
        }
        else
        {
                fprintf(stderr,"GenerateX509Req(): No Name -> Not creating X509 Cert Req\n");
		errString = "No Name, Aborting";
                return 0;
        }

	if (0 < strlen(email.c_str()))
	{
		//X509_NAME_add_entry_by_txt(x509_name, "Email", 0, 
		//  (unsigned char *) ui -> gen_email -> value(), -1, -1, 0);
		X509_NAME_add_entry_by_NID(x509_name, 48, 0, 
			(unsigned char *) email.c_str(), -1, -1, 0);
	}

	if (0 < strlen(org.c_str()))
	{
		X509_NAME_add_entry_by_txt(x509_name, "O", chtype, 
			(unsigned char *) org.c_str(), -1, -1, 0);
	}

	if (0 < strlen(loc.c_str()))
	{
		X509_NAME_add_entry_by_txt(x509_name, "L", chtype, 
			(unsigned char *) loc.c_str(), -1, -1, 0);
	}

	if (0 < strlen(state.c_str()))
	{
		X509_NAME_add_entry_by_txt(x509_name, "ST", chtype, 
			(unsigned char *) state.c_str(), -1, -1, 0);
	}

	if (0 < strlen(country.c_str()))
	{
		X509_NAME_add_entry_by_txt(x509_name, "C", chtype, 
			(unsigned char *) country.c_str(), -1, -1, 0);
	}

	if (!X509_REQ_set_subject_name(req,x509_name))
	{
		fprintf(stderr,"GenerateX509Req() Couldn't Set Name to Request!\n");
		X509_NAME_free(x509_name);

		errString = "Couldn't Set Name";
		return 0;
	}

	X509_NAME_free(x509_name);
	/**** SUBJECT         ********************************/

	if (!X509_REQ_sign(req,pkey,EVP_sha1()))
	{
		fprintf(stderr,"GenerateX509Req() Failed to Sign REQ\n");

		errString = "Couldn't Sign Req";
		return 0;
	}

	errString = "No Error";
	return req;
}

#define SERIAL_RAND_BITS 	64

X509 *SignX509Certificate(X509_NAME *issuer, EVP_PKEY *privkey, X509_REQ *req, long days)
{
	const EVP_MD *digest = EVP_sha1();
	ASN1_INTEGER *serial = ASN1_INTEGER_new();
	EVP_PKEY *tmppkey;
	X509 *x509 = X509_new();
	if (x509 == NULL)
		return NULL;

        BIGNUM *btmp = BN_new();
        if (!BN_pseudo_rand(btmp, SERIAL_RAND_BITS, 0, 0))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," pseudo_rand\n");

		return NULL;
	}
        if (!BN_to_ASN1_INTEGER(btmp, serial))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," int\n");

		return NULL;
	}
        BN_free(btmp);

	if (!X509_set_serialNumber(x509, serial)) 
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," serialNumber\n");

		return NULL;
	}
	ASN1_INTEGER_free(serial);

	if (!X509_set_issuer_name(x509, issuer))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," issuer\n");

		return NULL;
	}

        if (!X509_gmtime_adj(x509->cert_info->validity->notBefore, 0))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," notBefore\n");

		return NULL;
	}

	//x509->cert_info->validity->notAfter
        //if (!X509_gmtime_adj(X509_get_notAfter(x509), (long)60*60*24*days))
        if (!X509_gmtime_adj(x509->cert_info->validity->notAfter, (long)60*60*24*days))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," notAfter\n");

		return NULL;
	}

        if (!X509_set_subject_name(x509, X509_REQ_get_subject_name(req)))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," subject_name\n");

		return NULL;
	}


        tmppkey = X509_REQ_get_pubkey(req);
        if (!tmppkey || !X509_set_pubkey(x509,tmppkey))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," pubkey\n");

		return NULL;
	}


	/* Cleanup Algorithm part */

        X509_ALGOR *algor1 = x509->cert_info->signature;
        X509_ALGOR *algor2 = x509->sig_alg;

        X509_ALGOR *a;

        a = algor1;
        ASN1_TYPE_free(a->parameter);
        a->parameter=ASN1_TYPE_new();
        a->parameter->type=V_ASN1_NULL;

        ASN1_OBJECT_free(a->algorithm);
        a->algorithm=OBJ_nid2obj(digest->pkey_type);

        a = algor2;
        ASN1_TYPE_free(a->parameter);
        a->parameter=ASN1_TYPE_new();
        a->parameter->type=V_ASN1_NULL;

        ASN1_OBJECT_free(a->algorithm);
        a->algorithm=OBJ_nid2obj(digest->pkey_type);
  

        if (!X509_sign(x509,privkey,digest))
	{
		long e = ERR_get_error();

		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," signing Error: %ld\n", e);

		fprintf(stderr,"ERR: %s, %s, %s\n", 
			ERR_lib_error_string(e),
			ERR_func_error_string(e),
			ERR_reason_error_string(e));

        	int inl=i2d_X509(x509,NULL);
        	int outl=EVP_PKEY_size(privkey);
		fprintf(stderr,"Size Check: inl: %d, outl: %d\n", inl, outl);

		return NULL;
	}

	fprintf(stderr,"SignX509Certificate() Success\n");

	return x509;
}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


	/* Sign / Encrypt / Verify Data */
bool SSL_SignDataBin(const void *data, const uint32_t len, 
		unsigned char *sign, unsigned int *signlen, EVP_PKEY *pkey)
{
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
        unsigned int req_signlen = EVP_PKEY_size(pkey);
	if (req_signlen > *signlen)
	{
		/* not enough space */
		std::cerr << "SignDataBin() Not Enough Sign SpacegnInit Failure!" << std::endl;
		return false;
	}

	if (0 == EVP_SignInit(mdctx, EVP_sha1()))
	{
		std::cerr << "EVP_SignInit Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	if (0 == EVP_SignUpdate(mdctx, data, len))
	{
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

        if (0 == EVP_SignFinal(mdctx, sign, signlen, pkey))
	{
		std::cerr << "EVP_SignFinal Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	EVP_MD_CTX_destroy(mdctx);
	return true;
}

bool SSL_VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, X509 *cert)
{

        /* cert->cert_info->key->pkey is NULL....
	 * but is instantiated when we call X509_get_pubkey()
	 */

        EVP_PKEY *peerkey = X509_get_pubkey(cert);

	/* must free this key afterwards */
	bool ret = SSL_VerifySignBin(data, len, sign, signlen, peerkey);

	EVP_PKEY_free(peerkey);

	return ret;
}

/* Correct form of this function ... Internal for AuthSSL's usage
 */

bool SSL_VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, EVP_PKEY *peerkey)
{
        if(peerkey == NULL)
	{
		std::cerr << "VerifySignBin: no public key available !!" << std::endl ;
		return false ;
	}

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
	if (0 == EVP_VerifyInit(mdctx, EVP_sha1()))
	{
		std::cerr << "EVP_VerifyInit Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
                return false;
	}

	if (0 == EVP_VerifyUpdate(mdctx, data, len))
	{
		std::cerr << "EVP_VerifyUpdate Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	if(signlen == 0 || sign == NULL)
	{
		std::cerr << "AuthSSL::VerifySignBin: signlen=" << signlen << ", sign=" << (void*)sign << "!!" << std::endl ;
		EVP_MD_CTX_destroy(mdctx);
		return false ;
	}

	if (0 == EVP_VerifyFinal(mdctx, sign, signlen, peerkey))
	{
		std::cerr << "EVP_VerifyFinal Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	EVP_MD_CTX_destroy(mdctx);
	return true;
}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

X509 *loadX509FromPEM(std::string pem)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "loadX509FromPEM()";
	std::cerr << std::endl;
#endif

	/* Put the data into a mem BIO */
	char *certstr = strdup(pem.c_str());

	BIO *bp = BIO_new_mem_buf(certstr, -1);

	X509 *pc = PEM_read_bio_X509(bp, NULL, NULL, NULL);

	BIO_free(bp);
	free(certstr);

	return pc;
}

std::string saveX509ToPEM(X509* x509)
{
#ifdef AUTHSSL_DEBUG
        std::cerr << "saveX509ToPEM() " << std::endl;
#endif

        /* get the cert first */
        std::string certstr;
        BIO *bp = BIO_new(BIO_s_mem());

        PEM_write_bio_X509(bp, x509);

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


X509 *loadX509FromDER(const uint8_t *ptr, uint32_t len)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::LoadX509FromDER()";
	std::cerr << std::endl;
#endif

	X509 *tmp = NULL;
#ifdef __APPLE__
	unsigned char **certptr = (unsigned char **) &ptr;
#else
	const unsigned char **certptr = (const unsigned char **) &ptr;
#endif
	
	X509 *x509 = d2i_X509(&tmp, certptr, len);

	return x509;
}

bool saveX509ToDER(X509 *x509, uint8_t **ptr, uint32_t *len)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::saveX509ToDER()";
	std::cerr << std::endl;
#endif

	int certlen = i2d_X509(x509, (unsigned char **) ptr);
	if (certlen > 0)
	{
		*len = certlen;
		return true;
	}
	else
	{
		*len = 0;
		return false;
	}
	return false;
}


bool getX509id(X509 *x509, std::string &xid) {
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getX509id()";
	std::cerr << std::endl;
#endif

	xid = "";
	if (x509 == NULL)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSL::getX509id() NULL pointer";
		std::cerr << std::endl;
#endif
		return false;
	}

	// get the signature from the cert, and copy to the array.
	ASN1_BIT_STRING *signature = x509->signature;
	int signlen = ASN1_STRING_length(signature);
	if (signlen < CERTSIGNLEN)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSL::getX509id() ERROR: Short Signature";
		std::cerr << std::endl;
#endif
		return false;
	}

	// else copy in the first CERTSIGNLEN.
	unsigned char *signdata = ASN1_STRING_data(signature);
	
        std::ostringstream id;
	/* switched to the other end of the signature. for
	 * more randomness
	 */
	for(int i = signlen - CERTSIGNLEN; i < signlen; i++)
	{
		id << std::hex << std::setw(2) << std::setfill('0') 
			<< (uint16_t) (((uint8_t *) (signdata))[i]);
	}
	xid = id.str();
	return true;
}


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

int pem_passwd_cb(char *buf, int size, int rwflag, void *password)
{
	/* remove unused parameter warnings */
	(void) rwflag;

	strncpy(buf, (char *)(password), size);
	buf[size - 1] = '\0';
	return(strlen(buf));
}

/* XXX FIX */
bool CheckX509Certificate(X509 *x509)
{

	return true;
}


// Not dependent on sslroot. load, and detroys the X509 memory.
int	LoadCheckX509(const char *cert_file, std::string &issuerName, std::string &location, std::string &userId)
{
	/* This function loads the X509 certificate from the file, 
	 * and checks the certificate 
	 */

	FILE *tmpfp = RsDirUtil::rs_fopen(cert_file, "r");
	if (tmpfp == NULL)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "sslroot::LoadCheckAndGetX509Name()";
		std::cerr << " Failed to open Certificate File:" << cert_file;
		std::cerr << std::endl;
#endif
		return 0;
	}

	// get xPGP certificate.
	X509 *x509 = PEM_read_X509(tmpfp, NULL, NULL, NULL);
	fclose(tmpfp);

	// check the certificate.
	bool valid = false;
	if (x509)
	{
                valid = CheckX509Certificate(x509);
		if (valid)
		{
                	valid = getX509id(x509, userId);
		}
	}

	if (valid)
	{
		// extract the name.
		issuerName = getX509CNString(x509->cert_info->issuer);
		location = getX509LocString(x509->cert_info->subject);
	}

        #ifdef AUTHSSL_DEBUG
	std::cout << getX509Info(x509) << std::endl ;
        #endif
	// clean up.
	X509_free(x509);

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


std::string getX509TypeString(X509_NAME *name, const char *type, int len)
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


std::string getX509Info(X509 *cert)
{
	std::stringstream out;
	long l;

	out << "X509 Certificate:" << std::endl;
	l=X509_get_version(cert);
	out << "     Version: " << l+1 << "(0x" << l << ")" << std::endl;
	out << "     Subject: " << std::endl;
	out << "  " << getX509NameString(cert->cert_info->subject);
	out << std::endl;
	out << std::endl;
	out << "     Signatures:" << std::endl;
	return out.str();
}

/********** SSL ERROR STUFF ******************************************/

int printSSLError(SSL *ssl, int retval, int err, unsigned long err2, 
		std::ostream &out)
{
	(void) ssl; /* remove unused parameter warnings */

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

