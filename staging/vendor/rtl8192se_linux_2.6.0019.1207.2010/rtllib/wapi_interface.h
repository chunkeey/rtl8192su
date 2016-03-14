#ifndef __INC_WAPI_INTERFACE_H
#define __INC_WAPI_INTERFACE_H
#include <linux/spinlock.h>
#include <linux/if_ether.h> /* ETH_ALEN */

/*************************************************************
 * WAPI EVENT QUEUE : 										 *
 * Buffer events from driver to application.            					 * 
 *************************************************************/
#define WAPI_MAX_BUFF_LEN		2000	
#define WAPI_MAX_QUEUE_LEN		6

typedef enum
{
	E_WAPI_OK = 0,
	E_WAPI_ITEM_TOO_LARGE = 1,
	E_WAPI_QFULL = 2,
	E_WAPI_QEMPTY = 3,
	E_WAPI_QNULL = 4		
}WAPI_QUEUE_RET_VAL;

typedef struct _WAPI_QUEUE_NODE
{
    u16  ItemSize;
    u8   Item[WAPI_MAX_BUFF_LEN];
}WAPI_QUEUE_NODE;

typedef struct _WAPI_QUEUE
{
    int     Head;
    int     Tail;
    int     NumItem;
    int     MaxItem;
    WAPI_QUEUE_NODE        ItemArray[WAPI_MAX_QUEUE_LEN];
    int	MaxData;
}WAPI_QUEUE;

typedef enum{
    WAPI_EVENT_RCV_PREAUTHENTICATE = 1, 
    WAPI_EVENT_RCV_STAKEY_REQUEST = 2,
    WAPI_EVENT_RCV_AUTHENTICATE_ACTIVE = 3,
    WAPI_EVENT_RCV_ACCESS_AUTHENTICATE_REQUEST = 4,
    WAPI_EVENT_RCV_ACCESS_AUTHENTICATE_RESPONSE = 5,
    WAPI_EVENT_RCV_CERTIFICATE_AUTHENTICATE_REQUEST = 6,
    WAPI_EVENT_RCV_CERTIFICATE_AUTHENTICATE_RESPONSE = 7,
    WAPI_EVENT_RCV_USK_REQUEST = 8,
    WAPI_EVENT_RCV_USK_RESPONSE = 9,                    
    WAPI_EVENT_RCV_USK_CONFIRM = 10,
    WAPI_EVENT_RCV_MSK_NOTIFICATION = 11,
    WAPI_EVENT_RCV_MSK_RESPONSE = 12,                   
    WAPI_EVENT_AE_UPDATE_BK = 13,
    WAPI_EVENT_AE_UPDATE_USK = 14,
    WAPI_EVENT_AE_UPDATE_MSK = 15,
    WAPI_EVENT_ASUE_UPDATE_BK = 16,
    WAPI_EVENT_ASUE_UPDATE_USK = 17,
    WAPI_EVENT_FIRST_AUTHENTICATOR = 18,
    WAPI_EVENT_CONNECT = 19,
    WAPI_EVENT_DISCONNECT = 20,
    WAPI_EVENT_MAX_NUM = 21
} WAPI_EVENT_ID;

typedef struct _WAPI_EVENT_T{
    u8	EventId;
    u8	MACAddr[ETH_ALEN];
    u16	BuffLength;
    u8  Buff[0];
}__attribute__ ((packed))WAPI_EVENT_T;

#define WAPI_IsEmptyQueue(q) (q->NumItem==0 ? 1:0)
#define WAPI_IsFullQueue(q) (q->NumItem==q->MaxItem? 1:0)
#define WAPI_NumItemQueue(q) q->NumItem

void WAPI_InitQueue(WAPI_QUEUE *q, int szMaxItem, int szMaxData);
int WAPI_EnQueue(spinlock_t *plock, WAPI_QUEUE *q, u8 *item, int itemsize);
int WAPI_DeQueue(spinlock_t *plock, WAPI_QUEUE *q, u8 *item, int *itemsize);
void WAPI_PrintQueue(WAPI_QUEUE *q);

extern int pid_wapi;
extern void notifyWapiApplication(void);
/*************************************************************
 * WAPI IOCTL: FROM APPLICATION TO DRIVER 					 *
 *************************************************************/
#define WAPI_CMD_GET_WAPI_SUPPORT 	0X8B81
#define WAPI_CMD_SET_WAPI_ENABLE 	0X8B82
#define WAPI_CMD_SET_WAPI_PSK	 	0X8B83
#define WAPI_CMD_SET_KEY 				0X8B84
#define WAPI_CMD_SET_MULTICAST_PN 	0X8B85
#define WAPI_CMD_GET_PN 				0X8B86
#define WAPI_CMD_GET_WAPIIE 			0X8B87
#define WAPI_CMD_SET_SSID				0X8B88
#define WAPI_CMD_GET_BSSID			0X8B89
#define WAPI_CMD_SET_IW_MODE			0X8B8b
#define WAPI_CMD_SET_DISASSOCIATE		0X8B8c
#define WAPI_CMD_SAVE_PID			0X8B8d
#define WAPI_CMD_DEQUEUE 				0X8B90

#endif 

