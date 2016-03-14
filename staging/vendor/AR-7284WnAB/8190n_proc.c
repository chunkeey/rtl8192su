/*
 *      Handle routines for proc file system
 *
 *      $Id: 8190n_proc.c,v 1.9 2009/09/18 12:24:43 cathy Exp $
 */

#define _8190N_PROC_C_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/compile.h>
#include <linux/init.h>

#include "./8190n_cfg.h"
#include "./8190n.h"
#include "./ieee802_mib.h"
#include "./8190n_headers.h"

#ifdef CONFIG_RTK_MESH
#include "./mesh_ext/mesh_route.h"
#include "./mesh_ext/mesh_util.h"
#endif // CONFIG_RTK_MESH


#ifdef _INCLUDE_PROC_FS_
#include <asm/uaccess.h>

/*
 *	Note : These define someone copy to ./mesh_ext/mesh_proc.c !!
*/
#define PRINT_ONE(val, format, line_end) { 		\
	pos += sprintf(&buf[pos], format, val);		\
	if (line_end)					\
		strcat(&buf[pos++], "\n");		\
}
#define PRINT_ONE_T(val, format, line_end) { 		\
	pos += sprintf(&buf[pos], format, val);		\
	if (line_end == 1)					\
		strcat(&buf[pos++], "\t");		\
	else								\
		strcat(&buf[pos++], "\n");		\
}

#define PRINT_ARRAY(val, format, len, line_end) { 	\
	int index;					\
	for (index=0; index<len; index++)		\
		pos += sprintf(&buf[pos], format, val[index]); \
	if (line_end)					\
		strcat(&buf[pos++], "\n");		\
							\
}
#define PRINT_ARRAY_T(val, format, len, line_end) { 	\
	int index;					\
	for (index=0; index<len; index++)		\
		{ 	\
		pos += sprintf(&buf[pos], format, val[index]); \
		if(index<len-1)pos += sprintf(&buf[pos], "%s", ":"); \
		} \
	if (line_end)					\
		strcat(&buf[pos++], "\t");		\
							\
}

#define PRINT_SINGL_ARG(name, para, format) { \
	PRINT_ONE(name, "%s", 0); \
	PRINT_ONE(para, format, 1); \
}
#define PRINT_SINGL_ARG_T(para, format) { \
	PRINT_ONE_T(para, format, 1); \
}
#define PRINT_ARRAY_ARG(name, para, format, len) { \
	PRINT_ONE(name, "%s", 0); \
	PRINT_ARRAY(para, format, len, 1); \
}

#define CHECK_LEN { \
	len += size; \
	pos = begin + len; \
	if (pos < offset) { \
		len = 0; \
		begin = pos; \
	} \
	if (pos > offset + length) \
		goto _ret; \
}

const unsigned short MCS_DATA_RATE[2][2][16] =
{
	{{13, 26, 39, 52, 78, 104, 117, 130, 26, 52, 78 ,104, 156, 208, 234, 260},		// Long GI, 20MHz
	 {14, 29, 43, 58, 87, 116, 130, 144, 29, 58, 87, 116, 173, 231, 260, 289}},		// Short GI, 20MHz
	{{27, 54, 81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540},	// Long GI, 40MHz
	 {30, 60, 90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600}}	// Short GI, 40MHz
};


static int rtl8190_proc_mib_staconfig(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0, i;
	unsigned char tmpbuf[100];

	PRINT_ONE("  Dot11StationConfigEntry...", "%s", 1);
	PRINT_ARRAY_ARG("    dot11Bssid: ",
			priv->pmib->dot11StationConfigEntry.dot11Bssid, "%02x", 6);

	memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen);
	tmpbuf[priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen] = '\0';
	PRINT_ONE("    dot11DesiredSSID:(Len ", "%s", 0);
	PRINT_ONE(priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen, "%d) ", 0);
	PRINT_ONE(tmpbuf, "%s", 1);

	memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11DefaultSSID, priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen);
	tmpbuf[priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen] = '\0';
	PRINT_ONE("    dot11DefaultSSID:(Len ", "%s", 0);
	PRINT_ONE(priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen, "%d) ", 0);
	PRINT_ONE(tmpbuf, "%s", 1);

	memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11SSIDtoScan, priv->pmib->dot11StationConfigEntry.dot11SSIDtoScanLen);
	tmpbuf[priv->pmib->dot11StationConfigEntry.dot11SSIDtoScanLen] = '\0';
	PRINT_ONE("    dot11SSIDtoScan:(Len ", "%s", 0);
	PRINT_ONE(priv->pmib->dot11StationConfigEntry.dot11SSIDtoScanLen, "%d) ", 0);
	PRINT_ONE(tmpbuf, "%s", 1);

	PRINT_ARRAY_ARG("    dot11DesiredBssid: ",
			priv->pmib->dot11StationConfigEntry.dot11DesiredBssid, "%02x", 6);
	PRINT_ARRAY_ARG("    dot11OperationalRateSet: ",
			priv->pmib->dot11StationConfigEntry.dot11OperationalRateSet, "%02x",
			priv->pmib->dot11StationConfigEntry.dot11OperationalRateSetLen);
	PRINT_SINGL_ARG("    dot11OperationalRateSetLen: ",
			priv->pmib->dot11StationConfigEntry.dot11OperationalRateSetLen, "%d");
	PRINT_SINGL_ARG("    dot11BeaconPeriod: ",
			priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod, "%d");
	PRINT_SINGL_ARG("    dot11DTIMPeriod: ",
			priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod, "%d");
	PRINT_SINGL_ARG("    dot11swcrypto: ",
			priv->pmib->dot11StationConfigEntry.dot11swcrypto, "%d");
	PRINT_SINGL_ARG("    dot11AclMode: ",
			priv->pmib->dot11StationConfigEntry.dot11AclMode, "%d");
	PRINT_SINGL_ARG("    dot11AclNum: ",
			priv->pmib->dot11StationConfigEntry.dot11AclNum, "%d");

	for (i=0; i<priv->pmib->dot11StationConfigEntry.dot11AclNum; i++) {
		sprintf(tmpbuf, "    dot11AclAddr[%d]: ", i);
		PRINT_ARRAY_ARG(tmpbuf,	priv->pmib->dot11StationConfigEntry.dot11AclAddr[i], "%02x", 6);
	}

	PRINT_SINGL_ARG("    dot11SupportedRates: ",
			priv->pmib->dot11StationConfigEntry.dot11SupportedRates, "0x%x");
	PRINT_SINGL_ARG("    dot11BasicRates: ",
			priv->pmib->dot11StationConfigEntry.dot11BasicRates, "0x%x");
	PRINT_SINGL_ARG("    dot11RegDomain: ",
			priv->pmib->dot11StationConfigEntry.dot11RegDomain, "%d");
	PRINT_SINGL_ARG("    autoRate: ",
			priv->pmib->dot11StationConfigEntry.autoRate, "%d");
	PRINT_SINGL_ARG("    fixedTxRate: ",
			priv->pmib->dot11StationConfigEntry.fixedTxRate, "0x%x");
	PRINT_SINGL_ARG("    swTkipMic: ",
			priv->pmib->dot11StationConfigEntry.swTkipMic, "%d");
	PRINT_SINGL_ARG("    protectionDisabled: ",
			priv->pmib->dot11StationConfigEntry.protectionDisabled, "%d");
	PRINT_SINGL_ARG("    olbcDetectDisabled: ",
			priv->pmib->dot11StationConfigEntry.olbcDetectDisabled, "%d");
	PRINT_SINGL_ARG("    legacySTADeny: ",
			priv->pmib->dot11StationConfigEntry.legacySTADeny, "%d");
#ifdef CLIENT_MODE
	PRINT_SINGL_ARG("    fastRoaming: ",
			priv->pmib->dot11StationConfigEntry.fastRoaming, "%d");
#endif
	PRINT_SINGL_ARG("    lowestMlcstRate: ",
			priv->pmib->dot11StationConfigEntry.lowestMlcstRate, "%d");
	PRINT_SINGL_ARG("    supportedStaNum: ",
			priv->pmib->dot11StationConfigEntry.supportedStaNum, "%d");

	return pos;
}


static int rtl8190_proc_mib_auth(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  Dot1180211AuthEntry...", "%s", 1);
	PRINT_SINGL_ARG("    dot11AuthAlgrthm: ",
			priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm, "%d");
	PRINT_SINGL_ARG("    dot11PrivacyAlgrthm: ",
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm, "%d");
	PRINT_SINGL_ARG("    dot11PrivacyKeyIndex: ",
			priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex, "%d");
	PRINT_SINGL_ARG("    dot11PrivacyKeyLen: ",
			priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyLen, "%d");
#ifdef INCLUDE_WPA_PSK
	PRINT_SINGL_ARG("    dot11EnablePSK: ",
			priv->pmib->dot1180211AuthEntry.dot11EnablePSK, "%d");
	PRINT_SINGL_ARG("    dot11WPACipher: ",
			priv->pmib->dot1180211AuthEntry.dot11WPACipher, "%d");
#ifdef RTL_WPA2
	PRINT_SINGL_ARG("    dot11WPA2Cipher: ",
			priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher, "%d");
#endif
	PRINT_SINGL_ARG("    dot11PassPhrase: ",
			priv->pmib->dot1180211AuthEntry.dot11PassPhrase, "%s");
	PRINT_SINGL_ARG("    dot11PassPhraseGuest: ",
			priv->pmib->dot1180211AuthEntry.dot11PassPhraseGuest, "%s");
	PRINT_SINGL_ARG("    dot11GKRekeyTime: ",
			priv->pmib->dot1180211AuthEntry.dot11GKRekeyTime, "%ld");
#endif

	PRINT_ONE("  Dot118021xAuthEntry...", "%s", 1);
	PRINT_SINGL_ARG("    dot118021xAlgrthm: ",
			priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm, "%d");
	PRINT_SINGL_ARG("    dot118021xDefaultPort: ",
			priv->pmib->dot118021xAuthEntry.dot118021xDefaultPort, "%d");
	PRINT_SINGL_ARG("    dot118021xcontrolport: ",
			priv->pmib->dot118021xAuthEntry.dot118021xcontrolport, "%d");
#ifdef CONFIG_RTL_WAPI_SUPPORT
	PRINT_ONE("  Dot1180211WAPIEntry...", "%s", 1);
	PRINT_SINGL_ARG("    dot11EnableWAPI: ",
			priv->pmib->wapiInfo.wapiType, "%d");
#ifdef WAPI_SUPPORT_MULTI_ENCRYPT
	PRINT_SINGL_ARG("    dot11wapiUCastEncodeType: ",
			priv->pmib->wapiInfo.wapiUCastEncodeType, "%d");
	PRINT_SINGL_ARG("    dot11wapiMCastEncodeType: ",
			priv->pmib->wapiInfo.wapiMCastEncodeType, "%d");
#endif
	PRINT_ARRAY_ARG("    dot11wapiPsk: ",
			priv->pmib->wapiInfo.wapiPsk.octet, "%02x", WAPI_PSK_LEN);
	PRINT_SINGL_ARG("    dot11wapiwapiPsklen: ",
			priv->pmib->wapiInfo.wapiPsk.len, "%d");
	PRINT_SINGL_ARG("    dot11wapiUpdateUCastKeyType: ",
			priv->pmib->wapiInfo.wapiUpdateUCastKeyType, "%u");
	PRINT_SINGL_ARG("    dot11wapiUpdateUCastKeyTimeout: ",
			(unsigned int)priv->pmib->wapiInfo.wapiUpdateUCastKeyTimeout, "%u");
	PRINT_SINGL_ARG("    dot11wapiUpdateUCastKeyPktNum: ",
			(unsigned int)priv->pmib->wapiInfo.wapiUpdateUCastKeyPktNum, "%u");
	PRINT_SINGL_ARG("    dot11wapiUpdateMCastKeyType: ",
			priv->pmib->wapiInfo.wapiUpdateMCastKeyType, "%u");
	PRINT_SINGL_ARG("    dot11wapiUpdateMCastKeyTimeout: ",
			(unsigned int)priv->pmib->wapiInfo.wapiUpdateMCastKeyTimeout, "%u");
	PRINT_SINGL_ARG("    dot11wapiUpdateMCastKeyPktNum: ",
			(unsigned int)priv->pmib->wapiInfo.wapiUpdateMCastKeyPktNum, "%u");
	PRINT_ARRAY_ARG("    dot11wapiTimeout: ",
			priv->pmib->wapiInfo.wapiTimeout, "%08x", wapiTimeoutTotalNum);
#endif

	return pos;
}


static int rtl8190_proc_mib_dkeytbl(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  Dot11DefaultKeysTable...", "%s", 1);
	PRINT_ARRAY_ARG("    keytype[0].skey: ",
			priv->pmib->dot11DefaultKeysTable.keytype[0].skey, "%02x", 16);
	PRINT_ARRAY_ARG("    keytype[1].skey: ",
			priv->pmib->dot11DefaultKeysTable.keytype[1].skey, "%02x", 16);
	PRINT_ARRAY_ARG("    keytype[2].skey: ",
			priv->pmib->dot11DefaultKeysTable.keytype[2].skey, "%02x", 16);
	PRINT_ARRAY_ARG("    keytype[3].skey: ",
			priv->pmib->dot11DefaultKeysTable.keytype[3].skey, "%02x", 16);
	return pos;
}


static int rtl8190_proc_mib_gkeytbl(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;
	unsigned char *ptr;

	PRINT_ONE("  Dot11GroupKeysTable...", "%s", 1);
	PRINT_SINGL_ARG("    dot11Privacy: ",
			priv->pmib->dot11GroupKeysTable.dot11Privacy, "%d");
	PRINT_SINGL_ARG("    keyInCam: ", (priv->pmib->dot11GroupKeysTable.keyInCam? "yes" : "no"), "%s");
	PRINT_SINGL_ARG("    dot11EncryptKey.dot11TTKeyLen: ",
			priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen, "%d");
	PRINT_SINGL_ARG("    dot11EncryptKey.dot11TMicKeyLen: ",
			priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKeyLen, "%d");
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TTKey.skey: ",
			priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey, "%02x", 16);
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TMicKey1.skey: ",
			priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKey1.skey, "%02x", 16);
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TMicKey2.skey: ",
			priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKey2.skey, "%02x", 16);
	ptr = (unsigned char *)&priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48.val48;
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TXPN48.val48: ", ptr, "%02x", 8);
	ptr = (unsigned char *)&priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11RXPN48.val48;
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11RXPN48.val48: ", ptr, "%02x", 8);

	return pos;
}


static int rtl8190_proc_mib_operation(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;
	char tmpbuf[4];
	int idx = 0;

	PRINT_ONE("  Dot11OperationEntry...", "%s", 1);
	PRINT_ARRAY_ARG("    hwaddr: ",	priv->pmib->dot11OperationEntry.hwaddr, "%02x", 6);
	PRINT_SINGL_ARG("    opmode: ", priv->pmib->dot11OperationEntry.opmode, "0x%x");
	PRINT_SINGL_ARG("    hiddenAP: ", priv->pmib->dot11OperationEntry.hiddenAP, "%d");
	PRINT_SINGL_ARG("    dot11RTSThreshold: ", priv->pmib->dot11OperationEntry.dot11RTSThreshold, "%d");
	PRINT_SINGL_ARG("    dot11FragmentationThreshold: ", priv->pmib->dot11OperationEntry.dot11FragmentationThreshold, "%d");
	PRINT_SINGL_ARG("    dot11ShortRetryLimit: ", priv->pmib->dot11OperationEntry.dot11ShortRetryLimit, "%d");
	PRINT_SINGL_ARG("    dot11LongRetryLimit: ", priv->pmib->dot11OperationEntry.dot11LongRetryLimit, "%d");
	PRINT_SINGL_ARG("    expiretime: ", priv->pmib->dot11OperationEntry.expiretime, "%d");
	PRINT_SINGL_ARG("    led_type: ", priv->pmib->dot11OperationEntry.ledtype, "%d");
#ifdef RTL8190_SWGPIO_LED
	PRINT_SINGL_ARG("    led_route: ", priv->pmib->dot11OperationEntry.ledroute, "0x%x");
#endif
	PRINT_SINGL_ARG("    iapp_enable: ", priv->pmib->dot11OperationEntry.iapp_enable, "%d");
	PRINT_SINGL_ARG("    block_relay: ", priv->pmib->dot11OperationEntry.block_relay, "%d");
	PRINT_SINGL_ARG("    deny_any: ", priv->pmib->dot11OperationEntry.deny_any, "%d");
	PRINT_SINGL_ARG("    crc_log: ", priv->pmib->dot11OperationEntry.crc_log, "%d");
	PRINT_SINGL_ARG("    wifi_specific: ", priv->pmib->dot11OperationEntry.wifi_specific, "%d");
#ifdef SEMI_QOS
	PRINT_SINGL_ARG("    qos_enable: ", priv->pmib->dot11QosEntry.dot11QosEnable, "%d");
#ifdef WMM_APSD
	PRINT_SINGL_ARG("    apsd_enable: ", priv->pmib->dot11QosEntry.dot11QosAPSD, "%d");
#endif
#endif

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A)
		tmpbuf[idx++] = 'A';
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11B)
		tmpbuf[idx++] = 'B';
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G)
		tmpbuf[idx++] = 'G';
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		tmpbuf[idx++] = 'N';
	tmpbuf[idx] = '\0';
	PRINT_SINGL_ARG("    net_work_type: ", tmpbuf, "%s");

#ifdef TX_SHORTCUT
	PRINT_SINGL_ARG("    disable_txsc: ", priv->pmib->dot11OperationEntry.disable_txsc, "%d");
#endif

#ifdef RX_SHORTCUT
	PRINT_SINGL_ARG("    disable_rxsc: ", priv->pmib->dot11OperationEntry.disable_rxsc, "%d");
#endif

#ifdef BR_SHORTCUT
	PRINT_SINGL_ARG("    disable_brsc: ", priv->pmib->dot11OperationEntry.disable_brsc, "%d");
#endif

	PRINT_SINGL_ARG("    guest_access: ", priv->pmib->dot11OperationEntry.guest_access, "%d");

	return pos;
}


#ifdef DFS
static int rtl8190_proc_mib_DFS(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  Dot11DFSEntry...", "%s", 1);

	PRINT_SINGL_ARG("    disable_DFS: ", priv->pmib->dot11DFSEntry.disable_DFS, "%d");
	PRINT_SINGL_ARG("    DFS_timeout: ", priv->pmib->dot11DFSEntry.DFS_timeout, "%d");
	PRINT_SINGL_ARG("    DFS_detected: ", priv->pmib->dot11DFSEntry.DFS_detected, "%d");
	PRINT_SINGL_ARG("    NOP_timeout: ", priv->pmib->dot11DFSEntry.NOP_timeout, "%d");
	PRINT_SINGL_ARG("    rs1_threshold: ", priv->pmib->dot11DFSEntry.rs1_threshold, "%d");
	PRINT_SINGL_ARG("    disable_tx: ", priv->pmib->dot11DFSEntry.disable_tx, "%d");
	PRINT_SINGL_ARG("    Throughput_Threshold: ", priv->pmib->dot11DFSEntry.Throughput_Threshold, "%d");
	PRINT_SINGL_ARG("    RecordHistory_sec: ", priv->pmib->dot11DFSEntry.RecordHistory_sec, "%d");
	PRINT_SINGL_ARG("    temply_disable_DFS: ", priv->pmib->dot11DFSEntry.temply_disable_DFS, "%d");
	PRINT_SINGL_ARG("    Dump_Throughput: ", priv->pmib->dot11DFSEntry.Dump_Throughput, "%d");
	PRINT_SINGL_ARG("    disable_DtmDFS: ", priv->pmib->dot11DFSEntry.disable_DetermineDFSDisable, "%d");

	return pos;
}
#endif


static int rtl8190_proc_mib_rf(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;
	char *VersionID;
	unsigned char tmpbuf[8];

	PRINT_ONE("  Dot11RFEntry...", "%s", 1);
	PRINT_SINGL_ARG("    dot11RFType: ", priv->pmib->dot11RFEntry.dot11RFType, "%d");
	PRINT_SINGL_ARG("    dot11channel: ", priv->pmib->dot11RFEntry.dot11channel, "%d");
	PRINT_SINGL_ARG("    dot11ch_low: ", priv->pmib->dot11RFEntry.dot11ch_low, "%d");
	PRINT_SINGL_ARG("    dot11ch_hi: ", priv->pmib->dot11RFEntry.dot11ch_hi, "%d");
	PRINT_ARRAY_ARG("    pwrlevelCCK: ", priv->pmib->dot11RFEntry.pwrlevelCCK, "%02x", MAX_CCK_CHANNEL_NUM);
	PRINT_ARRAY_ARG("    pwrlevelOFDM_1SS: ", priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS, "%02x", 28);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS+28), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS+58), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS+88), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS+118), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS+148), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS+178), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_1SS+208), "%02x", 8);
	PRINT_ARRAY_ARG("    pwrlevelOFDM_2SS: ", priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS, "%02x", 28);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS+28), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS+58), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS+88), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS+118), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS+148), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS+178), "%02x", 30);
	PRINT_ARRAY_ARG("                  ", (priv->pmib->dot11RFEntry.pwrlevelOFDM_2SS+208), "%02x", 8);
	PRINT_SINGL_ARG("    shortpreamble: ", priv->pmib->dot11RFEntry.shortpreamble, "%d");
	PRINT_SINGL_ARG("    disable_ch14_ofdm: ", priv->pmib->dot11RFEntry.disable_ch14_ofdm, "%d");
	PRINT_SINGL_ARG("    LOFDM_pwd_A: ", priv->pmib->dot11RFEntry.LOFDM_pwd_A, "%d");
	PRINT_SINGL_ARG("    LOFDM_pwd_B: ", priv->pmib->dot11RFEntry.LOFDM_pwd_B, "%d");
	PRINT_SINGL_ARG("    tssi1: ", priv->pmib->dot11RFEntry.tssi1, "%d");
	PRINT_SINGL_ARG("    tssi2: ", priv->pmib->dot11RFEntry.tssi2, "%d");
	PRINT_SINGL_ARG("    ther: ", priv->pmib->dot11RFEntry.ther, "%d");
	if(priv->pshare->phw->MIMO_TR_hw_support == MIMO_2T2R){
		PRINT_SINGL_ARG("    MIMO_TR_hw_support: ", "2T2R", "%s");
	}else{
		PRINT_SINGL_ARG("    MIMO_TR_hw_support: ", ((priv->pshare->phw->MIMO_TR_hw_support == MIMO_1T2R) ? "1T2R" : "1T1R"), "%s");
	}
	switch (priv->pmib->dot11RFEntry.MIMO_TR_mode) {
	case MIMO_1T2R:
		sprintf(tmpbuf, "1T2R");
		break;
	case MIMO_1T1R:
		sprintf(tmpbuf, "1T1R");
		break;
	case MIMO_2T2R:
		sprintf(tmpbuf, "2T2R");
		break;
	default:
		sprintf(tmpbuf, "2T4R");
		break;
	}
	PRINT_SINGL_ARG("    MIMO_TR_mode: ", tmpbuf, "%s");

	VersionID = "RTL8192SU";
	PRINT_SINGL_ARG("    chipVersion: ", VersionID, "%s");

	return pos;
}


static int rtl8190_proc_mib_rsnie(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  Dot11RsnIE...", "%s", 1);
	PRINT_ARRAY_ARG("    rsnie: ",
			priv->pmib->dot11RsnIE.rsnie, "%02x", priv->pmib->dot11RsnIE.rsnielen);
	PRINT_SINGL_ARG("    rsnielen: ", priv->pmib->dot11RsnIE.rsnielen, "%d");

	return pos;
}


static int rtl8190_proc_mib_bssdesc(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;
	unsigned char tmpbuf[33];

	PRINT_ONE("  bss_desc...", "%s", 1);
	PRINT_ARRAY_ARG("    bssid: ", priv->pmib->dot11Bss.bssid, "%02x", MACADDRLEN);

	memcpy(tmpbuf, priv->pmib->dot11Bss.ssid, priv->pmib->dot11Bss.ssidlen);
	tmpbuf[priv->pmib->dot11Bss.ssidlen] = '\0';
	PRINT_SINGL_ARG("    ssid: ", tmpbuf, "%s");

	PRINT_SINGL_ARG("    ssidlen: ", priv->pmib->dot11Bss.ssidlen, "%d");
	PRINT_SINGL_ARG("    bsstype: ", priv->pmib->dot11Bss.bsstype, "%x");
	PRINT_SINGL_ARG("    beacon_prd: ", priv->pmib->dot11Bss.beacon_prd, "%d");
	PRINT_SINGL_ARG("    dtim_prd: ", priv->pmib->dot11Bss.dtim_prd, "%d");
	PRINT_ARRAY_ARG("    t_stamp(hex): ", priv->pmib->dot11Bss.t_stamp, "%08x", 2);
	PRINT_SINGL_ARG("    ibss_par.atim_win: ", priv->pmib->dot11Bss.ibss_par.atim_win, "%d");
	PRINT_SINGL_ARG("    capability(hex): ", priv->pmib->dot11Bss.capability, "%02x");
	PRINT_SINGL_ARG("    channel: ", priv->pmib->dot11Bss.channel, "%d");
	PRINT_SINGL_ARG("    basicrate(hex): ", priv->pmib->dot11Bss.basicrate, "%x");
	PRINT_SINGL_ARG("    supportrate(hex): ", priv->pmib->dot11Bss.supportrate, "%x");
	PRINT_ARRAY_ARG("    bdsa: ", priv->pmib->dot11Bss.bdsa, "%02x", MACADDRLEN);
	PRINT_SINGL_ARG("    rssi: ", priv->pmib->dot11Bss.rssi, "%d");
	PRINT_SINGL_ARG("    sq: ", priv->pmib->dot11Bss.sq, "%d");

	return pos;
}


static int rtl8190_proc_mib_erp(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  ERP info...", "%s", 1);
	PRINT_SINGL_ARG("    protection: ", priv->pmib->dot11ErpInfo.protection, "%d");
	PRINT_SINGL_ARG("    nonErpStaNum: ", priv->pmib->dot11ErpInfo.nonErpStaNum, "%d");
	PRINT_SINGL_ARG("    olbcDetected: ", priv->pmib->dot11ErpInfo.olbcDetected, "%d");
	PRINT_SINGL_ARG("    olbcExpired: ", priv->pmib->dot11ErpInfo.olbcExpired, "%d");
	PRINT_SINGL_ARG("    shortSlot: ", priv->pmib->dot11ErpInfo.shortSlot, "%d");
	PRINT_SINGL_ARG("    ctsToSelf: ", priv->pmib->dot11ErpInfo.ctsToSelf, "%d");
	PRINT_SINGL_ARG("    longPreambleStaNum: ", priv->pmib->dot11ErpInfo.longPreambleStaNum, "%d");

	return pos;
}


static int rtl8190_proc_cam_info(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0, i;
	unsigned char TempOutputMac[6];
	unsigned char TempOutputKey[16];
	unsigned short TempOutputCfg=0;

	PRINT_ONE("  CAM info...", "%s", 1);
	PRINT_ONE("    CAM occupied: ", "%s", 0);
	PRINT_ONE(priv->pshare->CamEntryOccupied, "%d", 1);
	for (i=0; i<32; i++)
	{
		PRINT_ONE("    Entry", "%s", 0);
		PRINT_ONE(i, " %2d:", 0);
		CAM_read_entry(priv,i,TempOutputMac,TempOutputKey,&TempOutputCfg);
		PRINT_ARRAY_ARG(" MAC addr: ", TempOutputMac, "%02x", 6);
		PRINT_SINGL_ARG("              Config: ", TempOutputCfg, "%x");
		PRINT_ARRAY_ARG("              Key: ", TempOutputKey, "%02x", 16);
	}

	return pos;
}


#ifdef WDS
static int rtl8190_proc_mib_wds(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0, i;
	unsigned char tmpbuf[100];

	PRINT_ONE("  WDS info...", "%s", 1);
	PRINT_SINGL_ARG("    wdsEnabled: ", priv->pmib->dot11WdsInfo.wdsEnabled, "%d");
	PRINT_SINGL_ARG("    wdsPure: ", priv->pmib->dot11WdsInfo.wdsPure, "%d");
	PRINT_SINGL_ARG("    wdsPriority: ", priv->pmib->dot11WdsInfo.wdsPriority, "%d");
	PRINT_SINGL_ARG("    wdsNum: ", priv->pmib->dot11WdsInfo.wdsNum, "%d");
	for (i=0; i<priv->pmib->dot11WdsInfo.wdsNum; i++) {
		sprintf(tmpbuf, "    wdsMacAddr[%d]: ", i);
		PRINT_ARRAY_ARG(tmpbuf,	priv->pmib->dot11WdsInfo.entry[i].macAddr, "%02x", 6);
		PRINT_SINGL_ARG("    wdsTxRate: ", priv->pmib->dot11WdsInfo.entry[i].txRate, "0x%x");
	}
	PRINT_SINGL_ARG("    wdsPrivacy: ", priv->pmib->dot11WdsInfo.wdsPrivacy, "%d");
	PRINT_ARRAY_ARG("    wdsWepKey: ",
			priv->pmib->dot11WdsInfo.wdsWepKey, "%02x", 16);
#ifdef INCLUDE_WPA_PSK
	PRINT_SINGL_ARG("    wds_passphrase: ",
			priv->pmib->dot11WdsInfo.wdsPskPassPhrase, "%s");
#endif

	return pos;
}
#endif // WDS


#ifdef RTK_BR_EXT
static int rtl8190_proc_mib_brext(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  BR Ext info...", "%s", 1);
	PRINT_SINGL_ARG("    nat25_disable: ", priv->pmib->ethBrExtInfo.nat25_disable, "%d");
	PRINT_SINGL_ARG("    macclone_enable: ", priv->pmib->ethBrExtInfo.macclone_enable, "%d");
	PRINT_SINGL_ARG("    dhcp_bcst_disable: ", priv->pmib->ethBrExtInfo.dhcp_bcst_disable, "%d");
	PRINT_SINGL_ARG("    addPPPoETag: ", priv->pmib->ethBrExtInfo.addPPPoETag, "%d");
	PRINT_SINGL_ARG("    nat25sc_disable: ", priv->pmib->ethBrExtInfo.nat25sc_disable, "%d");

	return pos;
}
#endif



static int rtl8190_proc_buf_info(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  buf info...", "%s", 1);
	PRINT_ONE("    hdr poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((UINT)&priv->pshare->wlan_hdrlist, "0x%.8x", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlan_hdr_poll->count, "%d", 1);

	PRINT_ONE("    hdrllc poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((UINT)&priv->pshare->wlanllc_hdrlist, "0x%.8x", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlanllc_hdr_poll->count, "%d", 1);

	PRINT_ONE("    mgmtbuf poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((UINT)&priv->pshare->wlanbuf_list, "0x%.8x", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlanbuf_poll->count, "%d", 1);

	PRINT_ONE("    icv poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((UINT)&priv->pshare->wlanicv_list, "0x%.8x", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlanicv_poll->count, "%d", 1);

	PRINT_ONE("    mic poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((UINT)&priv->pshare->wlanmic_list, "0x%.8x", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlanmic_poll->count, "%d", 1);

	return pos;
}


#ifdef ENABLE_RTL_SKB_STATS
static int rtl8190_proc_skb_info(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  skb counter...", "%s", 1);
	PRINT_SINGL_ARG("    skb_tx_cnt: ", rtl_atomic_read(&priv->rtl_tx_skb_cnt) , "%d");
	PRINT_SINGL_ARG("    skb_rx_cnt: ", rtl_atomic_read(&priv->rtl_rx_skb_cnt) , "%d");

	return pos;
}
#endif


static int rtl8190_proc_mib_11n(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  dot11nConfigEntry...", "%s", 1);

	PRINT_SINGL_ARG("    supportedmcs: ", priv->pmib->dot11nConfigEntry.dot11nSupportedMCS, "%08x");
	PRINT_SINGL_ARG("    basicmcs: ", priv->pmib->dot11nConfigEntry.dot11nBasicMCS, "%08x");
	PRINT_SINGL_ARG("    use40M: ", priv->pmib->dot11nConfigEntry.dot11nUse40M, "%d");
	PRINT_SINGL_ARG("    currBW: ", priv->pshare->is_40m_bw ? "40M":"20M", "%s");
	PRINT_ONE("    2ndchoffset: ", "%s", 0);
	switch (priv->pshare->offset_2nd_chan) {
	case HT_2NDCH_OFFSET_BELOW:
		PRINT_ONE("below", "%s", 1);
		break;
	case HT_2NDCH_OFFSET_ABOVE:
		PRINT_ONE("above", "%s", 1);
		break;
	default:
		PRINT_ONE("dontcare", "%s", 1);
		break;
	}

	PRINT_SINGL_ARG("    shortGI20M: ", priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M, "%d");
	PRINT_SINGL_ARG("    shortGI40M: ", priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M, "%d");
	PRINT_SINGL_ARG("    txStbc: ", priv->pmib->dot11nConfigEntry.dot11nTxSTBC, "%d");
	PRINT_SINGL_ARG("    ampdu: ", priv->pmib->dot11nConfigEntry.dot11nAMPDU, "%d");
	PRINT_SINGL_ARG("    amsdu: ", priv->pmib->dot11nConfigEntry.dot11nAMSDU, "%d");
	PRINT_SINGL_ARG("    ampduSndSz: ", priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz, "%d");
	PRINT_SINGL_ARG("    amsduMax: ", priv->pmib->dot11nConfigEntry.dot11nAMSDURecvMax, "%d");
	PRINT_SINGL_ARG("    amsduTimeout: ", priv->pmib->dot11nConfigEntry.dot11nAMSDUSendTimeout, "%d");
	PRINT_SINGL_ARG("    amsduNum: ", priv->pmib->dot11nConfigEntry.dot11nAMSDUSendNum, "%d");
	PRINT_SINGL_ARG("    lgyEncRstrct: ", priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct, "%d");

	if (priv->ht_cap_len) {
		unsigned char *pbuf = (unsigned char *)&priv->ht_cap_buf;
		PRINT_ARRAY_ARG("    ht_cap: ", pbuf, "%02x", priv->ht_cap_len);
	}
	else {
		PRINT_ONE("    ht_cap: none", "%s", 1);
	}
	if (priv->ht_ie_len) {
		unsigned char *pbuf = (unsigned char *)&priv->ht_ie_buf;
		PRINT_ARRAY_ARG("    ht_ie: ", pbuf, "%02x", priv->ht_ie_len);
	}
	else {
		PRINT_ONE("    ht_ie: none", "%s", 1);
	}

	PRINT_SINGL_ARG("    legacy_obss_to: ", priv->ht_legacy_obss_to, "%d");
	PRINT_SINGL_ARG("    legacy_sta_num: ", priv->ht_legacy_sta_num, "%d");
	PRINT_SINGL_ARG("    11nProtection: ", priv->ht_protection, "%d");
	PRINT_SINGL_ARG("    has_2r_sta: ", priv->pshare->has_2r_sta, "0x%08x");

	return pos;
}




#ifdef CONFIG_RTK_VLAN_SUPPORT
static int rtl8190_proc_vlan_read(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  vlan setting...", "%s", 1);
	PRINT_SINGL_ARG("    global_vlan: ",
			priv->pmib->vlan.global_vlan, "%d");
	PRINT_SINGL_ARG("    is_lan: ",
			priv->pmib->vlan.is_lan, "%d");
	PRINT_SINGL_ARG("    vlan_enable: ",
			priv->pmib->vlan.vlan_enable, "%d");
	PRINT_SINGL_ARG("    vlan_tag: ",
			priv->pmib->vlan.vlan_tag, "%d");
	PRINT_SINGL_ARG("    vlan_id: ",
			priv->pmib->vlan.vlan_id, "%d");
	PRINT_SINGL_ARG("    vlan_pri: ",
			priv->pmib->vlan.vlan_pri, "%d");
	PRINT_SINGL_ARG("    vlan_cfi: ",
			priv->pmib->vlan.vlan_cfi, "%d");

	return pos;
}


static int rtl8190_proc_vlan_write(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	char tmp[128];

	if (count < 2)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, 128)) {
		int num = sscanf(tmp, "%d %d %d %d %d %d %d",
			&priv->pmib->vlan.global_vlan, &priv->pmib->vlan.is_lan,
			&priv->pmib->vlan.vlan_enable, &priv->pmib->vlan.vlan_tag,
			&priv->pmib->vlan.vlan_id, &priv->pmib->vlan.vlan_pri,
			&priv->pmib->vlan.vlan_cfi);

		if (num !=  7) {
			panic_printk("invalid vlan parameter!\n");
		}
	}
	return count;
}
#endif // CONFIG_RTK_VLAN_SUPPORT

#ifdef RTL8192SU_EFUSE
static int rtl8190_proc_efuse(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0, i;
	unsigned char tmp;
	
	printk("EFUSE: ==============================\n");
	{
		for(i=0;i<128;i++)
		{
			if (i%16==0) 
			{
				printk("\n%3xh: ", i);
			}
			tmp=priv->EfuseMap[EFUSE_INIT_MAP][i];
			printk("%02x ",tmp);
		}
		printk("\n\n");
	}
	return pos;
}
#endif //RTL8192SU_EFUSE

#ifdef RTL8192SU_TXPKT
static int rtl8190_proc_send_pkt(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0, i;

	rtl8192SU_one_txdesc(priv, 0, 200);
	
	return pos;
}
#endif

static int rtl8190_proc_mib_all(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int len = 0;
	off_t begin = 0;
	off_t pos = 0;
	int size;

	size = sprintf(buf, "  Make info: %s by %s, v%d.%d (%s)\n", UTS_VERSION, LINUX_COMPILE_BY,
		DRV_VERSION_H, DRV_VERSION_L, DRV_RELDATE);
	CHECK_LEN;

	size = sprintf(buf+len, "  RTL8192 firmware version: %04x(%d.%d)\n",
		priv->pshare->fw_version, priv->pshare->fw_src_version, priv->pshare->fw_sub_version);
	CHECK_LEN;

	size = rtl8190_proc_mib_rf(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

	size = rtl8190_proc_mib_operation(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

	size = rtl8190_proc_mib_staconfig(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

	size = rtl8190_proc_mib_dkeytbl(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

	size = rtl8190_proc_mib_auth(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

	size = rtl8190_proc_mib_gkeytbl(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

	size = rtl8190_proc_mib_rsnie(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

	size = rtl8190_proc_mib_bssdesc(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

	size = rtl8190_proc_mib_erp(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

#ifdef WDS
	size = rtl8190_proc_mib_wds(buf+len, start, offset, length, eof, data);
	CHECK_LEN;
#endif

#ifdef RTK_BR_EXT
	size = rtl8190_proc_mib_brext(buf+len, start, offset, length, eof, data);
	CHECK_LEN;
#endif

#ifdef DFS
	size = rtl8190_proc_mib_DFS(buf+len, start, offset, length, eof, data);
	CHECK_LEN;
#endif

	size = rtl8190_proc_mib_11n(buf+len, start, offset, length, eof, data);
	CHECK_LEN;

#ifdef CONFIG_RTK_VLAN_SUPPORT
	size = rtl8190_proc_vlan_read(buf+len, start, offset, length, eof, data);
	CHECK_LEN;
#endif

	*eof = 1;

_ret:
	*start = buf + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */

	return len;
}


static int dump_one_stainfo(int num, struct stat_info *pstat, char *buf, char **start,
			off_t offset, int length, int *eof, void *data)
{
	int pos = 0, idx = 0;
	unsigned int m, n;
	char tmp[32];
	unsigned short rate;
	unsigned char tmpbuf[9];

	PRINT_ARRAY_T(pstat->hwaddr, "%02x", MACADDRLEN,1);
	PRINT_ONE_T(pstat->rssi, "%u", 1);
	PRINT_SINGL_ARG_T(pstat->rx_rate/2, "%d");
	PRINT_SINGL_ARG_T(pstat->state, "%x");
	PRINT_SINGL_ARG_T(0, "%d");
	PRINT_ONE("", "%s", 1);

	return pos;
}


static int rtl8190_proc_stainfo(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int len = 0;
	off_t begin = 0;
	off_t pos = 0;
	int size, num=1;
	struct list_head *phead, *plist;
	struct stat_info *pstat;

	//size = sprintf(buf, "-- STA info table -- (active: %d)\n", priv->assoc_num);
   	size = sprintf(buf, "-- 	MAC		RSSI	TXRETE	STATE	TYPE	stat_info-- (active: %d)\n", priv->assoc_num);
	CHECK_LEN;

	phead = &priv->asoc_list;
	if (!netif_running(dev) || list_empty(phead))
		goto _ret;

	plist = phead->next;
	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		size = dump_one_stainfo(num++, pstat, buf+len, start, offset, length,
					eof, data);
		CHECK_LEN;
#ifdef CONFIG_RTK_MESH
		// 3 line for Throughput statistics (sounder)
		size = dump_mesh_one_mpflow_sta(num, pstat, buf+len, start, offset, length,
					eof, data);
		CHECK_LEN;
#endif

		plist = plist->next;
	}

	*eof = 1;

_ret:
	*start = buf + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */

	return len;
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static int dump_one_sta_keyinfo(int num, struct stat_info *pstat, char *buf, char **start,
			off_t offset, int length, int *eof, void *data)
{
	int pos = 0;
	unsigned char *ptr;

	PRINT_ONE(num,  " %d: stat_keyinfo...", 1);
	PRINT_ARRAY_ARG("    hwaddr: ",	pstat->hwaddr, "%02x", MACADDRLEN);
	PRINT_SINGL_ARG("    keyInCam: ", (pstat->dot11KeyMapping.keyInCam? "yes" : "no"), "%s");
	PRINT_SINGL_ARG("    dot11Privacy: ",
			pstat->dot11KeyMapping.dot11Privacy, "%d");
	PRINT_SINGL_ARG("    dot11EncryptKey.dot11TTKeyLen: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKeyLen, "%d");
	PRINT_SINGL_ARG("    dot11EncryptKey.dot11TMicKeyLen: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKeyLen, "%d");
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TTKey.skey: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey, "%02x", 16);
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TMicKey1.skey: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKey1.skey, "%02x", 16);
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TMicKey2.skey: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKey2.skey, "%02x", 16);
	ptr = (unsigned char *)&pstat->dot11KeyMapping.dot11EncryptKey.dot11TXPN48.val48;
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TXPN48.val48: ", ptr, "%02x", 8);
	ptr = (unsigned char *)&pstat->dot11KeyMapping.dot11EncryptKey.dot11RXPN48.val48;
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11RXPN48.val48: ", ptr, "%02x", 8);

	PRINT_ONE("", "%s", 1);

	return pos;
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static int rtl8190_proc_sta_keyinfo(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int len = 0;
	off_t begin = 0;
	off_t pos = 0;
	int size, num=1;
	struct list_head *phead, *plist;
	struct stat_info *pstat;

	strcpy(buf, "-- STA key info table --\n");
	size = strlen(buf);
	CHECK_LEN;

	phead = &priv->asoc_list;
	if (!netif_running(dev) || list_empty(phead))
		goto _ret;

	plist = phead->next;
	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		size = dump_one_sta_keyinfo(num++, pstat, buf+len, start, offset, length,
					eof, data);
		CHECK_LEN;

		plist = plist->next;
	}

	*eof = 1;

_ret:
	*start = buf + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */

	return len;
}

extern unsigned int __drop_lowq_pkt;
#if !defined(PKT_PROCESSOR) && !defined(PRE_ALLOCATE_SKB)
extern atomic_t wifi_rxskb_num;
#endif
static int rtl8190_proc_stats(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0, idx = 0;
	unsigned int m, n, print=0;
	char tmp[32];

	PRINT_ONE("  Statistics...", "%s", 1);

	m = priv->up_time / 86400;
	n = priv->up_time % 86400;
	if (m) {
		idx += sprintf(tmp, "%d day ", m);
		print = 1;
	}
	m = n / 3600;
	n = n % 3600;
	if (m || print) {
		idx += sprintf(tmp+idx, "%d hr ", m);
		print = 1;
	}
	m = n / 60;
	n = n % 60;
	if (m || print) {
		idx += sprintf(tmp+idx, "%d min ", m);
		print = 1;
	}
	idx += sprintf(tmp+idx, "%d sec ", n);
#define TBS_STATISTIC
#ifdef TBS_STATISTIC
	pos +=sprintf(&buf[pos], "BSSID			TxPks	RxPks	TxByt	RxByt	TxDp	RxDp	TxErr	RxErr	\n");	
	PRINT_ARRAY_T(BSSID, "%02x", MACADDRLEN,1);
	PRINT_ONE_T(priv->net_stats.tx_packets, "%lu",1);
	PRINT_ONE_T(priv->net_stats.rx_packets, "%lu",1);
	PRINT_ONE_T(priv->net_stats.tx_bytes, "%lu",1);
	PRINT_ONE_T(priv->net_stats.rx_bytes, "%lu",1);
	PRINT_ONE_T(priv->ext_stats.tx_drops, "%lu",1);
	PRINT_ONE_T(priv->ext_stats.rx_data_drops, "%lu",1);
	PRINT_ONE_T(priv->net_stats.tx_errors, "%lu",1);
	PRINT_ONE_T(priv->net_stats.rx_errors, "%lu",2);
#else	
	PRINT_SINGL_ARG("    up_time: ", tmp, "%s");

	PRINT_SINGL_ARG("    tx_packets:    ", priv->net_stats.tx_packets, "%lu");
	PRINT_SINGL_ARG("    tx_bytes:      ", priv->net_stats.tx_bytes, "%lu");
	PRINT_SINGL_ARG("    tx_retrys:     ", priv->ext_stats.tx_retrys, "%lu");
	PRINT_SINGL_ARG("    tx_fails:      ", priv->net_stats.tx_errors, "%lu");
	PRINT_SINGL_ARG("    tx_drops:      ", priv->ext_stats.tx_drops, "%lu");
	PRINT_SINGL_ARG("    rx_packets:    ", priv->net_stats.rx_packets, "%lu");
	PRINT_SINGL_ARG("    rx_bytes:      ", priv->net_stats.rx_bytes, "%lu");
	PRINT_SINGL_ARG("    rx_retrys:     ", priv->ext_stats.rx_retrys, "%lu");
	PRINT_SINGL_ARG("    rx_crc_errors: ", priv->net_stats.rx_crc_errors, "%lu");
	PRINT_SINGL_ARG("    rx_errors:     ", priv->net_stats.rx_errors, "%lu");
	PRINT_SINGL_ARG("    rx_data_drops: ", priv->ext_stats.rx_data_drops, "%lu");
	PRINT_SINGL_ARG("    rx_decache:    ", priv->ext_stats.rx_decache, "%lu");
#ifdef _DEBUG_RTL8190_
	PRINT_SINGL_ARG("    rx_fifoO:      ", priv->ext_stats.rx_fifoO, "%lu");
	PRINT_SINGL_ARG("    rx_rdu:        ", priv->ext_stats.rx_rdu, "%lu");
#endif
	PRINT_SINGL_ARG("    beacon_ok:     ", priv->ext_stats.beacon_ok, "%lu");
	PRINT_SINGL_ARG("    beacon_er:     ", priv->ext_stats.beacon_er, "%lu");

	PRINT_SINGL_ARG("    freeskb_err:   ", priv->ext_stats.freeskb_err, "%lu");

#ifdef CHECK_HANGUP
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
	if (IS_ROOT_INTERFACE(priv))
#endif
	{
#ifdef CHECK_TX_HANGUP
		PRINT_SINGL_ARG("    reset_cnt_tx:  ", priv->reset_cnt_tx, "%d");
#endif
#ifdef CHECK_RX_HANGUP
		PRINT_SINGL_ARG("    reset_cnt_rx:  ", priv->reset_cnt_rx, "%d");
#endif
#ifdef CHECK_BEACON_HANGUP
		PRINT_SINGL_ARG("    reset_cnt_bcn: ", priv->reset_cnt_bcn, "%d");
#endif
		PRINT_SINGL_ARG("    reset_cnt_rst: ", priv->reset_cnt_rst, "%d");
#ifdef CHECK_BB_HANGUP
		PRINT_SINGL_ARG("    reset_cnt_bb:  ", priv->reset_cnt_bb, "%d");
		PRINT_SINGL_ARG("    reset_cnt_cca: ", priv->reset_cnt_cca, "%d");
#endif
	}
#endif


		PRINT_SINGL_ARG("    reused_skb:    ", priv->ext_stats.reused_skb, "%lu");

#ifdef CONFIG_RTL8190_PRIV_SKB
		{
			extern int skb_free_num;
			PRINT_SINGL_ARG("    skb_free_num:  ", skb_free_num+priv->pshare->skb_queue.qlen, "%d");
		}
#endif

#if defined(CONFIG_RTL8196B_TR)
	PRINT_SINGL_ARG("    tx_avarage:    ", priv->ext_stats.tx_avarage, "%lu");
	PRINT_SINGL_ARG("    tx_peak:       ", priv->ext_stats.tx_peak, "%lu");

	PRINT_SINGL_ARG("    rx_avarage:    ", priv->ext_stats.rx_avarage, "%lu");
	PRINT_SINGL_ARG("    rx_peak:       ", priv->ext_stats.rx_peak, "%lu");
#endif
//#ifdef CONFIG_RTL865X_AC
#if defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
	PRINT_SINGL_ARG("    tx_avarage:    ", priv->ext_stats.tx_avarage, "%lu");
	PRINT_SINGL_ARG("    tx_peak:       ", priv->ext_stats.tx_peak, "%lu");
	PRINT_SINGL_ARG("    rx_avarage:    ", priv->ext_stats.rx_avarage, "%lu");
	PRINT_SINGL_ARG("    rx_peak:       ", priv->ext_stats.rx_peak, "%lu");
#endif
#ifdef 	LOOPBACK_NORMAL_TX_MODE	
	PRINT_SINGL_ARG("    lb_tx_cnt:     ", priv->ext_stats.loopback_TX_cnt, "%lu");
#endif
#if defined(TX_SHORTCUT)&&defined(RTL8192SU_TXSC_DBG)
	PRINT_SINGL_ARG("    tx_sc_pkts:    ", priv->ext_stats.tx_shortcut_cnt, "%lu");
	PRINT_SINGL_ARG("    slowPath_pkts: ", priv->ext_stats.normal_pkt_cnt, "%lu");
	PRINT_SINGL_ARG("    mgt_pkts:      ", priv->ext_stats.mgt_pkt_cnt, "%lu");
	PRINT_SINGL_ARG("    !tx_sc_pkts:   ", priv->ext_stats.nonTxSC_pkt_cnt, "%lu");
#endif
#if !defined(PKT_PROCESSOR) && !defined(PRE_ALLOCATE_SKB)
	PRINT_SINGL_ARG("    wifi_rxskb_num:", atomic_read(&wifi_rxskb_num), "%lu");
#endif
#endif
	return pos;
}


static int rtl8190_proc_stats_clear(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;

	memset(&priv->net_stats, 0, sizeof(struct net_device_stats));
	memset(&priv->ext_stats, 0, sizeof(struct extra_stats));
#ifdef CONFIG_RTK_MESH
	memset(&priv->mesh_stats, 0, sizeof(struct net_device_stats));
#endif
#ifdef _11s_TEST_MODE_
	memset(priv->rvTestPacket, 0, 3000);
#endif
	return count;
}


static int rtl8190_proc_mib_misc(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  miscEntry...", "%s", 1);

	PRINT_SINGL_ARG("    show_hidden_bss: ", priv->pmib->miscEntry.show_hidden_bss, "%d");
	PRINT_SINGL_ARG("    ack_timeout: ", (unsigned char)priv->pmib->miscEntry.ack_timeout, "%d");
	PRINT_SINGL_ARG("    tx_priority: ", (unsigned char)priv->pmib->miscEntry.tx_priority, "0x%02x");
#ifdef A4_TUNNEL
	PRINT_SINGL_ARG("    a4tnl_enable: ", priv->pmib->miscEntry.a4tnl_enable, "%d");
#endif
	PRINT_ARRAY_ARG("    private_ie: ", priv->pmib->miscEntry.private_ie, "%02x", priv->pmib->miscEntry.private_ie_len);
	PRINT_SINGL_ARG("    rxInt: ", priv->pmib->miscEntry.rxInt_thrd, "%d");
#ifdef DRVMAC_LB
	PRINT_SINGL_ARG("    dmlb: ", priv->pmib->miscEntry.drvmac_lb, "%d");
	PRINT_ARRAY_ARG("    lb_da: ", priv->pmib->miscEntry.lb_da, "%02x", MACADDRLEN);
#endif
	PRINT_SINGL_ARG("    groupID: ", priv->pmib->miscEntry.groupID, "%d");
	PRINT_SINGL_ARG("    rc_enable: ", priv->pmib->reorderCtrlEntry.ReorderCtrlEnable, "%d");
	PRINT_SINGL_ARG("    rc_winsz: ", priv->pmib->reorderCtrlEntry.ReorderCtrlWinSz, "%d");
	PRINT_SINGL_ARG("    rc_timeout: ", priv->pmib->reorderCtrlEntry.ReorderCtrlTimeout, "%d");
	PRINT_SINGL_ARG("    func_off: ", priv->pmib->miscEntry.func_off, "%d");

#ifdef HW_QUICK_INIT
	PRINT_SINGL_ARG("    reinit: ", priv->pshare->last_reinit?"s/w":"h/w", "%s");
#endif

	return pos;
}


#ifdef WIFI_SIMPLE_CONFIG
static int rtl8190_proc_mib_wsc(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  wscEntry...", "%s", 1);
	PRINT_SINGL_ARG("    wsc_enable: ", priv->pmib->wscEntry.wsc_enable, "%d");
	PRINT_ARRAY_ARG("    beacon_ie: ",
			priv->pmib->wscEntry.beacon_ie, "%02x", priv->pmib->wscEntry.beacon_ielen);
	PRINT_SINGL_ARG("    beacon_ielen: ", priv->pmib->wscEntry.beacon_ielen, "%d");
	PRINT_ARRAY_ARG("    probe_rsp_ie: ",
			priv->pmib->wscEntry.probe_rsp_ie, "%02x", priv->pmib->wscEntry.probe_rsp_ielen);
	PRINT_SINGL_ARG("    probe_rsp_ielen: ", priv->pmib->wscEntry.probe_rsp_ielen, "%d");
	PRINT_ARRAY_ARG("    probe_req_ie: ",
			priv->pmib->wscEntry.probe_req_ie, "%02x", priv->pmib->wscEntry.probe_req_ielen);
	PRINT_SINGL_ARG("    probe_req_ielen: ", priv->pmib->wscEntry.probe_req_ielen, "%d");
	PRINT_ARRAY_ARG("    assoc_ie: ",
			priv->pmib->wscEntry.assoc_ie, "%02x", priv->pmib->wscEntry.assoc_ielen);
	PRINT_SINGL_ARG("    assoc_ielen: ", priv->pmib->wscEntry.assoc_ielen, "%d");

	return pos;
}
#endif


#ifdef RF_FINETUNE
static int rtl8190_proc_rfft(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  RF fine tune variables...", "%s", 1);

	PRINT_SINGL_ARG("    rssi: ", priv->pshare->rf_ft_var.rssi_dump, "%d");
	PRINT_SINGL_ARG("    rxfifoO: ", priv->pshare->rf_ft_var.rxfifoO, "%x");
	PRINT_SINGL_ARG("    raGoDownUpper: ", priv->pshare->rf_ft_var.raGoDownUpper, "%d");
	PRINT_SINGL_ARG("    raGoDown20MLower: ", priv->pshare->rf_ft_var.raGoDown20MLower, "%d");
	PRINT_SINGL_ARG("    raGoDown40MLower: ", priv->pshare->rf_ft_var.raGoDown40MLower, "%d");
	PRINT_SINGL_ARG("    raGoUpUpper: ", priv->pshare->rf_ft_var.raGoUpUpper, "%d");
	PRINT_SINGL_ARG("    raGoUp20MLower: ", priv->pshare->rf_ft_var.raGoUp20MLower, "%d");
	PRINT_SINGL_ARG("    raGoUp40MLower: ", priv->pshare->rf_ft_var.raGoUp40MLower, "%d");
	PRINT_SINGL_ARG("    digGoUpperLevel: ", priv->pshare->rf_ft_var.digGoUpperLevel, "%d");
	PRINT_SINGL_ARG("    digGoLowerLevel: ", priv->pshare->rf_ft_var.digGoLowerLevel, "%d");
	PRINT_SINGL_ARG("    rssiTx20MUpper: ", priv->pshare->rf_ft_var.rssiTx20MUpper, "%d");
	PRINT_SINGL_ARG("    rssiTx20MLower: ", priv->pshare->rf_ft_var.rssiTx20MLower, "%d");
	PRINT_SINGL_ARG("    rssi_expire_to: ", priv->pshare->rf_ft_var.rssi_expire_to, "%d");

	// bcm old 11n chipset iot debug

	PRINT_SINGL_ARG("    igUpperBound: ", priv->pshare->rf_ft_var.mlcstRxIgUpperBound, "%d");

	// dynamic Rx path selection by signal strength
	PRINT_SINGL_ARG("    ss_th_low: ", priv->pshare->rf_ft_var.ss_th_low, "%d");
	PRINT_SINGL_ARG("    diff_th: ", priv->pshare->rf_ft_var.diff_th, "%d");
	PRINT_SINGL_ARG("    cck_sel_ver: ", priv->pshare->rf_ft_var.cck_sel_ver, "%d");
	PRINT_SINGL_ARG("    cck_accu_num: ", priv->pshare->rf_ft_var.cck_accu_num, "%d");

	PRINT_SINGL_ARG("    tx_pwr_ctrl: ", priv->pshare->rf_ft_var.tx_pwr_ctrl, "%d");

	// 11n ap AES debug
	PRINT_SINGL_ARG("    aes_check_th: ", priv->pshare->rf_ft_var.aes_check_th, "%d KB");

	// dynamic CCK Tx power by rssi
	PRINT_SINGL_ARG("    cck_enhance: ", priv->pshare->rf_ft_var.cck_enhance, "%d");

	// Tx power tracking
	PRINT_SINGL_ARG("    tpt_period: ", priv->pshare->rf_ft_var.tpt_period, "%d");

	// TXOP enlarge
	PRINT_SINGL_ARG("    txop_enlarge_upper: ", priv->pshare->rf_ft_var.txop_enlarge_upper, "%d");
	PRINT_SINGL_ARG("    txop_enlarge_lower: ", priv->pshare->rf_ft_var.txop_enlarge_lower, "%d");

	// 2.3G support
	PRINT_SINGL_ARG("    frq_2_3G: ", priv->pshare->rf_ft_var.use_frq_2_3G, "%d");

	//Support IP multicast->unicast
#ifdef SUPPORT_TX_MCAST2UNI
	PRINT_SINGL_ARG("    mc2u_disable: ", priv->pshare->rf_ft_var.mc2u_disable, "%d");
#endif

#ifdef WIFI_11N_2040_COEXIST
	PRINT_SINGL_ARG("    coexist_enable: ", priv->pshare->rf_ft_var.coexist_enable, "%d");
	if (priv->pshare->rf_ft_var.coexist_enable) {
		PRINT_SINGL_ARG("    bg_ap_timeout: ", priv->bg_ap_timeout, "%d");
		PRINT_SINGL_ARG("    force_20_sta: ", priv->force_20_sta, "0x%08x");
		PRINT_SINGL_ARG("    switch_20_sta: ", priv->switch_20_sta, "0x%08x");
	}
#endif

	return pos;
}
#endif


#ifdef GBWC
static int rtl8190_proc_mib_gbwc(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	int pos = 0, i;

	PRINT_ONE("  miscGBWC...", "%s", 1);

	PRINT_SINGL_ARG("    GBWCMode: ", priv->pmib->gbwcEntry.GBWCMode, "%d");
	PRINT_SINGL_ARG("    GBWCThrd: ", priv->pmib->gbwcEntry.GBWCThrd, "%d kbps");
	PRINT_ONE("    Address List:", "%s", 1);
	for (i=0; i<priv->pmib->gbwcEntry.GBWCNum; i++) {
		PRINT_ARRAY_ARG("      ", priv->pmib->gbwcEntry.GBWCAddr[i], "%02x", MACADDRLEN);
	}

	return pos;
}
#endif


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static int rtl8190_proc_led(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8190_priv *priv = (struct rtl8190_priv *)dev->priv;
	char tmpbuf[100];
	int flag;

	if (buffer && !copy_from_user(tmpbuf, (char *)buffer, count)) {
		sscanf(tmpbuf, "%d", &flag);
		if (flag == 0) // disable
			control_wireless_led(priv, 0);
		else if (flag == 1) // enable
			control_wireless_led(priv, 1);
		else if (flag == 2) // restore
			control_wireless_led(priv, 2);
		else
			printk("flag [%d] not supported!\n", flag);
    }
	return count;
}


void MDL_DEVINIT rtl8190_proc_init(struct net_device *dev)
{
	struct rtl8190_priv *priv = dev->priv;
	struct proc_dir_entry *rtl8190_proc_root = NULL ;
	struct proc_dir_entry *res;

	rtl8190_proc_root = proc_mkdir(dev->name, NULL);
	priv->proc_root = rtl8190_proc_root ;
	if (rtl8190_proc_root == NULL) {
		printk("create proc root failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mib_all", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_all, (void *)dev) == NULL ) {
		printk("create proc mib_all failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mib_rf", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_rf, (void *)dev) == NULL ) {
		printk("create proc mib_rf failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mib_operation", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_operation, (void *)dev) == NULL ) {
		printk("create proc mib_operation failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mib_staconfig", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_staconfig, (void *)dev) == NULL ) {
		printk("create proc mib_staconfig failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mib_dkeytbl", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_dkeytbl, (void *)dev) == NULL ) {
		printk("create proc mib_dkeytbl failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mib_auth", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_auth, (void *)dev) == NULL ) {
		printk("create proc mib_auth failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mib_gkeytbl", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_gkeytbl, (void *)dev) == NULL ) {
		printk("create proc mib_gkeytbl failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mib_rsnie", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_rsnie, (void *)dev) == NULL ) {
		printk("create proc mib_rsnie failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mib_bssdesc", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_bssdesc, (void *)dev) == NULL ) {
		printk("create proc mib_bssdesc failed!\n");
		return;
	}
	if ( create_proc_read_entry ("sta_info", 0644, rtl8190_proc_root,
			rtl8190_proc_stainfo, (void *)dev) == NULL ) {
		printk("create proc sta_info failed!\n");
		return;
	}
	if ( create_proc_read_entry ("sta_keyinfo", 0644, rtl8190_proc_root,
			rtl8190_proc_sta_keyinfo, (void *)dev) == NULL ) {
		printk("create proc sta_keyinfo failed!\n");
		return;
	}


	if ( create_proc_read_entry ("buf_info", 0644, rtl8190_proc_root,
			rtl8190_proc_buf_info, (void *)dev) == NULL ) {
		printk("create proc buf_info failed!\n");
		return;
	}
	if ( (res = create_proc_read_entry ("stats", 0644, rtl8190_proc_root,
			rtl8190_proc_stats, (void *)dev)) == NULL ) {
		printk("create proc stats failed!\n");
		return;
	}
	res->write_proc = rtl8190_proc_stats_clear;

	if ( create_proc_read_entry ("mib_erp", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_erp, (void *)dev) == NULL ) {
		printk("create proc mib_erp failed!\n");
		return;
	}
	if ( create_proc_read_entry ("cam_info", 0644, rtl8190_proc_root,
			rtl8190_proc_cam_info, (void *)dev) == NULL ) {
		printk("create proc cam_info failed!\n");
		return;
	}

#ifdef WDS
	if ( create_proc_read_entry ("mib_wds", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_wds, (void *)dev) == NULL ) {
		printk("create proc mib_wds failed!\n");
		return;
	}
#endif

#ifdef RTK_BR_EXT
	if ( create_proc_read_entry ("mib_brext", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_brext, (void *)dev) == NULL ) {
		printk("create proc mib_brext failed!\n");
		return;
	}
#endif

#ifdef ENABLE_RTL_SKB_STATS
	if ( create_proc_read_entry ("skb_info", 0644, rtl8190_proc_root,
			rtl8190_proc_skb_info, (void *)dev) == NULL ) {
		printk("create proc skb_info failed!\n");
		return;
	}
#endif

#ifdef DFS
	if ( create_proc_read_entry ("mib_dfs", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_DFS, (void *)dev) == NULL ) {
		printk("create proc mib_DFS failed!\n");
		return;
	}
#endif

	if ( create_proc_read_entry ("mib_misc", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_misc, (void *)dev) == NULL ) {
		printk("create proc mib_misc failed!\n");
		return;
	}

#ifdef WIFI_SIMPLE_CONFIG
	if ( create_proc_read_entry ("mib_wsc", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_wsc, (void *)dev) == NULL ) {
		printk("create proc mib_wsc failed!\n");
		return;
	}
#endif

#ifdef RF_FINETUNE
	if ( create_proc_read_entry ("rf_finetune", 0644, rtl8190_proc_root,
			rtl8190_proc_rfft, (void *)dev) == NULL ) {
		printk("create proc rf_finetune failed!\n");
		return;
	}
#endif

#ifdef GBWC
	if ( create_proc_read_entry ("mib_gbwc", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_gbwc, (void *)dev) == NULL ) {
		printk("create proc mib_gbwc failed!\n");
		return;
	}
#endif

	if ( create_proc_read_entry ("mib_11n", 0644, rtl8190_proc_root,
			rtl8190_proc_mib_11n, (void *)dev) == NULL ) {
		printk("create proc mib_11n failed!\n");
		return;
	}

	if ((res=create_proc_entry("led", 0644, rtl8190_proc_root))== NULL) {
		printk("create proc led failed!\n");
	}
	res->write_proc = rtl8190_proc_led;
	res->data =  (void *)dev;



#ifdef CONFIG_RTK_VLAN_SUPPORT
	if ((res=create_proc_read_entry("mib_vlan", 0644, rtl8190_proc_root,
			rtl8190_proc_vlan_read, (void *)dev)) == NULL) {
		printk("create_proc vlan failed!\n");
		return;
	}
	res->write_proc = rtl8190_proc_vlan_write;
#endif

#ifdef CONFIG_RTK_MESH
#ifdef MESH_BOOTSEQ_AUTH
	if ( create_proc_read_entry ("mesh_auth_mpinfo", 0644, rtl8190_proc_root,
			rtl8190_mesh_auth_mpinfo, (void *)dev) == NULL ) {
		printk("create proc mesh_auth_mpinfo failed!\n");
		return;
	}
#endif

	if ( create_proc_read_entry ("mesh_unestablish_mpinfo", 0644, rtl8190_proc_root,
			rtl8190_mesh_unEstablish_mpinfo, (void *)dev) == NULL ) {
		printk("create proc mesh_unestablish_mpinfo failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_assoc_mpinfo", 0644, rtl8190_proc_root,
			rtl8190_mesh_assoc_mpinfo, (void *)dev) == NULL ) {
		printk("create proc mesh_assoc_mpinfo failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_stats", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_stats, (void *)dev) == NULL ) {
		printk("create proc mesh_stats failed!\n");
		return;
	}

	// 6 line for Throughput statistics (sounder)
	if ( (res = create_proc_read_entry ("flow_stats", 0644, rtl8190_proc_root,
			rtk8190_proc_flow_stats, (void *)dev)) == NULL ) {
		printk("create proc flow_stats failed!\n");
		return;
	}
	res->write_proc = rtk8190_proc_flow_stats_write;

	if ( create_proc_read_entry ("mesh_pathsel_routetable", 0644, rtl8190_proc_root,
			rtl8190_pathsel_routetable_info, (void *)dev) == NULL ) {
		printk("create proc mesh_pathsel_routetable failed!\n");
		return;
	}
	if ( create_proc_read_entry ("mesh_proxy_table", 0644, rtl8190_proc_root,
			rtl8190_proxy_table_info, (void *)dev) == NULL ) {
		printk("create proc mesh_proxy_table failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_portal_table", 0644, rtl8190_proc_root,
			rtl8190_portal_table_info, (void *)dev) == NULL ) {
		printk("create proc mesh_portal_table failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_root_info", 0644, rtl8190_proc_root,
			rtl8190_root_info, (void *)dev) == NULL ) {
		printk("create proc mesh_root_info failed!\n");
		return;
	}
#ifdef MESH_USE_METRICOP
	{ // change metric method
		struct proc_dir_entry *proc_file1;
		proc_file1 = create_proc_entry( "mesh_metric",
						0644,
						rtl8190_proc_root);
		if(proc_file1 == NULL)
		{
			printk("can't create proc: %s\r\n", "mesh_metric");
			return;
		}
		proc_file1->write_proc = rtl8190_mesh_metric_w;
		proc_file1->read_proc = rtl8190_mesh_metric_r;
		proc_file1->data = (void *)dev;
	}
#endif

#if DBG_NCTU_MESH
	if ( create_proc_read_entry ("mesh_skb", 0644, rtl8190_proc_root,
			showAllSkbs, (void *)dev) == NULL ) {
		printk("create proc mesh_skb failed!\n");
		return;
	}
	{
		struct proc_dir_entry *proc_file1;

		proc_file1 = create_proc_entry(
					"mesh_dbg",
					0644,
					rtl8190_proc_root);
		if(proc_file1 == NULL)
		{
			printk("can't create proc: %s\r\n", "mesh_dbg");
			return;
		}
		proc_file1->write_proc = rtl8190_setMeshDebugLevel;
	}
#endif
#ifdef _MESH_DEBUG_
	if ( create_proc_read_entry ("mesh_clearalltable", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_clear_table, (void *)dev) == NULL ) {
		printk("create proc mesh_clearalltable failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_issueauthreq", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_issueAuthReq, (void *)dev) == NULL ) {
		printk("create proc mesh_issueauthreq failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_issueauthrsp", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_issueAuthRsp, (void *)dev) == NULL ) {
		printk("create proc mesh_issueauthrsp failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_issuedeauth", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_issueDeAuth, (void *)dev) == NULL ) {
		printk("create proc mesh_issuedeauth failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_openconnect", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_openConnect, (void *)dev) == NULL ) {
		printk("create proc mesh_openconnect failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_issueopen", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_issueOpen, (void *)dev) == NULL ) {
		printk("create proc mesh_issueopen failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_issueconfirm", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_issueConfirm, (void *)dev) == NULL ) {
		printk("create proc mesh_issueconfirm failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_issueclose", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_issueClose, (void *)dev) == NULL ) {
		printk("create proc mesh_issueclose failed!\n");
		return;
	}

	if ( create_proc_read_entry ("mesh_closeconnect", 0644, rtl8190_proc_root,
			rtl8190_proc_mesh_closeConnect, (void *)dev) == NULL ) {
		printk("create proc mesh_closeconnect failed!\n");
		return;
	}

	{
	struct proc_dir_entry *proc_file1;

	proc_file1 = create_proc_entry(
					"mesh_setmacaddr",
					0644,
					rtl8190_proc_root);
	if(proc_file1 == NULL)
	{
		printk("can't create proc: %s\r\n", "mesh_setmacaddr");
		return;
	}
	proc_file1->write_proc = rtl8190_mesh_setMACAddr;
	}


#if (MESH_DBG_LV & MESH_DBG_COMPLEX)
	{
		struct proc_dir_entry *proc_file;

		proc_file = create_proc_entry(
						MESH_PROC_SME_TEST_FILENAME,
						0644,
						rtl8190_proc_root);
		if(proc_file == NULL)
		{
			printk("can't create proc: %s\r\n", MESH_PROC_SME_TEST_FILENAME);
			return;
		}
		proc_file->read_proc = mesh_test_sme_proc_read;
		proc_file->write_proc = mesh_test_sme_proc_write;
	}



/*	{
		struct proc_dir_entry *proc_file;

		proc_file = create_proc_entry(
						MESH_PROC_FILENAME,
						0644,
						rtl8190_proc_root);
		if(proc_file == NULL)
		{
			printk("can't create proc: %s\r\n", MESH_PROC_FILENAME);
			return;
		}
		proc_file->read_proc = mesh_proc_read;
		proc_file->write_proc = mesh_proc_write;
	}*/
#endif // (MESH_DBG_LV & MESH_DBG_COMPLEX)
#endif	// _MESH_DEBUG_
#endif	// CONFIG_RTK_MESH


#ifdef RTL8192SU_TXPKT
	if ( create_proc_read_entry ("send_pkt", 0644, rtl8190_proc_root,
			rtl8190_proc_send_pkt, (void *)dev) == NULL ) {
		printk("create proc mib_wds failed!\n");
		return;
	}
#endif

#ifdef RTL8192SU_EFUSE
	if ( create_proc_read_entry ("efuse", 0644, rtl8190_proc_root,
			rtl8190_proc_efuse, (void *)dev) == NULL ) {
		printk("create proc efuse failed!\n");
		return;
	}
#endif

	
}


void __devexit rtl8190_proc_remove (struct net_device *dev)
{
	struct rtl8190_priv *priv = dev->priv;
	struct proc_dir_entry *rtl8190_proc_root = priv->proc_root;

	if (rtl8190_proc_root != NULL) {
		remove_proc_entry( "mib_all", rtl8190_proc_root );
		remove_proc_entry( "mib_rf", rtl8190_proc_root );
		remove_proc_entry( "mib_operation", rtl8190_proc_root );
		remove_proc_entry( "mib_staconfig", rtl8190_proc_root );
		remove_proc_entry( "mib_dkeytbl", rtl8190_proc_root );
		remove_proc_entry( "mib_auth", rtl8190_proc_root );
		remove_proc_entry( "mib_gkeytbl", rtl8190_proc_root );
		remove_proc_entry( "mib_rsnie", rtl8190_proc_root );
		remove_proc_entry( "mib_bssdesc", rtl8190_proc_root );
		remove_proc_entry( "sta_info", rtl8190_proc_root );
		remove_proc_entry( "sta_keyinfo", rtl8190_proc_root );
		remove_proc_entry( "txdesc0", rtl8190_proc_root );
		remove_proc_entry( "txdesc1", rtl8190_proc_root );
		remove_proc_entry( "txdesc2", rtl8190_proc_root );
		remove_proc_entry( "txdesc3", rtl8190_proc_root );
		remove_proc_entry( "txdesc4", rtl8190_proc_root );
		remove_proc_entry( "txdesc5", rtl8190_proc_root );
		remove_proc_entry( "rxdesc", rtl8190_proc_root );
		remove_proc_entry( "desc_info", rtl8190_proc_root );
		remove_proc_entry( "buf_info", rtl8190_proc_root );
		remove_proc_entry( "stats", rtl8190_proc_root );
		remove_proc_entry( "mib_erp", rtl8190_proc_root );
		remove_proc_entry( "cam_info", rtl8190_proc_root );

#ifdef WDS
		remove_proc_entry( "mib_wds", rtl8190_proc_root );
#endif

#ifdef RTK_BR_EXT
		remove_proc_entry( "mib_brext", rtl8190_proc_root );
#endif

#ifdef ENABLE_RTL_SKB_STATS
		remove_proc_entry( "skb_info", rtl8190_proc_root );
#endif

#ifdef DFS
		remove_proc_entry( "mib_dfs", rtl8190_proc_root );
#endif

		remove_proc_entry( "mib_misc", rtl8190_proc_root );

#ifdef WIFI_SIMPLE_CONFIG
		remove_proc_entry( "mib_wsc", rtl8190_proc_root );
#endif

#ifdef RF_FINETUNE
		remove_proc_entry( "rf_finetune", rtl8190_proc_root );
#endif

#ifdef GBWC
		remove_proc_entry( "mib_gbwc", rtl8190_proc_root );
#endif

		remove_proc_entry( "mib_11n", rtl8190_proc_root );

#ifdef CONFIG_RTK_MESH
		remove_proc_entry( "mesh_auth_mpinfo", rtl8190_proc_root );
		remove_proc_entry( "mesh_unestablish_mpinfo", rtl8190_proc_root );
		remove_proc_entry( "mesh_assoc_mpinfo", rtl8190_proc_root );
		remove_proc_entry( "mesh_stats", rtl8190_proc_root );
		remove_proc_entry( "mesh_pathsel_routetable", rtl8190_proc_root );
		remove_proc_entry( "rtl8190_proxy_table_info", rtl8190_proc_root );
		remove_proc_entry( "rtl8190_root_info", rtl8190_proc_root );
		remove_proc_entry( "rtl8190_portal_table_info", rtl8190_proc_root );

#ifdef MESH_USE_METRICOP // remove proc file
                remove_proc_entry( "mesh_metric", rtl8190_proc_root );
#endif

#if DBG_NCTU_MESH
		remove_proc_entry( "mesh_skb", rtl8190_proc_root );
		remove_proc_entry( "mesh_dbg", rtl8190_proc_root );
#endif
#ifdef _MESH_DEBUG_
		remove_proc_entry( "mesh_clearalltable", rtl8190_proc_root );
		remove_proc_entry( "mesh_setmacaddr", rtl8190_proc_root );
		remove_proc_entry( "mesh_issueauthreq", rtl8190_proc_root );
		remove_proc_entry( "mesh_issueauthrsp", rtl8190_proc_root );
		remove_proc_entry( "mesh_issuedeauth", rtl8190_proc_root );
		remove_proc_entry( "mesh_openconnect", rtl8190_proc_root );
		remove_proc_entry( "mesh_issueopen", rtl8190_proc_root );
		remove_proc_entry( "mesh_issueconfirm", rtl8190_proc_root );
		remove_proc_entry( "mesh_issueclose", rtl8190_proc_root );
		remove_proc_entry( "mesh_closeconnect", rtl8190_proc_root );
#if (MESH_DBG_LV & MESH_DBG_COMPLEX)
		remove_proc_entry( MESH_PROC_SME_TEST_FILENAME, rtl8190_proc_root );
#endif
#endif	// _MESH_DEBUG_
#endif	// CONFIG_RTK_MESH

#ifdef RTL8192SU_EFUSE
		remove_proc_entry( "efuse", rtl8190_proc_root );
#endif

		remove_proc_entry( dev->name, NULL );
		rtl8190_proc_root = NULL;
	}
}


#endif // __INCLUDE_PROC_FS__

