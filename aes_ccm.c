/*
 * Copyright 2003-2004, Instant802 Networks, Inc.
 * Copyright 2005-2006, Devicescape Software, Inc.
 *
 * Rewrite: Copyright (C) 2013 Linaro Ltd <ard.biesheuvel@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/crypto.h>
#include <linux/ieee80211.h>
#include <linux/etherdevice.h>
#include <net/cfg80211.h>
#include <crypto/aead.h>
#include <crypto/aes.h>

#include "aes_ccm.h"

static void ccmp_special_blocks(struct ieee80211_hdr *hdr, size_t hdrlen,
				const u64 pn, u8 *b_0, u8 *aad)
{
	__le16 mask_fc;
	int a4_included, mgmt;
	u8 qos_tid;
	u16 len_a;

	/* Mask FC: zero subtype b4 b5 b6 (if not mgmt)
	 * Retry, PwrMgt, MoreData; set Protected
	 */
	mgmt = ieee80211_is_mgmt(hdr->frame_control);
	mask_fc = hdr->frame_control;
	mask_fc &= ~cpu_to_le16(IEEE80211_FCTL_RETRY |
				IEEE80211_FCTL_PM | IEEE80211_FCTL_MOREDATA);
	if (!mgmt)
		mask_fc &= ~cpu_to_le16(0x0070);
	mask_fc |= cpu_to_le16(IEEE80211_FCTL_PROTECTED);

	len_a = hdrlen - 2;
	a4_included = ieee80211_has_a4(hdr->frame_control);

	if (ieee80211_is_data_qos(hdr->frame_control))
		qos_tid = *ieee80211_get_qos_ctl(hdr) &
			IEEE80211_QOS_CTL_TID_MASK;
	else
		qos_tid = 0;

	/* In CCM, the initial vectors (IV) used for CTR mode encryption and CBC
	 * mode authentication are not allowed to collide, yet both are derived
	 * from this vector b_0. We only set L := 1 here to indicate that the
	 * data size can be represented in (L+1) bytes. The CCM layer will take
	 * care of storing the data length in the top (L+1) bytes and setting
	 * and clearing the other bits as is required to derive the two IVs.
	 */
	b_0[0] = 0x1;

	/* Nonce: Nonce Flags | A2 | PN
	 * Nonce Flags: Priority (b0..b3) | Management (b4) | Reserved (b5..b7)
	 */
	b_0[1] = qos_tid | (mgmt << 4);
	ether_addr_copy(&b_0[2], hdr->addr2);
	b_0[8]  = pn >> 40;
	b_0[9]  = pn >> 32;
	b_0[10] = pn >> 24;
	b_0[11] = pn >> 16;
	b_0[12] = pn >> 8;
	b_0[13] = pn;

	/* AAD (extra authenticate-only data) / masked 802.11 header
	 * FC | A1 | A2 | A3 | SC | [A4] | [QC]
	 */
	put_unaligned_be16(len_a, &aad[0]);
	put_unaligned(mask_fc, (__le16 *)&aad[2]);
	memcpy(&aad[4], &hdr->addr1, 3 * ETH_ALEN);

	/* Mask Seq#, leave Frag# */
	aad[22] = *((u8 *) &hdr->seq_ctrl) & 0x0f;
	aad[23] = 0;

	if (a4_included) {
		ether_addr_copy(&aad[24], hdr->addr4);
		aad[30] = qos_tid;
		aad[31] = 0;
	} else {
		memset(&aad[24], 0, ETH_ALEN + IEEE80211_QOS_CTL_LEN);
		aad[24] = qos_tid;
	}
}

/*
 * The scatterlist must not point at the stack (VMAP_STACK), so the request
 * and a copy of the AAD live in one heap allocation, as in mac80211's
 * aead_api.c. b_0 is only read through req->iv, which may stay on the stack.
 */
static int r92su_ccm_crypt(struct crypto_aead *tfm, bool enc, u8 *b_0,
			   const u8 *aad, u8 *data, size_t data_len, u8 *mic,
			   size_t mic_len)
{
	size_t aad_len = be16_to_cpup((__be16 *)aad);
	size_t reqsize = sizeof(struct aead_request) + crypto_aead_reqsize(tfm);
	struct aead_request *aead_req;
	struct scatterlist sg[3];
	u8 *__aad;
	int err;

	aead_req = kzalloc(reqsize + aad_len, GFP_ATOMIC);
	if (!aead_req)
		return -ENOMEM;

	__aad = (u8 *)aead_req + reqsize;
	memcpy(__aad, &aad[2], aad_len);

	sg_init_table(sg, 3);
	sg_set_buf(&sg[0], __aad, aad_len);
	sg_set_buf(&sg[1], data, data_len);
	sg_set_buf(&sg[2], mic, mic_len);

	aead_request_set_tfm(aead_req, tfm);
	aead_request_set_crypt(aead_req, sg, sg,
			       enc ? data_len : data_len + mic_len, b_0);
	aead_request_set_ad(aead_req, aad_len);

	err = enc ? crypto_aead_encrypt(aead_req) :
		    crypto_aead_decrypt(aead_req);
	kfree_sensitive(aead_req);
	return err;
}

int ieee80211_aes_ccm_encrypt(struct crypto_aead *tfm, struct sk_buff *skb,
			      const u64 pn, size_t mic_len)
{
	u8 aad[2 * AES_BLOCK_SIZE];
	u8 b_0[AES_BLOCK_SIZE];
	struct ieee80211_hdr *hdr = (void *)skb->data;
	size_t data_len, hdr_len;
	u8 *data, *mic;

	hdr_len = ieee80211_hdrlen(hdr->frame_control);
	if (skb->len < hdr_len + IEEE80211_CCMP_HDR_LEN ||
	    skb_tailroom(skb) < mic_len)
		return -EINVAL;
	data_len = skb->len - hdr_len - IEEE80211_CCMP_HDR_LEN;
	ccmp_special_blocks(hdr, hdr_len, pn, b_0, aad);

	data = skb->data + hdr_len + IEEE80211_CCMP_HDR_LEN;
	mic = skb_put(skb, mic_len);
	return r92su_ccm_crypt(tfm, true, b_0, aad, data, data_len,
			       mic, mic_len);
}

int ieee80211_aes_ccm_decrypt(struct crypto_aead *tfm, struct sk_buff *skb,
			      const u64 pn, size_t mic_len)
{
	u8 aad[2 * AES_BLOCK_SIZE];
	u8 b_0[AES_BLOCK_SIZE];
	struct ieee80211_hdr *hdr = (void *)skb->data;
	size_t data_len, hdr_len;
	u8 *data, *mic;

	hdr_len = ieee80211_hdrlen(hdr->frame_control);
	/* (the old size_t "data_len <= 0" test could never catch this) */
	if (skb->len <= hdr_len + mic_len)
		return -EINVAL;
	data_len = skb->len - hdr_len - mic_len;

	ccmp_special_blocks(hdr, hdr_len, pn, b_0, aad);

	mic = skb->data + skb->len - mic_len;
	data = skb->data + hdr_len;
	return r92su_ccm_crypt(tfm, false, b_0, aad, data, data_len,
			       mic, mic_len);
}

struct crypto_aead *ieee80211_aes_key_setup_encrypt(const u8 key[],
						    size_t key_len,
						    size_t mic_len)
{
	struct crypto_aead *tfm;
	int err;

	tfm = crypto_alloc_aead("ccm(aes)", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm))
		return tfm;

	err = crypto_aead_setkey(tfm, key, key_len);
	if (err)
		goto free_aead;
	err = crypto_aead_setauthsize(tfm, mic_len);
	if (err)
		goto free_aead;

	return tfm;

free_aead:
	crypto_free_aead(tfm);
	return ERR_PTR(err);
}

void ieee80211_aes_key_free(struct crypto_aead *tfm)
{
	crypto_free_aead(tfm);
}

