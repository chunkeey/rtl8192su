/******************************************************************************
 *
 * Copyright(c) 2009-2012  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 * Christian Lamparter <chunkeey@googlemail.com>
 *****************************************************************************/

#include <linux/moduleparam.h>
#include <linux/debugfs.h>
#include <linux/vmalloc.h>

#include "wifi.h"
#include "base.h"
#include "debug.h"

#ifdef CONFIG_RTLWIFI_DEBUG
void _rtl_dbg_trace(struct rtl_priv *rtlpriv, int comp, int level,
		    const char *fmt, ...)
{
	if (unlikely((comp & rtlpriv->cfg->mod_params->debug_mask) &&
		     (level <= rtlpriv->cfg->mod_params->debug_level))) {
		struct va_format vaf;
		va_list args;

		va_start(args, fmt);

		vaf.fmt = fmt;
		vaf.va = &args;

		pr_info(":<%lx> %pV", in_interrupt(), &vaf);

		va_end(args);
	}
}
EXPORT_SYMBOL_GPL(_rtl_dbg_trace);

void _rtl_dbg_print(struct rtl_priv *rtlpriv, u64 comp, int level,
		    const char *fmt, ...)
{
	if (unlikely((comp & rtlpriv->cfg->mod_params->debug_mask) &&
		     (level <= rtlpriv->cfg->mod_params->debug_level))) {
		struct va_format vaf;
		va_list args;

		va_start(args, fmt);

		vaf.fmt = fmt;
		vaf.va = &args;

		pr_info("%pV", &vaf);

		va_end(args);
	}
}
EXPORT_SYMBOL_GPL(_rtl_dbg_print);

void _rtl_dbg_print_data(struct rtl_priv *rtlpriv, u64 comp, int level,
			 const char *titlestring,
			 const void *hexdata, int hexdatalen)
{
	if (unlikely(((comp) & rtlpriv->cfg->mod_params->debug_mask) &&
		     ((level) <= rtlpriv->cfg->mod_params->debug_level))) {
		pr_info("In process \"%s\" (pid %i): %s\n",
			current->comm, current->pid, titlestring);
		print_hex_dump_bytes("", DUMP_PREFIX_NONE,
				     hexdata, hexdatalen);
	}
}
EXPORT_SYMBOL_GPL(_rtl_dbg_print_data);

#endif

#ifdef CONFIG_RTLWIFI_DEBUGFS

#define ADD(buf, off, max, fmt, args...)				\
	(off += snprintf(&buf[(off)], (max - off), (fmt), ##args))

struct rtl_debugfs_fops {
	unsigned int read_bufsize;
	umode_t attr;
	char *(*read)(struct ieee80211_hw *hw, char *buf, size_t bufsize,
		      ssize_t *len);
	ssize_t (*write)(struct ieee80211_hw *hw, const char *buf, size_t size);
	const struct file_operations fops;
};

static ssize_t rtl_debugfs_read(struct file *file, char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	struct rtl_debugfs_fops *dfops;
	struct ieee80211_hw *hw;
	struct rtl_priv *rtlpriv;
	char *buf = NULL, *res_buf = NULL;
	ssize_t ret = 0;
	int err = 0;

	if (!count)
		return 0;

	hw = file->private_data;

	if (!hw)
		return -ENODEV;
	rtlpriv = rtl_priv(hw);

	dfops = container_of(file->f_op, struct rtl_debugfs_fops, fops);

	if (!dfops->read)
		return -ENOENT;

	if (dfops->read_bufsize) {
		buf = vmalloc(dfops->read_bufsize);
		if (!buf)
			return -ENOMEM;
	}

	mutex_lock(&rtlpriv->locks.conf_mutex);

	res_buf = dfops->read(hw, buf, dfops->read_bufsize, &ret);

	if (ret > 0)
		err = simple_read_from_buffer(userbuf, count, ppos,
					      res_buf, ret);
	else
		err = ret;

	WARN_ONCE(dfops->read_bufsize && (res_buf != buf),
		  "failed to write output buffer back to debugfs");

	vfree(res_buf);
	mutex_unlock(&rtlpriv->locks.conf_mutex);
	return err;
}

static ssize_t rtl_debugfs_write(struct file *file,
	const char __user *userbuf, size_t count, loff_t *ppos)
{
	struct rtl_debugfs_fops *dfops;
	struct ieee80211_hw *hw;
	struct rtl_priv *rtlpriv;
	char *buf = NULL;
	int err = 0;

	if (!count)
		return 0;

	if (count > PAGE_SIZE)
		return -E2BIG;

	hw = file->private_data;

	if (!hw)
		return -ENODEV;

	rtlpriv = rtl_priv(hw);
	dfops = container_of(file->f_op, struct rtl_debugfs_fops, fops);

	if (!dfops->write)
		return -ENOENT;

	buf = vmalloc(count);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, userbuf, count)) {
		err = -EFAULT;
		goto out_free;
	}

	if (mutex_trylock(&rtlpriv->locks.conf_mutex) == 0) {
		err = -EAGAIN;
		goto out_free;
	}

	err = dfops->write(hw, buf, count);
	if (err)
		goto out_unlock;

out_unlock:
	mutex_unlock(&rtlpriv->locks.conf_mutex);

out_free:
	vfree(buf);
	return err;
}

#define __DEBUGFS_DECLARE_FILE(name, _read, _write, _read_bufsize,	\
			       _attr)					\
static const struct rtl_debugfs_fops rtl_debugfs_##name ##_ops = {	\
	.read_bufsize = _read_bufsize,					\
	.read = _read,							\
	.write = _write,						\
	.attr = _attr,							\
	.fops = {							\
		.open   = simple_open,					\
		.read   = rtl_debugfs_read,				\
		.write  = rtl_debugfs_write,				\
		.owner  = THIS_MODULE,					\
	},								\
}

#define DEBUGFS_DECLARE_FILE(name, _read, _write, _read_bufsize, _attr)	\
	__DEBUGFS_DECLARE_FILE(name, _read, _write, _read_bufsize,	\
			       _attr)

#define DEBUGFS_DECLARE_RO_FILE(name, _read_bufsize)			\
	DEBUGFS_DECLARE_FILE(name, rtl_debugfs_##name ##_read,	\
			     NULL, _read_bufsize, S_IRUSR)

#define DEBUGFS_DECLARE_WO_FILE(name)					\
	DEBUGFS_DECLARE_FILE(name, NULL, rtl_debugfs_##name ##_write,	\
			     0, S_IWUSR)

#define DEBUGFS_DECLARE_RW_FILE(name, _read_bufsize)			\
	DEBUGFS_DECLARE_FILE(name, rtl_debugfs_##name ##_read,	\
			     rtl_debugfs_##name ##_write,		\
			     _read_bufsize, S_IRUSR | S_IWUSR)

#define __DEBUGFS_DECLARE_RW_FILE(name, _read_bufsize, _dstate)		\
	__DEBUGFS_DECLARE_FILE(name, rtl_debugfs_##name ##_read,	\
			       rtl_debugfs_##name ##_write,		\
			       _read_bufsize, S_IRUSR | S_IWUSR, _dstate)

#define DEBUGFS_READONLY_FILE(name, _read_bufsize, fmt, value...)	\
static char *rtl_debugfs_ ##name ## _read(struct ieee80211_hw *hw,	\
					    char *buf, size_t buf_size,	\
					    ssize_t *len)		\
{									\
	ADD(buf, *len, buf_size, fmt "\n", ##value);			\
	return buf;							\
}									\
DEBUGFS_DECLARE_RO_FILE(name, _read_bufsize)

static const char *meminfo[__MAX_RTL_MEM_TYPE] = {
	[RTL_8] =  "byte",
	[RTL_16] = "word",
	[RTL_32] = " int"
};

static ssize_t rtl_debugfs_hw_ioread_write(struct ieee80211_hw *hw,
					     const char *buf, size_t count)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	int err = 0, n = 0, max_len = 32, res;
	unsigned int reg, tmp;
	enum rtl_mem_type_t type;

	if (!count)
		return 0;

	if (count > max_len)
		return -E2BIG;

	res = sscanf(buf, "0x%X %d", &reg, &n);
	if (res < 1) {
		err = -EINVAL;
		goto out;
	}

	if (res == 1)
		n = 1;

	switch (n) {
	case 4:
		tmp = rtl_read_dword(rtlpriv, reg);
		type = RTL_32;
		break;
	case 2:
		tmp = rtl_read_word(rtlpriv, reg);
		type = RTL_16;
		break;
	case 1:
		tmp = rtl_read_byte(rtlpriv, reg);
		type = RTL_8;
		break;
	default:
		err = -EMSGSIZE;
		goto out;
	}

	rtlpriv->dbg.ring[rtlpriv->dbg.ring_tail].reg = reg;
	rtlpriv->dbg.ring[rtlpriv->dbg.ring_tail].value = tmp;
	rtlpriv->dbg.ring[rtlpriv->dbg.ring_tail].type = type;
	rtlpriv->dbg.ring_tail++;
	rtlpriv->dbg.ring_tail %= RTL_DEBUG_RING_SIZE;

	if (rtlpriv->dbg.ring_len < RTL_DEBUG_RING_SIZE)
		rtlpriv->dbg.ring_len++;

out:
	return err ? err : count;
}

static char *rtl_debugfs_hw_ioread_read(struct ieee80211_hw *hw, char *buf,
					  size_t bufsize, ssize_t *ret)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	int i;

	ADD(buf, *ret, bufsize,
		"                      33222222 22221111 11111100 00000000\n");
	ADD(buf, *ret, bufsize,
		"                      10987654 32109876 54321098 76543210\n");

	while (rtlpriv->dbg.ring_len) {
		struct rtl_debug_mem_rbe *rbe;

		rbe = &rtlpriv->dbg.ring[rtlpriv->dbg.ring_head];
		ADD(buf, *ret, bufsize, "%.4x = %.8x [%s]",
		    rbe->reg, rbe->value, meminfo[rbe->type]);

		for (i = 31; i >= 0; i--) {
			ADD(buf, *ret, bufsize, "%c%s",
			    rbe->value & BIT(i) ? 'X' : ' ',
			    (i % 8) == 0 ? " " : "");
		}
		ADD(buf, *ret, bufsize, "\n");

		rtlpriv->dbg.ring_head++;
		rtlpriv->dbg.ring_head %= RTL_DEBUG_RING_SIZE;

		rtlpriv->dbg.ring_len--;
	}
	rtlpriv->dbg.ring_head = rtlpriv->dbg.ring_tail;
	return buf;
}
DEBUGFS_DECLARE_RW_FILE(hw_ioread, 160 + RTL_DEBUG_RING_SIZE * 40);

static ssize_t rtl_debugfs_hw_iowrite_write(struct ieee80211_hw *hw,
					      const char *buf, size_t count)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	int err = 0, max_len = 22, res;
	u32 reg, val, n;

	if (!count)
		return 0;

	if (count > max_len)
		return -E2BIG;

	res = sscanf(buf, "0x%X 0x%X %d", &reg, &val, &n);
	if (res != 3) {
		if (res != 2) {
			err = -EINVAL;
			goto out;
		}
		n = 1;
	}

	switch (n) {
	case 4:
		rtl_write_dword(rtlpriv, reg, val);
		err = 0;
		break;
	case 2:
		rtl_write_word(rtlpriv, reg, val);
		err = 0;
		break;
	case 1:
		rtl_write_byte(rtlpriv, reg, val);
		err = 0;
		break;
	default:
		err = -EINVAL;
		if (err)
			goto out;
		break;
	}

out:
	return err ? err : count;
}
DEBUGFS_DECLARE_WO_FILE(hw_iowrite);

int rtl_register_debugfs(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	rtlpriv->dbg.dfs = debugfs_create_dir(KBUILD_MODNAME,
		hw->wiphy->debugfsdir);

#define DEBUGFS_ADD(name)						\
	debugfs_create_file(#name, rtl_debugfs_##name ##_ops.attr,	\
			    rtlpriv->dbg.dfs, hw,			\
			    &rtl_debugfs_##name ## _ops.fops)

	if (!rtlpriv->dbg.dfs)
		return -EINVAL;

	DEBUGFS_ADD(hw_ioread);
	DEBUGFS_ADD(hw_iowrite);
	return 0;

#undef DEBUGFS_ADD
}
EXPORT_SYMBOL_GPL(rtl_register_debugfs);

void rtl_unregister_debugfs(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	debugfs_remove_recursive(rtlpriv->dbg.dfs);
	rtlpriv->dbg.dfs = NULL;
}
EXPORT_SYMBOL_GPL(rtl_unregister_debugfs);

#endif
