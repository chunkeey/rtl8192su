/*
 *      WPA PSK handling routines
 *
 *      $Id: 8190n_psk.c,v 1.8 2009/08/11 11:19:44 yachang Exp $
 */

#define _8190N_PSK_C_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/timer.h>
#include <linux/random.h>
#endif

#include "./8190n_cfg.h"

#ifdef INCLUDE_WPA_PSK

#include "./8190n.h"
#include "./wifi.h"
#include "./8190n_util.h"
#include "./8190n_headers.h"
#include "./8190n_security.h"
#include "./ieee802_mib.h"
#include "./8190n_debug.h"
#include "./8190n_psk.h"
#include "./1x_rc4.h"

#ifndef __KERNEL__
#include "./sys-support.h"
#endif

//#define DEBUG_PSK

#define ETHER_ADDRLEN					6
#define PMK_EXPANSION_CONST 	        "Pairwise key expansion"
#define PMK_EXPANSION_CONST_SIZE		22
#ifdef RTL_WPA2
#define PMKID_NAME_CONST 	        	"PMK Name"
#define PMKID_NAME_CONST_SIZE			8
#endif /* RTL_WPA2 */
#define GMK_EXPANSION_CONST				"Group key expansion"
#define GMK_EXPANSION_CONST_SIZE		19
#define RANDOM_EXPANSION_CONST			"Init Counter"
#define RANDOM_EXPANSION_CONST_SIZE	12
#define PTK_LEN_CCMP            		48

/*
	2008-12-16, For Corega CG-WLCB54GL 54Mbps NIC interoperability issue.
	The behavior of this NIC when it connect to the other AP with WPA/TKIP is:
		AP	<----------------------> 	STA
			....................
			------------> Assoc Rsp (ok)
			------------> EAPOL-key (4-way msg 1)
			<------------ unknown TKIP encryption data
			------------> EAPOL-key (4-way msg 1)
			<------------ unknown TKIP encryption data
			.....................
			<------------ disassoc (code=8, STA is leaving) when the 5 seconds timer timeout counting from Assoc_Rsp is got.
			....................
			------------> Assoc Rsp (ok)
			<-----------> EAPOL-key (4-way handshake success)

	If MAX_RESEND_NUM=3, our AP will send disassoc (code=15, 4-way timeout) to STA before STA sending disassoc to AP.
	And this NIC will always can not connect to our AP.
	set MAX_RESEND_NUM=5 can fix this issue.
 */
//#define MAX_RESEND_NUM					3
#define MAX_RESEND_NUM					5

#define RESEND_TIME						100	// in 10ms

#define LargeIntegerOverflow(x) (x.field.HighPart == 0xffffffff) && \
								(x.field.LowPart == 0xffffffff)
#define LargeIntegerZero(x) memset(&x.charData, 0, 8);

#define Octet16IntegerOverflow(x) LargeIntegerOverflow(x.field.HighPart) && \
								  LargeIntegerOverflow(x.field.LowPart)
#define Octet16IntegerZero(x) memset(&x.charData, 0, 16);

#define SetNonce(ocDst, oc32Counter) SetEAPOL_KEYIV(ocDst, oc32Counter)


#if defined(CONFIG_RTL8186_KB_N)
int authRes = 0;//0: success; 1: fail
#endif

extern void hmac_sha(
	unsigned char*	k,     /* secret key */
	int				lk,    /* length of the key in bytes */
	unsigned char*	d,     /* data */
	int				ld,    /* length of data in bytes */
	unsigned char*	out,   /* output buffer, at least "t" bytes */
	int				t
	);

extern void hmac_sha1(unsigned char *text, int text_len, unsigned char *key,
		 int key_len, unsigned char *digest);

extern void hmac_md5(unsigned char *text, int text_len, unsigned char *key,
		 int key_len, void * digest);

#ifdef RTL_WPA2
extern void AES_WRAP(unsigned char *plain, int plain_len,
		unsigned char *iv,	int iv_len,
		unsigned char *kek,	int kek_len,
		unsigned char *cipher, unsigned short *cipher_len);
#ifdef CLIENT_MODE
extern void AES_UnWRAP(unsigned char *cipher, int cipher_len, unsigned char *kek,
				int kek_len, unsigned char *plain);
#endif
#endif

static void UpdateGK(struct rtl8190_priv *priv);
static void SendEAPOL(struct rtl8190_priv *priv, struct stat_info *pstat, int resend);
#ifdef CLIENT_MODE
static void ClientSendEAPOL(struct rtl8190_priv *priv, struct stat_info *pstat, int resend);
#endif
static void ResendTimeout(unsigned long task_psta);


#ifdef DEBUG_PSK
static char *ID2STR(int id)
{
	switch(id) {
		case DOT11_EVENT_ASSOCIATION_IND:
			return("DOT11_EVENT_ASSOCIATION_IND");
		case DOT11_EVENT_REASSOCIATION_IND:
			return ("DOT11_EVENT_REASSOCIATION_IND");

		case DOT11_EVENT_DISASSOCIATION_IND:
			return ("DOT11_EVENT_DISASSOCIATION_IND");

		case DOT11_EVENT_EAP_PACKET:
			return ("DOT11_EVENT_EAP_PACKET");

		case DOT11_EVENT_MIC_FAILURE:
			return ("DOT11_EVENT_MIC_FAILURE");
		default:
			return ("Not support event");

	}
}

static char *STATE2RXMSG(struct stat_info *pstat)
{
	if (Message_KeyType(pstat->wpa_sta_info->EapolKeyMsgRecvd) == type_Pairwise) {
		if (pstat->wpa_sta_info->state == PSK_STATE_PTKSTART)
			return ("4-2");
		else if (pstat->wpa_sta_info->state == PSK_STATE_PTKINITNEGOTIATING) {
			if (Message_KeyDataLength(pstat->wpa_sta_info->EapolKeyMsgRecvd) != 0)
				return ("4-2 (duplicated)");
			else
				return ("4-4");
		}
		else if (pstat->wpa_sta_info->state == PSK_STATE_PTKINITDONE)
				return ("4-4 (duplicated)");
		else
			return ("invalid state");

	}
	else
		return ("2-2");

}
#endif // DEBUG_PSK

static OCTET_STRING SubStr(OCTET_STRING f, unsigned short s, unsigned short l)
{
	OCTET_STRING res;

	res.Length = l;
	res.Octet = f.Octet+s;
	return res;
}

static void i_P_SHA1(
	unsigned char*  key,                // pointer to authentication key
	int             key_len,            // length of authentication key
	unsigned char*  text,               // pointer to data stream
	int             text_len,           // length of data stream
	unsigned char*  digest,             // caller digest to be filled in
	int				digest_len			// in byte
	)
{
	int i;
	int offset=0;
	int step=20;
	int IterationNum=(digest_len+step-1)/step;

	for(i=0;i<IterationNum;i++)
	{
		text[text_len]=(unsigned char)i;
		hmac_sha(key,key_len,text,text_len+1,digest+offset,step);
		offset+=step;
	}
}

static void i_PRF(
	unsigned char*	secret,
	int				secret_len,
	unsigned char*	prefix,
	int				prefix_len,
	unsigned char*	random,
	int				random_len,
	unsigned char*  digest,             // caller digest to be filled in
	int				digest_len			// in byte
	)
{
	unsigned char data[1000];
	memcpy(data,prefix,prefix_len);
	data[prefix_len++]=0;
	memcpy(data+prefix_len,random,random_len);
	i_P_SHA1(secret,secret_len,data,prefix_len+random_len,digest,digest_len);
}


/*
 * F(P, S, c, i) = U1 xor U2 xor ... Uc
 * U1 = PRF(P, S || Int(i))
 * U2 = PRF(P, U1)
 * Uc = PRF(P, Uc-1)
 */

static void F(
	char *password,
	int passwordlength,
	unsigned char *ssid,
	int ssidlength,
	int iterations,
	int count,
	unsigned char *output)
{
	unsigned char digest[36], digest1[A_SHA_DIGEST_LEN];
	int i, j;

	/* U1 = PRF(P, S || int(i)) */
	memcpy(digest, ssid, ssidlength);
	digest[ssidlength] = (unsigned char)((count>>24) & 0xff);
	digest[ssidlength+1] = (unsigned char)((count>>16) & 0xff);
	digest[ssidlength+2] = (unsigned char)((count>>8) & 0xff);
	digest[ssidlength+3] = (unsigned char)(count & 0xff);
	hmac_sha1(digest, ssidlength + 4,
		(unsigned char*) password, (int)strlen(password),
           	digest1);

	/*
	hmac_sha1((unsigned char*) password, passwordlength,
           digest, ssidlength+4, digest1);
	*/

	/* output = U1 */
	memcpy(output, digest1, A_SHA_DIGEST_LEN);

	for (i = 1; i < iterations; i++) {
		/* Un = PRF(P, Un-1) */
		hmac_sha1(digest1, A_SHA_DIGEST_LEN, (unsigned char*) password,
				(int)strlen(password), digest);
		//hmac_sha1((unsigned char*) password, passwordlength,digest1, A_SHA_DIGEST_LEN, digest);
		memcpy(digest1, digest, A_SHA_DIGEST_LEN);

		/* output = output xor Un */
		for (j = 0; j < A_SHA_DIGEST_LEN; j++) {
			output[j] ^= digest[j];
		}
	}
}

/*
 * password - ascii string up to 63 characters in length
 * ssid - octet string up to 32 octets
 * ssidlength - length of ssid in octets
 * output must be 40 octets in length and outputs 256 bits of key
 */
static int PasswordHash (
	char *password,
	unsigned char *ssid,
	unsigned char *output)
{
	int passwordlength = strlen(password);
	int ssidlength = strlen(ssid);

	if ((passwordlength > 63) || (ssidlength > 32))
		return 0;

	F(password, passwordlength, ssid, ssidlength, 4096, 1, output);
	F(password, passwordlength, ssid, ssidlength, 4096, 2, &output[A_SHA_DIGEST_LEN]);
	return 1;
}

static void Message_ReplayCounter_OC2LI(OCTET_STRING f, LARGE_INTEGER * li)
{
	li->field.HighPart = ((unsigned long)(*(f.Octet + ReplayCounterPos + 3)))
					     + ((unsigned long)(*(f.Octet + ReplayCounterPos+ 2)) <<8 )
						 + ((unsigned long)(*(f.Octet + ReplayCounterPos + 1)) <<  16)
						 + ((unsigned long)(*(f.Octet + ReplayCounterPos + 0)) <<24);
	li->field.LowPart =  ((unsigned long)(*(f.Octet + ReplayCounterPos + 7)))
						 + ((unsigned long)(*(f.Octet + ReplayCounterPos + 6)) <<8 )
					  	 + ((unsigned long)(*(f.Octet + ReplayCounterPos + 5)) <<  16)
						 + ((unsigned long)(*(f.Octet + ReplayCounterPos + 4)) <<24);
}

#if 1
/*-----------------------------------------------------------------------------------------------
	f is EAPOL-KEY message
------------------------------------------------------------------------------------------------*/
static int Message_EqualReplayCounter(LARGE_INTEGER li1, OCTET_STRING f)
{
	LARGE_INTEGER li2;
	Message_ReplayCounter_OC2LI(f, &li2);
	if(li1.field.HighPart == li2.field.HighPart && li1.field.LowPart == li2.field.LowPart)
		return 1;
	else
		return 0;
}
#endif

#ifdef CLIENT_MODE
/*-------------------------------------------------------------------------------------------
	li1 is recorded replay counter on STA
	f is the replay counter from EAPOL-KEY message
---------------------------------------------------------------------------------------------*/
static int Message_SmallerEqualReplayCounter(LARGE_INTEGER li1, OCTET_STRING f)
{
	LARGE_INTEGER li2;
	Message_ReplayCounter_OC2LI(f, &li2);
	if(li2.field.HighPart > li1.field.HighPart)
		return 0;
	else if(li2.field.HighPart < li1.field.HighPart)
		return 1;
	else if(li2.field.LowPart > li1.field.LowPart)
		return 0;
	else if(li2.field.LowPart <= li1.field.LowPart)
		return 1;
	else
		return 0;
}
#endif

/*---------------------------------------------------------------------------------------------
	li1 is recorded replay counter on STA
	f is the replay counter from EAPOL-KEY message
-----------------------------------------------------------------------------------------------*/
static int Message_LargerReplayCounter(LARGE_INTEGER li1, OCTET_STRING f)
{
	LARGE_INTEGER li2;
	Message_ReplayCounter_OC2LI(f, &li2);

	if(li2.field.HighPart > li1.field.HighPart)
		return 1;
	else if(li2.field.LowPart > li1.field.LowPart)
		return 1;
	else
		return 0;
}

static void Message_setReplayCounter(OCTET_STRING f, unsigned long h, unsigned long l)
{
	LARGE_INTEGER *li = (LARGE_INTEGER *)(f.Octet + ReplayCounterPos);
	li->charData[0] = (unsigned char)(h >> 24) & 0xff;
	li->charData[1] = (unsigned char)(h >> 16) & 0xff;
	li->charData[2] = (unsigned char)(h >>  8) & 0xff;
	li->charData[3] = (unsigned char)(h >>  0) & 0xff;
	li->charData[4] = (unsigned char)(l >> 24) & 0xff;
	li->charData[5] = (unsigned char)(l >> 16) & 0xff;
	li->charData[6] = (unsigned char)(l >>  8) & 0xff;
	li->charData[7] = (unsigned char)(l >>  0) & 0xff;
}

static void ConstructIE(struct rtl8190_priv *priv, unsigned char *pucOut, int *usOutLen)
{
	DOT11_RSN_IE_HEADER dot11RSNIEHeader = { 0 };
	DOT11_RSN_IE_SUITE dot11RSNGroupSuite;
	DOT11_RSN_IE_COUNT_SUITE *pDot11RSNPairwiseSuite = NULL;
	DOT11_RSN_IE_COUNT_SUITE *pDot11RSNAuthSuite = NULL;
	unsigned short usSuitCount;
	unsigned long ulIELength = 0;
	unsigned long ulIndex = 0;
	unsigned long ulPairwiseLength = 0;
	unsigned long ulAuthLength = 0;
	unsigned char *pucBlob;
	DOT11_RSN_IE_COUNT_SUITE countSuite, authCountSuite;
#ifdef RTL_WPA2
	DOT11_RSN_CAPABILITY dot11RSNCapability = { 0 };
	unsigned long uCipherAlgo = 0;
	int bCipherAlgoEnabled = FALSE;
	unsigned long uAuthAlgo = 0;
	int bAuthAlgoEnabled = FALSE;
	unsigned long ulRSNCapabilityLength = 0;
#endif

	*usOutLen = 0;
	if ( priv->pmib->dot1180211AuthEntry.dot11EnablePSK & PSK_WPA) {
		//
		// Construct Information Header
		//
		dot11RSNIEHeader.ElementID = RSN_ELEMENT_ID;
		dot11RSNIEHeader.OUI[0] = 0x00;
		dot11RSNIEHeader.OUI[1] = 0x50;
		dot11RSNIEHeader.OUI[2] = 0xf2;
		dot11RSNIEHeader.OUI[3] = 0x01;
		dot11RSNIEHeader.Version = cpu_to_le16(RSN_VER1);
		ulIELength += sizeof(DOT11_RSN_IE_HEADER);

		// Construct Cipher Suite:
		// - Multicast Suite:
		memset(&dot11RSNGroupSuite, 0, sizeof dot11RSNGroupSuite);
		dot11RSNGroupSuite.OUI[0] = 0x00;
		dot11RSNGroupSuite.OUI[1] = 0x50;
		dot11RSNGroupSuite.OUI[2] = 0xF2;
		dot11RSNGroupSuite.Type = priv->wpa_global_info->MulticastCipher;
		ulIELength += sizeof(DOT11_RSN_IE_SUITE);

    	// - UnicastSuite
        pDot11RSNPairwiseSuite = &countSuite;
        memset(pDot11RSNPairwiseSuite, 0, sizeof(DOT11_RSN_IE_COUNT_SUITE));
		usSuitCount = 0;
        for (ulIndex=0; ulIndex<priv->wpa_global_info->NumOfUnicastCipher; ulIndex++)
        {
			pDot11RSNPairwiseSuite->dot11RSNIESuite[usSuitCount].OUI[0] = 0x00;
			pDot11RSNPairwiseSuite->dot11RSNIESuite[usSuitCount].OUI[1] = 0x50;
			pDot11RSNPairwiseSuite->dot11RSNIESuite[usSuitCount].OUI[2] = 0xF2;
			pDot11RSNPairwiseSuite->dot11RSNIESuite[usSuitCount].Type = priv->wpa_global_info->UnicastCipher[ulIndex];
			usSuitCount++;
        }
		pDot11RSNPairwiseSuite->SuiteCount = cpu_to_le16(usSuitCount);
        ulPairwiseLength = sizeof(pDot11RSNPairwiseSuite->SuiteCount) + usSuitCount*sizeof(DOT11_RSN_IE_SUITE);
        ulIELength += ulPairwiseLength;

		//
		// Construction of Auth Algo List
		//
        pDot11RSNAuthSuite = &authCountSuite;
        memset(pDot11RSNAuthSuite, 0, sizeof(DOT11_RSN_IE_COUNT_SUITE));
		usSuitCount = 0;
		pDot11RSNAuthSuite->dot11RSNIESuite[usSuitCount].OUI[0] = 0x00;
		pDot11RSNAuthSuite->dot11RSNIESuite[usSuitCount].OUI[1] = 0x50;
		pDot11RSNAuthSuite->dot11RSNIESuite[usSuitCount].OUI[2] = 0xF2;
		pDot11RSNAuthSuite->dot11RSNIESuite[usSuitCount].Type = DOT11_AuthKeyType_PSK;
	    usSuitCount++;

		pDot11RSNAuthSuite->SuiteCount = cpu_to_le16(usSuitCount);
        ulAuthLength = sizeof(pDot11RSNAuthSuite->SuiteCount) + usSuitCount*sizeof(DOT11_RSN_IE_SUITE);
        ulIELength += ulAuthLength;

	//---------------------------------------------------------------------------------------------
	// Do not encapsulate capability field to solve TI WPA issue
	//---------------------------------------------------------------------------------------------
	/*
        dot11RSNCapability.field.PreAuthentication = 0;//auth->RSNVariable.isSupportPreAuthentication
        dot11RSNCapability.field.PairwiseAsDefaultKey = auth->RSNVariable.isSupportPairwiseAsDefaultKey;
        switch(auth->RSNVariable.NumOfRxTSC)
        {
        case 1:
	        dot11RSNCapability.field.NumOfReplayCounter = 0;
        	break;
	case 2:
		dot11RSNCapability.field.NumOfReplayCounter = 1;
		break;
	case 4:
		dot11RSNCapability.field.NumOfReplayCounter = 2;
		break;
	case 16:
		dot11RSNCapability.field.NumOfReplayCounter = 3;
        	break;
        default:
		dot11RSNCapability.field.NumOfReplayCounter = 0;
        }

        ulRSNCapabilityLength = sizeof(DOT11_RSN_CAPABILITY);
        ulIELength += ulRSNCapabilityLength;
	*/

		pucBlob = pucOut;
		pucBlob += sizeof(DOT11_RSN_IE_HEADER);
		memcpy(pucBlob, &dot11RSNGroupSuite, sizeof(DOT11_RSN_IE_SUITE));
		pucBlob += sizeof(DOT11_RSN_IE_SUITE);
		memcpy(pucBlob, pDot11RSNPairwiseSuite, ulPairwiseLength);
		pucBlob += ulPairwiseLength;
		memcpy(pucBlob, pDot11RSNAuthSuite, ulAuthLength);
		pucBlob += ulAuthLength;

		*usOutLen = (int)ulIELength;
		pucBlob = pucOut;
		dot11RSNIEHeader.Length = (unsigned char)ulIELength - 2; //This -2 is to minus elementID and Length in OUI header
		memcpy(pucBlob, &dot11RSNIEHeader, sizeof(DOT11_RSN_IE_HEADER));
	}

#ifdef RTL_WPA2
	if ( priv->pmib->dot1180211AuthEntry.dot11EnablePSK & PSK_WPA2 ) {
       	DOT11_WPA2_IE_HEADER dot11WPA2IEHeader = { 0 };
		ulIELength = 0;
		ulIndex = 0;
		ulPairwiseLength = 0;
		uCipherAlgo = 0;
		bCipherAlgoEnabled = FALSE;
		ulAuthLength = 0;
		uAuthAlgo = 0;
		bAuthAlgoEnabled = FALSE;
		ulRSNCapabilityLength = 0;

		//
		// Construct Information Header
		//
		dot11WPA2IEHeader.ElementID = WPA2_ELEMENT_ID;
		dot11WPA2IEHeader.Version = cpu_to_le16(RSN_VER1);
		ulIELength += sizeof(DOT11_WPA2_IE_HEADER);

		// Construct Cipher Suite:
		//      - Multicast Suite:
		//
		memset(&dot11RSNGroupSuite, 0, sizeof(dot11RSNGroupSuite));
		dot11RSNGroupSuite.OUI[0] = 0x00;
		dot11RSNGroupSuite.OUI[1] = 0x0F;
		dot11RSNGroupSuite.OUI[2] = 0xAC;
		dot11RSNGroupSuite.Type = priv->wpa_global_info->MulticastCipher;;
		ulIELength += sizeof(DOT11_RSN_IE_SUITE);

		//      - UnicastSuite
        pDot11RSNPairwiseSuite = &countSuite;
        memset(pDot11RSNPairwiseSuite, 0, sizeof(DOT11_RSN_IE_COUNT_SUITE));
		usSuitCount = 0;
		for (ulIndex=0; ulIndex<priv->wpa_global_info->NumOfUnicastCipherWPA2; ulIndex++)
        {
			pDot11RSNPairwiseSuite->dot11RSNIESuite[usSuitCount].OUI[0] = 0x00;
			pDot11RSNPairwiseSuite->dot11RSNIESuite[usSuitCount].OUI[1] = 0x0F;
			pDot11RSNPairwiseSuite->dot11RSNIESuite[usSuitCount].OUI[2] = 0xAC;
			pDot11RSNPairwiseSuite->dot11RSNIESuite[usSuitCount].Type = priv->wpa_global_info->UnicastCipherWPA2[ulIndex];
			usSuitCount++;
        }
		pDot11RSNPairwiseSuite->SuiteCount = cpu_to_le16(usSuitCount);
        ulPairwiseLength = sizeof(pDot11RSNPairwiseSuite->SuiteCount) + usSuitCount*sizeof(DOT11_RSN_IE_SUITE);
        ulIELength += ulPairwiseLength;

		//
		// Construction of Auth Algo List
		//
        pDot11RSNAuthSuite = &authCountSuite;
        memset(pDot11RSNAuthSuite, 0, sizeof(DOT11_RSN_IE_COUNT_SUITE));
		usSuitCount = 0;
		pDot11RSNAuthSuite->dot11RSNIESuite[usSuitCount].OUI[0] = 0x00;
		pDot11RSNAuthSuite->dot11RSNIESuite[usSuitCount].OUI[1] = 0x0F;
		pDot11RSNAuthSuite->dot11RSNIESuite[usSuitCount].OUI[2] = 0xAC;
		pDot11RSNAuthSuite->dot11RSNIESuite[usSuitCount].Type = DOT11_AuthKeyType_PSK;
	    usSuitCount++;

		pDot11RSNAuthSuite->SuiteCount = cpu_to_le16(usSuitCount);
        ulAuthLength = sizeof(pDot11RSNAuthSuite->SuiteCount) + usSuitCount*sizeof(DOT11_RSN_IE_SUITE);
        ulIELength += ulAuthLength;

		//---------------------------------------------------------------------------------------------
		// Do not encapsulate capability field to solve TI WPA issue
		//---------------------------------------------------------------------------------------------

//#ifdef RTL_WPA2
#if 1
		dot11RSNCapability.field.PreAuthentication = 0;
#else
		dot11RSNCapability.field.PairwiseAsDefaultKey = auth->RSNVariable.isSupportPairwiseAsDefaultKey;
		switch(auth->RSNVariable.NumOfRxTSC)
		{
		case 1:
			dot11RSNCapability.field.NumOfReplayCounter = 0;
			break;
		case 2:
			dot11RSNCapability.field.NumOfReplayCounter = 1;
			break;
		case 4:
			dot11RSNCapability.field.NumOfReplayCounter = 2;
			break;
		case 16:
			dot11RSNCapability.field.NumOfReplayCounter = 3;
			break;
		default:
			dot11RSNCapability.field.NumOfReplayCounter = 0;
		}
#endif
		ulRSNCapabilityLength = sizeof(DOT11_RSN_CAPABILITY);
		ulIELength += ulRSNCapabilityLength;

		pucBlob = pucOut + *usOutLen;
		pucBlob += sizeof(DOT11_WPA2_IE_HEADER);
		memcpy(pucBlob, &dot11RSNGroupSuite, sizeof(DOT11_RSN_IE_SUITE));
		pucBlob += sizeof(DOT11_RSN_IE_SUITE);
		memcpy(pucBlob, pDot11RSNPairwiseSuite, ulPairwiseLength);
		pucBlob += ulPairwiseLength;
		memcpy(pucBlob, pDot11RSNAuthSuite, ulAuthLength);
		pucBlob += ulAuthLength;
		memcpy(pucBlob, &dot11RSNCapability, ulRSNCapabilityLength);

		pucBlob = pucOut + *usOutLen;
		dot11WPA2IEHeader.Length = (unsigned char)ulIELength - 2; //This -2 is to minus elementID and Length in OUI header
		memcpy(pucBlob, &dot11WPA2IEHeader, sizeof(DOT11_WPA2_IE_HEADER));
		*usOutLen = *usOutLen + (int)ulIELength;
   	}
#endif // RTL_WPA2

}

static void INCLargeInteger(LARGE_INTEGER * x)
{
	if (x->field.LowPart == 0xffffffff) {
		if (x->field.HighPart == 0xffffffff) {
			x->field.HighPart = 0;
			x->field.LowPart = 0;
		}
		else {
			x->field.HighPart++;
			x->field.LowPart = 0;
		}
	}
	else
		x->field.LowPart++;
}

static void INCOctet16_INTEGER(OCTET16_INTEGER * x)
{
	if (LargeIntegerOverflow(x->field.LowPart)){
		if (LargeIntegerOverflow(x->field.HighPart)) {
			LargeIntegerZero(x->field.HighPart);
			LargeIntegerZero(x->field.LowPart);
		}
		else {
			INCLargeInteger(&x->field.HighPart);
			LargeIntegerZero(x->field.LowPart);
		}
	}
	else
		INCLargeInteger(&x->field.LowPart);

}

static OCTET32_INTEGER *INCOctet32_INTEGER(OCTET32_INTEGER * x)
{
	if (Octet16IntegerOverflow(x->field.LowPart)) {
		if (Octet16IntegerOverflow(x->field.HighPart)) {
			Octet16IntegerZero(x->field.HighPart);
			Octet16IntegerZero(x->field.LowPart);
		}
		else {
			INCOctet16_INTEGER(&x->field.HighPart);
			Octet16IntegerZero( x->field.LowPart);
		}
	}
	else
		INCOctet16_INTEGER(&x->field.LowPart);
	return x;
}

#if defined(RTL8192SU) && defined(CONFIG_ENABLE_MIPS16)
__attribute__((nomips16))
#endif
static void SetEAPOL_KEYIV(OCTET_STRING ocDst, OCTET32_INTEGER oc32Counter)
{
	unsigned char *ptr = ocDst.Octet;
	unsigned long ulTmp;

	ulTmp = cpu_to_le32(oc32Counter.field.HighPart.field.HighPart.field.HighPart);
	memcpy(ptr, (char *)&ulTmp, sizeof(ulTmp));
	ptr+=4;

	ulTmp = cpu_to_le32(oc32Counter.field.HighPart.field.HighPart.field.LowPart);
	memcpy(ptr, (char *)&ulTmp, sizeof(ulTmp));
	ptr+=4;

	ulTmp = cpu_to_le32(oc32Counter.field.HighPart.field.LowPart.field.HighPart);
	memcpy(ptr, (char *)&ulTmp, sizeof(ulTmp));
	ptr+=4;

	ulTmp = cpu_to_le32(oc32Counter.field.HighPart.field.LowPart.field.LowPart);
	memcpy(ptr, (char *)&ulTmp, sizeof(ulTmp));

	if (ocDst.Length == 16) // for AES
		return;

	ptr+=4;

 	ulTmp = cpu_to_le32(oc32Counter.field.LowPart.field.HighPart.field.HighPart);
 	memcpy(ptr, (char *)&ulTmp, sizeof(ulTmp));
	ptr+=4;

 	ulTmp = cpu_to_le32(oc32Counter.field.LowPart.field.HighPart.field.LowPart);
 	memcpy(ptr, (char *)&ulTmp, sizeof(ulTmp));
	ptr+=4;

 	ulTmp = cpu_to_le32(oc32Counter.field.LowPart.field.LowPart.field.HighPart);
 	memcpy(ptr, (char *)&ulTmp, sizeof(ulTmp));
	ptr+=4;

 	ulTmp = cpu_to_le32(oc32Counter.field.LowPart.field.LowPart.field.LowPart);
 	memcpy(ptr, (char *)&ulTmp, sizeof(ulTmp));
}

static void EncGTK(struct rtl8190_priv *priv, struct stat_info *pstat,
			unsigned char *kek, int keklen, unsigned char *key,
			int keylen, unsigned char *out, unsigned short *outlen)
{

	unsigned char tmp1[257], tmp2[257];
	RC4_KEY	 rc4key;
#ifdef RTL_WPA2
	unsigned char default_key_iv[] = { 0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6 };
#endif
	OCTET_STRING EAPOLMsgSend;
	lib1x_eapol_key *eapolkey;

	EAPOLMsgSend.Octet = pstat->wpa_sta_info->EAPOLMsgSend.Octet;
	eapolkey = (lib1x_eapol_key *)(EAPOLMsgSend.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);

// should refer tx packet, david+2006-04-06
//	if (Message_KeyDescVer(pstat->wpa_sta_info->EapolKeyMsgRecvd) == key_desc_ver1) {
	if (Message_KeyDescVer(pstat->wpa_sta_info->EapolKeyMsgSend) == key_desc_ver1) {

		memcpy(tmp1, eapolkey->key_iv, KEY_IV_LEN);
		memcpy(tmp1 + KEY_IV_LEN, kek, keklen);

		RC4_set_key(&rc4key, KEY_IV_LEN + keklen, tmp1);

		//first 256 bytes are discarded
		RC4(&rc4key, 256, (unsigned char *)tmp1, (unsigned char *)tmp2);
   		RC4(&rc4key, keylen, (unsigned char *)key, out);
		*outlen = keylen;
	}
#ifdef RTL_WPA2
	else
		//according to p75 of 11i/D3.0, the IV should be put in the least significant octecs of
		//KeyIV field which shall be padded with 0, so eapolkey->key_iv + 8
		AES_WRAP(key, keylen, default_key_iv, 8, kek, keklen, out, outlen);
#endif
}

static int CheckMIC(OCTET_STRING EAPOLMsgRecvd, unsigned char *key, int keylen)
{
	int retVal = 0;
	OCTET_STRING EapolKeyMsgRecvd;
	unsigned char ucAlgo;
	OCTET_STRING tmp; //copy of overall 802.1x message
	unsigned char tmpbuf[512];
	struct lib1x_eapol *tmpeapol;
	lib1x_eapol_key *tmpeapolkey;
	unsigned char sha1digest[20];

	EapolKeyMsgRecvd.Octet = EAPOLMsgRecvd.Octet +
					ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;
	EapolKeyMsgRecvd.Length = EAPOLMsgRecvd.Length -
					(ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);
	ucAlgo = Message_KeyDescVer(EapolKeyMsgRecvd);

	tmp.Length = EAPOLMsgRecvd.Length;
	tmp.Octet = tmpbuf;
	memcpy(tmp.Octet, EAPOLMsgRecvd.Octet, EAPOLMsgRecvd.Length);
	tmpeapol = (struct lib1x_eapol *)(tmp.Octet + ETHER_HDRLEN);
	tmpeapolkey = (lib1x_eapol_key *)(tmp.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);
	memset(tmpeapolkey->key_mic, 0, KEY_MIC_LEN);

	if (ucAlgo == key_desc_ver1) {
		hmac_md5((unsigned char*)tmpeapol, EAPOLMsgRecvd.Length - ETHER_HDRLEN ,
					key, keylen, tmpeapolkey->key_mic);
#if 0
		lib1x_hexdump2(MESS_DBG_KEY_MANAGE, "CheckMIC", EapolKeyMsgRecvd.Octet +
					KeyMICPos, KEY_MIC_LEN, "Original");
		lib1x_hexdump2(MESS_DBG_KEY_MANAGE, "CheckMIC", tmpeapolkey->key_mic,
					KEY_MIC_LEN, "Calculated");
#endif
		if (!memcmp(tmpeapolkey->key_mic, EapolKeyMsgRecvd.Octet + KeyMICPos, KEY_MIC_LEN))
			retVal = 1;
	}
	else if (ucAlgo == key_desc_ver2) {
		hmac_sha1((unsigned char*)tmpeapol, EAPOLMsgRecvd.Length - ETHER_HDRLEN ,	key, keylen, sha1digest);
		if (!memcmp(sha1digest, EapolKeyMsgRecvd.Octet + KeyMICPos, KEY_MIC_LEN))
			retVal = 1;
	}
	return retVal;
}

static void CalcMIC(OCTET_STRING EAPOLMsgSend, int algo, unsigned char *key, int keylen)
{
	struct lib1x_eapol *eapol = (struct lib1x_eapol *)(EAPOLMsgSend.Octet+ETHER_HDRLEN);
	lib1x_eapol_key *eapolkey = (lib1x_eapol_key *)(EAPOLMsgSend.Octet+ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);
	unsigned char sha1digest[20];

	memset(eapolkey->key_mic, 0, KEY_MIC_LEN);

	if (algo == key_desc_ver1)
  		hmac_md5((unsigned char*)eapol, EAPOLMsgSend.Length - ETHER_HDRLEN ,
					key, keylen, eapolkey->key_mic);
	else if (algo == key_desc_ver2) {
		hmac_sha1((unsigned char*)eapol, EAPOLMsgSend.Length - ETHER_HDRLEN ,
					key, keylen, sha1digest);
		memcpy(eapolkey->key_mic, sha1digest, KEY_MIC_LEN);
	}
}

/* GTK-PRF-X
   X = 256 in TKIP
   X = 128 in CCMP, WRAP, and WEP
*/
static void CalcGTK(unsigned char *addr, unsigned char *nonce,
				unsigned char *keyin, int keyinlen,
				unsigned char *keyout, int keyoutlen)
{
	unsigned char data[ETHER_ADDRLEN + KEY_NONCE_LEN], tmp[64];

	memcpy(data, addr, ETHER_ADDRLEN);
	memcpy(data + ETHER_ADDRLEN, nonce, KEY_NONCE_LEN);

	i_PRF(keyin, keyinlen, (unsigned char*)GMK_EXPANSION_CONST,
						GMK_EXPANSION_CONST_SIZE, data, sizeof(data),
						tmp, keyoutlen);
	memcpy(keyout, tmp, keyoutlen);
}

static int MIN(unsigned char *ucStr1, unsigned char *ucStr2, unsigned long ulLen)
{
	int i;
	for (i=0 ; i<ulLen ; i++) {
		if ((unsigned char)ucStr1[i] < (unsigned char)ucStr2[i])
				return -1;
		else if((unsigned char)ucStr1[i] > (unsigned char)ucStr2[i])
			return 1;
		else if(i == ulLen - 1)
  			return 0;
		else
			continue;
	}
	return 0;
}

static void CalcPTK(unsigned char *addr1, unsigned char *addr2,
					unsigned char *nonce1, unsigned char *nonce2,
					unsigned char * keyin, int keyinlen,
					unsigned char *keyout, int keyoutlen)
{
	unsigned char data[2*ETHER_ADDRLEN+ 2*KEY_NONCE_LEN], tmpPTK[128];

	if(MIN(addr1, addr2, ETHER_ADDRLEN)<=0)
	{
		memcpy(data, addr1, ETHER_ADDRLEN);
		memcpy(data + ETHER_ADDRLEN, addr2, ETHER_ADDRLEN);
	}else
	{
		memcpy(data, addr2, ETHER_ADDRLEN);
		memcpy(data + ETHER_ADDRLEN, addr1, ETHER_ADDRLEN);
	}
	if(MIN(nonce1, nonce2, KEY_NONCE_LEN)<=0)
	{
		memcpy(data + 2*ETHER_ADDRLEN, nonce1, KEY_NONCE_LEN);
		memcpy(data + 2*ETHER_ADDRLEN + KEY_NONCE_LEN, nonce2, KEY_NONCE_LEN);
	}else
	{
		memcpy(data + 2*ETHER_ADDRLEN, nonce2, KEY_NONCE_LEN);
		memcpy(data + 2*ETHER_ADDRLEN + KEY_NONCE_LEN, nonce1, KEY_NONCE_LEN);
	}

	i_PRF(keyin, keyinlen, (unsigned char*)PMK_EXPANSION_CONST,
						PMK_EXPANSION_CONST_SIZE, data,sizeof(data),
						tmpPTK, PTK_LEN_TKIP);
	memcpy(keyout, tmpPTK, keyoutlen);
}

#ifdef CLIENT_MODE
/*
	decrypt WPA2 Message 3's Key Data
*/
// Use RC4 or AES to decode the keydata by checking desc-ver, david-2006-01-06
//int DecWPA2KeyData(u_char *key, int keylen, u_char *kek, int keklen, u_char *kout)
int DecWPA2KeyData(WPA_STA_INFO* pStaInfo, unsigned char *key, int keylen, unsigned char *kek, int keklen, unsigned char *kout)
{
	int	retVal = 0;
	unsigned char		default_key_iv[] = { 0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6 };
	unsigned char		tmp2[257];

// Use RC4 or AES to decode the keydata by checking desc-ver, david-2006-01-06
	unsigned char 	tmp1[257];
	RC4_KEY			rc4key;

	lib1x_eapol_key *eapolkey = (lib1x_eapol_key *)(pStaInfo->EAPOLMsgRecvd.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);

	if (Message_KeyDescVer(pStaInfo->EapolKeyMsgRecvd) == key_desc_ver1) {
		memcpy(tmp1, eapolkey->key_iv, KEY_IV_LEN);
		memcpy(tmp1+KEY_IV_LEN, kek, keklen);
		RC4_set_key(&rc4key, KEY_IV_LEN + keklen, tmp1);

		//first 256 bits is discard
		RC4(&rc4key, 256, (unsigned char*)tmp1, (unsigned char*)tmp2);

		//RC4(&rc4key, keylen, eapol_key->key_data, global->skm_sm->GTK[Message_KeyIndex(global->EapolKeyMsgRecvd)]);
		RC4(&rc4key, keylen, pStaInfo->EapolKeyMsgRecvd.Octet + KeyDataPos, (unsigned char*)tmp2);

		memcpy(kout, tmp2, keylen);
		//memcpy(&global->supp_kmsm->GTK[Message_KeyIndex(global->EapolKeyMsgRecvd)], tmp2, keylen);
		retVal = 1;
	}
	else {
//--------------------------------------------------------
		AES_UnWRAP(key, keylen, kek, keklen, tmp2);
		if(memcmp(tmp2, default_key_iv, 8))
			retVal = 0;
		else {
			memcpy(kout, tmp2+8, keylen);
			retVal = 1;
		}
	}
	return retVal;
}

int DecGTK(OCTET_STRING EAPOLMsgRecvd, unsigned char *kek, int keklen, int keylen, unsigned char *kout)
{
	int		retVal = 0;
	unsigned char		tmp1[257], tmp2[257];
	RC4_KEY		rc4key;
	lib1x_eapol_key * eapol_key = (lib1x_eapol_key *)(EAPOLMsgRecvd.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);
	OCTET_STRING	EapolKeyMsgRecvd;
	EapolKeyMsgRecvd.Octet = EAPOLMsgRecvd.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;
	EapolKeyMsgRecvd.Length = EAPOLMsgRecvd.Length - (ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);

	if(Message_KeyDescVer(EapolKeyMsgRecvd) == key_desc_ver1)
	{
		memcpy(tmp1, eapol_key->key_iv, KEY_IV_LEN);
		memcpy(tmp1 + KEY_IV_LEN, kek, keklen);
		RC4_set_key(&rc4key, KEY_IV_LEN + keklen, tmp1);
		//first 256 bits is discard
		RC4(&rc4key, 256, (unsigned char*)tmp1, (unsigned char*)tmp2);
      		//RC4(&rc4key, keylen, eapol_key->key_data, global->skm_sm->GTK[Message_KeyIndex(global->EapolKeyMsgRecvd)]);
		RC4(&rc4key, keylen, EapolKeyMsgRecvd.Octet + KeyDataPos, (unsigned char*)tmp2);
		memcpy(kout, tmp2, keylen);
		//memcpy(&global->supp_kmsm->GTK[Message_KeyIndex(global->EapolKeyMsgRecvd)], tmp2, keylen);
		retVal = 1;
	}
	else if(Message_KeyDescVer(EapolKeyMsgRecvd) == key_desc_ver2)
	{
		// kenny: should use default IV 0xA6A6A6A6A6A6A6A6
		unsigned char	default_key_iv[] = { 0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6 };
// david, get key len from eapol packet
//			AES_UnWRAP(EapolKeyMsgRecvd.Octet + KeyDataPos, keylen + 8, kek, keklen, tmp2);

		keylen = Message_KeyDataLength(EapolKeyMsgRecvd);
		AES_UnWRAP(EapolKeyMsgRecvd.Octet + KeyDataPos, keylen, kek, keklen, tmp2);
//------------------------- 2005-08-01

		//if(memcmp(tmp2, eapol_key->key_iv + 8, 8))
		if(memcmp(tmp2, default_key_iv, 8))
			retVal = 0;
		else
		{
			//memcpy(kout, tmp2, keylen);
			//memcpy(global->supp_kmsm->GTK[Message_KeyIndex(global->EapolKeyMsgRecvd)], tmp2 + 8, keylen - 8);
			memcpy(kout, tmp2+8, keylen);
			retVal = 1;
		}
	}
	return retVal;
}
#endif /* CLIENT_MODE */

static int parseIE(struct rtl8190_priv *priv, WPA_STA_INFO *pInfo,
						unsigned char *pucIE, unsigned long ulIELength)
{
	unsigned short usSuitCount;
	DOT11_RSN_IE_HEADER *pDot11RSNIEHeader;
	DOT11_RSN_IE_SUITE *pDot11RSNIESuite;
	DOT11_RSN_IE_COUNT_SUITE *pDot11RSNIECountSuite;

	DEBUG_TRACE;

	if (ulIELength < sizeof(DOT11_RSN_IE_HEADER)) {
		DEBUG_WARN("parseIE err 1!\n");
		return ERROR_INVALID_RSNIE;
	}

	pDot11RSNIEHeader = (DOT11_RSN_IE_HEADER *)pucIE;
	if (le16_to_cpu(pDot11RSNIEHeader->Version) != RSN_VER1) {
		DEBUG_WARN("parseIE err 2!\n");
		return ERROR_UNSUPPORTED_RSNEVERSION;
	}

	if (pDot11RSNIEHeader->ElementID != RSN_ELEMENT_ID ||
		pDot11RSNIEHeader->Length != ulIELength -2 ||
		pDot11RSNIEHeader->OUI[0] != 0x00 || pDot11RSNIEHeader->OUI[1] != 0x50 ||
		pDot11RSNIEHeader->OUI[2] != 0xf2 || pDot11RSNIEHeader->OUI[3] != 0x01 ) {
		DEBUG_WARN("parseIE err 3!\n");
		return ERROR_INVALID_RSNIE;
	}

	pInfo->RSNEnabled= PSK_WPA;	// wpa
	ulIELength -= sizeof(DOT11_RSN_IE_HEADER);
	pucIE += sizeof(DOT11_RSN_IE_HEADER);

	//----------------------------------------------------------------------------------
 	// Multicast Cipher Suite processing
	//----------------------------------------------------------------------------------
	if (ulIELength < sizeof(DOT11_RSN_IE_SUITE))
		return 0;

	pDot11RSNIESuite = (DOT11_RSN_IE_SUITE *)pucIE;
	if (pDot11RSNIESuite->OUI[0] != 0x00 ||
		pDot11RSNIESuite->OUI[1] != 0x50 ||
		pDot11RSNIESuite->OUI[2] != 0xF2) {
		DEBUG_WARN("parseIE err 4!\n");
		return ERROR_INVALID_RSNIE;
	}

	if (pDot11RSNIESuite->Type > DOT11_ENC_WEP104) {
		DEBUG_WARN("parseIE err 5!\n");
		return ERROR_INVALID_MULTICASTCIPHER;
	}

	if (pDot11RSNIESuite->Type != priv->wpa_global_info->MulticastCipher) {
		DEBUG_WARN("parseIE err 6!\n");
		return ERROR_INVALID_MULTICASTCIPHER;
	}

	ulIELength -= sizeof(DOT11_RSN_IE_SUITE);
	pucIE += sizeof(DOT11_RSN_IE_SUITE);

	//----------------------------------------------------------------------------------
	// Pairwise Cipher Suite processing
	//----------------------------------------------------------------------------------
	if (ulIELength < 2 + sizeof(DOT11_RSN_IE_SUITE))
		return 0;

	pDot11RSNIECountSuite = (PDOT11_RSN_IE_COUNT_SUITE)pucIE;
	pDot11RSNIESuite = pDot11RSNIECountSuite->dot11RSNIESuite;
	usSuitCount = le16_to_cpu(pDot11RSNIECountSuite->SuiteCount);

	if (usSuitCount != 1 ||
		pDot11RSNIESuite->OUI[0] != 0x00 ||
		pDot11RSNIESuite->OUI[1] != 0x50 ||
		pDot11RSNIESuite->OUI[2] != 0xF2) {
		DEBUG_WARN("parseIE err 7!\n");
		return ERROR_INVALID_RSNIE;
	}

	if (pDot11RSNIESuite->Type > DOT11_ENC_WEP104) {
		DEBUG_WARN("parseIE err 8!\n");
		return ERROR_INVALID_UNICASTCIPHER;
	}

	if ((pDot11RSNIESuite->Type < DOT11_ENC_WEP40)
		|| (!(BIT(pDot11RSNIESuite->Type - 1) & priv->pmib->dot1180211AuthEntry.dot11WPACipher))) {
		DEBUG_WARN("parseIE err 9!\n");
		return ERROR_INVALID_UNICASTCIPHER;
	}

	pInfo->UnicastCipher = pDot11RSNIESuite->Type;

#ifdef DEBUG_PSK
	printk("PSK: ParseIE -> WPA UnicastCipher=%x\n", pInfo->UnicastCipher);
#endif

	ulIELength -= sizeof(pDot11RSNIECountSuite->SuiteCount) + sizeof(DOT11_RSN_IE_SUITE);
	pucIE += sizeof(pDot11RSNIECountSuite->SuiteCount) + sizeof(DOT11_RSN_IE_SUITE);

	//----------------------------------------------------------------------------------
	// Authentication suite
	//----------------------------------------------------------------------------------
	if (ulIELength < 2 + sizeof(DOT11_RSN_IE_SUITE))
		return 0;

	pDot11RSNIECountSuite = (PDOT11_RSN_IE_COUNT_SUITE)pucIE;
	pDot11RSNIESuite = pDot11RSNIECountSuite->dot11RSNIESuite;
	usSuitCount = le16_to_cpu(pDot11RSNIECountSuite->SuiteCount);

	if (usSuitCount != 1 ||
		pDot11RSNIESuite->OUI[0] != 0x00 ||
		pDot11RSNIESuite->OUI[1] != 0x50 ||
		pDot11RSNIESuite->OUI[2] != 0xF2 ) {
		DEBUG_WARN("parseIE err 10!\n");
		return ERROR_INVALID_RSNIE;
	}

	if( pDot11RSNIESuite->Type < DOT11_AuthKeyType_RSN ||
		pDot11RSNIESuite->Type > DOT11_AuthKeyType_PSK) {
		DEBUG_WARN("parseIE err 11!\n");
		return ERROR_INVALID_AUTHKEYMANAGE;
	}

	if(pDot11RSNIESuite->Type != DOT11_AuthKeyType_PSK &&
	   pDot11RSNIESuite->Type != DOT11_AuthKeyType_RSN ) {
		DEBUG_WARN("parseIE err 12!\n");
		return ERROR_INVALID_AUTHKEYMANAGE;
	}

	pInfo->AuthKeyMethod = pDot11RSNIESuite->Type;
	ulIELength -= sizeof(pDot11RSNIECountSuite->SuiteCount) + sizeof(DOT11_RSN_IE_SUITE);
	pucIE += sizeof(pDot11RSNIECountSuite->SuiteCount) + sizeof(DOT11_RSN_IE_SUITE);

	// RSN Capability
	if (ulIELength < sizeof(DOT11_RSN_CAPABILITY))
		return 0;

//#ifndef RTL_WPA2
#if 0
	//----------------------------------------------------------------------------------
    // Capability field
	//----------------------------------------------------------------------------------
	pDot11RSNCapability = (DOT11_RSN_CAPABILITY * )pucIE;
	pInfo->isSuppSupportPreAuthentication = pDot11RSNCapability->field.PreAuthentication;
	pInfo->isSuppSupportPairwiseAsDefaultKey = pDot11RSNCapability->field.PairwiseAsDefaultKey;

	switch (pDot11RSNCapability->field.NumOfReplayCounter) {
	case 0:
		pInfo->NumOfRxTSC = 1;
		break;
	case 1:
		pInfo->NumOfRxTSC = 2;
		break;
	case 2:
		pInfo->NumOfRxTSC = 4;
		break;
	case 3:
		pInfo->NumOfRxTSC = 16;
		break;
	default:
		pInfo->NumOfRxTSC = 1;
	}
#endif /* RTL_WPA2 */

	return 0;
}

#ifdef RTL_WPA2
static int parseIEWPA2(struct rtl8190_priv *priv, WPA_STA_INFO *pInfo,
						unsigned char *pucIE, unsigned long ulIELength)
{
	unsigned short usSuitCount;
	DOT11_WPA2_IE_HEADER *pDot11WPA2IEHeader = NULL;
	DOT11_RSN_IE_SUITE  *pDot11RSNIESuite = NULL;
	DOT11_RSN_IE_COUNT_SUITE *pDot11RSNIECountSuite = NULL;
	DOT11_RSN_CAPABILITY *pDot11RSNCapability = NULL;

	DEBUG_TRACE;

	if (ulIELength < sizeof(DOT11_WPA2_IE_HEADER)) {
		DEBUG_WARN("ERROR_INVALID_RSNIE, err 1!\n");
		return ERROR_INVALID_RSNIE;
	}

	pDot11WPA2IEHeader = (DOT11_WPA2_IE_HEADER *)pucIE;
	if (le16_to_cpu(pDot11WPA2IEHeader->Version) != RSN_VER1) {
		DEBUG_WARN("ERROR_UNSUPPORTED_RSNEVERSION, err 2!\n");
		return ERROR_UNSUPPORTED_RSNEVERSION;
	}

	if (pDot11WPA2IEHeader->ElementID != WPA2_ELEMENT_ID ||
		pDot11WPA2IEHeader->Length != ulIELength -2 ) {
		DEBUG_WARN("ERROR_INVALID_RSNIE, err 3!\n");
		return ERROR_INVALID_RSNIE;
	}

	pInfo->RSNEnabled= PSK_WPA2;
	pInfo->PMKCached= FALSE;

	ulIELength -= sizeof(DOT11_WPA2_IE_HEADER);
	pucIE += sizeof(DOT11_WPA2_IE_HEADER);

	//----------------------------------------------------------------------------------
 	// Multicast Cipher Suite processing
	//----------------------------------------------------------------------------------
	if (ulIELength < sizeof(DOT11_RSN_IE_SUITE))
		return 0;

	pDot11RSNIESuite = (DOT11_RSN_IE_SUITE *)pucIE;
	if (pDot11RSNIESuite->OUI[0] != 0x00 ||
			pDot11RSNIESuite->OUI[1] != 0x0F ||
				pDot11RSNIESuite->OUI[2] != 0xAC) {
		DEBUG_WARN("ERROR_INVALID_RSNIE, err 4!\n");
		return ERROR_INVALID_RSNIE;
	}

	if (pDot11RSNIESuite->Type > DOT11_ENC_WEP104)	{
		DEBUG_WARN("ERROR_INVALID_MULTICASTCIPHER, err 5!\n");
		return ERROR_INVALID_MULTICASTCIPHER;
	}

	if (pDot11RSNIESuite->Type != priv->wpa_global_info->MulticastCipher) {
		DEBUG_WARN("ERROR_INVALID_MULTICASTCIPHER, err 6!\n");
		return ERROR_INVALID_MULTICASTCIPHER;
	}

	ulIELength -= sizeof(DOT11_RSN_IE_SUITE);
	pucIE += sizeof(DOT11_RSN_IE_SUITE);

	//----------------------------------------------------------------------------------
	// Pairwise Cipher Suite processing
	//----------------------------------------------------------------------------------
	if (ulIELength < 2 + sizeof(DOT11_RSN_IE_SUITE))
		return 0;

	pDot11RSNIECountSuite = (PDOT11_RSN_IE_COUNT_SUITE)pucIE;
	pDot11RSNIESuite = pDot11RSNIECountSuite->dot11RSNIESuite;
	usSuitCount = le16_to_cpu(pDot11RSNIECountSuite->SuiteCount);

	if (usSuitCount != 1 ||
		pDot11RSNIESuite->OUI[0] != 0x00 ||
			pDot11RSNIESuite->OUI[1] != 0x0F ||
				pDot11RSNIESuite->OUI[2] != 0xAC) {
		DEBUG_WARN("ERROR_INVALID_RSNIE, err 7!\n");
		return ERROR_INVALID_RSNIE;
	}

	if (pDot11RSNIESuite->Type > DOT11_ENC_WEP104) {
		DEBUG_WARN("ERROR_INVALID_UNICASTCIPHER, err 8!\n");
		return ERROR_INVALID_UNICASTCIPHER;
	}

	if ((pDot11RSNIESuite->Type < DOT11_ENC_WEP40)
		|| (!(BIT(pDot11RSNIESuite->Type - 1) & priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher))) {
		DEBUG_WARN("ERROR_INVALID_UNICASTCIPHER, err 9!\n");
		return ERROR_INVALID_UNICASTCIPHER;
	}

	pInfo->UnicastCipher = pDot11RSNIESuite->Type;
	ulIELength -= sizeof(pDot11RSNIECountSuite->SuiteCount) + sizeof(DOT11_RSN_IE_SUITE);
	pucIE += sizeof(pDot11RSNIECountSuite->SuiteCount) + sizeof(DOT11_RSN_IE_SUITE);

#ifdef DEBUG_PSK
	printk("PSK: ParseIE -> WPA2 UnicastCipher=%x\n", pInfo->UnicastCipher);
#endif

	//----------------------------------------------------------------------------------
	// Authentication suite
	//----------------------------------------------------------------------------------
	if (ulIELength < 2 + sizeof(DOT11_RSN_IE_SUITE))
		return 0;

	pDot11RSNIECountSuite = (PDOT11_RSN_IE_COUNT_SUITE)pucIE;
	pDot11RSNIESuite = pDot11RSNIECountSuite->dot11RSNIESuite;
	pDot11RSNIECountSuite->SuiteCount = le16_to_cpu(usSuitCount);

	if (usSuitCount != 1 ||
			pDot11RSNIESuite->OUI[0] != 0x00 ||
				pDot11RSNIESuite->OUI[1] != 0x0F ||
					pDot11RSNIESuite->OUI[2] != 0xAC ) {
		DEBUG_WARN("ERROR_INVALID_RSNIE, err 10!\n");
		return ERROR_INVALID_RSNIE;
	}

	if (pDot11RSNIESuite->Type < DOT11_AuthKeyType_RSN ||
		  pDot11RSNIESuite->Type > DOT11_AuthKeyType_PSK) {
		DEBUG_WARN("ERROR_INVALID_AUTHKEYMANAGE, err 11!\n");
		return ERROR_INVALID_AUTHKEYMANAGE;
	}

	if(pDot11RSNIESuite->Type != DOT11_AuthKeyType_PSK &&
	   pDot11RSNIESuite->Type != DOT11_AuthKeyType_RSN) {
		DEBUG_WARN("ERROR_INVALID_AUTHKEYMANAGE, err 12!\n");
		return ERROR_INVALID_AUTHKEYMANAGE;
	}

	pInfo->AuthKeyMethod = pDot11RSNIESuite->Type;
	ulIELength -= sizeof(pDot11RSNIECountSuite->SuiteCount) + sizeof(DOT11_RSN_IE_SUITE);
	pucIE += sizeof(pDot11RSNIECountSuite->SuiteCount) + sizeof(DOT11_RSN_IE_SUITE);

	// RSN Capability
	if (ulIELength < sizeof(DOT11_RSN_CAPABILITY)) {
		pInfo->NumOfRxTSC = 2;
		return 0;
	}

	//----------------------------------------------------------------------------------
	// Capability field
	//----------------------------------------------------------------------------------
	pDot11RSNCapability = (DOT11_RSN_CAPABILITY * )pucIE;
#if 0
	global->RSNVariable.isSuppSupportPreAuthentication = pDot11RSNCapability->field.PreAuthentication;
//#ifdef RTL_WPA2_PREAUTH  // kenny temp
	//wpa2_hexdump("WPA2 IE Capability", pucIE, 2);
	//global->RSNVariable.isSuppSupportPreAuthentication = (pDot11RSNCapability->charData[0] & 0x01)?TRUE:FALSE;
#endif

#if 1
	pInfo->NumOfRxTSC = 1;
#else
	global->RSNVariable.isSuppSupportPairwiseAsDefaultKey = pDot11RSNCapability->field.PairwiseAsDefaultKey;
	switch (pDot11RSNCapability->field.NumOfReplayCounter) {
	case 0:
		global->RSNVariable.NumOfRxTSC = 1;
		break;
	case 1:
		global->RSNVariable.NumOfRxTSC = 2;
		break;
	case 2:
		global->RSNVariable.NumOfRxTSC = 4;
		break;
	case 3:
		global->RSNVariable.NumOfRxTSC = 16;
		break;
	default:
		global->RSNVariable.NumOfRxTSC = 1;
	}
#endif
	pucIE += 2;

	// PMKID
	if ((ulIELength < 2 + PMKID_LEN))
		return 0;

	//----------------------------------------------------------------------------------
	// PMKID Count field
	//----------------------------------------------------------------------------------
	usSuitCount = le16_to_cpu(*((unsigned short *)pucIE));

	//printf("PMKID Count = %d\n",usSuitCount);
	pucIE += 2;
/*
	if ( usSuitCount > 0) {
		struct _WPA2_PMKSA_Node* pmksa_node;
		int i;
		for (i=0; i < usSuitCount; i++) {
			pmksa_node = find_pmksa(pucIE+(PMKID_LEN*i));
			if ( pmksa_node != NULL) {
				//wpa2_hexdump("Cached PMKID found", pmksa_node->pmksa.pmkid, PMKID_LEN);
				global->RSNVariable.PMKCached = TRUE;
				global->RSNVariable.cached_pmk_node = pmksa_node;
				break;
			}
		}

	}
*/
	return 0;
}
#endif // RTL_WPA2

static void GenNonce(unsigned char *nonce, unsigned char *addr)
{
	unsigned char secret[256], random[256], result[256];

	get_random_bytes(random, 256);
	memset(secret, 0, sizeof(secret));

	i_PRF(secret, sizeof(secret), (unsigned char*)RANDOM_EXPANSION_CONST, RANDOM_EXPANSION_CONST_SIZE,
                        random, sizeof(random), result, KEY_NONCE_LEN);

	memcpy(nonce, result, KEY_NONCE_LEN);
}

static void IntegrityFailure(struct rtl8190_priv *priv)
{
	priv->wpa_global_info->IntegrityFailed = FALSE;

	if (priv->wpa_global_info->GKeyFailure) {
		priv->wpa_global_info->GTKRekey = TRUE;
		priv->wpa_global_info->GKeyFailure = FALSE;
	}

	//waitupto60;
	INCOctet32_INTEGER(&priv->wpa_global_info->Counter);
//	SetNonce(global->akm_sm->ANonce, global->auth->Counter);

	INCOctet32_INTEGER(&priv->wpa_global_info->Counter);
//	SetNonce(global->akm_sm->ANonce, global->auth->Counter);
}

static void ToDrv_RspAssoc(struct rtl8190_priv *priv, int id, unsigned char *mac, int status)
{
	DOT11_ASSOCIATIIN_RSP 	Association_Rsp;
	struct iw_point wrq;

	DEBUG_TRACE;

#ifdef DEBUG_PSK
	printk("PSK: Issue assoc-rsp [%x], mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
		status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif

	wrq.pointer = (caddr_t)&Association_Rsp;
	wrq.length = sizeof(DOT11_ASSOCIATIIN_RSP);

	if (id == DOT11_EVENT_ASSOCIATION_IND)
		Association_Rsp.EventId = DOT11_EVENT_ASSOCIATION_RSP;
	else
		Association_Rsp.EventId = DOT11_EVENT_REASSOCIATION_RSP;

	Association_Rsp.IsMoreEvent = FALSE;
	Association_Rsp.Status = status;
	memcpy(Association_Rsp.MACAddr, mac, 6);

	rtl8190_ioctl_priv_daemonreq(priv->dev, &wrq);
}


static void ToDrv_RemovePTK(struct rtl8190_priv *priv, unsigned char *mac, int type)
{
	struct iw_point wrq;
	DOT11_DELETE_KEY Delete_Key;

#ifdef DEBUG_PSK
	printk("PSK: Remove PTK, mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif

	wrq.pointer = (caddr_t)&Delete_Key;
	wrq.length = sizeof(DOT11_DELETE_KEY);

	Delete_Key.EventId = DOT11_EVENT_DELETE_KEY;
	Delete_Key.IsMoreEvent = FALSE;
	Delete_Key.KeyType = type;
	memcpy(&Delete_Key.MACAddr, mac, 6);

	rtl8190_ioctl_priv_daemonreq(priv->dev, &wrq);
}

static void ToDrv_SetPTK(struct rtl8190_priv *priv, struct stat_info *pstat)
{
	unsigned long ulKeyLength = 0;
	unsigned char *pucKeyMaterial = 0;
	struct iw_point wrq;
	DOT11_SET_KEY Set_Key;

#ifdef DEBUG_PSK
	printk("PSK: Set PTK, mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
		pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
		pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5]);
#endif

	wrq.pointer = (caddr_t)&Set_Key;
	Set_Key.EventId = DOT11_EVENT_SET_KEY;
	Set_Key.IsMoreEvent = FALSE;
	Set_Key.KeyIndex = 0;
	Set_Key.KeyType = DOT11_KeyType_Pairwise;
	memcpy(&Set_Key.MACAddr, pstat->hwaddr, 6);
	switch (pstat->wpa_sta_info->UnicastCipher) {
		case DOT11_ENC_TKIP:
		ulKeyLength =  PTK_LEN_TKIP - (PTK_LEN_EAPOLMIC + PTK_LEN_EAPOLENC);
		break;

		// Kenny
		case DOT11_ENC_CCMP:
		ulKeyLength =  PTK_LEN_CCMP - (PTK_LEN_EAPOLMIC + PTK_LEN_EAPOLENC);
		break;
	}
	pucKeyMaterial = pstat->wpa_sta_info->PTK + (PTK_LEN_EAPOLMIC + PTK_LEN_EAPOLENC);

	//sc_yang
	memset(Set_Key.KeyMaterial,0, 64);
	memcpy(Set_Key.KeyMaterial, pucKeyMaterial, ulKeyLength);
	Set_Key.EncType = pstat->wpa_sta_info->UnicastCipher;
	Set_Key.KeyLen = ulKeyLength;
	wrq.length = sizeof(DOT11_SET_KEY) - 1 + ulKeyLength;

#ifdef DEBUG_PSK
	debug_out(NULL, Set_Key.KeyMaterial, ulKeyLength);
#endif

	rtl8190_ioctl_priv_daemonreq(priv->dev, &wrq);
}

void ToDrv_SetGTK(struct rtl8190_priv *priv)
{
	unsigned long ulKeyLength = 0;
	//unsigned long  ulKeyIndex = 0;
	struct iw_point wrq;
	DOT11_SET_KEY  Set_Key;
	unsigned char szBradcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

#ifdef DEBUG_PSK
	printk("PSK: Set GTK\n");
#endif

	wrq.pointer = (caddr_t)&Set_Key;
	wrq.length = sizeof(DOT11_SET_KEY);

	Set_Key.EventId = DOT11_EVENT_SET_KEY;
	Set_Key.IsMoreEvent = FALSE;

	Set_Key.KeyType = DOT11_KeyType_Group;
	memcpy(&Set_Key.MACAddr, szBradcast, 6);

	Set_Key.EncType = priv->wpa_global_info->MulticastCipher;
	//sc_yang
	memset(Set_Key.KeyMaterial,0, 64);
	ulKeyLength = 32;
#ifdef CLIENT_MODE
	if (OPMODE & WIFI_ADHOC_STATE)
	{
		memcpy(Set_Key.KeyMaterial, priv->wpa_global_info->PSK, ulKeyLength);
		Set_Key.KeyIndex = 0;
	}
	else
#endif
	{
		//ulKeyIndex = priv->wpa_global_info->GN;
		memcpy(Set_Key.KeyMaterial,
			priv->wpa_global_info->GTK[priv->wpa_global_info->GN],
			ulKeyLength);
		Set_Key.KeyIndex = priv->wpa_global_info->GN;
	}

	wrq.length = sizeof(DOT11_SET_KEY) - 1 + ulKeyLength;
	wrq.length = sizeof(DOT11_SET_KEY) - 1 + ulKeyLength;

#ifdef DEBUG_PSK
	debug_out(NULL, Set_Key.KeyMaterial, ulKeyLength);
#endif

	rtl8190_ioctl_priv_daemonreq(priv->dev, &wrq);
}



static void ToDrv_SetPort(struct rtl8190_priv *priv, struct stat_info *pstat, int status)
{
	struct iw_point wrq;
	DOT11_SET_PORT	Set_Port;

#ifdef DEBUG_PSK
	printk("PSK: Set PORT [%x], mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
		status, pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
		pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5]);
#endif

	wrq.pointer = (caddr_t)&Set_Port;
	wrq.length = sizeof(DOT11_SET_PORT);
	Set_Port.EventId = DOT11_EVENT_SET_PORT;
	Set_Port.PortStatus = status;
	memcpy(&Set_Port.MACAddr, pstat->hwaddr, 6);
	rtl8190_ioctl_priv_daemonreq(priv->dev, &wrq);
}

static void ToDrv_SetIE(struct rtl8190_priv *priv)
{
	struct iw_point wrq;
	DOT11_SET_RSNIE Set_Rsnie;

#ifdef DEBUG_PSK
	debug_out("PSK: Set RSNIE", priv->wpa_global_info->AuthInfoElement.Octet,
								priv->wpa_global_info->AuthInfoElement.Length);
#endif

	wrq.pointer = (caddr_t)&Set_Rsnie;
	wrq.length = sizeof(DOT11_SET_RSNIE);
	Set_Rsnie.EventId = DOT11_EVENT_SET_RSNIE;
	Set_Rsnie.IsMoreEvent = FALSE;
	Set_Rsnie.Flag = DOT11_Ioctl_Set;
	Set_Rsnie.RSNIELen = priv->wpa_global_info->AuthInfoElement.Length;
	memcpy(&Set_Rsnie.RSNIE,
			priv->wpa_global_info->AuthInfoElement.Octet,
			priv->wpa_global_info->AuthInfoElement.Length);

	rtl8190_ioctl_priv_daemonreq(priv->dev, &wrq);
}

static void ToDrv_DisconnectSTA(struct rtl8190_priv *priv, struct stat_info *pstat, int reason)
{
	struct iw_point wrq;
	DOT11_DISCONNECT_REQ	Disconnect_Req;

#ifdef DEBUG_PSK
	printk("PSK: disconnect sta, mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
		pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
		pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5]);
#endif

	wrq.pointer = (caddr_t)&Disconnect_Req;
	wrq.length = sizeof(DOT11_DISCONNECT_REQ);

	Disconnect_Req.EventId = DOT11_EVENT_DISCONNECT_REQ;
	Disconnect_Req.IsMoreEvent = FALSE;
	Disconnect_Req.Reason = (unsigned short)reason;
	memcpy(Disconnect_Req.MACAddr, pstat->hwaddr, 6);

	rtl8190_ioctl_priv_daemonreq(priv->dev, &wrq);
};

static void ToDrv_IndicateMICFail(struct rtl8190_priv *priv, unsigned char *mac)
{
	struct iw_point wrq;
	DOT11_MIC_FAILURE MIC_Failure;

	wrq.pointer = (caddr_t)&MIC_Failure;
	wrq.length = sizeof(DOT11_INIT_QUEUE);

	MIC_Failure.EventId = DOT11_EVENT_MIC_FAILURE;
	memcpy(MIC_Failure.MACAddr, mac, 6);

	rtl8190_ioctl_priv_daemonreq(priv->dev, &wrq);
}

static void reset_sta_info(struct rtl8190_priv *priv, struct stat_info *pstat)
{
	WPA_STA_INFO *pInfo = pstat->wpa_sta_info;

	unsigned long flags;

	SAVE_INT_AND_CLI(flags);

	if (timer_pending(&pInfo->resendTimer))
		del_timer_sync(&pInfo->resendTimer);

	if (OPMODE & WIFI_AP_STATE)
	{
		ToDrv_RemovePTK(priv, pstat->hwaddr, DOT11_KeyType_Pairwise);
		ToDrv_SetPort(priv, pstat, DOT11_PortStatus_Unauthorized);
	}

	memset((char *)pInfo, '\0', sizeof(WPA_STA_INFO));

	pInfo->ANonce.Octet = pInfo->AnonceBuf;
	pInfo->ANonce.Length = KEY_NONCE_LEN;

	pInfo->SNonce.Octet = pInfo->SnonceBuf;
	pInfo->SNonce.Length = KEY_NONCE_LEN;

	pInfo->EAPOLMsgSend.Octet = pInfo->eapSendBuf;
	pInfo->EapolKeyMsgSend.Octet = pInfo->EAPOLMsgSend.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;

	pInfo->EAPOLMsgRecvd.Octet = pInfo->eapRecvdBuf;
	pInfo->EapolKeyMsgRecvd.Octet = pInfo->EAPOLMsgRecvd.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;

	init_timer(&pInfo->resendTimer);
	pInfo->resendTimer.data = (unsigned long)pstat;
	pInfo->resendTimer.function = ResendTimeout;
	pInfo->priv = priv;

	if (OPMODE & WIFI_AP_STATE)
	{
		pInfo->state = PSK_STATE_IDLE;
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		memcpy(pInfo->PMK, priv->wpa_global_info->PSK, PMK_LEN);

		pInfo->clientHndshkProcessing = FALSE;
		pInfo->clientHndshkDone = FALSE;

		pInfo->CurrentReplayCounter.field.HighPart = 0xffffffff;
		pInfo->CurrentReplayCounter.field.LowPart = 0xffffffff;
	}
#endif

	RESTORE_INT(flags);
}

static void ResendTimeout(unsigned long task_psta)
{
	struct stat_info *pstat = (struct stat_info *)task_psta;
	struct rtl8190_priv *priv = pstat->wpa_sta_info->priv;

	DEBUG_TRACE;

	if (pstat == NULL)
		return;

	if (++pstat->wpa_sta_info->resendCnt > MAX_RESEND_NUM) {

// When the case of group rekey timeout, update GTK to driver when it is
// the last one node
		if (OPMODE & WIFI_AP_STATE) {
			if (pstat->wpa_sta_info->state == PSK_STATE_PTKINITDONE &&
					pstat->wpa_sta_info->gstate == PSK_GSTATE_REKEYNEGOTIATING) {
				if (priv->wpa_global_info->GKeyDoneStations > 0)
					priv->wpa_global_info->GKeyDoneStations--;

				if (priv->wpa_global_info->GKeyDoneStations==0 && !priv->wpa_global_info->GkeyReady) {
   		        	ToDrv_SetGTK(priv);
					priv->wpa_global_info->GkeyReady = TRUE;
					priv->wpa_global_info->GResetCounter = TRUE;

					// start groupkey rekey timer
					if (priv->pmib->dot1180211AuthEntry.dot11GKRekeyTime)
						mod_timer(&priv->wpa_global_info->GKRekeyTimer, jiffies + priv->pmib->dot1180211AuthEntry.dot11GKRekeyTime*100);
				}
			}
		}
//--------------------------------------------------------- david+2006-04-06

		ToDrv_DisconnectSTA(priv, pstat, RSN_4_way_handshake_timeout);
// need not reset because ToDrv_DisconnectSTA() will take care of it, david+2006-04-06
//		reset_sta_info(priv, pstat);

#ifdef DEBUG_PSK
		printk("PSK: Exceed max retry, disconnect sta\n");
#endif
	}
	else
	{
		if (OPMODE & WIFI_AP_STATE)
			SendEAPOL(priv, pstat, 1);
#ifdef CLIENT_MODE
		else if (OPMODE & WIFI_STATION_STATE)
			ClientSendEAPOL(priv, pstat, 1);
#endif
	}
}

static void GKRekeyTimeout(unsigned long task_priv)
{
	struct rtl8190_priv *priv = (struct rtl8190_priv *)task_priv;
	unsigned long flags;

	DEBUG_TRACE;

	SAVE_INT_AND_CLI(flags);

	priv->wpa_global_info->GTKRekey = TRUE;
	UpdateGK(priv);

	RESTORE_INT(flags);
}

static void SendEAPOL(struct rtl8190_priv *priv, struct stat_info *pstat, int resend)
{
	OCTET_STRING	IV, RSC, KeyID, MIC, KeyData, EAPOLMsgSend, EapolKeyMsgSend;
	unsigned char	IV_buff[KEY_IV_LEN], RSC_buff[KEY_RSC_LEN];
	unsigned char	ID_buff[KEY_ID_LEN], MIC_buff[KEY_MIC_LEN], KEY_buff[INFO_ELEMENT_SIZE];
	unsigned short	tmpKeyData_Length;
	unsigned char	KeyDescriptorVer=key_desc_ver1;
	int 			IfCalcMIC = 0;
	struct wlan_ethhdr_t *eth_hdr;
	struct lib1x_eapol *eapol;
	struct sk_buff	*pskb;
	lib1x_eapol_key	*eapol_key;
	WPA_STA_INFO	*pStaInfo;
	WPA_GLOBAL_INFO *pGblInfo;

	DEBUG_TRACE;

	if (priv == NULL || pstat == NULL)
		return;

	pStaInfo = pstat->wpa_sta_info;
	pGblInfo = priv->wpa_global_info;

	if (pStaInfo->state == PSK_STATE_IDLE)
		return;

	memset(&EapolKeyMsgSend, 0, sizeof(EapolKeyMsgSend));
	EAPOLMsgSend.Octet = pStaInfo->EAPOLMsgSend.Octet;
	EapolKeyMsgSend.Octet = EAPOLMsgSend.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;
	pStaInfo->EapolKeyMsgSend.Octet = EapolKeyMsgSend.Octet;
	eapol_key = (lib1x_eapol_key *)EapolKeyMsgSend.Octet;

	if ((pGblInfo->MulticastCipher == _CCMP_PRIVACY_) || (pStaInfo->UnicastCipher == _CCMP_PRIVACY_))
		KeyDescriptorVer = key_desc_ver2;

	if (resend) {
		EAPOLMsgSend.Length = pStaInfo->EAPOLMsgSend.Length;
		EapolKeyMsgSend.Length = pStaInfo->EapolKeyMsgSend.Length;
		Message_setReplayCounter(EapolKeyMsgSend, pStaInfo->CurrentReplayCounter.field.HighPart, pStaInfo->CurrentReplayCounter.field.LowPart);
		INCLargeInteger(&pStaInfo->CurrentReplayCounter);
		IfCalcMIC = TRUE;
		goto send_packet;
	}

	IV.Octet = IV_buff;
	IV.Length = KEY_IV_LEN;
	RSC.Octet = RSC_buff;
	RSC.Length = KEY_RSC_LEN;
	KeyID.Octet = ID_buff;
	KeyID.Length = KEY_ID_LEN;
	MIC.Octet = MIC_buff;
	MIC.Length = KEY_MIC_LEN;
	KeyData.Octet = KEY_buff;
	KeyData.Length = 0;

	switch(pStaInfo->state) {
		case PSK_STATE_PTKSTART:
			//send 1st message of 4-way handshake
			DEBUG_INFO("4-1\n");
			memset(EapolKeyMsgSend.Octet, 0, MAX_EAPOLKEYMSG_LEN);

#ifdef RTL_WPA2
			if (pStaInfo->RSNEnabled & PSK_WPA2)
				Message_setDescType(EapolKeyMsgSend, desc_type_WPA2);
			else
#endif
				Message_setDescType(EapolKeyMsgSend, desc_type_RSN);

			Message_setKeyDescVer(EapolKeyMsgSend, KeyDescriptorVer);
			Message_setKeyType(EapolKeyMsgSend, type_Pairwise);
			Message_setKeyIndex(EapolKeyMsgSend, 0);
			Message_setInstall(EapolKeyMsgSend, 0);
			Message_setKeyAck(EapolKeyMsgSend, 1);
			Message_setKeyMIC(EapolKeyMsgSend, 0);
			Message_setSecure(EapolKeyMsgSend, 0);
			Message_setError(EapolKeyMsgSend, 0);
			Message_setRequest(EapolKeyMsgSend, 0);
			Message_setReserved(EapolKeyMsgSend, 0);
			Message_setKeyLength(EapolKeyMsgSend, (pStaInfo->UnicastCipher  == DOT11_ENC_TKIP) ? 32:16);
//			Message_setKeyLength(EapolKeyMsgSend, 32);

			// make 4-1's ReplyCounter increased
			Message_setReplayCounter(EapolKeyMsgSend, pStaInfo->CurrentReplayCounter.field.HighPart, pStaInfo->CurrentReplayCounter.field.LowPart);
			memcpy(&pStaInfo->ReplayCounterStarted, &pStaInfo->CurrentReplayCounter, sizeof(LARGE_INTEGER)); // save started reply counter, david+1-12-2007

			INCLargeInteger(&pStaInfo->CurrentReplayCounter);

			INCOctet32_INTEGER(&pGblInfo->Counter);
//#ifndef RTL_WPA2_PREAUTH
			// ANonce is only updated in lib1x_init_authenticator()
			// or after 4-way handshake
			// To avoid different ANonce values among multiple issued 4-1 messages because of multiple association requests
			// Different ANonce values among multiple 4-1 messages induce 4-2 MIC failure.
			SetNonce(pStaInfo->ANonce, pGblInfo->Counter);
//#endif
			Message_setKeyNonce(EapolKeyMsgSend, pStaInfo->ANonce);

			memset(IV.Octet, 0, IV.Length);
			Message_setKeyIV(EapolKeyMsgSend, IV);
			memset(RSC.Octet, 0, RSC.Length);
			Message_setKeyRSC(EapolKeyMsgSend, RSC);
			memset(KeyID.Octet, 0, KeyID.Length);
			Message_setKeyID(EapolKeyMsgSend, KeyID);

// enable it to interoper with Intel 11n Centrino, david+2007-11-19
#if 1
			// otherwise PMK cache
			if ((pStaInfo->RSNEnabled & PSK_WPA2) && (pStaInfo->AuthKeyMethod == DOT11_AuthKeyType_PSK || pStaInfo->PMKCached) ) {
				static char PMKID_KDE_TYPE[] = { 0xDD, 0x14, 0x00, 0x0F, 0xAC, 0x04 };
				Message_setKeyDataLength(EapolKeyMsgSend, 22);
				memcpy(EapolKeyMsgSend.Octet + KeyDataPos,
							PMKID_KDE_TYPE, sizeof(PMKID_KDE_TYPE));
//				memcpy(EapolKeyMsgSend.Octet+KeyDataPos+sizeof(PMKID_KDE_TYPE),
//					global->akm_sm->PMKID, PMKID_LEN);
			}
			else
#endif
				Message_setKeyDataLength(EapolKeyMsgSend, 0);

			memset(MIC.Octet, 0, MIC.Length);
			Message_setMIC(EapolKeyMsgSend, MIC);

#ifdef RTL_WPA2
			if (pStaInfo->RSNEnabled & PSK_WPA2)
				EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN + Message_KeyDataLength(EapolKeyMsgSend);
			else
#endif
				EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN;
			EAPOLMsgSend.Length = ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN + EapolKeyMsgSend.Length;
			break;

		case PSK_STATE_PTKINITNEGOTIATING:

			//Construct Message3
			DEBUG_INFO("4-3\n");

			memset(EapolKeyMsgSend.Octet, 0, MAX_EAPOLKEYMSG_LEN);

#ifdef RTL_WPA2
			if (pStaInfo->RSNEnabled & PSK_WPA2)
				Message_setDescType(EapolKeyMsgSend, desc_type_WPA2);
			else
#endif
				Message_setDescType(EapolKeyMsgSend, desc_type_RSN);

			Message_setKeyDescVer(EapolKeyMsgSend, Message_KeyDescVer(pStaInfo->EapolKeyMsgRecvd));
			Message_setKeyType(EapolKeyMsgSend, Message_KeyType(pStaInfo->EapolKeyMsgRecvd));
			Message_setKeyIndex(EapolKeyMsgSend, Message_KeyIndex(pStaInfo->EapolKeyMsgRecvd));

			//Message_setInstall(global->EapolKeyMsgSend, global->RSNVariable.isSuppSupportUnicastCipher ? 1:0);
			Message_setInstall(EapolKeyMsgSend, 1);
			Message_setKeyAck(EapolKeyMsgSend, 1);
			Message_setKeyMIC(EapolKeyMsgSend, 1);
		//??
		//	Message_setSecure(pStaInfo->EapolKeyMsgSend, pStaInfo->RSNVariable.isSuppSupportMulticastCipher ? 0:1);
			Message_setSecure(EapolKeyMsgSend, 0);
		//??
			Message_setError(EapolKeyMsgSend, 0);
			Message_setRequest(EapolKeyMsgSend, 0);
			Message_setReserved(EapolKeyMsgSend, 0);
			Message_setKeyLength(EapolKeyMsgSend, (pStaInfo->UnicastCipher  == DOT11_ENC_TKIP) ? 32:16);
			Message_setReplayCounter(EapolKeyMsgSend, pStaInfo->CurrentReplayCounter.field.HighPart, pStaInfo->CurrentReplayCounter.field.LowPart);
			Message_setKeyNonce(EapolKeyMsgSend, pStaInfo->ANonce);
			memset(IV.Octet, 0, IV.Length);
			Message_setKeyIV(EapolKeyMsgSend, IV);

#ifdef RTL_WPA2
			if (pStaInfo->RSNEnabled & PSK_WPA2) {
				unsigned char key_data[128];
				unsigned char *key_data_pos = key_data;
				int i;
				unsigned char GTK_KDE_TYPE[] = {0xDD, 0x16, 0x00, 0x0F, 0xAC, 0x01, 0x01, 0x00 };

				EapolKeyMsgSend.Octet[1] = 0x13;
//???
				if (KeyDescriptorVer == key_desc_ver2 ) {
					INCOctet32_INTEGER(&pGblInfo->Counter);
					SetEAPOL_KEYIV(IV, pGblInfo->Counter);
					//memset(IV.Octet, 0x0, IV.Length);
					Message_setKeyIV(EapolKeyMsgSend, IV);
				}
				// RSN IE
				if (pGblInfo->AuthInfoElement.Octet[0] == WPA2_ELEMENT_ID) {
					int len = (unsigned char)pGblInfo->AuthInfoElement.Octet[1] + 2;
					if (len > 100) {
						DEBUG_ERR("invalid IE length!\n");
						return;
					}
					memcpy(key_data_pos, pGblInfo->AuthInfoElement.Octet, len);
					key_data_pos += len;
				} else {
					//find WPA2_ELEMENT_ID 0x30
					int len = (unsigned char)pGblInfo->AuthInfoElement.Octet[1] + 2;
					//printf("%s: global->auth->RSNVariable.AuthInfoElement.Octet[%d] = %02X\n", (char *)__FUNCTION__, len, global->auth->RSNVariable.AuthInfoElement.Octet[len]);
					if (pGblInfo->AuthInfoElement.Octet[len] == WPA2_ELEMENT_ID) {
						int len2 = (unsigned char)pGblInfo->AuthInfoElement.Octet[len+1] + 2;
						memcpy(key_data_pos, pGblInfo->AuthInfoElement.Octet+len, len2);
						key_data_pos += len2;
					} else {
						DEBUG_ERR("%d ERROR!\n", __LINE__);
					}
				}

				memcpy(key_data_pos, GTK_KDE_TYPE, sizeof(GTK_KDE_TYPE));
				key_data_pos[1] = (unsigned char) 6 + ((pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16);
				key_data_pos += sizeof(GTK_KDE_TYPE);


				// FIX GROUPKEY ALL ZERO
// david+2006-04-04, fix the issue of re-generating group key
//				pGblInfo->GInitAKeys = TRUE;
				UpdateGK(priv);
				memcpy(key_data_pos, pGblInfo->GTK[pGblInfo->GN], (pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16);

				key_data_pos += (pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16;
				i = (key_data_pos - key_data) % 8;
				if ( i != 0 ) {
					*key_data_pos = 0xdd;
					key_data_pos++;
					for (i=i+1; i<8; i++) {
						*key_data_pos = 0x0;
						key_data_pos++;
					}

				}
				EncGTK(priv, pstat,
					pStaInfo->PTK + PTK_LEN_EAPOLMIC, PTK_LEN_EAPOLENC,
					key_data,
					(key_data_pos - key_data),
					 KeyData.Octet, &tmpKeyData_Length);
				KeyData.Length = (int)tmpKeyData_Length;
				Message_setKeyData(EapolKeyMsgSend, KeyData);
				Message_setKeyDataLength(EapolKeyMsgSend, KeyData.Length);

				EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN +	KeyData.Length;
				memcpy((void *)RSC.Octet, (void *)&(priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48.val48), 6);
				RSC.Octet[6] = 0;
				RSC.Octet[7] = 0;
				Message_setKeyRSC(EapolKeyMsgSend, RSC);
			}
			else
#endif // RTL_WPA2
			{ // WPA
				memset(RSC.Octet, 0, RSC.Length);
				Message_setKeyRSC(EapolKeyMsgSend, RSC);
				memset(KeyID.Octet, 0, KeyID.Length);
				Message_setKeyID(EapolKeyMsgSend, KeyID);
				//lib1x_hexdump2(MESS_DBG_KEY_MANAGE, "lib1x_akmsm_ProcessEAPOL_proc", global->auth->RSNVariable.AuthInfoElement.Octet, global->auth->RSNVariable.AuthInfoElement.Length,"Append Authenticator Information Element");

				{ //WPA 0xDD
					//printf("%s: global->auth->RSNVariable.AuthInfoElement.Octet[0] = %02X\n", (char *)__FUNCTION__, global->auth->RSNVariable.AuthInfoElement.Octet[0]);

					int len = (unsigned char)pGblInfo->AuthInfoElement.Octet[1] + 2;

					if (pGblInfo->AuthInfoElement.Octet[0] == RSN_ELEMENT_ID) {
						memcpy(KeyData.Octet, pGblInfo->AuthInfoElement.Octet, len);
						KeyData.Length = len;
					} else {
						// impossible case??
						int len2 = (unsigned char)pGblInfo->AuthInfoElement.Octet[len+1] + 2;
						memcpy(KeyData.Octet, pGblInfo->AuthInfoElement.Octet+len, len2);
						KeyData.Length = len2;
					}
				}
				Message_setKeyDataLength(EapolKeyMsgSend, KeyData.Length);
				Message_setKeyData(EapolKeyMsgSend, KeyData);
				EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN + KeyData.Length;
			}

			INCLargeInteger(&pStaInfo->CurrentReplayCounter);

#if 0
			memset(RSC.Octet, 0, RSC.Length);
			Message_setKeyRSC(EapolKeyMsgSend, RSC);
			memset(KeyID.Octet, 0, KeyID.Length);
			Message_setKeyID(EapolKeyMsgSend, KeyID);
			//lib1x_hexdump2(MESS_DBG_KEY_MANAGE, "lib1x_akmsm_ProcessEAPOL_proc", global->auth->RSNVariable.AuthInfoElement.Octet, global->auth->RSNVariable.AuthInfoElement.Length,"Append Authenticator Information Element");
			Message_setKeyDataLength(EapolKeyMsgSend, pGblInfo->AuthInfoElement.Length);
			Message_setKeyData(EapolKeyMsgSend, pGblInfo->AuthInfoElement);
			//Message_setKeyDataLength(global->EapolKeyMsgSend, global->akm_sm->AuthInfoElement.Length);
			//Message_setKeyData(global->EapolKeyMsgSend, global->akm_sm->AuthInfoElement);
			EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN + pGblInfo->AuthInfoElement.Length;
#endif /* RTL_WPA2 */

			EAPOLMsgSend.Length = ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN + EapolKeyMsgSend.Length;
			IfCalcMIC = TRUE;
			break;

		case PSK_STATE_PTKINITDONE:
			//send 1st message of 2-way handshake
			DEBUG_INFO("2-1\n");
			memset(EapolKeyMsgSend.Octet, 0, MAX_EAPOLKEYMSG_LEN);

#ifdef RTL_WPA2
			if (pStaInfo->RSNEnabled & PSK_WPA2)
				Message_setDescType(EapolKeyMsgSend, desc_type_WPA2);
			else
#endif
				Message_setDescType(EapolKeyMsgSend, desc_type_RSN);

			Message_setKeyDescVer(EapolKeyMsgSend, KeyDescriptorVer);
			Message_setKeyType(EapolKeyMsgSend, type_Group);
			Message_setKeyIndex(EapolKeyMsgSend, 1);
			Message_setInstall(EapolKeyMsgSend, 1);
			Message_setKeyAck(EapolKeyMsgSend, 1);
			Message_setKeyMIC(EapolKeyMsgSend, 1);
			Message_setSecure(EapolKeyMsgSend, 1);
			Message_setError(EapolKeyMsgSend, 0);
			Message_setRequest(EapolKeyMsgSend, 0);
			Message_setReserved(EapolKeyMsgSend, 0);

			EapolKeyMsgSend.Octet[1] = 0x03;
			if (KeyDescriptorVer == key_desc_ver1 )
				EapolKeyMsgSend.Octet[2] = 0x91;
			else
				EapolKeyMsgSend.Octet[2] = 0x92;

			Message_setKeyLength(EapolKeyMsgSend, (pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16);
			Message_setReplayCounter(EapolKeyMsgSend, pStaInfo->CurrentReplayCounter.field.HighPart, pStaInfo->CurrentReplayCounter.field.LowPart);
			INCLargeInteger(&pStaInfo->CurrentReplayCounter);
			// kenny: n+2
			INCLargeInteger(&pStaInfo->CurrentReplayCounter);
			SetNonce(pGblInfo->GNonce, pGblInfo->Counter);
			Message_setKeyNonce(EapolKeyMsgSend, pGblInfo->GNonce);
			memset(IV.Octet, 0, IV.Length);
			Message_setKeyIV(EapolKeyMsgSend, IV);

			memcpy((void *)RSC.Octet, (void *)&(priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48.val48), 6);
			RSC.Octet[6] = 0;
			RSC.Octet[7] = 0;
			Message_setKeyRSC(EapolKeyMsgSend, RSC);

			memset(KeyID.Octet, 0, KeyID.Length);
			Message_setKeyID(EapolKeyMsgSend, KeyID);

#ifdef RTL_WPA2
			if (pStaInfo->RSNEnabled & PSK_WPA2) {
				char key_data[128];
				char *key_data_pos = key_data;
				static char GTK_KDE_TYPE[] = { 0xDD, 0x16, 0x00, 0x0F, 0xAC, 0x01, 0x01, 0x00 };
				memcpy(key_data_pos, GTK_KDE_TYPE, sizeof(GTK_KDE_TYPE));

//fix the bug of using default KDE length -----------
				key_data_pos[1] = (unsigned char)(6+((pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16));
//------------------------------ david+2006-04-04

				key_data_pos += sizeof(GTK_KDE_TYPE);

				EapolKeyMsgSend.Octet[1] = 0x13;
// fill key-data length after encrypt --------------------
#if 0
				if (KeyDescriptorVer == key_desc_ver1) {
// david+2006-01-06, fix the bug of using 0 as group key id
//					EapolKeyMsgSend.Octet[2] = 0x81;
					Message_setKeyDescVer(EapolKeyMsgSend, key_desc_ver1);
					Message_setKeyDataLength(EapolKeyMsgSend,
						(sizeof(GTK_KDE_TYPE) + ((pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16)));
				}
				else if (KeyDescriptorVer == key_desc_ver2) {
// david+2006-01-06, fix the bug of using 0 as group key id
//					EapolKeyMsgSend.Octet[2] = 0x82;
					Message_setKeyDescVer(EapolKeyMsgSend, key_desc_ver2);
					Message_setKeyDataLength(EapolKeyMsgSend,
					    	(sizeof(GTK_KDE_TYPE) + (8 + ((pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16))));
				}
#endif
//------------------------------------- david+2006-04-04

				memcpy(key_data_pos, pGblInfo->GTK[pGblInfo->GN], (pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16);
				EncGTK(priv, pstat, pStaInfo->PTK + PTK_LEN_EAPOLMIC, PTK_LEN_EAPOLENC,
					key_data,
					sizeof(GTK_KDE_TYPE) + ((pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16),
					 KeyData.Octet, &tmpKeyData_Length);
			}
			else
#endif // RTL_WPA2
			{
// fill key-data length after encrypt ---------------------
#if 0
				if (KeyDescriptorVer == key_desc_ver1)
					Message_setKeyDataLength(EapolKeyMsgSend,
						((pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16));
				else
					Message_setKeyDataLength(EapolKeyMsgSend,
				    	(8 + ((pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16) ));
#endif
//------------------------------------- david+2006-04-04
				EncGTK(priv, pstat, pStaInfo->PTK + PTK_LEN_EAPOLMIC, PTK_LEN_EAPOLENC,
					pGblInfo->GTK[pGblInfo->GN],
					(pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16,
					KeyData.Octet, &tmpKeyData_Length);
			}

			KeyData.Length = (int)tmpKeyData_Length;
			Message_setKeyData(EapolKeyMsgSend, KeyData);

//set keyData length after encrypt ------------------
			Message_setKeyDataLength(EapolKeyMsgSend, KeyData.Length);
//------------------------------- david+2006-04-04

/* Kenny
			global->EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN +
					((global->RSNVariable.MulticastCipher == DOT11_ENC_TKIP) ? 32:16);
*/
			EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN +	KeyData.Length;
			EAPOLMsgSend.Length = ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN + EapolKeyMsgSend.Length;
			IfCalcMIC = TRUE;
			break;
		default:
			DEBUG_ERR("Invalid wpa state [%x]\n", pStaInfo->state);
			return;
	}//switch

	pStaInfo->EAPOLMsgSend.Length = EAPOLMsgSend.Length;
	pStaInfo->EapolKeyMsgSend.Length = EapolKeyMsgSend.Length;

send_packet:
	eth_hdr = (struct wlan_ethhdr_t *)pStaInfo->EAPOLMsgSend.Octet;
	memcpy(eth_hdr->daddr, pstat->hwaddr, 6);
	memcpy(eth_hdr->saddr, GET_MY_HWADDR, 6);
	eth_hdr->type = htons(LIB1X_ETHER_EAPOL_TYPE);

	eapol = (struct lib1x_eapol *)(EAPOLMsgSend.Octet + ETHER_HDRLEN);
	eapol->protocol_version = LIB1X_EAPOL_VER;
    eapol->packet_type =  LIB1X_EAPOL_KEY;
	eapol->packet_body_length = htons(EapolKeyMsgSend.Length);

	if (IfCalcMIC)
		CalcMIC(EAPOLMsgSend, KeyDescriptorVer, pStaInfo->PTK, PTK_LEN_EAPOLMIC);

	pskb = rtl_dev_alloc_skb(priv, MAX_EAPOLMSG_LEN, _SKB_TX_, 1);
	if (pskb == NULL) {
		DEBUG_ERR("Allocate EAP skb failed!\n");
		return;
	}
	memcpy(pskb->data, (char *)eth_hdr, EAPOLMsgSend.Length);
	skb_put(pskb, EAPOLMsgSend.Length);

#ifdef DEBUG_PSK
	{	unsigned char *msg;
		if (pStaInfo->state == PSK_STATE_PTKSTART)
			msg = "4-1";
		else if (pStaInfo->state == PSK_STATE_PTKINITNEGOTIATING)
			msg = "4-3";
		else
			msg = "2-1";

		printk("PSK: Send a EAPOL %s, len=%x\n", msg, pskb->len);
		debug_out(NULL, pskb->data, pskb->len);
	}
#endif
#ifdef ENABLE_RTL_SKB_STATS
	rtl_atomic_dec(&priv->rtl_tx_skb_cnt);
#endif

#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
	pskb->fcpu=1;
#endif
	if (rtl8190_start_xmit(pskb, priv->dev))
		rtl_kfree_skb(priv, pskb, _SKB_TX_);

	mod_timer(&pstat->wpa_sta_info->resendTimer, jiffies + RESEND_TIME);
}

#ifdef CLIENT_MODE
static void ClientSendEAPOL(struct rtl8190_priv *priv, struct stat_info *pstat, int resend)
{
	OCTET_STRING	IV, RSC, KeyID, MIC, KeyData, EAPOLMsgSend, EapolKeyMsgSend, Nonce;
	unsigned char	IV_buff[KEY_IV_LEN], RSC_buff[KEY_RSC_LEN];
	unsigned char	ID_buff[KEY_ID_LEN], MIC_buff[KEY_MIC_LEN], KEY_buff[INFO_ELEMENT_SIZE], Nonce_buff[KEY_NONCE_LEN];
	struct wlan_ethhdr_t *eth_hdr;
	struct lib1x_eapol *eapol;
	struct sk_buff	*pskb;
	lib1x_eapol_key	*eapol_key;
	WPA_STA_INFO	*pStaInfo;
	WPA_GLOBAL_INFO *pGblInfo;

	DEBUG_TRACE;

	if (priv == NULL || pstat == NULL)
		return;

	pStaInfo = pstat->wpa_sta_info;
	pGblInfo = priv->wpa_global_info;

	EAPOLMsgSend.Octet = pStaInfo->EAPOLMsgSend.Octet;
	EapolKeyMsgSend.Octet = EAPOLMsgSend.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;
	pStaInfo->EapolKeyMsgSend.Octet = EapolKeyMsgSend.Octet;
	eapol_key = (lib1x_eapol_key *)EapolKeyMsgSend.Octet;

	if (resend)
	{
		EAPOLMsgSend.Length = pStaInfo->EAPOLMsgSend.Length;
		EapolKeyMsgSend.Length = pStaInfo->EapolKeyMsgSend.Length;
		//---goto send_packet
	}
	else
	{
		IV.Octet = IV_buff;
		IV.Length = KEY_IV_LEN;
		RSC.Octet = RSC_buff;
		RSC.Length = KEY_RSC_LEN;
		KeyID.Octet = ID_buff;
		KeyID.Length = KEY_ID_LEN;
		MIC.Octet = MIC_buff;
		MIC.Length = KEY_MIC_LEN;
		KeyData.Octet = KEY_buff;
		KeyData.Length = 0;

		Nonce.Octet = Nonce_buff;
		Nonce.Length = KEY_NONCE_LEN;

		if (!pStaInfo->clientHndshkDone)
		{
			if (!pStaInfo->clientHndshkProcessing)
			{
				//send 2nd message of 4-way handshake
				DEBUG_INFO("client mode 4-2\n");

				pStaInfo->clientHndshkProcessing = TRUE;

				memset(EapolKeyMsgSend.Octet, 0, MAX_EAPOLKEYMSG_LEN);

				Message_setDescType(EapolKeyMsgSend, Message_DescType(pStaInfo->EapolKeyMsgRecvd));
				Message_setKeyDescVer(EapolKeyMsgSend, Message_KeyDescVer(pStaInfo->EapolKeyMsgRecvd));
				Message_setKeyType(EapolKeyMsgSend, Message_KeyType(pStaInfo->EapolKeyMsgRecvd));
				Message_setKeyIndex(EapolKeyMsgSend, 0);
				Message_setInstall(EapolKeyMsgSend, Message_KeyIndex(pStaInfo->EapolKeyMsgRecvd));
				Message_setKeyAck(EapolKeyMsgSend, 0);
				Message_setKeyMIC(EapolKeyMsgSend, 1);
				Message_setSecure(EapolKeyMsgSend, Message_Secure(pStaInfo->EapolKeyMsgRecvd));
				Message_setError(EapolKeyMsgSend, Message_Error(pStaInfo->EapolKeyMsgRecvd));
				Message_setRequest(EapolKeyMsgSend, Message_Request(pStaInfo->EapolKeyMsgRecvd));
				Message_setReserved(EapolKeyMsgSend, 0);

				Message_setKeyLength(EapolKeyMsgSend, Message_KeyLength(pStaInfo->EapolKeyMsgRecvd));
				Message_CopyReplayCounter(EapolKeyMsgSend, pStaInfo->EapolKeyMsgRecvd);

				Message_setKeyNonce(EapolKeyMsgSend, pStaInfo->SNonce);
				memset(IV.Octet, 0, IV.Length);
				Message_setKeyIV(EapolKeyMsgSend, IV);
				memset(RSC.Octet, 0, RSC.Length);
				Message_setKeyRSC(EapolKeyMsgSend, RSC);
				memset(KeyID.Octet, 0, KeyID.Length);
				Message_setKeyID(EapolKeyMsgSend, KeyID);

				Message_setKeyDataLength(EapolKeyMsgSend, pGblInfo->AuthInfoElement.Length);
				Message_setKeyData(EapolKeyMsgSend, pGblInfo->AuthInfoElement);

				EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN + pGblInfo->AuthInfoElement.Length;
				EAPOLMsgSend.Length = ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN + EapolKeyMsgSend.Length;

			}
			else
			{
				//Construct Message4
				DEBUG_INFO("client mode 4-4\n");

				pStaInfo->clientHndshkDone = TRUE;

				memset(EapolKeyMsgSend.Octet, 0, MAX_EAPOLKEYMSG_LEN);

				Message_setDescType(EapolKeyMsgSend, Message_DescType(pStaInfo->EapolKeyMsgRecvd));
				Message_setKeyDescVer(EapolKeyMsgSend, Message_KeyDescVer(pStaInfo->EapolKeyMsgRecvd));
				Message_setKeyType(EapolKeyMsgSend, Message_KeyType(pStaInfo->EapolKeyMsgRecvd));
				Message_setKeyIndex(EapolKeyMsgSend, 0);
				Message_setInstall(EapolKeyMsgSend, 0);
				Message_setKeyAck(EapolKeyMsgSend, 0);
				Message_setKeyMIC(EapolKeyMsgSend, 1);
				Message_setSecure(EapolKeyMsgSend, Message_Secure(pStaInfo->EapolKeyMsgRecvd));
				Message_setError(EapolKeyMsgSend, 0);
				Message_setRequest(EapolKeyMsgSend, 0);
				Message_setReserved(EapolKeyMsgSend, 0);
				Message_setKeyLength(EapolKeyMsgSend, Message_KeyLength(pStaInfo->EapolKeyMsgRecvd));
				Message_CopyReplayCounter(EapolKeyMsgSend, pStaInfo->EapolKeyMsgRecvd);
				memset(IV.Octet, 0, IV.Length);
				Message_setKeyIV(EapolKeyMsgSend, IV);
				memset(RSC.Octet, 0, RSC.Length);
				Message_setKeyRSC(EapolKeyMsgSend, RSC);
				memset(KeyID.Octet, 0, KeyID.Length);
				Message_setKeyID(EapolKeyMsgSend, KeyID);
				Message_setKeyDataLength(EapolKeyMsgSend, 0);

				EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN;
				EAPOLMsgSend.Length = ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN + EapolKeyMsgSend.Length;

				LOG_MSG( "Open and authenticated\n");

//				printk("client mode 4-Way Message 4-4 done\n");
			}
		}
		else
		{
			//send 2nd message of 2-way handshake
			DEBUG_INFO("client mode 2-2\n");
			memset(EapolKeyMsgSend.Octet, 0, MAX_EAPOLKEYMSG_LEN);

			Message_setDescType(EapolKeyMsgSend, Message_DescType(pStaInfo->EapolKeyMsgRecvd));
			Message_setKeyDescVer(EapolKeyMsgSend, Message_KeyDescVer(pStaInfo->EapolKeyMsgRecvd));
			Message_setKeyType(EapolKeyMsgSend, Message_KeyType(pStaInfo->EapolKeyMsgRecvd));
			Message_setKeyIndex(EapolKeyMsgSend, pGblInfo->GN);
			Message_setInstall(EapolKeyMsgSend, 0);
			Message_setKeyAck(EapolKeyMsgSend, 0);
			Message_setKeyMIC(EapolKeyMsgSend, 1);
			Message_setSecure(EapolKeyMsgSend, 1);
			Message_setError(EapolKeyMsgSend, 0);
			Message_setRequest(EapolKeyMsgSend, 0);
			Message_setReserved(EapolKeyMsgSend, 0);

			Message_setKeyLength(EapolKeyMsgSend, Message_KeyLength(pStaInfo->EapolKeyMsgRecvd));
			Message_CopyReplayCounter(EapolKeyMsgSend, pStaInfo->EapolKeyMsgRecvd);
			memset(Nonce.Octet, 0, KEY_NONCE_LEN);
			Message_setKeyNonce(EapolKeyMsgSend, Nonce);
			memset(IV.Octet, 0, IV.Length);
			Message_setKeyIV(EapolKeyMsgSend, IV);
			memset(RSC.Octet, 0, RSC.Length);
			Message_setKeyRSC(EapolKeyMsgSend, RSC);
			memset(KeyID.Octet, 0, KeyID.Length);
			Message_setKeyID(EapolKeyMsgSend, KeyID);
			Message_setKeyDataLength(EapolKeyMsgSend, 0);

			EapolKeyMsgSend.Length = EAPOLMSG_HDRLEN;
			EAPOLMsgSend.Length = ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN + EapolKeyMsgSend.Length;

//			printk("client mode Group Message 2-2 done\n");
		}
	}

	//send_packet--------------------------------------------------------------

	eth_hdr = (struct wlan_ethhdr_t *)EAPOLMsgSend.Octet;
	memcpy(eth_hdr->daddr, pstat->hwaddr, 6);
	memcpy(eth_hdr->saddr, GET_MY_HWADDR, 6);
	eth_hdr->type = htons(LIB1X_ETHER_EAPOL_TYPE);

	eapol = (struct lib1x_eapol *)(EAPOLMsgSend.Octet + ETHER_HDRLEN);
	eapol->protocol_version = LIB1X_EAPOL_VER;
    	eapol->packet_type =  LIB1X_EAPOL_KEY;
	eapol->packet_body_length = htons(EapolKeyMsgSend.Length);

	if (!resend)
		CalcMIC(EAPOLMsgSend, Message_KeyDescVer(pStaInfo->EapolKeyMsgRecvd), pStaInfo->PTK, PTK_LEN_EAPOLMIC);

	pskb = rtl_dev_alloc_skb(priv, MAX_EAPOLMSG_LEN, _SKB_TX_, 1);

	if (pskb == NULL) {
		DEBUG_ERR("Allocate EAP skb failed!\n");
		printk("Allocate EAP skb failed!\n");
		return;
	}

	memcpy(pskb->data, (char *)eth_hdr, EAPOLMsgSend.Length);
	skb_put(pskb, EAPOLMsgSend.Length);

#ifdef ENABLE_RTL_SKB_STATS
	rtl_atomic_dec(&priv->rtl_tx_skb_cnt);
#endif

#if defined(PKT_PROCESSOR) || defined(PRE_ALLOCATE_SKB)
	pskb->fcpu=1;
#endif
	if (rtl8190_start_xmit(pskb, priv->dev))
		rtl_kfree_skb(priv, pskb, _SKB_TX_);

	if (!pStaInfo->clientHndshkDone) // only 4-2 need to check the time
		mod_timer(&pStaInfo->resendTimer, jiffies + RESEND_TIME);

	//--------------------------------------------------------------send_packet
}
#endif // CLIENT_MODE

static void AuthenticationRequest(struct rtl8190_priv *priv, struct stat_info *pstat)
{
	static unsigned int RC_toggle = 0;

	DEBUG_TRACE;

	// For some STA that can only process if Replay Counter is not 0
	if ((RC_toggle++)%2)
		INCLargeInteger(&pstat->wpa_sta_info->CurrentReplayCounter);

	INCOctet32_INTEGER(&priv->wpa_global_info->Counter);
	SetNonce(pstat->wpa_sta_info->ANonce, priv->wpa_global_info->Counter);

	memcpy(pstat->wpa_sta_info->PMK, priv->wpa_global_info->PSK, PMK_LEN);

//#ifdef RTL_WPA2
#if 0
	CalcPMKID(	akm_sm->PMKID,
			akm_sm->PMK, 	 // PMK
			global->theAuthenticator->global->TxRx->oursupp_addr,   // AA
			global->theAuthenticator->supp_addr); 			// SPA
#endif
	pstat->wpa_sta_info->state = PSK_STATE_PTKSTART;

	//send 1st message
	SendEAPOL(priv, pstat, 0);
}

//-------------------------------------------------------------
// Start 2-way handshake after receiving 4th message
//-------------------------------------------------------------
static void UpdateGK(struct rtl8190_priv *priv)
{
	WPA_GLOBAL_INFO *pGblInfo=priv->wpa_global_info;
	struct stat_info *pstat;
	int i;

	//------------------------------------------------------------
	// Execute Global Group key state machine
	//------------------------------------------------------------
	if (pGblInfo->GTKAuthenticator && (pGblInfo->GTKRekey || pGblInfo->GInitAKeys)) {
		pGblInfo->GTKRekey = FALSE;
		pGblInfo->GInitAKeys = FALSE; // david+2006-04-04, fix the issue of re-generating group key

		INCOctet32_INTEGER(&pGblInfo->Counter);

		// kenny:??? GNonce should be a random number ???
		SetNonce(pGblInfo->GNonce , pGblInfo->Counter);
		CalcGTK(GET_MY_HWADDR, pGblInfo->GNonce.Octet,
				pGblInfo->GMK, GMK_LEN, pGblInfo->GTK[pGblInfo->GN], GTK_LEN);

#ifdef DEBUG_PSK
		debug_out("PSK: Generated GTK=", pGblInfo->GTK[pGblInfo->GN], GTK_LEN);
#endif

		pGblInfo->GUpdateStationKeys = TRUE;
		pGblInfo->GkeyReady = FALSE;

		if (timer_pending(&pGblInfo->GKRekeyTimer))
			del_timer_sync(&pGblInfo->GKRekeyTimer);

		//---- In the case of updating GK to all STAs, only the STA that has finished
		//---- 4-way handshake is needed to be sent with 2-way handshake
		//gkm_sm->GKeyDoneStations = auth->NumOfSupplicant;
		pGblInfo->GKeyDoneStations = 0;

		for (i=0; i<NUM_STAT; i++) {
			if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)) {
#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
				if (priv != priv->pshare->aidarray[i]->priv)
					continue;
#endif

				if ((priv->pshare->aidarray[i]->station.state & WIFI_ASOC_STATE) &&
					(priv->pshare->aidarray[i]->station.wpa_sta_info->state == PSK_STATE_PTKINITDONE))
				pGblInfo->GKeyDoneStations++;
		}
	}
	}

	//------------------------------------------------------------
	// Execute Group key state machine of each STA
	//------------------------------------------------------------
	for (i=0; i<NUM_STAT; i++) {
		if (priv->pshare->aidarray[i] == NULL)
			continue;
		pstat = &priv->pshare->aidarray[i]->station;
		if ((priv->pshare->aidarray[i]->used != TRUE) ||!(pstat->state & WIFI_ASOC_STATE))
			continue;

#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
		if (priv != priv->pshare->aidarray[i]->priv)
			continue;
#endif

		//---- Group key handshake to only one supplicant ----
		if (pstat->wpa_sta_info->state == PSK_STATE_PTKINITDONE &&
			(pGblInfo->GkeyReady && pstat->wpa_sta_info->PInitAKeys)) {
			pstat->wpa_sta_info->PInitAKeys = FALSE;
			pstat->wpa_sta_info->gstate = PSK_GSTATE_REKEYNEGOTIATING; // set proper gstat, david+2006-04-06
			SendEAPOL(priv, pstat, 0);
		}
		//---- Updata group key to all supplicant----
		else if (pstat->wpa_sta_info->state == PSK_STATE_PTKINITDONE &&  //Done 4-way handshake
				(pGblInfo->GUpdateStationKeys ||                     //When new key is generated
					pstat->wpa_sta_info->gstate == PSK_GSTATE_REKEYNEGOTIATING))  { //1st message is not yet sent
			pstat->wpa_sta_info->PInitAKeys = FALSE;
			pstat->wpa_sta_info->gstate = PSK_GSTATE_REKEYNEGOTIATING; // set proper gstat, david+2006-04-06
			SendEAPOL(priv, pstat, 0);
		 }
	}
    pGblInfo->GUpdateStationKeys = FALSE;
};

static void EAPOLKeyRecvd(struct rtl8190_priv *priv, struct stat_info *pstat)
{
	WPA_GLOBAL_INFO *pGblInfo = priv->wpa_global_info;
	WPA_STA_INFO *pStaInfo = pstat->wpa_sta_info;
	LARGE_INTEGER recievedRC;
	struct lib1x_eapol *eapol;

	DEBUG_TRACE;

	eapol = ( struct lib1x_eapol * ) ( pstat->wpa_sta_info->EAPOLMsgRecvd.Octet + ETHER_HDRLEN );
	if (eapol->packet_type != LIB1X_EAPOL_KEY) {
#ifdef DEBUG_PSK
		printk("Not Eapol-key pkt (type %d), drop\n", eapol->packet_type);
#endif
		return;
	}

	pStaInfo->EapolKeyMsgRecvd.Octet = pStaInfo->EAPOLMsgRecvd.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;

#ifdef DEBUG_PSK
	printk("PSK: Rx a EAPOL %s, len=%x\n", STATE2RXMSG(pstat), pStaInfo->EAPOLMsgRecvd.Length);
	debug_out(NULL, pstat->wpa_sta_info->EAPOLMsgRecvd.Octet, pStaInfo->EAPOLMsgRecvd.Length);
#endif

	//----IEEE 802.11-03/156r2. MIC report : (1)MIC bit (2)error bit (3) request bit
	//----Check if it is MIC failure report. If it is, indicate to driver
	if (Message_KeyMIC(pStaInfo->EapolKeyMsgRecvd) && Message_Error(pStaInfo->EapolKeyMsgRecvd)
				&& Message_Request(pStaInfo->EapolKeyMsgRecvd)) {
#ifdef DEBUG_PSK
		printk("PSK: Rx MIC errir report from client\n");
#endif
		ToDrv_IndicateMICFail(priv, pstat->hwaddr);
		return;
	}

	if (Message_KeyType(pStaInfo->EapolKeyMsgRecvd) == type_Pairwise) {
		switch(pStaInfo->state) {
		case PSK_STATE_PTKSTART:
			//---- Receive 2nd message and send third
			DEBUG_INFO("4-2\n");

			// delete resend timer
			if (timer_pending(&pStaInfo->resendTimer))
				del_timer_sync(&pStaInfo->resendTimer);

			//check replay counter
			Message_ReplayCounter_OC2LI(pStaInfo->EapolKeyMsgRecvd, &recievedRC);
			INCLargeInteger(&recievedRC);
			if ( !(pStaInfo->CurrentReplayCounter.field.HighPart == recievedRC.field.HighPart
		             && pStaInfo->CurrentReplayCounter.field.LowPart == recievedRC.field.LowPart)) {
				DEBUG_ERR("4-2: ERROR_NONEEQUL_REPLAYCOUNTER\n");
				break;
			}
check_msg2:
			pStaInfo->SNonce = Message_KeyNonce(pStaInfo->EapolKeyMsgRecvd);
			CalcPTK(pStaInfo->EAPOLMsgRecvd.Octet, pStaInfo->EAPOLMsgRecvd.Octet + 6,
					pStaInfo->ANonce.Octet, pStaInfo->SNonce.Octet,
					pStaInfo->PMK, PMK_LEN, pStaInfo->PTK, PTK_LEN_TKIP);

#ifdef DEBUG_PSK
			debug_out("PSK: Generated PTK=", pStaInfo->PTK, PTK_LEN_TKIP);
#endif

			if (!CheckMIC(pStaInfo->EAPOLMsgRecvd, pStaInfo->PTK, PTK_LEN_EAPOLMIC)) {
				if (priv->pmib->dot1180211AuthEntry.dot11PassPhraseGuest[0]) {
					CalcPTK(pStaInfo->EAPOLMsgRecvd.Octet, pStaInfo->EAPOLMsgRecvd.Octet + 6,
						pStaInfo->ANonce.Octet, pStaInfo->SNonce.Octet,
						pGblInfo->PSKGuest, PMK_LEN, pStaInfo->PTK, PTK_LEN_TKIP);
					if (CheckMIC(pStaInfo->EAPOLMsgRecvd, pStaInfo->PTK, PTK_LEN_EAPOLMIC)) {
						pStaInfo->isGuest = 1;
						goto cont_msg;
					}
				}

				DEBUG_ERR("4-2: ERROR_MIC_FAIL\n");
#if 0
				if (global->RSNVariable.PMKCached ) {
					printf("\n%s:%d del_pmksa due to 4-2 ERROR_MIC_FAIL\n", (char *)__FUNCTION__, __LINE__);
					global->RSNVariable.PMKCached = FALSE;
					del_pmksa_by_spa(global->theAuthenticator->supp_addr);
				}
#endif

				LOG_MSG("Authentication failled! (4-2: MIC error)\n");
#if defined(CONFIG_RTL8186_KB_N)
				authRes = 1;//Auth failed
#endif

#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
				LOG_MSG_NOTICE("Authentication failed;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
					pstat->hwaddr[0],
					pstat->hwaddr[1],
					pstat->hwaddr[2],
					pstat->hwaddr[3],
					pstat->hwaddr[4],
					pstat->hwaddr[5]);
#elif defined(CONFIG_RTL8196B_TLD)
				LOG_MSG_DEL("[WLAN access rejected: incorrect security] from MAC address: %02x:%02x:%02x:%02x:%02x:%02x,\n",
					pstat->hwaddr[0],
					pstat->hwaddr[1],
					pstat->hwaddr[2],
					pstat->hwaddr[3],
					pstat->hwaddr[4],
					pstat->hwaddr[5]);
#endif

				ToDrv_DisconnectSTA(priv, pstat, RSN_MIC_failure);
				pStaInfo->state = PSK_STATE_IDLE;
				break;
			}
cont_msg:
			pStaInfo->state = PSK_STATE_PTKINITNEGOTIATING;

			SendEAPOL(priv, pstat, 0);	// send msg 3
			break;

		case PSK_STATE_PTKINITNEGOTIATING:
			//---- Receive 4th message ----
			DEBUG_INFO("4-4\n");

			// delete resend timer
			if (timer_pending(&pStaInfo->resendTimer))
				del_timer_sync(&pStaInfo->resendTimer);

			// test 2nd or 4th message
// check replay counter to determine if msg 2 or 4 received --------------
//			if ( Message_KeyDataLength(pStaInfo->EapolKeyMsgRecvd) != 0) {
			if (Message_EqualReplayCounter(pStaInfo->ReplayCounterStarted, pStaInfo->EapolKeyMsgRecvd)) {
//---------------------------------------------------- david+1-12-2007

				DEBUG_INFO("4-2 in akmsm_PTKINITNEGOTIATING: resend 4-3\n");

#if 0 // Don't check replay counter during dup 4-2
#ifdef RTL_WPA2
				Message_ReplayCounter_OC2LI(global->EapolKeyMsgRecvd, &recievedRC);
				INCLargeInteger(&recievedRC);
				if ( !(global->akm_sm->CurrentReplayCounter.field.HighPart == recievedRC.field.HighPart
			             && global->akm_sm->CurrentReplayCounter.field.LowPart == recievedRC.field.LowPart))
#else
				if(!Message_EqualReplayCounter(global->akm_sm->CurrentReplayCounter, global->EapolKeyMsgRecvd))
#endif
				{
#ifdef FOURWAY_DEBUG
					printf("4-2: ERROR_NONEEQUL_REPLAYCOUNTER\n");
					printf("global->akm_sm->CurrentReplayCounter.field.LowPart = %d\n", global->akm_sm->CurrentReplayCounter.field.LowPart);
					printf("recievedRC.field.LowPart = %d\n", recievedRC.field.LowPart);
#endif
					retVal = ERROR_NONEEQUL_REPLAYCOUNTER;k
				}else
#endif // Don't check replay counter during dup 4-2
//#ifndef RTL_WPA2
#if 0
					// kenny: already increase CurrentReplayCounter after 4-1. Do it at the end of 4-2
					INCLargeInteger(&global->akm_sm->CurrentReplayCounter);
#endif

				goto check_msg2;
			}

			if (!CheckMIC(pStaInfo->EAPOLMsgRecvd, pStaInfo->PTK, PTK_LEN_EAPOLMIC)) { // errror
				DEBUG_ERR("4-4: RSN_MIC_failure\n");
#if 0
					if (global->RSNVariable.PMKCached ) {
						printf("\n%s:%d del_pmksa due to 4-4 RSN_MIC_failure\n", (char *)__FUNCTION__, __LINE__);
						global->RSNVariable.PMKCached = FALSE;
						del_pmksa_by_spa(global->theAuthenticator->supp_addr);
					}
#endif
				LOG_MSG("Authentication failled! (4-4: MIC error)\n");
#if defined(CONFIG_RTL8186_KB_N)
				authRes = 1;//Auth failed
#endif

#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
				LOG_MSG_NOTICE("Authentication failed;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
					pstat->hwaddr[0],
					pstat->hwaddr[1],
					pstat->hwaddr[2],
					pstat->hwaddr[3],
					pstat->hwaddr[4],
					pstat->hwaddr[5]);
#elif defined(CONFIG_RTL8196B_TLD)
				LOG_MSG_DEL("[WLAN access rejected: incorrect security] from MAC address: %02x:%02x:%02x:%02x:%02x:%02x,\n",
					pstat->hwaddr[0],
					pstat->hwaddr[1],
					pstat->hwaddr[2],
					pstat->hwaddr[3],
					pstat->hwaddr[4],
					pstat->hwaddr[5]);
#endif

				ToDrv_DisconnectSTA(priv, pstat, RSN_MIC_failure);
				pStaInfo->state = PSK_STATE_IDLE;
				break;
			}

			LOG_MSG("Open and authenticated\n");

#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
			LOG_MSG_NOTICE("Authentication Success;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
					pstat->hwaddr[0],
					pstat->hwaddr[1],
					pstat->hwaddr[2],
					pstat->hwaddr[3],
					pstat->hwaddr[4],
					pstat->hwaddr[5]);
#elif defined(CONFIG_RTL8196B_TLD)
			if (!list_empty(&priv->wlan_acl_list)) {
				LOG_MSG_DEL("[WLAN access allowed] from MAC: %02x:%02x:%02x:%02x:%02x:%02x,\n",
					pstat->hwaddr[0],
					pstat->hwaddr[1],
					pstat->hwaddr[2],
					pstat->hwaddr[3],
					pstat->hwaddr[4],
					pstat->hwaddr[5]);
			}
#endif

//#ifdef RTL_WPA2_PREAUTH
#if 0
			// update ANonce for next 4-way handshake
			SetNonce(akm_sm->ANonce, global->auth->Counter);
#endif

			//MLME-SETKEYS.request
			INCLargeInteger(&pStaInfo->CurrentReplayCounter);
			// kenny: n+2
			INCLargeInteger(&pStaInfo->CurrentReplayCounter);

			ToDrv_SetPTK(priv, pstat);
			if (pStaInfo->isGuest)
				ToDrv_SetPort(priv, pstat, DOT11_PortStatus_Guest);
			else
			ToDrv_SetPort(priv, pstat, DOT11_PortStatus_Authorized);

// david+2006-04-04, fix the issue of re-generating group key, and need not
// update group key in WPA2
//			pGblInfo->GInitAKeys = TRUE;
#ifdef RTL_WPA2
			if (!(pStaInfo->RSNEnabled & PSK_WPA2))
#endif
				pStaInfo->PInitAKeys = TRUE;
			pStaInfo->state = PSK_STATE_PTKINITDONE;
			pStaInfo->gstate = PSK_GSTATE_REKEYNEGOTIATING;

			//lib1x_akmsm_UpdateGK_proc() calls lib1x_akmsm_SendEAPOL_proc for 2-way
			//if group key sent is needed, send msg 1 of 2-way handshake
#ifdef RTL_WPA2
			if (pStaInfo->RSNEnabled & PSK_WPA2) {
				//------------------------------------------------------
				// Only when the group state machine is in the state of
				// (1) The first STA Connected,
				// (2) UPDATE GK to all station
				// does the GKeyDoneStations needed to be decreased
				//------------------------------------------------------

				if(pGblInfo->GKeyDoneStations > 0)
					pGblInfo->GKeyDoneStations--;

				//Avaya akm_sm->TimeoutCtr = 0;
				//To Do : set port secure to driver
//				global->portSecure = TRUE;
				//akm_sm->state = akmsm_PTKINITDONE;

				pStaInfo->gstate = PSK_GSTATE_REKEYESTABLISHED;

				if (pGblInfo->GKeyDoneStations==0 && !pGblInfo->GkeyReady) {
					ToDrv_SetGTK(priv);
					pGblInfo->GkeyReady = TRUE;
					pGblInfo->GResetCounter = TRUE;

					// start groupkey rekey timer
					if (priv->pmib->dot1180211AuthEntry.dot11GKRekeyTime)
						mod_timer(&pGblInfo->GKRekeyTimer, jiffies + priv->pmib->dot1180211AuthEntry.dot11GKRekeyTime*100);
				}
#if 0
				if (global->RSNVariable.PMKCached) {
					global->RSNVariable.PMKCached = FALSE;  // reset
				}
#endif
				DEBUG_INFO("\nWPA2: 4-way handshake done\n");
			}
#endif	// RTL_WPA2
			#ifdef CONFIG_RTL8671
			//cathy, after 4-way handshake, record the time and don't disassoc sta if MIC error in 5 seconds.
			if( pstat->dot11KeyMapping.keyInCam == TRUE ) {
				pstat->setCamTime = jiffies;
				printk("setCamTime OK\n");
			}
			#endif
			if (!Message_Secure(pStaInfo->EapolKeyMsgRecvd))
				UpdateGK(priv); // send 2-1
			break;

		case PSK_STATE_PTKINITDONE:
			// delete resend timer
			if (timer_pending(&pStaInfo->resendTimer))
				del_timer_sync(&pStaInfo->resendTimer);

#if 0
			//receive message [with request bit set]
			if(Message_Request(global->EapolKeyMsgRecvd))
			//supp request to initiate 4-way handshake
			{

			}
#endif

			//------------------------------------------------
			// Supplicant request to init 4 or 2 way handshake
			//------------------------------------------------
			if (Message_Request(pStaInfo->EapolKeyMsgRecvd)) {
				pStaInfo->state = PSK_STATE_PTKSTART;
				 if (Message_KeyType(pStaInfo->EapolKeyMsgRecvd) == type_Pairwise) {
					if (Message_Error(pStaInfo->EapolKeyMsgRecvd))
						IntegrityFailure(priv);
				 	else {
				 		if (Message_KeyType(pStaInfo->EapolKeyMsgRecvd) == type_Group) {
							if (Message_Error(pStaInfo->EapolKeyMsgRecvd)) {
								//auth change group key, initilate 4-way handshake with supp and execute
								//the Group key handshake to all Supplicants
								pGblInfo->GKeyFailure = TRUE;
								IntegrityFailure(priv);
							}
						}
				 	}
					//---- Start 4-way handshake ----
					SendEAPOL(priv, pstat, 0);
				}
//#ifdef RTL_WPA2_PREAUTH
//				printf("kenny: %s() in akmsm_PTKINITDONE state. Call lib1x_akmsm_UpdateGK_proc()\n", (char *)__FUNCTION__);
//#endif
				//---- Execute Group Key state machine for each STA ----
				UpdateGK(priv);
			}
			else
			{
			}

			break;
		default:
			break;

		}//switch

	}
	else if (Message_KeyType(pStaInfo->EapolKeyMsgRecvd) == type_Group) {

		// delete resend timer
		if (timer_pending(&pStaInfo->resendTimer))
			del_timer_sync(&pStaInfo->resendTimer);

		//---- Receive 2nd message of 2-way handshake ----
		DEBUG_INFO("2-2\n");
		if (!Message_Request(pStaInfo->EapolKeyMsgRecvd)) {//2nd message of 2-way handshake
			//verify that replay counter maches one it has used in the Group Key handshake
			if (Message_LargerReplayCounter(pStaInfo->CurrentReplayCounter, pStaInfo->EapolKeyMsgRecvd)) {
				DEBUG_ERR("ERROR_LARGER_REPLAYCOUNTER\n");
				return;
			}
			if (!CheckMIC(pStaInfo->EAPOLMsgRecvd, pStaInfo->PTK, PTK_LEN_EAPOLMIC)) {
				DEBUG_ERR("ERROR_MIC_FAIL\n");

				LOG_MSG("2-way handshake failled! (2-2: MIC error)\n");
#if defined(CONFIG_RTL8186_KB_N)
				authRes = 1;//Auth failed
#endif

#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
				LOG_MSG_NOTICE("Authentication failed;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
					pstat->hwaddr[0],
					pstat->hwaddr[1],
					pstat->hwaddr[2],
					pstat->hwaddr[3],
					pstat->hwaddr[4],
					pstat->hwaddr[5]);
#elif defined(CONFIG_RTL8196B_TLD)
				LOG_MSG_DEL("[WLAN access rejected: incorrect security] from MAC address: %02x:%02x:%02x:%02x:%02x:%02x,\n",
					pstat->hwaddr[0],
					pstat->hwaddr[1],
					pstat->hwaddr[2],
					pstat->hwaddr[3],
					pstat->hwaddr[4],
					pstat->hwaddr[5]);
#endif

				ToDrv_DisconnectSTA(priv, pstat, RSN_MIC_failure);
				pStaInfo->state = PSK_STATE_IDLE;
				return;
			}
		}
		else //if(!Message_Request(global->EapolKeyMsgRecvd))
		//supp request to change group key
		{
		}

		//------------------------------------------------------
		// Only when the group state machine is in the state of
		// (1) The first STA Connected,
		// (2) UPDATE GK to all station
		// does the GKeyDoneStations needed to be decreased
		//------------------------------------------------------

		if (pGblInfo->GKeyDoneStations > 0)
				pGblInfo->GKeyDoneStations--;

		//Avaya akm_sm->TimeoutCtr = 0;
		//To Do : set port secure to driver
		pStaInfo->gstate = PSK_GSTATE_REKEYESTABLISHED;

		if (pGblInfo->GKeyDoneStations == 0 && !pGblInfo->GkeyReady) {
   	        ToDrv_SetGTK(priv);
			DEBUG_INFO("2-way Handshake is finished\n");
       		pGblInfo->GkeyReady = TRUE;
			pGblInfo->GResetCounter = TRUE;

			// start groupkey rekey timer
			if (priv->pmib->dot1180211AuthEntry.dot11GKRekeyTime)
				mod_timer(&pGblInfo->GKRekeyTimer, jiffies + priv->pmib->dot1180211AuthEntry.dot11GKRekeyTime*100);
		}
		else {
			DEBUG_INFO(" Receive bad group key handshake\n");
         }
	}
}

#ifdef CLIENT_MODE
static void ClientEAPOLKeyRecvd(struct rtl8190_priv *priv, struct stat_info *pstat)
{
	WPA_GLOBAL_INFO *pGblInfo = priv->wpa_global_info;
	WPA_STA_INFO *pStaInfo = pstat->wpa_sta_info;
	LARGE_INTEGER recievedRC;
	struct lib1x_eapol *eapol;
	int toSetKey = 0;

	eapol = ( struct lib1x_eapol * ) ( pstat->wpa_sta_info->EAPOLMsgRecvd.Octet + ETHER_HDRLEN );
	if (eapol->packet_type != LIB1X_EAPOL_KEY)
	{
#ifdef DEBUG_PSK
		printk("Not Eapol-key pkt (type %d), drop\n", eapol->packet_type);
#endif
		return;
	}

	pStaInfo->EapolKeyMsgRecvd.Octet = pStaInfo->EAPOLMsgRecvd.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;

#ifdef DEBUG_PSK
	debug_out(NULL, pStaInfo->EAPOLMsgRecvd.Octet, pStaInfo->EAPOLMsgRecvd.Length);
#endif

	if (Message_KeyType(pStaInfo->EapolKeyMsgRecvd) == type_Pairwise) {

		if (Message_KeyMIC(pStaInfo->EapolKeyMsgRecvd) == FALSE)
		{
			//---- Receive 1st message and send 2nd
			DEBUG_INFO("client mode 4-1\n");

			if(pStaInfo->clientHndshkDone || pStaInfo->clientHndshkProcessing)
			{
				DEBUG_INFO("AP trigger pairwise rekey\n");

				//reset the client info-------------------------------------------------------
				pStaInfo->CurrentReplayCounter.field.HighPart = 0xffffffff;
				pStaInfo->CurrentReplayCounter.field.LowPart = 0xffffffff;

				pStaInfo->clientHndshkProcessing = FALSE;
				pStaInfo->clientHndshkDone = FALSE;

				pGblInfo->GkeyReady = FALSE;

				//-------------------------------------------------------reset the client info
			}

			//check replay counter
			if(!Message_DefaultReplayCounter(pStaInfo->CurrentReplayCounter) &&
				Message_SmallerEqualReplayCounter(pStaInfo->CurrentReplayCounter, pStaInfo->EapolKeyMsgRecvd) )
			{
				DEBUG_ERR("client mode 4-1: ERROR_NONEEQUL_REPLAYCOUNTER\n");
				return;
			}


			//set wpa_sta_info parameter----------------------------------------------------
			pstat->wpa_sta_info->RSNEnabled = priv->pmib->dot1180211AuthEntry.dot11EnablePSK;
			if (pstat->wpa_sta_info->RSNEnabled == 1) {
				if (priv->pmib->dot1180211AuthEntry.dot11WPACipher == 2)
					pstat->wpa_sta_info->UnicastCipher = 0x2;
				else if (priv->pmib->dot1180211AuthEntry.dot11WPACipher == 8)
					pstat->wpa_sta_info->UnicastCipher = 0x4;
				else
					printk("unicastcipher in wpa = nothing\n");
			}
			else if (pstat->wpa_sta_info->RSNEnabled == 2) {
				if (priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher == 2)
					pstat->wpa_sta_info->UnicastCipher = 0x2;
				else if (priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher == 8)
					pstat->wpa_sta_info->UnicastCipher = 0x4;
				else
					printk("unicastcipher in wpa = nothing\n");
			}
			//----------------------------------------------------set wpa_sta_info parameter

			INCOctet32_INTEGER(&pGblInfo->Counter);
			SetNonce(pStaInfo->SNonce, pGblInfo->Counter);

			pStaInfo->ANonce = Message_KeyNonce(pStaInfo->EapolKeyMsgRecvd);
			CalcPTK(pStaInfo->EAPOLMsgRecvd.Octet, pStaInfo->EAPOLMsgRecvd.Octet + 6,
					pStaInfo->ANonce.Octet, pStaInfo->SNonce.Octet,
					pStaInfo->PMK, PMK_LEN, pStaInfo->PTK, PTK_LEN_TKIP);

#ifdef DEBUG_PSK
			debug_out("PSK: Generated PTK=", pStaInfo->PTK, PTK_LEN_TKIP);
#endif

			//pStaInfo->clientHndshkProcessing = TRUE;-----------leave it to sendeapol
			ClientSendEAPOL(priv, pstat, 0);	// send msg 2
		}
		else
		{
			//---- Receive 3rd message ----
			DEBUG_INFO("client mode 4-3\n");

			pStaInfo->resendCnt = 0; //victoryman

			if (!pStaInfo->clientHndshkProcessing)
			{
				DEBUG_ERR("client mode 4-3: ERROR_MSG_1_ABSENT\n");
				return;
			}

			// delete resend timer
			if (timer_pending(&pStaInfo->resendTimer))
				del_timer_sync(&pStaInfo->resendTimer);

			Message_ReplayCounter_OC2LI(pStaInfo->EapolKeyMsgRecvd, &recievedRC);
			if(!Message_DefaultReplayCounter(pStaInfo->CurrentReplayCounter) &&
				Message_SmallerEqualReplayCounter(pStaInfo->CurrentReplayCounter, pStaInfo->EapolKeyMsgRecvd) )
			{
				DEBUG_ERR("client mode 4-3: ERROR_NONEEQUL_REPLAYCOUNTER\n");
				return;
			}
			else if (!Message_EqualKeyNonce(pStaInfo->EapolKeyMsgRecvd, pStaInfo->ANonce))
			{
				DEBUG_ERR("client mode 4-3: ANonce not equal\n");
				return;
			}
			else if(!CheckMIC(pStaInfo->EAPOLMsgRecvd, pStaInfo->PTK, PTK_LEN_EAPOLMIC))
			{
				DEBUG_ERR("client mode 4-3: PTK MIC ERROR\n");
				LOG_MSG("Authentication failled! (4-3: MIC error)\n");
#if defined(CONFIG_RTL8186_KB_N)
				authRes = 1;//Auth failed
#endif
				return;
			}

			pStaInfo->CurrentReplayCounter.field.HighPart = recievedRC.field.HighPart;
			pStaInfo->CurrentReplayCounter.field.LowPart = recievedRC.field.LowPart;

			if ((pStaInfo->RSNEnabled & PSK_WPA2) && (Message_DescType(pStaInfo->EapolKeyMsgRecvd) == desc_type_WPA2))
			{
				unsigned char decrypted_data[128];
				unsigned char GTK_KDE_OUI[] = { 0x00, 0x0F, 0xAC, 0x01 };
				unsigned char WPA_IE_OUI[] = { 0x00, 0x50, 0xF2, 0x01 };
				unsigned char *pGTK_KDE;
				unsigned char *pKeyData;
				unsigned short keyDataLength;

				keyDataLength = Message_KeyDataLength(pStaInfo->EapolKeyMsgRecvd);
				pKeyData = pStaInfo->EapolKeyMsgRecvd.Octet + KeyDataPos;

				if(!DecWPA2KeyData(pStaInfo, pKeyData, keyDataLength, pStaInfo->PTK + PTK_LEN_EAPOLMIC, PTK_LEN_EAPOLENC
				, decrypted_data))
				{
					DEBUG_ERR("client mode 4-3: ERROR_AESKEYWRAP_MIC_FAIL\n");
					LOG_MSG("Authentication failled! (4-3: ERROR_AESKEYWRAP_MIC_FAIL)\n");
#if defined(CONFIG_RTL8186_KB_N)
					authRes = 1;//Auth failed
#endif
					return;
				}

				//wpa2_hexdump("4-3 KeyData (Decrypted)",decrypted_data,keyDataLength);
				if ( decrypted_data[0] == WPA2_ELEMENT_ID)
				{
					pGTK_KDE = &decrypted_data[2] + (unsigned char)decrypted_data[1];
					if ( *pGTK_KDE == WPA2_ELEMENT_ID )
					{	// The second optional RSNIE is present
						DEBUG_ERR("client mode 4-3: The second optional RSNIE is present! Cannot handle it yet!\n");
						return;
					}
					else if ( *pGTK_KDE == WPA_ELEMENT_ID )
					{
						// if contain RSN IE, skip it
						if (!memcmp((pGTK_KDE+2), WPA_IE_OUI, sizeof(WPA_IE_OUI)))
							pGTK_KDE += (unsigned char)*(pGTK_KDE+1) + 2;

						if (!memcmp((pGTK_KDE+2), GTK_KDE_OUI, sizeof(GTK_KDE_OUI)))
						{
							// GTK Key Data Encapsulation Format
							unsigned char gtk_len = (unsigned char)*(pGTK_KDE+1) - 6;
							unsigned char keyID = (unsigned char)*(pGTK_KDE+6) & 0x03;
							pGblInfo->GN = keyID;
							memcpy(pGblInfo->GTK[keyID], (pGTK_KDE+8), gtk_len);
							toSetKey = 1;
							pGblInfo->GkeyReady = TRUE;
						}
					}

					//check AP's RSNIE and set Group Key Chiper
					if (decrypted_data[7] == _TKIP_PRIVACY_)
						pGblInfo->MulticastCipher = _TKIP_PRIVACY_ ;
					else if (decrypted_data[7] == _CCMP_PRIVACY_)
						pGblInfo->MulticastCipher = _CCMP_PRIVACY_ ;
				}
			}
			else if ((pStaInfo->RSNEnabled & PSK_WPA) && (Message_DescType(pStaInfo->EapolKeyMsgRecvd) == desc_type_RSN))
			{
				unsigned char WPAkeyData[255];
				unsigned short DataLength;
				memset(WPAkeyData, 0, 255);
				DataLength = Message_KeyDataLength(pStaInfo->EapolKeyMsgRecvd);
				memcpy(WPAkeyData, pStaInfo->EapolKeyMsgRecvd.Octet+KeyDataPos, 255);

				//check AP's RSNIE and set Group Key Chiper
				if (WPAkeyData[11] == _TKIP_PRIVACY_)
					pGblInfo->MulticastCipher = _TKIP_PRIVACY_ ;
				else if (WPAkeyData[11] == _CCMP_PRIVACY_)
					pGblInfo->MulticastCipher = _CCMP_PRIVACY_ ;
			}

			ClientSendEAPOL(priv, pstat, 0);	// send msg 4

			if (toSetKey)
			{
				ToDrv_SetGTK(priv);
				toSetKey = 0;
			}
			ToDrv_SetPTK(priv, pstat);
			ToDrv_SetPort(priv, pstat, DOT11_PortStatus_Authorized);
		}
	}
	else if (Message_KeyType(pStaInfo->EapolKeyMsgRecvd) == type_Group) {

		unsigned char decrypted_data[128];
		unsigned char GTK_KDE_OUI[] = { 0x00, 0x0F, 0xAC, 0x01 };
		unsigned char *pGTK_KDE;
		unsigned char keyID;

		//---- Receive 1st message of 2-way handshake ----
		DEBUG_INFO("client mode receive 2-1\n");

		pStaInfo->resendCnt = 0; //victoryman

		Message_ReplayCounter_OC2LI(pStaInfo->EapolKeyMsgRecvd, &recievedRC);

		if(Message_SmallerEqualReplayCounter(pStaInfo->CurrentReplayCounter, pStaInfo->EapolKeyMsgRecvd) )
		{
			DEBUG_ERR("client mode 2-1: ERROR_NONEEQUL_REPLAYCOUNTER\n");
			return;
		}
		else if(!CheckMIC(pStaInfo->EAPOLMsgRecvd, pStaInfo->PTK, PTK_LEN_EAPOLMIC))
		{
			DEBUG_ERR("client mode 2-1: ERROR_MIC_FAIL\n");
			LOG_MSG("Authentication failled! (4-2: MIC error)\n");
#if defined(CONFIG_RTL8186_KB_N)
			authRes = 1;//Auth failed
#endif
			return;
		}
		else if(!DecGTK(pStaInfo->EAPOLMsgRecvd, pStaInfo->PTK + PTK_LEN_EAPOLMIC, PTK_LEN_EAPOLENC,
			((pGblInfo->MulticastCipher == DOT11_ENC_TKIP) ? 32:16),
			pGblInfo->GTK[Message_KeyIndex(pStaInfo->EapolKeyMsgRecvd)]))
		{
			DEBUG_ERR("client mode 2-1: ERROR_AESKEYWRAP_MIC_FAIL\n");
			return;
		}

		keyID = Message_KeyIndex(pStaInfo->EapolKeyMsgRecvd);
		if ((pStaInfo->RSNEnabled & PSK_WPA2) && (Message_DescType(pStaInfo->EapolKeyMsgRecvd) == desc_type_WPA2))
		{
			memcpy(decrypted_data, pGblInfo->GTK[keyID], Message_KeyDataLength(pStaInfo->EapolKeyMsgRecvd));
			pGTK_KDE = decrypted_data;
			if ( *pGTK_KDE == WPA_ELEMENT_ID && !memcmp((pGTK_KDE+2), GTK_KDE_OUI, sizeof(GTK_KDE_OUI)))
			{
				// GTK Key Data Encapsulation Format
				unsigned char gtk_len = (unsigned char)*(pGTK_KDE+1) - 6;
				keyID = (unsigned char)*(pGTK_KDE+6) & 0x03;
				pGblInfo->GN = keyID;
				memcpy(pGblInfo->GTK[keyID], (pGTK_KDE+8), gtk_len);
			}
		}
		else
		{
			pGblInfo->GN = keyID;
		}
		//MLME_SETKEYS.request() to set Group Key;
		pGblInfo->GkeyReady = TRUE;

		pStaInfo->CurrentReplayCounter.field.HighPart = recievedRC.field.HighPart;
		pStaInfo->CurrentReplayCounter.field.LowPart = recievedRC.field.LowPart;

		ToDrv_SetGTK(priv);
		ClientSendEAPOL(priv, pstat, 0); // send msg 2-1
	}
	else
		printk("Client EAPOL Key Receive ERROR!!\n");
}
#endif // CLIENT_MODE

void psk_init(struct rtl8190_priv *priv)
{
	WPA_GLOBAL_INFO *pGblInfo=priv->wpa_global_info;
	int i, j, low_cipher=0;

	DEBUG_TRACE;

// button 2009.05.21 
// do not reset multicast cipher
	low_cipher = pGblInfo->MulticastCipher ;

	memset((char *)pGblInfo, '\0', sizeof(WPA_GLOBAL_INFO));

	//---- Counter is initialized whenever boot time ----
	GenNonce(pGblInfo->Counter.charData, (unsigned char*)"addr");

	if (OPMODE & WIFI_AP_STATE)
	{
		//---- Initialize Goup Key state machine ----
		pGblInfo->GNonce.Octet = pGblInfo->GNonceBuf;
		pGblInfo->GNonce.Length = KEY_NONCE_LEN;
		pGblInfo->GTKAuthenticator = TRUE;
		pGblInfo->GN = 1;
		pGblInfo->GM = 2;
		pGblInfo->GInitAKeys = TRUE; // david+2006-04-04, fix the issue of re-generating group key

		init_timer(&pGblInfo->GKRekeyTimer);
		pGblInfo->GKRekeyTimer.data = (unsigned long)priv;
		pGblInfo->GKRekeyTimer.function = GKRekeyTimeout;
	}

	if (strlen(priv->pmib->dot1180211AuthEntry.dot11PassPhrase) == 64) // hex
		get_array_val(pGblInfo->PSK, priv->pmib->dot1180211AuthEntry.dot11PassPhrase, 64);
	else
		PasswordHash(priv->pmib->dot1180211AuthEntry.dot11PassPhrase, priv->pmib->dot11StationConfigEntry.dot11DesiredSSID,
					pGblInfo->PSK);

	if (priv->pmib->dot1180211AuthEntry.dot11PassPhraseGuest[0]) {
		if (strlen(priv->pmib->dot1180211AuthEntry.dot11PassPhraseGuest) == 64)
			get_array_val(pGblInfo->PSKGuest, priv->pmib->dot1180211AuthEntry.dot11PassPhraseGuest, 64);
		else
			PasswordHash(priv->pmib->dot1180211AuthEntry.dot11PassPhraseGuest, priv->pmib->dot11StationConfigEntry.dot11DesiredSSID,
						pGblInfo->PSKGuest);
	}

#ifdef DEBUG_PSK
	debug_out("PSK: PMK=", pGblInfo->PSK, PMK_LEN);
	if (priv->pmib->dot1180211AuthEntry.dot11PassPhraseGuest[0])
		debug_out("PSK-Guest: PMK=", pGblInfo->PSKGuest, PMK_LEN);
#endif

	if ((priv->pmib->dot1180211AuthEntry.dot11EnablePSK & PSK_WPA) &&
			!priv->pmib->dot1180211AuthEntry.dot11WPACipher) {
		DEBUG_ERR("psk_init failed, WPA cipher did not set!\n");
		return;
	}

#ifdef RTL_WPA2
	if ((priv->pmib->dot1180211AuthEntry.dot11EnablePSK & PSK_WPA2) &&
			!priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher) {
		DEBUG_ERR("psk_init failed, WPA2 cipher did not set!\n");
		return;
	}
#endif

	if ((priv->pmib->dot1180211AuthEntry.dot11EnablePSK & PSK_WPA2) &&		
			!(priv->pmib->dot1180211AuthEntry.dot11EnablePSK & PSK_WPA)) {
		if (priv->pmib->dot1180211AuthEntry.dot11WPACipher)
			priv->pmib->dot1180211AuthEntry.dot11WPACipher = 0;			
	}
	if ((priv->pmib->dot1180211AuthEntry.dot11EnablePSK & PSK_WPA) &&		
			!(priv->pmib->dot1180211AuthEntry.dot11EnablePSK & PSK_WPA2)) {
		if (priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher)
			priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher = 0;		
	}

	if (priv->pmib->dot1180211AuthEntry.dot11WPACipher) {
		for (i=0, j=0; i<_WEP_104_PRIVACY_; i++) {
			if (priv->pmib->dot1180211AuthEntry.dot11WPACipher & (1<<i)) {
				pGblInfo->UnicastCipher[j] = i+1;
				if (low_cipher == 0)
					low_cipher = pGblInfo->UnicastCipher[j];
				else {
					if (low_cipher == _WEP_104_PRIVACY_ &&
							pGblInfo->UnicastCipher[j] == _WEP_40_PRIVACY_)
						low_cipher = pGblInfo->UnicastCipher[j];
					else if (low_cipher == _TKIP_PRIVACY_ &&
							(pGblInfo->UnicastCipher[j] == _WEP_40_PRIVACY_ ||
								pGblInfo->UnicastCipher[j] == _WEP_104_PRIVACY_))
							low_cipher = pGblInfo->UnicastCipher[j];
					else if (low_cipher == _CCMP_PRIVACY_)
							low_cipher = pGblInfo->UnicastCipher[j];
				}
				if (++j >= MAX_UNICAST_CIPHER)
					break;
			}
		}
		pGblInfo->NumOfUnicastCipher = j;
	}

#ifdef CLIENT_MODE
	if((OPMODE & WIFI_ADHOC_STATE)&&(priv->pmib->dot1180211AuthEntry.dot11EnablePSK & 2)) // if WPA2
		low_cipher = 0;
#endif

#ifdef RTL_WPA2
	if (priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher) {
		for (i=0, j=0; i<_WEP_104_PRIVACY_; i++) {
			if (priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher & (1<<i)) {
				pGblInfo->UnicastCipherWPA2[j] = i+1;
				if (low_cipher == 0)
					low_cipher = pGblInfo->UnicastCipherWPA2[j];
				else {
					if (low_cipher == _WEP_104_PRIVACY_ &&
							pGblInfo->UnicastCipherWPA2[j] == _WEP_40_PRIVACY_)
						low_cipher = pGblInfo->UnicastCipherWPA2[j];
					else if (low_cipher == _TKIP_PRIVACY_ &&
							(pGblInfo->UnicastCipherWPA2[j] == _WEP_40_PRIVACY_ ||
								pGblInfo->UnicastCipherWPA2[j] == _WEP_104_PRIVACY_))
							low_cipher = pGblInfo->UnicastCipherWPA2[j];
					else if (low_cipher == _CCMP_PRIVACY_)
							low_cipher = pGblInfo->UnicastCipherWPA2[j];
				}
				if (++j >= MAX_UNICAST_CIPHER)
					break;
			}
		}
		pGblInfo->NumOfUnicastCipherWPA2= j;
	}
#endif

	pGblInfo->MulticastCipher = low_cipher;

#ifdef DEBUG_PSK
	printk("PSK: WPA unicast cipher= ");
	for (i=0; i<pGblInfo->NumOfUnicastCipher; i++)
		printk("%x ", pGblInfo->UnicastCipher[i]);
	printk("\n");

#ifdef RTL_WPA2
	printk("PSK: WPA2 unicast cipher= ");
	for (i=0; i<pGblInfo->NumOfUnicastCipherWPA2; i++)
		printk("%x ", pGblInfo->UnicastCipherWPA2[i]);
	printk("\n");
#endif

	printk("PSK: multicast cipher= %x\n", pGblInfo->MulticastCipher);
#endif

	pGblInfo->AuthInfoElement.Octet = pGblInfo->AuthInfoBuf;

	ConstructIE(priv, pGblInfo->AuthInfoElement.Octet,
					 &pGblInfo->AuthInfoElement.Length);

	ToDrv_SetIE(priv);
}

#if defined(WDS) && defined(INCLUDE_WPA_PSK)
void wds_psk_init(struct rtl8190_priv *priv)
{
	unsigned char pchar[40];
	int i, is_hashed=0;

	if ( !(OPMODE & WIFI_AP_STATE))
		return;

	for (i = 0; i < priv->pmib->dot11WdsInfo.wdsNum; i++)
	{
		if (strlen(priv->pmib->dot11WdsInfo.wdsPskPassPhrase) == 64) // hex
			get_array_val(priv->pmib->dot11WdsInfo.wdsMapingKey[i], priv->pmib->dot11WdsInfo.wdsPskPassPhrase, 64);
		else
		{
			memset(pchar, 0, sizeof(unsigned char)*40);
			if (!is_hashed) {
				PasswordHash(priv->pmib->dot11WdsInfo.wdsPskPassPhrase,"REALTEK",pchar);
				is_hashed = 1;
			}
			memcpy(priv->pmib->dot11WdsInfo.wdsMapingKey[i], pchar, sizeof(unsigned char)*32);
			//priv->pmib->dot11WdsInfo.wdsMappingKeyLen[i] = 32;
		}
		priv->pmib->dot11WdsInfo.wdsMappingKeyLen[i] = 32;
		priv->pmib->dot11WdsInfo.wdsMappingKeyLen[i] |= 0x80000000;  //set bit to protect the key
	}
}
#endif

#ifdef CONFIG_ENABLE_MIPS16
unsigned char tmpbuf[1024]; //for mips16 comipler error
#endif
void psk_indicate_evt(struct rtl8190_priv *priv, int id, unsigned char *mac, unsigned char *msg, int len)
{
	struct stat_info *pstat;
#if !defined(CONFIG_ENABLE_MIPS16)
	unsigned char tmpbuf[1024];
#endif
	int ret;
#ifdef RTL_WPA2
	int isWPA2=0;
#endif

	if (!priv->pmib->dot1180211AuthEntry.dot11EnablePSK ||
		!((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_) ||
		  (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_)))
		return;

#ifdef DEBUG_PSK
	printk("PSK: Got evt:%s[%x], sta: %02x:%02x:%02x:%02x:%02x:%02x, msg_len=%x\n",
			ID2STR(id), id,
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
#endif

	pstat = get_stainfo(priv, mac);
// button 2009.05.21
#if 0
	if (pstat == NULL) {
#else
	if (pstat == NULL && id!=DOT11_EVENT_WPA_MULTICAST_CIPHER && id!=DOT11_EVENT_WPA2_MULTICAST_CIPHER) {
#endif
		DEBUG_ERR("Invalid mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		return;
	}

	switch (id) {
	case DOT11_EVENT_ASSOCIATION_IND:
	case DOT11_EVENT_REASSOCIATION_IND:
		reset_sta_info(priv, pstat);

		if (OPMODE & WIFI_AP_STATE) {
			// check RSNIE
			if (len > 2 && msg != NULL) {
#ifdef DEBUG_PSK
				debug_out("PSK: Rx Assoc-ind, RSNIE", msg, len);
#endif

#ifdef RTL_WPA2
				memcpy(tmpbuf, msg, len);
				len -= 2;
#else
				tmpbuf[0] = RSN_ELEMENT_ID;
				tmpbuf[1] = len;
				memcpy(tmpbuf+2, msg, len);
#endif

#ifdef RTL_WPA2
				isWPA2 = (tmpbuf[0] == WPA2_ELEMENT_ID) ? 1 : 0;
				if (isWPA2)
					ret = parseIEWPA2(priv, pstat->wpa_sta_info, tmpbuf, len+2);
				else
#endif
					ret = parseIE(priv, pstat->wpa_sta_info, tmpbuf, len+2);
				if (ret != 0) {
					DEBUG_ERR("parse IE error [%x]!\n", ret);
				}

				// issue assoc-rsp successfully
				ToDrv_RspAssoc(priv, id, mac, -ret);

				if (ret == 0) {
#ifdef EVENT_LOG
					char *pmsg;
					switch (pstat->wpa_sta_info->UnicastCipher) {
						case DOT11_ENC_NONE:	pmsg = "none"; 	break;
						case DOT11_ENC_WEP40:	pmsg = "WEP40"; 	break;
						case DOT11_ENC_TKIP:	pmsg = "TKIP"; 	break;
						case DOT11_ENC_WRAP:	pmsg = "AES";	break;
						case DOT11_ENC_CCMP:	pmsg = "AES";	break;
						case DOT11_ENC_WEP104:	pmsg = "WEP104";	break;
						default: pmsg = "invalid algorithm";			break;
					}
#ifdef RTL_WPA2
					LOG_MSG("%s-%s PSK authentication in progress...\n", (isWPA2 ? "WPA2" : "WPA"), pmsg);
#else
					LOG_MSG("%s-WPA PSK authentication in progress...\n",  pmsg);
#endif
#endif

#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD)
					LOG_MSG_NOTICE("Authenticating......;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
						pstat->hwaddr[0],
						pstat->hwaddr[1],
						pstat->hwaddr[2],
						pstat->hwaddr[3],
						pstat->hwaddr[4],
						pstat->hwaddr[5]);
#endif
					AuthenticationRequest(priv, pstat); // send 4-1
				}
			}
			else { // RNSIE is null
				if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK)
					ToDrv_RspAssoc(priv, id, mac, -ERROR_INVALID_RSNIE);
			}
		}
		break;

	case DOT11_EVENT_DISASSOCIATION_IND:
		reset_sta_info(priv, pstat);
		break;

	case DOT11_EVENT_EAP_PACKET:
		if (OPMODE & WIFI_AP_STATE) {
			if (pstat->wpa_sta_info->state == PSK_STATE_IDLE) {
				DEBUG_ERR("Rx EAPOL packet but did not get Assoc-Ind yet!\n");
				break;
			}
		}

		if (len > MAX_EAPOLMSG_LEN) {
			DEBUG_ERR("Rx EAPOL packet which length is too long [%x]!\n", len);
			break;
		}

#ifdef CLIENT_MODE
		if ((OPMODE & WIFI_STATION_STATE) &&
				!(pstat->wpa_sta_info->clientHndshkProcessing||pstat->wpa_sta_info->clientHndshkDone)) {
#ifdef EVENT_LOG
			char *pmsg;
			switch (pstat->wpa_sta_info->UnicastCipher) {
				case DOT11_ENC_NONE:	pmsg = "none"; 	break;
				case DOT11_ENC_WEP40:	pmsg = "WEP40"; 	break;
				case DOT11_ENC_TKIP:	pmsg = "TKIP"; 	break;
				case DOT11_ENC_WRAP:	pmsg = "AES";	break;
				case DOT11_ENC_CCMP:	pmsg = "AES";	break;
				case DOT11_ENC_WEP104:	pmsg = "WEP104";	break;
				default: pmsg = "invalid algorithm";			break;
			}
			LOG_MSG("%s-%s PSK authentication in progress...\n", (isWPA2 ? "WPA2" : "WPA"), pmsg);
#endif
			reset_sta_info(priv, pstat);
		}
#endif
		memcpy(pstat->wpa_sta_info->EAPOLMsgRecvd.Octet, msg, len);
		pstat->wpa_sta_info->EAPOLMsgRecvd.Length = len;
		if (OPMODE & WIFI_AP_STATE)
			EAPOLKeyRecvd(priv, pstat);
#ifdef CLIENT_MODE
		else if (OPMODE & WIFI_STATION_STATE)
			ClientEAPOLKeyRecvd(priv, pstat);
#endif
		break;

	case DOT11_EVENT_MIC_FAILURE:
		// do nothing
		break;

// button 2009.05.21
	case DOT11_EVENT_WPA_MULTICAST_CIPHER:
	case DOT11_EVENT_WPA2_MULTICAST_CIPHER:
#ifdef CLIENT_MODE
		if (OPMODE & WIFI_STATION_STATE)
		{
			priv->wpa_global_info->MulticastCipher = *msg;
			ConstructIE(priv, priv->wpa_global_info->AuthInfoElement.Octet,
							 &priv->wpa_global_info->AuthInfoElement.Length);		
			memcpy((void *)priv->pmib->dot11RsnIE.rsnie, priv->wpa_global_info->AuthInfoElement.Octet
					, priv->wpa_global_info->AuthInfoElement.Length);
			DEBUG_WARN("####### MulticastCipher=%d\n", priv->wpa_global_info->MulticastCipher);
		}
#endif
		break;
	}

}

#endif // INCLUDE_WPA_PSK


