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
#define SIZE 1440

typedef struct{
	int seq;
	int total;
	int name;
	unsigned char msg[SIZE+1];
} packet;

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
	sin.sin_port = htons(atoi(argv[argc-2]));
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

	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		printf("socket error\n");

	//printf("Successfully build socket\n");

	//connect(sockfd, (struct sockaddr*)&sin, sizeof(sin));
	int n = 400 * 1024;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));

	struct timeval timeout;
	int tmp_sz = sizeof(struct timeval);
	timeout.tv_sec = 0;
	timeout.tv_usec = 300000;
	//setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
	//setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(int));

	char file_name[100];
    memset(file_name, 0, 100);
    strcat(file_name, file_directory);
    strcat(file_name, "/");

	struct sockaddr_in csin;
	socklen_t csinlen = sizeof(csin);
	int slen, rlen;
	//unsigned char msg[SIZE+1];

	for(int i = start; i <= end; i++){
		//printf("current file: %d\n", i);
		/* if(i == end+1){
			packet *END = (packet*)calloc(1, sizeof(packet));
			END->seq = -1;
			END->total = -1;
			END->name = i;
			while(1){
				slen = sendto(sockfd, END, sizeof(packet), 0, (struct sockaddr*)&sin, sinlen);
				if(slen < 0)
					exit(0);
			}
		} */

		//memset(file_name, 0, 100);
		//strcat(file_name, file_directory);
		//strcat(file_name, "/");
        file_name[strlen(file_directory)+1] = '\0';
		strcat(file_name, namelist[i]->d_name);
        //printf("filename: %s\n", file_name+7);
		FILE* entry_file = fopen(file_name, "rb");

        
		fseek(entry_file, 0L, SEEK_END);
		ssize_t sz = ftell(entry_file);	
		int file_size = sz;	
		fseek(entry_file, 0L, SEEK_SET);

		/* packet* pkt = calloc(1, sizeof(packet));
		pkt->seq = 1;
		pkt->total = file_size / SIZE;
		pkt->name = i - 2;
		if(pkt->total * SIZE < file_size)
			pkt->total++; */
		
		/* int total = file_size / SIZE;
		if(total * SIZE < file_size)
			total++; */

		packet* pkt = calloc(1, sizeof(packet));
		pkt->seq = 1;
		pkt->total = file_size / SIZE;
		pkt->name = i - 2;
		if(pkt->total * SIZE < file_size)
			pkt->total++;
		
		while(sz > 0){
			memset(pkt->msg, 0, SIZE);
			size_t nbytes = fread(pkt->msg, sizeof(unsigned char), SIZE, entry_file);

            for(int j = 0; j < 42; j++){
			    sendto(sockfd, pkt, sizeof(packet), 0, (struct sockaddr*)&sin, sinlen);
                usleep(40);
            }
            
            //usleep(4000);

			pkt->seq++;
			sz -= SIZE;
			//fseek(entry_file, SIZE, SEEK_CUR);
		}
			
		//printf("send file name: %s , file size: %d\n", file_name, file_size);
		fclose(entry_file);

		//usleep(10000);
		printf("send file name: %s , file size: %d\n", file_name, file_size);

		//printf("file name: %s\n", file_name);
	
		//usleep(1000);
		//sleep(10);
	}

    usleep(1500000);
	close(sockfd);
    //wait(NULL);
}