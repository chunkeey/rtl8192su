/*
 *      MP routines
 *
 *      $Id: 8190n_mp.c,v 1.9 2009/09/28 13:24:36 cathy Exp $
 */

#define _8190N_MP_C_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/circ_buf.h>
#endif

#include "./8190n_cfg.h"

#ifndef __KERNEL__
#include "./sys-support.h"
#endif

#include "./8190n_headers.h"
#include "./8190n_tx.h"
#include "./8190n_debug.h"

#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
#include <asm/mips16_lib.h>
#endif

#ifdef MP_TEST


#ifdef _LITTLE_ENDIAN_
typedef struct _R_ANTENNA_SELECT_OFDM {
	unsigned int		r_tx_antenna:4;
	unsigned int		r_ant_l:4;
	unsigned int		r_ant_non_ht:4;
	unsigned int		r_ant_ht1:4;
	unsigned int		r_ant_ht2:4;
	unsigned int		r_ant_ht_s1:4;
	unsigned int		r_ant_non_ht_s1:4;
	unsigned int		OFDM_TXSC:2;
	unsigned int		Reserved:2;
} R_ANTENNA_SELECT_OFDM;

typedef struct _R_ANTENNA_SELECT_CCK {
	unsigned char		r_cckrx_enable_2:2;
	unsigned char		r_cckrx_enable:2;
	unsigned char		r_ccktx_enable:4;
} R_ANTENNA_SELECT_CCK;

#else // _BIG_ENDIAN_

typedef struct _R_ANTENNA_SELECT_OFDM {
	unsigned int		Reserved:2;
	unsigned int		OFDM_TXSC:2;
	unsigned int		r_ant_non_ht_s1:4;
	unsigned int		r_ant_ht_s1:4;
	unsigned int		r_ant_ht2:4;
	unsigned int		r_ant_ht1:4;
	unsigned int		r_ant_non_ht:4;
	unsigned int		r_ant_l:4;
	unsigned int		r_tx_antenna:4;
} R_ANTENNA_SELECT_OFDM;

typedef struct _R_ANTENNA_SELECT_CCK {
	unsigned char		r_ccktx_enable:4;
	unsigned char		r_cckrx_enable:2;
	unsigned char		r_cckrx_enable_2:2;
} R_ANTENNA_SELECT_CCK;
#endif


extern int PHYCheckIsLegalRfPath8190Pci(struct rtl8190_priv *priv, unsigned int eRFPath);
static void mp_chk_sw_ant(struct rtl8190_priv *priv);


extern unsigned int TxPwrTrk_OFDM_SwingTbl[TxPwrTrk_OFDM_SwingTbl_Len];
extern unsigned char TxPwrTrk_CCK_SwingTbl[TxPwrTrk_CCK_SwingTbl_Len][8];
extern unsigned char TxPwrTrk_CCK_SwingTbl_CH14[TxPwrTrk_CCK_SwingTbl_Len][8];


#if defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)
#define _GIMR_			0xbd010000
#define _UART_RBR_		0xbd0100c3
#define _UART_LSR_		0xbd0100d7
#endif

#ifdef CONFIG_RTL8671
#ifdef RTL8192SU
#define _GIMR_			0xb8003000
#define _UART_RBR_		0xb8002000
#define _UART_LSR_		0xb8002014
#else
#define _GIMR_			0xb9c03010
#define _UART_RBR_		0xb9c00000
#define _UART_LSR_		0xb9c00014
#endif //RTL8192SU
#endif

#ifdef CONFIG_RTL865X
#define _GIMR_			0xbd012000
#define _ICU_UART0_MSK_	(0x80000000>>4)
#define _ICU_UART1_MSK_ (0x80000000>>3)

#define _UART0_RBR_		0xbd011100
#define _UART0_LSR_		0xbd011114
#define _UART1_RBR_		0xbd011000
#define _UART1_LSR_		0xbd011014
#endif

#ifdef USE_RTL8186_SDK
#define _GIMR_			0xb8003000
#define _ICU_UART0_MSK_	(0x80000000>>4)

#define _UART0_RBR_		0xb8002000
#define _UART0_LSR_		0xb8002014
#endif

#ifdef B2B_TEST
#define MP_PACKET_HEADER		("wlan-tx-test")
#define MP_PACKET_HEADER_LEN	12
#endif


#if defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)
#define DISABLE_UART0_INT() \
	do { \
		writel(readl(_GIMR_) & ~8, _GIMR_); \
		readb(_UART_RBR_); \
		readb(_UART_RBR_); \
	} while (0)

#define RESTORE_UART0_INT() \
	do { \
		writel(readl(_GIMR_) | 8, _GIMR_); \
	} while (0)

static inline int IS_KEYBRD_HIT(void)
{
	if (readb(_UART_LSR_) & 1) { // data ready
		readb(_UART_RBR_);	 // clear rx FIFO
		return 1;
	}
	return 0;
}
#endif // CONFIG_RTL_EB8186 && __KERNEL__


#ifdef CONFIG_RTL8671
#ifdef RTL8192SU
#define RTL_UART_W16(reg, val16) writew ((val16), (unsigned int)((reg)))
#define RTL_UART_W32(reg, val32) writel ((val32), (unsigned int)((reg)))
#define RTL_UART_R16(reg) readw ((unsigned int)((reg)))
#define RTL_UART_R32(reg) readl ((unsigned int)((reg)))

#define DISABLE_UART0_INT() \
	do { \
		ioaddr = 0; \
		RTL_UART_W32(_GIMR_, RTL_UART_R32(_GIMR_) & ~0x1000); \
		RTL_UART_R32(_UART_RBR_); \
		RTL_UART_R32(_UART_RBR_); \
		ioaddr = priv->pshare->ioaddr; \
	} while (0)

#define RESTORE_UART0_INT() \
	do { \
		ioaddr = 0; \
		RTL_UART_W32(_GIMR_, RTL_UART_R32(_GIMR_) | 0x1000); \
		ioaddr = priv->pshare->ioaddr; \
	} while (0)

static inline int IS_KEYBRD_HIT(void)
{
	//int ioaddr = 0;
	if (RTL_UART_R32(_UART_LSR_) & 0x01000000) { // data ready
		RTL_UART_R32(_UART_RBR_);	 // clear rx FIFO
		return 1;
	}
	return 0;
}
#else
#define RTL_UART_W16(reg, val16) writew ((val16), (unsigned int)((reg)))
#define RTL_UART_R16(reg) readw ((unsigned int)((reg)))
#define RTL_UART_R32(reg) readl ((unsigned int)((reg)))

#define DISABLE_UART0_INT() \
	do { \
		RTL_UART_W16(_GIMR_, RTL_UART_R16(_GIMR_) & ~0x8000); \
		RTL_UART_R32(_UART_RBR_); \
		RTL_UART_R32(_UART_RBR_); \
	} while (0)

#define RESTORE_UART0_INT() \
	do { \
		RTL_UART_W16(_GIMR_, RTL_UART_R16(_GIMR_) | 0x8000); \
	} while (0)

static inline int IS_KEYBRD_HIT(void)
{
	if (RTL_UART_R32(_UART_LSR_) & 0x01000000) { // data ready
		RTL_UART_R32(_UART_RBR_);	 // clear rx FIFO
		return 1;
	}
	return 0;
}
#endif //RTL8192SU
#endif // CONFIG_RTL8671


#ifdef CONFIG_RTL865X
#ifdef CONFIG_RTL865XB
int _chip_is_shared_pci_mode(void);

#define DISABLE_UART0_INT() \
	do { \
		if ( _chip_is_shared_pci_mode() == 0 ) \
		{ /* UART0 */ \
			writel(readl(_GIMR_) & ~_ICU_UART0_MSK_, _GIMR_); \
			readl(_UART0_RBR_); \
			readl(_UART0_RBR_); \
		} \
		else \
		{ /* UART1 */ \
			writel(readl(_GIMR_) & ~_ICU_UART1_MSK_, _GIMR_); \
			readl(_UART1_RBR_); \
			readl(_UART1_RBR_); \
		} \
	} while (0);

#define RESTORE_UART0_INT() \
	do { \
		if ( _chip_is_shared_pci_mode() == 0 ) \
		{	/* UART0 */ \
			writel(readl(_GIMR_) | _ICU_UART0_MSK_, _GIMR_); \
		} \
		else \
		{	/* UART1 */ \
			writel(readl(_GIMR_) | _ICU_UART1_MSK_, _GIMR_); \
		} \
	} while (0);
#elif defined(CONFIG_RTL865XC)
#define DISABLE_UART0_INT() \
	do { \
		{ /* UART1 */ \
			writel(readl(_GIMR_) & ~_ICU_UART1_MSK_, _GIMR_); \
			readl(_UART1_RBR_); \
			readl(_UART1_RBR_); \
		} \
	} while (0);

#define RESTORE_UART0_INT() \
	do { \
		{	/* UART1 */ \
			writel(readl(_GIMR_) | _ICU_UART1_MSK_, _GIMR_); \
		} \
	} while (0);
#endif

static inline int IS_KEYBRD_HIT(void)
{
	volatile unsigned char *lsr;
#ifdef CONFIG_RTL865XB
	if ( _chip_is_shared_pci_mode() == 0 )
	{
		/* UART0 */
		lsr = (unsigned char *)_UART0_LSR_;
		if (*lsr & 1){ // data ready
			readb(_UART0_RBR_);	 // clear rx FIFO
			return 1;
		}
	}
	else
#endif
	{
		/* UART1 */
		lsr = (unsigned char *)_UART1_LSR_;
		if (*lsr & 1){ // data ready
			readb(_UART1_RBR_);	 // clear rx FIFO
			return 1;
		}
	}
	return 0;
}
#endif // CONFIG_RTL865X

#ifdef USE_RTL8186_SDK
#ifdef __LINUX_2_6__
#define RTL_UART_R8(reg)		readb((unsigned char *)reg)
#define RTL_UART_R32(reg)		readl((unsigned char *)reg)
#define RTL_UART_W8(reg, val)	writeb(val, (unsigned char *)reg)
#define RTL_UART_W32(reg, val)	writel(val, (unsigned char *)reg)
#else
#define RTL_UART_R8(reg)		readb((unsigned int)reg)
#define RTL_UART_R32(reg)		readl((unsigned int)reg)
#define RTL_UART_W8(reg, val)	writeb(val, (unsigned int)reg)
#define RTL_UART_W32(reg, val)	writel(val, (unsigned int)reg)
#endif

#define DISABLE_UART0_INT() \
	do { \
		RTL_UART_W32(_GIMR_, RTL_UART_R32(_GIMR_) & ~_ICU_UART0_MSK_); \
		RTL_UART_R8(_UART0_RBR_); \
		RTL_UART_R8(_UART0_RBR_); \
	} while (0)

#define RESTORE_UART0_INT() \
	do { \
		RTL_UART_W32(_GIMR_, RTL_UART_R32(_GIMR_) | _ICU_UART0_MSK_); \
	} while (0)

static inline int IS_KEYBRD_HIT(void)
{
	if (RTL_UART_R8(_UART0_LSR_) & 1) { // data ready
		RTL_UART_R8(_UART0_RBR_);	 // clear rx FIFO
		return 1;
	}
	return 0;
}
#endif // USE_RTL8186_SDK


/*
 *  find a token in a string. If succes, return pointer of token next. If fail, return null
 */
static char *get_value_by_token(char *data, char *token)
{
		int idx=0, src_len=strlen(data), token_len=strlen(token);

		while (src_len >= token_len) {
			if (!memcmp(&data[idx], token, token_len))
				return (&data[idx+token_len]);
			src_len--;
			idx++;
		}
		return NULL;
}


static __inline__ int isLegalRate(unsigned int rate)
{
	unsigned int res = 0;

	switch(rate)
	{
	case _1M_RATE_:
	case _2M_RATE_:
	case _5M_RATE_:
	case _6M_RATE_:
	case _9M_RATE_:
	case _11M_RATE_:
	case _12M_RATE_:
	case _18M_RATE_:
	case _24M_RATE_:
	case _36M_RATE_:
	case _48M_RATE_:
	case _54M_RATE_:
		res = 1;
		break;
	case _MCS0_RATE_:
	case _MCS1_RATE_:
	case _MCS2_RATE_:
	case _MCS3_RATE_:
	case _MCS4_RATE_:
	case _MCS5_RATE_:
	case _MCS6_RATE_:
	case _MCS7_RATE_:
	case _MCS8_RATE_:
	case _MCS9_RATE_:
	case _MCS10_RATE_:
	case _MCS11_RATE_:
	case _MCS12_RATE_:
	case _MCS13_RATE_:
	case _MCS14_RATE_:
	case _MCS15_RATE_:
		res = 1;
		break;
	default:
		res = 0;
		break;
	}

	return res;
}


static void mp_RL5975e_Txsetting(struct rtl8190_priv *priv)
{
	RF90_RADIO_PATH_E eRFPath;
	unsigned int rfReg0x14, rfReg0x15, rfReg0x2c;

	// reg0x14
	rfReg0x14 = 0x5ab;
	if (priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
	{
		//channel = 1, 11, in 20MHz mode, set RF-reg[0x14] = 0x59b
		if(priv->pshare->working_channel == 1 || priv->pshare->working_channel == 11)
		{
			if(!is_CCK_rate(priv->pshare->mp_datarate)) //OFDM, MCS rates
				rfReg0x14 = 0x59b;
		}
	}
	else
	{
		//channel = 3, 9, in 40MHz mode, set RF-reg[0x14] = 0x59b
		if(priv->pshare->working_channel == 3 || priv->pshare->working_channel == 9)
			rfReg0x14 = 0x59b;
	}
	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++)
	{
		if (!PHYCheckIsLegalRfPath8190Pci(priv, eRFPath))
			continue;
/*
		if (get_rf_mimo_mode(priv) == MIMO_1T2R) {
			if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B))
				continue;
		}
		else if (get_rf_mimo_mode(priv) == MIMO_2T2R) {
			if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
				continue;
		}
*/
		PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, 0x14, bMask12Bits, rfReg0x14);
		delay_us(100);
	}

	// reg0x15
	rfReg0x15 = 0xf80;
	if(priv->pshare->mp_datarate == 4)
	{
		if(priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
			rfReg0x15 = 0xfc0;
	}
	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++)
	{
		if (!PHYCheckIsLegalRfPath8190Pci(priv, eRFPath))
			continue;
/*
		if (get_rf_mimo_mode(priv) == MIMO_1T2R) {
			if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B))
				continue;
		}
		else if (get_rf_mimo_mode(priv) == MIMO_2T2R) {
			if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
				continue;
		}
*/
		PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, 0x15, bMask12Bits, rfReg0x15);
		delay_us(100);
	}

	//reg0x2c
	if(priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
	{
		rfReg0x2c = 0x3d7;
		if(is_CCK_rate(priv->pshare->mp_datarate)) //CCK rate
		{
			if(priv->pshare->working_channel == 1 || priv->pshare->working_channel == 11)
				rfReg0x2c = 0x3f7;
		}
	}
	else
	{
		rfReg0x2c = 0x3ff;
	}
	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++)
	{
		if (!PHYCheckIsLegalRfPath8190Pci(priv, eRFPath))
			continue;
/*
		if (get_rf_mimo_mode(priv) == MIMO_1T2R) {
			if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B))
				continue;
		}
		else if (get_rf_mimo_mode(priv) == MIMO_2T2R) {
			if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
				continue;
		}
*/
		PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, 0x2c, bMask12Bits, rfReg0x2c);
		delay_us(100);
	}

	if (priv->pshare->rf_ft_var.use_frq_2_3G)
		PHY_SetRFReg(priv, RF90_PATH_C, 0x2c, 0x60, 0);
}


static void mp_RF_RxLPFsetting(struct rtl8190_priv *priv)
{
	unsigned int rfBand_A=0, rfBand_B=0, rfBand_C=0, rfBand_D=0;

	//==================================================
	//because the EVM issue, CCK ACPR spec, asked by bryant.
	//when BAND_20MHZ_MODE, should overwrite CCK Rx path RF, let the bandwidth
	//from 10M->8M, we should overwrite the following values to the cck rx rf.
	//RF_Reg[0xb]:bit[11:8] = 0x4, otherwise RF_Reg[0xb]:bit[11:8] = 0x0
	switch(priv->pshare->mp_antenna_rx)
	{
	case ANTENNA_A:
	case ANTENNA_AC:
	case ANTENNA_ABCD:
		rfBand_A = 0x500; //for TxEVM, CCK ACPR
		break;
	case ANTENNA_B:
	case ANTENNA_BD:
		rfBand_B = 0x500; //for TxEVM, CCK ACPR
		break;
	case ANTENNA_C:
	case ANTENNA_CD:
		rfBand_C = 0x500; //for TxEVM, CCK ACPR
		break;
	case ANTENNA_D:
		rfBand_D = 0x500; //for TxEVM, CCK ACPR
		break;
	}

	if (priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
	{
		if(!rfBand_A)
			rfBand_A = 0x100;
		if(!rfBand_B)
			rfBand_B = 0x100;
		if(!rfBand_C)
			rfBand_C = 0x100;
		if(!rfBand_D)
			rfBand_D = 0x100;
	}
	else
	{
		rfBand_A = 0x300;
		rfBand_B = 0x300;
		rfBand_C = 0x300;
		rfBand_D = 0x300;
	}

	PHY_SetRFReg(priv, RF90_PATH_A, 0x0b, bMask12Bits, rfBand_A);
	delay_us(100);
	PHY_SetRFReg(priv, RF90_PATH_B, 0x0b, bMask12Bits, rfBand_B);
	delay_us(100);
	PHY_SetRFReg(priv, RF90_PATH_C, 0x0b, bMask12Bits, rfBand_C);
	delay_us(100);
	PHY_SetRFReg(priv, RF90_PATH_D, 0x0b, bMask12Bits, rfBand_D);
	delay_us(100);
}


#if defined(RTL8192SE) || defined(RTL8192SU)
static void mp_8192SE_tx_setting(struct rtl8190_priv *priv)
{
	RF90_RADIO_PATH_E eRFPath;

	if (priv->pshare->IC_Class != IC_INFERIORITY_A)
	{
		for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++) {
			if (priv->pshare->CurrentChannelBW) {
				if (priv->pshare->working_channel == 3)
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xa8f5);
				else if ((priv->pshare->working_channel == 9) ||
						(priv->pshare->working_channel == 10) ||
						(priv->pshare->working_channel == 11))
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xf8f6);
				else
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xf8f5);
			}
			else {
				if (priv->pshare->working_channel == 1)
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xa8f5);
				else if (priv->pshare->working_channel == 11)
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xf8f6);
				else
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xf8f5);
			}

			if (is_CCK_rate(priv->pshare->mp_datarate))
				PHY_SetRFReg(priv, eRFPath, 0x26, bMask20Bits, 0x4440);
			else
				PHY_SetRFReg(priv, eRFPath, 0x26, bMask20Bits, 0xf200);
		}
	}
	else
	{
		for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++) {
			if (priv->pshare->CurrentChannelBW) {
				if (priv->pshare->working_channel == 3)
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xa8f4);
				else if (priv->pshare->working_channel == 9)
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xf8f5);
				else
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xf8f4);
				PHY_SetRFReg(priv, eRFPath, 0x26, bMask20Bits, 0xf200);
			}
			else {
				if (priv->pshare->working_channel == 1)
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xa8f4);
				else if (priv->pshare->working_channel == 11)
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xf8f5);
				else
					PHY_SetRFReg(priv, eRFPath, 0x15, bMask20Bits, 0xf8f4);

				if (is_CCK_rate(priv->pshare->mp_datarate))
					PHY_SetRFReg(priv, eRFPath, 0x26, bMask20Bits, 0x4440);
				else
					PHY_SetRFReg(priv, eRFPath, 0x26, bMask20Bits, 0xf200);
			}
		}
	}
}
#endif


static void mp_set_tx_power_by_rate(struct rtl8190_priv *priv)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned int val32;
#ifdef RTL8190
	if (is_MCS_rate(priv->pshare->mp_datarate))
		val32 = priv->pshare->mp_txpwr_mcs;
	else
		val32 = priv->pshare->mp_txpwr_ofdm;

	val32 = (val32 << 24) | (val32 << 16) | (val32 << 8) | val32;
	RTL_W32(_MCS_TXAGC_0_, val32);
	RTL_W32(_MCS_TXAGC_1_, val32);

	val32 = priv->pshare->mp_txpwr_cck;
	val32 = (val32 << 8) | val32;
	RTL_W32(_CCK_TXAGC_, val32);
#elif defined(RTL8192SE)
	if (is_MCS_1SS_rate(priv->pshare->mp_datarate))
		val32 = priv->pshare->mp_txpwr_mcs_1ss;
	else if(is_MCS_2SS_rate(priv->pshare->mp_datarate))
		val32 = priv->pshare->mp_txpwr_mcs_2ss;
	else
		val32 = priv->pshare->mp_txpwr_ofdm;

	val32 = (val32 << 24) | (val32 << 16) | (val32 << 8) | val32;

	RTL_W32(rTxAGC_Rate18_06, val32);
	RTL_W32(rTxAGC_Rate54_24, val32);
	RTL_W32(rTxAGC_Mcs03_Mcs00, val32);
	RTL_W32(rTxAGC_Mcs07_Mcs04, val32);
	RTL_W32(rTxAGC_Mcs11_Mcs08, val32);
	RTL_W32(rTxAGC_Mcs15_Mcs12, val32);
	
	val32 = priv->pshare->mp_txpwr_cck;
	val32 = (val32 << 8) | val32;
	RTL_W16(rTxAGC_CCK_Mcs32, val32);
#elif defined(RTL8192SU)
	if (is_MCS_1SS_rate(priv->pshare->mp_datarate))
		val32 = priv->pshare->mp_txpwr_mcs_1ss;
	else if(is_MCS_2SS_rate(priv->pshare->mp_datarate))
		val32 = priv->pshare->mp_txpwr_mcs_2ss;
	else
		val32 = priv->pshare->mp_txpwr_ofdm;

	val32 = (val32 << 24) | (val32 << 16) | (val32 << 8) | val32;

	PHY_SetBBReg(priv, rTxAGC_Rate18_06, bMaskDWord, val32);
	PHY_SetBBReg(priv, rTxAGC_Rate54_24, bMaskDWord, val32);
	PHY_SetBBReg(priv, rTxAGC_Mcs03_Mcs00, bMaskDWord, val32);
	PHY_SetBBReg(priv, rTxAGC_Mcs07_Mcs04, bMaskDWord, val32);
	PHY_SetBBReg(priv, rTxAGC_Mcs11_Mcs08, bMaskDWord, val32);
	PHY_SetBBReg(priv, rTxAGC_Mcs15_Mcs12, bMaskDWord, val32);
	val32 = priv->pshare->mp_txpwr_cck;
	val32 = (val32 << 8) | val32;
	PHY_SetBBReg(priv, rTxAGC_CCK_Mcs32, bMaskLWord, val32);
#endif
}


static void mpt_StartCckContTx(struct rtl8190_priv *priv)
{
	unsigned int cckrate;
#if defined(RTL8192SE) || defined(RTL8192SU)
// We need to turn off ADC before entering CTX mode
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif
	RTL_W32(0xe70, (RTL_R32(0xe70) & 0xfe1fffff) );
	delay_us(100);
#endif
	// 1. if CCK block on?
	if (!PHY_QueryBBReg(priv, rFPGA0_RFMOD, bCCKEn))
		PHY_SetBBReg(priv, rFPGA0_RFMOD, bCCKEn, bEnable);//set CCK block on

	//Turn Off All Test Mode
	PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
	PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
	//Set CCK Tx Test Rate
	switch (priv->pshare->mp_datarate)
	{
		case 2:
			cckrate = 0;
			break;
		case 4:
			cckrate = 1;
			break;
		case 11:
			cckrate = 2;
			break;
		case 22:
			cckrate = 3;
			break;
		default:
			cckrate = 0;
			break;
	}
	PHY_SetBBReg(priv, rCCK0_System, bCCKTxRate, cckrate);

	PHY_SetBBReg(priv, rCCK0_System, bCCKBBMode, 0x2);    //transmit mode
	PHY_SetBBReg(priv, rCCK0_System, bCCKScramble, 0x1);  //turn on scramble setting
#if defined(RTL8192SE) || defined(RTL8192SU)
	PHY_SetBBReg(priv, 0x820, 0x400, 0x1);
	PHY_SetBBReg(priv, 0x828, 0x400, 0x1);
#endif
}


static void mpt_StopCckCoNtTx(struct rtl8190_priv *priv)
{
#if defined(RTL8192SE) || defined(RTL8192SU)
// We need to turn on ADC before entering CTX mode
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif
	RTL_W32(0xe70, (RTL_R32(0xe70) | 0x01E00000) );
	delay_us(100);
#endif
	PHY_SetBBReg(priv, rCCK0_System, bCCKBBMode, 0x0);    //normal mode
	PHY_SetBBReg(priv, rCCK0_System, bCCKScramble, 0x1);  //turn on scramble setting

	//BB Reset
	PHY_SetBBReg(priv, rPMAC_Reset, bBBResetB, 0x0);
	PHY_SetBBReg(priv, rPMAC_Reset, bBBResetB, 0x1);

#if defined(RTL8192SE) || defined(RTL8192SU)
	PHY_SetBBReg(priv, 0x820, 0x400, 0x0);
	PHY_SetBBReg(priv, 0x828, 0x400, 0x0);
#endif
}


static void mpt_StartOfdmContTx(struct rtl8190_priv *priv)
{

#if defined(RTL8192SE) || defined(RTL8192SU)
// We need to turn off ADC before entering CTX mode
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif
	RTL_W32(0xe70, (RTL_R32(0xe70) & 0xFE1FFFFF ) );
	delay_us(100);
#endif
	// 1. if OFDM block on?
	if (!PHY_QueryBBReg(priv, rFPGA0_RFMOD, bOFDMEn))
		PHY_SetBBReg(priv, rFPGA0_RFMOD, bOFDMEn, bEnable);//set OFDM block on

	// 2. set CCK test mode off, set to CCK normal mode
	PHY_SetBBReg(priv, rCCK0_System, bCCKBBMode, bDisable);

	// 3. turn on scramble setting
	PHY_SetBBReg(priv, rCCK0_System, bCCKScramble, bEnable);

	// 4. Turn On Continue Tx and turn off the other test modes.
	PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMContinueTx, bEnable);
	PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
#if defined(RTL8192SE) || defined(RTL8192SU)
	PHY_SetBBReg(priv, 0x820, 0x400, 0x1);
	PHY_SetBBReg(priv, 0x828, 0x400, 0x1);
#endif
}


static void mpt_StopOfdmContTx(struct rtl8190_priv *priv)
{
#if defined(RTL8192SE) || defined(RTL8192SU)
// We need to turn on ADC before entering CTX mode
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif
	RTL_W32(0xe70, (RTL_R32(0xe70) | 0x01e00000) );
	delay_us(100);
#endif

	PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
	PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
	//Delay 10 ms
	delay_ms(10);
	//BB Reset
	PHY_SetBBReg(priv, rPMAC_Reset, bBBResetB, 0x0);
	PHY_SetBBReg(priv, rPMAC_Reset, bBBResetB, 0x1);
	
#if defined(RTL8192SE) || defined(RTL8192SU)
	PHY_SetBBReg(priv, 0x820, 0x400, 0x0);
	PHY_SetBBReg(priv, 0x828, 0x400, 0x0);
#endif

}


static void mpt_ProSetCarrierSupp(struct rtl8190_priv *priv, int enable)
{
	if (enable)
	{ // Start Carrier Suppression.
		// 1. if CCK block on?
		if(!PHY_QueryBBReg(priv, rFPGA0_RFMOD, bCCKEn))
			PHY_SetBBReg(priv, rFPGA0_RFMOD, bCCKEn, bEnable);//set CCK block on

		//Turn Off All Test Mode
		PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
		PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
		PHY_SetBBReg(priv, rOFDM1_LSTF, bOFDMSingleTone, bDisable);

		PHY_SetBBReg(priv, rCCK0_System, bCCKBBMode, 0x2);    //transmit mode
		PHY_SetBBReg(priv, rCCK0_System, bCCKScramble, 0x0);  //turn off scramble setting
   		//Set CCK Tx Test Rate
		//PHY_SetBBReg(pAdapter, rCCK0_System, bCCKTxRate, pMgntInfo->ForcedDataRate);
		PHY_SetBBReg(priv, rCCK0_System, bCCKTxRate, 0x0);    //Set FTxRate to 1Mbps
	}
	else
	{ // Stop Carrier Suppression.
		PHY_SetBBReg(priv, rCCK0_System, bCCKBBMode, 0x0);    //normal mode
		PHY_SetBBReg(priv, rCCK0_System, bCCKScramble, 0x1);  //turn on scramble setting

		//BB Reset
		PHY_SetBBReg(priv, rPMAC_Reset, bBBResetB, 0x0);
		PHY_SetBBReg(priv, rPMAC_Reset, bBBResetB, 0x1);
	}
}


/*
 * start mp testing. stop beacon and change to mp mode
 */
void mp_start_test(struct rtl8190_priv *priv)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (OPMODE & WIFI_MP_STATE)
	{
		printk("\nFail: already in MP mode\n");
		return;
	}

#ifdef UNIVERSAL_REPEATER
	if (IS_VXD_INTERFACE(priv)) {
		printk("\nFail: only root interface supports MP mode\n");
		return;
	}
	else if (IS_ROOT_INTERFACE(priv) && IS_DRV_OPEN(GET_VXD_PRIV(priv)))
		rtl8190_close(GET_VXD_PRIV(priv)->dev);
#endif

#ifdef MBSSID
	if (
#if defined(RTL8192SE) || defined(RTL8192SU)
		GET_ROOT(priv)->pmib->miscEntry.vap_enable && 
#endif
		IS_VAP_INTERFACE(priv)) {
		printk("\nFail: only root interface supports MP mode\n");
		return;
	}
	else if (IS_ROOT_INTERFACE(priv)) {
		int i;
#if defined(RTL8192SE) || defined(RTL8192SU)
		if (priv->pmib->miscEntry.vap_enable)
#endif
		{
			for (i=0; i<RTL8190_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
					rtl8190_close(priv->pvap_priv[i]->dev);
			}
		}
	}
#endif

	// initialize rate to 54M (or 1M ?)
	priv->pshare->mp_datarate = _54M_RATE_;

	// initialize antenna
	priv->pshare->mp_antenna_tx = ANTENNA_A;
	priv->pshare->mp_antenna_rx = ANTENNA_A;
	mp_chk_sw_ant(priv);

	// initialize swing index
	{
		unsigned int regval, i;

		regval = PHY_QueryBBReg(priv, rOFDM0_XCTxIQImbalance, bMaskDWord);
		for (i=0; i<TxPwrTrk_OFDM_SwingTbl_Len; i++) {
			if (regval == TxPwrTrk_OFDM_SwingTbl[i]) {
				priv->pshare->mp_ofdm_swing_idx = (unsigned char)i;
				break;
			}
		}

		priv->pshare->mp_cck_swing_idx = get_cck_swing_idx(priv->pshare->CurrentChannelBW, priv->pshare->mp_ofdm_swing_idx);
	}

	// change mode to mp mode
	OPMODE |= WIFI_MP_STATE;

	// disable beacon
	RTL_W8(_MSR_, _HW_STATE_NOLINK_);

	priv->pmib->dot11StationConfigEntry.autoRate = 0;
	priv->pmib->dot11StationConfigEntry.protectionDisabled = 1;
	priv->pmib->dot11ErpInfo.ctsToSelf = 0;
	priv->pmib->dot11ErpInfo.protection = 0;
	priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 0;
	OPMODE &= ~WIFI_STATION_STATE;
	OPMODE |= WIFI_AP_STATE;

	// stop site survey
	if (timer_pending(&priv->ss_timer))
		del_timer_sync(&priv->ss_timer);

#if	defined(RTL8190) 
	// stop receiving packets
	RTL_W32(_RCR_, _NO_ERLYRX_);
#elif defined(RTL8192SE) || defined(RTL8192SU)
	// stop receiving packets
	RTL_W32(RCR, _NO_ERLYRX_);
#endif
	// stop dynamic mechanism
	if ((get_rf_mimo_mode(priv) == MIMO_2T4R) && (priv->pmib->dot11BssType.net_work_type != WIRELESS_11B))
		rx_path_by_rssi(priv, NULL, FALSE);
	tx_power_control(priv, NULL, FALSE);

#if	defined(RTL8190)
	// DIG off and set initial gain
	RTL_W32(_RATR_POLL_, 0x800);
	RTL_W8(0xc50, 0x20);
	RTL_W8(0xc58, 0x20);
	RTL_W8(0xc60, 0x20);
	RTL_W8(0xc68, 0x20);
#elif defined(RTL8192SE) || defined(RTL8192SU)
	// DIG off and set initial gain
	RTL_W8(0x364, RTL_R8(0x364) & ~FW_REG364_DIG);
	delay_ms(1);
	RTL_W8(0xc50, 0x1a);
	RTL_W8(0xc58, 0x1a);

	mp_8192SE_tx_setting(priv);
#endif

#ifdef GREEN_HILL
	printk("Enter testing mode\n");
#else
	printk("\nUsage:\n");
	printk("  iwpriv wlanx mp_stop\n");
	printk("  iwpriv wlanx mp_rate {2-108,128-143}\n");
	printk("  iwpriv wlanx mp_channel {1-14}\n");
	printk("  iwpriv wlanx mp_bandwidth [40M={0|1},shortGI={0|1}]\n");
	printk("        - default: 40M=0, shortGI=0\n");
	printk("  iwpriv wlanx mp_txpower [cck=x,ofdm=y,mcs=z]\n");
	printk("        - if no parameters, driver will set tx power according to flash setting.\n");
#if	defined(RTL8190)
	printk("  iwpriv wlanx mp_phypara {xcap=x,antCdiff=y}\n");
#elif defined(RTL8192SE) || defined(RTL8192SU)
	printk("  iwpriv wlanx mp_phypara antBdiff=y\n");
#endif
	printk("  iwpriv wlanx mp_bssid 001122334455\n");
#if	defined(RTL8190)
	printk("  iwpriv wlanx mp_ant_tx {a,c,ac}\n");
	printk("  iwpriv wlanx mp_ant_rx {a,b,c,d,ac,bd,cd,abcd}\n");
#elif defined(RTL8192SE) || defined(RTL8192SU)
	printk("  iwpriv wlanx mp_ant_tx {a,b,ab}\n");
	printk("  iwpriv wlanx mp_ant_rx {a,b,ab}\n");
#endif
	printk("  iwpriv wlanx mp_arx {start,stop}\n");
	printk("  iwpriv wlanx mp_ctx [time=t,count=n,background,stop,pkt,cs,stone]\n");
	printk("        - if \"time\" is set, tx in t sec. if \"count\" is set, tx with n packet.\n");
	printk("        - if \"background\", it will tx continuously until \"stop\" is issued.\n");
	printk("        - if \"pkt\", send cck packet in packet mode (not h/w).\n");
	printk("        - if \"cs\", send cck packet with carrier suppression.\n");
	printk("        - if \"stone\", send packet in single-tone.\n");
	printk("        - default: tx infinitely (no background).\n");
	printk("  iwpriv wlanx mp_query\n");
#ifdef RTL8190
	printk("  iwpriv wlanx mp_tssi\n");
	printk("  iwpriv wlanx mp_pwrtrk [tssi=x]\n");
#elif defined(RTL8192SE) || defined(RTL8192SU)
	printk("  iwpriv wlanx mp_ther\n");
	printk("  iwpriv wlanx mp_pwrtrk [ther={7-29}, stop]\n");
#endif

#ifdef B2B_TEST
	printk("  iwpriv wlanx mp_tx [da=xxxxxx,time=n,count=n,len=n,retry=n,err=n]\n");
	printk("        - if \"time\" is set, tx in t sec. if \"count\" is set, tx with n packet.\n");
	printk("        - if \"time=-1\", tx infinitely.\n");
	printk("        - If \"err=1\", display statistics when tx err.\n");
 	printk("        - default: da=ffffffffffff, time=0, count=1000, len=1500,\n");
 	printk("              retry=6(mac retry limit), err=1.\n");
#if 0
	printk("  iwpriv wlanx mp_rx [ra=xxxxxx,quiet=t,interval=n]\n");
	printk("        - ra: rx mac. defulat is burn-in mac\n");
	printk("        - quiet_time: quit rx if no rx packet during quiet_time. default is 5s\n");
	printk("        - interval: report rx statistics periodically in sec.\n");
	printk("              default is 0 (no report).\n");
#endif
	printk("  iwpriv wlanx mp_brx {start[,ra=xxxxxx],stop}\n");
	printk("        - start: start rx immediately.\n");
 	printk("        - ra: rx mac. defulat is burn-in mac.\n");
	printk("        - stop: stop rx immediately.\n");
#endif
#endif // GREEN_HILL
#ifdef  RTL8192SU
	priv->pshare->drop_pkt=0;
	priv->pshare->tmpTCR=RTL_R32(TCR);
#endif
}


/*
 * stop mp testing. recover system to previous status.
 */
void mp_stop_test(struct rtl8190_priv *priv)
{
	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

#ifdef HIGH_POWER_EXT_PA
	if(priv->pshare->rf_ft_var.use_ext_pa)
	{
		PHY_SetBBReg(priv, 0x878,0xf, 0x0);
		PHY_SetBBReg(priv, 0x878,0x000f0000, 0x0);
		PHY_SetBBReg(priv, 0x870,0x00000400, 0x1); 
		PHY_SetBBReg(priv, 0x860,0x400, 0x0);		//Path A PAPE low
		PHY_SetBBReg(priv, 0x870,0x04000000, 0x1); 			
		PHY_SetBBReg(priv, 0x864,0x400, 0x0);		//Path B PAPE low
		PHY_SetBBReg(priv, 0xee8,0x10000000, 0x0);	
	}
#endif

	OPMODE &= ~WIFI_MP_STATE;
	printk("Please restart the interface\n");
}


/*
 * set data rate
 */
void mp_set_datarate(struct rtl8190_priv *priv, unsigned char *data)
{
	unsigned char rate, rate_org;
	char tmpbuf[32];
#if defined(RTL8192SE) || defined(RTL8192SU)
	RF90_RADIO_PATH_E eRFPath;
#endif

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

	rate = _atoi((char *)data, 10);
	if(!isLegalRate(rate))
	{
		printk("(%d/2) Mbps data rate may not be supported\n", rate);
		return;
	}

	rate_org = priv->pshare->mp_datarate;
	priv->pshare->mp_datarate = rate;
	mp_set_tx_power_by_rate(priv);
#if	defined(RTL8190)
	mp_RL5975e_Txsetting(priv);

	// modify RF reg0xf of path a and c according to Willis
	if (is_CCK_rate(priv->pshare->mp_datarate)) {
		if ((get_rf_mimo_mode(priv) != MIMO_1T2R) && (get_rf_mimo_mode(priv) != MIMO_1T1R)) {
			PHY_SetRFReg(priv, RF90_PATH_A, 0xf, bMask12Bits, 0xff0);
			delay_us(100);
		}
		PHY_SetRFReg(priv, RF90_PATH_C, 0xf, bMask12Bits, 0xff0);
		delay_us(100);
	}
	else {
		if ((get_rf_mimo_mode(priv) != MIMO_1T2R) && (get_rf_mimo_mode(priv) != MIMO_1T1R)) {
			PHY_SetRFReg(priv, RF90_PATH_A, 0xf, bMask12Bits, 0x990);
			delay_us(100);
		}
		PHY_SetRFReg(priv, RF90_PATH_C, 0xf, bMask12Bits, 0x990);
		delay_us(100);
	}
#elif	defined(RTL8192SE) || defined(RTL8192SU)

	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++)
	{
		if (!PHYCheckIsLegalRfPath8190Pci(priv, eRFPath))
			continue;
		if(priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
			PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, rRfChannel, (BIT(11)|BIT(10)), 0x01);
		else
			PHY_SetRFReg(priv, (RF90_RADIO_PATH_E)eRFPath, rRfChannel, (BIT(11)|BIT(10)), 0x00);	
		delay_us(100);
	}

	mp_8192SE_tx_setting(priv);
#endif

	if (rate <= 108)
		sprintf(tmpbuf, "Set data rate to %d Mbps\n", rate/2);
	else
		sprintf(tmpbuf, "Set data rate to MCS %d\n", rate&0x7f);
	printk(tmpbuf);
}


/*
 * set channel
 */
void mp_set_channel(struct rtl8190_priv *priv, unsigned char *data)
{
	unsigned char channel, channel_org;
	char tmpbuf[48];
#if	defined(RTL8190)
	RF90_RADIO_PATH_E eRFPath;
	unsigned int RetryCnt, RfRetVal;
#endif
	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

	channel = (unsigned char)_atoi((char *)data, 10);

	if (priv->pshare->is_40m_bw &&
		((channel < 3) || (channel > 12))) {
		sprintf(tmpbuf, "channel %d is invalid\n", channel);
		printk(tmpbuf);
		return;
	}

	channel_org = priv->pmib->dot11RFEntry.dot11channel;
	priv->pmib->dot11RFEntry.dot11channel = channel;

	if (priv->pshare->rf_ft_var.use_frq_2_3G)
		channel += 14;

#if	defined(RTL8190)
	for (eRFPath = RF90_PATH_A; eRFPath<priv->pshare->phw->NumTotalRFPath; eRFPath++)
	{
		if (!PHYCheckIsLegalRfPath8190Pci(priv, eRFPath))
			continue;
/*
  		if (get_rf_mimo_mode(priv) == MIMO_1T2R) {
			if ((eRFPath == RF90_PATH_A) || (eRFPath == RF90_PATH_B))
				continue;
  		}
		else if (get_rf_mimo_mode(priv) == MIMO_2T2R) {
			if ((eRFPath == RF90_PATH_B) || (eRFPath == RF90_PATH_D))
				continue;
		}
*/
		RetryCnt = 3;
		do {
			PHY_SetRFReg(priv, eRFPath, rZebra1_Channel, bZebra1_ChannelNum, channel);
			delay_ms(10);

			RfRetVal = PHY_QueryRFReg(priv, eRFPath, rZebra1_Channel, bZebra1_ChannelNum, 1);
			RetryCnt--;
		} while ((channel!=RfRetVal) && (RetryCnt!=0));
	}

#elif defined(RTL8192SE) || defined(RTL8192SU)
	{
		unsigned int val_read;
		unsigned int val= channel;
/*
		if (priv->pshare->CurrentChannelBW) {
			if (priv->pshare->offset_2nd_chan == 1)
				val -= 2;
			else
				val += 2;
		}
*/
		val_read = PHY_QueryRFReg(priv, 0, 0x18, bMask12Bits, 1);
		val_read &= 0xfffffff0;
		PHY_SetRFReg(priv, 0, 0x18, bMask12Bits, val_read | val);
		PHY_SetRFReg(priv, 1, 0x18, bMask12Bits, val_read | val);
		channel = val;
	}
	
#endif
	if (priv->pshare->rf_ft_var.use_frq_2_3G)
		channel -= 14;

	priv->pshare->working_channel = channel;
#if	defined(RTL8190)
	mp_RL5975e_Txsetting(priv);
#elif defined(RTL8192SE) || defined(RTL8192SU)
	mp_8192SE_tx_setting(priv);
#endif

	sprintf(tmpbuf, "Change channel %d to channel %d\n", channel_org, channel);
	printk(tmpbuf);
}


/*
 * set tx power
 */
void mp_set_tx_power(struct rtl8190_priv *priv, unsigned char *data)
{
	unsigned int channel = priv->pmib->dot11RFEntry.dot11channel;
	char *val, tmpbuf[64];

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

	if (strlen(data) == 0) {
		if (channel <= 14)
			priv->pshare->mp_txpwr_cck = priv->pmib->dot11RFEntry.pwrlevelCCK[channel-1];

#if	defined(RTL8190)
		priv->pshare->mp_txpwr_mcs = priv->pmib->dot11RFEntry.pwrlevelOFDM[channel-1];
		if (priv->pshare->use_default_para)
			priv->pshare->mp_txpwr_ofdm = priv->pshare->mp_txpwr_mcs; // use 1ss as ofdm default
		else {
			if (priv->pshare->legacyOFDM_pwrdiff & 0x80)
				priv->pshare->mp_txpwr_ofdm = priv->pshare->mp_txpwr_mcs - (priv->pshare->legacyOFDM_pwrdiff & 0x0f);
			else
				priv->pshare->mp_txpwr_ofdm = priv->pshare->mp_txpwr_mcs + (priv->pshare->legacyOFDM_pwrdiff & 0x0f);
		}
#elif defined(RTL8192SE) || defined(RTL8192SU)
		priv->pshare->mp_txpwr_mcs_1ss = priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS[channel-1];
		priv->pshare->mp_txpwr_mcs_2ss = priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS[channel-1];

		if (priv->pshare->use_default_para)
			priv->pshare->mp_txpwr_ofdm = priv->pshare->mp_txpwr_mcs_1ss; // use 1ss as ofdm default
		else {
			if (priv->pshare->legacyOFDM_pwrdiff_A & 0x80) // use pwrdiff_A as main index?
				priv->pshare->mp_txpwr_ofdm = priv->pshare->mp_txpwr_mcs_1ss - (priv->pshare->legacyOFDM_pwrdiff_A & 0x0f);
			else
				priv->pshare->mp_txpwr_ofdm = priv->pshare->mp_txpwr_mcs_1ss + (priv->pshare->legacyOFDM_pwrdiff_A & 0x0f);
		}	
#endif
	}
	else
	{
		val = get_value_by_token((char *)data, "cck=");
		if (val) {
			priv->pshare->mp_txpwr_cck = _atoi(val, 10);
		}

		val = get_value_by_token((char *)data, "ofdm=");
		if (val) {
			priv->pshare->mp_txpwr_ofdm = _atoi(val, 10);
		}

		val = get_value_by_token((char *)data, "mcs=");
		if (val) {
#if	defined(RTL8190)
			priv->pshare->mp_txpwr_mcs = _atoi(val, 10);
#elif defined(RTL8192SE) || defined(RTL8192SU)
			priv->pshare->mp_txpwr_mcs_1ss = _atoi(val, 10);
			priv->pshare->mp_txpwr_mcs_2ss = _atoi(val, 10);
#endif
		}
	}
#if	defined(RTL8190)
	if ((OPMODE & WIFI_MP_CTX_BACKGROUND) && !(OPMODE & WIFI_MP_CTX_PACKET)) {
		// for tx power adjust
		PHY_SetBBReg(priv, rFPGA0_XA_HSSIParameter1, bContTxHSSI, 1);
		PHY_SetBBReg(priv, rFPGA0_XC_HSSIParameter1, bContTxHSSI, 1);
	}
#endif

	mp_set_tx_power_by_rate(priv);

#if	defined(RTL8190)
	sprintf(tmpbuf, "Set power level CCK:%d OFDM:%d MCS:%d\n",
		priv->pshare->mp_txpwr_cck, priv->pshare->mp_txpwr_ofdm, priv->pshare->mp_txpwr_mcs);
#elif defined(RTL8192SE) || defined(RTL8192SU)
	sprintf(tmpbuf, "Set power level CCK:%d OFDM:%d MCS:%d\n",
		priv->pshare->mp_txpwr_cck, priv->pshare->mp_txpwr_ofdm, priv->pshare->mp_txpwr_mcs_1ss);
#endif
	printk(tmpbuf);
}


/*
 * continuous tx
 *  command: "iwpriv wlanx mp_ctx [time=t,count=n,background,stop,pkt,cs,stone]"
 *			  if "time" is set, tx in t sec. if "count" is set, tx with n packet
 *			  if "background", it will tx continuously until "stop" is issue
 *			  if "pkt", send cck packets with packet mode (not hardware)
 *			  if "cs", send cck packet with carrier suppression
 *			  if "stone", send packet in single-tone
 *			  default: tx infinitely (no background)
 */
void mp_ctx(struct rtl8190_priv *priv, unsigned char *data)
{
	unsigned int ioaddr = priv->pshare->ioaddr;
#if defined(RTL8190)
	unsigned int orgTCR = RTL_R32(_TCR_);
#elif defined(RTL8192SE)
	unsigned int orgTCR = RTL_R32(TCR);
#elif defined(RTL8192SU)
	unsigned int orgTCR=0;
	unsigned int nowTCR=0;
#endif
	unsigned char pbuf[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	int count=0, payloadlen=1500, time=-1;
	struct sk_buff *skb;
	struct wlan_ethhdr_t *pethhdr;
	int len, i=0, q_num;
	unsigned char pattern;
	char *val;
	unsigned long end_time=0;
	unsigned long flags=0;
	int tx_from_isr=0, background=0;
#if !defined(RTL8192SU)
	struct rtl8190_hw *phw = GET_HW(priv);
	volatile unsigned int head, tail;
#else
	int cnt1, cnt2;
#endif	
	RF90_RADIO_PATH_E eRFPath;

#ifdef RTL8192SE
// We need to turn off ADC before entering CTX mode
	RTL_W32(0xe70, (RTL_R32(0xe70) & 0xFE1FFFFF ) );
	delay_us(100);
#endif

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

#ifdef RTL8192SU	
	val = get_value_by_token((char *)data, "da=");
	if (val) {
		for(cnt1=0;cnt1<12;cnt1+=2)
		{
				cnt2=cnt1>>1;// /2
				pbuf[cnt2]=((val[cnt1]&0xf)<<4) | (val[cnt1+1]&0xf);
		}
	}
	// get length
	val = get_value_by_token((char *)data, "L=");
	if (val) {
		payloadlen = _atoi(val, 10);
	}
	else
		payloadlen=1500;
#endif

	// get count
	val = get_value_by_token((char *)data, "count=");
	if (val) {
		count = _atoi(val, 10);
		if (count)
			time = 0;
	}

	// get time
	val = get_value_by_token((char *)data, "time=");
	if (val) {
		if (!memcmp(val, "-1", 2))
			time = -1;
		else
			time = _atoi(val, 10)*100;
		if (time > 0)
			end_time = jiffies + time;
	}

	// get background
	val = get_value_by_token((char *)data, "background");
	if (val)
		background = 1;

	// get packet mode
	val = get_value_by_token((char *)data, "pkt");
	if (val)
		OPMODE |= WIFI_MP_CTX_PACKET;

	// get carrier suppression mode
	val = get_value_by_token((char *)data, "cs");
	if (val) {
		if (!is_CCK_rate(priv->pshare->mp_datarate)) {
			printk("Specify carrier suppression but not CCK rate!\n");
			return;
		}
		else
			OPMODE |= WIFI_MP_CTX_CCK_CS;
	}

	// get single-tone
	val = get_value_by_token((char *)data, "stone");
	if (val)
		OPMODE |= WIFI_MP_CTX_ST;

	// get stop
	val = get_value_by_token((char *)data, "stop");
	if (val) {
		unsigned long flags;

		if (!(OPMODE & WIFI_MP_CTX_BACKGROUND)) {
			printk("Error! Continuous-Tx is not on-going.\n");
			return;
		}
		printk("Stop continuous TX\n");

		SAVE_INT_AND_CLI(flags);
		OPMODE &= ~(WIFI_MP_CTX_BACKGROUND | WIFI_MP_CTX_BACKGROUND_PENDING);
		RESTORE_INT(flags);

		delay_ms(1000);
		for (i=0; i<NUM_MP_SKB; i++)
			kfree(priv->pshare->skb_pool[i]->head);
		kfree(priv->pshare->skb_pool_ptr);
#ifdef RTL8192SU
		orgTCR = RTL_R32(TCR);
#endif		

		goto stop_tx;
	}


	// get tx-isr flag, which is set in ISR when Tx ok
	val = get_value_by_token((char *)data, "tx-isr");
	if (val) {
		if (OPMODE & WIFI_MP_CTX_BACKGROUND) {
			if (OPMODE & WIFI_MP_CTX_OFDM_HW)
				return;

			tx_from_isr = 1;
			time = -1;
#ifdef RTL8192SU
		orgTCR=priv->pshare->tmpTCR;
#endif

		}
	}
#ifdef RTL8192SU
	else
	{
		orgTCR = RTL_R32(TCR);
	}
#endif

	if (!tx_from_isr && (OPMODE & WIFI_MP_CTX_BACKGROUND)) {
		printk("Continuous-Tx is on going. You can't issue any tx command except 'stop'.\n");
		return;
	}

	if (background) {
		OPMODE |= WIFI_MP_CTX_BACKGROUND;
		time = -1; // set as infinite

		priv->pshare->skb_pool_ptr = kmalloc(sizeof(struct sk_buff)*NUM_MP_SKB, GFP_KERNEL);
		if (priv->pshare->skb_pool_ptr == NULL) {
			printk("Allocate skb fail!\n");
			return;
		}
		memset(priv->pshare->skb_pool_ptr, 0, sizeof(struct sk_buff)*NUM_MP_SKB);
		for (i=0; i<NUM_MP_SKB; i++) {
			priv->pshare->skb_pool[i] = (struct sk_buff *)(priv->pshare->skb_pool_ptr + i * sizeof(struct sk_buff));
			priv->pshare->skb_pool[i]->head = kmalloc(RX_BUF_LEN, GFP_KERNEL);
			if (priv->pshare->skb_pool[i]->head == NULL) {
				for (i=0; i<NUM_MP_SKB; i++) {
					if (priv->pshare->skb_pool[i]->head)
						kfree(priv->pshare->skb_pool[i]->head);
					else
						break;
				}
				kfree(priv->pshare->skb_pool_ptr);
				printk("Allocate skb fail!\n");
				return;
			}
			else {
				priv->pshare->skb_pool[i]->data = priv->pshare->skb_pool[i]->head;
				priv->pshare->skb_pool[i]->tail = priv->pshare->skb_pool[i]->data;
				priv->pshare->skb_pool[i]->end = priv->pshare->skb_pool[i]->head + RX_BUF_LEN;
				priv->pshare->skb_pool[i]->len = 0;
			}
		}
		priv->pshare->skb_head = 0;
		priv->pshare->skb_tail = 0;
	}

	len = payloadlen + WLAN_ETHHDR_LEN;
	pattern = 0xAB;
	q_num = BE_QUEUE;

#ifdef RTL8192SU
	if (!(OPMODE & WIFI_MP_CTX_PACKET))
	{
		// We need to turn off ADC before entering CTX mode
		RTL_W32(0xe70, (RTL_R32(0xe70) & 0xfe1fffff) );
		delay_us(100);
	}
#endif

	if (!tx_from_isr) {
#ifdef GREEN_HILL
		printk("Start continuous TX");
#else
		if (time < 0) // infinite
			printk("Start continuous DA=%02x%02x%02x%02x%02x%02x len=%d infinite=yes",
				pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5], payloadlen);
			else if (time > 0) // by time
				printk("Start continuous DA=%02x%02x%02x%02x%02x%02x len=%d time=%ds",
					pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
					payloadlen, time/100);
			else // by count
				printk("Start TX DA=%02x%02x%02x%02x%02x%02x len=%d count=%d",
					pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
					payloadlen, count);

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X) || defined(USE_RTL8186_SDK) || defined(RTL8192SU)
		if (!background)
	  		printk(", press any key to escape.\n");
		else
#endif
	  	printk(".\n");
#endif // GREEN_HILL

#if	defined(RTL8190)
		RTL_W32(_IMR_, RTL_R32(_IMR_) | _TBEDOK_);
#endif

		if (OPMODE & WIFI_MP_CTX_PACKET) {
#if	defined(RTL8190)
		  	RTL_W32(_TCR_, RTL_R32(_TCR_) & ~_DISCW_);
#elif defined(RTL8192SE)
			RTL_W32(TCR, RTL_R32(TCR) & ~_DISCW_);
#elif defined(RTL8192SU)
			nowTCR = RTL_R32(TCR) & ~_DISCW_;
			RTL_W32(TCR, nowTCR);
			priv->pshare->tmpTCR=nowTCR;
#endif
			RTL_W32(_ACBE_PARM_, (RTL_R32(_ACBE_PARM_) & 0xffffff00) | (10 + 2 * 20));
		}
		else {
#if	defined(RTL8190)
		  	RTL_W32(_TCR_, RTL_R32(_TCR_) | _DISCW_);
#elif defined(RTL8192SE)
			RTL_W32(TCR, RTL_R32(TCR) | _DISCW_);
#elif defined(RTL8192SU)
			nowTCR = RTL_R32(TCR) | _DISCW_;
			RTL_W32(TCR, nowTCR);
			priv->pshare->tmpTCR=nowTCR;
#endif
			RTL_W32(_ACBE_PARM_, (RTL_R32(_ACBE_PARM_) & 0xffffff00) | 0x01);

			if (is_CCK_rate(priv->pshare->mp_datarate)) {
				if (OPMODE & WIFI_MP_CTX_CCK_CS)
					mpt_ProSetCarrierSupp(priv, TRUE);
				else
					mpt_StartCckContTx(priv);
			}
			else {
				if (!(OPMODE & WIFI_MP_CTX_ST))
					mpt_StartOfdmContTx(priv);
				OPMODE |= WIFI_MP_CTX_OFDM_HW;	// 8190 support h/w OFDM continuous tx
			}
		}
#if	defined(RTL8190)
		RTL_W32(_RCR_, _NO_ERLYRX_);
#elif defined(RTL8192SE) || defined(RTL8192SU)
		RTL_W32(RCR, _NO_ERLYRX_);
#endif

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X) || defined(USE_RTL8186_SDK) || defined(RTL8192SU)
		if (!background)
	  		DISABLE_UART0_INT();
#endif

		memset(&priv->net_stats, 0,  sizeof(struct net_device_stats));
	}

	if (background)
		SAVE_INT_AND_CLI(flags);

	i = 0;
	while (1)
	{

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X) || defined(USE_RTL8186_SDK) || defined(RTL8192SU)
		if (!tx_from_isr && !background && IS_KEYBRD_HIT())
			break;
#endif

		if (time) {
			if (time != -1) {
				if (jiffies > end_time)
					break;
			}
		}
		else {
			if (i >= count)
				break;
		}
		i++;

		if ((OPMODE & WIFI_MP_CTX_ST) &&
			(i == 1)) {
			i++;

#ifdef RTL8190
			if ((get_rf_mimo_mode(priv) == MIMO_1T2R) || (get_rf_mimo_mode(priv) == MIMO_1T1R))
				eRFPath = RF90_PATH_C;
			else
#endif
			{
				switch (priv->pshare->mp_antenna_tx) {
				case ANTENNA_A:
#ifdef RTL8192E
				default:
#endif
					eRFPath = RF90_PATH_A;
					break;
#if defined(RTL8192SE) || defined(RTL8192SU)
				case ANTENNA_B:
				default:
					eRFPath = RF90_PATH_B;
					break;
#elif defined(RTL8190)
				case ANTENNA_C:
				default:
					eRFPath = RF90_PATH_C;
					break;
#endif
				}
			}

#if defined(RTL8192SE) || defined(RTL8192SU)
			// Start Single Tone.	
			PHY_SetBBReg(priv, rFPGA0_RFMOD, bCCKEn, 0x0);
			PHY_SetBBReg(priv, rFPGA0_RFMOD, bOFDMEn, 0x0);
			PHY_SetRFReg(priv, RF90_PATH_A, 0x21, bMask20Bits, 0xd4000);
			delay_us(100);
			if (eRFPath == RF90_PATH_A) {
				PHY_SetRFReg(priv, eRFPath, 0x00, bMask20Bits, 0x20014);
			}
			else {
				PHY_SetRFReg(priv, eRFPath, 0x00, bMask20Bits, 0x20000);	
				RTL_W8(0x87b, 0x80);
			}
			delay_us(100);
#else
			PHY_SetRFReg(priv, eRFPath, 0x00, bMask12Bits, 0x0b0);
			delay_us(100);
			PHY_SetRFReg(priv, eRFPath, 0x01, bMask12Bits, 0x41f);
			delay_us(100);
			PHY_SetRFReg(priv, eRFPath, 0x0c, bMask12Bits, 0xc40);
			delay_us(100);
			PHY_SetRFReg(priv, eRFPath, 0x08, bMask12Bits, 0xe01);
			delay_us(100);
			PHY_SetRFReg(priv, eRFPath, 0x09, bMask12Bits, 0x5f0);
			delay_us(100);
#endif
		}

		if ((OPMODE & WIFI_MP_CTX_OFDM_HW) &&
			(i > 1)) {
			if (background) {
				RESTORE_INT(flags);
				return;
			}
			else
				continue;
		}

		if (background || tx_from_isr) {
			if (CIRC_SPACE(priv->pshare->skb_head, priv->pshare->skb_tail, NUM_MP_SKB) > 1) {
				skb = priv->pshare->skb_pool[priv->pshare->skb_head];
				priv->pshare->skb_head = (priv->pshare->skb_head + 1) & (NUM_MP_SKB - 1);
			}
			else {
				OPMODE |= WIFI_MP_CTX_BACKGROUND_PENDING;
				if (background)
					RESTORE_INT(flags);
				return;
			}
		}
		else
		{
#ifdef RTL8192SU
			if (atomic_read(&priv->tx_pending[BE_QUEUE])<(NUM_TX_DESC>>2))
			{
				priv->pshare->drop_pkt=0;
				delay_ms(5);
			}
			if (atomic_read(&priv->tx_pending[BE_QUEUE])>((NUM_TX_DESC>>2)*3) || priv->pshare->drop_pkt==1)
			{
				priv->pshare->drop_pkt=1;
				if (i>0)i--;
				continue;
			}
#endif
			skb = dev_alloc_skb(len);
		}

		if (skb != NULL)
		{
			DECLARE_TXINSN(txinsn);
#ifdef RTL8190_FASTEXTDEV_FUNCALL
			rtl865x_extDev_pktFromProtocolStack(skb);
#endif
			skb->dev = priv->dev;
			skb_put(skb, len);

			pethhdr = (struct wlan_ethhdr_t *)(skb->data);
			memcpy((void *)pethhdr->daddr, pbuf, MACADDRLEN);
			memcpy((void *)pethhdr->saddr, BSSID, MACADDRLEN);
			pethhdr->type = payloadlen;

			memset(skb->data+WLAN_ETHHDR_LEN, pattern, payloadlen);

			txinsn.q_num	= q_num; //using low queue for data queue
			txinsn.fr_type	= _SKB_FRAME_TYPE_;
			txinsn.pframe	= skb;
			skb->cb[1] = 0;
#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
			skb->fcpu=1;
#endif

			txinsn.tx_rate	= txinsn.lowest_tx_rate = priv->pshare->mp_datarate;
			txinsn.fixed_rate = 1;
			txinsn.retry	= 0;
			txinsn.phdr		= get_wlanllchdr_from_poll(priv);

			memset((void *)txinsn.phdr, 0, sizeof(struct wlanllc_hdr));
			SetFrDs(txinsn.phdr);
			SetFrameType(txinsn.phdr, WIFI_DATA);

			if(rtl8190_firetx(priv, &txinsn) == CONGESTED)
			{
				//printk("Congested\n");
				if (tx_from_isr) {
#if !defined(RTL8192SU)
					head = get_txhead(phw, BE_QUEUE);
					tail = get_txtail(phw, BE_QUEUE);
					if (head == tail) // if Q empty,invoke 1s-timer to send
#else
					if (atomic_read(&priv->tx_pending[BE_QUEUE])==0)
#endif
						OPMODE |= (WIFI_MP_CTX_BACKGROUND | WIFI_MP_CTX_BACKGROUND_PENDING);

					return;
				}
				i--;
				if (txinsn.phdr)
					release_wlanllchdr_to_poll(priv, txinsn.phdr);
				if (skb)
					rtl_kfree_skb(priv, skb, _SKB_TX_);

#if !defined(RTL8192SU)
				if (!tx_from_isr) {
					SAVE_INT_AND_CLI(flags);
					rtl8190_tx_dsr((unsigned long)priv);
					RESTORE_INT(flags);
				}
#endif
			}
		}
		else
		{
			//printk("Can't allocate sk_buff\n");
			if (tx_from_isr) {
#if !defined(RTL8192SU)
				head = get_txhead(phw, BE_QUEUE);
				tail = get_txtail(phw, BE_QUEUE);
				if (head == tail) // if Q empty,invoke 1s-timer to send
#else
				if (atomic_read(&priv->tx_pending[BE_QUEUE])==0)
#endif
					OPMODE |= (WIFI_MP_CTX_BACKGROUND | WIFI_MP_CTX_BACKGROUND_PENDING);
				return;
			}
			i--;
			delay_ms(1);
#ifdef RTL8192SU
			//FIXME!!!, 2009-02-12 chihhsing
#else
			SAVE_INT_AND_CLI(flags);
			rtl8190_tx_dsr((unsigned long)priv);
			RESTORE_INT(flags);
#endif			
		}

		if ((background || tx_from_isr) && (i == NUM_TX_DESC/4)) {
			OPMODE &= ~WIFI_MP_CTX_BACKGROUND_PENDING;
#ifdef RTL8192SU
			priv->pshare->tmpTCR=orgTCR;
#endif
			if (background)
				RESTORE_INT(flags);
			return;
		}
	}

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X) || defined(USE_RTL8186_SDK) || defined(RTL8192SU)
	if (!tx_from_isr && !background)
		RESTORE_UART0_INT();
#endif

stop_tx:

#if	defined(RTL8190)
	RTL_W32(_TCR_, orgTCR);
#elif defined(RTL8192SE) || defined(RTL8192SU)
	RTL_W32(TCR, orgTCR);
// turn on ADC
	RTL_W32(0xe70, (RTL_R32(0xe70) | 0x01e00000) );
	delay_us(100);
#endif

	if (OPMODE & WIFI_MP_CTX_PACKET)
		OPMODE &= ~WIFI_MP_CTX_PACKET;
	else {
		if (is_CCK_rate(priv->pshare->mp_datarate)) {
			if (OPMODE & WIFI_MP_CTX_CCK_CS) {
				OPMODE &= ~WIFI_MP_CTX_CCK_CS;
				mpt_ProSetCarrierSupp(priv, FALSE);
			}
			else
				mpt_StopCckCoNtTx(priv);
		}
		else {
			mpt_StopOfdmContTx(priv);
			if (OPMODE & WIFI_MP_CTX_OFDM_HW)
				OPMODE &= ~WIFI_MP_CTX_OFDM_HW;
			if (OPMODE & WIFI_MP_CTX_ST) {
				OPMODE &= ~WIFI_MP_CTX_ST;

				if ((get_rf_mimo_mode(priv) == MIMO_1T2R) || (get_rf_mimo_mode(priv) == MIMO_1T1R))
					eRFPath = RF90_PATH_C;
				else {
					switch (priv->pshare->mp_antenna_tx) {
					case ANTENNA_A:
						eRFPath = RF90_PATH_A;
						break;
					case ANTENNA_C:
						eRFPath = RF90_PATH_C;
						break;
					default:
						eRFPath = RF90_PATH_C;
						break;
					}
				}
#if defined(RTL8192SE) || defined(RTL8192SU)
				// Stop Single Tone.
				PHY_SetBBReg(priv, rFPGA0_RFMOD, bCCKEn, 0x1);
				PHY_SetBBReg(priv, rFPGA0_RFMOD, bOFDMEn, 0x1);
				PHY_SetRFReg(priv, RF90_PATH_A, 0x21, bMask20Bits, 0x54000);
				delay_us(100);
				PHY_SetRFReg(priv, eRFPath, 0x00, bMask20Bits, 0x30000); // PAD all on.
				delay_us(100);
				if (eRFPath == RF90_PATH_B)
					RTL_W8(0x87b, 0);
#else
				PHY_SetRFReg(priv, eRFPath, 0x01, bMask12Bits, 0xee0);
				delay_us(100);
				PHY_SetRFReg(priv, eRFPath, 0x0c, bMask12Bits, 0x240);
				delay_us(100);
				PHY_SetRFReg(priv, eRFPath, 0x08, bMask12Bits, 0xe1c);
				delay_us(100);
				PHY_SetRFReg(priv, eRFPath, 0x09, bMask12Bits, 0x7f0);
				delay_us(100);
				PHY_SetRFReg(priv, eRFPath, 0x00, bMask12Bits, 0x0bf);
				delay_us(100);
#endif
			}
		}

		PHY_SetBBReg(priv, rFPGA0_XA_HSSIParameter1, bContTxHSSI, 0);
		PHY_SetBBReg(priv, rFPGA0_XC_HSSIParameter1, bContTxHSSI, 0);
	}
}


int mp_query_stats(struct rtl8190_priv *priv, unsigned char *data)
{
	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return 0;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return 0;
	}

	sprintf(data, "Tx OK:%d, Tx Fail:%d, Rx OK:%lu, CRC error:%lu",
		(int)(priv->net_stats.tx_packets-priv->net_stats.tx_errors),
		(int)priv->net_stats.tx_errors,
		priv->net_stats.rx_packets, priv->net_stats.rx_crc_errors);
	return strlen(data)+1;
}


void mp_txpower_tracking(struct rtl8190_priv *priv, unsigned char *data)
{
	char *val;

	if (!netif_running(priv->dev)) {
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE)) {
		printk("Fail: not in MP mode\n");
		return;
	}

#if defined(RTL8190) || defined(RTL8192E)
	char *tmpbuf[8];
	unsigned int i, val32, tssi, target_tssi, tssi_total, reg_backup;
	unsigned char datarate_bkp=0, antenna_tx_bkp=0;
	unsigned char *CCK_SwingEntry;

	val = get_value_by_token((char *)data, "tssi=");
	if (val) {
		target_tssi = _atoi(val, 10);
	}
	else
		target_tssi = priv->pmib->dot11RFEntry.ther_rfic;

	//printk("target_tssi=%d, ofdm_idx=%d, cck_idx=%d\n",
	//	target_tssi, priv->pshare->mp_ofdm_swing_idx, priv->pshare->mp_cck_swing_idx);

	priv->pshare->mp_txpwr_tracking = TRUE;

	// check date rate, must be MCS7
	if (priv->pshare->mp_datarate != _MCS7_RATE_) {
		datarate_bkp = priv->pshare->mp_datarate;
		mp_set_datarate(priv, "135");
	}

	// check tx antenna, use ac to send MCS7 in 2T config
	if ((get_rf_mimo_mode(priv) != MIMO_1T2R) && 
		(get_rf_mimo_mode(priv) != MIMO_1T1R) && 
		(priv->pshare->mp_antenna_tx != ANTENNA_AC)) {
		antenna_tx_bkp = priv->pshare->mp_antenna_tx;
		mp_set_ant_tx(priv, "ac");
	}

	// check 0xc04
	reg_backup = PHY_QueryBBReg(priv, rOFDM0_TRxPathEnable, 0x0000000f);
	PHY_SetBBReg(priv, rOFDM0_TRxPathEnable, 0x0000000f, 0x0000000f);

	// start packet transmitting
	mp_ctx(priv, "pkt,background");

	// Tx power tracking
	while (1) {
		delay_ms(500);

		tssi_total = 0;
		i = 0;
		while (i < 3) {
			delay_ms(5);
			PHY_SetBBReg(priv, rFPGA0_XAB_RFParameter, BIT(25), 0);
			val32 = PHY_QueryBBReg(priv, rFPGA0_XCD_RFInterfaceRB, bMaskDWord);
			PHY_SetBBReg(priv, rFPGA0_XAB_RFParameter, BIT(25), 1);
			tssi = ((val32 & 0x04000000) >> 20) |
				   ((val32 & 0x00600000) >> 17) |
				   ((val32 & 0x00000c00) >> 8)  |
				   ((val32 & 0x00000060) >> 5);
			if (tssi) {
				tssi_total += tssi;
				i++;
			}
		}
		tssi = tssi_total / 3;

		//printk("tssi=%d\n", tssi);

		if (tssi >= target_tssi) {
			if ((tssi - target_tssi) <= TxPwrTrk_E_Val)
				break;
			else {
				if (priv->pshare->mp_ofdm_swing_idx < (TxPwrTrk_OFDM_SwingTbl_Len-1)) {
					priv->pshare->mp_ofdm_swing_idx++;
					priv->pshare->mp_cck_swing_idx = get_cck_swing_idx(priv->pshare->CurrentChannelBW, priv->pshare->mp_ofdm_swing_idx);
				}
				else
					break;
			}
		}
		else {
			if ((target_tssi - tssi) <= TxPwrTrk_E_Val)
				break;
			else {
				if (priv->pshare->mp_ofdm_swing_idx > 0) {
					priv->pshare->mp_ofdm_swing_idx--;
					priv->pshare->mp_cck_swing_idx = get_cck_swing_idx(priv->pshare->CurrentChannelBW, priv->pshare->mp_ofdm_swing_idx);
				}
				else
					break;
			}
		}

		//printk("adjust ofdm_idx=%d, cck_idx=%d\n", priv->pshare->mp_ofdm_swing_idx, priv->pshare->mp_cck_swing_idx);

		val32 = TxPwrTrk_OFDM_SwingTbl[priv->pshare->mp_ofdm_swing_idx];
		PHY_SetBBReg(priv, rOFDM0_XATxIQImbalance, bMaskDWord, val32);
		PHY_SetBBReg(priv, rOFDM0_XBTxIQImbalance, bMaskDWord, val32);
		PHY_SetBBReg(priv, rOFDM0_XCTxIQImbalance, bMaskDWord, val32);
		PHY_SetBBReg(priv, rOFDM0_XDTxIQImbalance, bMaskDWord, val32);
		if (priv->pmib->dot11RFEntry.dot11channel == 14)
			CCK_SwingEntry = &TxPwrTrk_CCK_SwingTbl_CH14[priv->pshare->mp_cck_swing_idx][0];
		else
			CCK_SwingEntry = &TxPwrTrk_CCK_SwingTbl[priv->pshare->mp_cck_swing_idx][0];
		val32 = CCK_SwingEntry[0] | (CCK_SwingEntry[1] << 8);
		PHY_SetBBReg(priv, rCCK0_TxFilter1, bMaskHWord, val32);
		val32 = CCK_SwingEntry[2] | (CCK_SwingEntry[3] << 8) | (CCK_SwingEntry[4] << 16) | (CCK_SwingEntry[5] << 24);
		PHY_SetBBReg(priv, rCCK0_TxFilter2, bMaskDWord, val32);
		val32 = CCK_SwingEntry[6] | (CCK_SwingEntry[7] << 8);
		PHY_SetBBReg(priv, rCCK0_DebugPort, bMaskLWord, val32);
	}

	// stop packet transmitting
	mp_ctx(priv, "stop");

	// recover data rate
	if (datarate_bkp != 0) {
		sprintf(tmpbuf, "%d", datarate_bkp);
		mp_set_datarate(priv, tmpbuf);
	}

	// recover tx antenna
	if (antenna_tx_bkp != 0) {
		switch (antenna_tx_bkp) {
		case ANTENNA_A:
			sprintf(tmpbuf, "a");
			break;
		case ANTENNA_C:
			sprintf(tmpbuf, "c");
			break;
		}
		mp_set_ant_tx(priv, tmpbuf);
	}

	// recover 0xc04
	PHY_SetBBReg(priv, rOFDM0_TRxPathEnable, 0x0000000f, reg_backup);
#else // RTL8192SE, RTL8192SU
	unsigned int target_ther = 0;

	val = get_value_by_token((char *)data, "stop");
	if (val) {
		if (priv->pshare->mp_txpwr_tracking==FALSE)
			return;
		set_fw_reg(priv, 0xfd000018, 0, 0);
		priv->pshare->mp_txpwr_tracking = FALSE;
		printk("mp tx power tracking stop\n");
		return;
	}

	val = get_value_by_token((char *)data, "ther=");
	if (val)
		target_ther = _atoi(val, 10);
	else if (priv->pmib->dot11RFEntry.ther)
		target_ther = priv->pmib->dot11RFEntry.ther;
	target_ther &= 0xff;
	
	if (!target_ther) {
		printk("Fail: tx power tracking has no target thermal value\n");
		return;
	}

	if ((target_ther < 0x07) || (target_ther > 0x1d)) {
		printk("Warning: tx power tracking should have target thermal value 7-29\n");
		if (target_ther < 0x07)
			target_ther = 0x07;
		else
			target_ther = 0x1d;
		printk("Warning: reset target thermal value as %d\n", target_ther);
	}

	set_fw_reg(priv, 0xfd000017 | (target_ther << 8), 0, 0);

	priv->pshare->mp_txpwr_tracking = TRUE;
	printk("mp tx power tracking start, target value=%d\n", target_ther);
#endif
}


int mp_query_tssi(struct rtl8190_priv *priv, unsigned char *data)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif	
	unsigned int i=0, j=0, val32, tssi, tssi_total=0, tssi_reg, reg_backup;

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return 0;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return 0;
	}

	if (priv->pshare->mp_txpwr_tracking) {
		priv->pshare->mp_txpwr_tracking = FALSE;
		sprintf(data, "8651");
		return strlen(data)+1;
	}

	if (is_CCK_rate(priv->pshare->mp_datarate)) {
		reg_backup = RTL_R32(rFPGA0_AnalogParameter4);
		RTL_W32(rFPGA0_AnalogParameter4, (reg_backup & 0xfffff0ff));

		while (i < 5) {
			j++;
			delay_ms(10);
			val32 = PHY_QueryBBReg(priv, rCCK0_TRSSIReport, bMaskByte0);
			tssi = val32 & 0x7f;
			if (tssi > 10) {
				tssi_total += tssi;
				i++;
			}
			if (j > 20)
				break;
		}

		RTL_W32(rFPGA0_AnalogParameter4, reg_backup);

		if (i > 0)
			tssi = tssi_total / i;
		else
			tssi = 0;
	}
	else
	{
		if (priv->pshare->mp_antenna_tx == ANTENNA_A)
			tssi_reg = rFPGA0_XAB_RFInterfaceRB;
		else
			tssi_reg = rFPGA0_XCD_RFInterfaceRB;

		reg_backup = PHY_QueryBBReg(priv, rOFDM0_TRxPathEnable, 0x0000000f);
		PHY_SetBBReg(priv, rOFDM0_TRxPathEnable, 0x0000000f, 0x0000000f);

		PHY_SetBBReg(priv, rFPGA0_XAB_RFParameter, BIT(25), 1);

		while (i < 5) {
			delay_ms(5);
			PHY_SetBBReg(priv, rFPGA0_XAB_RFParameter, BIT(25), 0);
			val32 = PHY_QueryBBReg(priv, tssi_reg, bMaskDWord);
			PHY_SetBBReg(priv, rFPGA0_XAB_RFParameter, BIT(25), 1);
			tssi = ((val32 & 0x04000000) >> 20) |
				   ((val32 & 0x00600000) >> 17) |
				   ((val32 & 0x00000c00) >> 8)  |
				   ((val32 & 0x00000060) >> 5);
			if (tssi) {
				tssi_total += tssi;
				i++;
			}
		}

		PHY_SetBBReg(priv, rOFDM0_TRxPathEnable, 0x0000000f, reg_backup);

		tssi = tssi_total / 5;
	}

	sprintf(data, "%d", tssi);
	return strlen(data)+1;
}


#if defined(RTL8192SE) || defined(RTL8192SU)
int mp_query_ther(struct rtl8190_priv *priv, unsigned char *data)
{
#if !defined(RTL8192SU)
	unsigned long ioaddr = priv->pshare->ioaddr;
#endif
	unsigned int ther=0;

	if (!netif_running(priv->dev)) {
		printk("\nFail: interface not opened\n");
		return 0;
	}

	if (!(OPMODE & WIFI_MP_STATE)) {
		printk("Fail: not in MP mode\n");
		return 0;
	}

	// enable power and trigger
	PHY_SetRFReg(priv, RF90_PATH_A, 0x24, bMask20Bits, 0x60);

	// delay for 1 second
	delay_ms(1000);

	// query rf reg 0x24[4:0], for thermal meter value
	ther = PHY_QueryRFReg(priv, RF90_PATH_A, 0x24, bMask20Bits, 1) & 0x01f;

	sprintf(data, "%d", ther);
	return strlen(data)+1;
}
#endif


#ifdef B2B_TEST
/* Do checksum and verification for configuration data */
static unsigned char byte_checksum(unsigned char *data, int len)
{
	int i;
	unsigned char sum=0;

	for (i=0; i<len; i++)
		sum += data[i];

	sum = ~sum + 1;
	return sum;
}

static int is_byte_checksum_ok(unsigned char *data, int len)
{
	int i;
	unsigned char sum=0;

	for (i=0; i<len; i++)
		sum += data[i];

	if (sum == 0)
		return 1;
	else
		return 0;
}


static void mp_init_sta(struct rtl8190_priv *priv,unsigned char *da_mac)
{
	struct stat_info *pstat;
	unsigned char *da;
#if !defined(RTL8192SU)
	unsigned long ioaddr=priv->pshare->ioaddr;
#endif

	da = da_mac;
	// prepare station info
	if (memcmp(da, "\x0\x0\x0\x0\x0\x0", 6) && !IS_MCAST(da))
	{
		pstat = get_stainfo(priv, da);
		if (pstat == NULL)
		{
			pstat = alloc_stainfo(priv, da, -1);
			pstat->state = WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE;
			memcpy(pstat->bssrateset, AP_BSSRATE, AP_BSSRATE_LEN);
			pstat->bssratelen = AP_BSSRATE_LEN;
			pstat->expire_to = 30000;
			list_add_tail(&pstat->asoc_list, &priv->asoc_list);
			cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);
			if (QOS_ENABLE)
				pstat->QosEnabled = 1;
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				pstat->ht_cap_len = priv->ht_cap_len;
				memcpy(&pstat->ht_cap_buf, &priv->ht_cap_buf, priv->ht_cap_len);
			}
			pstat->current_tx_rate = priv->pshare->mp_datarate;
			update_fwtbl_asoclst(priv, pstat);
			add_update_RATid(priv, pstat);
		}
	}
}


/*
 * tx pakcet.
 *  command: "iwpriv wlanx mp_tx,da=xxx,time=n,count=n,len=n,retry=n,tofr=n,wait=n,delay=n,err=n"
 *		default: da=ffffffffffff, time=0,count=1000, len=1500, retry=6, tofr=0, wait=0, delay=0(ms), err=1
 *		note: if time is set, it will take time (in sec) rather count.
 *		     if "time=-1", tx will continue tx until ESC. If "err=1", display statistics when tx err.
 */
int mp_tx(struct rtl8190_priv *priv, unsigned char *data)
{
#if !defined(RTL8192SU)
	unsigned int ioaddr = priv->pshare->ioaddr;
#endif
	unsigned int orgTCR = RTL_R32(_TCR_);
	unsigned char increaseIFS=0; // set to 1 to increase the inter frame spacing while in PER test
	unsigned char pbuf[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int count=1000, payloadlen=1500, retry=6, tofr=0, wait=0, delay=0, time=0;
	int err=1;
	struct sk_buff *skb;
	struct wlan_ethhdr_t *pethhdr;
	int len, i, q_num, ret, resent;
	unsigned char pattern=0xab;
	char *val;
	struct rtl8190_hw *phw = GET_HW(priv);
	static int last_tx_err;
	unsigned long end_time=0, flags;

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return 0;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return 0;
	}

	// get da
	val = get_value_by_token((char *)data, "da=");
	if (val) {
		ret = get_array_val(pbuf, val, 12);
		if (ret != 6) {
			printk("Error da format\n");
			return 0;
		}
	}

	// get time
	val = get_value_by_token((char *)data, "time=");
	if (val) {
		if (!memcmp(val, "-1", 2))
			time = -1;
		else {
			time = _atoi(val, 10);
			time = time*100;	// in 10ms
		}
	}

	// get count
	val = get_value_by_token((char *)data, "count=");
	if (val) {
		count = _atoi(val, 10);
	}

	// get payload len
	val = get_value_by_token((char *)data, "len=");
	if (val) {
		payloadlen = _atoi(val, 10);
		if (payloadlen < 20) {
			printk("len should be greater than 20!\n");
			return 0;
		}
	}

	// get retry number
	val = get_value_by_token((char *)data, "retry=");
	if (val) {
		retry = _atoi(val, 10);
	}

	// get tofr
	val = get_value_by_token((char *)data, "tofr=");
	if (val) {
		tofr = _atoi(val, 10);
	}

	// get wait
	val = get_value_by_token((char *)data, "wait=");
	if (val) {
		wait = _atoi(val, 10);
	}

	// get err
	val = get_value_by_token((char *)data, "err=");
	if (val) {
		err = _atoi(val, 10);
	}

	len = payloadlen + WLAN_ETHHDR_LEN;
	q_num = BE_QUEUE;

	if (time)
		printk("Start TX DA=%02x%02x%02x%02x%02x%02x len=%d tofr=%d retry=%d wait=%s time=%ds",
			pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
			payloadlen, tofr, retry, (wait ? "yes" : "no"), ((time > 0) ? time/100 : -1));
	else
		printk("Start TX DA=%02x%02x%02x%02x%02x%02x count=%d len=%d tofr=%d retry=%d wait=%s",
			pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
			count, payloadlen, tofr, retry, (wait ? "yes" : "no"));

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X) || defined(USE_RTL8186_SDK)
	printk(", press any key to escape.\n");
#else
	printk("\n");
#endif

	RTL_W32(_IMR_, RTL_R32(_IMR_) | _TBEDOK_);
	RTL_W32(_RCR_, _NO_ERLYRX_);

	if (increaseIFS) {
		RTL_W32(_TCR_, RTL_R32(_TCR_) | _DISCW_);
	}

	memset(&priv->net_stats, 0, sizeof(struct net_device_stats));
	priv->ext_stats.tx_retrys=0;
	last_tx_err = 0;

	if (time > 0) {
		end_time = jiffies + time;
	}

	i = 0;
	resent = 0;

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X) || defined(USE_RTL8186_SDK)
	DISABLE_UART0_INT();
#endif

	mp_init_sta(priv, &pbuf[0]);

	while (1)
	{
#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X) || defined(USE_RTL8186_SDK)
		if ( IS_KEYBRD_HIT())
			break;
#endif

		if (time) {
			if (time != -1) {
				if (jiffies > end_time)
					break;
			}
		}
		else {
			if (!resent && i >= count)
				break;
		}
		if (!resent)
			i++;

		skb = dev_alloc_skb(len);

		if (skb != NULL)
		{
			DECLARE_TXINSN(txinsn);
#ifdef RTL8190_FASTEXTDEV_FUNCALL
			rtl865x_extDev_pktFromProtocolStack(skb);
#endif
			skb->dev = priv->dev;
			skb_put(skb, len);

			pethhdr = (struct wlan_ethhdr_t *)(skb->data);
			memcpy((void *)pethhdr->daddr, pbuf, MACADDRLEN);
			memcpy((void *)pethhdr->saddr, BSSID, MACADDRLEN);
			pethhdr->type = payloadlen;

			// construct tx patten
			memset(skb->data+WLAN_ETHHDR_LEN, pattern, payloadlen);

			memcpy(skb->data+WLAN_ETHHDR_LEN, MP_PACKET_HEADER, MP_PACKET_HEADER_LEN); // header
			memcpy(skb->data+WLAN_ETHHDR_LEN+12, &i, 4); // packet sequence
			skb->data[len-1] = byte_checksum(skb->data+WLAN_ETHHDR_LEN,	payloadlen-1); // checksum

			txinsn.q_num	= q_num; //using low queue for data queue
			txinsn.fr_type	= _SKB_FRAME_TYPE_;
			txinsn.pframe	= skb;
			txinsn.tx_rate	= txinsn.lowest_tx_rate = priv->pshare->mp_datarate;
			txinsn.fixed_rate = 1;
			txinsn.retry	= retry;
			txinsn.phdr		= get_wlanllchdr_from_poll(priv);
			skb->cb[1] = 0;

			memset((void *)txinsn.phdr, 0, sizeof(struct wlanllc_hdr));

			if (tofr & 2)
				SetToDs(txinsn.phdr);
			if (tofr & 1)
				SetFrDs(txinsn.phdr);

			SetFrameType(txinsn.phdr, WIFI_DATA);

#if !defined(RTL8192SU)
			if (wait) {
				while (1) {
					volatile unsigned int head, tail;
					head = get_txhead(phw, BE_QUEUE);
					tail = get_txtail(phw, BE_QUEUE);
					if (head == tail)
						break;
					delay_ms(1);
				}
			}
#else
			//FIXME!!!, 2009-02-02 chihhsing
#endif

			if(rtl8190_firetx(priv, &txinsn) == CONGESTED)
			{
				if (txinsn.phdr)
					release_wlanllchdr_to_poll(priv, txinsn.phdr);
				if (skb)
					rtl_kfree_skb(priv, skb, _SKB_TX_);

				//printk("CONGESTED : busy waiting...\n");
				delay_ms(1);
				resent = 1;

#ifdef RTL8192SU
				//FIXME!!!, 2008-02-12 chihhsing
#else
				SAVE_INT_AND_CLI(flags);
				rtl8190_tx_dsr((unsigned long)priv);
				RESTORE_INT(flags);
#endif
			}
			else {
#ifdef RTL8192SU
				//FIXME!!!, 2008-02-12 chihhsing
#else
				SAVE_INT_AND_CLI(flags);
				rtl8190_tx_dsr((unsigned long)priv);
				RESTORE_INT(flags);
#endif

				if (err && ((int)priv->net_stats.tx_errors) != last_tx_err) { // err happen
					printk("\tout=%d\tfail=%d\n", (int)priv->net_stats.tx_packets,
							(int)priv->net_stats.tx_errors);
					last_tx_err = (int)priv->net_stats.tx_errors;
				}
				else {
					if ( (i%10000) == 0 )
						printk("Tx status: ok=%d\tfail=%d\tretry=%ld\n", (int)(priv->net_stats.tx_packets-priv->net_stats.tx_errors),
							(int)priv->net_stats.tx_errors, priv->ext_stats.tx_retrys);
				}
				resent = 0;
			}
			if (delay)
				delay_ms(delay);
		}
		else
		{
			printk("Can't allocate sk_buff\n");
			delay_ms(1);
			resent = 1;

#if !defined(RTL8192SU)
			SAVE_INT_AND_CLI(flags);
			rtl8190_tx_dsr((unsigned long)priv);
			RESTORE_INT(flags);
#endif
		}
	}

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X) || defined(USE_RTL8186_SDK)
	RESTORE_UART0_INT();
#endif

	// wait till all tx is done
	printk("\nwaiting tx is finished...");
	i = 0;
	while (1) {
#ifdef RTL8192SU
		//FIXME!!!, 2008-02-12 chihhsing
#else
		volatile unsigned int head, tail;
		head = get_txhead(phw, BE_QUEUE);
		tail = get_txtail(phw, BE_QUEUE);
		if (head == tail)
			break;

		SAVE_INT_AND_CLI(flags);
		rtl8190_tx_dsr((unsigned long)priv);
		RESTORE_INT(flags);
#endif

		delay_ms(1);

		if (i++ >10000)
			break;
	}
	printk("done.\n");

	RTL_W32(_TCR_, orgTCR);

	sprintf(data, "Tx result: ok=%d,fail=%d", (int)(priv->net_stats.tx_packets-priv->net_stats.tx_errors),
			(int)priv->net_stats.tx_errors);
	return strlen(data)+1;
}


/*
 * validate rx packet. rx packet format:
 *	|wlan-header(24 byte)|MP_PACKET_HEADER (12 byte)|sequence(4 bytes)|....|checksum(1 byte)|
 *
 */
void mp_validate_rx_packet(struct rtl8190_priv *priv, unsigned char *data, int len)
{
	int tofr = get_tofr_ds(data);
	unsigned int type=GetFrameType(data);
	int header_size = 24;
	unsigned long sequence;
	unsigned short fr_seq;

	if (!priv->pshare->mp_rx_waiting)
		return;

	if (type != WIFI_DATA)
		return;

	fr_seq = GetTupleCache(data);
	if (GetRetry(data) && fr_seq == priv->pshare->mp_cached_seq) {
		priv->pshare->mp_rx_dup++;
		return;
	}

	if (tofr == 3)
		header_size = 30;

	if (len < (header_size+20) )
		return;

	// see if test header matched
	if (memcmp(&data[header_size], MP_PACKET_HEADER, MP_PACKET_HEADER_LEN))
		return;

	priv->pshare->mp_cached_seq = fr_seq;

	memcpy(&sequence, &data[header_size+MP_PACKET_HEADER_LEN], 4);

	if (!is_byte_checksum_ok(&data[header_size], len-header_size)) {
#if 0
		printk("mp_rx: checksum error!\n");
#endif
		printk("mp_brx: checksum error!\n");
	}
	else {
		if (sequence <= priv->pshare->mp_rx_sequence) {
#if 0
			printk("mp_rx: invalid sequece (%ld) <= current (%ld)!\n",
									sequence, priv->pshare->mp_rx_sequence);
#endif
			printk("mp_brx: invalid sequece (%ld) <= current (%ld)!\n",
									sequence, priv->pshare->mp_rx_sequence);
		}
		else {
			if (sequence > (priv->pshare->mp_rx_sequence+1))
				priv->pshare->mp_rx_lost_packet += (sequence-priv->pshare->mp_rx_sequence-1);
			priv->pshare->mp_rx_sequence = sequence;
			priv->pshare->mp_rx_ok++;
		}
	}
}


#if 0
/*
 * Rx test packet.
 *   command: "iwpriv wlanx mp_rx [ra=xxxxxx,quiet=t,interval=n]"
 *	           - ra: rx mac. defulat is burn-in mac
 *             - quiet_time: quit rx if no rx packet during quiet_time. default is 5s
 *             - interval: report rx statistics periodically in sec. default is 0 (no report)
 */
static void mp_rx(struct rtl8190_priv *priv, unsigned char *data)
{
	unsigned int ioaddr = priv->pshare->ioaddr;
	char *val;
	unsigned char pbuf[6];
	int quiet_time=5, interval_time=0, quiet_period=0, interval_period=0, ret;
	unsigned int o_rx_ok, o_rx_lost_packet, mac_changed=0;
	unsigned long reg, counter=0;

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

	// get ra
	val = get_value_by_token((char *)data, "ra=");
	if (val) {
		ret =0;
		if (strlen(val) >=12)
			ret = get_array_val(pbuf, val, 12);
		if (ret != 6) {
			printk("Error mac format\n");
			return;
		}
		printk("set ra to %02X:%02X:%02X:%02X:%02X:%02X\n",
				pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5]);

		memcpy(&reg, pbuf, 4);
		RTL_W32(_IDR0_, (cpu_to_le32(reg)));

		memcpy(&reg, pbuf+4, 4);
		RTL_W32(_IDR0_ + 4, (cpu_to_le32(reg)));
		mac_changed = 1;
	}

	// get quiet time
	val = get_value_by_token((char *)data, "quiet=");
	if (val)
		quiet_time = _atoi(val, 10);

	// get interval time
	val = get_value_by_token((char *)data, "interval=");
	if (val)
		interval_time = _atoi(val, 10);

	RTL_W32(_RCR_, _ENMARP_ | _APWRMGT_ | _AMF_ | _ADF_ | _NO_ERLYRX_ |
			_RX_DMA64_ | _ACRC32_ | _AB_ | _AM_ | _APM_ | _AAP_);

	priv->pshare->mp_cached_seq = 0;
	priv->pshare->mp_rx_ok = 0;
	priv->pshare->mp_rx_sequence = 0;
	priv->pshare->mp_rx_lost_packet = 0;
	priv->pshare->mp_rx_dup = 0;

	printk("Waiting for rx packet, quit if no packet in %d sec", quiet_time);

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X)
	printk(", or press any key to escape.\n");
	DISABLE_UART0_INT();
#else
	printk(".\n");
#endif

	priv->pshare->mp_rx_waiting = 1;

	while (1) {
		// save old counter
		o_rx_ok = priv->pshare->mp_rx_ok;
		o_rx_lost_packet = priv->pshare->mp_rx_lost_packet;

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X)
		if ( IS_KEYBRD_HIT())
			break;
#endif

		delay_ms(1000);

		if (interval_time && ++interval_period == interval_time) {
			printk("\tok=%ld\tlost=%ld\n", priv->pshare->mp_rx_ok, priv->pshare->mp_rx_lost_packet);
			interval_period=0;
		}
		else {
			if ((priv->pshare->mp_rx_ok-counter) > 10000) {
				printk("Rx status: ok=%ld\tlost=%ld, duplicate=%ld\n", priv->pshare->mp_rx_ok, priv->pshare->mp_rx_lost_packet, priv->pshare->mp_rx_dup);
				counter += 10000;
			}
		}

		if (o_rx_ok == priv->pshare->mp_rx_ok && o_rx_lost_packet == priv->pshare->mp_rx_lost_packet)
			quiet_period++;
		else
			quiet_period = 0;

		if (quiet_period >= quiet_time)
			break;
	}

//	printk("\nRx result: ok=%ld\tlost=%ld, duplicate=%ld\n\n", priv->pshare->mp_rx_ok, priv->pshare->mp_rx_lost_packet, priv->mp_rx_dup);
	printk("\nRx reseult: ok=%ld\tlost=%ld\n\n", priv->pshare->mp_rx_ok, priv->pshare->mp_rx_lost_packet);

	priv->pshare->mp_rx_waiting = 0;

	if (mac_changed) {
		memcpy(pbuf, priv->pmib->dot11OperationEntry.hwaddr, MACADDRLEN);

		memcpy(&reg, pbuf, 4);
		RTL_W32(_IDR0_, (cpu_to_le32(reg)));

		memcpy(&reg, pbuf+4, 4);
		RTL_W32(_IDR0_ + 4, (cpu_to_le32(reg)));
	}

#if (defined(CONFIG_RTL_EB8186) && defined(__KERNEL__)) || defined(CONFIG_RTL865X)
	RESTORE_UART0_INT();
#endif

}
#endif


/*
 * Rx test packet.
 *   command: "iwpriv wlanx mp_brx start[,ra=xxxxxx]"
 *	           - ra: rx mac. defulat is burn-in mac
 *   command: "iwpriv wlanx mp_brx stop"
 *               - stop rx immediately
 */
int mp_brx(struct rtl8190_priv *priv, unsigned char *data)
{
	unsigned int ioaddr = priv->pshare->ioaddr;
	char *val;
	unsigned char pbuf[6];
	int ret;
	unsigned long reg;
	unsigned long	flags;

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return 0;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return 0;
	}

	SAVE_INT_AND_CLI(flags);

	if (!strcmp(data, "stop"))
		goto stop_brx;

	// get start
	val = get_value_by_token((char *)data, "start");
	if (val) {
		// get ra if it exists
		val = get_value_by_token((char *)data, "ra=");
		if (val) {
			ret =0;
			if (strlen(val) >=12)
				ret = get_array_val(pbuf, val, 12);
			if (ret != 6) {
				printk("Error mac format\n");
				RESTORE_INT(flags);
				return 0;
			}
			printk("set ra to %02X:%02X:%02X:%02X:%02X:%02X\n",
				pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5]);

			memcpy(&reg, pbuf, 4);
			RTL_W32(_IDR0_, (cpu_to_le32(reg)));

			memcpy(&reg, pbuf+4, 4);
			RTL_W32(_IDR0_ + 4, (cpu_to_le32(reg)));
			priv->pshare->mp_mac_changed = 1;
		}
	}
	else {
		RESTORE_INT(flags);
		return 0;
	}

	priv->pshare->mp_cached_seq = 0;
	priv->pshare->mp_rx_ok = 0;
	priv->pshare->mp_rx_sequence = 0;
	priv->pshare->mp_rx_lost_packet = 0;
	priv->pshare->mp_rx_dup = 0;
	priv->pshare->mp_rx_waiting = 1;

	// record the start time of MP throughput test
	priv->pshare->txrx_start_time = jiffies;

	OPMODE |= WIFI_MP_RX;
	RTL_W32(_RCR_, _ENMARP_ | _APWRMGT_ | _AMF_ | _ADF_ | _NO_ERLYRX_ |
			_RX_DMA64_ | _ACRC32_ | _AB_ | _AM_ | _APM_);

	memset(&priv->net_stats, 0,  sizeof(struct net_device_stats));
	memset(&priv->ext_stats, 0,  sizeof(struct extra_stats));

	RESTORE_INT(flags);
	return 0;

stop_brx:
	OPMODE &= ~WIFI_MP_RX;

	RTL_W32(_RCR_, _ENMARP_ | _NO_ERLYRX_ | _RX_DMA64_);
	priv->pshare->mp_rx_waiting = 0;

	//record the elapsed time of MP throughput test
	priv->pshare->txrx_elapsed_time = jiffies - priv->pshare->txrx_start_time;

	if (priv->pshare->mp_mac_changed) {
		memcpy(pbuf, priv->pmib->dot11OperationEntry.hwaddr, MACADDRLEN);

		memcpy(&reg, pbuf, 4);
		RTL_W32(_IDR0_, (cpu_to_le32(reg)));

		memcpy(&reg, pbuf+4, 4);
		RTL_W32(_IDR0_ + 4, (cpu_to_le32(reg)));
	}
	priv->pshare->mp_mac_changed = 0;

	RESTORE_INT(flags);

	sprintf(data, "Rx reseult: ok=%ld,lost=%ld,elapsed time=%ld",
		priv->pshare->mp_rx_ok, priv->pshare->mp_rx_lost_packet, priv->pshare->txrx_elapsed_time);
	return strlen(data)+1;
}
#endif // B2B_TEST


void mp_set_bandwidth(struct rtl8190_priv *priv, unsigned char *data)
{
	char *val;
	int use40M=0, shortGI=0;
	unsigned int regval, i, val32;
	unsigned char *CCK_SwingEntry;

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

	// get 20M~40M , 40M=1 or 40M=0(20M)
	val = get_value_by_token((char *)data, "40M=");
	if (val) {
		use40M = _atoi(val, 10);
	}

    // get shortGI=1 or 0.
	val = get_value_by_token((char *)data, "shortGI=");
	if (val) {
		shortGI = _atoi(val, 10);
	}

	// modify short GI
	if(shortGI)
	{
		priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M = 1;
		priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M = 1;
	}
	else
	{
		priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M = 0;
		priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M = 0;
	}

	// modify 40M
	if (use40M) {
		priv->pshare->is_40m_bw = 1;
		priv->pshare->CurrentChannelBW = 1;
		priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_BELOW;
	}
	else {
		priv->pshare->is_40m_bw = 0;
		priv->pshare->CurrentChannelBW = 0;
		priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;
	}

	SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);

#if defined(RTL8192SE) || defined(RTL8192SU)
	mp_8192SE_tx_setting(priv);
	return; // end here
#endif

	// check and restore CCK swing
	priv->pshare->mp_cck_swing_idx = get_cck_swing_idx(priv->pshare->CurrentChannelBW, priv->pshare->mp_ofdm_swing_idx);
	regval = PHY_QueryBBReg(priv, rCCK0_TxFilter1, bMaskByte2);
	for (i=0; i<TxPwrTrk_CCK_SwingTbl_Len; i++) {
		if (regval == TxPwrTrk_CCK_SwingTbl[i][0]) {
			break;
		}
	}
	if (priv->pshare->mp_cck_swing_idx != (unsigned char)i) {
		if (priv->pmib->dot11RFEntry.dot11channel == 14)
			CCK_SwingEntry = &TxPwrTrk_CCK_SwingTbl_CH14[priv->pshare->mp_cck_swing_idx][0];
		else
			CCK_SwingEntry = &TxPwrTrk_CCK_SwingTbl[priv->pshare->mp_cck_swing_idx][0];
		val32 = CCK_SwingEntry[0] | (CCK_SwingEntry[1] << 8);
		PHY_SetBBReg(priv, rCCK0_TxFilter1, bMaskHWord, val32);
		val32 = CCK_SwingEntry[2] | (CCK_SwingEntry[3] << 8) | (CCK_SwingEntry[4] << 16) | (CCK_SwingEntry[5] << 24);
		PHY_SetBBReg(priv, rCCK0_TxFilter2, bMaskDWord, val32);
		val32 = CCK_SwingEntry[6] | (CCK_SwingEntry[7] << 8);
		PHY_SetBBReg(priv, rCCK0_DebugPort, bMaskLWord, val32);
	}

	mp_RL5975e_Txsetting(priv);
	mp_RF_RxLPFsetting(priv);
	printk("Set 40M=%d, shortGI=%d\n", priv->pshare->CurrentChannelBW, priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M);
}


/*
 * auto-rx
 * accept CRC32 error pkt
 * accept destination address pkt
 * drop tx pkt (implemented in other functions)
 */
#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif 
int mp_arx(struct rtl8190_priv *priv, unsigned char *data)
{
#if !defined(RTL8192SU)
	unsigned int ioaddr = priv->pshare->ioaddr;
#endif

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return 0;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return 0;
	}

	if (!strcmp(data, "start")) {
		OPMODE |= WIFI_MP_RX;
#if	defined(RTL8190)
		RTL_W32(_RCR_, _ENMARP_ | _APWRMGT_ | _AMF_ | _ADF_ | _NO_ERLYRX_ |
			_RX_DMA64_ | _ACRC32_ | _AB_ | _AM_ | _APM_ | _AAP_);
#elif defined(RTL8192SE)
		RTL_W32(RCR, RCR_APWRMGT | RCR_AMF | RCR_ADF | _NO_ERLYRX_ | RCR_ACRC32 |RCR_AB | RCR_AM | RCR_APM | RCR_AAP);
#elif defined(RTL8192SU)
		RTL_W32(RCR, RCR_APWRMGT | RCR_AMF | RCR_ADF | _NO_ERLYRX_ | RCR_AB | RCR_AM | RCR_APM | RCR_AAP);
#endif
		if (priv->pshare->rf_ft_var.use_frq_2_3G)
			PHY_SetBBReg(priv, rCCK0_System, bCCKEqualizer, 0x0);

		memset(&priv->net_stats, 0,  sizeof(struct net_device_stats));
		memset(&priv->ext_stats, 0,  sizeof(struct extra_stats));
	}
	else if (!strcmp(data, "stop")) {
		OPMODE &= ~WIFI_MP_RX;
#if	defined(RTL8190)
	  	RTL_W32(_RCR_, _ENMARP_ | _NO_ERLYRX_ | _RX_DMA64_);
#elif defined(RTL8192SE) || defined(RTL8192SU)
		RTL_W32(RCR, _NO_ERLYRX_);
#endif

		if (priv->pshare->rf_ft_var.use_frq_2_3G)
			PHY_SetBBReg(priv, rCCK0_System, bCCKEqualizer, 0x1);

		sprintf(data, "Received packet OK:%lu  CRC error:%lu\n", priv->net_stats.rx_packets, priv->net_stats.rx_crc_errors);
		return strlen(data)+1;
	}
	return 0;
}


#if defined(CONFIG_RTL865X) && defined(CONFIG_RTL865X_CLE)
/*
 * flash write
 */
unsigned char rtl8190mp_cmd[256];
extern void rtl8651_8185flashCfg(char *cmd, unsigned int cmdLen);
void mp_flash_cfg(struct rtl8190_priv *priv, unsigned char *data)
{
	int index=priv->dev->name[strlen(priv->dev->name)-1]-'0';
	sprintf(rtl8190mp_cmd,"%d ", index);
	strcat(rtl8190mp_cmd, data);
	rtl8651_8185flashCfg(rtl8190mp_cmd, strlen(rtl8190mp_cmd));
	return;
}
#endif


/*
 * set bssid
 */
void mp_set_bssid(struct rtl8190_priv *priv, unsigned char *data)
{
	unsigned char pbuf[6];
	int ret;

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

	ret = get_array_val(pbuf, (char *)data, strlen(data));

	if (ret != 6) {
		printk("Error bssid format\n");
		return;
	}
	else
		printk("set bssid to %02X:%02X:%02X:%02X:%02X:%02X\n",
			pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5]);

	memcpy(BSSID, pbuf, MACADDRLEN);
}


static void mp_chk_sw_ant(struct rtl8190_priv *priv)
{
	R_ANTENNA_SELECT_OFDM	*p_ofdm_tx;	/* OFDM Tx register */
	R_ANTENNA_SELECT_CCK	*p_cck_txrx;
	unsigned char			r_rx_antenna_ofdm=0, r_ant_select_cck_val=0;
	unsigned char			chgTx=0, chgRx=0;
	unsigned int			r_ant_sel_cck_val=0, r_ant_select_ofdm_val=0, r_ofdm_tx_en_val=0;

	p_ofdm_tx = (R_ANTENNA_SELECT_OFDM *)&r_ant_select_ofdm_val;
	p_cck_txrx = (R_ANTENNA_SELECT_CCK *)&r_ant_select_cck_val;

	p_ofdm_tx->r_ant_ht1			= 0x1;

#if	defined(RTL8190)
	p_ofdm_tx->r_ant_non_ht 		= 0x5;
	p_ofdm_tx->r_ant_ht2			= 0x4;
#elif defined(RTL8192SE) || defined(RTL8192SU)
	p_ofdm_tx->r_ant_non_ht 		= 0x3;
	p_ofdm_tx->r_ant_ht2			= 0x2;
#endif

	// 原因是Tx 3-wire enable需隨Tx Ant path打開才會開啟，
	// 所以需在設BB 0x824與0x82C時，同時將BB 0x804[3:0]設為3(同時打開Ant. A and B)。
	// 要省電的情況下，A Tx時 0x90C=0x11111111，B Tx時 0x90C=0x22222222，AB同時開就維持原來設定0x3321333
	

	switch(priv->pshare->mp_antenna_tx)
	{
	case ANTENNA_A:
		p_ofdm_tx->r_tx_antenna		= 0x1;
		r_ofdm_tx_en_val			= 0x1;
		p_ofdm_tx->r_ant_l 			= 0x1;
		p_ofdm_tx->r_ant_ht_s1 		= 0x1;
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x1;
		p_cck_txrx->r_ccktx_enable	= 0x8;
		chgTx = 1;
#if defined(RTL8192SE) || defined(RTL8192SU)
		// From SD3 Willis suggestion !!! Set RF A=TX and B as standby
			PHY_SetBBReg(priv, rFPGA0_XA_HSSIParameter2, 0xe, 2);
			PHY_SetBBReg(priv, rFPGA0_XB_HSSIParameter2, 0xe, 1);
			r_ofdm_tx_en_val			= 0x3;
			// Power save
			r_ant_select_ofdm_val = 0x11111111;

			PHY_SetBBReg(priv, 0x878,0xf, 0x2); // for CCK TX
#ifdef HIGH_POWER_EXT_PA
			if(priv->pshare->rf_ft_var.use_ext_pa)
			{
				PHY_SetBBReg(priv, 0x878,0x000f0000, 0x0);
				PHY_SetBBReg(priv, 0x870,0x00000400, 0x0);
				PHY_SetBBReg(priv, 0xee8,0x10000000, 0x1);			
			}
#endif
			// turn off PATH B
			PHY_SetBBReg(priv, 0x870,0x04000000, 0x1); // sw control
			PHY_SetBBReg(priv, 0x864,0x400, 0x0);
			
/*
			// 2008/10/31 MH From SD3 Willi's suggestion. We must read RFA 1T table.
			if ((pHalData->VersionID == VERSION_8192S_ACUT)) // For RTL8192SU A-Cut only, by Roger, 2008.11.07.
			{	
				mpt_RFConfigFromPreParaArrary(pAdapter, 0, RF90_PATH_A);
			}
*/
#endif
		break;
	case ANTENNA_B:
		p_ofdm_tx->r_tx_antenna		= 0x2;
		r_ofdm_tx_en_val			= 0x2;
		p_ofdm_tx->r_ant_l 			= 0x2;
		p_ofdm_tx->r_ant_ht_s1 		= 0x2;
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x2;
		p_cck_txrx->r_ccktx_enable	= 0x4;
		chgTx = 1;
#if defined(RTL8192SE) || defined(RTL8192SU)
			PHY_SetBBReg(priv, rFPGA0_XA_HSSIParameter2, 0xe, 1);
			PHY_SetBBReg(priv, rFPGA0_XB_HSSIParameter2, 0xe, 2);
			r_ofdm_tx_en_val			= 0x3;
			// Power save
			r_ant_select_ofdm_val = 0x22222222;

#ifdef HIGH_POWER_EXT_PA
			if(priv->pshare->rf_ft_var.use_ext_pa)
			{
				PHY_SetBBReg(priv, 0x878,0xf, 0x0);
				PHY_SetBBReg(priv, 0x870,0x00000400, 0x1); 
				PHY_SetBBReg(priv, 0x860,0x400, 0x0);				//Path A PAPE low
				PHY_SetBBReg(priv, 0xee8,0x10000000, 0x0);			
			}
#endif
			PHY_SetBBReg(priv, 0x878,0x000f0000, 0x2);
			
			// turn on PATHB
			if((PHY_QueryBBReg(priv, 0x870, bMaskDWord) & BIT(26)) != 0){ //sw control
				PHY_SetBBReg(priv, 0x864,0x400, 0x1); // turn on
				PHY_SetBBReg(priv, 0x870,0x04000000, 0x0); // hw control
			}
#endif
		break;
	case ANTENNA_C:
		p_ofdm_tx->r_tx_antenna		= 0x4;
		r_ofdm_tx_en_val			= 0x4;
		p_ofdm_tx->r_ant_l 			= 0x4;
		p_ofdm_tx->r_ant_ht_s1 		= 0x4;
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x4;
		p_cck_txrx->r_ccktx_enable	= 0x2;
		chgTx = 1;
		break;
	case ANTENNA_D:
		p_ofdm_tx->r_tx_antenna		= 0x8;
		r_ofdm_tx_en_val			= 0x8;
		p_ofdm_tx->r_ant_l 			= 0x8;
		p_ofdm_tx->r_ant_ht_s1 		= 0x8;
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x8;
		p_cck_txrx->r_ccktx_enable	= 0x1;
		chgTx = 1;
		break;
	case ANTENNA_AC:
		p_ofdm_tx->r_tx_antenna		= 0x5;
		r_ofdm_tx_en_val			= 0x5;
		p_ofdm_tx->r_ant_l 			= 0x5;
		p_ofdm_tx->r_ant_ht_s1 		= 0x5;
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x5;
		p_cck_txrx->r_ccktx_enable	= 0xA;
		chgTx = 1;
		break;
		
#if defined(RTL8192SE) || defined(RTL8192SU)
	case ANTENNA_AB:
		p_ofdm_tx->r_tx_antenna		= 0x3;
		r_ofdm_tx_en_val			= 0x3;
		p_ofdm_tx->r_ant_l 			= 0x3;
		p_ofdm_tx->r_ant_ht_s1 		= 0x3;
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x3;
		p_cck_txrx->r_ccktx_enable	= 0xC;
		chgTx = 1;

		// turn on PATHB
		if((PHY_QueryBBReg(priv, 0x870, bMaskDWord) & BIT(26)) != 0){ //sw control
			PHY_SetBBReg(priv, 0x864,0x400, 0x1); // turn on
			PHY_SetBBReg(priv, 0x870,0x04000000, 0x0); // hw control
		}

#ifdef HIGH_POWER_EXT_PA
		if(priv->pshare->rf_ft_var.use_ext_pa)
		{
			PHY_SetBBReg(priv, 0x878,0xf, 0x2);
			PHY_SetBBReg(priv, 0x878,0x000f0000, 0x2);
			PHY_SetBBReg(priv, 0x870,0x00000400, 0x0);		//b10
			PHY_SetBBReg(priv, 0xee8,0x10000000, 0x1);
		}
#endif

		// From SD3 Willis suggestion !!! Set RF B as standby
		PHY_SetBBReg(priv, rFPGA0_XA_HSSIParameter2, 0xe, 2);
		PHY_SetBBReg(priv, rFPGA0_XB_HSSIParameter2, 0xe, 2);
		// Disable Power save
		r_ant_select_ofdm_val = 0x3321333;
/*
		// 2008/10/31 MH From SD3 Willi's suggestion. We must read RFA 2T table.
		if ((pHalData->VersionID == VERSION_8192S_ACUT)) // For RTL8192SU A-Cut only, by Roger, 2008.11.07.
		{
			mpt_RFConfigFromPreParaArrary(pAdapter, 1, RF90_PATH_A);
		}
*/	
		break;
#endif
	default:
		break;
	}

	//
	// r_rx_antenna_ofdm, bit0=A, bit1=B, bit2=C, bit3=D
	// r_cckrx_enable : CCK default, 0=A, 1=B, 2=C, 3=D
	// r_cckrx_enable_2 : CCK option, 0=A, 1=B, 2=C, 3=D
	//
	switch(priv->pshare->mp_antenna_rx)
	{
	case ANTENNA_A:
		r_rx_antenna_ofdm 			= 0x1;	// A
		p_cck_txrx->r_cckrx_enable 	= 0x0;	// default: A
		p_cck_txrx->r_cckrx_enable_2= 0x0;	// option: A
		chgRx = 1;
#if defined(RTL8192SE) || defined(RTL8192SU)
		PHY_SetRFReg(priv, 0, 7, 0x3, 0x0);
#endif
		break;
	case ANTENNA_B:
		r_rx_antenna_ofdm 			= 0x2;	// B
		p_cck_txrx->r_cckrx_enable 	= 0x1;	// default: B
		p_cck_txrx->r_cckrx_enable_2= 0x1;	// option: B
		chgRx = 1;
#if defined(RTL8192SE) || defined(RTL8192SU)
		PHY_SetRFReg(priv, 0, 7, 0x3, 0x1);
#endif
		break;
	case ANTENNA_C:
		r_rx_antenna_ofdm 			= 0x4;	// C
		p_cck_txrx->r_cckrx_enable 	= 0x2;	// default: C
		p_cck_txrx->r_cckrx_enable_2= 0x2;	// option: C
		chgRx = 1;
		break;
	case ANTENNA_D:
		r_rx_antenna_ofdm 			= 0x8;	// D
		p_cck_txrx->r_cckrx_enable 	= 0x3;	// default: D
		p_cck_txrx->r_cckrx_enable_2= 0x3;	// option: D
		chgRx = 1;
		break;
	case ANTENNA_AC:
		r_rx_antenna_ofdm 			= 0x5;	// AC
		p_cck_txrx->r_cckrx_enable 	= 0x0;	// default: A
		p_cck_txrx->r_cckrx_enable_2= 0x2;	// option: C
		chgRx = 1;
		break;
	case ANTENNA_BD:
		r_rx_antenna_ofdm 			= 0xA;	// BD
		p_cck_txrx->r_cckrx_enable 	= 0x1;	// default: B
		p_cck_txrx->r_cckrx_enable_2= 0x3;	// option: D
		chgRx = 1;
		break;
#if defined(RTL8192SE) || defined(RTL8192SU)
	case ANTENNA_AB:	// For 8192S and 8192E/U...
		r_rx_antenna_ofdm 			= 0x3;	// AB
		p_cck_txrx->r_cckrx_enable 	= 0x0;	// default:A
		p_cck_txrx->r_cckrx_enable_2= 0x1;		// option:B
		chgRx = 1;
		break;
#endif
	case ANTENNA_CD:
		r_rx_antenna_ofdm 			= 0xC;	// CD
		p_cck_txrx->r_cckrx_enable 	= 0x2;	// default: C
		p_cck_txrx->r_cckrx_enable_2= 0x3;	// option: D
		chgRx = 1;
		break;
	case ANTENNA_ABCD:
		r_rx_antenna_ofdm 			= 0xF;	// ABCD
		p_cck_txrx->r_cckrx_enable 	= 0x0;	// default: A
		p_cck_txrx->r_cckrx_enable_2= 0x2;	// option: C
		chgRx = 1;
		break;
	}

	if(chgTx && chgRx)
	{
		r_ant_sel_cck_val = r_ant_select_cck_val;//(r_ant_select_cck_val<<24);
		PHY_SetBBReg(priv, rFPGA1_TxInfo, 0x0fffffff, r_ant_select_ofdm_val);		//OFDM Tx
		PHY_SetBBReg(priv, rFPGA0_TxInfo, 0x0000000f, r_ofdm_tx_en_val);		//OFDM Tx
		PHY_SetBBReg(priv, rOFDM0_TRxPathEnable, 0x0000000f, r_rx_antenna_ofdm);	//OFDM Rx
		PHY_SetBBReg(priv, rOFDM1_TRxPathEnable, 0x0000000f, r_rx_antenna_ofdm);	//OFDM Rx
		PHY_SetBBReg(priv, rCCK0_AFESetting, bMaskByte3, r_ant_sel_cck_val);		//CCK TxRx
	}

#if !defined(RTL8192SE) && !defined(RTL8192SU) // we dont know how to set for 92SE!!
	mp_RF_RxLPFsetting(priv);
#endif
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
void mp_set_ant_tx(struct rtl8190_priv *priv, unsigned char *data)
{
	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

#if	defined(RTL8190)
	if (!strcmp(data, "a"))
		priv->pshare->mp_antenna_tx = ANTENNA_A;
	else if (!strcmp(data, "c"))
		priv->pshare->mp_antenna_tx = ANTENNA_C;
	else if (!strcmp(data, "ac"))
		priv->pshare->mp_antenna_tx = ANTENNA_AC;
	else {
		printk("Usage: mp_ant_tx {a,c,ac}\n");
		return;
	}
#elif defined(RTL8192SE) || defined(RTL8192SU)
	if (!strcmp(data, "a"))
		priv->pshare->mp_antenna_tx = ANTENNA_A;
	else if (!strcmp(data, "b"))
		priv->pshare->mp_antenna_tx = ANTENNA_B;
	else if  (!strcmp(data, "ab"))
		priv->pshare->mp_antenna_tx = ANTENNA_AB;
	else {
		printk("Usage: mp_ant_tx {a,b,ab}\n");
		return;
	}
#endif

	mp_chk_sw_ant(priv);
	printk("switch Tx antenna to %s\n", data);
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
void mp_set_ant_rx(struct rtl8190_priv *priv, unsigned char *data)
{
	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}

	if (!strcmp(data, "a"))
		priv->pshare->mp_antenna_rx = ANTENNA_A;
	else if (!strcmp(data, "b"))
		priv->pshare->mp_antenna_rx = ANTENNA_B;
	else if (!strcmp(data, "c"))
		priv->pshare->mp_antenna_rx = ANTENNA_C;
	else if (!strcmp(data, "d"))
		priv->pshare->mp_antenna_rx = ANTENNA_D;
	else if (!strcmp(data, "ac"))
		priv->pshare->mp_antenna_rx = ANTENNA_AC;
	else if (!strcmp(data, "bd"))
		priv->pshare->mp_antenna_rx = ANTENNA_BD;
	else if (!strcmp(data, "cd"))
		priv->pshare->mp_antenna_rx = ANTENNA_CD;
	else if (!strcmp(data, "abcd"))
		priv->pshare->mp_antenna_rx = ANTENNA_ABCD;
#if	defined(RTL8192SE) || defined(RTL8192SU)
	else if (!strcmp(data, "ab"))
		priv->pshare->mp_antenna_rx = ANTENNA_AB;
#endif
	else {
		printk("Usage: mp_ant_rx {a,b,c,d,ac,bd,cd,abcd}\n");
		return;
	}

	mp_chk_sw_ant(priv);
	printk("switch Rx antenna to %s\n", data);
}


void mp_set_phypara(struct rtl8190_priv *priv, unsigned char *data)
{
	char *val;

	int xcap=-32, antCdiff=-32, sign;

	if (!netif_running(priv->dev))
	{
		printk("\nFail: interface not opened\n");
		return;
	}

	if (!(OPMODE & WIFI_MP_STATE))
	{
		printk("Fail: not in MP mode\n");
		return;
	}
	// get CrystalCap value
	val = get_value_by_token((char *)data, "xcap=");
	if (val) {
		if (*val == '-') {
			sign = 1;
			val++;
		}
		else
			sign = 0;

		xcap = _atoi(val, 10);
		if (sign)
			xcap = 0 - xcap;
	}


#if	defined(RTL8190)
    // get antenna C power diff value
	val = get_value_by_token((char *)data, "antCdiff=");
#elif defined(RTL8192SE) || defined(RTL8192SU)
	val = get_value_by_token((char *)data, "antBdiff=");
#endif
	if (val) {
		if (*val == '-') {
			sign = 1;
			val++;
		}
		else
			sign = 0;

		antCdiff = _atoi(val, 10);
		if (sign)
			antCdiff = 0 - antCdiff;
	}

	// set antenna C power diff value
	printk("Set");
	if (antCdiff != -32) {
#if	defined(RTL8190)
		PHY_SetBBReg(priv, rFPGA0_TxGainStage, bXCTxAGC, (antCdiff & 0x0000000f));
		printk(" antCdiff=%d", antCdiff);
#elif	defined(RTL8192SE) || defined(RTL8192SU)
		PHY_SetBBReg(priv, rFPGA0_TxGainStage, bXBTxAGC, (antCdiff & 0x0000000f));
		printk(" antBdiff=%d", antCdiff);
#endif
	}
#if !defined(RTL8192SE) && !defined(RTL8192SU) // 92SE not support CrystalCap
	// set CrystalCap value
	if (xcap != -32) {
		PHY_SetBBReg(priv, rFPGA0_AnalogParameter1, bXtalCap01, (xcap & 0x00000003));
		PHY_SetBBReg(priv, rFPGA0_AnalogParameter2, bXtalCap23, ((xcap & 0x0000000c) >> 2));
		printk(" xcap=%d", xcap);
	}
#endif
	printk("\n");
}


#endif	// MP_TEST

