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
#define SIZE 1440

#define err_quit(m) { perror(m); exit(-1); }

typedef struct{
	int seq;
	int total;
	int name;
	unsigned char msg[SIZE+1];
} packet;

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
	int port = atoi(argv[argc-1]);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	struct stat st = {0};
	if (stat(file_directory, &st) == -1) {
    	mkdir(file_directory, 0777);
	}

	pid_t pid;
	int start = 0;
	//int end = 499;
    int end = 999;

	/* pid = fork();
	if(pid == 0){
		//p2
		sin.sin_port++;
		start = end + 1;
		end = 999;
	} */

	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	if(bind(sockfd, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		err_quit("bind");
	
	//printf("Server using address %s:%u\n", inet_ntoa(sin.sin_addr), sin.sin_port);

	int n = 400 * 1024;
	//int tmp;
	//int tmp_len = sizeof(int);
	//setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));
	//int timeout = 10;
	struct timeval timeout, tmp;
	int tmp_sz = sizeof(struct timeval);
	timeout.tv_sec = 0;
	timeout.tv_usec = 300000;
	//setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)); 

	//setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(int));
	//getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tmp, &tmp_sz);
	//printf("timeout: %ld:%ld\n", tmp.tv_sec, tmp.tv_usec);

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

		//memset(output_file, 0, 100);
		//memset(content, 0, 32*1024);
		sprintf(file_name, "%06d", i);
		//printf("Current file: %s\n", file_name);		
		//strcat(output_file, file_directory);
        output_file[strlen(file_directory)] = '\0';
		strcat(output_file, file_name);
		FILE* entry_file = fopen(output_file, "wb");
		//setvbuf(entry_file, NULL, _IONBF, 0);

		int total = 0;
        int ack = 1;
        int count = 0;

		unsigned char msg[2*SIZE];
        unsigned char* content = calloc(32*1024, sizeof(unsigned char));

		//printf("recv name: %d\n", i);
		while(1){
			//unsigned char* msg = calloc(SIZE+12, sizeof(unsigned char));
			memset(msg, 0, 2 * SIZE);
			/* if( (rlen = recvfrom(sockfd, msg, sizeof(msg), 0, (struct sockaddr*)&csin, &csinlen)) < 0){
				//printf("timeout\n");
				sprintf(ACK, "%03d", ack);
				strncat(ACK, file_name+3, 3);
				sendto(sockfd, ACK, sizeof(ACK), 0, (struct sockaddr*)&csin, csinlen);
				//usleep(20000);
				//free(msg);
				continue;
			} */
			rlen = recvfrom(sockfd, msg, sizeof(msg), 0, (struct sockaddr*)&csin, &csinlen);
			
			packet* pkt = (packet*)msg;
            
			//printf("current file: %d, receive file: %d\n", i, pkt->name);
			/* if(i != pkt->name){
				//printf("finish current file\n");
				break;
			} */

            if(pkt->name != i)
                continue;

			if(ack == pkt->seq){
                strcat(content, pkt->msg);
				//ssize_t nbytes = fwrite(pkt->msg, sizeof(unsigned char), strlen(pkt->msg), entry_file);
                //count += nbytes;
                ack++;
				//printf("file_name: %d, current seq/total: %d/%d, size: %ld/%ld\n", pkt->name, pkt->seq, pkt->total, strlen(pkt->msg), strlen(buf[pkt->seq]));
			}

            if(pkt->total == pkt->seq)
                break;
            
		}
		
		//printf("finish receiving\n");
		/* for(int j = 1; j < 32; j++){
			printf("size: %ld\n", strlen(buf[j]));
		} */

		ssize_t nbytes = fwrite(content, sizeof(unsigned char), strlen(content), entry_file);
		printf("Writing file: %s with size: %ld\n", output_file, nbytes);
		//fwrite(buf, sizeof(unsigned char), strlen(buf), entry_file);
		//printf("Finish Writing...\n");
		fclose(entry_file);
		//usleep(10000);
	}

	close(sockfd);
    //wait(NULL);
}