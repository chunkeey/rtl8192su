/*
 *      Debug headler file. It defines various print out method
 *
 *      $Id: 8190n_debug.h,v 1.8 2009/09/28 13:27:47 cathy Exp $
 */

#ifndef _8190N_DEBUG_H_
#define _8190N_DEBUG_H_

#ifdef CONFIG_RTK_MESH
#define	_MESH_MOD_	//plus add 0119
#define printMac(da)	printk("%02X:%02X:%02X:%02X:%02X:%02X   ",  0xff&*(da), 0xff&*(da+1), 0xff&*(da+2), 0xff&*(da+3), 0xff&*(da+4), 0xff&*(da+5));
#define printMac4(pframe) 		{\
		 printMac(GetAddr1Ptr(pframe));\
		 printMac(GetAddr2Ptr(pframe));\
		 printMac(GetAddr3Ptr(pframe));\
		 printMac(GetAddr4Ptr(pframe));\
}


#define printHex(d,n)		{int i; \
	for(i=0; i<n; i++) 	{  printk("%02X:", *(d+i)); \
		if( i%40==39) printk("\n "); \
	} }
	
#endif

#define DEBUG_MP(fmt, args...)		printk(fmt, ##args)
#define DEBUG_MP(fmt, args...)
#define _DEBUG_INFO(fmt, args...)

#ifdef _DEBUG_RTL8190_

extern unsigned long rtl8190_debug_err;		/* err flag */
extern unsigned long rtl8190_debug_info;	/* info flag */
extern unsigned long rtl8190_debug_trace;	/* trace flag */
extern unsigned long rtl8190_debug_warn;	/* warn flag */

/* Bit definition for bit31-bit8 of rtl8190_debug */
enum _module_define_ {
	_OSDEP_ =		0x00000001,
	_SME_ =			0x00000002,
	_IOCTL_ =		0x00000004,
	_TX_ =			0x00000008,
	_RX_ =			0x00000010,
	_HW_ =			0x00000020,
	_SECURITY_ =	0x00000040,
	_UTIL_ =		0x00000080,
	_TKIP_ =		0x00000100,
	_AES_ =			0x00000200,
	_CAM_ =			0x00000400,
	_BR_EXT_ =		0x00000800,
	_EEPROM_ =		0x00001000,
	_PSK_ =			0x00002000,
	_MP_ =			0x00004000,
	_MIB_=			0x00008000,
	_MESH_=			0x00010000,	//plus add 0119
};

#if defined(_8190N_OSDEP_C_)
	#define _MODULE_DEFINE	_OSDEP_
	#define _MODULE_NAME	"osdep"

#elif defined(_8190N_SME_C_)
	#define _MODULE_DEFINE _SME_
	#define _MODULE_NAME	"sme"

#elif defined(_8190N_IOCTL_C_)
	#define _MODULE_DEFINE _IOCTL_
	#define _MODULE_NAME	"ioctl"

#elif defined(_8190N_TX_C_)
	#define _MODULE_DEFINE _TX_
	#define _MODULE_NAME	"tx"

#elif defined(_8190N_RX_C_)
	#define _MODULE_DEFINE _RX_
	#define _MODULE_NAME	"rx"

#elif defined(_8190N_HW_C_)
	#define _MODULE_DEFINE _HW_
	#define _MODULE_NAME	"hw"

#elif defined(_8190N_SECURITY_C_)
	#define _MODULE_DEFINE _SECURITY_
	#define _MODULE_NAME	"security"

#elif defined(_8190N_UTILS_C_)
	#define _MODULE_DEFINE _UTIL_
	#define _MODULE_NAME	"util"

#elif defined(_8190N_TKIP_C_)
	#define _MODULE_DEFINE _TKIP_
	#define _MODULE_NAME	"tkip"

#elif defined(_8190N_AES_C_)
	#define _MODULE_DEFINE _AES_
	#define _MODULE_NAME	"aes"

#elif defined(_8190N_CAM_C_)
	#define _MODULE_DEFINE _CAM_
	#define _MODULE_NAME	"cam"

#elif defined(_8190N_BR_EXT_C_)
	#define _MODULE_DEFINE _BR_EXT_
	#define _MODULE_NAME	"br_ext"

#elif defined(_8190N_EEPROM_C_)
	#define _MODULE_DEFINE _EEPROM_
	#define _MODULE_NAME	"eeprom"

#elif defined(_8190N_PSK_C_)
	#define _MODULE_DEFINE _PSK_
	#define _MODULE_NAME	"psk"

#elif defined(_8190N_MP_C_)
	#define _MODULE_DEFINE _MP_
	#define _MODULE_NAME	"mp"

#elif defined(_8190N_MIB_C_)
	#define _MODULE_DEFINE _MIB_
	#define _MODULE_NAME	"mib"

#elif defined(_MESH_MOD_)	//plus add 0119
	#define _MODULE_DEFINE _MESH_
	#define _MODULE_NAME	"mesh"
#else
	#error "error, no debug module is specified!\n"
#endif

/* Macro for DEBUG_ERR(), DEBUG_TRACE(), DEBUG_WARN(), DEBUG_INFO() */

#ifdef __GNUC__
#ifdef CONFIG_RTL8671
#define DEBUG_ERR		printk
#define DEBUG_TRACE		printk
#define DEBUG_INFO		printk
#define DEBUG_WARN		printk

#define _DEBUG_ERR		printk
#define _DEBUG_INFO		printk

#define DBFENTER
#define DBFEXIT
#define PRINT_INFO		printk

#else

#define __DEBUG_ERR(name, fmt, args...) \
	if (rtl8190_debug_err&_MODULE_DEFINE) \
		printk("%s-"_MODULE_NAME"-err: " fmt, name, ## args);
#define __DEBUG_TRACE(name) \
	if (rtl8190_debug_trace&_MODULE_DEFINE) \
		printk("%s-"_MODULE_NAME"-trace: %s----->\n", name, (char *)__FUNCTION__);
#define __DEBUG_INFO(name, fmt, args...) \
	if (rtl8190_debug_info&_MODULE_DEFINE) \
		printk("%s-"_MODULE_NAME"-info: " fmt, name, ## args);
#define __DEBUG_WARN(name, fmt, args...) \
	if (rtl8190_debug_warn&_MODULE_DEFINE) \
		printk("%s-"_MODULE_NAME"-warn: " fmt, name, ## args);

#define DEBUG_ERR(fmt, args...)		__DEBUG_ERR(priv->dev->name, fmt, ## args)
#define DEBUG_INFO(fmt, args...)	__DEBUG_INFO(priv->dev->name, fmt, ## args)
#define DEBUG_TRACE					__DEBUG_TRACE(priv->dev->name)
#define DEBUG_WARN(fmt, args...)	__DEBUG_WARN(priv->dev->name, fmt, ## args)

#define _DEBUG_ERR(fmt, args...)	__DEBUG_ERR("wlan", fmt, ## args)
#define _DEBUG_INFO(fmt, args...)	__DEBUG_INFO("wlan", fmt, ## args)
#define _DEBUG_TRACE				__DEBUG_TRACE("wlan")
#define _DEBUG_WARN(fmt, args...)	__DEBUG_WARN("wlan", fmt, ## args)

#define DBFENTER	printk("----->%s\n", (char *)__FUNCTION__)
#define DBFEXIT		printk("%s----->\n", (char *)__FUNCTION__)
#define PRINT_INFO(fmt, args...)	printk(fmt, ## args)
#endif
#endif	// __GNUC__

#ifdef __DRAYTEK_OS__
#define __FUNCTION__	""

#define DEBUG_ERR		Print
#define DEBUG_INFO		Print
#define DEBUG_TRACE
#define DEBUG_WARN		Print

#define _DEBUG_ERR		DEBUG_ERR
#define _DEBUG_INFO		DEBUG_INFO
#define _DEBUG_TRACE	DEBUG_TRACE
#define _DEBUG_WARN		DEBUG_WARN

#define DBFENTER
#define DBFEXIT
#define PRINT_INFO		Print
#endif // __DRAYTEK_OS__

#ifdef GREEN_HILL
#define DEBUG_ERR		printk
#define DEBUG_INFO		printk
#define DEBUG_TRACE		printk
#define DEBUG_WARN		printk

#define _DEBUG_ERR		printk
#define _DEBUG_INFO		printk
#define _DEBUG_TRACE	printk
#define _DEBUG_WARN		printk

#define DBFENTER		printk
#define DBFEXIT			printk
#define PRINT_INFO		printk
#endif // GREEN_HILL

#elif defined(RTL8192SU)
/* Macro for DEBUG_ERR(), DEBUG_TRACE(), DEBUG_WARN(), DEBUG_INFO() */

#ifdef CONFIG_ENABLE_MIPS16 //tysu: mips16 call printk directly will cause crash-message show on console.
#define DEBUG_ERR(fmt, args...)		do { static char *print_buf=fmt;  printk(print_buf); } while(0);  //for toolchain 1.2.8 

#else

#define DEBUG_ERR(fmt, args...)		printk(fmt, ##args)
//#define DEBUG_ERR(fmt, args...)
#endif

//#define DEBUG_INFO(fmt, args...)		printk(fmt, ##args)
#define DEBUG_INFO(fmt, args...)		
//#define DEBUG_TRACE				printk("TRACE: %s %s %d\n",__FILE__,__FUNCTION__,__LINE__)
#define DEBUG_TRACE
#define DEBUG_WARN(fmt, args...)
//#define DEBUG_WARN(fmt, args...)		printk(fmt, ##args)

#define _DEBUG_ERR(fmt, args...)		printk(fmt, ##args)
//#define _DEBUG_INFO(fmt, args...)		printk(fmt, ##args)
#define _DEBUG_INFO(fmt, args...)

#define DBFENTER
#define DBFEXIT
#define PRINT_INFO		//printk

#else // not _DEBUG_RTL8190_

#ifdef __GNUC__
#define DEBUG_ERR(fmt, args...) {}
#define DEBUG_INFO(fmt, args...) {}
#define DEBUG_TRACE {}
#define DEBUG_WARN(fmt, args...) {}

#define _DEBUG_ERR(fmt, args...) {}
#define _DEBUG_INFO(fmt, args...) {}
#define _DEBUG_TRACE {}
#define _DEBUG_WARN(fmt, args...) {}

#define DBFENTER	{}
#define DBFEXIT		{}
#define PRINT_INFO(fmt, args...)	{}
#endif // __GNUC__

#ifdef __DRAYTEK_OS__
#define __FUNCTION__	""

#define DEBUG_ERR
#define DEBUG_INFO
#define DEBUG_TRACE
#define DEBUG_WARN

#define _DEBUG_ERR
#define _DEBUG_INFO
#define _DEBUG_TRACE
#define _DEBUG_WARN

#define DBFENTER
#define DBFEXIT
#define PRINT_INFO
#endif // __DRAYTEK_OS__

#ifdef GREEN_HILL
#define DEBUG_ERR(fmt, args...) {}
#define DEBUG_INFO(fmt, args...) {}
#define DEBUG_TRACE {}
#define DEBUG_WARN(fmt, args...) {}

#define _DEBUG_ERR(fmt, args...) {}
#define _DEBUG_INFO(fmt, args...) {}
#define _DEBUG_TRACE {}
#define _DEBUG_WARN(fmt, args...) {}

#define DBFENTER	{}
#define DBFEXIT		{}
#define PRINT_INFO(fmt, args...)	{}
#endif // GREEN_HILL


#endif // _DEBUG_RTL8190_
#endif // _8190N_DEBUG_H_

