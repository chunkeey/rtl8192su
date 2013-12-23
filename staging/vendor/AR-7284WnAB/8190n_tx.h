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


#if !defined(RTL8192SU)
// the purpose if actually just to link up all the desc in the same q
static __inline__ void init_txdesc(struct rtl8190_priv *priv, struct tx_desc *pdesc,
				unsigned long ringaddr, unsigned int i)
{
	#if	defined(RTL8190) || defined(RTL8192E)
	if (i == (NUM_TX_DESC - 1))
		(pdesc + i)->n_desc = set_desc(ringaddr);
	else
		(pdesc + i)->n_desc = set_desc(ringaddr + (i+1) * sizeof(struct tx_desc));

	#elif defined(RTL8192SE)
	if (i == (NUM_TX_DESC - 1))
		(pdesc + i)->Dword9 = set_desc(ringaddr); // NextDescAddress
	else
		(pdesc + i)->Dword9 = set_desc(ringaddr + (i+1) * sizeof(struct tx_desc)); // NextDescAddress

	#endif
}
#endif

static __inline__ unsigned int get_mpdu_len(struct tx_insn *txcfg, unsigned int fr_len)
{
	return (txcfg->hdr_len + txcfg->llc + txcfg->iv + txcfg->icv + txcfg->mic + _CRCLNG_ + fr_len);
}

#if !defined(RTL8192SU)
// the function should be called in critical section
static __inline__ void txdesc_rollover(struct tx_desc *ptxdesc, unsigned int *ptxhead)
{
#ifdef RTL8192SE
	*ptxhead = (*ptxhead + 1) % NUM_TX_DESC;
#else
	*ptxhead = (*ptxhead + 1) & (NUM_TX_DESC - 1);
#endif
}

static __inline__ void txdesc_rollback(unsigned int *ptxhead)
{
	if (*ptxhead == 0)
		*ptxhead = NUM_TX_DESC - 1;
	else
		*ptxhead = *ptxhead - 1;
}

static __inline__ void tx_poll(struct rtl8190_priv *priv, int q_num)
{
	unsigned long ioaddr = priv->pshare->ioaddr;

#ifdef CONFIG_RTL8671
#ifdef CONFIG_CPU_RLX4181
	r3k_flush_dcache_range(0,0);
#endif
#endif

#if	defined(RTL8190) || defined(RTL8192E)
	switch (q_num) {
	case MGNT_QUEUE:
		RTL_W8(_TXPOLL_, POLL_MGT);
		break;
	case BK_QUEUE:
		RTL_W8(_TXPOLL_, POLL_BK);
		break;
	case BE_QUEUE:
		RTL_W8(_TXPOLL_, POLL_BE);
		break;
	case VI_QUEUE:
		RTL_W8(_TXPOLL_, POLL_VI);
		break;
	case VO_QUEUE:
		RTL_W8(_TXPOLL_, POLL_VO);
		break;
	case HIGH_QUEUE:
		RTL_W8(_TXPOLL_, POLL_HIGH);
		break;
	default:
		break;
	}

#elif defined(RTL8192SE)

	switch (q_num) {
	case MGNT_QUEUE:
		RTL_W16(TPPoll, TPPoll_MQ);
		break;
	case BK_QUEUE:
		RTL_W16(TPPoll, TPPoll_BKQ);
		break;
	case BE_QUEUE:
		RTL_W16(TPPoll, TPPoll_BEQ);
		break;
	case VI_QUEUE:
		RTL_W16(TPPoll, TPPoll_VIQ);
		break;
	case VO_QUEUE:
		RTL_W16(TPPoll, TPPoll_VOQ);
		break;
	case HIGH_QUEUE:
		RTL_W16(TPPoll, TPPoll_HQ);
		break;
	default:
		break;
	}
#endif
}
#endif //!RTl8192SU

static __inline__ void desc_copy(struct tx_desc *dst, struct tx_desc *src)
{
#if	defined(RTL8190) || defined(RTL8192E)
	dst->cmd   = src->cmd;
	dst->opt   = src->opt;
	dst->flen  = src->flen;
#elif defined(RTL8192SE) || defined(RTL8192SU)
	memcpy(dst, src, 32);
#endif

}

static __inline__ void descinfo_copy(struct tx_desc_info *dst, struct tx_desc_info *src)
{
	dst->type  = src->type;
	dst->len   = src->len;
	dst->rate  = src->rate;
#ifdef RTL8192SU
	dst->hdr_type = src->hdr_type;
	dst->q_num = src->q_num;
	dst->enc_type = src->enc_type;
#endif
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

