#include <cstdint>
#include <string.h>
#include <stdio.h>
#include "sm4.h"

static const uint8_t Sbox[256] =
{
	0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7, 0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05,
	0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3, 0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
	0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a, 0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62,
	0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95, 0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6,
	0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba, 0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8,
	0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b, 0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35,
	0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2, 0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87,
	0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52, 0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e,
	0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5, 0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1,
	0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55, 0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3,
	0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60, 0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f,
	0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f, 0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51,
	0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f, 0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8,
	0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd, 0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0,
	0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e, 0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84,
	0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20, 0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48
};

/* System parameter */
static const uint32_t FK[4] =
{
	0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc
};

/* fixed parameter */
static const uint32_t CK[32] =
{
	0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
	0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
	0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
	0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
	0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
	0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
	0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
	0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279
};


/*
*rotate shift left marco definition
*
*/
#define SHL(x,n) (((x) & 0xFFFFFFFF) << n)
#define ROTL(x,n) (SHL((x),n) | ((x) >> (32 - n)))


static inline uint32_t get_uint32_BE(const_buf b)
{
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

static inline void put_uint32_BE( uint32_t n, uint8_t * b)
{
	b[0] = (n >> 24) & 0xff;
	b[1] = (n >> 16) & 0xff;
	b[2] = (n >> 8) & 0xff;
	b[3] = (n) & 0xff;
}

static uint32_t sm4_Lt(uint32_t ka)
{
    uint8_t a[4];
	put_uint32_BE(ka, a);

    a[0] = Sbox[a[0]];
    a[1] = Sbox[a[1]];
    a[2] = Sbox[a[2]];
    a[3] = Sbox[a[3]];

	uint32_t interim = get_uint32_BE(a);

    return interim ^ (ROTL(interim, 2)) ^ (ROTL(interim, 10)) ^ (ROTL(interim, 18)) ^ (ROTL(interim, 24));
}

static inline uint32_t sm4_F(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t rk)
{
    return (x0 ^ sm4_Lt(x1 ^ x2 ^ x3 ^ rk));
}


static uint32_t sm4_calc_iRK(uint32_t ka)
{
    uint8_t a[4];

	put_uint32_BE(ka, a);
	a[0] = Sbox[a[0]];
	a[1] = Sbox[a[1]];
	a[2] = Sbox[a[2]];
	a[3] = Sbox[a[3]];

	uint32_t interim = get_uint32_BE(a);
    return interim ^ (ROTL(interim, 13)) ^ (ROTL(interim, 23));
}

void sm4_block( uint32_t sk[32], const_buf input, uint8_t output[SM4_BLOCK_SIZE] )
{
    uint32_t buf[36];

	buf[0] = get_uint32_BE(input);
	buf[1] = get_uint32_BE(input + 4);
	buf[2] = get_uint32_BE(input + 8);
	buf[3] = get_uint32_BE(input + 12);

	for ( size_t i = 0; i < 32; ++i )
    {
        buf[i + 4] = sm4_F(buf[i], buf[i+1], buf[i+2], buf[i+3], sk[i]);
    }
    
	put_uint32_BE(buf[35], output);
	put_uint32_BE(buf[34], output + 4);
	put_uint32_BE(buf[33], output + 8);
	put_uint32_BE(buf[32], output + 12);
}

void sm4_setkey( sm4_ctx_t & ctx, uint8_t key[SM4_BLOCK_SIZE] )
{
    uint32_t k[36];

	k[0] = get_uint32_BE(key + 0) ^ FK[0];
    k[1] = get_uint32_BE(key + 4) ^ FK[1];
    k[2] = get_uint32_BE(key + 8) ^ FK[2];
    k[3] = get_uint32_BE(key + 12) ^ FK[3];

    for( size_t i = 0; i < 32; ++i )
    {
        k[i + 4] = k[i] ^ sm4_calc_iRK(k[i+1] ^ k[i+2] ^ k[i+3] ^ CK[i]);
        ctx.skDec[31 - i] = ctx.skEnc[i] = k[i + 4];
	}
}


void sm4_encrypt_ecb( buf output, const_buf input, size_t length, uint8_t key[SM4_BLOCK_SIZE] )
{
	if ( length % SM4_BLOCK_SIZE != 0 )
		return;

	sm4_ctx_t ctx;

	sm4_setkey( ctx, key );

	for ( size_t i = 0; i < length; i += SM4_BLOCK_SIZE )
        sm4_block( ctx.skEnc, input + i, output + i );
}

void sm4_decrypt_ecb( buf output, const_buf input, size_t length, uint8_t key[SM4_BLOCK_SIZE] )
{
	if ( length % SM4_BLOCK_SIZE != 0 )
		return;

	sm4_ctx_t ctx;

	sm4_setkey( ctx, key );

	for ( size_t i = 0; i < length; i += SM4_BLOCK_SIZE )
        sm4_block( ctx.skDec, input + i, output + i );
}

void sm4_encrypt_cbc( buf output, const_buf input, size_t length, uint8_t key[SM4_BLOCK_SIZE], uint8_t iv[SM4_BLOCK_SIZE] )
{
	if ( length % SM4_BLOCK_SIZE != 0 )
		return;
	
	sm4_ctx_t ctx;

	sm4_setkey( ctx, key );

	for ( size_t i = 0; i < length; i += SM4_BLOCK_SIZE )
    {
        for( size_t j = 0; j < SM4_BLOCK_SIZE; ++j )
            output[i + j] = input[i + j] ^ iv[j];

        sm4_block( ctx.skEnc, output + i, output + i );
        memcpy( iv, output + i, SM4_BLOCK_SIZE );
    }
}

void sm4_decrypt_cbc( buf output, const_buf input, size_t length, uint8_t key[SM4_BLOCK_SIZE], uint8_t iv[SM4_BLOCK_SIZE] )
{
	if ( length % SM4_BLOCK_SIZE != 0 )
		return;
	
	sm4_ctx_t ctx;

	sm4_setkey( ctx, key );

    uint8_t temp[SM4_BLOCK_SIZE];

    for ( size_t i = 0; i < length; i += SM4_BLOCK_SIZE )
    {
        memcpy( temp, input + i, SM4_BLOCK_SIZE );
        sm4_block( ctx.skDec, input + i, output + i );

        for( size_t j = 0; j < SM4_BLOCK_SIZE; ++j )
            output[i + j] = output[i + j] ^ iv[j];

		memcpy( iv, temp, SM4_BLOCK_SIZE );
    }
}
