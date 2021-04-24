#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include "hgcm.h"
#include "chromium.h"


int alloc_buf(int client,int size,const void *msg,int msg_len) {
   int rc = hgcm_call(client,SHCRGL_GUEST_FN_WRITE_BUFFER,"%u%u%u%b",0,size,0,"in",msg,msg_len);
   if (rc) {
      die("[-] alloc_buf error");
   }
   return ans_buf[0];
}

int write_buf(int client,int buf_id,int size,int offset,const void *msg,int msg_len) {
   int rc = hgcm_call(client,SHCRGL_GUEST_FN_WRITE_BUFFER,"%u%u%u%b",buf_id,size,offset,"in",msg,msg_len);
   if (rc) {
      die("[-] write_buf error");
   }
   return ans_buf[0];
}

int write_buf_test(int client,int buf_id,int size) {
   int rc = hgcm_call(client,SHCRGL_GUEST_FN_WRITE_BUFFER,"%u%u%u%b",buf_id,size,0,"in","test",5);
   if (rc) {
      return 1;
   }
   return 0;
}

int read_hostbuf(int client,int size,void *buf) {
   int rc = hgcm_call(client,SHCRGL_GUEST_FN_READ,"%b%u","out",buf,size,size);
   if (rc) {
      die("[-] read_hostbuf error");
   }
   return ans_buf[1];
}

int set_version(int client) {
   int rc = hgcm_call(client,SHCRGL_GUEST_FN_SET_VERSION,"%u%u",CR_PROTOCOL_VERSION_MAJOR,CR_PROTOCOL_VERSION_MINOR);
   if (rc) {
      die("[-] set_version error");
   }
   return 0;
}

int crmsg_with_bufid(int client,int buf_id) {
   int rc = hgcm_call(client,SHCRGL_GUEST_FN_WRITE_READ_BUFFERED,"%u%b%u",buf_id,"out",crmsg_buf,0x1000,0x1000);
   if (rc) {
      die("[-] crmsg error");
   }
   return 0;
}

int crmsg(int client,int size,const void *msg,int msg_len) {
   int buf_id = alloc_buf(client,size,msg,msg_len);
   return crmsg_with_bufid(client,buf_id);
}

