#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include "hgcm.h"

void die(char *msg) {
   perror(msg);
   exit(-1);
}


//device fd
int fd = -1;

int hgcm_connect(const char *service_name) {
   if (fd == -1) {
      fd = open("/dev/vboxuser",O_RDWR);
      if (fd == -1) {
         die("open device error");
      }
   }
   VBGLIOCHGCMCONNECT data = {
      .Hdr.cbIn = sizeof(VBGLIOCHGCMCONNECT),
      .Hdr.uVersion = VBGLREQHDR_VERSION,
      .Hdr.uType = VBGLREQHDR_TYPE_DEFAULT,
      .Hdr.rc = VERR_INTERNAL_ERROR,
      .Hdr.cbOut = sizeof(VBGLREQHDR) + sizeof(uint32_t),
      .Hdr.uReserved = 0,
      .u.In.Loc.type = VMMDevHGCMLoc_LocalHost_Existing
   };
   memset(data.u.In.Loc.u.host.achName,0,128);
   strncpy(data.u.In.Loc.u.host.achName,service_name,128);
   ioctl(fd,VBGL_IOCTL_HGCM_CONNECT,&data);
   if (data.Hdr.rc) { //error
      return -1;
   }
   return data.u.Out.idClient;
}

HGCMFunctionParameter64 arg_buf[0x100];

int hgcm_call(int client_id,int func,char *params_fmt,...) {
   va_list ap;
   char *p,*bval,*type;
   uint32_t ival;
   uint64_t lval;
   HGCMFunctionParameter64 params;
   uint16_t index = 0;

   va_start(ap,params_fmt);
   for(p = params_fmt;*p;p++) {
      if(*p!='%') {
         continue;
      }

      switch (*++p) {
         case 'u': //整数类型
            ival = va_arg(ap,uint32_t);
            params.type = VMMDevHGCMParmType_32bit;
            params.u.value64 = 0;
            params.u.value32 = ival;
            arg_buf[index++] = params;
            break;
         case 'l':
            lval = va_arg(ap,uint64_t);
            params.type = VMMDevHGCMParmType_64bit;
            params.u.value64 = lval;
            arg_buf[index++] = params;
         case 'b': //buffer类型
            type = va_arg(ap,char *);
            bval = va_arg(ap,char *);
            ival = va_arg(ap,uint32_t);
            if (!strcmp(type,"in")) {
               params.type = VMMDevHGCMParmType_LinAddr_In;
            } else if (!strcmp(type,"out")) {
               params.type = VMMDevHGCMParmType_LinAddr_Out;
            } else {
               params.type = VMMDevHGCMParmType_LinAddr;
            }
            params.u.Pointer.size = ival;
            params.u.Pointer.u.linearAddr = (uintptr_t)bval;
            arg_buf[index++] = params;
            break;
      }
   }
   va_end(ap);
   //printf("params count=%d\n",index);
   uint8_t *data_buf = (uint8_t *)malloc(sizeof(VBGLIOCHGCMCALL) + sizeof(HGCMFunctionParameter64)*index);
   VBGLIOCHGCMCALL data = {
      .Hdr.cbIn = sizeof(VBGLIOCHGCMCALL) + sizeof(HGCMFunctionParameter64)*index,
      .Hdr.uVersion = VBGLREQHDR_VERSION,
      .Hdr.uType = VBGLREQHDR_TYPE_DEFAULT,
      .Hdr.rc = VERR_INTERNAL_ERROR,
      .Hdr.cbOut = sizeof(VBGLIOCHGCMCALL) + sizeof(HGCMFunctionParameter64)*index,
      .Hdr.uReserved = 0,
      .u32ClientID = client_id,
      .u32Function = func,
      .cMsTimeout = 100000, //忽略
      .fInterruptible = 0,
      .bReserved = 0,
      .cParms = index
   };
   memcpy(data_buf,&data,sizeof(VBGLIOCHGCMCALL));
   memcpy(data_buf+sizeof(VBGLIOCHGCMCALL),arg_buf,sizeof(HGCMFunctionParameter64)*index);

   /*printf("=============before=====================\n");
   unsigned char *d = (unsigned char *)data_buf;
   for (int i=0;i<0x100;i++) { 
      printf("%02x ",d[i]);
   }
   printf("\n");
   printf("=============after=====================\n");*/

   ioctl(fd,VBGL_IOCTL_CODE_SIZE(IOCTL_HGCM_CALL,sizeof(VBGLIOCHGCMCALL) + sizeof(HGCMFunctionParameter64)*index),data_buf);
   int error = ((VBGLIOCHGCMCALL *)data_buf)->Hdr.rc;
   HGCMFunctionParameter64 *ans = (HGCMFunctionParameter64 *)(data_buf + sizeof(VBGLIOCHGCMCALL));
   /*d = (unsigned char *)data_buf;
   for (int i=0;i<0x100;i++) {
      printf("%02x ",d[i]);
   }
   printf("\n");*/
   for (int i=0;i<index;i++) {
      HGCMFunctionParameter64 e = ans[i];
      switch (e.type) {
         case VMMDevHGCMParmType_32bit:
            ans_buf[i] = e.u.value32;
            break;
         case VMMDevHGCMParmType_64bit:
            ans_buf[i] = e.u.value64;
            break;
         case VMMDevHGCMParmType_PhysAddr:
         case VMMDevHGCMParmType_LinAddr_Out:
            ans_buf[i] = e.u.Pointer.u.linearAddr;
            break;
      }
   }
   free(data_buf);

   if (error) { //error
      return error;
   }
   /*for (int i=0;i<sizeof(VBGLIOCHGCMCALL)+sizeof(HGCMFunctionParameter64)*index;i++) {
      printf("%02x",data_buf[i]);
   }
   printf("\n");*/
   return 0;
}

int hgcm_disconnect(int client_id) {
   VBGLIOCHGCMDISCONNECT data = {
      .Hdr.cbIn = sizeof(VBGLIOCHGCMDISCONNECT),
      .Hdr.uVersion = VBGLREQHDR_VERSION,
      .Hdr.uType = VBGLREQHDR_TYPE_DEFAULT,
      .Hdr.rc = VERR_INTERNAL_ERROR,
      .Hdr.cbOut = sizeof(VBGLREQHDR),
      .Hdr.uReserved = 0,
      .u.In.idClient = client_id,
   };
   ioctl(fd,VBGL_IOCTL_HGCM_DISCONNECT,&data);
   if (data.Hdr.rc) { //error
      return -1;
   }
   return 0;
}

/*int main() {
   //打开设备
   /*fd = open("/dev/vboxuser",O_RDWR);
   if (fd == -1) {
      die("open device error");
   }*/
   /*int idClient = hgcm_connect("VBoxGuestPropSvc");
   printf("idClient=%d\n",idClient);
   char ans[0x100] = {0};
   int ret = hgcm_call(idClient,2,"%b%b","in","foo",4,"in","bar",4);
   ret = hgcm_call(idClient,1,"%b%b%u%u","in","foo",4,"out",ans,0x100,0,0);

   printf("%s\n",ans);
   printf("%d\n",hgcm_disconnect(idClient));

}*/
