# 3dpwn_c
## What's this
This is the C Pwn library for VirtualBox's HGCM and Chromium Protocol.Just same as the ```niklasb```'s [3dpwn](https://github.com/niklasb/3dpwn),but this is a C library.If you want to know more about HGCM and Chromium  Analyse,you can view my [blog](https://www.anquanke.com/member/146878)

## files
the ```exp.c```was a exploit for VirtualBox-6.0.0 Nday by using ```crUnpackExtendShaderSource``` and```crUnpackExtendGetUniformLocation```'s vulnerability and the ```qwb2020_VbEscape_exp.c``` was a exploit for qwb2020's ```VbEscape```,other files was the C library.

## How to use
first you need to open the driver
```
fd = open("/dev/vboxuser",O_RDWR);
if (fd == -1) {
	die("open device error");
}
```
then you can use ```hgcm_connect``` to connect a VBox service.For example,I use it to connect ```VBoxGuestPropSvc``` service.
```
int idClient = hgcm_connect("VBoxGuestPropSvc");
printf("idClient=%d\n",idClient);
```
after you connect to the service,you can use ```hgcm_call``` to call the service's function.For example,I call ```VBoxGuestPropSvc```'s ```GUEST_PROP_FN_SET_PROP``` and it has two params.
```
int ret = hgcm_call(idClient,GUEST_PROP_FN_SET_PROP,"%b%b","in","foo",4,"in","bar",4);
```
the ```hgcm_call``` use format string to specify params' count.the ```%b``` means the param is the buffer,you need three param to express it.The first param is explain the buffer's type(in/out/inout).This means if the buffer can only be read on Host or can be writeback.The second param is the buffer's address and the third param is the buffer's length.Besides,```%u``` means the param is the ```unsigned int``` and ```%l``` means ```unsigned int64```
Now,we call ```GUEST_PROP_FN_GET_PROP``` to get back the strings we store before.
```
char ans[0x100] = {0};
ret = hgcm_call(idClient,1,"%b%b%u%u","in","foo",4,"out",ans,0x100,0,0);
printf("%s\n",ans);
```

finally we need to disconnect the ```HGCM``` service
```
printf("%d\n",hgcm_disconnect(idClient));
```
