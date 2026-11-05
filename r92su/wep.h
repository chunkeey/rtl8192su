/*
 * Software WEP encryption implementation
 * Copyright 2002, Jouni Malinen <jkmaline@cc.hut.fi>
 * Copyright 2003, Instant802 Networks, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef WEP_H
#define WEP_H

#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <crypto/arc4.h>

/* RC4 state for one key. TX and RX may run at the same time, and each
 * operation rekeys, so the state is used under its lock. */
struct r92su_rc4 {
	spinlock_t lock;
	struct arc4_ctx ctx;
};

struct r92su_rc4 *ieee80211_wep_init(void);
void ieee80211_wep_free(struct r92su_rc4 *tfm);

void ieee80211_wep_encrypt_data(struct r92su_rc4 *tfm, u8 *rc4key,
				size_t klen, u8 *data, size_t data_len);

void ieee80211_wep_encrypt(struct r92su_rc4 *tfm, struct sk_buff *skb,
			   const u8 *key, const u32 iv, int keylen,
			   int keyidx);

int ieee80211_wep_decrypt_data(struct r92su_rc4 *tfm, u8 *rc4key,
			       size_t klen, u8 *data, size_t data_len);

int ieee80211_wep_decrypt(struct r92su_rc4 *tfm, struct sk_buff *skb,
			  const u8 *key, const u32 iv, int keylen,
			  int keyidx);
#endif /* WEP_H */
