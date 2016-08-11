#include "claa_cs.h"

void claa_set_tcp_buffer(st_tcp_buffer *buffer)
{	
    buffer->data[0] = '\0';
    buffer->len = 0;
	
    return;
}

sint32 sock_open(schar *addr, uint16 portno)
{
    sint32 sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    /*
    创建socket套接字
    */
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
	{
        printf("Open socket failed!\n");
        return -1; 
    }
	
    /*
    获取服务器信息
    */
    server = GetHostByName(addr);
    if (server == NULL) 
	{
        printf("Get host[%s] failed!\n",addr);
        return -1;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
//    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	
    /*
    客户端与TCP服务器建立连接
    */
    if (Connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    {
        printf("Connect to MSP server  %s:%d failed!\n");
        return -1;
    }
    printf("Connect to MSP server %s:%d SUCC!...\n", addr, portno);

    return sockfd;
}

void sock_close(sint32 *sockfd)
{
	if(*sockfd >= 0)
	{
		Close(*sockfd);
	}
	*sockfd = -1;

	return;
}

uint8 sock_reopen(sint32 *sockfd)
{
    sint32 tmp_sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    /*
    关闭socket
    */
	sock_close(&g_sockfd);
	sleep(1);
	
    /*
    创建socket套接字
    */
    tmp_sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    if (tmp_sockfd < 0) 
	{
        printf("Reopen socket failed!\n");
        return 0; 
    }
	
    /*
    获取服务器信息
    */
    server = GetHostByName(g_serip);
    if (server == NULL) 
	{
        printf("Get host[%s] failed!\n",g_serip);
        return 0;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(g_serport);
//    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	
    /*
    客户端与TCP服务器建立连接
    */
    if (Connect(tmp_sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    {
        printf("Reconnect to MSP server  %s:%d failed!\n");
        return 0;
    }
    printf("Reconnect to MSP server %s:%d SUCC!...\n", g_serip, g_serport);

	*sockfd = tmp_sockfd;
	
    return 1;
}

uint8 tcp_send(sint32 sockfd, const uint8* buffer, uint16 len)
{
	uint16 total  = 0;
	uint16 n = 0;
	
	while (len != total)
	{
		/* 试着发送len - total个字节的数据 */
		n = Send(sockfd, buffer+total, len-total, MSG_NOSIGNAL);
		if (n <= 0)
		{
			return n;
		}
		/* 成功发送了n个字节的数据 */
		total += n;
	}

	return total;
}

uint32 claa_write_bytes(st_tcp_buffer* buf, uint8* data, sint32 len)
{
	printf("buf->len[%d] len[%d]\n", buf->len, len);
	if (buf->len + len > MAX_MSG_BUFFER_SIZE)
	{
		return 0;
	}
	
    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
	
    return 1;
}

/*
void claa_set_tcp_buffer(st_tcp_buffer *buffer)
{	
	if (NULL != buffer->data)
	{
		free(buffer->data);
	}
    buffer->data = (uint8*)malloc(sizeof(uint8) * BUFFER_SIZE);
    buffer->len = 0;
    buffer->capacity = BUFFER_SIZE;
	
    return;
}

uint32 claa_write_bytes(st_tcp_buffer* buf, uint8* data, sint32 len)
{
    if (check_buffer_capacity(buf, len))
	{
        return 0;
	}

    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
	
    return 1;
}

uint8 check_buffer_capacity(st_tcp_buffer* buf, uint32 len)
{
    uint32 cap_len = buf->capacity;
    uint8* pdata = NULL;

    while (cap_len - buf->len < len) 
    {
        cap_len = cap_len + BUFFER_SIZE;
        if (cap_len > MAX_MSG_BUFFER_SIZE)
    	{
			return 0;
    	}
    }
	
    if (cap_len > buf->capacity)
    {
        pdata = (uint8*)malloc(sizeof(uint8) * cap_len);
        memcpy(pdata, buf->data, buf->len);
        free(buf->data);
        buf->data = pdata;
        buf->capacity = cap_len;
    }
	
    return 1;
}
*/


