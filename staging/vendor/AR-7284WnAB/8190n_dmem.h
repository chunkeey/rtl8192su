/*
* Copyright c                  Realtek Semiconductor Corporation, 2007
* All rights reserved.
*
* Program : Header file of D-MEM supporting module for RTL8190 802.11N wireless NIC on RTL865x platform
* Abstract :
* Creator : Yi-Lun Chen (chenyl@realtek.com.tw)
* Author :  Yi-Lun Chen (chenyl@realtek.com.tw)
* $Id: 8190n_dmem.h,v 1.5 2009/08/06 11:41:28 yachang Exp $
*/

#ifndef _8190N_DMEM_H
#define _8190N_DMEM_H

/* ========================= External ========================= */

enum _RTL8190_DMEM_ITEM_ID {
	_RTL8190_DMEM_ITEM_MIN,
	/* ============================== Add here ============================== */
	AID_OBJ,
	PMIB,
	/* =================================================================== */
	_RTL8190_DMEM_ITEM_MAX,
};

void rtl8190_dmem_init( void );
void *rtl8190_dmem_alloc( enum _RTL8190_DMEM_ITEM_ID id, void *miscInfo );
void rtl8190_dmem_free( enum _RTL8190_DMEM_ITEM_ID id, void *miscInfo );


/* ========================= Internal ========================= */
typedef void* (*_dummyFunc_voidStar_voidStar)(void*);
typedef void (*_dummyFunc_void_voidStar)(void*);
typedef void (*_dummyFunc_void_void)(void);
typedef struct _rtl8190_dmem_list_s
{
	int		id;
	void *	initCallBackFunc;
	void *	allcateCallBackFunc;
	void *	freeCallBackFunc;
} _rtl8190_dmem_callBack_t;

#endif /* _8190N_DMEM_H */
