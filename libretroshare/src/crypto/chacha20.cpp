/*
 * RetroShare C++ File sharing default variables
 *
 *      file_sharing/file_sharing_defaults.h
 *
 * Copyright 2016 by Mr.Alice
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
 * Please report all bugs and problems to "retroshare.project@gmail.com".
 *
 */
#include <assert.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include <openssl/crypto.h>

#include <iostream>
#include <stdlib.h>

#include "crypto/chacha20.h"

#pragma once

#define rotl(x,n) { x = (x << n) | (x >> (-n & 31)) ;}

namespace librs {
namespace crypto {

/*!
 * \brief The uint256_32 struct
 * 			This structure represents 256bits integers, to be used for computing poly1305 authentication tags.
 */
struct uint256_32
{
    uint64_t b[8] ;

    uint256_32() { memset(&b[0],0,8*sizeof(uint64_t)) ; }

    uint256_32(uint32_t b7,uint32_t b6,uint32_t b5,uint32_t b4,uint32_t b3,uint32_t b2,uint32_t b1,uint32_t b0)
    {
        b[0]=b0; b[1]=b1; b[2]=b2; b[3]=b3;
        b[4]=b4; b[5]=b5; b[6]=b6; b[7]=b7;
    }

    static uint256_32 random()		// non cryptographically secure random. Just for testing.
    {
        uint256_32 r ;
        for(uint32_t i=0;i<8;++i)
            r.b[i] = lrand48() & 0xffffffff ;

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
        b[0] += u.b[0];
        b[1] += u.b[1] + (b[0]>>32);
        b[2] += u.b[2] + (b[1]>>32);
        b[3] += u.b[3] + (b[2]>>32);
        b[4] += u.b[4] + (b[3]>>32);
        b[5] += u.b[5] + (b[4]>>32);
        b[6] += u.b[6] + (b[5]>>32);
        b[7] += u.b[7] + (b[6]>>32);

        b[0] &= 0xffffffff;
        b[1] &= 0xffffffff;
        b[2] &= 0xffffffff;
        b[3] &= 0xffffffff;
        b[4] &= 0xffffffff;
        b[5] &= 0xffffffff;
        b[6] &= 0xffffffff;
        b[7] &= 0xffffffff;
    }
    void operator -=(const uint256_32& u) { *this += ~u ; *this += uint256_32(0,0,0,0,0,0,0,1); }

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

        r.b[0] = (~b[0]) & 0xffffffff ;
        r.b[1] = (~b[1]) & 0xffffffff ;
        r.b[2] = (~b[2]) & 0xffffffff ;
        r.b[3] = (~b[3]) & 0xffffffff ;
        r.b[4] = (~b[4]) & 0xffffffff ;
        r.b[5] = (~b[5]) & 0xffffffff ;
        r.b[6] = (~b[6]) & 0xffffffff ;
        r.b[7] = (~b[7]) & 0xffffffff ;

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
                    uint64_t s = u.b[j]*b[i] ;

                    uint256_32 partial ;
                    partial.b[i+j] = (s & 0xffffffff) ;

                    if(i+j+1 < 8)
                        partial.b[i+j+1] = (s >> 32) ;

                    r += partial;
                }
        *this = r;

        assert(!(b[0] & 0xffffffff00000000)) ;
        assert(!(b[1] & 0xffffffff00000000)) ;
        assert(!(b[2] & 0xffffffff00000000)) ;
        assert(!(b[3] & 0xffffffff00000000)) ;
        assert(!(b[4] & 0xffffffff00000000)) ;
        assert(!(b[5] & 0xffffffff00000000)) ;
        assert(!(b[6] & 0xffffffff00000000)) ;
        assert(!(b[7] & 0xffffffff00000000)) ;
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
                if( (b[c] & 0xff000000) != 0) return c*32 + 3*8 + max_non_zero_of_height_bits(b[c] >> 24) ;
                if( (b[c] & 0x00ff0000) != 0) return c*32 + 2*8 + max_non_zero_of_height_bits(b[c] >> 16) ;
                if( (b[c] & 0x0000ff00) != 0) return c*32 + 1*8 + max_non_zero_of_height_bits(b[c] >>  8) ;

                return c*32 + 0*8 + max_non_zero_of_height_bits(b[c]) ;
            }
        return -1;
    }
    void lshift()
    {
        int r = 0 ;

        for(int i=0;i<8;++i)
        {
            uint32_t r1 = (b[i] >> 31) ;
            b[i] = (b[i] << 1) & 0xffffffff;
            b[i] += r ;
            r = r1 ;
        }
    }
    void rshift()
    {
        uint32_t r = 0 ;

        for(int i=7;i>=0;--i)
        {
            uint32_t r1 = b[i] & 0x1;
            b[i] >>= 1 ;
            b[i] += r << 31;
            r = r1 ;
        }
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

    uint256_32 m(0,0,0,0,0,0,0,1) ;
    uint256_32 d = p ;

    for(int i=0;i<bmax;++i)
        m.lshift(), d.lshift() ;

    for(int b=bmax;b>=0;--b,d.rshift(),m.rshift())
        if(! (r < d))
        {
            r -= d ;
            q += m ;
        }
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

static void apply_20_rounds(chacha20_state& s)
{
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
}

static void print(const chacha20_state& s)
{
    fprintf(stdout,"%08x %08x %08x %08x\n",s.c[0 ],s.c[1 ],s.c[2 ],s.c[3 ]) ;
    fprintf(stdout,"%08x %08x %08x %08x\n",s.c[4 ],s.c[5 ],s.c[6 ],s.c[7 ]) ;
    fprintf(stdout,"%08x %08x %08x %08x\n",s.c[8 ],s.c[9 ],s.c[10],s.c[11]) ;
    fprintf(stdout,"%08x %08x %08x %08x\n",s.c[12],s.c[13],s.c[14],s.c[15]) ;
}

static void add(chacha20_state& s,const chacha20_state& t) { for(uint32_t i=0;i<16;++i) s.c[i] += t.c[i] ; }

// static uint8_t read16bits(char s)
// {
//     if(s >= '0' && s <= '9')
//         return s - '0' ;
//     else if(s >= 'a' && s <= 'f')
//         return s - 'a' + 10 ;
//     else if(s >= 'A' && s <= 'F')
//         return s - 'A' + 10 ;
//     else
//         throw std::runtime_error("Not an hex string!") ;
// }
// 
// static uint256_32 create_256bit_int(const std::string& s)
// {
//     uint256_32 r(0,0,0,0,0,0,0,0) ;
//
//     fprintf(stdout,"Scanning %s\n",s.c_str()) ;
//
//     for(int i=0;i<(int)s.length();++i)
//     {
//         uint32_t byte = (s.length() -1 - i)/2 ;
//         uint32_t p = byte/4 ;
//         uint32_t val;
//
//         if(p >= 8)
//             continue ;
//
//         val = read16bits(s[i]) ;
//
//         r.b[p] |= (( (val << (( (s.length()-i+1)%2)*4))) << (8*byte)) ;
//     }
//
//     return r;
// }
// static uint256_32 create_256bit_int_from_serialized(const std::string& s)
// {
//     uint256_32 r(0,0,0,0,0,0,0,0) ;
//
//     fprintf(stdout,"Scanning %s\n",s.c_str()) ;
//
//     for(int i=0;i<(int)s.length();i+=3)
//     {
//         int byte = i/3 ;
//         int p = byte/4 ;
//         int sub_byte = byte - 4*p ;
//
//         uint8_t b1 = read16bits(s[i+0]) ;
//         uint8_t b2 = read16bits(s[i+1]) ;
//         uint32_t b = (b1 << 4) + b2 ;
//
//         r.b[p] |= ( b << (8*sub_byte)) ;
//     }
//     return r ;
// }

void chacha20_encrypt(uint8_t key[32], uint32_t block_counter, uint8_t nonce[12], uint8_t *data, uint32_t size)
{
    for(uint32_t i=0;i<size/64 + 1;++i)
    {
        chacha20_state s(key,block_counter+i,nonce) ;
        chacha20_state t(s) ;

        fprintf(stdout,"Block %d:\n",i) ;
        print(s) ;

        apply_20_rounds(s) ;
        add(s,t) ;

        fprintf(stdout,"Cipher %d:\n",i) ;
        print(s) ;

        for(uint32_t k=0;k<64;++k)
            if(k+64*i < size)
                data[k + 64*i] ^= uint8_t(((s.c[k/4]) >> (8*(k%4))) & 0xff) ;
    }
}

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
static void poly1305_add(poly1305_state& s,uint8_t *message,uint32_t size)
{
    for(uint32_t i=0;i<(size+15)/16;++i)
    {
        uint256_32 block ;
        uint32_t j;

        for(j=0;j<16 && i*16+j < size;++j)
            block.b[j/4] += ((uint64_t)message[i*16+j]) << (8*(j & 0x3)) ;

        block.b[j/4] += 0x01 << (8*(j& 0x3));

        s.a += block ;
        s.a *= s.r ;

        uint256_32 q,rst;
        quotient(s.a,s.p,q,rst) ;
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
    poly1305_add   (s,message,size);
    poly1305_finish(s,tag);
}

static void poly1305_key_gen(uint8_t key[32], uint8_t nonce[12], uint8_t generated_key[32])
{
    uint32_t counter = 0 ;

    chacha20_state s(key,counter,nonce);
    apply_20_rounds(s) ;

    for(uint32_t k=0;k<16;++k)
        for(uint32_t i=0;i<4;++i)
            generated_key[k*4 + i] = (s.c[k] >> 8*i) & 0xff ;
}

bool constant_time_memory_compare(const uint8_t *m1,const uint8_t *m2,uint32_t size)
{
    return !CRYPTO_memcmp(m1,m2,size) ;
}

bool AEAD_chacha20_poly1305(uint8_t key[32], uint8_t nonce[12],uint8_t *data,uint32_t data_size,uint8_t *aad,uint32_t aad_size,uint8_t tag[16],bool encrypt)
{
    // encrypt + tag. See RFC7539-2.8

    uint8_t session_key[32];
    poly1305_key_gen(key,nonce,session_key);

    uint8_t lengths_vector[16] ;
    for(uint32_t i=0;i<8;++i)
    {
       lengths_vector[0+i] = ( aad_size >> i) & 0xff ;
       lengths_vector[8+i] = (data_size >> i) & 0xff ;
    }

    if(encrypt)
    {
       chacha20_encrypt(session_key,1,nonce,data,data_size);

       poly1305_state pls ;

       poly1305_init(pls,session_key);

       poly1305_add(pls,aad,aad_size);		// add and pad the aad
       poly1305_add(pls,data,data_size);	// add and pad the cipher text
       poly1305_add(pls,lengths_vector,16);	// add the lengths

       poly1305_finish(pls,tag);
       return true ;
    }
    else
    {
       poly1305_state pls ;
       uint8_t computed_tag[16];

       poly1305_init(pls,session_key);

       poly1305_add(pls,aad,aad_size);		// add and pad the aad
       poly1305_add(pls,data,data_size);	// add and pad the cipher text
       poly1305_add(pls,lengths_vector,16);	// add the lengths

       poly1305_finish(pls,computed_tag);

       // decrypt

       chacha20_encrypt(session_key,1,nonce,data,data_size);

       return constant_time_memory_compare(tag,computed_tag,16) ;
    }
}

void perform_tests()
{
    std::cerr << "Testing Chacha20" << std::endl;

    // RFC7539 - 2.1.1

    std::cerr << "  quarter round..." ;

    uint32_t a = 0x11111111 ;
    uint32_t b = 0x01020304 ;
    uint32_t c = 0x9b8d6f43 ;
    uint32_t d = 0x01234567 ;

    quarter_round(a,b,c,d) ;

    assert(a == 0xea2a92f4) ;
    assert(b == 0xcb1cf8ce) ;
    assert(c == 0x4581472e) ;
    assert(d == 0x5881c4bb) ;

    std::cerr << " - OK" << std::endl;

    // RFC7539 - 2.3.2

    std::cerr << "  RFC7539 - 2.3.2..." ;

    uint8_t key[32]    = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b, \
                            0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, \
                                  0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f } ;
    uint8_t nounce[12] = { 0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x4a,0x00,0x00,0x00,0x00 } ;

    chacha20_state s(key,1,nounce) ;
    chacha20_state t(s) ;

    print(s) ;

    fprintf(stdout,"\n") ;

    apply_20_rounds(s) ;

    print(s) ;
    add(t,s) ;

    fprintf(stdout,"\n") ;

    print(t) ;

    std::cerr << " - OK" << std::endl;

    // RFC7539 - 2.4.2

    std::cerr << "  RFC7539 - 2.4.2..." ;

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

    chacha20_encrypt(key,1,nounce2,plaintext,7*16+2) ;

#ifdef PRINT_RESULTS
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
        assert(check_cipher_text[i] == plaintext[i] );

    std::cerr << " - OK" << std::endl;

    // sums/diffs of numbers

    for(uint32_t i=0;i<100;++i)
    {
        uint256_32 a = uint256_32::random() ;
        uint256_32 b = uint256_32::random() ;
#ifdef PRINT_RESULTS
        fprintf(stdout,"Adding ") ;
        uint256_32::print(a) ;
        fprintf(stdout,"\n    to ") ;
        uint256_32::print(b) ;
#endif

        uint256_32 c(a) ;
        assert(c == a) ;

        c += b  ;

#ifdef PRINT_RESULTS
        fprintf(stdout,"\n found ") ;
        uint256_32::print(c) ;
#endif

        c -= b ;

#ifdef PRINT_RESULTS
        fprintf(stdout,"\n subst ") ;
        uint256_32::print(c) ;
        fprintf(stdout,"\n") ;
#endif

        assert(a == a) ;
        assert(c == a) ;
    }
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

        assert(atcmbtcmatdpbtd == ambtcmd);
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

        assert(x == y) ;
    }
    std::cerr << "  left/right shifting                   OK" << std::endl;

    // test modulo by computing modulo and recomputing the product.

    for(uint32_t i=0;i<100;++i)
    {
        uint256_32 q1(0,0,0,0,0,0,0,0),r1(0,0,0,0,0,0,0,0) ;

        uint256_32 n1 = uint256_32::random();
        uint256_32 p1 = uint256_32::random();

        if(drand48() < 0.2)
        {
            p1.b[7] = 0 ;

            if(drand48() < 0.1)
                p1.b[6] = 0 ;
        }

        quotient(n1,p1,q1,r1) ;
#ifdef PRINT_RESULTS
        fprintf(stdout,"result: q=") ; chacha20::uint256_32::print(q1) ; fprintf(stdout," r=") ; chacha20::uint256_32::print(r1) ; fprintf(stdout,"\n") ;
#endif

        uint256_32 res(q1) ;
        q1 *= p1 ;
        q1 += r1 ;

        assert(q1 == n1) ;
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

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }

    std::cerr << "  RFC7539 poly1305 test vector #001     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #1
    //

    {
        uint8_t key[32] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;
        uint8_t tag[16] ;
        uint8_t text[64] ;
        memset(text,0,64) ;

        poly1305_tag(key,text,64,tag) ;

        uint8_t test_tag[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }

    std::cerr << "  RFC7539 poly1305 test vector #002     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #2
    //

    {
        uint8_t key[32] = {  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x36,0xe5,0xf6,0xb5,0xc5,0xe0,0x60,0x70,0xf0,0xef,0xca,0x96,0x22,0x7a,0x86,0x3e } ;
        uint8_t tag[16] ;

        std::string msg("Any submission to the IETF intended by the Contributor for publication as all or part of an IETF Internet-Draft or RFC and any statement made within the context of an IETF activity is considered an \"IETF Contribution\". Such statements include oral statements in IETF sessions, as well as written and electronic communications made at any time or place, which are addressed to") ;

        poly1305_tag(key,(uint8_t*)msg.c_str(),msg.length(),tag) ;

        uint8_t test_tag[16] = { 0x36,0xe5,0xf6,0xb5,0xc5,0xe0,0x60,0x70,0xf0,0xef,0xca,0x96,0x22,0x7a,0x86,0x3e };

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }

    std::cerr << "  RFC7539 poly1305 test vector #003     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #3
    //

    {
        uint8_t key[32] = {  0x36,0xe5,0xf6,0xb5,0xc5,0xe0,0x60,0x70,0xf0,0xef,0xca,0x96,0x22,0x7a,0x86,0x3e,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;
        uint8_t tag[16] ;

        std::string msg("Any submission to the IETF intended by the Contributor for publication as all or part of an IETF Internet-Draft or RFC and any statement made within the context of an IETF activity is considered an \"IETF Contribution\". Such statements include oral statements in IETF sessions, as well as written and electronic communications made at any time or place, which are addressed to") ;

        poly1305_tag(key,(uint8_t*)msg.c_str(),msg.length(),tag) ;

        uint8_t test_tag[16] = { 0xf3,0x47,0x7e,0x7c,0xd9,0x54,0x17,0xaf,0x89,0xa6,0xb8,0x79,0x4c,0x31,0x0c,0xf0 } ;

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }

    std::cerr << "  RFC7539 poly1305 test vector #004     OK" << std::endl;
    // RFC7539 - Poly1305 test vector #4
    //
    {
        uint8_t key[32] = {   0x1c ,0x92 ,0x40 ,0xa5 ,0xeb ,0x55 ,0xd3 ,0x8a ,0xf3 ,0x33 ,0x88 ,0x86 ,0x04 ,0xf6 ,0xb5 ,0xf0,
                                     0x47 ,0x39 ,0x17 ,0xc1 ,0x40 ,0x2b ,0x80 ,0x09 ,0x9d ,0xca ,0x5c ,0xbc ,0x20 ,0x70 ,0x75 ,0xc0 };
        uint8_t tag[16] ;

        std::string msg("'Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.") ;

        poly1305_tag(key,(uint8_t*)msg.c_str(),msg.length(),tag) ;

        uint8_t test_tag[16] = { 0x45,0x41,0x66,0x9a,0x7e,0xaa,0xee,0x61,0xe7,0x08,0xdc,0x7c,0xbc,0xc5,0xeb,0x62 } ;

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }

    std::cerr << "  RFC7539 poly1305 test vector #005     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #5
    //
    {
        uint8_t key[32] = {  0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        uint8_t tag[16] ;

        uint8_t msg[] = { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };

        poly1305_tag(key,msg,16,tag) ;

        uint8_t test_tag[16] = { 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }

    std::cerr << "  RFC7539 poly1305 test vector #006     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #6
    //
    {
        uint8_t key[32] = {  0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };
        uint8_t tag[16] ;

        uint8_t msg[16] = { 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

        poly1305_tag(key,msg,16,tag) ;

        uint8_t test_tag[16] = { 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #007     OK" << std::endl;

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

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #008     OK" << std::endl;

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

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #009     OK" << std::endl;

    // RFC7539 - Poly1305 test vector #9
    //
    {
        uint8_t key[32] = {  0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
        uint8_t tag[16] ;

        uint8_t msg[16] = { 0xfd,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff } ;

        poly1305_tag(key,msg,16,tag) ;

        uint8_t test_tag[16] = { 0xfa,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff } ;

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #010     OK" << std::endl;

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

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }
    std::cerr << "  RFC7539 poly1305 test vector #011     OK" << std::endl;

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

        assert(constant_time_memory_compare(tag,test_tag,16)) ;
    }
}

}
}


