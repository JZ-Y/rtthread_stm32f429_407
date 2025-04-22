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
	/* 推挽 */
	rt_pin_mode(HW_WDI, PIN_MODE_OUTPUT);	//外部定时器电路WDI

	rt_device_t hw_timer;
	rt_hwtimerval_t timeout_s;      		/* 定时器超时值 */
	rt_err_t res;
	rt_uint32_t freq = 10000;               /* 计数频率 */
	rt_uint32_t mode = HWTIMER_MODE_PERIOD;
	timeout_s.sec = 1;		//s
	timeout_s.usec = 600000;		//us
	
	hw_timer = rt_device_find(HWTIMER_DEV_NAME);
	if(hw_timer == NULL) {rt_kprintf("find err!\r\n"); return RT_ERROR;}
	
	/* 以读写方式打开设备  内部有Init操作*/
	res = rt_device_open(hw_timer, RT_DEVICE_OFLAG_RDWR);
	if(res != RT_EOK) {rt_kprintf("open err!\r\n"); return RT_ERROR;}
	
	/* 设置超时回调函数 */
	res = rt_device_set_rx_indicate(hw_timer, hwtimer_callback);
	if(res != RT_EOK) {rt_kprintf("set rx indicate err!\r\n"); return RT_ERROR;}
	
	/* 设置计数频率 */
    res = rt_device_control(hw_timer, HWTIMER_CTRL_FREQ_SET, &freq);
	
	/* 设置周期性 */
	res = rt_device_control(hw_timer, HWTIMER_CTRL_MODE_SET, &mode);

	/* 设置为周期1.6s */
	rt_device_write(hw_timer, 0, &timeout_s, sizeof(timeout_s));
	
	rt_kprintf("hw timer init successful!\r\n");
	return RT_EOK;
}
INIT_DEVICE_EXPORT(timer_Init);

#include <netdb.h>
#include <sys/socket.h> /* 使用BSD socket，需要包含socket.h头文件 */
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
	
	//获取url和port
	url = agrv[1];
	port = (int)strtoul(agrv[2], 0, 10);
	
	/* 设置 hints（提示、建议） 结构体，指定希望返回的地址族和协议 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;			//ipv4
	hints.ai_socktype = SOCK_STREAM;	//tcp
	hints.ai_protocol = 0;				//自动选择协议 此处根据SOCK_STREAM选择
	
	/*
		nodename:协议地址
		servname:服务名/端口号（"http/port"）
		struct *hint
		struct **res
	*/
	ret = getaddrinfo(url, agrv[2], &hints, &res);
	
	if(ret != RT_EOK)
	{
		rt_kprintf("getaddrinfo error!\n");	// \n换行  \r 会将光标移动到行首
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
		domain:协议族
		type:套接字类型
		protocol:协议，为0则表示根据套接字类型进行自动选择
		
		return:socket 描述符 / 句柄
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
		freeaddrinfo(res);		// 解析完成后释放资源
	}
	

	while(1)
	{
		//接收成功return 字节数，失败return -1
		/* 
			s:socket描述符
			*mem:接收数据的缓冲区
			len:缓冲区大小
			flag:0、MSG_PEEK、MSG_WAITALL、MSG_DONTWAIT
		*/
		bytes_received = recv(sock, recv_data, BUFSZ - 1, 0);	//此处要确保有足够的空间保存\0，所以（BUFSZ - 1）
		
		//server 断开则显示 received error, close the socket
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
		
		//网络接收到的数据是没有 \0 结束符的，因为网络协议（比如 TCP 或 UDP）传输的是原始的字节流，没有特定的结束符
		recv_data[bytes_received] = '\0';
		
		if(strncmp(recv_data, "q", 1) == 0 || strncmp(recv_data, "Q", 1) == 0)		//" " 、''
		{
			closesocket(sock);	//对于一个TCP连接，通常来说，关闭套接字就意味着“断开连接”,4次挥手
			rt_kprintf("disconnect!\n");
			
			rt_free(recv_data);
			break;
		}
		else
		{
			rt_kprintf("recv_data = %s\n", recv_data);
		}
		
		/*
			s:socket描述符
			*dataptr:发送的数据buf
			size:发送数据的长度
			flags: 0（默认行为）、MSG_DONTWAIT（非阻塞发送）、MSG_NOSIGNAL（防止 SIGPIPE）、MSG_OOB（发送带外数据）等
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

static tcp_struct *tcp_info;		//只创建了指针，需要手动分配内存


void tcpclientinit_(void *parameter)
{
	char *url;
	int port;
	struct addrinfo hints, *res;
	int ret;
	int sock, bytes_received;
	char *recv_data;
	char send_data[] = "receive successful from rtt\n";
	
	
	//获取url和port
	url = ((tcp_struct *)parameter)->_url;//(char *)((tcp_struct *)parameter->_url);  //parameter
	port = (int)strtoul(((tcp_struct *)parameter)->_port, 0, 10);//5000; //(int)strtoul(agrv[2], 0, 10);
	rt_kprintf("port:%d\n",port);
	
	/* 设置 hints（提示、建议） 结构体，指定希望返回的地址族和协议 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;			//ipv4
	hints.ai_socktype = SOCK_STREAM;	//tcp
	hints.ai_protocol = 0;				//自动选择协议 此处根据SOCK_STREAM选择
	
	/*
		nodename:协议地址
		servname:服务名/端口号（"http/port"）
		struct *hint
		struct **res
	*/
	ret = getaddrinfo(url, ((tcp_struct *)parameter)->_port, &hints, &res); //agrv[2]
	
	if(ret != RT_EOK)
	{
		rt_kprintf("getaddrinfo error!\n");	// \n换行  \r 会将光标移动到行首
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
		domain:协议族
		type:套接字类型
		protocol:协议，为0则表示根据套接字类型进行自动选择
		
		return:socket 描述符 / 句柄
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
		freeaddrinfo(res);		// 解析完成后释放资源
	}

	while(1)
	{
		//接收成功return 字节数，失败return -1
		/* 
			s:socket描述符
			*mem:接收数据的缓冲区
			len:缓冲区大小
			flag:0、MSG_PEEK、MSG_WAITALL、MSG_DONTWAIT
		*/
		bytes_received = recv(sock, recv_data, BUFSZ - 1, 0);	//此处要确保有足够的空间保存\0，所以（BUFSZ - 1）
		
		//server 断开则显示 received error, close the socket
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
		
		//网络接收到的数据是没有 \0 结束符的，因为网络协议（比如 TCP 或 UDP）传输的是原始的字节流，没有特定的结束符
		recv_data[bytes_received] = '\0';
		
		if(strncmp(recv_data, "q", 1) == 0 || strncmp(recv_data, "Q", 1) == 0)		//" " 、''
		{
			closesocket(sock);	//对于一个TCP连接，通常来说，关闭套接字就意味着“断开连接”,4次挥手
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
			s:socket描述符
			*dataptr:发送的数据buf
			size:发送数据的长度
			flags: 0（默认行为）、MSG_DONTWAIT（非阻塞发送）、MSG_NOSIGNAL（防止 SIGPIPE）、MSG_OOB（发送带外数据）等
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
	
	if(tcp_thread != NULL) return;		//防止开启多个，close 或 err 都需要 tcp_thread = NULL;
	if(agrc < 3)
	{
//		printf("testtcp IP			PORT\r\n");
		printf("example :TCPclient 192.168.0.1 5000\r\n");
		return ;
	}
	
	//因为创建的只是指针static tcp_struct *tcp_info; 所以申请内存|| static tcp_struct tcp_info则是分配了局部变量。
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
	// 485只做输出

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

