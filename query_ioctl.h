#ifndef QUERY_IOCTL_H
#define QUERY_IOCTL_H
#include <linux/ioctl.h>
#include <linux/types.h>

#define BUFFER_SIZE 1000
#define VBOX_POC_SEND_MSG _IOW(13371337, 1337, struct poc_msg *)

struct poc_msg {
  __u32 length;
  char buffer[BUFFER_SIZE];
};

#endif
