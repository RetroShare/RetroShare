/*
 * Copyright (c) 2005-2009 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer, Alasdair Mackintosh.
 * The Contributors have asserted their moral rights under the
 * UK Copyright Design and Patents Act 1988 to
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

#include <openpgpsdk/signature.h>
#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/create.h>
#include <openpgpsdk/literal.h>
#include <openpgpsdk/partial.h>
#include <openpgpsdk/writer_armoured.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <openpgpsdk/final.h>
#include <openpgpsdk/opsdir.h>

#include <openssl/dsa.h>

static int debug=0;
#define MAXBUF 1024 /*<! Standard buffer size to use */

/** \ingroup Core_Create
 * needed for signature creation
 */
struct ops_create_signature
    {
    ops_hash_t hash; 
    ops_signature_t sig; 
    ops_memory_t *mem; 
    ops_create_info_t *info; /*!< how to do the writing */
    unsigned hashed_count_offset;
    unsigned hashed_data_length;
    unsigned unhashed_count_offset;
    };

/**
   \ingroup Core_Signature
   Creates new ops_create_signature_t
   \return new ops_create_signature_t
   \note It is the caller's responsibility to call ops_create_signature_delete()
   \sa ops_create_signature_delete()
*/
ops_create_signature_t *ops_create_signature_new()
    { return ops_mallocz(sizeof(ops_create_signature_t)); }

/**
   \ingroup Core_Signature
   Free signature and memory associated with it
   \param sig struct to free
   \sa ops_create_signature_new()
*/
void ops_create_signature_delete(ops_create_signature_t *sig)
    {
    ops_create_info_delete(sig->info);
    sig->info=NULL;
    free(sig);
    }

static unsigned char prefix_md5[]={ 0x30,0x20,0x30,0x0C,0x06,0x08,0x2A,0x86,
												0x48,0x86,0xF7,0x0D,0x02,0x05,0x05,0x00,
												0x04,0x10 };

static unsigned char prefix_sha1[]={ 0x30,0x21,0x30,0x09,0x06,0x05,0x2b,0x0E,
											    0x03,0x02,0x1A,0x05,0x00,0x04,0x14 };

static unsigned char prefix_sha224[]={ 0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
							                  0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04, 0x05,
													0x00, 0x04, 0x1C };

static unsigned char prefix_sha256[]={ 0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,
                                       0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,
                                       0x00,0x04,0x20 };

static unsigned char prefix_sha384[]={ 0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
							                  0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, 0x05,
													0x00, 0x04, 0x30 };

static unsigned char prefix_sha512[]={ 0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
							                  0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05,
													0x00, 0x04, 0x40 };

static unsigned char prefix_ripemd[]={ 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2B, 0x24,
							                  0x03, 0x02, 0x01, 0x05, 0x00, 0x04, 0x14 };
/**
   \ingroup Core_Create
   implementation of EMSA-PKCS1-v1_5, as defined in OpenPGP RFC
   \param M
   \param mLen
   \param hash_alg Hash algorithm to use
   \param EM
   \return ops_true if OK; else ops_false
*/
ops_boolean_t encode_hash_buf(const unsigned char *M, size_t mLen,
			      const ops_hash_algorithm_t hash_alg,
			      unsigned char* EM)
{
	// implementation of EMSA-PKCS1-v1_5, as defined in OpenPGP RFC

	unsigned i;
	int n;
	ops_hash_t hash;
	int hash_sz=0;
	//    int encoded_hash_sz=0;
	int prefix_sz=0;
	unsigned padding_sz=0;
	unsigned encoded_msg_sz=0;
	unsigned char* prefix=NULL;

	if(hash_alg != OPS_HASH_SHA1)
	{
		fprintf(stderr,"encode_hash_buf: unsupported hash algorithm %x. Sorry.",hash_alg) ;
		return ops_false ;
	}

	// 1. Apply hash function to M

	ops_hash_any(&hash, hash_alg);
	hash.init(&hash);
	hash.add(&hash, M, mLen);

	// \todo combine with rsa_sign

	// 2. Get hash prefix

	switch(hash_alg)
	{
		case OPS_HASH_SHA1:
			prefix=prefix_sha1; 
			prefix_sz=sizeof prefix_sha1;
			hash_sz=OPS_SHA1_HASH_SIZE;
			//        encoded_hash_sz=hash_sz+prefix_sz;
			// \todo why is Ben using a PS size of 90 in rsa_sign?
			// (keysize-hashsize-1-2)
			padding_sz=90;
			break;

		default:
			return ops_false ;	// according to the test at start, this should never happen, so no error handling is necessary.
	}

	// \todo 3. Test for len being too short

	// 4 and 5. Generate PS and EM

	EM[0]=0x00;
	EM[1]=0x01;

	for (i=0; i<padding_sz; i++)
		EM[2+i]=0xFF;

	i+=2;

	EM[i++]=0x00;

	memcpy(&EM[i], prefix, prefix_sz);
	i+=prefix_sz;

	// finally, write out hashed result

	n=hash.finish(&hash, &EM[i]);
	if(n != hash_sz)
	{
		fprintf(stderr,"encode_hash_buf(): Error which hashing data. n=%d != hash_sz=%d",n,hash_sz) ;
		return ops_false ;
	}

	encoded_msg_sz=i+hash_sz-1;

	// \todo test n for OK response?

	if (debug)
	{
		fprintf(stderr, "Encoded Message: \n");
		for (i=0; i<encoded_msg_sz; i++)
			fprintf(stderr, "%2x ", EM[i]);
		fprintf(stderr, "\n");
	}

	return ops_true;
}

// XXX: both this and verify would be clearer if the signature were
// treated as an MPI.
static ops_boolean_t rsa_sign(ops_hash_t *hash, const ops_rsa_public_key_t *rsa,
		     const ops_rsa_secret_key_t *srsa, ops_create_info_t *opt)
{
	unsigned char hashbuf[8192];
	unsigned char sigbuf[8192];
	unsigned keysize;
	unsigned hashsize;
	unsigned n;
	unsigned t;
	BIGNUM *bn;

	int plen ;
	unsigned int hash_length ;
	unsigned char *prefix ;

	switch(hash->algorithm)
	{
	case OPS_HASH_SHA1: 	hashsize=OPS_SHA1_HASH_SIZE+sizeof prefix_sha1;
							hash_length=OPS_SHA1_HASH_SIZE ;
							prefix = prefix_sha1;
							plen = sizeof prefix_sha1 ;
							break ;

	case OPS_HASH_SHA224: 	hashsize=OPS_SHA224_HASH_SIZE+sizeof prefix_sha224;
							hash_length=OPS_SHA224_HASH_SIZE ;
							prefix = prefix_sha224;
							plen = sizeof prefix_sha224 ;
							break ;

	case OPS_HASH_SHA256: 	hashsize=OPS_SHA256_HASH_SIZE+sizeof prefix_sha256;
							hash_length=OPS_SHA256_HASH_SIZE ;
							prefix = prefix_sha256;
							plen = sizeof prefix_sha256 ;
							break ;

	case OPS_HASH_SHA384: 	hashsize=OPS_SHA384_HASH_SIZE+sizeof prefix_sha384;
							hash_length=OPS_SHA384_HASH_SIZE ;
							prefix = prefix_sha384;
							plen = sizeof prefix_sha384 ;
							break ;

	case OPS_HASH_SHA512: 	hashsize=OPS_SHA512_HASH_SIZE+sizeof prefix_sha512;
							hash_length=OPS_SHA512_HASH_SIZE ;
							prefix = prefix_sha512;
							plen = sizeof prefix_sha512 ;
							break ;

	case OPS_HASH_MD5: 		fprintf(stderr,"(insecure) MD5+RSA signatures not supported in RSA sign") ;
							return ops_false ;

	default: 				fprintf(stderr,"Hash algorithm %d not supported in RSA sign",hash->algorithm) ;
							return ops_false ;
	}
	// XXX: we assume hash is sha-1 for now

	keysize=BN_num_bytes(rsa->n);

	if(keysize > sizeof hashbuf)
	{
		fprintf(stderr,"Keysize too large. limit is %lu, size was %u",sizeof(hashbuf), keysize) ;
		return ops_false ;
	}
	if(10+hashsize > keysize)
	{
		fprintf(stderr,"10+hashsize > keysize. Can't sign!") ;
		return ops_false ;
	}

	hashbuf[0]=0;
	hashbuf[1]=1;
	if (debug)
		printf("rsa_sign: PS is %d\n", keysize-hashsize-1-2);
	for(n=2 ; n < keysize-hashsize-1 ; ++n)
		hashbuf[n]=0xff;
	hashbuf[n++]=0;

	memcpy(&hashbuf[n], prefix, plen);
	n+=plen;

	t=hash->finish(hash, &hashbuf[n]);

	if(t != hash_length)
	{
		fprintf(stderr,"Wrong hash size. Should be 20! can't sign.") ;
		return ops_false ;
	}

	ops_write(&hashbuf[n], 2, opt);

	n+=t;

	if(n != keysize)
	{
		fprintf(stderr,"Size error in hashed data. can't sign.") ;
		return ops_false ;
	}

	t=ops_rsa_private_encrypt(sigbuf, hashbuf, keysize, srsa, rsa);
	bn=BN_bin2bn(sigbuf, t, NULL);
	ops_write_mpi(bn, opt);
	BN_free(bn);

	return ops_true ;
}

static ops_boolean_t dsa_sign(ops_hash_t *hash, const ops_dsa_public_key_t *dsa,
                     const ops_dsa_secret_key_t *sdsa, ops_create_info_t *cinfo)
{
	unsigned char hashbuf[8192];
	unsigned hashsize;
	unsigned t;

	// hashsize must be "equal in size to the number of bits of q, 
	// the group generated by the DSA key's generator value
	// 160/8 = 20

	hashsize=20;

	// finalise hash
	t=hash->finish(hash, &hashbuf[0]);

	if(t != 20)
	{
		fprintf(stderr,"Wrong hash size. Should be 20! can't sign.") ;
		return ops_false ;
	}

	ops_write(&hashbuf[0], 2, cinfo);

	// write signature to buf
	DSA_SIG* dsasig;
	dsasig=ops_dsa_sign(hashbuf, hashsize, sdsa, dsa);

	// convert and write the sig out to memory
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	ops_write_mpi(dsasig->r, cinfo);
	ops_write_mpi(dsasig->s, cinfo);
#else
    const BIGNUM *rr=NULL,*ss=NULL ;

    DSA_SIG_get0(dsasig,&rr,&ss) ;

	ops_write_mpi(rr, cinfo);
	ops_write_mpi(ss, cinfo);
#endif

	DSA_SIG_free(dsasig);

	return ops_true ;
}

static ops_boolean_t rsa_verify(ops_hash_algorithm_t type,
				const unsigned char *hash, size_t hash_length,
				const ops_rsa_signature_t *sig,
				const ops_rsa_public_key_t *rsa)
{
	unsigned char sigbuf[8192];
	unsigned char hashbuf_from_sig[8192];
	unsigned n;
	unsigned keysize;
	unsigned char *prefix;
	int plen;

	keysize=BN_num_bytes(rsa->n);
	/* RSA key can't be bigger than 65535 bits, so... */

	if(keysize > sizeof hashbuf_from_sig)
	{
		fprintf(stderr,"Can't verify signature: keysize too big.") ;
		return ops_false ;
	}
	if((unsigned)BN_num_bits(sig->sig) > 8*sizeof sigbuf)
	{
		fprintf(stderr,"Can't verify signature: signature too big.") ;
		return ops_false ;
	}
	BN_bn2bin(sig->sig, sigbuf);

	n=ops_rsa_public_decrypt(hashbuf_from_sig, sigbuf, BN_num_bytes(sig->sig), rsa);
	int debug_len_decrypted=n;

	if(n != keysize) // obviously, this includes error returns
		return ops_false;

	// XXX: why is there a leading 0? The first byte should be 1...
	// XXX: because the decrypt should use keysize and not sigsize?
	if(hashbuf_from_sig[0] != 0 || hashbuf_from_sig[1] != 1)
		return ops_false;

	switch(type)
	{
		case OPS_HASH_MD5		: prefix=prefix_md5		; plen=sizeof prefix_md5; break;
		case OPS_HASH_SHA1	: prefix=prefix_sha1		; plen=sizeof prefix_sha1; break;
		case OPS_HASH_SHA224 : prefix=prefix_sha224	; plen=sizeof prefix_sha224; break;
		case OPS_HASH_SHA256 : prefix=prefix_sha256	; plen=sizeof prefix_sha256; break;
		case OPS_HASH_SHA384 : prefix=prefix_sha384	; plen=sizeof prefix_sha384; break;
		case OPS_HASH_SHA512 : prefix=prefix_sha512	; plen=sizeof prefix_sha512; break;
		case OPS_HASH_RIPEMD : prefix=prefix_ripemd	; plen=sizeof prefix_ripemd; break;

		default: 
									 fprintf(stderr,"Warning: unhandled hash type in signature verification code: %d\n",type) ;
									 return ops_false ;
	}

	if(keysize-plen-hash_length < 10)
		return ops_false;

	for(n=2 ; n < keysize-plen-hash_length-1 ; ++n)
		if(hashbuf_from_sig[n] != 0xff)
			return ops_false;

	if(hashbuf_from_sig[n++] != 0)
		return ops_false;

	if (debug)
	{
		int zz;

		printf("\n");
		printf("hashbuf_from_sig\n");
		for (zz=0; zz<debug_len_decrypted; zz++)
			printf("%02x ", hashbuf_from_sig[n+zz]);
		printf("\n");
		printf("prefix\n");
		for (zz=0; zz<plen; zz++)
			printf("%02x ", prefix[zz]);
		printf("\n");

		printf("\n");
		printf("hash from sig\n");
		unsigned uu;
		for (uu=0; uu<hash_length; uu++)
			printf("%02x ", hashbuf_from_sig[n+plen+uu]);
		printf("\n");
		printf("hash passed in (should match hash from sig)\n");
		for (uu=0; uu<hash_length; uu++)
			printf("%02x ", hash[uu]);
		printf("\n");
	}
	if(memcmp(&hashbuf_from_sig[n], prefix, plen)
			|| memcmp(&hashbuf_from_sig[n+plen], hash, hash_length))
		return ops_false;

	return ops_true;
}

static void hash_add_key(ops_hash_t *hash, const ops_public_key_t *key)
    {
    ops_memory_t *mem=ops_memory_new();
    size_t l;

    ops_build_public_key(mem, key, ops_false);

    l=ops_memory_get_length(mem);
    ops_hash_add_int(hash, 0x99, 1);
    ops_hash_add_int(hash, l, 2);
    hash->add(hash, ops_memory_get_data(mem), l);

    ops_memory_free(mem);
    }

static void initialise_hash(ops_hash_t *hash, const ops_signature_t *sig)
    {
    ops_hash_any(hash, sig->info.hash_algorithm);
    hash->init(hash);
    }

static void init_key_signature(ops_hash_t *hash, const ops_signature_t *sig,
			       const ops_public_key_t *key)
    {
    initialise_hash(hash, sig);
    hash_add_key(hash, key);
    }

static void hash_add_trailer(ops_hash_t *hash, const ops_signature_t *sig,
			     const unsigned char *raw_packet)
    {
    if(sig->info.version == OPS_V4)
	{
	if(raw_packet)
	    hash->add(hash, raw_packet+sig->v4_hashed_data_start,
		      sig->info.v4_hashed_data_length);
	ops_hash_add_int(hash, sig->info.version, 1);
	ops_hash_add_int(hash, 0xff, 1);
	ops_hash_add_int(hash, sig->info.v4_hashed_data_length, 4);
	}
    else
	{
	ops_hash_add_int(hash, sig->info.type, 1);
	ops_hash_add_int(hash, sig->info.creation_time, 4);
	}
    }

/**
   \ingroup Core_Signature
   \brief Checks a signature
   \param hash Signature Hash to be checked
   \param length Signature Length
   \param sig The Signature to be checked
   \param signer The signer's public key
   \return ops_true if good; else ops_false
*/
ops_boolean_t ops_check_signature(const unsigned char *hash, unsigned length,
				     const ops_signature_t *sig,
				     const ops_public_key_t *signer)
{
	ops_boolean_t ret;

	/*
		printf(" hash=");
	//    hashout[0]=0;
	hexdump(hash,length);
	 */

	switch(sig->info.key_algorithm)
	{
		case OPS_PKA_DSA:
			ret=ops_dsa_verify(hash, length, &sig->info.signature.dsa,
					&signer->key.dsa);
			/*		fprintf(stderr,"Cannot verify DSA signature. skipping.\n") ;
					ret = ops_false ; */
			break;

		case OPS_PKA_RSA:
			ret=rsa_verify(sig->info.hash_algorithm, hash, length,
					&sig->info.signature.rsa, &signer->key.rsa);
			break;

		default:
			fprintf(stderr,"Cannot verify signature. Unknown key signing algorithm %d. skipping.\n",sig->info.key_algorithm) ;
			ret = ops_false ;

	}

	return ret;
}

static ops_boolean_t hash_and_check_signature(ops_hash_t *hash,
					      const ops_signature_t *sig,
					      const ops_public_key_t *signer)
    {
    int n;
    unsigned char hashout[OPS_MAX_HASH_SIZE];

    n=hash->finish(hash, hashout);

    return ops_check_signature(hashout, n, sig, signer);
    }

static ops_boolean_t finalise_signature(ops_hash_t *hash,
					const ops_signature_t *sig,
					const ops_public_key_t *signer,
					const unsigned char *raw_packet)
    {
    hash_add_trailer(hash, sig, raw_packet);
    return hash_and_check_signature(hash, sig, signer);
    }

/**
 * \ingroup Core_Signature
 *
 * \brief Verify a certification signature.
 *
 * \param key The public key that was signed.
 * \param id The user ID that was signed
 * \param sig The signature.
 * \param signer The public key of the signer.
 * \param raw_packet The raw signature packet.
 * \return ops_true if OK; else ops_false
 */
ops_boolean_t
ops_check_user_id_certification_signature(const ops_public_key_t *key,
					  const ops_user_id_t *id,
					  const ops_signature_t *sig,
					  const ops_public_key_t *signer,
					  const unsigned char *raw_packet)
    {
    ops_hash_t hash;
    size_t user_id_len=strlen((char *)id->user_id);

    init_key_signature(&hash, sig, key);

    if(sig->info.version == OPS_V4)
	{
	ops_hash_add_int(&hash, 0xb4, 1);
	ops_hash_add_int(&hash, user_id_len, 4);
	}
    hash.add(&hash, id->user_id, user_id_len);

    return finalise_signature(&hash, sig, signer, raw_packet);
    }

/**
 * \ingroup Core_Signature
 *
 * Verify a certification signature.
 *
 * \param key The public key that was signed.
 * \param attribute The user attribute that was signed
 * \param sig The signature.
 * \param signer The public key of the signer.
 * \param raw_packet The raw signature packet.
 * \return ops_true if OK; else ops_false
 */
ops_boolean_t
ops_check_user_attribute_certification_signature(const ops_public_key_t *key,
					  const ops_user_attribute_t *attribute,
					  const ops_signature_t *sig,
					  const ops_public_key_t *signer,
					  const unsigned char *raw_packet)
    {
    ops_hash_t hash;

    init_key_signature(&hash, sig, key);

    if(sig->info.version == OPS_V4)
	{
	ops_hash_add_int(&hash, 0xd1, 1);
	ops_hash_add_int(&hash, attribute->data.len, 4);
	}
    hash.add(&hash, attribute->data.contents, attribute->data.len);

    return finalise_signature(&hash, sig, signer, raw_packet);
    }

/**
 * \ingroup Core_Signature
 *
 * Verify a subkey signature.
 *
 * \param key The public key whose subkey was signed.
 * \param subkey The subkey of the public key that was signed.
 * \param sig The signature.
 * \param signer The public key of the signer.
 * \param raw_packet The raw signature packet.
 * \return ops_true if OK; else ops_false
 */
ops_boolean_t
ops_check_subkey_signature(const ops_public_key_t *key,
			   const ops_public_key_t *subkey,
			   const ops_signature_t *sig,
			   const ops_public_key_t *signer,
			   const unsigned char *raw_packet)
    {
    ops_hash_t hash;

    init_key_signature(&hash, sig, key);
    hash_add_key(&hash, subkey);

    return finalise_signature(&hash, sig, signer, raw_packet);
    }

/**
 * \ingroup Core_Signature
 *
 * Verify a direct signature.
 *
 * \param key The public key which was signed.
 * \param sig The signature.
 * \param signer The public key of the signer.
 * \param raw_packet The raw signature packet.
 * \return ops_true if OK; else ops_false
 */
ops_boolean_t
ops_check_direct_signature(const ops_public_key_t *key,
			   const ops_signature_t *sig,
			   const ops_public_key_t *signer,
			   const unsigned char *raw_packet)
    {
    ops_hash_t hash;

    init_key_signature(&hash, sig, key);
    return finalise_signature(&hash, sig, signer, raw_packet);
    }

/**
 * \ingroup Core_Signature
 *
 * Verify a signature on a hash (the hash will have already been fed
 * the material that was being signed, for example signed cleartext).
 *
 * \param hash A hash structure of appropriate type that has been fed
 * the material to be signed. This MUST NOT have been finalised.
 * \param sig The signature to be verified.
 * \param signer The public key of the signer.
 * \return ops_true if OK; else ops_false
 */
ops_boolean_t
ops_check_hash_signature(ops_hash_t *hash, const ops_signature_t *sig,
			 const ops_public_key_t *signer)
    {
    if(sig->info.hash_algorithm != hash->algorithm)
	return ops_false;

    return finalise_signature(hash, sig, signer, NULL);
    }

static void start_signature_in_mem(ops_create_signature_t *sig)
    {
    // since this has subpackets and stuff, we have to buffer the whole
    // thing to get counts before writing.
    sig->mem=ops_memory_new();
    ops_memory_init(sig->mem, 100);
    ops_writer_set_memory(sig->info, sig->mem);

    // write nearly up to the first subpacket
    ops_write_scalar(sig->sig.info.version, 1, sig->info);
    ops_write_scalar(sig->sig.info.type, 1, sig->info);
    ops_write_scalar(sig->sig.info.key_algorithm, 1, sig->info);
    ops_write_scalar(sig->sig.info.hash_algorithm, 1, sig->info);

    // dummy hashed subpacket count
    sig->hashed_count_offset=ops_memory_get_length(sig->mem);
    ops_write_scalar(0, 2, sig->info);
    }    

/**
 * \ingroup Core_Signature
 *
 * ops_signature_start() creates a V4 public key signature with a SHA1 hash.
 * 
 * \param sig The signature structure to initialise
 * \param key The public key to be signed
 * \param id The user ID being bound to the key
 * \param type Signature type
 */
void ops_signature_start_key_signature(ops_create_signature_t *sig,
				       const ops_public_key_t *key,
				       const ops_user_id_t *id,
				       ops_sig_type_t type)
    {
    sig->info=ops_create_info_new();

    // XXX: refactor with check (in several ways - check should probably
    // use the buffered writer to construct packets (done), and also should
    // share code for hash calculation)
    sig->sig.info.version=OPS_V4;
    sig->sig.info.hash_algorithm=OPS_HASH_SHA1;
    sig->sig.info.key_algorithm=key->algorithm;
    sig->sig.info.type=type;

    sig->hashed_data_length=-1;

    init_key_signature(&sig->hash, &sig->sig, key);

    ops_hash_add_int(&sig->hash, 0xb4, 1);
    ops_hash_add_int(&sig->hash, strlen((char *)id->user_id), 4);
    sig->hash.add(&sig->hash, id->user_id, strlen((char *)id->user_id));

    start_signature_in_mem(sig);
    }

/**
 * \ingroup Core_Signature
 *
 * Create a V4 public key signature over some cleartext.
 * 
 * \param sig The signature structure to initialise
 * \param id
 * \param type
 * \todo Expand description. Allow other hashes.
 */

static void ops_signature_start_signature(ops_create_signature_t *sig,
					  const ops_secret_key_t *key,
					  const ops_hash_algorithm_t hash,
					  const ops_sig_type_t type)
    {
    sig->info=ops_create_info_new();

    // XXX: refactor with check (in several ways - check should probably
    // use the buffered writer to construct packets (done), and also should
    // share code for hash calculation)
    sig->sig.info.version=OPS_V4;
    sig->sig.info.key_algorithm=key->public_key.algorithm;
    sig->sig.info.hash_algorithm=hash;
    sig->sig.info.type=type;

    sig->hashed_data_length=-1;

    if (debug)
        { fprintf(stderr, "initialising hash for sig in mem\n"); }
    initialise_hash(&sig->hash, &sig->sig);
    start_signature_in_mem(sig);
    }

/**
 * \ingroup Core_Signature
 * \brief Setup to start a cleartext's signature
 */
void ops_signature_start_cleartext_signature(ops_create_signature_t *sig,
					     const ops_secret_key_t *key,
					     const ops_hash_algorithm_t hash,
					     const ops_sig_type_t type)
    {
    ops_signature_start_signature(sig, key, hash, type);
    }

/**
 * \ingroup Core_Signature
 * \brief Setup to start a message's signature
 */
void ops_signature_start_message_signature(ops_create_signature_t *sig,
					   const ops_secret_key_t *key,
					   const ops_hash_algorithm_t hash,
					   const ops_sig_type_t type)
    {
    ops_signature_start_signature(sig, key, hash, type);
    }

/**
 * \ingroup Core_Signature
 *
 * Add plaintext data to a signature-to-be.
 *
 * \param sig The signature-to-be.
 * \param buf The plaintext data.
 * \param length The amount of plaintext data.
 */
void ops_signature_add_data(ops_create_signature_t *sig, const void *buf,
			    size_t length)
    {
    if (debug)
        { fprintf(stderr, "ops_signature_add_data adds to hash\n"); }
    sig->hash.add(&sig->hash, buf, length);
    }

/**
 * \ingroup Core_Signature
 *
 * Mark the end of the hashed subpackets in the signature
 *
 * \param sig
 */

ops_boolean_t ops_signature_hashed_subpackets_end(ops_create_signature_t *sig)
    {
    sig->hashed_data_length=ops_memory_get_length(sig->mem)
	-sig->hashed_count_offset-2;
    ops_memory_place_int(sig->mem, sig->hashed_count_offset,
			 sig->hashed_data_length, 2);
    // dummy unhashed subpacket count
    sig->unhashed_count_offset=ops_memory_get_length(sig->mem);
    return ops_write_scalar(0, 2, sig->info);
    }

/**
 * \ingroup Core_Signature
 *
 * Write out a signature
 *
 * \param sig
 * \param key
 * \param skey
 * \param info
 *
 */

ops_boolean_t ops_write_signature(ops_create_signature_t *sig,
				  const ops_public_key_t *key,
				  const ops_secret_key_t *skey,
				  ops_create_info_t *info)
{
	ops_boolean_t rtn=ops_false;
	size_t l=ops_memory_get_length(sig->mem);

	// check key not decrypted
	switch (skey->public_key.algorithm)
	{
		case OPS_PKA_RSA:
		case OPS_PKA_RSA_ENCRYPT_ONLY:
		case OPS_PKA_RSA_SIGN_ONLY:
			if(skey->key.rsa.d == NULL)
			{
				fprintf(stderr, "Malformed secret key when signing (rsa.d = 0). Can't sign.\n") ;
				return ops_false ;
			}
			break;

		case OPS_PKA_DSA:
			if(skey->key.dsa.x == NULL)
			{
				fprintf(stderr, "Malformed secret key when signing (dsa.x = 0). Can't sign.\n") ;
				return ops_false ;
			}
			break;

		default:
			fprintf(stderr, "Unsupported algorithm %d when signing. Sorry.\n", skey->public_key.algorithm);
			return ops_false ;
	}

	if(sig->hashed_data_length == (unsigned)-1)
	{
		fprintf(stderr, "Hashed data not initialized properly when signing. Sorry.\n") ;
		return ops_false ;
	}

	ops_memory_place_int(sig->mem, sig->unhashed_count_offset,
			l-sig->unhashed_count_offset-2, 2);

	// add the packet from version number to end of hashed subpackets

	if (debug)
	{ fprintf(stderr, "--- Adding packet to hash from version number to"
			" hashed subpkts\n"); }

	sig->hash.add(&sig->hash, ops_memory_get_data(sig->mem),
			sig->unhashed_count_offset);

	// add final trailer
	ops_hash_add_int(&sig->hash, sig->sig.info.version, 1);
	ops_hash_add_int(&sig->hash, 0xff, 1);
	// +6 for version, type, pk alg, hash alg, hashed subpacket length
	ops_hash_add_int(&sig->hash, sig->hashed_data_length+6, 4);

	if (debug)
	{ fprintf(stderr, "--- Finished adding packet to hash from version"
			" number to hashed subpkts\n"); }

	// XXX: technically, we could figure out how big the signature is
	// and write it directly to the output instead of via memory.
	switch(skey->public_key.algorithm)
	{
		case OPS_PKA_RSA:
		case OPS_PKA_RSA_ENCRYPT_ONLY:
		case OPS_PKA_RSA_SIGN_ONLY:
			if(!rsa_sign(&sig->hash, &key->key.rsa, &skey->key.rsa, sig->info))
			{
				fprintf(stderr, "error in rsa_sign. Can't produce signature\n") ;
				return ops_false;
			}
			break;

		case OPS_PKA_DSA:
			if(!dsa_sign(&sig->hash, &key->key.dsa, &skey->key.dsa, sig->info))
			{
				fprintf(stderr, "error in dsa_sign. Can't produce signature\n") ;
				return ops_false;
			}
			break;

		default:
			fprintf(stderr, "Unsupported algorithm %d in signature\n", skey->public_key.algorithm);
			return ops_false ;
	}

	rtn=ops_write_ptag(OPS_PTAG_CT_SIGNATURE, info);
	if (rtn)
	{
		l=ops_memory_get_length(sig->mem);
		rtn = ops_write_length(l, info)
			&& ops_write(ops_memory_get_data(sig->mem), l, info);
	}

	ops_memory_free(sig->mem);

	if (!rtn)
		OPS_ERROR(&info->errors, OPS_E_W, "Cannot write signature");
	return rtn;
}

/**
 * \ingroup Core_Signature
 * 
 * ops_signature_add_creation_time() adds a creation time to the signature.
 * 
 * \param sig
 * \param when
 */
ops_boolean_t ops_signature_add_creation_time(ops_create_signature_t *sig,
					      time_t when)
    {
    return ops_write_ss_header(5, OPS_PTAG_SS_CREATION_TIME, sig->info)
        && ops_write_scalar(when, 4, sig->info);
    }

/**
 * \ingroup Core_Signature
 *
 * Adds issuer's key ID to the signature
 *
 * \param sig
 * \param keyid
 */

ops_boolean_t
ops_signature_add_issuer_key_id(ops_create_signature_t *sig,
				const unsigned char keyid[OPS_KEY_ID_SIZE])
    {
    return ops_write_ss_header(OPS_KEY_ID_SIZE+1, OPS_PTAG_SS_ISSUER_KEY_ID,
			       sig->info)
        && ops_write(keyid, OPS_KEY_ID_SIZE, sig->info);
    }

/**
 * \ingroup Core_Signature
 *
 * Adds primary user ID to the signature
 *
 * \param sig
 * \param primary
 */
void ops_signature_add_primary_user_id(ops_create_signature_t *sig,
				       ops_boolean_t primary)
    {
    ops_write_ss_header(2, OPS_PTAG_SS_PRIMARY_USER_ID, sig->info);
    ops_write_scalar(primary, 1, sig->info);
    }

/**
 * \ingroup Core_Signature
 *
 * Get the hash structure in use for the signature.
 *
 * \param sig The signature structure.
 * \return The hash structure.
 */
ops_hash_t *ops_signature_get_hash(ops_create_signature_t *sig)
    { return &sig->hash; }

static int open_output_file(ops_create_info_t **cinfo,
			    const char* input_filename,
			    const char* output_filename,
			    const ops_boolean_t use_armour,
			    const ops_boolean_t overwrite)
    {
    int fd_out;

    // setup output file

    if (output_filename)
        fd_out=ops_setup_file_write(cinfo, output_filename, overwrite);
    else
        {
        char *myfilename=NULL;
        unsigned filenamelen=strlen(input_filename)+4+1;
        myfilename=ops_mallocz(filenamelen);
        if (use_armour)
            snprintf(myfilename, filenamelen, "%s.asc", input_filename);
        else
            snprintf(myfilename, filenamelen, "%s.gpg", input_filename);
        fd_out=ops_setup_file_write(cinfo,  myfilename,  overwrite);
        free(myfilename);
        } 

    return fd_out;
    }

/**
   \ingroup HighLevel_Sign
   \brief Sign a file with a Cleartext Signature
   \param input_filename Name of file to be signed
   \param output_filename Filename to be created. If NULL, filename will be constructed from the input_filename.
   \param skey Secret Key to sign with
   \param overwrite Allow output file to be overwritten, if set
   \return ops_true if OK, else ops_false

   Example code:
   \code
   void example(const ops_secret_key_t *skey, ops_boolean_t overwrite)
   {
   if (ops_sign_file_as_cleartext("mytestfile.txt",NULL,skey,overwrite)==ops_true)
       printf("OK");
   else
       printf("ERR");
   }
   \endcode
*/
ops_boolean_t ops_sign_file_as_cleartext(const char* input_filename,
					 const char* output_filename,
					 const ops_secret_key_t *skey,
					 const ops_boolean_t overwrite)
{
	// \todo allow choice of hash algorithams
	// enforce use of SHA1 for now

	unsigned char keyid[OPS_KEY_ID_SIZE];
	ops_create_signature_t *sig=NULL;

	int fd_in=0;
	int fd_out=0;
	ops_create_info_t *cinfo=NULL;
	unsigned char buf[MAXBUF];
	//int flags=0;
	ops_boolean_t rtn=ops_false;
	ops_boolean_t use_armour=ops_true;

	// open file to sign

	fd_in=ops_open(input_filename, O_RDONLY | O_BINARY, 0);

	if(fd_in < 0)
	{
		return ops_false;
	}

	// set up output file

	fd_out=open_output_file(&cinfo, input_filename, output_filename, use_armour,
			overwrite);

	if (fd_out < 0)
	{
		close(fd_in);
		return ops_false;
	}

	// set up signature
	sig=ops_create_signature_new();
	if (!sig)
	{
		close (fd_in);
		ops_teardown_file_write(cinfo, fd_out);
		return ops_false;
	}

	// \todo could add more error detection here
	ops_signature_start_cleartext_signature(sig, skey, OPS_HASH_SHA1, OPS_SIG_BINARY);

	if (!ops_writer_push_clearsigned(cinfo, sig))
		return ops_false;

	// Do the signing

	for (;;)
	{
		int n=0;

		n=read(fd_in, buf, sizeof(buf));
		if (!n)
			break;

		if(n < 0)
		{
			fprintf(stderr, "Read error in ops_sign_file_as_cleartext\n");
			close(fd_in) ;
			ops_teardown_file_write(cinfo, fd_out);
			return ops_false ;
		}
		ops_write(buf, n, cinfo);
	}
	close(fd_in);

	// add signature with subpackets:
	// - creation time
	// - key id
	rtn = ops_writer_switch_to_armoured_signature(cinfo)
		&& ops_signature_add_creation_time(sig, time(NULL));

	if (!rtn)
	{
		ops_teardown_file_write(cinfo, fd_out);
		return ops_false;
	}

	ops_keyid(keyid, &skey->public_key);

	rtn = ops_signature_add_issuer_key_id(sig, keyid)
		&& ops_signature_hashed_subpackets_end(sig)
		&& ops_write_signature(sig, &skey->public_key, skey, cinfo);

	if (!rtn)
		OPS_ERROR(&cinfo->errors, OPS_E_W, "Cannot sign file as cleartext");

    // we can't get errors in cinfo->errors while closing
    // because ops_teardown_file_write frees everything
    // it seems that writer finalisers do not report errors anyway
    // and ops_teardown_file_write does not have an return value, so we could not bubble something up
    ops_teardown_file_write(cinfo, fd_out);

	return rtn;
}


/** 
 * \ingroup HighLevel_Sign
 * \brief Sign a buffer with a Cleartext signature
 * \param cleartext Text to be signed
 * \param len Length of text
 * \param signed_cleartext ops_memory_t struct in which to write the signed cleartext
 * \param skey Secret key with which to sign the cleartext
 * \return ops_true if OK; else ops_false

 * \note It is the calling function's responsibility to free signed_cleartext
 * \note signed_cleartext should be a NULL pointer when passed in 

 Example code:
 \code
 void example(const ops_secret_key_t *skey)
 {
   ops_memory_t* mem=NULL;
   const char* buf="Some example text";
   size_t len=strlen(buf);
   if (ops_sign_buf_as_cleartext(buf,len, &mem, skey)==ops_true)
     printf("OK");
   else
     printf("ERR");
   // free signed cleartext after use
   ops_memory_free(mem);
 }
 \endcode
 */
ops_boolean_t ops_sign_buf_as_cleartext(const char* cleartext, const size_t len,
					ops_memory_t** signed_cleartext,
					const ops_secret_key_t *skey)
{
	ops_boolean_t rtn=ops_false;

	// \todo allow choice of hash algorithams
	// enforce use of SHA1 for now

	unsigned char keyid[OPS_KEY_ID_SIZE];
	ops_create_signature_t *sig=NULL;

	ops_create_info_t *cinfo=NULL;

	if(*signed_cleartext != NULL)
	{
		fprintf(stderr,"ops_sign_buf_as_cleartext: error. Variable signed_cleartext should point to NULL.\n") ;
		return ops_false ;
	}

	// set up signature
	sig=ops_create_signature_new();
	if (!sig)
		return ops_false;

	// \todo could add more error detection here
	ops_signature_start_cleartext_signature(sig, skey, OPS_HASH_SHA1, OPS_SIG_BINARY);

	// set up output file
	ops_setup_memory_write(&cinfo, signed_cleartext, len);

	// Do the signing
	// add signature with subpackets:
	// - creation time
	// - key id
	rtn = ops_writer_push_clearsigned(cinfo, sig)
		&& ops_write(cleartext, len, cinfo)
		&& ops_writer_switch_to_armoured_signature(cinfo)
		&& ops_signature_add_creation_time(sig, time(NULL));

	if (!rtn)
		return ops_false;

	ops_keyid(keyid, &skey->public_key);

	rtn = ops_signature_add_issuer_key_id(sig, keyid)
		&& ops_signature_hashed_subpackets_end(sig)
		&& ops_write_signature(sig, &skey->public_key, skey, cinfo)
		&& ops_writer_close(cinfo);

	// Note: the calling function must free signed_cleartext
	ops_create_info_delete(cinfo);

	return rtn;
}

/**
\ingroup HighLevel_Sign
\brief Sign a file
\param input_filename Input filename
\param output_filename Output filename. If NULL, a name is constructed from the input filename.
\param skey Secret Key to use for signing
\param use_armour Write armoured text, if set.
\param overwrite May overwrite existing file, if set.
\return ops_true if OK; else ops_false;

Example code:
\code
void example(const ops_secret_key_t *skey)
{
  const char* filename="mytestfile";
  const ops_boolean_t use_armour=ops_false;
  const ops_boolean_t overwrite=ops_false;
  if (ops_sign_file(filename, NULL, skey, use_armour, overwrite)==ops_true)
    printf("OK");
  else
    printf("ERR");  
}
\endcode
*/
ops_boolean_t ops_sign_file(const char* input_filename,
			    const char* output_filename,
			    const ops_secret_key_t *skey,
			    const ops_boolean_t use_armour,
			    const ops_boolean_t overwrite)
    {
    // \todo allow choice of hash algorithams
    // enforce use of SHA1 for now

    unsigned char keyid[OPS_KEY_ID_SIZE];
    ops_create_signature_t *sig=NULL;

    int fd_out=0;
    ops_create_info_t *cinfo=NULL;

    ops_hash_algorithm_t hash_alg=OPS_HASH_SHA1;
    ops_sig_type_t sig_type=OPS_SIG_BINARY;

    ops_memory_t* mem_buf=NULL;
    ops_hash_t* hash=NULL;

    // read input file into buf

    int errnum;
    mem_buf=ops_write_mem_from_file(input_filename, &errnum);
    if (errnum)
        return ops_false;

    // setup output file

    fd_out=open_output_file(&cinfo, input_filename, output_filename, use_armour,
			    overwrite);

    if (fd_out < 0)
        {
        ops_memory_free(mem_buf);
        return ops_false;
        }

    // set up signature
    sig=ops_create_signature_new();
    ops_signature_start_message_signature(sig, skey, hash_alg, sig_type);

    //  set armoured/not armoured here
    if (use_armour)
        ops_writer_push_armoured_message(cinfo);

    if (debug)
        { fprintf(stderr, "** Writing out one pass sig\n"); } 

    // write one_pass_sig
    ops_write_one_pass_sig(skey, hash_alg, sig_type, cinfo);

    // hash file contents
    hash=ops_signature_get_hash(sig);
    hash->add(hash, ops_memory_get_data(mem_buf),
	      ops_memory_get_length(mem_buf));
    
    // output file contents as Literal Data packet

    if (debug)
        fprintf(stderr,"** Writing out data now\n");

    ops_write_literal_data_from_buf(ops_memory_get_data(mem_buf),
				    ops_memory_get_length(mem_buf),
				    OPS_LDT_BINARY, cinfo);

    if (debug)
        fprintf(stderr, "** After Writing out data now\n");

    // add subpackets to signature
    // - creation time
    // - key id

    ops_signature_add_creation_time(sig, time(NULL));

    ops_keyid(keyid, &skey->public_key);
    ops_signature_add_issuer_key_id(sig, keyid);

    ops_signature_hashed_subpackets_end(sig);

    // write out sig
    ops_write_signature(sig, &skey->public_key, skey, cinfo);

    ops_teardown_file_write(cinfo, fd_out);

    // tidy up
    ops_create_signature_delete(sig);
    ops_memory_free(mem_buf);

    return ops_true;
    }

/**
\ingroup HighLevel_Sign
\brief Signs a buffer
\param input Input text to be signed
\param input_len Length of input text
\param sig_type Signature type
\param skey Secret Key
\param use_armour Write armoured text, if set
\param include_data Includes the signed data in the output message. If not, creates a detached signature.
\return New ops_memory_t struct containing signed text
\note It is the caller's responsibility to call ops_memory_free(me)

Example Code:
\code
void example(const ops_secret_key_t *skey)
{
  const char* buf="Some example text";
  const size_t len=strlen(buf);
  const ops_boolean_t use_armour=ops_true;

  ops_memory_t* mem=NULL;
  
  mem=ops_sign_buf(buf,len,OPS_SIG_BINARY,skey,use_armour);
  if (mem)
  {
    printf ("OK");
    ops_memory_free(mem);
  }
  else
  {
    printf("ERR");
  }
}
\endcode
*/
ops_memory_t* ops_sign_buf(const void* input, const size_t input_len,
                           const ops_sig_type_t sig_type,
                           const ops_hash_algorithm_t hash_algorithm,
                           const ops_secret_key_t *skey,
                           const ops_boolean_t use_armour,
                           ops_boolean_t include_data,
                           ops_boolean_t include_creation_time,
                           ops_boolean_t include_key_id)
    {
    // \todo allow choice of hash algorithams
    // enforce use of SHA1 for now

    unsigned char keyid[OPS_KEY_ID_SIZE];
    ops_create_signature_t *sig=NULL;

    ops_create_info_t *cinfo=NULL;
    ops_memory_t *mem=ops_memory_new();

    ops_hash_algorithm_t hash_alg=hash_algorithm ;
    ops_literal_data_type_t ld_type;
    ops_hash_t* hash=NULL;

    // setup literal data packet type
    if (sig_type == OPS_SIG_BINARY)
        ld_type=OPS_LDT_BINARY;
    else
        ld_type=OPS_LDT_TEXT;

    // set up signature
    sig=ops_create_signature_new();
    ops_signature_start_message_signature(sig, skey, hash_alg, sig_type);

    // setup writer
    ops_setup_memory_write(&cinfo, &mem, input_len);

    //  set armoured/not armoured here
    if (use_armour)
        ops_writer_push_armoured_message(cinfo);

    if (debug)
	fprintf(stderr, "** Writing out one pass sig\n");

    // write one_pass_sig
	 if(include_data)
		 ops_write_one_pass_sig(skey, hash_alg, sig_type, cinfo);

    // hash file contents
    hash=ops_signature_get_hash(sig);
    hash->add(hash, input, input_len);
    
    // output file contents as Literal Data packet

    if (debug)
        fprintf(stderr,"** Writing out data now\n");

	 if(include_data)
		 ops_write_literal_data_from_buf(input, input_len, ld_type, cinfo);

    if (debug)
        fprintf(stderr,"** After Writing out data now\n");

    // add subpackets to signature
    // - creation time
    // - key id

	 if(include_creation_time)
		 ops_signature_add_creation_time(sig, time(NULL));

	 if(include_key_id)
	 {
		 ops_keyid(keyid, &skey->public_key);
		 ops_signature_add_issuer_key_id(sig, keyid);
	 }

    ops_signature_hashed_subpackets_end(sig);

    // write out sig
    ops_write_signature(sig, &skey->public_key, skey, cinfo);

    // tidy up
    ops_writer_close(cinfo);
	 free(cinfo) ;
    ops_create_signature_delete(sig);

    return mem;
    }

typedef struct {
  ops_create_signature_t *signature;
  const ops_secret_key_t *skey;
  ops_hash_algorithm_t hash_alg;
  ops_sig_type_t sig_type;
} signature_arg_t;

static ops_boolean_t stream_signature_writer(const unsigned char *src,
                                             unsigned length,
                                             ops_error_t **errors,
                                             ops_writer_info_t *winfo)
    {
    signature_arg_t* arg = ops_writer_get_arg(winfo);
    // Add the input data to the hash. At the end, we will use the hash
    // to generate a signature packet.
    ops_hash_t* hash = ops_signature_get_hash(arg->signature);
    hash->add(hash, src, length);

    return ops_stacked_write(src, length, errors, winfo);
    }

static ops_boolean_t stream_signature_write_trailer(ops_create_info_t *cinfo,
                                                    void *data)
    {
    signature_arg_t* arg = data;
    unsigned char keyid[OPS_KEY_ID_SIZE];

    // add subpackets to signature
    // - creation time
    // - key id
    ops_signature_add_creation_time(arg->signature,time(NULL));
    ops_keyid(keyid, &arg->skey->public_key);
    ops_signature_add_issuer_key_id(arg->signature, keyid);
    ops_signature_hashed_subpackets_end(arg->signature);

    // write out signature
    return ops_write_signature(arg->signature, &arg->skey->public_key,
			       arg->skey, cinfo);
    }

static void stream_signature_destroyer(ops_writer_info_t *winfo)
    {
    signature_arg_t* arg = ops_writer_get_arg(winfo);
    ops_create_signature_delete(arg->signature);
    free(arg);
    }

/**
\ingroup Core_WritePackets
\brief Pushes a signed writer onto the stack.

Data written will be encoded as a onepass signature packet, followed
by a literal packet, followed by a signature packet. Once this writer
has been added to the stack, cleartext can be written straight to the
output, and it will be encoded as a literal packet and signed.

\param cinfo Write settings
\param sig_type the type of input to be signed (text or binary)
\param skey the key used to sign the stream.
\return false if the initial onepass packet could not be created.
*/
ops_boolean_t ops_writer_push_signed(ops_create_info_t *cinfo,
                                     const ops_sig_type_t sig_type,
                                     const ops_secret_key_t *skey)
    {
    // \todo allow choice of hash algorithams
    // enforce use of SHA1 for now

    // Create arg to be used with this writer
    // Remember to free this in the destroyer
    signature_arg_t *signature_arg = ops_mallocz(sizeof *signature_arg);
    signature_arg->signature = ops_create_signature_new();
    signature_arg->hash_alg = OPS_HASH_SHA1;
    signature_arg->skey = skey;
    signature_arg->sig_type = sig_type;
    ops_signature_start_message_signature(signature_arg->signature,
					  signature_arg->skey,
					  signature_arg->hash_alg,
					  signature_arg->sig_type);

    if (!ops_write_one_pass_sig(signature_arg->skey,
				signature_arg->hash_alg,
				signature_arg->sig_type,
				cinfo))
	return ops_false;

    ops_writer_push_partial_with_trailer(0, cinfo, OPS_PTAG_CT_LITERAL_DATA,
					 write_literal_header, NULL,
					 stream_signature_write_trailer,
					 signature_arg);
    // And push writer on stack
    ops_writer_push(cinfo, stream_signature_writer, NULL,
		    stream_signature_destroyer,signature_arg);
    return ops_true;
    }

// EOF
