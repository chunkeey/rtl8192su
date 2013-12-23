#define _8190N_FILE_C_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#endif


//Joshua
//sys_open can not be called by module, use this function instead.

int rtl8190n_fileopen(const char *filename, int flags, int mode){
        char *tmp = getname(filename);
        int fd = get_unused_fd();
        struct file *f = filp_open(tmp, flags, mode);
        fd_install(fd, f);
	printk("fd install \n");
        putname(tmp);
        return fd;

}

//EXPORT_SYMBOL(my_open);
