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

//NTP Packet header
#define PORTNUM 123
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4
#define PREC -6

#define NTP_LEN 48
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
	int8_t precision;
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

//This function build a entire NTP client packet
void Build_Packet(struct ntppacket* packet);
//Initialize a socket
int32_t  Init_Soacket(int para, char *str[], struct sockaddr_in server_sockaddr);
//Send the initial packet
void Send_NTP_Packet(struct ntppacket* packet, int32_t size, int32_t clientfd, struct sockaddr_in server_sockaddr);
//Receive the data that server returned
void Receive_NTP_Packet(struct ntppacket* packet, int32_t size, int32_t clientfd, struct sockaddr_in server_sockaddr);

int main(int argc, char *argv[])
{
	struct ntppacket sdpacket,rcvpacket;
	struct sockaddr_in s_sockaddr;
	int32_t fd,sin_size;;
	Build_Packet(&sdpacket);
	fd = Init_Soacket(argc,argv,s_sockaddr);
	sin_size = sizeof(struct sockaddr);
	Send_NTP_Packet(&sdpacket,sin_size,fd,s_sockaddr);
	Receive_NTP_Packet(&rcvpacket,sin_size,fd,s_sockaddr);
	//close
	close(fd);
	exit(0);
}


void Build_Packet(struct ntppacket* packet)
{
	time_t time_Local;
	//header
	packet->li_vn_mode = htonl((LI<<6)|(VN<<3)|MODE);
	packet->stratum = htonl(STRATUM);
	packet->poll = htonl(POLL);
	packet->precision = htonl(PREC&0xff);

	packet->root_delay = htonl(1<<16);
	packet->root_dispersion = htonl(1<<16);
	packet->reference_identifier = 0;

	//no need to set other three time stamps, just stay zero
	time(&time_Local);
	packet->transmit_timestamp.coarse = htonl(JAN_1970+time_Local);
	packet->transmit_timestamp.fine = htonl(NTPFRAC(time_Local));

}

int32_t Init_Soacket(int para, char *str[], struct sockaddr_in server_sockaddr)
{
	int32_t clientfd;
	struct hostent *host;
	struct in_addr aimaddr;

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
	if((clientfd = socket(AF_INET,SOCK_DGRAM,0)) == -1)
	{
		fprintf(stderr,"An Error Occured:%s, Create Failed!\n",strerror(errno));
		exit(1);
	}

	//initialize the sockaddr
	memset(&server_sockaddr,0,sizeof(struct sockaddr_in));

	//fill the sockaddr with server information that you want to connect
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(PORTNUM);
	/*server_sockaddr.sin_addr = *((struct in_addr *)host->h_addr);*/
	return(clientfd);
}

void Send_NTP_Packet(struct ntppacket* packet, int32_t size, int32_t clientfd, struct sockaddr_in server_sockaddr)
{
	if((sendto(clientfd,packet,sizeof(packet),0,(struct sockaddr*)&server_sockaddr,size) == -1))
	{
		fprintf(stderr,"An Error Occurred:%s, Send Failed!\n",strerror(errno));
		exit(1);
	}
	else
		printf("Send Packet successful!");
}

void Receive_NTP_Packet(struct ntppacket* packet, int32_t size, int32_t clientfd, struct sockaddr_in server_sockaddr)
{
	if((recvfrom(clientfd,packet,NTP_LEN*8,0,(struct sockaddr*)&server_sockaddr,&size) == -1))
		{
			fprintf(stderr,"An Error Occurred:%s, Send Failed!\n",strerror(errno));
			exit(1);
		}
		else
			printf("%a",ntohl(packet->li_vn_mode));
}
