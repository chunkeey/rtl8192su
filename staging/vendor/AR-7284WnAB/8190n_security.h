/*
 *      Header file defines the interface with AUTH daemon (802.1x authenticator)
 *
 *      $Id: 8190n_security.h,v 1.7 2009/08/06 11:41:29 yachang Exp $
 */

#ifndef	_8190N_SECURITY_H_
#define _8190N_SECURITY_H_

#include "./8190n_cfg.h"

typedef struct _DOT11_QUEUE_NODE
{
        unsigned short  ItemSize;
        unsigned char   Item[MAXDATALEN];
}DOT11_QUEUE_NODE;

typedef struct _DOT11_QUEUE
{
        int     Head;
        int     Tail;
        int     NumItem;
        int     MaxItem;
        DOT11_QUEUE_NODE        ItemArray[MAXQUEUESIZE];
}DOT11_QUEUE;

typedef unsigned char DOT11_KEY_RSC[8];

typedef enum{
        DOT11_KeyType_Group = 0,
        DOT11_KeyType_Pairwise = 1
} DOT11_KEY_TYPE;

typedef enum{
	DOT11_EAPOL_GROUP_INDEX = 0,
	DOT11_EAPOL_PAIRWISE_INDEX = 3
} DOT11_EAPOL_KEY_INDEX;

typedef enum{
        DOT11_KeyUsage_ENC,
        DOT11_KeyUsage_MIC
} DOT11_KEY_USAGE;

typedef enum{
        DOT11_Role_Auth,
        DOT11_Role_Supp
} DOT11_ROLE;

typedef enum{
        DOT11_VARIABLE_MACEnable,
        DOT11_VARIABLE_SystemAuthControl,
        DOT11_VARIABLE_AuthControlledPortStatus,
        DOT11_VARIABLE_AuthControlledPortControl,
        DOT11_VARIABLE_AuthenticationType,
        DOT11_VARIABLE_KeyManagement,
        DOT11_VARIABLE_MulticastCipher,
        DOT11_VARIABLE_UnicastCipher
} DOT11_VARIABLE_TYPE;

typedef enum{
        DOT11_SysAuthControl_Disabled,
        DOT11_SysAuthControl_Enabled
} DOT11_SYSTEM_AUTHENTICATION_CONTROL;

typedef enum{
        DOT11_PortControl_ForceUnauthorized,
        DOT11_PortControl_ForceAuthorized,
        DOT11_PortControl_Auto
} DOT11_PORT_CONTROL;

typedef enum{
        DOT11_PortStatus_Unauthorized,
        DOT11_PortStatus_Authorized,
        DOT11_PortStatus_Guest
}DOT11_PORT_STATUS;

typedef enum{
        DOT11_Association_Fail,
        DOT11_Association_Success
}DOT11_ASSOCIATION_RESULT;

typedef enum{
        DOT11_AuthKeyType_RSN = 1,
        DOT11_AuthKeyType_PSK = 2,
        DOT11_AuthKeyType_NonRSN802dot1x = 3
} DOT11_AUTHKEY_TYPE;

typedef enum{
	DOT11_AuthKeyType_RSN_MAP = 1,
        DOT11_AuthKeyType_PSK_MAP = 2,
        DOT11_AuthKeyType_NonRSN802dot1x_MAP = 4
} DOT11_AUTHKEY_TYPE_MAP;

typedef enum{
	DOT11_Ioctl_Query = 0,
	DOT11_Ioctl_Set = 1
} DOT11_Ioctl_Flag;

typedef enum{
        DOT11_ENC_NONE  = 0,
        DOT11_ENC_WEP40 = 1,
        DOT11_ENC_TKIP  = 2,
        DOT11_ENC_WRAP  = 3,
        DOT11_ENC_CCMP  = 4,
        DOT11_ENC_WEP104= 5
} DOT11_ENC_ALGO;

typedef enum{
        DOT11_ENC_NONE_MAP  = 1,
        DOT11_ENC_WEP40_MAP = 2,
        DOT11_ENC_TKIP_MAP  = 4,
        DOT11_ENC_WRAP_MAP  = 8,
        DOT11_ENC_CCMP_MAP  = 16,
        DOT11_ENC_WEP104_MAP= 32
} DOT11_ENC_ALGO_MAP;


typedef enum{
        DOT11_EVENT_NO_EVENT = 1,
        DOT11_EVENT_REQUEST = 2,
        DOT11_EVENT_ASSOCIATION_IND = 3,
        DOT11_EVENT_ASSOCIATION_RSP = 4,
        DOT11_EVENT_AUTHENTICATION_IND = 5,
        DOT11_EVENT_REAUTHENTICATION_IND = 6,
        DOT11_EVENT_DEAUTHENTICATION_IND = 7,
        DOT11_EVENT_DISASSOCIATION_IND = 8,
        DOT11_EVENT_DISCONNECT_REQ = 9,
        DOT11_EVENT_SET_802DOT11 = 10,
        DOT11_EVENT_SET_KEY = 11,
        DOT11_EVENT_SET_PORT = 12,
        DOT11_EVENT_DELETE_KEY = 13,
        DOT11_EVENT_SET_RSNIE = 14,
        DOT11_EVENT_GKEY_TSC = 15,
        DOT11_EVENT_MIC_FAILURE = 16,
        DOT11_EVENT_ASSOCIATION_INFO = 17,
        DOT11_EVENT_INIT_QUEUE = 18,
        DOT11_EVENT_EAPOLSTART = 19,

        DOT11_EVENT_ACC_SET_EXPIREDTIME = 31,
        DOT11_EVENT_ACC_QUERY_STATS = 32,
        DOT11_EVENT_ACC_QUERY_STATS_ALL = 33,
        DOT11_EVENT_REASSOCIATION_IND = 34,
        DOT11_EVENT_REASSOCIATION_RSP = 35,
        DOT11_EVENT_STA_QUERY_BSSID = 36,
        DOT11_EVENT_STA_QUERY_SSID = 37,
        DOT11_EVENT_EAP_PACKET = 41,

#ifdef RTL_WPA2_PREAUTH
        DOT11_EVENT_EAPOLSTART_PREAUTH = 45,
        DOT11_EVENT_EAP_PACKET_PREAUTH = 46,
#endif

        DOT11_EVENT_WPA2_MULTICAST_CIPHER = 47,
        DOT11_EVENT_WPA_MULTICAST_CIPHER = 48,

#ifdef WIFI_SIMPLE_CONFIG
		DOT11_EVENT_WSC_SET_IE = 55,
		DOT11_EVENT_WSC_PROBE_REQ_IND = 56,
		DOT11_EVENT_WSC_PIN_IND = 57,
		DOT11_EVENT_WSC_ASSOC_REQ_IE_IND = 58,
#endif
#ifdef	CONFIG_RTK_MESH 
	DOT11_EVENT_PATHSEL_GEN_RREQ = 59, 
	DOT11_EVENT_PATHSEL_GEN_RERR = 60,
	DOT11_EVENT_PATHSEL_RECV_RREQ = 61,                    
	DOT11_EVENT_PATHSEL_RECV_RREP = 62,                   
	DOT11_EVENT_PATHSEL_RECV_RERR = 63,
	DOT11_EVENT_PATHSEL_RECV_RREP_ACK = 64,
	DOT11_EVENT_PATHSEL_RECV_PANN = 65,
	DOT11_EVENT_PATHSEL_RECV_RANN = 66,
#endif // CONFIG_RTK_MESH
#ifdef CONFIG_RTL_WAPI_SUPPORT
	DOT11_EVENT_WAPI_INIT_QUEUE =67,
	DOT11_EVENT_WAPI_READ_QUEUE = 68,
	DOT11_EVENT_WAPI_WRITE_QUEUE  =69
#endif
} DOT11_EVENT;

#ifdef WIFI_SIMPLE_CONFIG
enum {SET_IE_FLAG_BEACON=1, SET_IE_FLAG_PROBE_RSP=2, SET_IE_FLAG_PROBE_REQ=3,
		SET_IE_FLAG_ASSOC_RSP=4, SET_IE_FLAG_ASSOC_REQ=5};
#endif

typedef struct _DOT11_GENERAL{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        unsigned char   *Data;
}DOT11_GENERAL;

typedef struct _DOT11_NOEVENT{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
}DOT11_NO_EVENT;

typedef struct _DOT11_REQUEST{
        unsigned char   EventId;
}DOT11_REQUEST;

typedef struct _DOT11_WPA2_MULTICAST_CIPHER{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        unsigned char	MulticastCipher;
}DOT11_WPA2_MULTICAST_CIPHER;

typedef struct _DOT11_WPA_MULTICAST_CIPHER{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        unsigned char	MulticastCipher;
}DOT11_WPA_MULTICAST_CIPHER;

typedef struct _DOT11_ASSOCIATION_IND{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
        unsigned short  RSNIELen;
        char            RSNIE[MAXRSNIELEN]; // include ID and Length by kenny
}DOT11_ASSOCIATION_IND;

typedef struct _DOT11_ASSOCIATION_RSP{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
        unsigned char   Status;
}DOT11_ASSOCIATIIN_RSP;

typedef struct _DOT11_REASSOCIATION_IND{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
        unsigned short  RSNIELen;
        char            RSNIE[MAXRSNIELEN];
        char            OldAPaddr[MACADDRLEN];
}DOT11_REASSOCIATION_IND;

typedef struct _DOT11_REASSOCIATION_RSP{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
        unsigned char   Status;
        char            CurrAPaddr[MACADDRLEN];
}DOT11_REASSOCIATIIN_RSP;

typedef struct _DOT11_AUTHENTICATION_IND{
	unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
}DOT11_AUTHENTICATION_IND;

typedef struct _DOT11_REAUTHENTICATION_IND{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
}DOT11_REAUTHENTICATION_IND;

#ifdef WIFI_SIMPLE_CONFIG
typedef struct _DOT11_PROBE_REQUEST_IND{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
        unsigned short  ProbeIELen;
        char            ProbeIE[PROBEIELEN];
}DOT11_PROBE_REQUEST_IND;

typedef struct _DOT11_WSC_ASSOC_IND{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
        unsigned short  AssocIELen;
        char            AssocIE[PROBEIELEN];
	  unsigned char wscIE_included;
}DOT11_WSC_ASSOC_IND;
#endif

typedef struct _DOT11_DEAUTHENTICATION_IND{
	unsigned char   EventId;
	unsigned char   IsMoreEvent;
	char            MACAddr[MACADDRLEN];
	unsigned long	tx_packets;       // == transmited packets
	unsigned long	rx_packets;       // == received packets
	unsigned long	tx_bytes;         // == transmited bytes
	unsigned long	rx_bytes;         // == received bytes
	unsigned long  	Reason;
}DOT11_DEAUTHENTICATION_IND;

typedef struct _DOT11_DISASSOCIATION_IND{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
	unsigned long	tx_packets;       // == transmited packets
	unsigned long	rx_packets;       // == received packets
	unsigned long	tx_bytes;         // == transmited bytes
	unsigned long	rx_bytes;         // == received bytes
	unsigned long  	Reason;
}DOT11_DISASSOCIATION_IND;

typedef struct _DOT11_DISCONNECT_REQ{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        unsigned short  Reason;
        char            MACAddr[MACADDRLEN];
}DOT11_DISCONNECT_REQ;

typedef struct _DOT11_SET_802DOT11{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        unsigned char   VariableType;
        unsigned char   VariableValue;
	char            MACAddr[MACADDRLEN];
}DOT11_SET_802DOT11;

typedef struct _DOT11_SET_KEY{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
	unsigned long   KeyIndex;
	unsigned long   KeyLen;
	unsigned char   KeyType;
	unsigned char	EncType;
        unsigned char   MACAddr[MACADDRLEN];
	DOT11_KEY_RSC   KeyRSC;
	unsigned char   KeyMaterial[64];
}DOT11_SET_KEY;

typedef struct _DOT11_DELETE_KEY{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
        unsigned char   KeyType;
}DOT11_DELETE_KEY;

typedef struct _DOT11_SET_RSNIE{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
	unsigned short	Flag;
        unsigned short  RSNIELen;
        char            RSNIE[MAXRSNIELEN];
	char            MACAddr[MACADDRLEN];
}DOT11_SET_RSNIE;

typedef struct _DOT11_SET_PORT{
        unsigned char EventId;
        unsigned char PortStatus;
        unsigned char PortType;
        unsigned char MACAddr[MACADDRLEN];
}DOT11_SET_PORT;

typedef struct _DOT11_GKEY_TSC{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
	unsigned char	KeyTSC[8];
}DOT11_GKEY_TSC;

typedef struct _DOT11_MIC_FAILURE{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
}DOT11_MIC_FAILURE;

typedef struct _DOT11_STA_QUERY_BSSID{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        unsigned long   IsValid;
        char            Bssid[MACADDRLEN];
}DOT11_STA_QUERY_BSSID;

typedef struct _DOT11_STA_QUERY_SSID{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        unsigned long   IsValid;
        char            ssid[32];
        int             ssid_len;
}DOT11_STA_QUERY_SSID;

typedef struct _DOT11_EAPOL_START{
        unsigned char   EventId;
        unsigned char   IsMoreEvent;
        char            MACAddr[MACADDRLEN];
}DOT11_EAPOL_START;

typedef struct _DOT11_SET_EXPIREDTIME{
        unsigned char EventId;
        unsigned char IsMoreEvent;
        unsigned char MACAddr[MACADDRLEN];
		unsigned long ExpireTime;
}DOT11_SET_EXPIREDTIME;

typedef struct _DOT11_QUERY_STATS{
	unsigned char   EventId;
	unsigned char   IsMoreEvent;
	unsigned char	MACAddr[MACADDRLEN];
	unsigned long   IsSuccess;
	unsigned long	tx_packets;       // == transmited packets
	unsigned long	rx_packets;       // == received packets
	unsigned long	tx_bytes;         // == transmited bytes
	unsigned long	rx_bytes;         // == received bytes
}DOT11_QUERY_STATS;

typedef struct _DOT11_EAP_PACKET{
	unsigned char	EventId;
	unsigned char	IsMoreEvent;
	unsigned short  packet_len;
	unsigned char	packet[1550];
}DOT11_EAP_PACKET;

typedef DOT11_ASSOCIATION_IND DOT11_AUTH_IND;

#ifdef WIFI_SIMPLE_CONFIG
typedef struct _DOT11_WSC_PIN_IND{
	unsigned char EventId;
	unsigned char IsMoreEvent;
	unsigned char code[256];
} DOT11_WSC_PIN_IND;
#endif

#define DOT11_AI_REQFI_CAPABILITIES      1
#define DOT11_AI_REQFI_LISTENINTERVAL    2
#define DOT11_AI_REQFI_CURRENTAPADDRESS  4

#define DOT11_AI_RESFI_CAPABILITIES      1
#define DOT11_AI_RESFI_STATUSCODE        2
#define DOT11_AI_RESFI_ASSOCIATIONID     4

typedef struct _DOT11_ASSOCIATION_INFO
{
    unsigned char   EventId;
    unsigned char   IsMoreEvent;
    unsigned char   SupplicantAddress[MACADDRLEN];
    UINT32 Length;
    UINT16 AvailableRequestFixedIEs;
    struct _DOT11_AI_REQFI {
                UINT16 Capabilities;
                UINT16 ListenInterval;
        	char    CurrentAPAddress[MACADDRLEN];
    } RequestFixedIEs;
    UINT32 RequestIELength;
    UINT32 OffsetRequestIEs;
    UINT16 AvailableResponseFixedIEs;
    struct _DOT11_AI_RESFI {
                UINT16 Capabilities;
                UINT16 StatusCode;
                UINT16 AssociationId;
    } ResponseFixedIEs;
    UINT32 ResponseIELength;
    UINT32 OffsetResponseIEs;
} DOT11_ASSOCIATION_INFO, *PDOT11_ASSOCIATION_INFO;

typedef struct _DOT11_INIT_QUEUE
{
    unsigned char EventId;
    unsigned char IsMoreEvent;
} DOT11_INIT_QUEUE, *PDOT11_INIT_QUEUE;


//*------The following are defined to handle the event Queue for security event--------*/
//    For Event Queue related function
void DOT11_InitQueue(DOT11_QUEUE *q);
#ifndef WITHOUT_ENQUEUE
int DOT11_EnQueue(unsigned long task_priv, DOT11_QUEUE *q, unsigned char *item, int itemsize);
int DOT11_DeQueue(unsigned long task_priv, DOT11_QUEUE *q, unsigned char *item, int *itemsize);
#endif
void DOT11_PrintQueue(DOT11_QUEUE *q);
char *DOT11_ErrMsgQueue(int err);
#define DOT11_IsEmptyQueue(q) (q->NumItem==0 ? 1:0)
#define DOT11_IsFullQueue(q) (q->NumItem==q->MaxItem? 1:0)
#define DOT11_NumItemQueue(q) q->NumItem


typedef enum{
	ERROR_BUFFER_TOO_SMALL = -1,
	ERROR_INVALID_PARA = -2,
	ERROR_INVALID_RSNIE = -13,
	ERROR_INVALID_MULTICASTCIPHER = -18,
	ERROR_INVALID_UNICASTCIPHER = -19,
	ERROR_INVALID_AUTHKEYMANAGE = -20,
	ERROR_UNSUPPORTED_RSNEVERSION = -21,
	ERROR_INVALID_CAPABILITIES = -22
} INFO_ERROR;

#define RSN_STRERROR_BUFFER_TOO_SMALL			"Input Buffer too small"
#define RSN_STRERROR_INVALID_PARAMETER			"Invalid RSNIE Parameter"
#define RSN_STRERROR_INVALID_RSNIE				"Invalid RSNIE"
#define RSN_STRERROR_INVALID_MULTICASTCIPHER	"Multicast Cipher is not valid"
#define RSN_STRERROR_INVALID_UNICASTCIPHER		"Unicast Cipher is not valid"
#define RSN_STRERROR_INVALID_AUTHKEYMANAGE		"Authentication Key Management Protocol is not valid"
#define RSN_STRERROR_UNSUPPORTED_RSNEVERSION	"Unsupported RSNE version"
#define RSN_STRERROR_INVALID_CAPABILITIES		"Invalid RSNE Capabilities"

#define DOT11_s2n(s,c)   	(*((c))=(unsigned char)(((s)>> 8)&0xff), \
                         	*((c)+1)=(unsigned char)(((s)    )&0xff))

#define DOT11_n2s(c,s)   	(s =((unsigned short)(*((c))))<< 8, \
                          	s|=((unsigned short)(*((c)+1))))

#define DOT11_lc2s(bc,s)   	(s = ((unsigned short)(*((bc)+1)))<< 8, \
                          	s |= ((unsigned short)(*((bc)))))


void DOT11_Dump(char *fun, UINT8 *buf, int size, char *comment);

typedef enum _COUNTERMEASURE_TEST
{
	TEST_TYPE_PAIRWISE_ERROR = 0,
	TEST_TYPE_GROUP_ERROR = 1,
	TEST_TYPE_SEND_BAD_UNICAST_PACKET = 2,
	TEST_TYPE_SEND_BAD_BROADCAST_PACKET = 3
} COUNTERMEASURE_TEST;

#define	MIC_TIMER_PERIOD	100*60	//unit: 10 milli-seconds
#define REJECT_ASSOC_PERIOD	100*60


//*---------- The followings are for processing of RSN Information Element------------*/
#define RSN_ELEMENT_ID					0xDD
#define RSN_VER1						0x01
#define DOT11_MAX_CIPHER_ALGORITHMS		0x0a
#define DOT11_GROUPFLAG					0x02
#define DOT11_REPLAYBITSSHIFT			2
#define	DOT11_REPLAYBITS				3
#define IsPairwiseUsingDefaultKey(Cap)	((Cap[0] & DOT11_GROUPFLAG)?TRUE:FALSE)
#define IsPreAuthentication(Cap)		((Cap[0] & 0x01)?TRUE:FALSE)
#define DOT11_GetNumOfRxTSC(Cap)		(2<<((Cap[0] >> DOT11_REPLAYBITSSHIFT) & DOT11_REPLAYBITS))


typedef struct _DOT11_RSN_IE_HEADER {
	UINT8	ElementID;
	UINT8	Length;
	UINT8   OUI[4];
	UINT16	Version;
}DOT11_RSN_IE_HEADER;


typedef struct _DOT11_RSN_IE_SUITE{
	UINT8	OUI[3];
	UINT8	Type;
}DOT11_RSN_IE_SUITE;


typedef struct _DOT11_RSN_IE_COUNT_SUITE{

	UINT16	SuiteCount;
	DOT11_RSN_IE_SUITE	dot11RSNIESuite[DOT11_MAX_CIPHER_ALGORITHMS];
}DOT11_RSN_IE_COUNT_SUITE, *PDOT11_RSN_IE_COUNT_SUITE;

typedef	union _DOT11_RSN_CAPABILITY{

	UINT16	shortData;
	UINT8	charData[2];

#ifdef RTL_WPA2
	struct
	{
		unsigned short Reserved1:2; // B7 B6
		unsigned short GtksaReplayCounter:2; // B5 B4
		unsigned short PtksaReplayCounter:2; // B3 B2
		unsigned short NoPairwise:1; // B1
		unsigned short PreAuthentication:1; // B0
		unsigned short Reserved2:8;
	}field;
#else
	struct
	{
		unsigned short PreAuthentication:1;
		unsigned short PairwiseAsDefaultKey:1;
		unsigned short NumOfReplayCounter:2;
		unsigned short Reserved:12;
	}field;
#endif

}DOT11_RSN_CAPABILITY;

#endif // _8190N_SECURITY_H_

