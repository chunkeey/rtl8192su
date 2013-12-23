/*
 *      Header file define some tx inline functions
 *
 *      $Id: 8192se_tx.h,v 1.6 2009/08/06 11:41:29 yachang Exp $
 */

#ifndef _8192SE_TX_H_
#define _8192SE_TX_H_

#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./8190n_util.h"
#include "./8190n_hw.h"
#include "./8190n_tx.h"

// the purpose if actually just to link up all the desc in the same q
static __inline__ void init_txdesc_8192SE(struct rtl8190_priv *priv, PTX_DESC_8192SE pdesc,
				unsigned long ringaddr, unsigned int i)
{
	if (i == (NUM_TX_DESC - 1))
		(pdesc + i)->NextDescAddress = set_desc(ringaddr);
	else
		(pdesc + i)->NextDescAddress = set_desc(ringaddr + (i+1) * sizeof(struct _TX_DESC_8192SE));
}

// the function should be called in critical section
static __inline__ void rtl8192SE_txdesc_rollover(PTX_DESC_8192SE ptxdesc, unsigned int *ptxhead)
{
	*ptxhead = (*ptxhead + 1) & (NUM_TX_DESC - 1);
}

static __inline__ void rtl8192SE_txdesc_rollback(unsigned int *ptxhead)
{
	if (*ptxhead == 0)
		*ptxhead = NUM_TX_DESC - 1;
	else
		*ptxhead = *ptxhead - 1;
}

static __inline__ void rtl8192SE_tx_poll(struct rtl8190_priv *priv, int q_num)
{
	unsigned long ioaddr = priv->pshare->ioaddr;
#ifdef CONFIG_RTL8671
#ifdef CONFIG_CPU_RLX4181
	r3k_flush_dcache_range(0,0);
#endif
#endif

	switch (q_num) {
	case MGNT_QUEUE:
		RTL_W8(TPPoll, POLL_MGT);
		break;
	case BK_QUEUE:
		RTL_W8(TPPoll, POLL_BK);
		break;
	case BE_QUEUE:
		RTL_W8(TPPoll, POLL_BE);
		break;
	case VI_QUEUE:
		RTL_W8(TPPoll, POLL_VI);
		break;
	case VO_QUEUE:
		RTL_W8(TPPoll, POLL_VO);
		break;
	case HIGH_QUEUE:
		RTL_W8(TPPoll, POLL_HIGH);
		break;
	default:
		break;
	}
}

static __inline__ void rtl8192SE_desc_copy(PTX_DESC_8192SE dst, PTX_DESC_8192SE src)
{
	dst->cmd   = src->cmd;
	dst->opt   = src->opt;
	dst->flen  = src->flen;
}

static __inline__ void rtl8192SE_descinfo_copy(struct tx_desc_info *dst, struct tx_desc_info *src)
{
	dst->type  = src->type;
	dst->len   = src->len;
	dst->rate  = src->rate;
}


#endif // _8192SE_TX_H_

