#ifndef _CLAA_SSL_H_
#define _CLAA_SSL_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h> 
#include <openssl/err.h>

#include "claa_base.h"

uint8 claa_ssl_init(SSL_CTX **ctx);
void claa_ssl_close(SSL **ssl, SSL_CTX **ctx);
uint8 claa_ssl_connect(SSL **ssl, SSL_CTX *ctx, sint32 *sockfd);
uint8 claa_ssl_re_connect(SSL **ssl, SSL_CTX **ctx, sint32 *sockfd);
sint32 claa_ssl_send(SSL *ssl, schar *pbuf, uint32 buflen);
sint32 claa_ssl_recv(SSL *ssl, schar *pbuf, uint32 maxlen);

#endif

