#ifndef _CLAA_CS_H_
#define _CLAA_CS_H_
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>

#include "claa_base.h"
#include "claa_tcp.h"

#define EUI_LEN 16
#define APPKEY_LEN 16
#define MAX_PAYLOAD_LEN 222
#define MAX_MSG_LEN 256
#define MAX_DATA_LEN 1024
#define EUI_STR_LEN 23

#define MAX_FRAME_LEN 235
#define MAX_BASE64_TEXT_LEN ((4*MAX_FRAME_LEN)/3 + 1)

#define CLAA_MIN(a,b) (((a)<(b))?(a):(b))

#define BUFFER_SIZE 1024
#define MAX_MSG_BUFFER_SIZE (4*1024)
//#define MAX_JSON_LEN (4*1024)

typedef struct cmd_join {
	uint32 cmdseq;
	uint64 appeui;
	uint8 appkey[APPKEY_LEN];
}st_cmd_join;

typedef struct cmd_quit {
	uint32 cmdseq;
	uint64 appeui;
}st_cmd_quit;

typedef struct cmd_sendto {
	uint32 cmdseq;
	uint64 appeui;
	uint64 deveui;
	uint8 cfm;
	uint8 payload[MAX_FRAME_LEN];
	uint16 pydlen;
	uint8 port;
}st_cmd_sendto;

typedef struct updata {
	uint32 code;
	uint32 cmdseq;
	uint64 appeui;
	uint64 deveui;
	schar msg[MAX_MSG_LEN];
	uint32 upseq;
	uint8 payload[MAX_PAYLOAD_LEN];
	sint16 pydlen;
	uint8 port;
}st_updata;

typedef struct ack {
	uint32 code;
	uint32 cmdseq;
	uint64 appeui;
	schar cmd[MAX_MSG_LEN];
	schar msg[MAX_MSG_LEN];
}st_ack;

typedef struct tcp_buffer {
	uint8  data[MAX_MSG_BUFFER_SIZE];
	uint32 len;
}st_tcp_buffer;
/*
typedef struct tcp_buffer {
	uint8 *data;
	uint32 datalen;
	uint32 capacity;
}st_tcp_buffer;
*/

extern sint32 g_sockfd;
extern schar g_serip[];
extern uint16 g_serport;
extern schar src_str[];
extern uint32 g_cmdseq;
extern uint8 g_appkey[];
extern uint64 g_appeui;
extern uint8 g_encrypt;

uint8 claa_cmd_join(st_cmd_join join, uint8 *outdata, uint16 *datalen);
uint8 claa_cmd_quit(st_cmd_quit quit, uint8 *outdata, uint16 *datalen);
uint8 claa_cmd_sendto(st_cmd_sendto sendto, uint8 *outdata, uint16 *datalen);
sint8 claa_packet_check(st_tcp_buffer *buffer, uint8 *jsonstr);
sint8 claa_msg_type(uint8 *pjson);
uint8 claa_parser_updata(uint8 *pjson, st_updata *updata);
uint8 claa_parser_ack(uint8 *pjson, st_ack *ack);
uint8 claa_hexstr_to_uint64(schar *instr, uint64 *outdata);
uint8 claa_check_endian(void);

#endif

