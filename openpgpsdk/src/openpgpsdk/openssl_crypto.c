/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. 
 * 
 * You may obtain a copy of the License at 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * 
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file
 */

#include <openssl/bio.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <assert.h>
#include <stdlib.h>

#include <openpgpsdk/configure.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/readerwriter.h>
#include "keyring_local.h"
#include <openpgpsdk/std_print.h>

#include <openpgpsdk/final.h>

static int debug=0;

void test_secret_key(const ops_secret_key_t *skey)
    {
    RSA* test=RSA_new();

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    test->n=BN_dup(skey->public_key.key.rsa.n);
    test->e=BN_dup(skey->public_key.key.rsa.e);
    test->d=BN_dup(skey->key.rsa.d);

    test->p=BN_dup(skey->key.rsa.p);
    test->q=BN_dup(skey->key.rsa.q);
#else
    RSA_set0_key(test,
		    BN_dup(skey->public_key.key.rsa.n),
    		BN_dup(skey->public_key.key.rsa.e),
		    BN_dup(skey->key.rsa.d));

    RSA_set0_factors(test, BN_dup(skey->key.rsa.p), BN_dup(skey->key.rsa.q));
#endif

    assert(RSA_check_key(test)==1);
    RSA_free(test);
    }

static void md5_init(ops_hash_t *hash)
    {
    assert(!hash->data);
    hash->data=malloc(sizeof(MD5_CTX));
    MD5_Init(hash->data);
    }

static void md5_add(ops_hash_t *hash,const unsigned char *data,unsigned length)
    {
    MD5_Update(hash->data,data,length);
    }

static unsigned md5_finish(ops_hash_t *hash,unsigned char *out)
    {
    MD5_Final(out,hash->data);
    free(hash->data);
    hash->data=NULL;
    return 16;
    }

static ops_hash_t md5={OPS_HASH_MD5,MD5_DIGEST_LENGTH,"MD5",md5_init,md5_add,
		       md5_finish,NULL};

/**
   \ingroup Core_Crypto
   \brief Initialise to MD5
   \param hash Hash to initialise
*/
void ops_hash_md5(ops_hash_t *hash)
    {
    *hash=md5;
    }

static void sha1_init(ops_hash_t *hash)
    {
    if (debug)
        {
        fprintf(stderr,"***\n***\nsha1_init\n***\n");
        }
    assert(!hash->data);
    hash->data=malloc(sizeof(SHA_CTX));
    SHA1_Init(hash->data);
    }

static void sha1_add(ops_hash_t *hash,const unsigned char *data,
		     unsigned length)
    {
    if (debug)
        {
        unsigned int i=0;
        fprintf(stderr,"adding %d to hash:\n ", length);
        for (i=0; i<length; i++)
            {
            fprintf(stderr,"0x%02x ", data[i]);
            if (!((i+1) % 16))
                fprintf(stderr,"\n");
            else if (!((i+1) % 8))
                fprintf(stderr,"  ");
            }
        fprintf(stderr,"\n");
        }
    SHA1_Update(hash->data,data,length);
    }

static unsigned sha1_finish(ops_hash_t *hash,unsigned char *out)
    {
    SHA1_Final(out,hash->data);
    if (debug)
        {
        unsigned i=0;
        fprintf(stderr,"***\n***\nsha1_finish\n***\n");
        for (i=0; i<SHA_DIGEST_LENGTH; i++)
            fprintf(stderr,"0x%02x ",out[i]);
        fprintf(stderr,"\n");
        }
    free(hash->data);
    hash->data=NULL;
    return SHA_DIGEST_LENGTH;
    }

static ops_hash_t sha1={OPS_HASH_SHA1,SHA_DIGEST_LENGTH,"SHA1",sha1_init,
			sha1_add,sha1_finish,NULL};

/**
   \ingroup Core_Crypto
   \brief Initialise to SHA1
   \param hash Hash to initialise
*/
void ops_hash_sha1(ops_hash_t *hash)
    {
    *hash=sha1;
    }

static void sha256_init(ops_hash_t *hash)
    {
    if (debug)
        {
        fprintf(stderr,"***\n***\nsha256_init\n***\n");
        }
    assert(!hash->data);
    hash->data=malloc(sizeof(SHA256_CTX));
    SHA256_Init(hash->data);
    }

static void sha256_add(ops_hash_t *hash,const unsigned char *data,
		     unsigned length)
    {
    if (debug)
        {
        unsigned int i=0;
        fprintf(stderr,"adding %d to hash:\n ", length);
        for (i=0; i<length; i++)
            {
            fprintf(stderr,"0x%02x ", data[i]);
            if (!((i+1) % 16))
                fprintf(stderr,"\n");
            else if (!((i+1) % 8))
                fprintf(stderr,"  ");
            }
        fprintf(stderr,"\n");
        }
    SHA256_Update(hash->data,data,length);
    }

static unsigned sha256_finish(ops_hash_t *hash,unsigned char *out)
    {
    SHA256_Final(out,hash->data);
    if (debug)
        {
        unsigned i=0;
        fprintf(stderr,"***\n***\nsha1_finish\n***\n");
        for (i=0; i<SHA256_DIGEST_LENGTH; i++)
            fprintf(stderr,"0x%02x ",out[i]);
        fprintf(stderr,"\n");
        }
    free(hash->data);
    hash->data=NULL;
    return SHA256_DIGEST_LENGTH;
    }

static ops_hash_t sha256={OPS_HASH_SHA256,SHA256_DIGEST_LENGTH,"SHA256",sha256_init,
			sha256_add,sha256_finish,NULL};

void ops_hash_sha256(ops_hash_t *hash)
    {
    *hash=sha256;
    }

/*
 * SHA384
 */

static void sha384_init(ops_hash_t *hash)
    {
    if (debug)
        {
        fprintf(stderr,"***\n***\nsha384_init\n***\n");
        }
    assert(!hash->data);
    hash->data=malloc(sizeof(SHA512_CTX));
    SHA384_Init(hash->data);
    }

static void sha384_add(ops_hash_t *hash,const unsigned char *data,
		     unsigned length)
    {
    if (debug)
        {
        unsigned int i=0;
        fprintf(stderr,"adding %d to hash:\n ", length);
        for (i=0; i<length; i++)
            {
            fprintf(stderr,"0x%02x ", data[i]);
            if (!((i+1) % 16))
                fprintf(stderr,"\n");
            else if (!((i+1) % 8))
                fprintf(stderr,"  ");
            }
        fprintf(stderr,"\n");
        }
    SHA384_Update(hash->data,data,length);
    }

static unsigned sha384_finish(ops_hash_t *hash,unsigned char *out)
    {
    SHA384_Final(out,hash->data);
    if (debug)
        {
        unsigned i=0;
        fprintf(stderr,"***\n***\nsha1_finish\n***\n");
        for (i=0; i<SHA384_DIGEST_LENGTH; i++)
            fprintf(stderr,"0x%02x ",out[i]);
        fprintf(stderr,"\n");
        }
    free(hash->data);
    hash->data=NULL;
    return SHA384_DIGEST_LENGTH;
    }

static ops_hash_t sha384={OPS_HASH_SHA384,SHA384_DIGEST_LENGTH,"SHA384",sha384_init,
			sha384_add,sha384_finish,NULL};

void ops_hash_sha384(ops_hash_t *hash)
    {
    *hash=sha384;
    }

/*
 * SHA512
 */

static void sha512_init(ops_hash_t *hash)
    {
    if (debug)
        {
        fprintf(stderr,"***\n***\nsha512_init\n***\n");
        }
    assert(!hash->data);
    hash->data=malloc(sizeof(SHA512_CTX));
    SHA512_Init(hash->data);
    }

static void sha512_add(ops_hash_t *hash,const unsigned char *data,
		     unsigned length)
    {
    if (debug)
        {
        unsigned int i=0;
        fprintf(stderr,"adding %d to hash:\n ", length);
        for (i=0; i<length; i++)
            {
            fprintf(stderr,"0x%02x ", data[i]);
            if (!((i+1) % 16))
                fprintf(stderr,"\n");
            else if (!((i+1) % 8))
                fprintf(stderr,"  ");
            }
        fprintf(stderr,"\n");
        }
    SHA512_Update(hash->data,data,length);
    }

static unsigned sha512_finish(ops_hash_t *hash,unsigned char *out)
    {
    SHA512_Final(out,hash->data);
    if (debug)
        {
        unsigned i=0;
        fprintf(stderr,"***\n***\nsha1_finish\n***\n");
        for (i=0; i<SHA512_DIGEST_LENGTH; i++)
            fprintf(stderr,"0x%02x ",out[i]);
        fprintf(stderr,"\n");
        }
    free(hash->data);
    hash->data=NULL;
    return SHA512_DIGEST_LENGTH;
    }

static ops_hash_t sha512={OPS_HASH_SHA512,SHA512_DIGEST_LENGTH,"SHA512",sha512_init,
			sha512_add,sha512_finish,NULL};

void ops_hash_sha512(ops_hash_t *hash)
    {
    *hash=sha512;
    }

/*
 * SHA224
 */

static void sha224_init(ops_hash_t *hash)
    {
    if (debug)
        {
        fprintf(stderr,"***\n***\nsha1_init\n***\n");
        }
    assert(!hash->data);
    hash->data=malloc(sizeof(SHA256_CTX));
    SHA224_Init(hash->data);
    }

static void sha224_add(ops_hash_t *hash,const unsigned char *data,
		     unsigned length)
    {
    if (debug)
        {
        unsigned int i=0;
        fprintf(stderr,"adding %d to hash:\n ", length);
        for (i=0; i<length; i++)
            {
            fprintf(stderr,"0x%02x ", data[i]);
            if (!((i+1) % 16))
                fprintf(stderr,"\n");
            else if (!((i+1) % 8))
                fprintf(stderr,"  ");
            }
        fprintf(stderr,"\n");
        }
    SHA224_Update(hash->data,data,length);
    }

static unsigned sha224_finish(ops_hash_t *hash,unsigned char *out)
    {
    SHA224_Final(out,hash->data);
    if (debug)
        {
        unsigned i=0;
        fprintf(stderr,"***\n***\nsha1_finish\n***\n");
        for (i=0; i<SHA224_DIGEST_LENGTH; i++)
            fprintf(stderr,"0x%02x ",out[i]);
        fprintf(stderr,"\n");
        }
    free(hash->data);
    hash->data=NULL;
    return SHA224_DIGEST_LENGTH;
    }

static ops_hash_t sha224={OPS_HASH_SHA224,SHA224_DIGEST_LENGTH,"SHA224",sha224_init,
			sha224_add,sha224_finish,NULL};

void ops_hash_sha224(ops_hash_t *hash)
    {
    *hash=sha224;
    }

ops_boolean_t already_said = ops_false ;

ops_boolean_t ops_dsa_verify(const unsigned char *hash,size_t hash_length,
			     const ops_dsa_signature_t *sig,
			     const ops_dsa_public_key_t *dsa)
    {
    DSA_SIG *osig;
    DSA *odsa;
    int ret;

    osig=DSA_SIG_new();

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    osig->r=sig->r;
    osig->s=sig->s;
#else
    DSA_SIG_set0(osig,BN_dup(sig->r),BN_dup(sig->s)) ;
#endif

	 if(BN_num_bits(dsa->q) != 160)
	 {
		 if(!already_said)
		 {
			 fprintf(stderr,"(WW) ops_dsa_verify: openssl does only supports 'q' of 160 bits. Current is %d bits.\n",BN_num_bits(dsa->q)) ;
			 already_said=ops_true ;
		 }

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
		 osig->r=NULL;			// in this case, the values are not copied.
		 osig->s=NULL;
#endif

		 DSA_SIG_free(osig);
		 return ops_false ;
	 }

    odsa=DSA_new();
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    odsa->p=dsa->p;
    odsa->q=dsa->q;
    odsa->g=dsa->g;

    odsa->pub_key=dsa->y;
#else
    DSA_set0_pqg(odsa,BN_dup(dsa->p),BN_dup(dsa->q),BN_dup(dsa->g));
    DSA_set0_key(odsa,BN_dup(dsa->y),NULL) ;
#endif

    if (debug)
        {
        fprintf(stderr,"hash passed in:\n");
        unsigned i;
        for (i=0; i<hash_length; i++)
            {
            fprintf(stderr,"%02x ", hash[i]);
            }
        fprintf(stderr,"\n");
        }
    //printf("hash_length=%ld\n", hash_length);
    //printf("Q=%d\n", BN_num_bytes(odsa->q));
    unsigned int qlen=BN_num_bytes(dsa->q);

    if (qlen < hash_length)
        hash_length=qlen;
    //    ret=DSA_do_verify(hash,hash_length,osig,odsa);
    ret=DSA_do_verify(hash,hash_length,osig,odsa);
    if (debug)
        {
        fprintf(stderr,"ret=%d\n",ret);
        }

	 if(ret < 0)
	 {
		 ERR_load_crypto_strings() ;
		 unsigned long err = 0 ;
         while((err = ERR_get_error()) > 0)
			 fprintf(stderr,"DSA_do_verify(): ERR = %ld. lib error:\"%s\", func_error:\"%s\", reason:\"%s\"\n",err,ERR_lib_error_string(err),ERR_func_error_string(err),ERR_reason_error_string(err)) ;
    	//assert(ret >= 0);
		return ops_false ;
	 }

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    osig->r=NULL;
    osig->s=NULL;

    odsa->p=NULL;
    odsa->q=NULL;
    odsa->g=NULL;
    odsa->pub_key=NULL;
#endif

    DSA_free(odsa);
    DSA_SIG_free(osig);

    return ret != 0;
    }

/**
   \ingroup Core_Crypto
   \brief Recovers message digest from the signature
   \param out Where to write decrypted data to
   \param in Encrypted data
   \param length Length of encrypted data
   \param rsa RSA public key
   \return size of recovered message digest
*/
int ops_rsa_public_decrypt(unsigned char *out,const unsigned char *in,
			   size_t length,const ops_rsa_public_key_t *rsa)
    {
    RSA *orsa;
    int n;

    orsa=RSA_new();
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    orsa->n=rsa->n;
    orsa->e=rsa->e;
#else
    RSA_set0_key(orsa,BN_dup(rsa->n),BN_dup(rsa->e),NULL) ;
#endif

    n=RSA_public_decrypt(length,in,out,orsa,RSA_NO_PADDING);

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    orsa->n=orsa->e=NULL;
#endif
    RSA_free(orsa);

    return n;
    }

/**
   \ingroup Core_Crypto
   \brief Signs data with RSA
   \param out Where to write signature
   \param in Data to sign
   \param length Length of data
   \param srsa RSA secret key
   \param rsa RSA public key
   \return number of bytes decrypted
*/
int ops_rsa_private_encrypt(unsigned char *out,const unsigned char *in,
			    size_t length,const ops_rsa_secret_key_t *srsa,
			    const ops_rsa_public_key_t *rsa)
    {
    RSA *orsa;
    int n;

    orsa=RSA_new();
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    orsa->n=rsa->n;	// XXX: do we need n?
    orsa->d=srsa->d;
    orsa->p=srsa->q;
    orsa->q=srsa->p;

    /* debug */
    orsa->e=rsa->e;

    // If this isn't set, it's very likely that the programmer hasn't
    // decrypted the secret key. RSA_check_key segfaults in that case.
    // Use ops_decrypt_secret_key_from_data() to do that.
    assert(orsa->d);
#else
    RSA_set0_key(orsa,BN_dup(rsa->n),BN_dup(rsa->e),BN_dup(srsa->d)) ;
    RSA_set0_factors(orsa,BN_dup(srsa->p),BN_dup(srsa->q));
#endif

    assert(RSA_check_key(orsa) == 1);
    /* end debug */

    // WARNING: this function should *never* be called for direct encryption, because of the padding.
    // It's actually only called in the signature function now, where an adapted padding is placed.

    n=RSA_private_encrypt(length,in,out,orsa,RSA_NO_PADDING);

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    orsa->n=orsa->d=orsa->p=orsa->q=NULL;
    orsa->e=NULL;
#endif
    RSA_free(orsa);

    return n;
    }

/**
\ingroup Core_Crypto
\brief Decrypts RSA-encrypted data
\param out Where to write the plaintext
\param in Encrypted data
\param length Length of encrypted data
\param srsa RSA secret key
\param rsa RSA public key
\return size of recovered plaintext
*/
int ops_rsa_private_decrypt(unsigned char *out,const unsigned char *in,
			    size_t length,const ops_rsa_secret_key_t *srsa,
			    const ops_rsa_public_key_t *rsa)
    {
    RSA *orsa;
    int n;
    char errbuf[1024];

    orsa=RSA_new();
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    orsa->n=rsa->n;	// XXX: do we need n?
    orsa->d=srsa->d;
    orsa->p=srsa->q;
    orsa->q=srsa->p;
    orsa->e=rsa->e;
#else
    RSA_set0_key(orsa,BN_dup(rsa->n),BN_dup(rsa->e),BN_dup(srsa->d)) ;
    RSA_set0_factors(orsa,BN_dup(srsa->p),BN_dup(srsa->q));
#endif

    /* debug */
    assert(RSA_check_key(orsa) == 1);
    /* end debug */

    n=RSA_private_decrypt(length,in,out,orsa,RSA_NO_PADDING);

    //    printf("ops_rsa_private_decrypt: n=%d\n",n);

    errbuf[0]='\0';
    if (n==-1)
        {
        unsigned long err=ERR_get_error();
        ERR_error_string(err,&errbuf[0]);
        fprintf(stderr,"openssl error : %s\n",errbuf);
        }
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    orsa->n=orsa->d=orsa->p=orsa->q=NULL;
    orsa->e=NULL;
#endif
    RSA_free(orsa);

    return n;
    }

/**
   \ingroup Core_Crypto
   \brief RSA-encrypts data
   \param out Where to write the encrypted data
   \param in Plaintext
   \param length Size of plaintext
   \param rsa RSA Public Key
*/
int ops_rsa_public_encrypt(unsigned char *out,const unsigned char *in,
			   size_t length,const ops_rsa_public_key_t *rsa)
    {
    RSA *orsa;
    int n;

    //    printf("ops_rsa_public_encrypt: length=%ld\n", length);

    orsa=RSA_new();
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    orsa->n=rsa->n;
    orsa->e=rsa->e;
#else
    RSA_set0_key(orsa,BN_dup(rsa->n),BN_dup(rsa->e),NULL);
#endif

    //    printf("len: %ld\n", length);
    //    ops_print_bn("n: ", orsa->n);
    //    ops_print_bn("e: ", orsa->e);
    n=RSA_public_encrypt(length,in,out,orsa,RSA_NO_PADDING);

    if (n==-1)
    {
	    BIO *fd_out;
	    fd_out=BIO_new_fd(fileno(stderr), BIO_NOCLOSE);
	    ERR_print_errors(fd_out);
	    BIO_free(fd_out) ;
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    orsa->n=orsa->e=NULL;
#endif
    RSA_free(orsa);

    return n;
    }

/**
   \ingroup Core_Crypto
   \brief initialises openssl
   \note Would usually call ops_init() instead
   \sa ops_init()
*/
void ops_crypto_init()
    {
#ifdef DMALLOC
    CRYPTO_malloc_debug_init();
    CRYPTO_dbg_set_options(V_CRYPTO_MDEBUG_ALL);
    CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
#endif
    }

/**
   \ingroup Core_Crypto
   \brief Finalise openssl
   \note Would usually call ops_finish() instead
   \sa ops_finish()
*/
void ops_crypto_finish()
    {
    CRYPTO_cleanup_all_ex_data();
    // FIXME: what should we do instead (function is deprecated)?
    //    ERR_remove_state(0);
#ifdef DMALLOC
    CRYPTO_mem_leaks_fp(stderr);
#endif
    }

/**
   \ingroup Core_Hashes
   \brief Get Hash name
   \param hash Hash struct
   \return Hash name
*/
const char *ops_text_from_hash(ops_hash_t *hash)
    { return hash->name; }

/**
 \ingroup HighLevel_KeyGenerate
 \brief Generates an RSA keypair
 \param numbits Modulus size
 \param e Public Exponent
 \param keydata Pointer to keydata struct to hold new key
 \return ops_true if key generated successfully; otherwise ops_false
 \note It is the caller's responsibility to call ops_keydata_free(keydata)
*/
ops_boolean_t ops_rsa_generate_keypair(const int numbits, const unsigned long e,
				       ops_keydata_t* keydata)
    {
    ops_secret_key_t *skey=NULL;
    RSA *rsa=RSA_new();
    BN_CTX *ctx=BN_CTX_new();
    BIGNUM *ebn=BN_new();

    ops_keydata_init(keydata,OPS_PTAG_CT_SECRET_KEY);
    skey=ops_get_writable_secret_key_from_data(keydata);

    // generate the key pair

    BN_set_word(ebn,e);
    RSA_generate_key_ex(rsa,numbits,ebn,NULL);

    // populate ops key from ssl key

    skey->public_key.version=4;
    skey->public_key.creation_time=time(NULL);
    skey->public_key.days_valid=0;
    skey->public_key.algorithm= OPS_PKA_RSA;

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    skey->public_key.key.rsa.n=BN_dup(rsa->n);
    skey->public_key.key.rsa.e=BN_dup(rsa->e);
    skey->key.rsa.d=BN_dup(rsa->d);
#else
    const BIGNUM *nn=NULL,*ee=NULL,*dd=NULL ;

    RSA_get0_key(rsa,&nn,&ee,&dd) ;

    skey->public_key.key.rsa.n=BN_dup(nn) ;
    skey->public_key.key.rsa.e=BN_dup(ee) ;
    skey->key.rsa.d=BN_dup(dd) ;
#endif

    skey->s2k_usage=OPS_S2KU_ENCRYPTED_AND_HASHED;
    skey->s2k_specifier=OPS_S2KS_SALTED;
    //skey->s2k_specifier=OPS_S2KS_SIMPLE;
    skey->algorithm=OPS_SA_CAST5; // \todo make param
    skey->hash_algorithm=OPS_HASH_SHA1; // \todo make param
    skey->octet_count=0;
    skey->checksum=0;

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    skey->key.rsa.p=BN_dup(rsa->p);
    skey->key.rsa.q=BN_dup(rsa->q);
    skey->key.rsa.u=BN_mod_inverse(NULL,rsa->p, rsa->q, ctx);
#else
    const BIGNUM *pp=NULL,*qq=NULL ;

    RSA_get0_factors(rsa,&pp,&qq) ;

    skey->key.rsa.p=BN_dup(pp);
    skey->key.rsa.q=BN_dup(qq);

    skey->key.rsa.u=BN_mod_inverse(NULL,pp,qq, ctx);
#endif
    assert(skey->key.rsa.u);
    BN_CTX_free(ctx);

    RSA_free(rsa);

    ops_keyid(keydata->key_id, &keydata->key.skey.public_key);
    ops_fingerprint(&keydata->fingerprint, &keydata->key.skey.public_key);

    // Generate checksum

    ops_create_info_t *cinfo=NULL;
    ops_memory_t *mem=NULL;

    ops_setup_memory_write(&cinfo, &mem, 128);

    ops_push_skey_checksum_writer(cinfo, skey);

    switch(skey->public_key.algorithm)
	{
	//    case OPS_PKA_DSA:
	//	return ops_write_mpi(key->key.dsa.x,info);

    case OPS_PKA_RSA:
    case OPS_PKA_RSA_ENCRYPT_ONLY:
    case OPS_PKA_RSA_SIGN_ONLY:
	if(!ops_write_mpi(skey->key.rsa.d,cinfo)
	   || !ops_write_mpi(skey->key.rsa.p,cinfo)
	   || !ops_write_mpi(skey->key.rsa.q,cinfo)
	   || !ops_write_mpi(skey->key.rsa.u,cinfo))
	    return ops_false;
	break;

	//    case OPS_PKA_ELGAMAL:
	//	return ops_write_mpi(key->key.elgamal.x,info);

    default:
	assert(0);
	break;
	}

    // close rather than pop, since its the only one on the stack
    ops_writer_close(cinfo);
    ops_teardown_memory_write(cinfo, mem);

    // should now have checksum in skey struct

    // test
    if (debug)
        test_secret_key(skey);

    return ops_true;
    }

/**
 \ingroup HighLevel_KeyGenerate
 \brief Creates a self-signed RSA keypair
 \param numbits Modulus size
 \param e Public Exponent
 \param userid User ID
 \return The new keypair or NULL

 \note It is the caller's responsibility to call ops_keydata_free(keydata)
 \sa ops_rsa_generate_keypair()
 \sa ops_keydata_free()
*/
ops_keydata_t* ops_rsa_create_selfsigned_keypair(const int numbits, const unsigned long e, ops_user_id_t * userid)
    {
    ops_keydata_t *keydata=NULL;

    keydata=ops_keydata_new();

    if (ops_rsa_generate_keypair(numbits, e, keydata) != ops_true
        || ops_add_selfsigned_userid_to_keydata(keydata, userid) != ops_true)
        {
        ops_keydata_free(keydata);
        return NULL;
        }

    return keydata;
    }

/*
int ops_dsa_size(const ops_dsa_public_key_t *dsa)
    {
    int size;
    DSA *odsa;
    odsa=DSA_new();
    odsa->p=dsa->p;
    odsa->q=dsa->q;
    odsa->g=dsa->g;
    odsa->pub_key=dsa->y;

    DSAparams_print_fp(stderr, odsa);
    size=DSA_size(odsa);

    odsa->p=odsa->q=odsa->g=odsa->pub_key=odsa->priv_key=NULL;
    DSA_free(odsa);

    return size;
    }
*/

DSA_SIG* ops_dsa_sign(unsigned char* hashbuf, unsigned hashsize, const ops_dsa_secret_key_t *sdsa, const ops_dsa_public_key_t *dsa)
    {
    DSA *odsa;
    DSA_SIG *dsasig;

    odsa=DSA_new();
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    odsa->p=dsa->p;
    odsa->q=dsa->q;
    odsa->g=dsa->g;
    odsa->pub_key=dsa->y;
    odsa->priv_key=sdsa->x;
#else
    DSA_set0_pqg(odsa,BN_dup(dsa->p),BN_dup(dsa->q),BN_dup(dsa->g));
    DSA_set0_key(odsa,BN_dup(dsa->y),BN_dup(sdsa->x));
#endif

    dsasig=DSA_do_sign(hashbuf,hashsize,odsa);

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    odsa->p=odsa->q=odsa->g=odsa->pub_key=odsa->priv_key=NULL;
#endif
    DSA_free(odsa);

    return dsasig;
    }

// eof
