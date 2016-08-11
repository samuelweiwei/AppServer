/**********************************
命令示例: 
set appeui ff-ff-ff-ff-ff-ff-ff-fe
set appkey 11:22:33:44:55:66:77:88:99:aa:bb:cc:dd:ee:ff:00
join <appeui ff-ff-ff-ff-ff-ff-ff-fe>
quit <appeui ff-ff-ff-ff-ff-ff-ff-fe>
sendto <appeui ff-ff-ff-ff-ff-ff-ff-fe> deveui 00-00-00-00-01-02-0a-0b cfm 1 payload 80:12:1f:ab:28:09:21 <port 28>
show
**********************************/
#include "claa_base.h"
#include "claa_cs.h"

/*
下面关于socket的宏定义是为了统一linux和windows上的Socket api
linux下编译程序时需要定义CLAA_LINUX
*/

enum cmd
{
	JOIN=1,
	QUIT,
	SENDTO,
	SET,
	SHOW,
	HELP
};

enum show_type
{
	ALL,
	APPEUI,
	APPKEY
};

enum msg_type
{
	DATA=100,
	ACK=200
};

/*
 * buffer按十六进制输出
 */
void hexdump(const unsigned char *buf, uint32 num)
{
    uint32 i = 0;
    for (; i < num; i++) 
    {
        printf("%02X ", buf[i]);
        if ((i+1)%8 == 0) 
            printf("\n");
    }
    printf("\n");
}

void claa_set_tcp_buffer(st_tcp_buffer *buffer);
uint32 claa_write_bytes(st_tcp_buffer* buf, uint8* data, sint32 len);
sint32 sock_open(schar *addr, uint16 portno);
void sock_close(sint32 *sockfd);
uint8 sock_reopen(sint32 *sockfd);
uint8 tcp_send(sint32 sockfd, const uint8* buffer, uint16 len);
void join_cmd_proc(void);
void quit_cmd_proc(void);
void sendto_cmd_proc(void);
void set_cmd_proc(void);
void show_cmd_proc(uint8 type);
void help_cmd_proc(void);
uint8 set_eui(schar *euistr, uint64 *eui);
uint8 set_payload(schar *payloadstr, uint8 *payload, uint16 *pydlen);
uint8 set_appkey(schar *keystr, uint8 *appkey);
uint8 get_cmd(schar *srcstr, schar *outstr, uint16 len);
uint8 get_para(schar *outstr, uint16 len);
sint32 creat_thread(void);
void tcp_recv_thread_func(void);
void usage(void);

sint32 g_sockfd = -1;
schar g_serip[15+1] = {0};
uint16 g_serport = 0;
schar src_str[1024];
uint32 g_cmdseq=0;
uint8 g_appkey[APPKEY_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
uint64 g_appeui = 0xfffffffffffffffe;
uint8 g_encrypt=0;

void join_cmd_proc(void)
{
	schar para[256];
	schar value[256];
	st_cmd_join join;
	uint8 senddata[MAX_DATA_LEN];
	uint16 datalen=0;

	/*
	使用默认appeui或者获取键盘输入的appeui
	*/
	if (!get_para(para, sizeof(para)))
	{
		printf("Use default appeui[%lx] for 'join' command.\n", g_appeui);
		join.appeui = g_appeui;
	}
	else
	{
		if (!strcasecmp(para, "appeui"))
		{
			if (!get_para(value, sizeof(value)))
			{
				printf("Syntax error in command '%s', can't find the value of 'appeui' para.\n", src_str);
				return;
			}
			if (!set_eui(value, &join.appeui))
			{
				printf("Syntax error in command '%s' word 3.\n", src_str);
				return;
			}
		}
		else
		{
			printf("Syntax error in command '%s' word 2.\n", src_str);
			return;
		}
	}

	/*
	使用默认appkey
	*/
	memcpy(join.appkey, g_appkey, APPKEY_LEN);

	/*
	设置命令序列号cmdseq
	*/
	g_cmdseq++;
	join.cmdseq = g_cmdseq;

	/*
	构造JOIN命令并发送
	*/
	if (claa_cmd_join(join, senddata, &datalen))
	{
		if (datalen != tcp_send(g_sockfd ,(const uint8 *)senddata, datalen))
		{
			printf("Send command FAIL.\n");
			g_cmdseq--;
			return;
		}
		printf("\nSend command SUCC : len:%s\n",senddata);
	}
	else
	{
		g_cmdseq--;
		printf("Creat 'join' command Json error.\n");
	}
	
	return;
}

void quit_cmd_proc(void)
{
	schar para[256];
	schar value[256];
	st_cmd_quit quit;
	uint8 senddata[MAX_DATA_LEN];
	uint16 datalen=0;

	/*
	使用默认appeui或者获取键盘输入的appeui
	*/
	if (!get_para(para, sizeof(para)))
	{
		printf("Use default appeui[%lx] for 'quit' command.\n", g_appeui);
		quit.appeui = g_appeui;
	}
	else
	{
		if (!strcasecmp(para, "appeui"))
		{
			if (!get_para(value, sizeof(value)))
			{
				printf("Syntax error in command '%s', can't find the value of 'appeui' para.\n", src_str);
				return;
			}
			if (!set_eui(value, &quit.appeui))
			{
				printf("Syntax error in command '%s' word 3.\n", src_str);
				return;
			}
		}
		else
		{
			printf("Syntax error in command '%s' word 2.\n", src_str);
			return;
		}
	}

	/*
	设置命令序列号cmdseq
	*/
	g_cmdseq++;
	quit.cmdseq = g_cmdseq;

	/*
	构造QUIT命令并发送
	*/
	if (claa_cmd_quit(quit, senddata, &datalen))
	{
		if (datalen != tcp_send(g_sockfd ,(const uint8 *)senddata, datalen))
		{
			printf("Send command FAIL.\n");
			g_cmdseq--;
			return;
		}
		printf("\nSend command SUCC : len:%s\n",senddata);
	}
	else
	{
		g_cmdseq--;
		printf("Creat 'quit' command Json error.\n");
	}
	
	return;
}

void sendto_cmd_proc(void)
{
	schar para[256];
	schar value[3 * MAX_DATA_LEN];
	uint8 paranum=1;
	uint8 cmdflag=0;
	st_cmd_sendto sendto;
	uint8 senddata[MAX_DATA_LEN];
	uint16 datalen=0;
	uint32 tmpport=0;

	/*将appeui预置为默认appeui*/
	sendto.appeui = g_appeui;
	/*将port预置为0*/
	sendto.port = 0;
	
	for (;;)
	{
		if (!get_para(para, sizeof(para)))
		{
			if (cmdflag<3)
			{
				printf("Syntax error in command 'sendto', the command of 'sendto' lack of parameter.\n");
			}
			break;
		}
		else
		{
			paranum++;
			
			/*
			获取键盘输入的appeui
			*/
			if (!strcasecmp(para, "appeui"))
			{
				if (!get_para(value, sizeof(value)))
				{
					printf("Syntax error in command '%s', can't find the value of 'appeui' para.\n", src_str);
					return;
				}
				paranum++;
				if (!set_eui(value, &sendto.appeui))
				{
					printf("Syntax error in command '%s' word %d.\n", src_str, paranum);
					return;
				}
			}
			/*
			获取键盘输入的deveui
			*/
			else if (!strcasecmp(para, "deveui"))
			{
				if (!get_para(value, sizeof(value)))
				{
					printf("Syntax error in command '%s', can't find the value of 'deveui' para.\n", src_str);
					return;
				}
				paranum++;
				if (!set_eui(value, &sendto.deveui))
				{
					printf("Syntax error in command '%s' word %d.\n", src_str, paranum);
					return;
				}
				cmdflag++;
			}
			/*
			获取键盘输入的cfm，输入值只能为0或1
			*/
			else if (!strcasecmp(para, "cfm"))
			{
				if (!get_para(value, sizeof(value)))
				{
					printf("Syntax error in command '%s', can't find the value of 'cfm' para.\n", src_str);
					return;
				}
				paranum++;
				if (!strcmp(value, "1"))
				{
					sendto.cfm = 1;
				}
				else if (!strcmp(value, "0"))
				{
					sendto.cfm = 0;
				}
				else
				{
					printf("Syntax error in command '%s' word %d.\n", src_str, paranum);
					return;
				}
				cmdflag++;
			}
			/*
			获取键盘输入的payload数据，
			payload用以":"分割的十六进制数字字符串表示每个字节的值
			*/
			else if (!strcasecmp(para, "payload"))
			{
				if (!get_para(value, sizeof(value)))
				{
					printf("Syntax error in command '%s', can't find the value of 'payload' para.\n", src_str);
					return;
				}
				paranum++;
				if (!set_payload(value, sendto.payload, &sendto.pydlen))
				{
					printf("Syntax error in command '%s' word %d.\n", src_str, paranum);
					return;
				}
				cmdflag++;
			}
			/*
			获取键盘输入的port值，port值用十进制的字符串表示
			*/
			else if (!strcasecmp(para, "port"))
			{
				if (!get_para(value, sizeof(value)))
				{
					printf("Syntax error in command '%s', can't find the value of 'port' para.\n", src_str);
					return;
				}
				paranum++;
				tmpport = (uint32)atoi(value);
				if ((tmpport < 21)||(223 < tmpport))
				{
					printf("Syntax error in command '%s' word %d.\n", src_str, paranum);
					return;
				}
				sendto.port = tmpport;
				printf("tmpport %d.\n", tmpport);
			}
		}
	}

	/*
	设置命令序列号cmdseq
	*/
	g_cmdseq++;
	sendto.cmdseq = g_cmdseq;

	/*
	构造SENDTO命令并发送
	*/
	if (claa_cmd_sendto(sendto, senddata, &datalen))
	{
		if (datalen != tcp_send(g_sockfd ,(const uint8 *)senddata, datalen))
		{
			printf("Send command FAIL.\n");
			g_cmdseq--;
			return;
		}
		printf("\nSend command SUCC : len:%s\n",senddata);
	}
	else
	{
		g_cmdseq--;
		printf("Creat 'sendto' command Json error.\n");
	}

	return;
}

void set_cmd_proc(void)
{
	schar para[256];
	schar value[256];
	uint8 cmdflag=0;
	uint8 paranum=0;

	for (;;)
	{
		if (!get_para(para, sizeof(para)))
		{
			if (!cmdflag)
			{
				printf("Syntax error in command '%s', the command of 'set' lack of parameter.\n", src_str);
			}
			return;
		}
		else
		{
			paranum++;
			
			/*
			获取键盘输入的appeui，并将输入值赋值给缺省的appeui
			*/
			if (!strcasecmp(para, "appeui"))
			{
				if (!get_para(value, sizeof(value)))
				{
					printf("Syntax error in command '%s', can't find the value of 'appeui' para.\n", src_str);
					return;
				}
				paranum++;
				if (!set_eui(value, &g_appeui))
				{
					printf("Syntax error in command '%s' word %d.\n", src_str, paranum);
					return;
				}
				cmdflag=1;
				printf("Set appeui SUCC\n");
				show_cmd_proc(APPEUI);
			}
			/*
			获取键盘输入的appkey数据，并将其赋给缺省的appkey，
			appkey用以":"分割的十六进制数字字符串表示每个字节的值，
			appkey共16个字节，每个字节都需要赋值(值可以为0)
			*/
			else if (!strcasecmp(para, "appkey"))
			{
				if (!get_para(value, sizeof(value)))
				{
					printf("Syntax error in command '%s', can't find the value of 'appkey' para.\n", src_str);
					return;
				}
				paranum++;
				if (!set_appkey(value, g_appkey))
				{
					printf("Syntax error in command '%s' word %d.\n", src_str, paranum);
					return;
				}
				cmdflag=1;
				printf("Set appkey SUCC\n");
				show_cmd_proc(APPKEY);
			}
		}
	}

	return;
}

void show_cmd_proc(uint8 type)
{
	uint8 i;
	uint8 *p;
	uint8 endian_type;

	/*
	显示缺省的appeui
	*/
	if ((ALL == type)||(APPEUI == type))
	{
		endian_type = claa_check_endian();
		if(endian_type)
		{
			p = (uint8 *)(&g_appeui) + sizeof(g_appeui) - 1;
		}
		else
		{
			p = (uint8 *)(&g_appeui);
		}
		printf("APPEUI : ");	
		for (i=0;i<sizeof(g_appeui);i++)
		{
			printf("%02lX", *p);
			if (i <= (sizeof(g_appeui)-2))
			{
				printf("-");	
			}
			if(endian_type)
			{
				p--;
			}
			else
			{
				p++;
			}
		}
		printf("\n");
	}
	
	/*
	显示缺省的appkey
	*/
	if ((ALL == type)||(APPKEY == type))
	{
		printf("APPKEY : ");	
		for (i=0;i<APPKEY_LEN;i++)
		{
			printf("%02lX", g_appkey[i]);
			if (i <= (APPKEY_LEN-2))
			{
				printf(":");	
			}
		}
		printf("\n");
	}

	return;
}

void help_cmd_proc(void)
{
    printf("The follows are the command examples.\n");
    printf("\n");
    printf("set appeui ff-ff-ff-ff-ff-ff-ff-fe\n");
    printf("set appkey 11:22:33:44:55:66:77:88:99:aa:bb:cc:dd:ee:ff:00\n");
    printf("join <appeui ff-ff-ff-ff-ff-ff-ff-fe>\n");
    printf("quit <appeui ff-ff-ff-ff-ff-ff-ff-fe>\n");
    printf("sendto <appeui ff-ff-ff-ff-ff-ff-ff-fe> deveui 00-00-00-00-01-02-0a-0b cfm 1 payload 80:12:1f:ab:28:09:21 <port 28>\n");
    printf("show\n");
    printf("help\n");

	return;
}

/**********************************
*函数名称: set_eui
*函数功能: 配置appeui
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
uint8 set_eui(schar *euistr, uint64 *eui)
{
	schar tmpeui[EUI_LEN+1];
	uint16 i,j;

	if (strlen(euistr) > EUI_STR_LEN)
	{
		return 0;
	}

	j = 0;
	for (i=0;i<strlen(euistr);i++)
	{
		if (euistr[i] != '-')
		{
			tmpeui[j] = euistr[i];
			j++;
		}
	}
	if (j > EUI_LEN)
	{
		return 0;
	}
	tmpeui[j] = '\0';

	if (!claa_hexstr_to_uint64(tmpeui, eui)) 
	{
		return 0;
	}

	return 1;
}

/**********************************
*函数名称: set_payload
*函数功能: 设置payload
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
uint8 set_payload(schar *payloadstr, uint8 *payload, uint16 *pydlen)
{
	uint16 i,j=0;
	uint8 tmp[3] = {0};
	uint64 data;
	uint16 len=0;
	uint8 tmppyd[MAX_FRAME_LEN];

	for (i=0; i<strlen(payloadstr); i++)
	{
		if (len >= MAX_FRAME_LEN)
		{
			return 0;
		}
		if (payloadstr[i] != ':')
		{
			if (j < sizeof(tmp))
			{
				tmp[j] = payloadstr[i];
				j++;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			tmp[j] = '\0';
			if (!claa_hexstr_to_uint64(tmp, &data)) 
			{
				return 0;
			}
			tmppyd[len] = (uint8)data;
			len++;
			j = 0;
			tmp[0] = '\0';
		}
	}
	if (strlen(tmp))
	{
		tmp[j] = '\0';
		if (!claa_hexstr_to_uint64(tmp, &data)) 
		{
			return 0;
		}
		tmppyd[len] = (uint8)data;
		len++;
	}
	memcpy(payload, tmppyd, len);
	*pydlen = len;

	return 1;
}

/**********************************
*函数名称: set_appkey
*函数功能: 设置appkey
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
uint8 set_appkey(schar *keystr, uint8 *appkey)
{
	uint16 i,j=0;
	uint8 tmp[3] = {0};
	uint64 data;
	uint16 len=0;
	uint8 tmpkey[APPKEY_LEN];

	for (i=0; i<strlen(keystr); i++)
	{
		if (len >= APPKEY_LEN)
		{
			return 0;
		}
		if (keystr[i] != ':')
		{
			if (j < sizeof(tmp))
			{
				tmp[j] = keystr[i];
				j++;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			tmp[j] = '\0';
			if (!claa_hexstr_to_uint64(tmp, &data)) 
			{
				return 0;
			}
			tmpkey[len] = (uint8)data;
			len++;
			j = 0;
			tmp[0] = '\0';
		}
	}
	if (strlen(tmp))
	{
		tmp[j] = '\0';
		if (!claa_hexstr_to_uint64(tmp, &data)) 
		{
			return 0;
		}
		tmpkey[len] = (uint8)data;
		len++;
	}
	if(APPKEY_LEN != len)
	{
		return 0;
	}
	memcpy(appkey, tmpkey, len);

	return 1;
}

uint8 get_cmd(schar *srcstr, schar *outstr, uint16 len)
{
	uint8 ret = 0;
	schar *pstr = NULL;
	schar delim[10] = " ";

	pstr = strtok(srcstr, delim);
	if ((NULL == pstr)||(strlen(pstr) >= len))
	{
		return 0;
	}
	strcpy(outstr, pstr);
	outstr[CLAA_MIN(strlen(pstr),len-1)] = '\0';
//	printf("CMD[%s]\n",outstr);

	return 1;
}

uint8 get_para(schar *outstr, uint16 len)
{
	uint8 ret = 0;
	schar *pstr = NULL;
	schar delim[10] = " ";

	pstr = strtok(NULL, delim);
	if ((NULL == pstr)||(strlen(pstr) >= len))
	{
		return 0;
	}
	strcpy(outstr, pstr);
	outstr[CLAA_MIN(strlen(pstr),len-1)] = '\0';
//	printf("para[%s]\n",outstr);

	return 1;
}

void usage(void)
{
	printf("\n");	
    printf("Usage:csdemo [options]\n");
    printf("-h List help document\n");
    printf("-i The ip of the edpacc\n");
    printf("-p The port of the edpacc\n");
    printf("-e Encrypt\n");

	return;
}
//extern schar *optarg;

int main(int argc, char *argv[])
{
	schar opt;
    schar* ip = NULL;
    schar* port = NULL;
    pthread_t p_id;
	schar cmdstr[256];
	uint8 cmd_stat;

	while ((opt = getopt(argc, argv, "hi:p:e")) != -1) 
	{
		switch (opt){
		case 'i':
			strcpy(g_serip, optarg);
			break;

		case 'p':
			g_serport = (uint16)(atoi(optarg));
			break;

		case 'e':
			g_encrypt = 1;
			break;

		case 'h': 
		default:
			usage();
		    exit(0);
		}
	}
	
	if (!strlen(g_serip) || !g_serport)
	{
		usage();
		return 0;
	}
	printf("\n");	
	
	/*创建socket并与MSP服务器建立TCP连接*/
	g_sockfd = sock_open(g_serip, g_serport);
	if (g_sockfd < 0) 
	{
		printf("\n");	
		printf("Create a socket and connect to MSP server failed!\n");	
		exit(0);
	}
	
	if (0 != creat_thread())
	{
		printf("\n");	
		printf("Create tcp receive thread failed!\n");	
		exit(0);
	}

	usleep(100*1000);
	printf("csdemo is working now ...\n");
	printf("=========================\n");
	for (;;)
	{
		usleep(100*1000);

		src_str[0] = '\0';
		fgets(src_str, sizeof(src_str), stdin);
		if (src_str[strlen(src_str)-1] == '\n')
		{
			src_str[strlen(src_str)-1] = '\0'; 
		}
		
		if ((strlen(src_str))&&('\n' != src_str[0]))
		{
			printf("-------------------------\n");	
			if (!get_cmd(src_str, cmdstr, sizeof(cmdstr)))
			{
				continue;
			}
			else
			{
				if (!strcasecmp(cmdstr, "JOIN"))
				{
					cmd_stat = JOIN;
				}
				else if (!strcasecmp(cmdstr, "QUIT"))
				{
					cmd_stat = QUIT;
				}
				else if (!strcasecmp(cmdstr, "SENDTO"))
				{
					cmd_stat = SENDTO;
				}
				else if (!strcasecmp(cmdstr, "SET"))
				{
					cmd_stat = SET;
				}
				else if (!strcasecmp(cmdstr, "SHOW"))
				{
					cmd_stat = SHOW;
				}
				else if (!strcasecmp(cmdstr, "HELP"))
				{
					cmd_stat = HELP;
				}
				else if (!strcasecmp(cmdstr, "QUIT"))
				{
					break;
				}
				else
				{
					printf("Unknow command!\n",cmdstr);
					printf("=========================\n");
					continue;
				}
			}

			switch (cmd_stat)
			{
			case JOIN:
				join_cmd_proc();
				break;
			case QUIT:
				quit_cmd_proc();
				break;
			case SENDTO:
				sendto_cmd_proc();
				break;
			case SET:
				set_cmd_proc();
				break;
			case SHOW:
				show_cmd_proc(ALL);
				break;
			case HELP:
				help_cmd_proc();
				break;
			}
			
			printf("=========================\n");
		}
	}
	
    sock_close(&g_sockfd);
	printf("Close tcp socket and quit csdemo process!\n");
	
    return 0;
}

sint32 creat_thread(void)
{
    pthread_t p_id;
	uint32 stack_size = 1024 * 1024;
	
	pthread_attr_t threadAttr;

	pthread_attr_init(&threadAttr);

	/* Set the stack size of the thread */
	pthread_attr_setstacksize(&threadAttr, stack_size);

	/* Set thread to detached state. No need for pthread_join */
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

	return pthread_create(&p_id, &threadAttr, (void *(*) (void *))tcp_recv_thread_func, NULL);
}

void tcp_recv_thread_func(void)
{
    sint32 n;
    uint8 recv_buf[BUFFER_SIZE];
	st_tcp_buffer tcp_buf;
	sint32 json_pos;
	uint8 json[MAX_MSG_BUFFER_SIZE];
	uint8 msgtype;
	st_updata updata;
	st_ack ack;
    
    printf("TCP recv thread start ...\n");

	for (;;)
	{
		if (g_sockfd >= 0)
		{
			claa_set_tcp_buffer(&tcp_buf);
			for (;;)
			{
				/*每次接收1024个字节的数据 */
				n = Recv(g_sockfd, recv_buf, BUFFER_SIZE, MSG_NOSIGNAL);
				if (n <= 0)
				{
					printf("Receive data error, ret[%d]\n",n);
					break;
				}
				printf("Receive data bytes: %d\n", n);//debug
				hexdump((const unsigned char *)recv_buf, n);//debug
		
				/*将接收的n个字节的数据写入buffer*/
				if (!claa_write_bytes(&tcp_buf, recv_buf, n))
				{
					claa_set_tcp_buffer(&tcp_buf);
					printf("Receive data len[%d] more than max message buffer size, tcp_buf-len[%d]\n", n, tcp_buf.len);
					continue;
				}
		
				/*检查是否获取一个完成的CLAA应用包*/
				if (!claa_packet_check(&tcp_buf, json))
				{
					printf("Need more bytes...\n");
					continue;
				}
				printf("Get Json[%s]\n",json);//debug
		
				msgtype = claa_msg_type(json);
				if (-1 == msgtype)
				{
					printf("Get message type failed!\n");
				}
				else
				{
					printf("Receive message type : %d\n",msgtype);//debug
					switch (msgtype)
					{
					case DATA:
						memset((uint8 *)(&updata), 0, sizeof(updata));
						if (!claa_parser_updata(json, &updata))
						{
							printf("Parser updata message failed.\n");
						}
						printf("Parser updata message SUCC :\n");//debug
						printf("\t code\t\t[%d]\n \t cmdseq\t\t[%d]\n \t appeui\t\t[%lX]\n \t deveui\t\t[%lX]\n \t msg\t\t[%s]\n \t pydlen\t\t[%d]\n \t port\t\t[%d]\n",
							updata.code, updata.cmdseq, updata.appeui, updata.deveui, updata.msg, updata.pydlen, updata.port);//debug
						printf("Palload :\n");//debug
						hexdump((const unsigned char *)updata.payload, updata.pydlen);//debug
												
						break;
					case ACK:
						memset((uint8 *)(&ack), 0, sizeof(ack));
						if (!claa_parser_ack(json, &ack))
						{
							printf("Parser ack message failed.\n");
						}
						printf("Parser ack message SUCC :\n");//debug
						printf("\t code\t\t[%d]\n \t cmdseq\t\t[%d]\n \t appeui\t\t[%lX]\n \t cmd\t\t[%s]\n \t msg\t\t[%s]\n",
							ack.code, ack.cmdseq, ack.appeui, ack.cmd, ack.msg);//debug

						break;
					default:
						printf("Receive unknow message type.\n");
						break;
					}			
					printf("=========================\n");
				}
			}
			sock_reopen(&g_sockfd);
		}
		else
		{
			sock_reopen(&g_sockfd);
		}
	}

    printf("TCP recv thread end ...\n");

	return;
}


