/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <math.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#define SIZE 1440
#define IPPROTO_XDP 180

typedef struct{
	uint8_t seq;
	uint8_t total;
	//uint16_t name;
	unsigned char msg[SIZE+1];
} packet;

unsigned short cksum(void *in, int sz){
	long sum = 0;
	unsigned short *ptr = (unsigned short*)in;

	for(; sz > 1; sz -=2) 
		sum += *ptr++;
	
	if(sz > 0)
		sum += *((unsigned char*)ptr);

	while(sum >> 16) 
		sum = (sum & 0xffff) + (sum >> 16);
	
	return ~sum;
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	char* file_directory = argv[1];
	int num;
	struct dirent **namelist;
	//unsigned char msg[1024];
	num = scandir(file_directory, &namelist, 0, alphasort);

	int file_num = atoi(argv[2]);

	int sockfd;
	//unsigned char buf[SIZE+1];
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	socklen_t sinlen = sizeof(sin);
	
	DIR* fd;
	struct dirent* in_file;
	
	sin.sin_family = AF_INET;
	sin.sin_port = htons(10001);
	sin.sin_addr.s_addr = inet_addr(argv[argc-1]);

	pid_t pid;
	int start = 2;
	//int end = 501;
    int end = 1001;

	/* pid = fork();
	if(pid == 0){
		//p2
		sin.sin_port++;
		start = end + 1;
		end = 1001;
	} */

	if((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_XDP)) < 0)
		printf("socket error\n");

	//printf("Successfully build socket\n");

	//connect(sockfd, (struct sockaddr*)&sin, sizeof(sin));
	int on = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

	connect(sockfd, (struct sockaddr*)&sin, sizeof(sin));

	unsigned char datagram[1500], *data_ptr;
	memset(datagram, 0, 1500);
	struct iphdr *iph = (struct iphdr*)datagram;
	//struct udphdr *udph = (struct udphdr*)(datagram + (iph->ihl<<2));
	data_ptr = datagram + sizeof(struct iphdr) /* + sizeof(struct udphdr) */;
	//data_ptr = datagram + sizeof(struct iphdr);
	
	// IP Header
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + /* sizeof(struct udphdr) + */ sizeof(packet);
	iph->id = htonl(33333);	//Id of this packet
	iph->frag_off = 0;
	iph->ttl = 255;
	iph->protocol = IPPROTO_XDP;
	iph->check = 0;		//Set to 0 before calculating checksum
	iph->saddr = sin.sin_addr.s_addr;	//Spoof the source ip address
	iph->daddr = sin.sin_addr.s_addr;
	
	iph->check = cksum(datagram, (iph->ihl << 2));

	// UDP Header
	/* udph->len = sizeof(struct udphdr) + sizeof(packet);
	udph->source = htons(10000);
	udph->dest = htons(10001);
	udph->check = 0; */

	char file_name[100];
    memset(file_name, 0, 100);
    strcat(file_name, file_directory);
    strcat(file_name, "/");

	struct sockaddr_in csin;
	socklen_t csinlen = sizeof(csin);
	int slen, rlen;
	//unsigned char msg[SIZE+1];

	for(int i = start; i <= end; i++){

        file_name[strlen(file_directory)+1] = '\0';
		strcat(file_name, namelist[i]->d_name);
		FILE* entry_file = fopen(file_name, "rb");

        
		fseek(entry_file, 0L, SEEK_END);
		ssize_t sz = ftell(entry_file);	
		int file_size = sz;	
		fseek(entry_file, 0L, SEEK_SET);

		packet* pkt = calloc(1, sizeof(packet));
		pkt->seq = 1;
		pkt->total = file_size / SIZE;
		//pkt->name = i - 2;
		if(pkt->total * SIZE < file_size)
			pkt->total++;
		
		//printf("pkt total: %d\n", pkt->total);

		while(sz > 0){
			memset(pkt->msg, 0, SIZE);
			size_t nbytes = fread(pkt->msg, sizeof(unsigned char), SIZE, entry_file);

			//printf("read size: %lu\n", strlen(pkt->msg));
			unsigned char* buf = (unsigned char*)pkt;
			strncpy(data_ptr, buf, sizeof(packet));
			slen = sendto(sockfd, datagram, sizeof(datagram), 0, (struct sockaddr*)&sin, sinlen);
			if(slen < 0)
				printf("send error\n");
			usleep(10000);
			
			
		
			/* for(int j = 0; j < 2; j++){
				slen = sendto(sockfd, datagram, sizeof(datagram), 0, (struct sockaddr*)&sin, sinlen);
				usleep(10000);
			} */
		
            /* for(int j = 0; j < 2; j++){
			    slen = sendto(sockfd, datagram, sizeof(datagram), 0, (struct sockaddr*)&sin, sinlen);
                //printf("slen: %d\n", slen);
				usleep(100000);
            } */

			pkt->seq++;
			sz -= SIZE;
			//printf("pkt total: %d\n", pkt->total);
		}
		fclose(entry_file);
		printf("send file name: %s , file size: %d\n", file_name, file_size);
	}

    usleep(1500000);
	close(sockfd);
    //wait(NULL);
}