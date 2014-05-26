#if defined (RTL8192S_WAPI_SUPPORT)

#include "wapi.h"
#include "wapi_interface.h"  
#include "rtllib.h"


/*************************************************************
 * WAPI EVENT QUEUE : 										 *
 * Buffer events from driver to application.            					 * 
 *************************************************************/
void WAPI_InitQueue(WAPI_QUEUE * q, int szMaxItem, int szMaxData)
{
	RT_ASSERT_RET(q);
	
	q->Head = 0;
	q->Tail = 0;
	q->NumItem = 0;
	q->MaxItem = szMaxItem;
	q->MaxData = szMaxData;
}

int WAPI_EnQueue(spinlock_t *plock, WAPI_QUEUE *q, u8 *item, int itemsize)
{
	unsigned long flags;

	RT_ASSERT_RET_VALUE(plock, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(q, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(item, (-E_WAPI_QNULL));
	
	if(WAPI_IsFullQueue(q))
		return -E_WAPI_QFULL;
	if(itemsize > q->MaxData)
		return -E_WAPI_ITEM_TOO_LARGE;

 	spin_lock_irqsave(plock, flags);

	q->ItemArray[q->Tail].ItemSize = itemsize;
	memset(q->ItemArray[q->Tail].Item, 0, sizeof(q->ItemArray[q->Tail].Item));
	memcpy(q->ItemArray[q->Tail].Item, item, itemsize);
	q->NumItem++;
	if((q->Tail+1) == q->MaxItem)
		q->Tail = 0;
	else
		q->Tail++;

	spin_unlock_irqrestore(plock, flags);
	
	return E_WAPI_OK;
}


int WAPI_DeQueue(spinlock_t *plock, WAPI_QUEUE *q, u8 *item, int *itemsize)
{
	unsigned long flags;

	RT_ASSERT_RET_VALUE(plock, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(q, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(item, (-E_WAPI_QNULL));
	
	if(WAPI_IsEmptyQueue(q))
		return -E_WAPI_QEMPTY;

 	spin_lock_irqsave(plock, flags);
	
	memcpy(item, q->ItemArray[q->Head].Item, q->ItemArray[q->Head].ItemSize);
	*itemsize = q->ItemArray[q->Head].ItemSize;
	q->NumItem--;
	if((q->Head+1) == q->MaxItem)
		q->Head = 0;
	else
		q->Head++;

	spin_unlock_irqrestore(plock, flags);
	
	return E_WAPI_OK;
}

void WAPI_PrintQueue(WAPI_QUEUE *q)
{
	int i, j, index;

	RT_ASSERT_RET(q);
	
	printk("\n/-------------------------------------------------\n");
	printk("[DOT11_PrintQueue]: MaxItem = %d, NumItem = %d, Head = %d, Tail = %d\n", q->MaxItem, q->NumItem, q->Head, q->Tail);
	for(i=0; i<q->NumItem; i++) {
		index = (i + q->Head) % q->MaxItem;
		printk("Queue[%d].ItemSize = %d  ", index, q->ItemArray[index].ItemSize);
		for(j=0; j<q->ItemArray[index].ItemSize; j++)
			printk(" %x", q->ItemArray[index].Item[j]);
		printk("\n");
	}
	printk("------------------------------------------------/\n");
}

int pid_wapi = 0;
void notifyWapiApplication()
{ 
	struct task_struct *p;
        
	if(pid_wapi != 0){

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
		p = find_task_by_pid(pid_wapi);
#else
                p = find_task_by_vpid(pid_wapi);
#endif
		if(p){
			send_sig(SIGUSR1,p,0); 
		}else {
			pid_wapi = 0;
		}
	}
}

int WAPI_CreateEvent_Send(struct rtllib_device *ieee, u8 EventId, u8 *MacAddr, u8 *Buff, u16 BufLen)
{
	WAPI_EVENT_T *pEvent;
	u8 *pbuf = NULL;
	int ret = 0;
	
	WAPI_TRACE(WAPI_API, "==========> %s: EventId=%d\n", __FUNCTION__, EventId);
	
	RT_ASSERT_RET_VALUE(ieee, -1);
	RT_ASSERT_RET_VALUE(MacAddr, -1);

	pbuf= (u8 *)kmalloc((sizeof(WAPI_EVENT_T) + BufLen), GFP_ATOMIC);
	if(NULL == pbuf)
	    return -1;

	pEvent = (WAPI_EVENT_T *)pbuf;	
	pEvent->EventId = EventId;
	memcpy(pEvent->MACAddr, MacAddr, ETH_ALEN);
	pEvent->BuffLength = BufLen;
	if(BufLen > 0){
		memcpy(pEvent->Buff, Buff, BufLen);
	}

	ret = WAPI_EnQueue(&ieee->wapi_queue_lock, ieee->wapi_queue, pbuf, (sizeof(WAPI_EVENT_T) + BufLen));
	notifyWapiApplication();

	if(pbuf)
		kfree(pbuf);

	WAPI_TRACE(WAPI_API, "<========== %s\n", __FUNCTION__);
	return ret;
}

#endif

