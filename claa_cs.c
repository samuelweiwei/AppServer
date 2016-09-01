#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "claa_base.h"
#include "cJSON.h"
//#include "Openssl.h"
#include "claa_cs.h"
#include "base64.h"
#include "cmac.h"

static uint8 creat_outdata(schar *json_str, uint8 *outdata, uint16 *datalen);
static void challenge_identification(uint8 appkey[APPKEY_LEN], uint64 appeui, uint32 appnonce, schar *challenge);

/**********************************
*函数名称: claa_cmd_join
*函数功能: 生成JOIN命令消息
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
uint8 claa_cmd_join(st_cmd_join join, uint8 *outdata, uint16 *datalen)
{
	uint8 ret = 0;
	schar challenge[32+1];
	uint32 appnonce;
	schar appeui[EUI_LEN+1];
	cJSON *json = NULL;

	srand((uint32)time(NULL));
	appnonce = (uint32)rand();

	challenge_identification(join.appkey, join.appeui, appnonce, challenge);
	
	sprintf(appeui, "%lX", join.appeui);
	appeui[EUI_LEN] = '\0';

	json = cJSON_CreateObject();
	if (NULL == json)
	{
		return 0;
	}

	cJSON_AddStringToObject(json, "CMD", "JOIN");
	cJSON_AddNumberToObject(json, "CmdSeq", join.cmdseq);
	cJSON_AddStringToObject(json, "AppEUI", appeui);
	cJSON_AddNumberToObject(json, "AppNonce", appnonce);
	cJSON_AddStringToObject(json, "Challenge", challenge);

	schar * json_str = cJSON_Print(json);
	if (NULL == json_str)
	{
		cJSON_Delete(json);
		return 0;
	}

	ret = creat_outdata(json_str, outdata, datalen);

	cJSON_Delete(json);	
	free(json_str);

	return ret;
}

/**********************************
*函数名称: claa_cmd_quit
*函数功能: 生成QUIT命令消息
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
uint8 claa_cmd_quit(st_cmd_quit quit, uint8 *outdata, uint16 *datalen)
{
	uint8 ret = 0;
	cJSON *json = NULL;
	schar appeui[EUI_LEN+1];

	sprintf(appeui, "%lX", quit.appeui);
	appeui[EUI_LEN] = '\0';

	json = cJSON_CreateObject();
	if (NULL == json) 
	{
		return 0;
	}

	cJSON_AddStringToObject(json, "CMD", "QUIT");
	cJSON_AddStringToObject(json, "AppEUI", appeui);
	cJSON_AddNumberToObject(json, "CmdSeq", quit.cmdseq);

	schar * json_str = cJSON_Print(json);
	if (NULL == json_str) 
	{
		cJSON_Delete(json);
		return 0;
	}

	ret = creat_outdata(json_str, outdata, datalen);

	cJSON_Delete(json);	
	free(json_str);

	return ret;
}

/**********************************
*函数名称: claa_cmd_sendto
*函数功能: 生成SENDTO命令消息
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
uint8 claa_cmd_sendto(st_cmd_sendto sendto, uint8 *outdata, uint16 *datalen)
{
	uint8 ret=0;
	cJSON *json = NULL;
	uint8 base64_text[MAX_PAYLOAD_LEN * 2 + 1];
	schar appeui[EUI_LEN+1];
	schar deveui[EUI_LEN+1];

	sprintf(appeui, "%lX", sendto.appeui);
	appeui[EUI_LEN] = '\0';
	
	sprintf(deveui, "%lX", sendto.deveui);
	deveui[EUI_LEN] = '\0';

	if (0 >= ConvertBinaryArrayToBase64Text(sendto.payload, sendto.pydlen, base64_text, 0)) 
	{
		return 0;
	}

	json = cJSON_CreateObject();
	if (NULL == json) 
	{
		return 0;
	}

	cJSON_AddStringToObject(json, "CMD", "SENDTO");
	cJSON_AddStringToObject(json, "AppEUI", appeui);
	cJSON_AddNumberToObject(json, "CmdSeq", sendto.cmdseq);
	cJSON_AddStringToObject(json, "DevEUI", deveui);
	cJSON_AddBoolToObject(json, "Confirm", sendto.cfm);
	cJSON_AddStringToObject(json, "payload", base64_text);
	if ( ((21 <= sendto.port)&&(sendto.port <= 223)) || (sendto.port == 10) )
	{
		cJSON_AddNumberToObject(json, "Port", sendto.port);
	}

	schar *json_str = cJSON_Print(json);
	if (NULL == json_str) 
	{
		cJSON_Delete(json);
		return 0;
	}

	ret = creat_outdata(json_str, outdata, datalen);

	cJSON_Delete(json);	
	free(json_str);

	return ret;
}

/**********************************
*函数名称: claa_packet_check
*函数功能: 解析上行数据消息
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
sint8 claa_packet_check(st_tcp_buffer *buffer, uint8 *jsonstr)
{
	sint8 res = 0;
	uint8 jsonlenstr[11]={0};
	uint16 jsonlen = 0;
	uint8 len_or_json=0;
	uint8 repeat_n=0;
	uint8 startflag=0;
	uint8 badflag=0;
	uint16 i, j=0;
	uint8 tmp[MAX_MSG_BUFFER_SIZE];

	while (1)
	{
		for (i=0; i<buffer->len; i++)
		{
			if (!badflag)
			{
				if (buffer->data[i] != '\n')
				{
					startflag = 1;
					repeat_n = 0;
					if (0 == len_or_json)
					{
						if (j >= sizeof(jsonlenstr))
						{
							badflag = 1;
						}
						jsonlenstr[j] = buffer->data[i];
						j++;
					}
					else
					{
						if (j >= MAX_MSG_BUFFER_SIZE)
						{
							badflag = 1;
						}
						jsonstr[j] = buffer->data[i];
						j++;
					}
				}
				else
				{
					if(startflag)
					{
						if (!repeat_n)
						{
							if(0 == len_or_json)
							{
								jsonlenstr[j] = '\0';
								jsonlen = (uint32)atoi(jsonlenstr);
							}
							else
							{
								jsonstr[j] = '\0';
								if (jsonlen == strlen(jsonstr))
								{
									res = 1;
								}
								else
								{
									res = -1;
								}
								break;
							}
							len_or_json = 1-len_or_json;
							j = 0;
						}
						repeat_n = 1;
					}
				}
			}
			else
			{
				if (buffer->data[i] == '\n')
				{
					res = -1;
					break;
				}
			}
		}
		
		if (0 == res)
		{
			jsonstr[0] = '\0';
			return 0;
		}
		else if(1 == res)
		{
			i++;
			jsonstr[j+1] = '\0';
			if (buffer->len - i > 0)
			{
				memcpy(tmp, buffer->data+i, buffer->len-i);
				memcpy(buffer->data, tmp, buffer->len-i);
			}
			buffer->len = buffer->len-i;
			return 1;
		}
		else if (-1 == res)
		{
			if (buffer->len - i > 0)
			{
				memcpy(tmp, buffer->data+i, buffer->len-i);
				memcpy(buffer->data, tmp, buffer->len-i);
			}
			buffer->len = buffer->len-i;
			res = 0;
			len_or_json = 0;
			repeat_n = 0;
			badflag = 0;	
			j = 0;
		}
	}
}

sint8 claa_msg_type(uint8 *pjson)
{
	cJSON *json, *item;
	uint32 code;

	json= cJSON_Parse(pjson);
	if (NULL == json) 
	{
		return -1;
	}
	
	item = cJSON_GetObjectItem(json, "CODE");
	if (NULL == item) 
	{
		return -1;
	}
	code = item->valueint;

	return code;
}


/**********************************
*函数名称: claa_parser_updata
*函数功能: 解析上行数据消息
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
uint8 claa_parser_updata(uint8 *pjson, st_updata *updata)
{
	cJSON *json, *item;
	schar eui[EUI_LEN+1];
	uint8 payload[MAX_BASE64_TEXT_LEN];

	if(NULL == pjson)
	{
		return 0;
	}

	json= cJSON_Parse(pjson);
	if (NULL == json) 
	{
		return 0;
	}

	item = cJSON_GetObjectItem(json, "CODE");
	if (NULL == item) 
	{
		return 0;
	}
	updata->code = item->valueint;
	
	item = cJSON_GetObjectItem(json, "AppEUI");
	if ((NULL == item) || (NULL == item->valuestring))
	{
		return 0;
	}
	strncpy(eui, item->valuestring, CLAA_MIN(strlen(item->valuestring),sizeof(eui)-1));
	eui[CLAA_MIN(strlen(item->valuestring),sizeof(eui)-1)] = '\0';
	if (!claa_hexstr_to_uint64(eui, &updata->appeui)) 
	{
		return 0;
	}
	
	item = cJSON_GetObjectItem(json, "CmdSeq");
	if (NULL == item) 
	{
		return 0;
	}
	updata->cmdseq = item->valueint;

	item = cJSON_GetObjectItem(json, "DevEUI");
	if ((NULL == item) || (NULL == item->valuestring)) 
	{
		return 0;
	}
	strncpy(eui, item->valuestring, CLAA_MIN(strlen(item->valuestring),sizeof(eui)-1));
	eui[CLAA_MIN(strlen(item->valuestring),sizeof(eui)-1)] = '\0';
	if (!claa_hexstr_to_uint64(eui, &updata->deveui)) 
	{
		return 0;
	}
	
	item = cJSON_GetObjectItem(json, "MSG");
	if ((NULL == item) || (NULL == item->valuestring)) 
	{
		return 0;
	}
	strncpy(updata->msg, item->valuestring, CLAA_MIN(strlen(item->valuestring),sizeof(updata->msg)-1));
	updata->msg[CLAA_MIN(strlen(item->valuestring),sizeof(updata->msg)-1)] = '\0';

	item = cJSON_GetObjectItem(json, "UpSeq");
	if (NULL != item) 
	{
		updata->upseq = item->valueint;
	}
	else
	{
		updata->upseq = (uint32)(-1);
	}

	item = cJSON_GetObjectItem(json, "payload");
	if ((NULL == item) || (NULL == item->valuestring)) 
	{
		return 0;
	}
	strncpy(payload, item->valuestring, CLAA_MIN(strlen(item->valuestring),sizeof(updata->payload)-1));
	payload[CLAA_MIN(strlen(item->valuestring),sizeof(updata->payload)-1)] = '\0';
	updata->pydlen = ConvertBase64TextToBinaryArray(payload, updata->payload, MAX_PAYLOAD_LEN);
	if (0 >= updata->pydlen) 
	{
		return 0;
	}

	item = cJSON_GetObjectItem(json, "Port");
	if (NULL != item) 
	{
		updata->port = item->valueint;
	}
	else
	{
		updata->port = 0;
	}
	
	return 1;
}

/**********************************
*函数名称: claa_parser_ack
*函数功能: 解析ACK消息
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
uint8 claa_parser_ack(uint8 *pjson, st_ack *ack)
{
	cJSON *json, *item;
	schar eui[EUI_LEN+1];

	if(NULL == pjson)
	{
		return 0;
	}

	json= cJSON_Parse(pjson);
	if (NULL == json) 
	{
		return 0;
	}

	item = cJSON_GetObjectItem(json, "CODE");
	if (NULL == item) 
	{
		return 0;
	}
	ack->code = item->valueint;
	
	item = cJSON_GetObjectItem(json, "AppEUI");
	if ((NULL == item) || (NULL == item->valuestring)) 
	{
		return 0;
	}
	strncpy(eui, item->valuestring, CLAA_MIN(strlen(item->valuestring),sizeof(eui)-1));
	eui[CLAA_MIN(strlen(item->valuestring),sizeof(eui)-1)] = '\0';
	if (!claa_hexstr_to_uint64(eui, &ack->appeui)) 
	{
		return 0;
	}
	
	item = cJSON_GetObjectItem(json, "CMD");
	if ((NULL == item) || (NULL == item->valuestring)) 
	{
		return 0;
	}
	strncpy(ack->cmd, item->valuestring, CLAA_MIN(strlen(item->valuestring),sizeof(ack->cmd)-1));
	ack->cmd[CLAA_MIN(strlen(item->valuestring),sizeof(ack->cmd)-1)] = '\0';
	
	item = cJSON_GetObjectItem(json, "CmdSeq");
	if (NULL == item) 
	{
		return 0;
	}
	ack->cmdseq = item->valueint;
	
	item = cJSON_GetObjectItem(json, "MSG");
	if ((NULL == item) || (NULL == item->valuestring)) 
	{
		return 0;
	}
	strncpy(ack->msg, item->valuestring, CLAA_MIN(strlen(item->valuestring),sizeof(ack->msg)-1));
	ack->msg[CLAA_MIN(strlen(item->valuestring),sizeof(ack->msg)-1)] = '\0';
	
	return 1;
}


/**********************************
*函数名称: claa_hexstr_to_uint64
*函数功能: 十六进制表示的数组字符串转换为uint64类型数据
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
uint8 claa_hexstr_to_uint64(schar *instr, uint64 *outdata)
{
	uint64 data=0;
	uint64 tmpdata;
	uint8 datalen = 0;
	uint8 i;

	if (NULL == instr) 
	{
		return 0;
	}
	
	datalen = strlen(instr);
	if ((datalen == 0)||(datalen > 16)) 
	{
		return 0;
	}

	for (i=0; i<datalen; i++) 
	{
		switch (instr[i]) 
		{
		case '0':
			tmpdata = 0;
			break;
		case '1':
			tmpdata = 1;
			break;
		case '2':
			tmpdata = 2;
			break;
		case '3':
			tmpdata = 3;
			break;
		case '4':
			tmpdata = 4;
			break;
		case '5':
			tmpdata = 5;
			break;
		case '6':
			tmpdata = 6;
			break;
		case '7':
			tmpdata = 7;
			break;
		case '8':
			tmpdata = 8;
			break;
		case '9':
			tmpdata = 9;
			break;
		case 'a':
		case 'A':
			tmpdata = 10;
			break;
		case 'b':
		case 'B':
			tmpdata = 11;
			break;
		case 'c':
		case 'C':
			tmpdata = 12;
			break;
		case 'd':
		case 'D':
			tmpdata = 13;
			break;
		case 'e':
		case 'E':
			tmpdata = 14;
			break;
		case 'f':
		case 'F':
			tmpdata = 15;
			break;
		default:
			return 0;
		}
		data += tmpdata<<(4*(datalen-i-1));
	}

	*outdata = data;
	
	return 1;
}

/**********************************
*函数名称: claa_check_endian
*函数功能: 大端小端判断函数
*参数说明:
*    入参: 无
*    出参: 无
*返回值  : 0 - 大端；1 - 小端
*********************************/
uint8 claa_check_endian(void)  
{  
	union check  
	{  
		int i;  
		char ch;  
	}c;  
	c.i=1;  
	return (c.ch==1);  
}

/**********************************
*函数名称: challenge_identification
*函数功能: 生成挑战字
*参数说明:
*    入参: appkey    - appkey数组，长度为16字节。
*	       appeui    - appeui值
*	       appnonce  - appnoce值
*    出参: challenge - 返回计算出的挑战字的值，该指针需要指向char类型的数组首地址，数组长度必须大于32字节。
*返回值  : 无
**********************************/
static void challenge_identification(uint8 appkey[APPKEY_LEN], uint64 appeui, uint32 appnonce, schar *challenge)
{
	uint8 *ptmp = NULL;
	uint8 msg[16]={0};
	uint8 tmp_challenge[16]={0};
	AES_CMAC_CTX cmacctx;
	uint8 i,j;
	uint8 high,low;
	uint8 flag = 0;

	/*
	* 生成msg
	* msg = appeui|appnonce|0 (注：msg最后补32位的0，成128bit信息块)
	*/
	ptmp = (uint8 *)(&appeui);
	for (i=0;i<8;i++) 
	{
		msg[i] = ptmp[8-1-i];
	}
	ptmp = (uint8 *)(&appnonce);
	for (i=0;i<4;i++) 
	{
		msg[i+8] = ptmp[4-1-i];
	}
	for (i=0;i<4;i++) 
	{
		msg[i+12] = 0;
	}

	/*
	* aes_cmac 加密生成挑战字
	*/
	AES_CMAC_Init(&cmacctx);
	AES_CMAC_SetKey(&cmacctx, appkey);
	AES_CMAC_Update(&cmacctx, msg, 16);
	AES_CMAC_Final(tmp_challenge, &cmacctx);

	/*
	* 将挑战字转化为十六进制整数表示的字符串，(十六进制数字的字母部分采用大写，前面不加"0x")
	*/
	j=0;
	for (i=0;i<16;i++) 
	{
		high = ((tmp_challenge[15-i])&(0xF0))>>4;
		low  =  (tmp_challenge[15-i])&(0x0F);
		if ((0 == flag)&&((0 == high)&&(0 == low))) 
		{
			continue;
		}
		if ((0 == flag)&&(0 == high)&&(0 != low)) 
		{
			if(low<=9) 
			{
				challenge[j] = '0'+low;
			} 
			else 
			{
				challenge[j] = 'A'+(low-10);
			}
			j++;
			flag = 1;
			continue;
		}
		flag = 1;
		if (high<=9) 
		{
			challenge[j] = '0'+high;
		} 
		else 
		{
			challenge[j] = 'A'+(high-10);
		}
		j++;
		if (low<=9) 
		{
			challenge[j] = '0'+low;
		} 
		else 
		{
			challenge[j] = 'A'+(low-10);
		}
		j++;
	}
	challenge[32] = '\0';

	return;
}

/**********************************
*函数名称: creat_outdata
*函数功能: 生成命令消息
*参数说明:
*    入参: 
*    出参: 
*返回值  : 
**********************************/
static uint8 creat_outdata(schar *json_str, uint8 *outdata, uint16 *datalen)
{
	uint16 json_len = 0;
	sint16 head_len = 0;
	uint16 data_len = 0;

	json_len = strlen(json_str);
	head_len = sprintf(outdata, "%d", json_len);
	outdata[head_len] = '\n';
	data_len = head_len + 1 + json_len;
	if (data_len >= MAX_DATA_LEN) 
	{
		outdata[0] = '\0';
		return 0;
	}
	*datalen = data_len;
	memcpy(outdata+head_len+1, json_str, json_len);
	outdata[*datalen] = '\0';

	return 1;
}





