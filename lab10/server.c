/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define SIZE 1440
#define IPPROTO_XDP 180

#define err_quit(m) { perror(m); exit(-1); }

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
	int sockfd;
	struct sockaddr_in sin;
	char* file_directory = argv[1];

	int file_num = atoi(argv[2]);
	DIR* fd;
	struct dirent* in_file;

	if(argc < 2) {
		return -fprintf(stderr, "usage: %s ... <port>\n", argv[0]);
	}

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	//int port = atoi(argv[argc-1]);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(10001);
	sin.sin_addr.s_addr = inet_addr(argv[argc-1]);

	struct stat st = {0};
	if (stat(file_directory, &st) == -1) {
    	mkdir(file_directory, 0777);
	}

	pid_t pid;
	int start = 0;
	//int end = 499;
    int end = 999;

	if((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_XDP)) < 0)
		err_quit("socket");

	int on = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

	bind(sockfd, (struct sockaddr*)&sin, sizeof(sin));

	//printf("aaa\n");
	char file_name[7];
	char output_file[100];
    memset(output_file, 0, 100);
	strcat(file_directory, "/");
    strcat(output_file, file_directory);
	//unsigned char ACK[7];	

	int rlen, slen;
	struct sockaddr_in csin;
	bzero(&csin, sizeof(csin));
	socklen_t csinlen = sizeof(csin);

	for(int i = start; i <= end; i++){

		sprintf(file_name, "%06d", i);
        output_file[strlen(file_directory)] = '\0';
		strcat(output_file, file_name);
		FILE* entry_file = fopen(output_file, "wb");

		int total = 0;
        int ack = 1;
        int count = 0;

		unsigned char msg[2*SIZE];
        unsigned char* content = calloc(32*1024, sizeof(unsigned char));

		while(1){
			memset(msg, 0, 2 * SIZE);
			
			//printf("aaaa\n");
			rlen = recvfrom(sockfd, msg, sizeof(msg), 0, (struct sockaddr*)&csin, &csinlen);
			//printf("received!!\n");
			struct iphdr* iph = (struct iphdr*)msg;
			//struct udphdr* udph = (struct udphdr*)(msg + sizeof(struct iphdr));
			packet* pkt = (packet*)(msg + sizeof(struct iphdr) /* + sizeof(struct udphdr)*/ ) ;
            
			if(iph->version != 4 || iph->saddr != csin.sin_addr.s_addr || htons(iph->tot_len) < (iph->ihl<<1)+8){
				printf("something is wrong with the packet\n");
				continue;
			}

			/* if(udph->dest != htons(10001) || udph->source != htons(10000))
				continue; */

			//packet* pkt = (packet*)(msg + (iph->ihl<<2) + 8);
			//printf("iph->ihl: %u\n", iph->ihl);	

			//printf("pkt->name: %d, pkt->seq: %d, pkt->total: %d\n", pkt->name, pkt->seq, pkt->total);		
		
            /* if(pkt->name != i){
                //printf("pkt name: %d, i: %d\n", pkt->name, i);
				continue;
			} */

			if(ack == pkt->seq){
				//printf("current file: %d, receive file: %d\n", i, pkt->name);
				//printf("pkt seq: %d, pkt total: %d\n", pkt->seq, pkt->total);
				
                strcat(content, pkt->msg);
				//printf("msg size: %lu\n", strlen(pkt->msg));
                ack++;
			}

            if(pkt->total == pkt->seq){
				//printf("pkt->total: %d, pkt->seq: %d\n", pkt->total, pkt->seq);
				usleep(20000);
				break;
			}
            
		}
	
		ssize_t nbytes = fwrite(content, sizeof(unsigned char), strlen(content), entry_file);
		printf("Writing file: %s with size: %ld\n", output_file, nbytes);

		fclose(entry_file);
		//usleep(10000);
	}

	close(sockfd);
    //wait(NULL);
}