#ifndef RTL_RFKILL_H
#define RTL_RFKILL_H

#ifdef CONFIG_RTL_RFKILL 
struct net_device;

bool rtl8192_rfkill_init(struct net_device *dev);
void rtl8192_rfkill_poll(struct net_device *dev);
void rtl8192_rfkill_exit(struct net_device *dev);
#endif

#endif /* RTL_RFKILL_H */
