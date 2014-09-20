/***********************************************************************
NTP Client
Send NTP request and receive current time.
**********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>

//NTP Packet header
#define PORTNUM 123
#define LI 0x03
#define VN 0x04
#define MODE 0x03
#define STRATUM 0x00
#define POLL 0x04
#define PREC 0xfa

#define NTP_LEN 48
#define NTP_LEN_RCV 384
#define JAN_1970 0x83aa7e80
#define NTPFRAC(x) (4294*(x)+((1981*(x))>>11))
#define USEC(x) (((x)>>12-759*((((X)>>10)+32768)>>16))

//NTP 64bit format
struct ntptime
{
	uint32_t coarse;
	uint32_t fine;
};

struct ntppacket
{
	//2bits LI,3bits VN,3bits Mode
	uint8_t li_vn_mode;
	//8bits Stratum
	uint8_t stratum;
	//8bits Poll(with signed)
	int8_t poll;
	//8Bits Precision(with signed)
	uint8_t precision;
	//32bits root delay(with signed)
	int32_t root_delay;
	//32bits root dispersion
	int32_t root_dispersion;
	//32bits reference identifier
	int32_t reference_identifier;
	//64bit reference time stamp
	struct ntptime reference_timestamp;
	//64bit originate time stamp
	struct ntptime originage_timestamp;
	//64bit receive time stamp
	struct ntptime receive_timestamp;
	//64bit transmit time stamp
	struct ntptime transmit_timestamp;
};

struct sockpack
{
	int32_t fd;
	struct sockaddr_in s_sockaddr;
};

//This function build a entire NTP client packet
void Build_Packet(struct ntppacket* packet, uint8_t *data);
//Initialize a socket
struct sockpack  Init_Soacket(int para, char *str[]);
//Send the initial packet
void Send_NTP_Packet(uint8_t *data, int32_t size, int32_t clientfd, struct sockaddr_in server_sockaddr);
//Receive the data that server returned
void Receive_NTP_Packet(uint8_t *data, int32_t *size, int32_t clientfd, struct sockaddr_in server_sockaddr);

int main(int argc, char *argv[])
{
	struct ntppacket *sdpacket,*rcvpacket;
	struct sockpack sdpack;
	uint8_t sddata[NTP_LEN],rcvdata[NTP_LEN*8];
	int32_t sin_size;

	sdpacket = (struct ntppacket *)malloc(sizeof(struct ntppacket));
	rcvpacket = (struct ntppacket *)malloc(sizeof(struct ntppacket));
	Build_Packet(sdpacket,sddata);
	sdpack = Init_Soacket(argc,argv);
	sin_size = sizeof(struct sockaddr);
	Send_NTP_Packet(sddata,sin_size,sdpack.fd,sdpack.s_sockaddr);
	Receive_NTP_Packet(rcvdata,&sin_size,sdpack.fd,sdpack.s_sockaddr);
	//close
	close(sdpack.fd);
	exit(0);
}


void Build_Packet(struct ntppacket* packet, uint8_t *data)
{
	time_t time_Local;
	uint32_t temp;
	//header
	packet->li_vn_mode = (uint8_t)((uint8_t)MODE|(uint8_t)(VN<<3)|(uint8_t)(LI<<6));
	packet->stratum = (uint8_t)STRATUM;
	packet->poll = (int8_t)POLL;
	packet->precision = (uint8_t)PREC;

	packet->root_delay = (int32_t)(1<<16);
	packet->root_dispersion = (int32_t)(1<<16);
	packet->reference_identifier = 0;

	//no need to set other three time stamps, just stay zero
	time(&time_Local);
	packet->transmit_timestamp.coarse = (int32_t)(JAN_1970+time_Local);
	packet->transmit_timestamp.fine = (int32_t)(NTPFRAC(time_Local));

	//build packet
	temp = htonl(((packet -> li_vn_mode) << 24)|((packet -> stratum) << 16)|((packet -> poll) << 8)|(packet -> precision));
	memcpy(data,&temp,sizeof(temp));
	temp = htonl(packet->root_delay);
	memcpy(data+4,&temp,sizeof(temp));
	temp = htonl(packet->root_dispersion);
	memcpy(data+8,&temp,sizeof(temp));
	temp = htonl(packet->reference_identifier);
	memcpy(data+12,&temp,sizeof(temp));
	temp = htonl(packet->transmit_timestamp.coarse);
	memcpy(data+40,&temp,sizeof(temp));
	temp = htonl(packet->transmit_timestamp.fine);
	memcpy(data+44,&temp,sizeof(temp));

}

struct sockpack Init_Soacket(int para, char *str[])
{
	int32_t clientfd;
	struct sockaddr_in server_sockaddr;
	struct sockpack sdpack;
	//struct hostent *host;
	//struct in_addr aimaddr;

	//initialize the sockaddr
	memset(&server_sockaddr,0,sizeof(struct sockaddr_in));

	//obtain host name/IP from argv
	if(para != 2)
	{
		printf("Please input your host name/IP address as a parameter!\n");
		exit(1);
	}
	if((inet_aton(str[1],&server_sockaddr.sin_addr)) == -1)
	{
		printf("Get IP error!\n");
		exit(1);
	}
	/*if((host = gethostbyname(argv[1])) == NULL)
	{
		printf("Get host error!\n");
		exit(1);
	}*/

	//create a socket
	if((clientfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == -1)
	{
		fprintf(stderr,"An Error Occured:%s, Create Failed!\n",strerror(errno));
		exit(1);
	}

	//fill the sockaddr with server information that you want to connect
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(PORTNUM);
	/*server_sockaddr.sin_addr = *((struct in_addr *)host->h_addr);*/

	//return the socket packet
	sdpack.fd = clientfd;
	sdpack.s_sockaddr = server_sockaddr;
	return(sdpack);
}

void Send_NTP_Packet(uint8_t *data, int32_t size, int32_t clientfd, struct sockaddr_in server_sockaddr)
{
	if((sendto(clientfd,data,NTP_LEN,0,(struct sockaddr*)&server_sockaddr,size) == -1))
	{
		fprintf(stderr,"An Error Occurred:%s, Send Failed!\n",strerror(errno));
		exit(1);
	}
	else
		printf("Send Packet successful!\n");
}

void Receive_NTP_Packet(uint8_t *data, int32_t *size, int32_t clientfd, struct sockaddr_in server_sockaddr)
{
	struct sockaddr_in client_sockaddr;
	if((recvfrom(clientfd,data,NTP_LEN*8,0,(struct sockaddr*)&client_sockaddr,size) == -1))
		{
			fprintf(stderr,"An Error Occurred:%s, Receive Failed!\n",strerror(errno));
			exit(1);
		}
		else
		{
			fprintf(stdout,"From %s ",inet_ntoa(client_sockaddr.sin_addr));
			int32_t temp;
			memcpy(&temp,data,4);
			fprintf(stdout,"LI_VN_MODE_strtum_poll_precision:%x\n",ntohl(temp));
			memcpy(&temp,data+4,4);
			fprintf(stdout,"root_delay:%x\n",ntohl(temp));
			memcpy(&temp,data+8,4);
			fprintf(stdout,"root_dispersion:%x\n",ntohl(temp));
			memcpy(&temp,data+12,4);
			fprintf(stdout,"reference_identifier:%x\n",ntohl(temp));
			memcpy(&temp,data+16,4);
			fprintf(stdout,"reference_timestamp_coarse:%x\n",ntohl(temp));
			memcpy(&temp,data+20,4);
			fprintf(stdout,"reference_timestamp_fine:%x\n",ntohl(temp));
			memcpy(&temp,data+24,4);
			fprintf(stdout,"originage_timestamp_coarse:%x\n",ntohl(temp));
			memcpy(&temp,data+28,4);
			fprintf(stdout,"originage_timestamp_fine:%x\n",ntohl(temp));
			memcpy(&temp,data+32,4);
			fprintf(stdout,"receive_timestamp_coarse:%x\n",ntohl(temp));
			memcpy(&temp,data+36,4);
			fprintf(stdout,"receive_timestamp_fine:%x\n",ntohl(temp));
			memcpy(&temp,data+40,4);
			fprintf(stdout,"receive_timestamp_coarse:%x\n",ntohl(temp));
			memcpy(&temp,data+44,4);
			fprintf(stdout,"receive_timestamp_fine:%x\n",ntohl(temp));
		}
}
