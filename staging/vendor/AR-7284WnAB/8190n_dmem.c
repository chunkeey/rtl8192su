/*
* Copyright c                  Realtek Semiconductor Corporation, 2007
* All rights reserved.
*
* Program : D-MEM supporting module for RTL8190 802.11N wireless NIC on RTL865x platform
* Abstract :
* Creator : Yi-Lun Chen (chenyl@realtek.com.tw)
* Author :  Yi-Lun Chen (chenyl@realtek.com.tw)
* $Id: 8190n_dmem.c,v 1.6 2009/08/06 11:41:28 yachang Exp $
*/

#include "./8190n.h"
//#include "./8190n_fastExtDev.h"
#include "./8190n_cfg.h"

#ifdef RTL8190_VARIABLE_USED_DMEM
#include "./8190n_dmem.h"

#define RTL8190_MAX_SPEEDUP_STA			2
#define RTL8190_SPEEDUP_PRIV_COUNT		1

/* ========================== All variables using D-MEM ========================== */
static void rtl8190_dmem_AID_OBJ_init(void);
static void *rtl8190_dmem_AID_OBJ_alloc(void *miscInfo);
static void rtl8190_dmem_AID_OBJ_free(void *miscInfo);

static void rtl8190_dmem_pmib_init(void);
static void *rtl8190_dmem_pmib_alloc(void *miscInfo);
static void rtl8190_dmem_pmib_free(void *miscInfo);

#ifdef PRIV_STA_BUF
	extern struct aid_obj *alloc_sta_obj(void);
	extern void free_sta_obj(struct rtl8190_priv *priv, struct aid_obj *obj);
#endif

static _rtl8190_dmem_callBack_t _8190_dmem_callBack_list[] =
{
		/*		ID			Init CallBack				Allocate CallBack				Free CallBack */
		{		AID_OBJ,	rtl8190_dmem_AID_OBJ_init,	rtl8190_dmem_AID_OBJ_alloc,		rtl8190_dmem_AID_OBJ_free},
		{		PMIB,		rtl8190_dmem_pmib_init,		rtl8190_dmem_pmib_alloc,		rtl8190_dmem_pmib_free},
		/* ==================================================================== */
		{		_RTL8190_DMEM_ITEM_MAX,	NULL,								NULL},
};

/* ========================== External APIs of D-MEM module ========================== */

/*
	Initiation function for DMEM library
*/
void rtl8190_dmem_init( void )
{
	_rtl8190_dmem_callBack_t *ptr;

	ptr = &_8190_dmem_callBack_list[0];

	while (	(ptr->id > _RTL8190_DMEM_ITEM_MIN) &&
			(ptr->id < _RTL8190_DMEM_ITEM_MAX))
	{
		/* Call the Callback function to decide the memory of allocated */
		if (ptr->initCallBackFunc)
		{
			((_dummyFunc_void_void)(ptr->initCallBackFunc))();
		}

		/* Next Entry */
		ptr ++;
	}
}

void *rtl8190_dmem_alloc( enum _RTL8190_DMEM_ITEM_ID id, void *miscInfo )
{
	void *retval;
	_rtl8190_dmem_callBack_t *ptr;

	retval = NULL;

	if (	(id <= _RTL8190_DMEM_ITEM_MIN) ||
		(id >= _RTL8190_DMEM_ITEM_MAX))
	{
		printk("%s %d : ERROR (%d)\n", __FUNCTION__, __LINE__, id);
		goto out;
	}

	ptr = &_8190_dmem_callBack_list[0];

	while ( ptr->allcateCallBackFunc )
	{
		if ( ptr->id == id )
		{
			/* Call the Callback function to decide the memory of allocated */
			retval = ((_dummyFunc_voidStar_voidStar)(ptr->allcateCallBackFunc))(miscInfo);
			goto out;
		}
		/* Next Entry */
		ptr ++;
	}

out:
	return retval;
}

void rtl8190_dmem_free( enum _RTL8190_DMEM_ITEM_ID id, void *miscInfo )
{
	_rtl8190_dmem_callBack_t *ptr;

	if (	(id <= _RTL8190_DMEM_ITEM_MIN) ||
		(id >= _RTL8190_DMEM_ITEM_MAX))
	{
		printk("%s %d : ERROR (%d)\n", __FUNCTION__, __LINE__, id);
		goto out;
	}

	ptr = &_8190_dmem_callBack_list[0];

	while ( ptr->freeCallBackFunc )
	{
		if ( ptr->id == id )
		{
			/* Call the Callback function to decide the memory of allocated */
			((_dummyFunc_void_voidStar)(ptr->freeCallBackFunc))(miscInfo);
			goto out;
		}
		/* Next Entry */
		ptr ++;
	}

out:
	return;
}


/* ========================== Internal APIs for per-variable of D-MEM module ========================== */

/* ==============================================
  *
  *		AID_OBJ
  *
  *
  * ============================================== */
__DRAM_FASTEXTDEV struct aid_obj _rtl8190_aid_Array[RTL8190_MAX_SPEEDUP_STA];
void *_rtl8190_aid_externalMem_Array[NUM_STAT];

static void rtl8190_dmem_AID_OBJ_init(void)
{
	memset(_rtl8190_aid_Array, 0, sizeof(struct aid_obj) * RTL8190_MAX_SPEEDUP_STA);
	memset(_rtl8190_aid_externalMem_Array, 0, sizeof(_rtl8190_aid_externalMem_Array));
}

static void *rtl8190_dmem_AID_OBJ_alloc(void *miscInfo)
{
	/* For AID_OBJ : miscInfo would be [unsigned int *] to decision the index of aidarray to allocate */
	unsigned int index = *((unsigned int*)miscInfo);

	if (	(index < 0) ||
		(index >= NUM_STAT))
	{
		printk("%s %d : ERROR ( Index : %d )\n", __FUNCTION__, __LINE__, index);
		return NULL;
	}

	/* Allocate from external memory */
	if ( index >= RTL8190_MAX_SPEEDUP_STA )
	{
#ifdef PRIV_STA_BUF
		_rtl8190_aid_externalMem_Array[index] = alloc_sta_obj();
#else
		_rtl8190_aid_externalMem_Array[index] = kmalloc(sizeof(struct aid_obj), GFP_ATOMIC);
#endif
		if (_rtl8190_aid_externalMem_Array[index] == NULL)
		{
			printk("%s %d : Error : Allocation FAILED!\n", __FUNCTION__, __LINE__);
			return NULL;
		}
		return _rtl8190_aid_externalMem_Array[index];
	}

	memset(&(_rtl8190_aid_Array[index]), 0, sizeof(struct aid_obj));

	return (void*)(&(_rtl8190_aid_Array[index]));
}

static void rtl8190_dmem_AID_OBJ_free(void *miscInfo)
{
	/* For AID_OBJ : miscInfo would be [unsigned int *] to decision the index of aidarray to free */
	unsigned int index = *((unsigned int*)miscInfo);

	if (	(index < 0) ||
		(index >= NUM_STAT))
	{
		printk("%s %d : ERROR ( Index : %d )\n", __FUNCTION__, __LINE__, index);
		return;
	}

	/* Free memory to external memory module */
	if ( index >= RTL8190_MAX_SPEEDUP_STA )
	{
		if ( _rtl8190_aid_externalMem_Array[index] )
		{
#ifdef PRIV_STA_BUF
			free_sta_obj(NULL, _rtl8190_aid_externalMem_Array[index]);
#else
			kfree(_rtl8190_aid_externalMem_Array[index]);
#endif
			_rtl8190_aid_externalMem_Array[index] = NULL;
		}

		return;
	}

	memset(&(_rtl8190_aid_Array[index]), 0, sizeof(struct aid_obj));
}


/* =================== The following variable are mapped to PRIV =================== */

/* ==============================================
  *
  *		PMIB
  *
  *
  * ============================================== */
__DRAM_FASTEXTDEV struct wifi_mib _rtl8190_pmib[RTL8190_SPEEDUP_PRIV_COUNT];
int _rtl8190_pmib_usageMap[RTL8190_SPEEDUP_PRIV_COUNT];

static void rtl8190_dmem_pmib_init(void)
{
	memset(_rtl8190_pmib_usageMap, 0, sizeof(int) * RTL8190_SPEEDUP_PRIV_COUNT);
	memset(_rtl8190_pmib, 0, sizeof(struct wifi_mib) * RTL8190_SPEEDUP_PRIV_COUNT);
}

static void *rtl8190_dmem_pmib_alloc(void *miscInfo)
{
	int idx ;

	/* miscInfo is useless */
	for ( idx = 0 ; idx < RTL8190_SPEEDUP_PRIV_COUNT ; idx ++ )
	{
		if ( _rtl8190_pmib_usageMap[idx] == 0 )
		{	/* Unused entry : use it */
			_rtl8190_pmib_usageMap[idx] = 1;
			memset(&(_rtl8190_pmib[idx]), 0, sizeof(struct wifi_mib));
			return &(_rtl8190_pmib[idx]);
		}
	}

	/* Allocate from externel memory if speedup PMIB is exhausted */
	return kmalloc(sizeof(struct wifi_mib), GFP_KERNEL);

}

static void rtl8190_dmem_pmib_free(void *miscInfo)
{
	int idx;

	/* miscInfo is pointed to the address of PMIB to free */

	/* Free PMIB if it is speeded up by DMEM */
	for ( idx = 0 ; idx < RTL8190_SPEEDUP_PRIV_COUNT ; idx ++ )
	{
		if ( (unsigned int)(&(_rtl8190_pmib[idx])) == (unsigned int)miscInfo )
		{	/* Entry is found : free it */
			memset(&(_rtl8190_pmib[idx]), 0, sizeof(struct wifi_mib));
			_rtl8190_pmib_usageMap[idx] = 0;
			return;
		}
	}

	/* It would be allocated from external memory: kfree it */
	kfree(miscInfo);

}
#endif // RTL8190_VARIABLE_USED_DMEM

