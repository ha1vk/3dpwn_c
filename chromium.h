#ifndef CR_H
#define CR_H

#define SHCRGL_GUEST_FN_READ        (3)
#define SHCRGL_GUEST_FN_SET_VERSION (6)
#define SHCRGL_GUEST_FN_WRITE_BUFFER        (13)
#define SHCRGL_GUEST_FN_WRITE_READ_BUFFERED (14)

/*For now guest is allowed to connect host opengl service if protocol version matches exactly*/
/*Note: that after any change to this file, or glapi_parser\apispec.txt version should be changed*/
#define CR_PROTOCOL_VERSION_MAJOR 9
#define CR_PROTOCOL_VERSION_MINOR 1

#define CR_MESSAGE_OPCODES 0x77474c01
#define CR_EXTEND_OPCODE 247
#define CR_GETUNIFORMLOCATION_EXTEND_OPCODE 164
#define CR_SHADERSOURCE_EXTEND_OPCODE 240

unsigned char crmsg_buf[0x1000];
int alloc_buf(int client,int size,const void *msg,int msg_len);
int write_buf(int client,int buf_id,int size,int offset,const void *msg,int msg_len);
int read_hostbuf(int client,int size,void *buf);
int set_version(int client);
int crmsg_with_bufid(int client,int buf_id);
int crmsg(int client,int size,const void *msg,int msg_len);

#endif
