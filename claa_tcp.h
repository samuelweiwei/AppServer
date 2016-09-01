#ifndef _CLAA_TCP_H_
#define _CLAA_TCP_H_

#include "claa_base.h"

#if 1//def CLAA_LINUX
#define Socket(a,b,c)          socket(a,b,c)
#define Connect(a,b,c)         connect(a,b,c)
#define Close(a)               close(a)
#define Read(a,b,c)            read(a,b,c)
#define Recv(a,b,c,d)          recv(a, (void *)(b), c, d)
#define Select(a,b,c,d,e)      select(a,b,c,d,e)
#define Send(a,b,c,d)          send(a, (const sint8 *)(b), c, d)
#define Write(a,b,c)           write(a,b,c)
#define GetSockopt(a,b,c,d,e)  getsockopt((int)(a),(int)(b),(int)(c),(void *)(d),(socklen_t *)(e))
#define SetSockopt(a,b,c,d,e)  setsockopt((int)(a),(int)(b),(int)(c),(const void *)(d),(int)(e))
#define GetHostByName(a)       gethostbyname((const schar *)(a))
#endif



#endif

