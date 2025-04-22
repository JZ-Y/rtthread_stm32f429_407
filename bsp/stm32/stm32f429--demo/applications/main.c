/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stdio.h>

//#include "mb.h"
//#include "user_mb_app.h"
#define SLAVE_ADDR      MB_SAMPLE_SLAVE_ADDR
#define PORT_NUM        MB_SLAVE_USING_PORT_NUM
#define PORT_BAUDRATE   MB_SLAVE_USING_PORT_BAUDRATE
#define PORT_PARITY     MB_PAR_NONE //MB_PAR_EVEN

/* defined the LED0 pin: PB1 */
#define LED0_PIN    GET_PIN(B, 1)

#define HWTIMER_DEV_NAME "timer3"
#define HW_WDI	GET_PIN(A, 3)


rt_err_t hwtimer_callback(rt_device_t dev,rt_size_t size)
{
	rt_pin_write(HW_WDI, !rt_pin_read(HW_WDI));
//	rt_kprintf("hw\r\n");
	
	return RT_EOK;
}

static int timer_Init(void)
{
	/* ���� */
	rt_pin_mode(HW_WDI, PIN_MODE_OUTPUT);	//�ⲿ��ʱ����·WDI

	rt_device_t hw_timer;
	rt_hwtimerval_t timeout_s;      		/* ��ʱ����ʱֵ */
	rt_err_t res;
	rt_uint32_t freq = 10000;               /* ����Ƶ�� */
	rt_uint32_t mode = HWTIMER_MODE_PERIOD;
	timeout_s.sec = 1;		//s
	timeout_s.usec = 600000;		//us
	
	hw_timer = rt_device_find(HWTIMER_DEV_NAME);
	if(hw_timer == NULL) {rt_kprintf("find err!\r\n"); return RT_ERROR;}
	
	/* �Զ�д��ʽ���豸  �ڲ���Init����*/
	res = rt_device_open(hw_timer, RT_DEVICE_OFLAG_RDWR);
	if(res != RT_EOK) {rt_kprintf("open err!\r\n"); return RT_ERROR;}
	
	/* ���ó�ʱ�ص����� */
	res = rt_device_set_rx_indicate(hw_timer, hwtimer_callback);
	if(res != RT_EOK) {rt_kprintf("set rx indicate err!\r\n"); return RT_ERROR;}
	
	/* ���ü���Ƶ�� */
    res = rt_device_control(hw_timer, HWTIMER_CTRL_FREQ_SET, &freq);
	
	/* ���������� */
	res = rt_device_control(hw_timer, HWTIMER_CTRL_MODE_SET, &mode);

	/* ����Ϊ����1.6s */
	rt_device_write(hw_timer, 0, &timeout_s, sizeof(timeout_s));
	
	rt_kprintf("hw timer init successful!\r\n");
	return RT_EOK;
}
INIT_DEVICE_EXPORT(timer_Init);

#include <netdb.h>
#include <sys/socket.h> /* ʹ��BSD socket����Ҫ����socket.hͷ�ļ� */
#define BUFSZ  1024

#if 0
static void tcpclientinit(int agrc, char **agrv)
{
	char *url;
	int port;
	struct addrinfo hints, *res;
	int ret;
	int sock, bytes_received;
	char *recv_data;
	char send_data[] = "receive successful from rtt\n";
	
	if(agrc < 3)
	{
		printf("testtcp IP			PORT\r\n");
		printf("testtcp 192.168.0.1 5000\r\n");
		return ;
	}
	
	//��ȡurl��port
	url = agrv[1];
	port = (int)strtoul(agrv[2], 0, 10);
	
	/* ���� hints����ʾ�����飩 �ṹ�壬ָ��ϣ�����صĵ�ַ���Э�� */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;			//ipv4
	hints.ai_socktype = SOCK_STREAM;	//tcp
	hints.ai_protocol = 0;				//�Զ�ѡ��Э�� �˴�����SOCK_STREAMѡ��
	
	/*
		nodename:Э���ַ
		servname:������/�˿ںţ�"http/port"��
		struct *hint
		struct **res
	*/
	ret = getaddrinfo(url, agrv[2], &hints, &res);
	
	if(ret != RT_EOK)
	{
		rt_kprintf("getaddrinfo error!\n");	// \n����  \r �Ὣ����ƶ�������
		return;
	}
	
	recv_data = rt_malloc(BUFSZ);
	if(recv_data == NULL)
	{
		rt_kprintf("No memory\n");
		freeaddrinfo(res);
		return;
	}
	
	/*
		domain:Э����
		type:�׽�������
		protocol:Э�飬Ϊ0���ʾ�����׽������ͽ����Զ�ѡ��
		
		return:socket ������ / ���
	*/
	sock = socket(res->ai_family, res->ai_socktype, 0);
	if(sock == -1)
	{
		rt_kprintf("Socket error!\n");
		rt_free(recv_data);
		freeaddrinfo(res);
		return;
	}
	
	if(connect(sock, res->ai_addr, res->ai_addrlen) == -1)
	{
		rt_kprintf("connect error!\n");
		closesocket(sock);
		rt_free(recv_data);
		freeaddrinfo(res);
		return;
	}else{
		rt_kprintf("connect successful!\n");
		freeaddrinfo(res);		// ������ɺ��ͷ���Դ
	}
	

	while(1)
	{
		//���ճɹ�return �ֽ�����ʧ��return -1
		/* 
			s:socket������
			*mem:�������ݵĻ�����
			len:��������С
			flag:0��MSG_PEEK��MSG_WAITALL��MSG_DONTWAIT
		*/
		bytes_received = recv(sock, recv_data, BUFSZ - 1, 0);	//�˴�Ҫȷ�����㹻�Ŀռ䱣��\0�����ԣ�BUFSZ - 1��
		
		//server �Ͽ�����ʾ received error, close the socket
		if(bytes_received < 0)
		{
			closesocket(sock);
			rt_kprintf("received error, close the socket.\n");
			rt_free(recv_data);
			break;
		}
		else if(bytes_received == 0)
		{
			closesocket(sock);
			rt_kprintf("received error, close the socket.\n");
			rt_free(recv_data);
			break;
		}
		
		//������յ���������û�� \0 �������ģ���Ϊ����Э�飨���� TCP �� UDP���������ԭʼ���ֽ�����û���ض��Ľ�����
		recv_data[bytes_received] = '\0';
		
		if(strncmp(recv_data, "q", 1) == 0 || strncmp(recv_data, "Q", 1) == 0)		//" " ��''
		{
			closesocket(sock);	//����һ��TCP���ӣ�ͨ����˵���ر��׽��־���ζ�š��Ͽ����ӡ�,4�λ���
			rt_kprintf("disconnect!\n");
			
			rt_free(recv_data);
			break;
		}
		else
		{
			rt_kprintf("recv_data = %s\n", recv_data);
		}
		
		/*
			s:socket������
			*dataptr:���͵�����buf
			size:�������ݵĳ���
			flags: 0��Ĭ����Ϊ����MSG_DONTWAIT�����������ͣ���MSG_NOSIGNAL����ֹ SIGPIPE����MSG_OOB�����ʹ������ݣ���
		*/
		ret = send(sock, send_data, strlen(send_data), 0);
		if(ret != strlen(send_data))
		{
			rt_kprintf("send error!\n");
			closesocket(sock);
			rt_free(recv_data);
			break;
		}
		
	}
	return;
	
}
#endif


#define TCP_STACK_SIZE 2048
#define TCP_PRIORITY   10
rt_thread_t tcp_thread = NULL;

typedef struct{
	char * _url;
	char * _port;
}tcp_struct;

static tcp_struct *tcp_info;		//ֻ������ָ�룬��Ҫ�ֶ������ڴ�


void tcpclientinit_(void *parameter)
{
	char *url;
	int port;
	struct addrinfo hints, *res;
	int ret;
	int sock, bytes_received;
	char *recv_data;
	char send_data[] = "receive successful from rtt\n";
	
	
	//��ȡurl��port
	url = ((tcp_struct *)parameter)->_url;//(char *)((tcp_struct *)parameter->_url);  //parameter
	port = (int)strtoul(((tcp_struct *)parameter)->_port, 0, 10);//5000; //(int)strtoul(agrv[2], 0, 10);
	rt_kprintf("port:%d\n",port);
	
	/* ���� hints����ʾ�����飩 �ṹ�壬ָ��ϣ�����صĵ�ַ���Э�� */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;			//ipv4
	hints.ai_socktype = SOCK_STREAM;	//tcp
	hints.ai_protocol = 0;				//�Զ�ѡ��Э�� �˴�����SOCK_STREAMѡ��
	
	/*
		nodename:Э���ַ
		servname:������/�˿ںţ�"http/port"��
		struct *hint
		struct **res
	*/
	ret = getaddrinfo(url, ((tcp_struct *)parameter)->_port, &hints, &res); //agrv[2]
	
	if(ret != RT_EOK)
	{
		rt_kprintf("getaddrinfo error!\n");	// \n����  \r �Ὣ����ƶ�������
		tcp_thread = NULL;
		return;
	}
	
	recv_data = rt_malloc(BUFSZ);
	if(recv_data == NULL)
	{
		rt_kprintf("No memory\n");
		freeaddrinfo(res);
		tcp_thread = NULL;
		return;
	}
	
	/*
		domain:Э����
		type:�׽�������
		protocol:Э�飬Ϊ0���ʾ�����׽������ͽ����Զ�ѡ��
		
		return:socket ������ / ���
	*/
	sock = socket(res->ai_family, res->ai_socktype, 0);
	if(sock == -1)
	{
		rt_kprintf("Socket error!\n");
		rt_free(recv_data);
		freeaddrinfo(res);
		tcp_thread = NULL;
		return;
	}
	
	if(connect(sock, res->ai_addr, res->ai_addrlen) == -1)
	{
		rt_kprintf("connect error!\n");
		closesocket(sock);
		rt_free(recv_data);
		freeaddrinfo(res);
		tcp_thread = NULL;
		return;
	}else{
		rt_kprintf("TCP connect successful!\n");
		freeaddrinfo(res);		// ������ɺ��ͷ���Դ
	}

	while(1)
	{
		//���ճɹ�return �ֽ�����ʧ��return -1
		/* 
			s:socket������
			*mem:�������ݵĻ�����
			len:��������С
			flag:0��MSG_PEEK��MSG_WAITALL��MSG_DONTWAIT
		*/
		bytes_received = recv(sock, recv_data, BUFSZ - 1, 0);	//�˴�Ҫȷ�����㹻�Ŀռ䱣��\0�����ԣ�BUFSZ - 1��
		
		//server �Ͽ�����ʾ received error, close the socket
		if(bytes_received < 0)
		{
			closesocket(sock);
			rt_kprintf("received error, close the socket.\n");
			rt_free(recv_data);
			tcp_thread = NULL;
			break;
		}
		else if(bytes_received == 0)
		{
			closesocket(sock);
			rt_kprintf("received error, close the socket.\n");
			rt_free(recv_data);
			tcp_thread = NULL;
			break;
		}
		
		//������յ���������û�� \0 �������ģ���Ϊ����Э�飨���� TCP �� UDP���������ԭʼ���ֽ�����û���ض��Ľ�����
		recv_data[bytes_received] = '\0';
		
		if(strncmp(recv_data, "q", 1) == 0 || strncmp(recv_data, "Q", 1) == 0)		//" " ��''
		{
			closesocket(sock);	//����һ��TCP���ӣ�ͨ����˵���ر��׽��־���ζ�š��Ͽ����ӡ�,4�λ���
			rt_kprintf("TCP disconnect!\n");
			tcp_thread = NULL;
			rt_free(recv_data);
			break;
		}
		else
		{
			rt_kprintf("recv_data = %s\n", recv_data);
		}
		
		/*
			s:socket������
			*dataptr:���͵�����buf
			size:�������ݵĳ���
			flags: 0��Ĭ����Ϊ����MSG_DONTWAIT�����������ͣ���MSG_NOSIGNAL����ֹ SIGPIPE����MSG_OOB�����ʹ������ݣ���
		*/
		ret = send(sock, send_data, strlen(send_data), 0);
		if(ret != strlen(send_data))
		{
			rt_kprintf("send error!\n");
			closesocket(sock);
			rt_free(recv_data);
			tcp_thread = NULL;
			break;
		}
		
	}
	return;	

}


static void TCPclient(int agrc, char **agrv)
{
	
	if(tcp_thread != NULL) return;		//��ֹ���������close �� err ����Ҫ tcp_thread = NULL;
	if(agrc < 3)
	{
//		printf("testtcp IP			PORT\r\n");
		printf("example :TCPclient 192.168.0.1 5000\r\n");
		return ;
	}
	
	//��Ϊ������ֻ��ָ��static tcp_struct *tcp_info; ���������ڴ�|| static tcp_struct tcp_info���Ƿ����˾ֲ�������
	tcp_info = rt_malloc(sizeof(tcp_struct));	
	tcp_info->_url = agrv[1];
	tcp_info->_port = agrv[2];
	
	if((tcp_thread = rt_thread_create("tcp", tcpclientinit_, (tcp_struct *)tcp_info, TCP_STACK_SIZE, TCP_PRIORITY, 10)) == NULL)	//(tcp_struct *)tcp_info
	{
		rt_kprintf("tcp thread create error!\n");
		tcp_thread = NULL;
	}
	rt_thread_startup(tcp_thread);
}
MSH_CMD_EXPORT(TCPclient, open tcp client);

int main(void)
{
	// 485ֻ�����

	//	rt_pin_mode(GET_PIN(A, 8), PIN_MODE_OUTPUT);
//	rt_pin_mode(GET_PIN(D, 0), PIN_MODE_OUTPUT);
//	rt_pin_write(GET_PIN(A, 8), PIN_LOW);
//	rt_pin_write(GET_PIN(D, 0), PIN_HIGH);


//	rt_kprintf("111\r\n");
//	RT_ASSERT(0);
//	eMBInit(MB_RTU, SLAVE_ADDR, PORT_NUM, PORT_BAUDRATE, PORT_PARITY);
//	eMBEnable();
//	while(1)
//	{
//        eMBPoll();
//        rt_thread_mdelay(200);
//	}
	
	
    return RT_EOK;
}

void phy_reset(void){
	;
}

