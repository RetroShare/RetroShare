/*******************************************************************************
 * libretroshare/src/crypto: chacha20.cc                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 by Mr.Alice        <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <stdexcept>
#include <stdint.h>
#include <assert.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <iostream>
#include <stdlib.h>

#include "crypto/chacha20.h"
#include "util/rsprint.h"
#include "util/rsrandom.h"
#include "util/rstime.h"

#define rotl(x,n) { x = (x << n) | (x >> (-n & 31)) ;}

//#define DEBUG_CHACHA20

#if OPENSSL_VERSION_NUMBER >= 0x010100000L && !defined(LIBRESSL_VERSION_NUMBER)
    #define AEAD_chacha20_poly1305_openssl AEAD_chacha20_poly1305
#else
    #define AEAD_chacha20_poly1305_rs AEAD_chacha20_poly1305
#endif

namespace librs {
namespace crypto {

/*!
 * \brief The uint256_32 struct
 * 			This structure represents 256bits integers, to be used for computing poly1305 authentication tags.
 */
struct uint256_32
{
    uint32_t b[8] ;

    uint256_32() { memset(&b[0],0,8*sizeof(uint32_t)) ; }

    uint256_32(uint32_t b7,uint32_t b6,uint32_t b5,uint32_t b4,uint32_t b3,uint32_t b2,uint32_t b1,uint32_t b0)
    {
        b[0]=b0; b[1]=b1; b[2]=b2; b[3]=b3;
        b[4]=b4; b[5]=b5; b[6]=b6; b[7]=b7;
    }

    static uint256_32 random()
    {
        uint256_32 r ;
        for(uint32_t i=0;i<8;++i)
            r.b[i] = RSRandom::random_u32();

        return r;
    }

    // constant cost ==
    bool operator==(const uint256_32& u)
    {
        bool res = true ;

        if(b[0] != u.b[0]) res = false ;
        if(b[1] != u.b[1]) res = false ;
        if(b[2] != u.b[2]) res = false ;
        if(b[3] != u.b[3]) res = false ;
        if(b[4] != u.b[4]) res = false ;
        if(b[5] != u.b[5]) res = false ;
        if(b[6] != u.b[6]) res = false ;
        if(b[7] != u.b[7]) res = false ;

        return res ;
    }

    // Constant cost sum.
    //
    void operator +=(const uint256_32& u)
    {
        uint64_t v(0) ;

        v += (uint64_t)b[0] + (uint64_t)u.b[0] ; b[0] = (uint32_t)v ; v >>= 32;
        v += (uint64_t)b[1] + (uint64_t)u.b[1] ; b[1] = (uint32_t)v ; v >>= 32;
        v += (uint64_t)b[2] + (uint64_t)u.b[2] ; b[2] = (uint32_t)v ; v >>= 32;
        v += (uint64_t)b[3] + (uint64_t)u.b[3] ; b[3] = (uint32_t)v ; v >>= 32;
        v += (uint64_t)b[4] + (uint64_t)u.b[4] ; b[4] = (uint32_t)v ; v >>= 32;
        v += (uint64_t)b[5] + (uint64_t)u.b[5] ; b[5] = (uint32_t)v ; v >>= 32;
        v += (uint64_t)b[6] + (uint64_t)u.b[6] ; b[6] = (uint32_t)v ; v >>= 32;
        v += (uint64_t)b[7] + (uint64_t)u.b[7] ; b[7] = (uint32_t)v ;
    }
    void operator -=(const uint256_32& u)
    {
        *this += ~u ;
        ++*this ;
    }
    void operator++()
    {
        for(int i=0;i<8;++i)
            if( ++b[i] )
                break ;
    }

    bool operator<(const uint256_32& u) const
    {
        for(int i=7;i>=0;--i)
            if(b[i] < u.b[i])
                return true ;
            else
                if(b[i] > u.b[i])
                    return false ;

        return false ;
    }
    uint256_32 operator~() const
    {
        uint256_32 r(*this) ;

        r.b[0] = ~b[0] ;
        r.b[1] = ~b[1] ;
        r.b[2] = ~b[2] ;
        r.b[3] = ~b[3] ;
        r.b[4] = ~b[4] ;
        r.b[5] = ~b[5] ;
        r.b[6] = ~b[6] ;
        r.b[7] = ~b[7] ;

        return r ;
    }

    void poly1305clamp()
    {
        b[0]  &= 0x0fffffff;
        b[1]  &= 0x0ffffffc;
        b[2]  &= 0x0ffffffc;
        b[3]  &= 0x0ffffffc;
    }
    // Constant cost product.
    //
    void operator *=(const uint256_32& u)
    {
        uint256_32 r ;

        for(int i=0;i<8;++i)
            for(int j=0;j<8;++j)
                if(i+j < 8)
                {
                    uint64_t s = (uint64_t)u.b[j]*(uint64_t)b[i] ;

                    uint256_32 partial ;
                    partial.b[i+j] = (s & 0xffffffff) ;

                    if(i+j+1 < 8)
                        partial.b[i+j+1] = (s >> 32) ;

                    r += partial;
                }
        *this = r;
    }

    static void print(const uint256_32& s)
    {
        fprintf(stdout,"%08x %08x %08x %08x %08x %08x %08x %08x",(uint32_t)s.b[7],(uint32_t)s.b[6],(uint32_t)s.b[5],(uint32_t)s.b[4],
                (uint32_t)s.b[3],(uint32_t)s.b[2],(uint32_t)s.b[1],(uint32_t)s.b[0]) ;
    }
    static int max_non_zero_of_height_bits(uint8_t s)
    {
        for(int i=7;i>=0;--i)
            if((s & (1<<i)) != 0)
                return i;
        return -1;
    }
    int max_non_zero_bit() const
    {
        for(int c=7;c>=0;--c)
            if(b[c] != 0)
            {
                if( (b[c] & 0xff000000) != 0) return (c<<5) + 3*8 + max_non_zero_of_height_bits(b[c] >> 24) ;
                if( (b[c] & 0x00ff0000) != 0) return (c<<5) + 2*8 + max_non_zero_of_height_bits(b[c] >> 16) ;
                if( (b[c] & 0x0000ff00) != 0) return (c<<5) + 1*8 + max_non_zero_of_height_bits(b[c] >>  8) ;

                return c*32 + 0*8 + max_non_zero_of_height_bits(b[c]) ;
            }
        return -1;
    }
    void lshift(uint32_t n)
    {
        uint32_t p = n >> 5;		// n/32
        uint32_t u = n & 0x1f ;	// n%32

        if(p > 0)
            for(int i=7;i>=0;--i)
                b[i] = (i>=(int)p)?b[i-p]:0 ;

        uint32_t r = 0 ;

        if(u>0)
        for(int i=0;i<8;++i)
        {
            uint32_t r1 = (b[i] >> (31-u+1)) ;
            b[i] = (b[i] << u) & 0xffffffff;
            b[i] += r ;
            r = r1 ;
        }
    }
    void lshift()
    {
        uint32_t r ;
        uint32_t r1 ;

        r1 = (b[0] >> 31) ; b[0] <<= 1;             r = r1 ;
        r1 = (b[1] >> 31) ; b[1] <<= 1; b[1] += r ; r = r1 ;
        r1 = (b[2] >> 31) ; b[2] <<= 1; b[2] += r ; r = r1 ;
        r1 = (b[3] >> 31) ; b[3] <<= 1; b[3] += r ; r = r1 ;
        r1 = (b[4] >> 31) ; b[4] <<= 1; b[4] += r ; r = r1 ;
        r1 = (b[5] >> 31) ; b[5] <<= 1; b[5] += r ; r = r1 ;
        r1 = (b[6] >> 31) ; b[6] <<= 1; b[6] += r ; r = r1 ;
                            b[7] <<= 1; b[7] += r ;
    }
    void rshift()
    {
        uint32_t r ;
        uint32_t r1 ;

        r1 = b[7] & 0x1; b[7] >>= 1 ;                            r = r1 ;
        r1 = b[6] & 0x1; b[6] >>= 1 ; if(r) b[6] += 0x80000000 ; r = r1 ;
        r1 = b[5] & 0x1; b[5] >>= 1 ; if(r) b[5] += 0x80000000 ; r = r1 ;
        r1 = b[4] & 0x1; b[4] >>= 1 ; if(r) b[4] += 0x80000000 ; r = r1 ;
        r1 = b[3] & 0x1; b[3] >>= 1 ; if(r) b[3] += 0x80000000 ; r = r1 ;
        r1 = b[2] & 0x1; b[2] >>= 1 ; if(r) b[2] += 0x80000000 ; r = r1 ;
        r1 = b[1] & 0x1; b[1] >>= 1 ; if(r) b[1] += 0x80000000 ; r = r1 ;
                         b[0] >>= 1 ; if(r) b[0] += 0x80000000 ;
    }
};

// Compute quotient and modulo of n by p where both n and p are 256bits integers.
//
static void quotient(const uint256_32& n,const uint256_32& p,uint256_32& q,uint256_32& r)
{
    // simple algorithm: add up multiples of u while keeping below *this. Once done, substract.

    r = n ;
    q = uint256_32(0,0,0,0,0,0,0,0) ;

    int bmax = n.max_non_zero_bit() - p.max_non_zero_bit();

    uint256_32 m(0,0,0,0,0,0,0,0) ;
    uint256_32 d = p ;

    m.b[bmax/32] = (1u << (bmax%32)) ;	// set m to be 2^bmax

    d.lshift(bmax);

    for(int b=bmax;b>=0;--b,d.rshift(),m.rshift())
        if(! (r < d))
        {
            r -= d ;
            q += m ;
        }
}

static void remainder(const uint256_32& n,const uint256_32& p,uint256_32& r)
{
    // simple algorithm: add up multiples of u while keeping below *this. Once done, substract.

    r = n ;
    int bmax = n.max_non_zero_bit() - p.max_non_zero_bit();

    uint256_32 d = p ;
    d.lshift(bmax);

    for(int b=bmax;b>=0;--b,d.rshift())
        if(! (r < d))
            r -= d ;
}

class chacha20_state
{
public:
    uint32_t c[16] ;

    chacha20_state(uint8_t key[32],uint32_t block_counter,uint8_t nounce[12])
    {
        c[0] = 0x61707865 ;
        c[1] = 0x3320646e ;
        c[2] = 0x79622d32 ;
        c[3] = 0x6b206574 ;

        c[ 4] = (uint32_t)key[0 ] + (((uint32_t)key[1 ])<<8) + (((uint32_t)key[2 ])<<16) + (((uint32_t)key[3 ])<<24);
        c[ 5] = (uint32_t)key[4 ] + (((uint32_t)key[5 ])<<8) + (((uint32_t)key[6 ])<<16) + (((uint32_t)key[7 ])<<24);
        c[ 6] = (uint32_t)key[8 ] + (((uint32_t)key[9 ])<<8) + (((uint32_t)key[10])<<16) + (((uint32_t)key[11])<<24);
        c[ 7] = (uint32_t)key[12] + (((uint32_t)key[13])<<8) + (((uint32_t)key[14])<<16) + (((uint32_t)key[15])<<24);
        c[ 8] = (uint32_t)key[16] + (((uint32_t)key[17])<<8) + (((uint32_t)key[18])<<16) + (((uint32_t)key[19])<<24);
        c[ 9] = (uint32_t)key[20] + (((uint32_t)key[21])<<8) + (((uint32_t)key[22])<<16) + (((uint32_t)key[23])<<24);
        c[10] = (uint32_t)key[24] + (((uint32_t)key[25])<<8) + (((uint32_t)key[26])<<16) + (((uint32_t)key[27])<<24);
        c[11] = (uint32_t)key[28] + (((uint32_t)key[29])<<8) + (((uint32_t)key[30])<<16) + (((uint32_t)key[31])<<24);

        c[12] = block_counter ;

        c[13] = (uint32_t)nounce[0 ] + (((uint32_t)nounce[1 ])<<8) + (((uint32_t)nounce[2 ])<<16) + (((uint32_t)nounce[3 ])<<24);
        c[14] = (uint32_t)nounce[4 ] + (((uint32_t)nounce[5 ])<<8) + (((uint32_t)nounce[6 ])<<16) + (((uint32_t)nounce[7 ])<<24);
        c[15] = (uint32_t)nounce[8 ] + (((uint32_t)nounce[9 ])<<8) + (((uint32_t)nounce[10])<<16) + (((uint32_t)nounce[11])<<24);
    }
};

static void quarter_round(uint32_t& a,uint32_t& b,uint32_t& c,uint32_t& d)
{
    a += b ; d ^= a; rotl(d,16) ; //d <<<=16 ;
    c += d ; b ^= c; rotl(b,12) ; //b <<<=12 ;
    a += b ; d ^= a; rotl(d,8)  ; //d <<<=8 ;
    c += d ; b ^= c; rotl(b,7)  ; //b <<<=7 ;
}

static void add(chacha20_state& s,const chacha20_state& t) { for(uint32_t i=0;i<16;++i) s.c[i] += t.c[i] ; }

static void apply_20_rounds(chacha20_state& s)
{
    chacha20_state t(s) ;

    for(uint32_t i=0;i<10;++i)
    {
        quarter_round(s.c[ 0],s.c[ 4],s.c[ 8],s.c[12]) ;
        quarter_round(s.c[ 1],s.c[ 5],s.c[ 9],s.c[13]) ;
        quarter_round(s.c[ 2],s.c[ 6],s.c[10],s.c[14]) ;
        quarter_round(s.c[ 3],s.c[ 7],s.c[11],s.c[15]) ;
        quarter_round(s.c[ 0],s.c[ 5],s.c[10],s.c[15]) ;
        quarter_round(s.c[ 1],s.c[ 6],s.c[11],s.c[12]) ;
        quarter_round(s.c[ 2],s.c[ 7],s.c[ 8],s.c[13]) ;
        quarter_round(s.c[ 3],s.c[ 4],s.c[ 9],s.c[14]) ;
    }

    add(s,t) ;
}

#ifdef DEBUG_CHACHA20
static void print(const chacha20_state& s)
{
    fprintf(stdout,"%08x %08x %08x %08x\n",s.c[0 ],s.c[1 ],s.c[2 ],s.c[3 ]) ;
    fprintf(stdout,"%08x %08x %08x %08x\n",s.c[4 ],s.c[5 ],s.c[6 ],s.c[7 ]) ;
    fprintf(stdout,"%08x %08x %08x %08x\n",s.c[8 ],s.c[9 ],s.c[10],s.c[11]) ;
    fprintf(stdout,"%08x %08x %08x %08x\n",s.c[12],s.c[13],s.c[14],s.c[15]) ;
}
#endif

void chacha20_encrypt_rs(uint8_t key[32], uint32_t block_counter, uint8_t nonce[12], uint8_t *data, uint32_t size)
{
    for(uint32_t i=0;i<size/64 + 1;++i)
    {
        chacha20_state s(key,block_counter+i,nonce) ;

#ifdef DEBUG_CHACHA20
        fprintf(stdout,"Block %d:\n",i) ;
        print(s) ;
#endif
        apply_20_rounds(s) ;

#ifdef DEBUG_CHACHA20
        fprintf(stdout,"Cipher %d:\n",i) ;
        print(s) ;
#endif

        for(uint32_t k=0;k<64;++k)
            if(k+64*i < size)
                data[k + 64*i] ^= uint8_t(((s.c[k/4]) >> (8*(k%4))) & 0xff) ;
    }
}

#if OPENSSL_VERSION_NUMBER >= 0x010100000L && !defined(LIBRESSL_VERSION_NUMBER)
void chacha20_encrypt_openssl(uint8_t key[32], uint32_t block_counter, uint8_t nonce[12], uint8_t *data, uint32_t size)
{
    EVP_CIPHER_CTX *ctx;

    int len;
    int tmp_len;
    uint8_t tmp[size];
    uint8_t iv[16];

    // create iv with nonce and block counter
    memcpy(iv, &block_counter, 4);
    memcpy(iv + 4, nonce, 12);

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) return;

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_chacha20(), NULL, key, iv)) goto out;

    /* Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, tmp, &len, data, size)) goto out;
    tmp_len = len;

    /* Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, tmp + len, &len)) goto out;
    tmp_len += len;

    memcpy(data, tmp, tmp_len);

out:
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
}
#endif

struct poly1305_state
{
    uint256_32 r ;
    uint256_32 s ;
    uint256_32 p ;
    uint256_32 a ;
};

static void poly1305_init(poly1305_state& s,uint8_t key[32])
{
    s.r =   uint256_32( 0,0,0,0,
            ((uint32_t)key[12] << 0) + ((uint32_t)key[13] << 8) + ((uint32_t)key[14] << 16) + ((uint32_t)key[15] << 24),
            ((uint32_t)key[ 8] << 0) + ((uint32_t)key[ 9] << 8) + ((uint32_t)key[10] << 16) + ((uint32_t)key[11] << 24),
            ((uint32_t)key[ 4] << 0) + ((uint32_t)key[ 5] << 8) + ((uint32_t)key[ 6] << 16) + ((uint32_t)key[ 7] << 24),
            ((uint32_t)key[ 0] << 0) + ((uint32_t)key[ 1] << 8) + ((uint32_t)key[ 2] << 16) + ((uint32_t)key[ 3] << 24)
            );

    s.r.poly1305clamp();

    s.s = uint256_32( 0,0,0,0,
            ((uint32_t)key[28] << 0) + ((uint32_t)key[29] << 8) + ((uint32_t)key[30] << 16) + ((uint32_t)key[31] << 24),
            ((uint32_t)key[24] << 0) + ((uint32_t)key[25] << 8) + ((uint32_t)key[26] << 16) + ((uint32_t)key[27] << 24),
            ((uint32_t)key[20] << 0) + ((uint32_t)key[21] << 8) + ((uint32_t)key[22] << 16) + ((uint32_t)key[23] << 24),
            ((uint32_t)key[16] << 0) + ((uint32_t)key[17] << 8) + ((uint32_t)key[18] << 16) + ((uint32_t)key[19] << 24)
            );

    s.p = uint256_32(0,0,0,0x3,0xffffffff,0xffffffff,0xffffffff,0xfffffffb) ;
    s.a = uint256_32(0,0,0,  0,         0,         0,         0,         0) ;
}

// Warning: each call will automatically *pad* the data to a multiple of 16 bytes.
//
static void poly1305_add(poly1305_state& s,uint8_t *message,uint32_t size,bool pad_to_16_bytes=false)
{
#ifdef DEBUG_CHACHA20
    std::cerr << "Poly1305: digesting " << RsUtil::BinToHex(message,size) << std::endl;
#endif

    for(uint32_t i=0;i<(size+15)/16;++i)
    {
        uint256_32 block ;
        uint32_t j;

        for(j=0;j<16 && i*16+j < size;++j)
            block.b[j/4] += ((uint64_t)message[i*16+j]) << (8*(j & 0x3)) ;

        if(pad_to_16_bytes)
            block.b[4] += 0x01 ;
        else
            block.b[j/4] += 0x01 << (8*(j& 0x3));

        s.a += block ;
        s.a *= s.r ;

        uint256_32 q,rst;
        remainder(s.a,s.p,rst) ;
        s.a = rst ;
    }
}

static void poly1305_finish(poly1305_state& s,uint8_t tag[16])
{
    s.a += s.s ;

    tag[ 0] = (s.a.b[0] >> 0) & 0xff ; tag[ 1] = (s.a.b[0] >> 8) & 0xff ; tag[ 2] = (s.a.b[0] >>16) & 0xff ; tag[ 3] = (s.a.b[0] >>24) & 0xff ;
    tag[ 4] = (s.a.b[1] >> 0) & 0xff ; tag[ 5] = (s.a.b[1] >> 8) & 0xff ; tag[ 6] = (s.a.b[1] >>16) & 0xff ; tag[ 7] = (s.a.b[1] >>24) & 0xff ;
    tag[ 8] = (s.a.b[2] >> 0) & 0xff ; tag[ 9] = (s.a.b[2] >> 8) & 0xff ; tag[10] = (s.a.b[2] >>16) & 0xff ; tag[11] = (s.a.b[2] >>24) & 0xff ;
    tag[12] = (s.a.b[3] >> 0) & 0xff ; tag[13] = (s.a.b[3] >> 8) & 0xff ; tag[14] = (s.a.b[3] >>16) & 0xff ; tag[15] = (s.a.b[3] >>24) & 0xff ;
}

void poly1305_tag(uint8_t key[32],uint8_t *message,uint32_t size,uint8_t tag[16])
{
    poly1305_state s;

    poly1305_init  (s,key);
    poly1305_add(s,message,size) ;
    poly1305_finish(s,tag);
}

static void poly1305_key_gen(uint8_t key[32], uint8_t nonce[12], uint8_t generated_key[32])
{
    uint32_t counter = 0 ;

    chacha20_state s(key,counter,nonce);
    apply_20_rounds(s) ;

    for(uint32_t k=0;k<8;++k)
        for(uint32_t i=0;i<4;++i)
            generated_key[k*4 + i] = (s.c[k] >> 8*i) & 0xff ;
}

bool constant_time_memory_compare(const uint8_t *m1,const uint8_t *m2,uint32_t size)
{
    return !CRYPTO_memcmp(m1,m2,size) ;
}

bool AEAD_chacha20_poly1305_rs(uint8_t key[32], uint8_t nonce[12],uint8_t *data,uint32_t data_size,uint8_t *aad,uint32_t aad_size,uint8_t tag[16],bool encrypt)
{
    // encrypt + tag. See RFC7539-2.8

    uint8_t session_key[32];
    poly1305_key_gen(key,nonce,session_key);

    uint8_t lengths_vector[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } ;

    for(uint32_t i=0;i<4;++i)
    {
       lengths_vector[0+i] = ( aad_size >> (8*i)) & 0xff ;
       lengths_vector[8+i] = (data_size >> (8*i)) & 0xff ;
    }

    if(encrypt)
    {
       chacha20_encrypt_rs(key,1,nonce,data,data_size);

       poly1305_state pls ;

       poly1305_init(pls,session_key);

       poly1305_add(pls,aad,aad_size,true);		// add and pad the aad
       poly1305_add(pls,data,data_size,true);	// add and pad the cipher text
       poly1305_add(pls,lengths_vector,16,true);	// add the lengths

       poly1305_finish(pls,tag);
       return true ;
    }
    else
    {
       poly1305_state pls ;
       uint8_t computed_tag[16];

       poly1305_init(pls,session_key);

       poly1305_add(pls,aad,aad_size,true);		// add and pad the aad
       poly1305_add(pls,data,data_size,true);	// add and pad the cipher text
       poly1305_add(pls,lengths_vector,16,true);	// add the lengths

       poly1305_finish(pls,computed_tag);

       // decrypt

       chacha20_encrypt_rs(key,1,nonce,data,data_size);

       return constant_time_memory_compare(tag,computed_tag,16) ;
    }
}

#if OPENSSL_VERSION_NUMBER >= 0x010100000L && !defined(LIBRESSL_VERSION_NUMBER)
#define errorOut {ret = false; goto out;}

bool AEAD_chacha20_poly1305_openssl(uint8_t key[32], uint8_t nonce[12], uint8_t *data, uint32_t data_size, uint8_t *aad, uint32_t aad_size, uint8_t tag[16], bool encrypt_or_decrypt)
{
    EVP_CIPHER_CTX *ctx;

    bool ret = true;
    int len;
    const uint8_t tag_len = 16;
    int tmp_len;
    uint8_t tmp[data_size];

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) return false;

    if (encrypt_or_decrypt) {
        /* Initialise the encryption operation. */
        if(1 != EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL)) errorOut

        /* Initialise key and IV */
        if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, nonce)) errorOut

        /* Provide any AAD data. This can be called zero or more times as
         * required
         */
        if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_size)) errorOut

        /* Provide the message to be encrypted, and obtain the encrypted output.
         * EVP_EncryptUpdate can be called multiple times if necessary
         */
        if(1 != EVP_EncryptUpdate(ctx, tmp, &len, data, data_size)) errorOut
        tmp_len = len;

        /* Finalise the encryption. Normally ciphertext bytes may be written at
         * this stage, but this does not occur in GCM mode
         */
        if(1 != EVP_EncryptFinal_ex(ctx, data + len, &len)) errorOut
        tmp_len += len;

        /* Get the tag */
        if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, tag_len, tag)) errorOut
    } else {
        /* Initialise the decryption operation. */
        if(!EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce)) errorOut

        /* Provide any AAD data. This can be called zero or more times as
         * required
         */
        if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_size)) errorOut

        /* Provide the message to be decrypted, and obtain the plaintext output.
         * EVP_DecryptUpdate can be called multiple times if necessary
         */
        if(!EVP_DecryptUpdate(ctx, tmp, &len, data, data_size)) errorOut
        tmp_len = len;

        /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
        if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, tag_len, tag)) errorOut

        /* Finalise the decryption. A positive return value indicates success,
         * anything else is a failure - the plaintext is not trustworthy.
         */
        if(EVP_DecryptFinal_ex(ctx, tmp + len, &len) > 0) {
            /* Success */
            tmp_len += len;
            ret = true;
        } else {
            /* Verify failed */
            errorOut
        }
    }

    memcpy(data, tmp, tmp_len);

out:
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
    return !!ret;
}

#undef errorOut
#endif

bool AEAD_chacha20_sha256(uint8_t key[32], uint8_t nonce[12],uint8_t *data,uint32_t data_size,uint8_t *aad,uint32_t aad_size,uint8_t tag[16],bool encrypt)
{
    // encrypt + tag. See RFC7539-2.8

    if(encrypt)
    {
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
        chacha20_encrypt_rs(key,1,nonce,data,data_size);
#else
        chacha20_encrypt_openssl(key, 1, nonce, data, data_size);
#endif

       uint8_t computed_tag[EVP_MAX_MD_SIZE];
       unsigned int md_size ;

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
       HMAC_CTX hmac_ctx ;
       HMAC_CTX_init(&hmac_ctx) ;

       HMAC_Init(&hmac_ctx,key,32,EVP_sha256()) ;
       HMAC_Update(&hmac_ctx,aad,aad_size) ;
       HMAC_Update(&hmac_ctx,data,data_size) ;
       HMAC_Final(&hmac_ctx,computed_tag,&md_size) ;

       HMAC_CTX_cleanup(&hmac_ctx) ;
#else
       HMAC_CTX *hmac_ctx = HMAC_CTX_new();

       HMAC_Init_ex(hmac_ctx,key,32,EVP_sha256(),NULL) ;
       HMAC_Update(hmac_ctx,aad,aad_size) ;
       HMAC_Update(hmac_ctx,data,data_size) ;
       HMAC_Final(hmac_ctx,computed_tag,&md_size) ;

       HMAC_CTX_free(hmac_ctx) ;
       hmac_ctx=NULL;
#endif

       assert(md_size >= 16);

       memcpy(tag,computed_tag,16) ;

       return true ;
    }
    else
    {
       uint8_t computed_tag[EVP_MAX_MD_SIZE];
       unsigned int md_size ;

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
       HMAC_CTX hmac_ctx ;
       HMAC_CTX_init(&hmac_ctx) ;

       HMAC_Init(&hmac_ctx,key,32,EVP_sha256()) ;
       HMAC_Update(&hmac_ctx,aad,aad_size) ;
       HMAC_Update(&hmac_ctx,data,data_size) ;
       HMAC_Final(&hmac_ctx,computed_tag,&md_size) ;

       HMAC_CTX_cleanup(&hmac_ctx) ;
#else
       HMAC_CTX *hmac_ctx = HMAC_CTX_new();

       HMAC_Init_ex(hmac_ctx,key,32,EVP_sha256(),NULL) ;
       HMAC_Update(hmac_ctx,aad,aad_size) ;
       HMAC_Update(hmac_ctx,data,data_size) ;
       HMAC_Final(hmac_ctx,computed_tag,&md_size) ;

       HMAC_CTX_free(hmac_ctx) ;
       hmac_ctx=NULL;
#endif

       // decrypt

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
        chacha20_encrypt_rs(key,1,nonce,data,data_size);
#else
        chacha20_encrypt_openssl(key, 1, nonce, data, data_size);
#endif

       return constant_time_memory_compare(tag,computed_tag,16) ;
    }
}


bool perform_tests()
{
    // RFC7539 - 2.1.1

    std::cerr << "  quarter round                        " ;

    uint32_t a = 0x11111111 ;
    uint32_t b = 0x01020304 ;
    uint32_t c = 0x9b8d6f43 ;
    uint32_t d = 0x01234567 ;

    quarter_round(a,b,c,d) ;

    if(!(a == 0xea2a92f4)) return false ;
    if(!(b == 0xcb1cf8ce)) return false ;
    if(!(c == 0x4581472e)) return false ;
    if(!(d == 0x5881c4bb)) return false ;

    std::cerr << " OK" << std::endl;

    // RFC7539 - 2.3.2

    std::cerr << "  RFC7539 - 2.3.2                      " ;

    uint8_t key[32]    = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b, \
                            0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, \
                                  0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f } ;
    uint8_t nounce[12] = { 0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x4a,0x00,0x00,0x00,0x00 } ;

    chacha20_state s(key,1,nounce) ;

#ifdef DEBUG_CHACHA20
    print(s) ;
#endif

    apply_20_rounds(s) ;

#ifdef DEBUG_CHACHA20
    fprintf(stdout,"\n") ;

    print(s) ;
#endif

    uint32_t check_vals[16] = {
        0xe4e7f110, 0x15593bd1, 0x1fdd0f50, 0xc47120a3,
        0xc7f4d1c7, 0x0368c033, 0x9aaa2204, 0x4e6cd4c3,
        0x466482d2, 0x09aa9f07, 0x05d7c214, 0xa2028bd9,
        0xd19c12b5, 0xb94e16de, 0xe883d0cb, 0x4e3c50a2
    };

    for(uint32_t i=0;i<16;++i)
          if(s.c[i] != check_vals[i])
                return false ;

    std::cerr << " OK" << std::endl;

    // RFC7539 - 2.4.2

    std::cerr << "  RFC7539 - 2.4.2                      " ;

    uint8_t nounce2[12] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4a,0x00,0x00,0x00,0x00 } ;

    uint8_t plaintext[7*16+2] = {
        0x4c, 0x61, 0x64, 0x69, 0x65, 0x73, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x47, 0x65, 0x6e, 0x74, 0x6c,
        0x65, 0x6d, 0x65, 0x6e, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x61, 0x73,
        0x73, 0x20, 0x6f, 0x66, 0x20, 0x27, 0x39, 0x39, 0x3a, 0x20, 0x49, 0x66, 0x20, 0x49, 0x20, 0x63,
        0x6f, 0x75, 0x6c, 0x64, 0x20, 0x6f, 0x66, 0x66, 0x65, 0x72, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x6f,
        0x6e, 0x6c, 0x79, 0x20, 0x6f, 0x6e, 0x65, 0x20, 0x74, 0x69, 0x70, 0x20, 0x66, 0x6f, 0x72, 0x20,
        0x74, 0x68, 0x65, 0x20, 0x66, 0x75, 0x74, 0x75, 0x72, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x6e, 0x73,
        0x63, 0x72, 0x65, 0x65, 0x6e, 0x20, 0x77, 0x6f, 0x75, 0x6c, 0x64, 0x20, 0x62, 0x65, 0x20, 0x69,
        0x74, 0x2e
    };

    chacha20_encrypt_rs(key,1,nounce2,plaintext,7*16+2) ;

#ifdef DEBUG_CHACHA20
    fprintf(stdout,"CipherText: \n") ;

    for(uint32_t k=0;k<7*16+2;++k)
    {
        fprintf(stdout,"%02x ",plaintext[k]) ;

        if( (k % 16) == 15)
            fprintf(stdout,"\n") ;
    }
    fprintf(stdout,"\n") ;
#endif

    uint8_t check_cipher_text[7*16+2] = {
        0x6e, 0x2e, 0x35, 0x9a, 0x25, 0x68, 0xf9, 0x80, 0x41, 0xba, 0x07, 0x28, 0xdd, 0x0d, 0x69, 0x81,
        0xe9, 0x7e, 0x7a, 0xec, 0x1d, 0x43, 0x60, 0xc2, 0x0a, 0x27, 0xaf, 0xcc, 0xfd, 0x9f, 0xae, 0x0b,
        0xf9, 0x1b, 0x65, 0xc5, 0x52, 0x47, 0x33, 0xab, 0x8f, 0x59, 0x3d, 0xab, 0xcd, 0x62, 0xb3, 0x57,
        0x16, 0x39, 0xd6, 0x24, 0xe6, 0x51, 0x52, 0xab, 0x8f, 0x53, 0x0c, 0x35, 0x9f, 0x08, 0x61, 0xd8,
        0x07, 0xca, 0x0d, 0xbf, 0x50, 0x0d, 0x6a, 0x61, 0x56, 0xa3, 0x8e, 0x08, 0x8a, 0x22, 0xb6, 0x5e,
        0x52, 0xbc, 0x51, 0x4d, 0x16, 0xcc, 0xf8, 0x06, 0x81, 0x8c, 0xe9, 0x1a, 0xb7, 0x79, 0x37, 0x36,
        0x5a, 0xf9, 0x0b, 0xbf, 0x74, 0xa3, 0x5b, 0xe6, 0xb4, 0x0b, 0x8e, 0xed, 0xf2, 0x78, 0x5e, 0x42,
        0x87, 0x4d
    };

    for(uint32_t i=0;i<7*16+2;++i)
        if(!(check_cipher_text[i] == plaintext[i] ))
            return false;

    std::cerr << " OK" << std::endl;

    // operators

    { uint256_32 uu(0,0,0,0,0,0,0,0         ) ; ++uu ;  if(!(uu == uint256_32(0,0,0,0,0,0,0,1))) return false ; }
    { uint256_32 uu(0,0,0,0,0,0,0,0xffffffff) ; ++uu ;  if(!(uu == uint256_32(0,0,0,0,0,0,1,0))) return false ; }
    { uint256_32 uu(0,0,0,0,0,0,0,0) ; uu = ~uu;++uu ;  if(!(uu == uint256_32(0,0,0,0,0,0,0,0))) return false ; }

    std::cerr << "  operator++ on 256bits numbers         OK" << std::endl;

    // sums/diffs of numbers

    for(uint32_t i=0;i<100;++i)
    {
        uint256_32 a = uint256_32::random() ;
        uint256_32 b = uint256_32::random() ;
#ifdef DEBUG_CHACHA20
        fprintf(stdout,"Adding ") ;
        uint256_32::print(a) ;
        fprintf(stdout,"\n    to ") ;
        uint256_32::print(b) ;
#endif

        uint256_32 c(a) ;
        if(!(c == a) )
           return false;

        c += b  ;

#ifdef DEBUG_CHACHA20
        fprintf(stdout,"\n found ") ;
        uint256_32::print(c) ;
#endif

        c -= b ;

#ifdef DEBUG_CHACHA20
        fprintf(stdout,"\n subst ") ;
        uint256_32::print(c) ;
        fprintf(stdout,"\n") ;
#endif

        if(!(a == a)) return false ;
        if(!(c == a)) return false ;

        uint256_32 vv(0,0,0,0,0,0,0,1) ;
        vv -= a ;
        vv += a ;

        if(!(vv == uint256_32(0,0,0,0,0,0,0,1))) return false ;

    }
    uint256_32 vv(0,0,0,0,0,0,0,0) ;
    uint256_32 ww(0,0,0,0,0,0,0,1) ;
    vv -= ww ;
    if(!(vv == ~uint256_32(0,0,0,0,0,0,0,0))) return false;

    std::cerr << "  Sums / diffs of 256bits numbers       OK" << std::endl;

    // check that (a-b)*(c-d) = ac - bc - ad + bd

    for(uint32_t i=0;i<100;++i)
    {
        uint256_32 a = uint256_32::random();
        uint256_32 b = uint256_32::random();
        uint256_32 c = uint256_32::random();
        uint256_32 d = uint256_32::random();

        uint256_32 amb(a) ;
        amb -= b;
        uint256_32 cmd(c) ;
        cmd -= d;
        uint256_32 ambtcmd(amb);
        ambtcmd *= cmd ;

        uint256_32 atc(a) ; atc *= c ;
        uint256_32 btc(b) ; btc *= c ;
        uint256_32 atd(a) ; atd *= d ;
        uint256_32 btd(b) ; btd *= d ;

        uint256_32 atcmbtcmatdpbtd(atc) ;
        atcmbtcmatdpbtd -= btc ;
        atcmbtcmatdpbtd -= atd ;
        atcmbtcmatdpbtd += btd ;

        if(!(atcmbtcmatdpbtd == ambtcmd)) return false ;
    }
    std::cerr << "  (a-b)*(c-d) == ac-bc-ad+bd on random  OK" << std::endl;

    // shifts

    for(uint32_t i=0;i<100;++i)
    {
        uint256_32 x = uint256_32::random();
        uint256_32 y(x) ;

        uint32_t r = x.b[0] & 0x1 ;
        x.rshift() ;
        x.lshift() ;

        x.b[0] += r ;

        if(!(x == y) ) return false ;
    }
    std::cerr << "  left/right shifting                   OK" << std::endl;

    // test modulo by computing modulo and recomputing the product.

    for(uint32_t i=0;i<100;++i)
    {
        uint256_32 q1(0,0,0,0,0,0,0,0),r1(0,0,0,0,0,0,0,0) ;

        uint256_32 n1 = uint256_32::random();
        uint256_32 p1 = uint256_32::random();

        if(RSRandom::random_f32() < 0.2)
        {
            p1.b[7] = 0 ;

            if(RSRandom::random_f32() < 0.1)
                p1.b[6] = 0 ;
        }

        quotient(n1,p1,q1,r1) ;
#ifdef DEBUG_CHACHA20
        fprintf(stdout,"result: q=") ; uint256_32::print(q1) ; fprintf(stdout," r=") ; uint256_32::print(r1) ; fprintf(stdout,"\n") ;
#endif

        //uint256_32 res(q1) ;
        q1 *= p1 ;
        q1 += r1 ;

        if(!(q1 == n1)) return false ;
    }
    std::cerr << "  Quotient/modulo on random numbers     OK" << std::endl;

    // RFC7539 - 2.5
    //
    {
        uint8_t key[32] = { 0x85,0xd6,0xbe,0x78,0x57,0x55,0x6d,0x33,0x7f,0x44,0x52,0xfe,0x42,0xd5,0x06,0xa8,0x01,0x03,0x80,0x8a,0xfb,0x0d,0xb2,0xfd,0x4a,0xbf,0xf6,0xaf,0x41,0x49,0xf5,0x1b } ;
        uint8_t tag[16] ;
        std::string msg("Cryptographic Forum Research Group") ;

        poly1305_tag(key,(uint8_t*)msg.c_str(),msg.length(),tag) ;

        uint8_t test_tag[16] = { 0xa8,0x06,0x1d,0xc1,0x30,0x51,0x36,0xc6,0xc2,0x2b,0x8b,0xaf,0x0c,0x01,0x27,0xa9 };

        if(!(constant_time_memory_compare(tag,test_tag,16))) return false ;
    }
    std::cerr << "  RFC7539 - 2.5                         OK" << std::endl;

    // RFC7539 - Poly1305 test vector #1
    //
    {
        uint8_t key[32] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;
        uint8_t tag[16] ;
        uint8_t text[64] ;
        memset(text,0,64) ;

        poly1305_tag(key,text,64,tag) ;

        uint8_t test_tag[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #001     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #2
    //
    {
        uint8_t key[32] = {  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x36,0xe5,0xf6,0xb5,0xc5,0xe0,0x60,0x70,0xf0,0xef,0xca,0x96,0x22,0x7a,0x86,0x3e } ;
        uint8_t tag[16] ;

        std::string msg("Any submission to the IETF intended by the Contributor for publication as all or part of an IETF Internet-Draft or RFC and any statement made within the context of an IETF activity is considered an \"IETF Contribution\". Such statements include oral statements in IETF sessions, as well as written and electronic communications made at any time or place, which are addressed to") ;

        poly1305_tag(key,(uint8_t*)msg.c_str(),msg.length(),tag) ;

        uint8_t test_tag[16] = { 0x36,0xe5,0xf6,0xb5,0xc5,0xe0,0x60,0x70,0xf0,0xef,0xca,0x96,0x22,0x7a,0x86,0x3e };

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #002     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #3
    //
    {
        uint8_t key[32] = {  0x36,0xe5,0xf6,0xb5,0xc5,0xe0,0x60,0x70,0xf0,0xef,0xca,0x96,0x22,0x7a,0x86,0x3e,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;
        uint8_t tag[16] ;

        std::string msg("Any submission to the IETF intended by the Contributor for publication as all or part of an IETF Internet-Draft or RFC and any statement made within the context of an IETF activity is considered an \"IETF Contribution\". Such statements include oral statements in IETF sessions, as well as written and electronic communications made at any time or place, which are addressed to") ;

        poly1305_tag(key,(uint8_t*)msg.c_str(),msg.length(),tag) ;

        uint8_t test_tag[16] = { 0xf3,0x47,0x7e,0x7c,0xd9,0x54,0x17,0xaf,0x89,0xa6,0xb8,0x79,0x4c,0x31,0x0c,0xf0 } ;

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #003     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #4
    //
    {
        uint8_t key[32] = {   0x1c ,0x92 ,0x40 ,0xa5 ,0xeb ,0x55 ,0xd3 ,0x8a ,0xf3 ,0x33 ,0x88 ,0x86 ,0x04 ,0xf6 ,0xb5 ,0xf0,
                                     0x47 ,0x39 ,0x17 ,0xc1 ,0x40 ,0x2b ,0x80 ,0x09 ,0x9d ,0xca ,0x5c ,0xbc ,0x20 ,0x70 ,0x75 ,0xc0 };
        uint8_t tag[16] ;

        std::string msg("'Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.") ;

        poly1305_tag(key,(uint8_t*)msg.c_str(),msg.length(),tag) ;

        uint8_t test_tag[16] = { 0x45,0x41,0x66,0x9a,0x7e,0xaa,0xee,0x61,0xe7,0x08,0xdc,0x7c,0xbc,0xc5,0xeb,0x62 } ;

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #004     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #5
    //
    {
        uint8_t key[32] = {  0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        uint8_t tag[16] ;

        uint8_t msg[] = { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };

        poly1305_tag(key,msg,16,tag) ;

        uint8_t test_tag[16] = { 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #005     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #6
    //
    {
        uint8_t key[32] = {  0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };
        uint8_t tag[16] ;

        uint8_t msg[16] = { 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

        poly1305_tag(key,msg,16,tag) ;

        uint8_t test_tag[16] = { 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #006     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #7
    //
    {
        uint8_t key[32] = {  0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        uint8_t tag[16] ;

        uint8_t msg[48] = { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                            0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                            0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;

        poly1305_tag(key,msg,48,tag) ;

        uint8_t test_tag[16] = { 0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #007     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #8
    //
    {
        uint8_t key[32] = {  0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        uint8_t tag[16] ;

        uint8_t msg[48] = { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                            0xfb,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,
                            0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01 } ;

        poly1305_tag(key,msg,48,tag) ;

        uint8_t test_tag[16] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #008     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #9
    //
    {
        uint8_t key[32] = {  0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        uint8_t tag[16] ;

        uint8_t msg[16] = { 0xfd,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff } ;

        poly1305_tag(key,msg,16,tag) ;

        uint8_t test_tag[16] = { 0xfa,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff } ;

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #009     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #10
    //
    {
        uint8_t key[32] = {  0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        uint8_t tag[16] ;

        uint8_t msg[64] = {
                           0xE3 ,0x35 ,0x94 ,0xD7 ,0x50 ,0x5E ,0x43 ,0xB9 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
                           0x33 ,0x94 ,0xD7 ,0x50 ,0x5E ,0x43 ,0x79 ,0xCD ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
                           0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
                           0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 };

        poly1305_tag(key,msg,64,tag) ;

        uint8_t test_tag[16] = { 0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;

        if(!(constant_time_memory_compare(tag,test_tag,16)) ) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #010     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #11
    //
    {
        uint8_t key[32] = {  0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        uint8_t tag[16] ;

        uint8_t msg[48] = {
                           0xE3 ,0x35 ,0x94 ,0xD7 ,0x50 ,0x5E ,0x43 ,0xB9 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
                           0x33 ,0x94 ,0xD7 ,0x50 ,0x5E ,0x43 ,0x79 ,0xCD ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
                           0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 } ;

        poly1305_tag(key,msg,48,tag) ;

        uint8_t test_tag[16] = { 0x13,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;

        if(!constant_time_memory_compare(tag,test_tag,16)) return false ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #011     OK" << std::endl;

    // RFC7539 - 2.6.2
    //
    {
        uint8_t key[32] = { 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
                            0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f };

        uint8_t session_key[32] ;
        uint8_t test_session_key[32] = { 0x8a,0xd5,0xa0,0x8b,0x90,0x5f,0x81,0xcc,0x81,0x50,0x40,0x27,0x4a,0xb2,0x94,0x71,
                                         0xa8,0x33,0xb6,0x37,0xe3,0xfd,0x0d,0xa5,0x08,0xdb,0xb8,0xe2,0xfd,0xd1,0xa6,0x46 };

        uint8_t nonce[12] = { 0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07 };

        poly1305_key_gen(key,nonce,session_key) ;

        if(!constant_time_memory_compare(session_key,test_session_key,32)) return false ;
    }
    std::cerr << "  RFC7539 - 2.6.2                       OK" << std::endl;

    // RFC7539 - Poly1305 key generation. Test vector #1
    //
    {
        uint8_t key[32] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

        uint8_t nonce[12] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        uint8_t session_key[32] ;
        uint8_t test_session_key[32] = {   0x76,0xb8,0xe0,0xad,0xa0,0xf1,0x3d,0x90,0x40,0x5d,0x6a,0xe5,0x53,0x86,0xbd,0x28,
                                           0xbd,0xd2,0x19,0xb8,0xa0,0x8d,0xed,0x1a,0xa8,0x36,0xef,0xcc,0x8b,0x77,0x0d,0xc7 };

        poly1305_key_gen(key,nonce,session_key) ;

        if(!constant_time_memory_compare(session_key,test_session_key,32)) return false ;
    }
    std::cerr << "  RFC7539 poly1305 key gen. TVec #1     OK" << std::endl;

    // RFC7539 - Poly1305 key generation. Test vector #2
    //
    {
        uint8_t key[32] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01 };

        uint8_t nonce[12] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02 };
        uint8_t session_key[32] ;
        uint8_t test_session_key[32] = {    0xec,0xfa,0x25,0x4f,0x84,0x5f,0x64,0x74,0x73,0xd3,0xcb,0x14,0x0d,0xa9,0xe8,0x76,
                                            0x06,0xcb,0x33,0x06,0x6c,0x44,0x7b,0x87,0xbc,0x26,0x66,0xdd,0xe3,0xfb,0xb7,0x39 };



        poly1305_key_gen(key,nonce,session_key) ;

        if(!constant_time_memory_compare(session_key,test_session_key,32)) return false ;
    }
    std::cerr << "  RFC7539 poly1305 key gen. TVec #2     OK" << std::endl;

    // RFC7539 - Poly1305 key generation. Test vector #3
    //
    {
        uint8_t key[32] = {   0x1c,0x92,0x40,0xa5,0xeb,0x55,0xd3,0x8a,0xf3,0x33,0x88,0x86,0x04,0xf6,0xb5,0xf0,
                              0x47,0x39,0x17,0xc1,0x40,0x2b,0x80,0x09,0x9d,0xca,0x5c,0xbc,0x20,0x70,0x75,0xc0 };

        uint8_t nonce[12] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02 };
        uint8_t session_key[32] ;
        uint8_t test_session_key[32] = {      0x96,0x5e,0x3b,0xc6,0xf9,0xec,0x7e,0xd9,0x56,0x08,0x08,0xf4,0xd2,0x29,0xf9,0x4b,
                                              0x13,0x7f,0xf2,0x75,0xca,0x9b,0x3f,0xcb,0xdd,0x59,0xde,0xaa,0xd2,0x33,0x10,0xae  };

        poly1305_key_gen(key,nonce,session_key) ;

        if(!constant_time_memory_compare(session_key,test_session_key,32)) return false ;
    }
    std::cerr << "  RFC7539 poly1305 key gen. TVec #3     OK" << std::endl;

    // RFC7539 - 2.8.2
    //
    {
        uint8_t msg[7*16+2] = {
            0x4c,0x61,0x64,0x69,0x65,0x73,0x20,0x61,0x6e,0x64,0x20,0x47,0x65,0x6e,0x74,0x6c,
            0x65,0x6d,0x65,0x6e,0x20,0x6f,0x66,0x20,0x74,0x68,0x65,0x20,0x63,0x6c,0x61,0x73,
            0x73,0x20,0x6f,0x66,0x20,0x27,0x39,0x39,0x3a,0x20,0x49,0x66,0x20,0x49,0x20,0x63,
            0x6f,0x75,0x6c,0x64,0x20,0x6f,0x66,0x66,0x65,0x72,0x20,0x79,0x6f,0x75,0x20,0x6f,
            0x6e,0x6c,0x79,0x20,0x6f,0x6e,0x65,0x20,0x74,0x69,0x70,0x20,0x66,0x6f,0x72,0x20,
            0x74,0x68,0x65,0x20,0x66,0x75,0x74,0x75,0x72,0x65,0x2c,0x20,0x73,0x75,0x6e,0x73,
            0x63,0x72,0x65,0x65,0x6e,0x20,0x77,0x6f,0x75,0x6c,0x64,0x20,0x62,0x65,0x20,0x69,
            0x74,0x2e } ;

        uint8_t test_msg[7*16+2] = {
            0xd3,0x1a,0x8d,0x34,0x64,0x8e,0x60,0xdb,0x7b,0x86,0xaf,0xbc,0x53,0xef,0x7e,0xc2,
            0xa4,0xad,0xed,0x51,0x29,0x6e,0x08,0xfe,0xa9,0xe2,0xb5,0xa7,0x36,0xee,0x62,0xd6,
            0x3d,0xbe,0xa4,0x5e,0x8c,0xa9,0x67,0x12,0x82,0xfa,0xfb,0x69,0xda,0x92,0x72,0x8b,
            0x1a,0x71,0xde,0x0a,0x9e,0x06,0x0b,0x29,0x05,0xd6,0xa5,0xb6,0x7e,0xcd,0x3b,0x36,
            0x92,0xdd,0xbd,0x7f,0x2d,0x77,0x8b,0x8c,0x98,0x03,0xae,0xe3,0x28,0x09,0x1b,0x58,
            0xfa,0xb3,0x24,0xe4,0xfa,0xd6,0x75,0x94,0x55,0x85,0x80,0x8b,0x48,0x31,0xd7,0xbc,
            0x3f,0xf4,0xde,0xf0,0x8e,0x4b,0x7a,0x9d,0xe5,0x76,0xd2,0x65,0x86,0xce,0xc6,0x4b,
            0x61,0x16 };

        uint8_t   aad[12] = { 0x50,0x51,0x52,0x53,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7 };
        uint8_t nonce[12] = { 0x07,0x00,0x00,0x00,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47 };

        uint8_t key[32] = { 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
                            0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f };
        uint8_t tag[16] ;
        uint8_t test_tag[16] = { 0x1a,0xe1,0x0b,0x59,0x4f,0x09,0xe2,0x6a,0x7e,0x90,0x2e,0xcb,0xd0,0x60,0x06,0x91 };

        AEAD_chacha20_poly1305_rs(key,nonce,msg,7*16+2,aad,12,tag,true) ;

        if(!constant_time_memory_compare(msg,test_msg,7*16+2)) return false ;
        if(!constant_time_memory_compare(tag,test_tag,16)) return false ;

        bool res = AEAD_chacha20_poly1305_rs(key,nonce,msg,7*16+2,aad,12,tag,false) ;

        if(!res) return false ;
    }
    std::cerr << "  RFC7539 - 2.8.2                       OK" << std::endl;


    // RFC7539 - AEAD checking and decryption
    //
    {
        uint8_t key[32] = { 0x1c,0x92,0x40,0xa5,0xeb,0x55,0xd3,0x8a,0xf3,0x33,0x88,0x86,0x04,0xf6,0xb5,0xf0,
                            0x47,0x39,0x17,0xc1,0x40,0x2b,0x80,0x09,0x9d,0xca,0x5c,0xbc,0x20,0x70,0x75,0xc0 };

        uint8_t nonce[12] = { 0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08 };

        uint8_t ciphertext[16*16 + 9] = {
            0x64,0xa0,0x86,0x15,0x75,0x86,0x1a,0xf4,0x60,0xf0,0x62,0xc7,0x9b,0xe6,0x43,0xbd,
            0x5e,0x80,0x5c,0xfd,0x34,0x5c,0xf3,0x89,0xf1,0x08,0x67,0x0a,0xc7,0x6c,0x8c,0xb2,
            0x4c,0x6c,0xfc,0x18,0x75,0x5d,0x43,0xee,0xa0,0x9e,0xe9,0x4e,0x38,0x2d,0x26,0xb0,
            0xbd,0xb7,0xb7,0x3c,0x32,0x1b,0x01,0x00,0xd4,0xf0,0x3b,0x7f,0x35,0x58,0x94,0xcf,
            0x33,0x2f,0x83,0x0e,0x71,0x0b,0x97,0xce,0x98,0xc8,0xa8,0x4a,0xbd,0x0b,0x94,0x81,
            0x14,0xad,0x17,0x6e,0x00,0x8d,0x33,0xbd,0x60,0xf9,0x82,0xb1,0xff,0x37,0xc8,0x55,
            0x97,0x97,0xa0,0x6e,0xf4,0xf0,0xef,0x61,0xc1,0x86,0x32,0x4e,0x2b,0x35,0x06,0x38,
            0x36,0x06,0x90,0x7b,0x6a,0x7c,0x02,0xb0,0xf9,0xf6,0x15,0x7b,0x53,0xc8,0x67,0xe4,
            0xb9,0x16,0x6c,0x76,0x7b,0x80,0x4d,0x46,0xa5,0x9b,0x52,0x16,0xcd,0xe7,0xa4,0xe9,
            0x90,0x40,0xc5,0xa4,0x04,0x33,0x22,0x5e,0xe2,0x82,0xa1,0xb0,0xa0,0x6c,0x52,0x3e,
            0xaf,0x45,0x34,0xd7,0xf8,0x3f,0xa1,0x15,0x5b,0x00,0x47,0x71,0x8c,0xbc,0x54,0x6a,
            0x0d,0x07,0x2b,0x04,0xb3,0x56,0x4e,0xea,0x1b,0x42,0x22,0x73,0xf5,0x48,0x27,0x1a,
            0x0b,0xb2,0x31,0x60,0x53,0xfa,0x76,0x99,0x19,0x55,0xeb,0xd6,0x31,0x59,0x43,0x4e,
            0xce,0xbb,0x4e,0x46,0x6d,0xae,0x5a,0x10,0x73,0xa6,0x72,0x76,0x27,0x09,0x7a,0x10,
            0x49,0xe6,0x17,0xd9,0x1d,0x36,0x10,0x94,0xfa,0x68,0xf0,0xff,0x77,0x98,0x71,0x30,
            0x30,0x5b,0xea,0xba,0x2e,0xda,0x04,0xdf,0x99,0x7b,0x71,0x4d,0x6c,0x6f,0x2c,0x29,
            0xa6,0xad,0x5c,0xb4,0x02,0x2b,0x02,0x70,0x9b };

        uint8_t aad[12] = { 0xf3,0x33,0x88,0x86,0x00,0x00,0x00,0x00,0x00,0x00,0x4e,0x91 };

        uint8_t received_tag[16] = { 0xee,0xad,0x9d,0x67,0x89,0x0c,0xbb,0x22,0x39,0x23,0x36,0xfe,0xa1,0x85,0x1f,0x38 };

        if(!AEAD_chacha20_poly1305_rs(key,nonce,ciphertext,16*16+9,aad,12,received_tag,false))
            return false ;

        uint8_t cleartext[16*16+9] = {
            0x49,0x6e,0x74,0x65,0x72,0x6e,0x65,0x74,0x2d,0x44,0x72,0x61,0x66,0x74,0x73,0x20,
            0x61,0x72,0x65,0x20,0x64,0x72,0x61,0x66,0x74,0x20,0x64,0x6f,0x63,0x75,0x6d,0x65,
            0x6e,0x74,0x73,0x20,0x76,0x61,0x6c,0x69,0x64,0x20,0x66,0x6f,0x72,0x20,0x61,0x20,
            0x6d,0x61,0x78,0x69,0x6d,0x75,0x6d,0x20,0x6f,0x66,0x20,0x73,0x69,0x78,0x20,0x6d,
            0x6f,0x6e,0x74,0x68,0x73,0x20,0x61,0x6e,0x64,0x20,0x6d,0x61,0x79,0x20,0x62,0x65,
            0x20,0x75,0x70,0x64,0x61,0x74,0x65,0x64,0x2c,0x20,0x72,0x65,0x70,0x6c,0x61,0x63,
            0x65,0x64,0x2c,0x20,0x6f,0x72,0x20,0x6f,0x62,0x73,0x6f,0x6c,0x65,0x74,0x65,0x64,
            0x20,0x62,0x79,0x20,0x6f,0x74,0x68,0x65,0x72,0x20,0x64,0x6f,0x63,0x75,0x6d,0x65,
            0x6e,0x74,0x73,0x20,0x61,0x74,0x20,0x61,0x6e,0x79,0x20,0x74,0x69,0x6d,0x65,0x2e,
            0x20,0x49,0x74,0x20,0x69,0x73,0x20,0x69,0x6e,0x61,0x70,0x70,0x72,0x6f,0x70,0x72,
            0x69,0x61,0x74,0x65,0x20,0x74,0x6f,0x20,0x75,0x73,0x65,0x20,0x49,0x6e,0x74,0x65,
            0x72,0x6e,0x65,0x74,0x2d,0x44,0x72,0x61,0x66,0x74,0x73,0x20,0x61,0x73,0x20,0x72,
            0x65,0x66,0x65,0x72,0x65,0x6e,0x63,0x65,0x20,0x6d,0x61,0x74,0x65,0x72,0x69,0x61,
            0x6c,0x20,0x6f,0x72,0x20,0x74,0x6f,0x20,0x63,0x69,0x74,0x65,0x20,0x74,0x68,0x65,
            0x6d,0x20,0x6f,0x74,0x68,0x65,0x72,0x20,0x74,0x68,0x61,0x6e,0x20,0x61,0x73,0x20,
            0x2f,0xe2,0x80,0x9c,0x77,0x6f,0x72,0x6b,0x20,0x69,0x6e,0x20,0x70,0x72,0x6f,0x67,
            0x72,0x65,0x73,0x73,0x2e,0x2f,0xe2,0x80,0x9d } ;

        if(!constant_time_memory_compare(cleartext,ciphertext,16*16+9))
            return false ;
    }
    std::cerr << "  RFC7539 AEAD test vector #1           OK" << std::endl;

    // bandwidth test
    //

    {
        uint32_t SIZE = 1*1024*1024 ;
        uint8_t *ten_megabyte_data = (uint8_t*)malloc(SIZE) ;

        memset(ten_megabyte_data,0x37,SIZE) ;	// put something. We dont really care here.

        uint8_t key[32] = { 0x1c,0x92,0x40,0xa5,0xeb,0x55,0xd3,0x8a,0xf3,0x33,0x88,0x86,0x04,0xf6,0xb5,0xf0,
                            0x47,0x39,0x17,0xc1,0x40,0x2b,0x80,0x09,0x9d,0xca,0x5c,0xbc,0x20,0x70,0x75,0xc0 };

        uint8_t nonce[12] = { 0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08 };
        uint8_t aad[12] = { 0xf3,0x33,0x88,0x86,0x00,0x00,0x00,0x00,0x00,0x00,0x4e,0x91 };

        uint8_t received_tag[16] ;

        {
            rstime::RsScopeTimer s("AEAD1") ;
            chacha20_encrypt_rs(key, 1, nonce, ten_megabyte_data,SIZE) ;

            std::cerr << "  Chacha20 encryption speed             : " << SIZE / (1024.0*1024.0) / s.duration() << " MB/s" << std::endl;
        }
        {
            rstime::RsScopeTimer s("AEAD2") ;
            AEAD_chacha20_poly1305_rs(key,nonce,ten_megabyte_data,SIZE,aad,12,received_tag,true) ;

            std::cerr << "  AEAD/poly1305 own encryption speed    : " << SIZE / (1024.0*1024.0) / s.duration() << " MB/s" << std::endl;
        }
#if OPENSSL_VERSION_NUMBER >= 0x010100000L && !defined(LIBRESSL_VERSION_NUMBER)
        {
            rstime::RsScopeTimer s("AEAD3") ;
            AEAD_chacha20_poly1305_openssl(key,nonce,ten_megabyte_data,SIZE,aad,12,received_tag,true) ;

            std::cerr << "  AEAD/poly1305 openssl encryption speed: " << SIZE / (1024.0*1024.0) / s.duration() << " MB/s" << std::endl;
        }
#endif
        {
            rstime::RsScopeTimer s("AEAD4") ;
            AEAD_chacha20_sha256(key,nonce,ten_megabyte_data,SIZE,aad,12,received_tag,true) ;

            std::cerr << "  AEAD/sha256 encryption speed          : " << SIZE / (1024.0*1024.0) / s.duration() << " MB/s" << std::endl;
        }

        free(ten_megabyte_data) ;
    }

    return true;
}

}
}


