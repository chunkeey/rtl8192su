/*
 *	    Headler file defines some data structure and macro of bridge extention
 *
 *	    $Id: 8190n_br_ext.h,v 1.6 2009/08/06 11:41:28 yachang Exp $
 */

#ifndef _8190N_BR_EXT_H_
#define _8190N_BR_EXT_H_

#define NAT25_HASH_BITS		4
#define NAT25_HASH_SIZE		(1 << NAT25_HASH_BITS)
#define NAT25_AGEING_TIME	300

struct nat25_network_db_entry
{
	struct nat25_network_db_entry	*next_hash;
	struct nat25_network_db_entry	**pprev_hash;
	atomic_t						use_count;
	unsigned char					macAddr[6];
	unsigned long					ageing_timer;
	unsigned char    				networkAddr[11];
};

enum NAT25_METHOD {
	NAT25_MIN,
	NAT25_CHECK,
	NAT25_INSERT,
	NAT25_LOOKUP,
	NAT25_PARSE,
	NAT25_MAX
};

#endif // _8190N_BR_EXT_H_

