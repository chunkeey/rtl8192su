/*
 *      Software AES encryption/descryption routines
 *
 *      $Id: 8190n_aes.c,v 1.7 2009/08/06 11:41:28 yachang Exp $
 */

#define _8190N_AES_C_

#ifdef __KERNEL__
#include <asm/byteorder.h>
#endif

#include "./8190n_cfg.h"

#ifndef __KERNEL__
#include "./sys-support.h"
#endif

#include "./8190n.h"
#include "./ieee802_mib.h"
#include "./8190n_util.h"
#include "./8190n_headers.h"
#include "./8190n_debug.h"

#define SHOW_INIT_BLOCKS
#define SHOW_HEADER_FIELDS
#define SHOW_CTR_PRELOAD

#define MAX_MSG_SIZE	2048
/*****************************/
/******** SBOX Table *********/
/*****************************/

    unsigned char sbox_table[256] =
    {
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
        0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
        0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
        0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
        0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
        0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
        0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
        0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
        0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
        0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
        0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
        0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
        0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
        0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
        0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
        0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
        0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
    };

/*****************************/
/**** Function Prototypes ****/
/*****************************/

void bitwise_xor(unsigned char *ina, unsigned char *inb, unsigned char *out);
void construct_mic_iv(
                        unsigned char *mic_header1,
                        int qc_exists,
                        int a4_exists,
                        unsigned char *mpdu,
                        unsigned int payload_length,
                        unsigned char * pn_vector);
void construct_mic_header1(
                        unsigned char *mic_header1,
                        int header_length,
                        unsigned char *mpdu);
void construct_mic_header2(
                    unsigned char *mic_header2,
                    unsigned char *mpdu,
                    int a4_exists,
                    int qc_exists);
void construct_ctr_preload(
                        unsigned char *ctr_preload,
                        int a4_exists,
                        int qc_exists,
                        unsigned char *mpdu,
                        unsigned char *pn_vector,
                        int c);
void xor_128(unsigned char *a, unsigned char *b, unsigned char *out);
void xor_32(unsigned char *a, unsigned char *b, unsigned char *out);
unsigned char sbox(unsigned char a);
void next_key(unsigned char *key, int round);
void byte_sub(unsigned char *in, unsigned char *out);
void shift_row(unsigned char *in, unsigned char *out);
void mix_column(unsigned char *in, unsigned char *out);
void add_round_key( unsigned char *shiftrow_in,
                    unsigned char *mcol_in,
                    unsigned char *block_in,
                    int round,
                    unsigned char *out);
void aes128k128d(unsigned char *key, unsigned char *data, unsigned char *ciphertext);


/****************************************/
/* aes128k128d()                        */
/* Performs a 128 bit AES encrypt with  */
/* 128 bit data.                        */
/****************************************/
void xor_128(unsigned char *a, unsigned char *b, unsigned char *out)
{
    int i;
    for (i=0;i<16; i++)
    {
        out[i] = a[i] ^ b[i];
    }
}


void xor_32(unsigned char *a, unsigned char *b, unsigned char *out)
{
    int i;
    for (i=0;i<4; i++)
    {
        out[i] = a[i] ^ b[i];
    }
}


unsigned char sbox(unsigned char a)
{
    return sbox_table[(int)a];
}


void next_key(unsigned char *key, int round)
{
    unsigned char rcon;
    unsigned char sbox_key[4];
    unsigned char rcon_table[12] =
    {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
        0x1b, 0x36, 0x36, 0x36
    };

    sbox_key[0] = sbox(key[13]);
    sbox_key[1] = sbox(key[14]);
    sbox_key[2] = sbox(key[15]);
    sbox_key[3] = sbox(key[12]);

    rcon = rcon_table[round];

    xor_32(&key[0], sbox_key, &key[0]);
    key[0] = key[0] ^ rcon;

    xor_32(&key[4], &key[0], &key[4]);
    xor_32(&key[8], &key[4], &key[8]);
    xor_32(&key[12], &key[8], &key[12]);
}


void byte_sub(unsigned char *in, unsigned char *out)
{
    int i;
    for (i=0; i< 16; i++)
    {
        out[i] = sbox(in[i]);
    }
}


void shift_row(unsigned char *in, unsigned char *out)
{
    out[0] =  in[0];
    out[1] =  in[5];
    out[2] =  in[10];
    out[3] =  in[15];
    out[4] =  in[4];
    out[5] =  in[9];
    out[6] =  in[14];
    out[7] =  in[3];
    out[8] =  in[8];
    out[9] =  in[13];
    out[10] = in[2];
    out[11] = in[7];
    out[12] = in[12];
    out[13] = in[1];
    out[14] = in[6];
    out[15] = in[11];
}


void mix_column(unsigned char *in, unsigned char *out)
{
    int i;
    unsigned char add1b[4];
    unsigned char add1bf7[4];
    unsigned char rotl[4];
    unsigned char swap_halfs[4];
    unsigned char andf7[4];
    unsigned char rotr[4];
    unsigned char temp[4];
    unsigned char tempb[4];

    for (i=0 ; i<4; i++)
    {
        if ((in[i] & 0x80)== 0x80)
            add1b[i] = 0x1b;
        else
            add1b[i] = 0x00;
    }

    swap_halfs[0] = in[2];    /* Swap halfs */
    swap_halfs[1] = in[3];
    swap_halfs[2] = in[0];
    swap_halfs[3] = in[1];

    rotl[0] = in[3];        /* Rotate left 8 bits */
    rotl[1] = in[0];
    rotl[2] = in[1];
    rotl[3] = in[2];

    andf7[0] = in[0] & 0x7f;
    andf7[1] = in[1] & 0x7f;
    andf7[2] = in[2] & 0x7f;
    andf7[3] = in[3] & 0x7f;

    for (i = 3; i>0; i--)    /* logical shift left 1 bit */
    {
        andf7[i] = andf7[i] << 1;
        if ((andf7[i-1] & 0x80) == 0x80)
        {
            andf7[i] = (andf7[i] | 0x01);
        }
    }
    andf7[0] = andf7[0] << 1;
    andf7[0] = andf7[0] & 0xfe;

    xor_32(add1b, andf7, add1bf7);

    xor_32(in, add1bf7, rotr);

    temp[0] = rotr[0];         /* Rotate right 8 bits */
    rotr[0] = rotr[1];
    rotr[1] = rotr[2];
    rotr[2] = rotr[3];
    rotr[3] = temp[0];

    xor_32(add1bf7, rotr, temp);
    xor_32(swap_halfs, rotl,tempb);
    xor_32(temp, tempb, out);
}


void aes128k128d(unsigned char *key, unsigned char *data, unsigned char *ciphertext)
{
    int round;
    int i;
    unsigned char intermediatea[16];
    unsigned char intermediateb[16];
    unsigned char round_key[16];

    for(i=0; i<16; i++) round_key[i] = key[i];

    for (round = 0; round < 11; round++)
    {
        if (round == 0)
        {
            xor_128(round_key, data, ciphertext);
            next_key(round_key, round);
        }
        else if (round == 10)
        {
            byte_sub(ciphertext, intermediatea);
            shift_row(intermediatea, intermediateb);
            xor_128(intermediateb, round_key, ciphertext);
        }
        else    /* 1 - 9 */
        {
            byte_sub(ciphertext, intermediatea);
            shift_row(intermediatea, intermediateb);
            mix_column(&intermediateb[0], &intermediatea[0]);
            mix_column(&intermediateb[4], &intermediatea[4]);
            mix_column(&intermediateb[8], &intermediatea[8]);
            mix_column(&intermediateb[12], &intermediatea[12]);
            xor_128(intermediatea, round_key, ciphertext);
            next_key(round_key, round);
        }
    }

}


/************************************************/
/* construct_mic_iv()                           */
/* Builds the MIC IV from header fields and PN  */
/************************************************/
void construct_mic_iv(
                        unsigned char *mic_iv,
                        int qc_exists,
                        int a4_exists,
                        unsigned char *mpdu,
                        unsigned int payload_length,
                        unsigned char *pn_vector
                        )
{
    int i;

    mic_iv[0] = 0x59;
    if (qc_exists && a4_exists) mic_iv[1] = mpdu[30] & 0x0f;    /* QoS_TC           */
    if (qc_exists && !a4_exists) mic_iv[1] = mpdu[24] & 0x0f;   /* mute bits 7-4    */
    if (!qc_exists) mic_iv[1] = 0x00;
    for (i = 2; i < 8; i++)
        mic_iv[i] = mpdu[i + 8];                    /* mic_iv[2:7] = A2[0:5] = mpdu[10:15] */
    #ifdef CONSISTENT_PN_ORDER
        for (i = 8; i < 14; i++)
            mic_iv[i] = pn_vector[i - 8];           /* mic_iv[8:13] = PN[0:5] */
    #else
        for (i = 8; i < 14; i++)
            mic_iv[i] = pn_vector[13 - i];          /* mic_iv[8:13] = PN[5:0] */
    #endif
    mic_iv[14] = (unsigned char) (payload_length / 256);
    mic_iv[15] = (unsigned char) (payload_length % 256);
}


/************************************************/
/* construct_mic_header1()                      */
/* Builds the first MIC header block from       */
/* header fields.                               */
/************************************************/
void construct_mic_header1(
                        unsigned char *mic_header1,
                        int header_length,
                        unsigned char *mpdu
                        )
{
    mic_header1[0] = (unsigned char)((header_length - 2) / 256);
    mic_header1[1] = (unsigned char)((header_length - 2) % 256);
    mic_header1[2] = mpdu[0] & 0xcf;    /* Mute CF poll & CF ack bits */
    mic_header1[3] = mpdu[1] & 0xc7;    /* Mute retry, more data and pwr mgt bits */
    mic_header1[4] = mpdu[4];       /* A1 */
    mic_header1[5] = mpdu[5];
    mic_header1[6] = mpdu[6];
    mic_header1[7] = mpdu[7];
    mic_header1[8] = mpdu[8];
    mic_header1[9] = mpdu[9];
    mic_header1[10] = mpdu[10];     /* A2 */
    mic_header1[11] = mpdu[11];
    mic_header1[12] = mpdu[12];
    mic_header1[13] = mpdu[13];
    mic_header1[14] = mpdu[14];
    mic_header1[15] = mpdu[15];
}


/************************************************/
/* construct_mic_header2()                      */
/* Builds the last MIC header block from        */
/* header fields.                               */
/************************************************/
void construct_mic_header2(
                unsigned char *mic_header2,
                unsigned char *mpdu,
                int a4_exists,
                int qc_exists
                )
{
    int i;

    for (i = 0; i<16; i++) mic_header2[i]=0x00;

    mic_header2[0] = mpdu[16];    /* A3 */
    mic_header2[1] = mpdu[17];
    mic_header2[2] = mpdu[18];
    mic_header2[3] = mpdu[19];
    mic_header2[4] = mpdu[20];
    mic_header2[5] = mpdu[21];

    //mic_header2[6] = mpdu[22] & 0xf0;   /* SC */
    mic_header2[6] = 0x00;
    mic_header2[7] = 0x00; /* mpdu[23]; */


    if (!qc_exists & a4_exists)
    {
        for (i=0;i<6;i++) mic_header2[8+i] = mpdu[24+i];   /* A4 */

    }

    if (qc_exists && !a4_exists)
    {
        mic_header2[8] = mpdu[24] & 0x0f; /* mute bits 15 - 4 */
        mic_header2[9] = mpdu[25] & 0x00;
    }

    if (qc_exists && a4_exists)
    {
        for (i=0;i<6;i++) mic_header2[8+i] = mpdu[24+i];   /* A4 */

        mic_header2[14] = mpdu[30] & 0x0f;
        mic_header2[15] = mpdu[31] & 0x00;
    }


}


/************************************************/
/* construct_mic_header2()                      */
/* Builds the last MIC header block from        */
/* header fields.                               */
/************************************************/
void construct_ctr_preload(
                        unsigned char *ctr_preload,
                        int a4_exists,
                        int qc_exists,
                        unsigned char *mpdu,
                        unsigned char *pn_vector,
                        int c
                        )
{
    int i = 0;
    for (i=0; i<16; i++) ctr_preload[i] = 0x00;
    i = 0;

    ctr_preload[0] = 0x01;                                  /* flag */
    if (qc_exists && a4_exists) ctr_preload[1] = mpdu[30] & 0x0f;   /* QoC_Control */
    if (qc_exists && !a4_exists) ctr_preload[1] = mpdu[24] & 0x0f;

    for (i = 2; i < 8; i++)
        ctr_preload[i] = mpdu[i + 8];                       /* ctr_preload[2:7] = A2[0:5] = mpdu[10:15] */
    #ifdef CONSISTENT_PN_ORDER
      for (i = 8; i < 14; i++)
            ctr_preload[i] =    pn_vector[i - 8];           /* ctr_preload[8:13] = PN[0:5] */
    #else
      for (i = 8; i < 14; i++)
            ctr_preload[i] =    pn_vector[13 - i];          /* ctr_preload[8:13] = PN[5:0] */
    #endif
    ctr_preload[14] =  (unsigned char) (c / 256); /* Ctr */
    ctr_preload[15] =  (unsigned char) (c % 256);
}


/************************************/
/* bitwise_xor()                    */
/* A 128 bit, bitwise exclusive or  */
/************************************/
void bitwise_xor(unsigned char *ina, unsigned char *inb, unsigned char *out)
{
    int i;
    for (i=0; i<16; i++)
    {
        out[i] = ina[i] ^ inb[i];
    }
}


/*-----------------------------------------------------------------------------
hdr: wlanhdr
llc: llc_snap
pframe: raw data payload
plen: length of raw data payload
mic: mic of AES
------------------------------------------------------------------------------*/
static void aes_tx(struct rtl8190_priv *priv, UINT8 *key, UINT8 keyid,
			union PN48 *pn48, UINT8 *hdr, UINT8 *llc,
			UINT8 *pframe, UINT32 plen, UINT8* txmic)
{
	static UINT8	message[MAX_MSG_SIZE];
	UINT32	qc_exists, a4_exists, i, j, payload_remainder,
			num_blocks,payload_length, payload_index;

	UINT8 pn_vector[6];
	UINT8 mic_iv[16];
    UINT8 mic_header1[16];
    UINT8 mic_header2[16];
    UINT8 ctr_preload[16];

    /* Intermediate Buffers */
    UINT8 chain_buffer[16];
    UINT8 aes_out[16];
    UINT8 padded_buffer[16];
    UINT8 mic[8];

	UINT32	offset = 0;
	UINT32	hdrlen  = get_hdrlen(priv, hdr);

	memset((void *)mic_iv, 0, 16);
	memset((void *)mic_header1, 0, 16);
	memset((void *)mic_header2, 0, 16);
	memset((void *)ctr_preload, 0, 16);
	memset((void *)chain_buffer, 0, 16);
	memset((void *)aes_out, 0, 16);
	memset((void *)padded_buffer, 0, 16);

	if (get_tofr_ds(hdr) != 0x03)
		a4_exists = 0;
	else
		a4_exists = 1;

	if (is_qos_data(hdr)) {
		qc_exists = 1;
		//hdrlen += 2;	// these 2 bytes has already added
	}
	else
		qc_exists = 0;

	// below is to collecting each frag(hdr, llc, pay, and mic into single message buf)

	// extiv (8 bytes long) should have been appended
	pn_vector[0]  = hdr[hdrlen]   = pn48->_byte_.TSC0;
	pn_vector[1]  = hdr[hdrlen+1] = pn48->_byte_.TSC1;
	hdr[hdrlen+2] =  0x00;
	hdr[hdrlen+3] = (0x20 | (keyid << 6));
	pn_vector[2]  = hdr[hdrlen+4] = pn48->_byte_.TSC2;
	pn_vector[3]  = hdr[hdrlen+5] = pn48->_byte_.TSC3;
	pn_vector[4]  = hdr[hdrlen+6] = pn48->_byte_.TSC4;
	pn_vector[5]  = hdr[hdrlen+7] = pn48->_byte_.TSC5;

	memcpy((void *)message, hdr, (hdrlen + 8)); //8 is for ext iv len
	offset = (hdrlen + 8);

	if (llc)
	{
		memcpy((void *)(message + offset), (void *)llc, 8);
		offset += 8;
	}
	memcpy((void *)(message + offset), (void *)pframe, plen);
	offset += plen;

	// now we have collecting all the bytes into single message buf

	payload_length = plen; // 8 is for llc

	if (llc)
		payload_length += 8;

	construct_mic_iv(
                        mic_iv,
                        qc_exists,
                        a4_exists,
                        message,
                        (payload_length),
                        pn_vector
                        );

    construct_mic_header1(
                            mic_header1,
                            hdrlen,
                            message
                            );
    construct_mic_header2(
                            mic_header2,
                            message,
                            a4_exists,
                            qc_exists
                            );


	payload_remainder = (payload_length) % 16;
    num_blocks = (payload_length) / 16;

    /* Find start of payload */
    payload_index = (hdrlen + 8);

    /* Calculate MIC */
    aes128k128d(key, mic_iv, aes_out);
    bitwise_xor(aes_out, mic_header1, chain_buffer);
    aes128k128d(key, chain_buffer, aes_out);
    bitwise_xor(aes_out, mic_header2, chain_buffer);
    aes128k128d(key, chain_buffer, aes_out);

	for (i = 0; i < num_blocks; i++)
    {
        bitwise_xor(aes_out, &message[payload_index], chain_buffer);

        payload_index += 16;
        aes128k128d(key, chain_buffer, aes_out);
    }

    /* Add on the final payload block if it needs padding */
    if (payload_remainder > 0)
    {
        for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
        for (j = 0; j < payload_remainder; j++)
        {
            padded_buffer[j] = message[payload_index++];
        }
        bitwise_xor(aes_out, padded_buffer, chain_buffer);
        aes128k128d(key, chain_buffer, aes_out);

    }

    for (j = 0 ; j < 8; j++) mic[j] = aes_out[j];

    /* Insert MIC into payload */
    for (j = 0; j < 8; j++)
    	message[payload_index+j] = mic[j];

	payload_index = hdrlen + 8;
	for (i=0; i< num_blocks; i++)
    {
        construct_ctr_preload(
                                ctr_preload,
                                a4_exists,
                                qc_exists,
                                message,
                                pn_vector,
                                i+1);
        aes128k128d(key, ctr_preload, aes_out);
        bitwise_xor(aes_out, &message[payload_index], chain_buffer);
        for (j=0; j<16;j++) message[payload_index++] = chain_buffer[j];
    }

    if (payload_remainder > 0)          /* If there is a short final block, then pad it,*/
    {                                   /* encrypt it and copy the unpadded part back   */
        construct_ctr_preload(
                                ctr_preload,
                                a4_exists,
                                qc_exists,
                                message,
                                pn_vector,
                                num_blocks+1);

        for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
        for (j = 0; j < payload_remainder; j++)
        {
            padded_buffer[j] = message[payload_index+j];
        }
        aes128k128d(key, ctr_preload, aes_out);
        bitwise_xor(aes_out, padded_buffer, chain_buffer);
        for (j=0; j<payload_remainder;j++) message[payload_index++] = chain_buffer[j];
    }

    /* Encrypt the MIC */
    construct_ctr_preload(
                        ctr_preload,
                        a4_exists,
                        qc_exists,
                        message,
                        pn_vector,
                        0);

    for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
    for (j = 0; j < 8; j++)
    {
        padded_buffer[j] = message[j+hdrlen+8+payload_length];
    }

    aes128k128d(key, ctr_preload, aes_out);
    bitwise_xor(aes_out, padded_buffer, chain_buffer);
    for (j=0; j<8;j++) message[payload_index++] = chain_buffer[j];

	// now, going to copy the final result back to the input buf...
	offset =0;

	//if (llc)
	{
		memcpy((void *)hdr, (void *)(&message[offset]), (hdrlen + 8 )); //8 is for ext iv
    	offset += (hdrlen + 8);
    }

    if (llc)
    {
    	memcpy((void *)llc, (void *)(&message[offset]),  8 ); //8 is for llc
    	offset +=  (8);
    }


    memcpy((void *)pframe, (void *)(&message[offset]), (plen)); //now is for plen
    offset += (plen);


    memcpy((void *)txmic, (void *)(&message[offset]), 8); //now is for mic
    offset += 8;

	rtl_cache_sync_wback(priv, (unsigned long)hdr, hdrlen + 8, PCI_DMA_TODEVICE);
	if (llc)
		rtl_cache_sync_wback(priv, (unsigned long)llc, 8, PCI_DMA_TODEVICE);
	rtl_cache_sync_wback(priv, (unsigned long)pframe, plen, PCI_DMA_TODEVICE);
	rtl_cache_sync_wback(priv, (unsigned long)txmic, 8, PCI_DMA_TODEVICE);

    _DEBUG_INFO("--txmic=%X %X %X %X %X %X %X %X\n",
    txmic[0], txmic[1], txmic[2], txmic[3],
    txmic[4], txmic[5], txmic[6], txmic[7]);

   	if (pn48->val48 == 0xffffffffffffULL)
		pn48->val48 = 0;
	else
		pn48->val48++;
}


void aesccmp_encrypt(struct rtl8190_priv *priv, unsigned char *pwlhdr,
				unsigned char *frag1,
				unsigned char *frag2, unsigned int frag2_len,
				unsigned char *frag3)
{
	unsigned char *da;
	unsigned char *ttkey = NULL;
	union PN48 	  *pn48 = NULL;
	unsigned int	keyid = 0;
	struct stat_info	*pstat = NULL;

	da = get_da(pwlhdr);

	if (OPMODE & WIFI_AP_STATE)
	{
#ifdef WDS
		unsigned int to_fr_ds = (GetToDs(pwlhdr) << 1) | GetFrDs(pwlhdr);
		if (to_fr_ds == 3)
			da = GetAddr1Ptr(pwlhdr);
#endif
#ifdef CONFIG_RTK_MESH
//modify by Joule for SECURITY
		pstat = get_stainfo (priv, da);
		if( isMeshPoint(pstat) )
		{
			ttkey  = ((GET_MIB(priv))->dot11sKeysTable.dot11EncryptKey.dot11TTKey.skey);
			pn48 = (&((GET_MIB(priv))->dot11sKeysTable.dot11EncryptKey.dot11TXPN48));	 
			keyid = 0;
		} else
#endif
		if (IS_MCAST(da))
		{
			ttkey = GET_GROUP_ENCRYP_KEY;
			pn48 = GET_GROUP_ENCRYP_PN;
			keyid = 1;
		}
		else
		{
			pstat = get_stainfo(priv, da);
			if (pstat == NULL) {
				DEBUG_ERR("tx aes pstat == NULL\n");
				return;
			}
			ttkey = GET_UNICAST_ENCRYP_KEY;
			pn48 = GET_UNICAST_ENCRYP_PN;
			keyid = 0;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		pstat = get_stainfo(priv, BSSID);
		if (pstat == NULL) {
			DEBUG_ERR("tx aes pstat == NULL\n");
			return;
		}
		ttkey = GET_UNICAST_ENCRYP_KEY;
		pn48 = GET_UNICAST_ENCRYP_PN;
		keyid = 0;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		ttkey = GET_GROUP_ENCRYP_KEY;
		pn48 = GET_GROUP_ENCRYP_PN;
		keyid = 0;
	}
#endif

	if ((ttkey == NULL) || (pn48 == NULL)) {
		DEBUG_ERR("no encrypt key for AES due to ttkey=NULL or pn48=NULL\n");
		return;
	}

	aes_tx(priv, ttkey, keyid, pn48, pwlhdr, frag1, frag2, frag2_len, frag3);
}


static void aes_rx(UINT8 *ttkey, UINT8 qc_exists, UINT8 a4_exists,
				UINT8 *pframe, UINT32 hdrlen, UINT32 plen)
{
	UINT32	i, j, payload_remainder,
			num_blocks, payload_index;

	UINT8 pn_vector[6];
	UINT8 mic_iv[16];
    UINT8 mic_header1[16];
    UINT8 mic_header2[16];
    UINT8 ctr_preload[16];

    UINT8 chain_buffer[16];
    UINT8 aes_out[16];
    UINT8 padded_buffer[16];
    UINT8 mic[8];

	memset((void *)mic_iv, 0, 16);
	memset((void *)mic_header1, 0, 16);
	memset((void *)mic_header2, 0, 16);
	memset((void *)ctr_preload, 0, 16);
	memset((void *)chain_buffer, 0, 16);
	memset((void *)aes_out, 0, 16);
	memset((void *)mic, 0, 8);

	num_blocks = plen / 16; //(plen including llc, payload_length and mic )

	payload_remainder = plen % 16;

	pn_vector[0]  = pframe[hdrlen];
	pn_vector[1]  = pframe[hdrlen+1];
	pn_vector[2]  = pframe[hdrlen+4];
	pn_vector[3]  = pframe[hdrlen+5];
	pn_vector[4]  = pframe[hdrlen+6];
	pn_vector[5]  = pframe[hdrlen+7];

	// now, decrypt pframe with hdrlen offset and plen long

	payload_index = hdrlen + 8; // 8 is for extiv
	for (i=0; i< num_blocks; i++)
    {
        construct_ctr_preload(
                                ctr_preload,
                                a4_exists,
                                qc_exists,
                                pframe,
                                pn_vector,
                                i+1
                            );

        aes128k128d(ttkey, ctr_preload, aes_out);
        bitwise_xor(aes_out, &pframe[payload_index], chain_buffer);

        for (j=0; j<16;j++) pframe[payload_index++] = chain_buffer[j];
    }

    if (payload_remainder > 0)          /* If there is a short final block, then pad it,*/
    {                                   /* encrypt it and copy the unpadded part back   */
        construct_ctr_preload(
                                ctr_preload,
                                a4_exists,
                                qc_exists,
                                pframe,
                                pn_vector,
                                num_blocks+1
                            );

        for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
        for (j = 0; j < payload_remainder; j++)
        {
            padded_buffer[j] = pframe[payload_index+j];
        }
        aes128k128d(ttkey, ctr_preload, aes_out);
        bitwise_xor(aes_out, padded_buffer, chain_buffer);
        for (j=0; j<payload_remainder;j++) pframe[payload_index++] = chain_buffer[j];
    }
}


unsigned int aesccmp_decrypt(struct rtl8190_priv *priv, struct rx_frinfo *pfrinfo)
{
	UINT8	*ttkey = NULL;
	UINT32	hdrlen, keylen = 0;
	UINT8	a4_exists;
	UINT8	qc_exists;
	UINT8	*sa	= pfrinfo->sa;
	UINT8	*pframe = get_pframe(pfrinfo);
	UINT8	to_fr_ds = pfrinfo->to_fr_ds;
	struct stat_info	*pstat = NULL;

	if (to_fr_ds != 0x03) {
		hdrlen = WLAN_HDR_A3_LEN;
		a4_exists = 0;
	}
	else {
		hdrlen = WLAN_HDR_A4_LEN;
		a4_exists = 1;
	}

	if (is_qos_data(pframe)) {
		qc_exists = 1;
		hdrlen += 2;
	}
	else
		qc_exists = 0;

	if (OPMODE & WIFI_AP_STATE)
	{
#if defined(WDS) || defined(CONFIG_RTK_MESH)
		if (to_fr_ds == 3)
			pstat = get_stainfo (priv, GetAddr2Ptr(pframe));
		else
#endif
		pstat = get_stainfo (priv, sa);
#ifdef CONFIG_RTK_MESH
//modify by Joule for SECURITY
		if( isMeshPoint(pstat) )
		{
			keylen = ((GET_MIB(priv))->dot11sKeysTable.dot11EncryptKey.dot11TTKeyLen);
			ttkey  = ((GET_MIB(priv))->dot11sKeysTable.dot11EncryptKey.dot11TTKey.skey);
		}else
#endif
		{
		if (pstat == NULL)
		{
			DEBUG_ERR("AES Rx fails! sa=%02X%02X%02X%02X%02X%02X\n",
				sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);
			return FALSE;
		}

		keylen = GET_UNICAST_ENCRYP_KEYLEN;
		ttkey  = GET_UNICAST_ENCRYP_KEY;
		}
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		if (IS_MCAST(pfrinfo->da))
		{
			keylen = GET_GROUP_ENCRYP_KEYLEN;
			ttkey  = GET_GROUP_ENCRYP_KEY;
		}
		else
		{
			pstat = get_stainfo(priv, BSSID);
			if (pstat == NULL) {
				DEBUG_ERR("rx aes pstat == NULL\n");
				return FALSE;
			}
			keylen = GET_UNICAST_ENCRYP_KEYLEN;
			ttkey  = GET_UNICAST_ENCRYP_KEY;
		}
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		keylen = GET_GROUP_ENCRYP_KEYLEN;
		ttkey  = GET_GROUP_ENCRYP_KEY;
	}
#endif

	if (keylen == 0) {
		DEBUG_ERR("no descrypt key for AES due to keylen=0\n");
		return FALSE;
	}

	aes_rx(ttkey, qc_exists, a4_exists, pframe, hdrlen, pfrinfo->pktlen-hdrlen-8);
	return TRUE;
}

