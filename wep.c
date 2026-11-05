/* Software WEP encryption implementation
 * Copyright 2002, Jouni Malinen <jkmaline@cc.hut.fi>
 * Copyright 2003, Instant802 Networks, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/crc32.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>
#include <linux/unaligned.h>

#include "wep.h"

struct r92su_rc4 *ieee80211_wep_init(void)
{
	struct r92su_rc4 *rc4 = kzalloc(sizeof(*rc4), GFP_ATOMIC);

	if (!rc4)
		return ERR_PTR(-ENOMEM);
	spin_lock_init(&rc4->lock);
	return rc4;
}

void ieee80211_wep_free(struct r92su_rc4 *tfm)
{
	if (!IS_ERR_OR_NULL(tfm))
		kfree_sensitive(tfm);
}

static void r92su_rc4_crypt(struct r92su_rc4 *rc4, const u8 *key,
			    size_t klen, u8 *data, size_t len)
{
	unsigned long flags;

	spin_lock_irqsave(&rc4->lock, flags);
	arc4_setkey(&rc4->ctx, key, klen);
	arc4_crypt(&rc4->ctx, data, data, len);
	spin_unlock_irqrestore(&rc4->lock, flags);
}

/* Perform WEP encryption using given key. data buffer must have tailroom
 * for 4-byte ICV. data_len must not include this ICV. Note: this function
 * does _not_ add IV. data = RC4(data | CRC32(data))
 */
void ieee80211_wep_encrypt_data(struct r92su_rc4 *tfm, u8 *rc4key,
				size_t klen, u8 *data, size_t data_len)
{
	__le32 icv;

	icv = cpu_to_le32(~crc32_le(~0, data, data_len));
	put_unaligned(icv, (__le32 *)(data + data_len));

	r92su_rc4_crypt(tfm, rc4key, klen, data,
			data_len + IEEE80211_WEP_ICV_LEN);
}


/* Perform WEP encryption on given skb. 4 bytes of extra space (IV) in the
 * beginning of the buffer 4 bytes of extra space (ICV) in the end of the
 * buffer will be added. Both IV and ICV will be transmitted, so the
 * payload length increases with 8 bytes.
 *
 * WEP frame payload: IV + TX key idx, RC4(data), ICV = RC4(CRC32(data))
 */
void ieee80211_wep_encrypt(struct r92su_rc4 *tfm, struct sk_buff *skb,
			   const u8 *key, const u32 iv, int keylen,
			   int keyidx)
{
	struct ieee80211_hdr *hdr = (void *)skb->data;
	void *data;
	size_t len, offset;
	u8 rc4key[3 + WLAN_KEY_LEN_WEP104];

	offset = ieee80211_hdrlen(hdr->frame_control) + IEEE80211_WEP_IV_LEN;
	len = skb->len - offset;
	data = skb->data + offset;

	/* Prepend 24-bit IV to RC4 key */
	rc4key[0] = iv >> 16;
	rc4key[1] = iv >> 8;
	rc4key[2] = iv;

	/* Copy rest of the WEP key (the secret part) */
	memcpy(rc4key + 3, key, keylen);

	/* Add room for ICV */
	skb_put(skb, IEEE80211_WEP_ICV_LEN);

	ieee80211_wep_encrypt_data(tfm, rc4key, keylen + 3, data, len);
}

/* Perform WEP decryption using given key. data buffer includes encrypted
 * payload, including 4-byte ICV, but _not_ IV. data_len must not include ICV.
 * Return 0 on success and -1 on ICV mismatch.
 */
int ieee80211_wep_decrypt_data(struct r92su_rc4 *tfm, u8 *rc4key,
			       size_t klen, u8 *data, size_t data_len)
{
	__le32 crc;

	if (IS_ERR_OR_NULL(tfm))
		return -1;

	r92su_rc4_crypt(tfm, rc4key, klen, data,
			data_len + IEEE80211_WEP_ICV_LEN);

	crc = cpu_to_le32(~crc32_le(~0, data, data_len));
	if (memcmp(&crc, data + data_len, IEEE80211_WEP_ICV_LEN) != 0)
		/* ICV mismatch */
		return -1;

	return 0;
}


/* Perform WEP decryption on given skb. Buffer includes whole WEP part of
 * the frame: IV (4 bytes), encrypted payload (including SNAP header),
 * ICV (4 bytes). skb->len includes ICV.
 *
 * Returns 0 if frame was decrypted successfully and ICV was correct and -1 on
 * failure. If frame is OK, IV and ICV will be removed, i.e., decrypted payload
 * is moved to the beginning of the skb and skb length will be reduced.
 */
int ieee80211_wep_decrypt(struct r92su_rc4 *tfm, struct sk_buff *skb,
			  const u8 *key, const u32 iv, int keylen,
			  int keyidx)
{
	u32 klen;
	u8 rc4key[3 + WLAN_KEY_LEN_WEP104];
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
	void *data;
	size_t len, offset;

	offset = ieee80211_hdrlen(hdr->frame_control);
	if (skb->len < offset + IEEE80211_WEP_ICV_LEN)
		return -1;
	data = skb->data + offset;
	/* _decrypt_data wants the length without the ICV (as TKIP passes it) */
	len = skb->len - offset - IEEE80211_WEP_ICV_LEN;

	klen = 3 + keylen;

	/* Prepend 24-bit IV to RC4 key */
	rc4key[0] = iv >> 16;
	rc4key[1] = iv >> 8;
	rc4key[2] = iv;

	/* Copy rest of the WEP key (the secret part) */
	memcpy(rc4key + 3, key, keylen);

	return ieee80211_wep_decrypt_data(tfm, rc4key, klen, data, len);
}
