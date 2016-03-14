/*
 *      Header file define some tx inline functions
 *
 *      $Id: 8190n_tx.h,v 1.7 2009/08/06 11:41:29 yachang Exp $
 */

#ifndef _8190N_TX_H_
#define _8190N_TX_H_

#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./8190n_util.h"

#ifdef CONFIG_RTK_MESH
#define rtl8190_wlantx(p,t)	rtl8190_firetx(p, t)
#endif

enum _TX_QUEUE_ {
	MGNT_QUEUE		= 0,
	BK_QUEUE		= 1,
	BE_QUEUE		= 2,
	VI_QUEUE		= 3,
	VO_QUEUE		= 4,
	HIGH_QUEUE		= 5,
	BEACON_QUEUE	= 6,
#ifdef RTL8192SU_FWBCN
	TXCMD_QUEUE		= 7,
#endif
};

#define MCAST_QNUM		HIGH_QUEUE



static __inline__ unsigned int get_mpdu_len(struct tx_insn *txcfg, unsigned int fr_len)
{
	return (txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->icv + txcfg->mic + _CRCLNG_ + fr_len);
}


static __inline__ void desc_copy(struct tx_desc *dst, struct tx_desc *src)
{
	memcpy(dst, src, 32);

}

static __inline__ void descinfo_copy(struct tx_desc_info *dst, struct tx_desc_info *src)
{
	dst->type  = src->type;
	dst->len   = src->len;
	dst->rate  = src->rate;
	dst->hdr_type = src->hdr_type;
	dst->q_num = src->q_num;
	dst->enc_type = src->enc_type;
}

#ifdef WDS
#define DECLARE_TXINSN(A)	struct tx_insn A; \
	do {	\
		memset(&A, 0, sizeof(struct tx_insn)); \
		A.wdsIdx  = -1; \
	} while (0)

#define DECLARE_TXCFG(P, TEMPLATE)	struct tx_insn *P = &(TEMPLATE); \
	do {	\
		memset(P, 0, sizeof(struct tx_insn)); \
		P->wdsIdx  = -1; \
	} while (0)

#else
#define DECLARE_TXINSN(A)	struct tx_insn A; \
	do {	\
		memset(&A, 0, sizeof(struct tx_insn)); \
	} while (0)

#define DECLARE_TXCFG(P, TEMPLATE)	struct tx_insn* P = &(TEMPLATE); \
	do {	\
		memset(P, 0, sizeof(struct tx_insn)); \
	} while (0)

#endif // WDS

#endif // _8190N_TX_H_

