#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>	
#include <sys/time.h>
#include <sys/signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>

static struct timeval _t0;
static unsigned long long bytesent = 0;
int connfd[10];
int cmd_fd;
struct sockaddr_in serv;
struct sockaddr_in recv_addr;
char buf[100];

//int count = 0;

double tv2s(struct timeval *ptv) {
	return 1.0*(ptv->tv_sec) + 0.000001*(ptv->tv_usec);
}

void handler(int s) {

    cmd_fd = socket(AF_INET, SOCK_STREAM, 0);
    connect(cmd_fd, (struct sockaddr*)&serv, sizeof(serv));

    char msg[] = "/report\n";
    write(cmd_fd, msg, strlen(msg));
    //fprintf(stderr, "write: %ld", wn);
    
    read(cmd_fd, buf, sizeof(buf));
    //fprintf(stderr, "read: %ld", rn);
    fprintf(stderr, "%s\n", buf);

    close(cmd_fd);
	/* struct timeval _t1;
	double t0, t1;
	gettimeofday(&_t1, NULL);
	t0 = tv2s(&_t0);
	t1 = tv2s(&_t1);
	fprintf(stderr, "\n%lu.%06lu %llu bytes sent in %.6fs (%.6f Mbps; %.6f MBps)\n",
		_t1.tv_sec, _t1.tv_usec, bytesent, t1-t0, 8.0*(bytesent/1000000.0)/(t1-t0), (bytesent/1000000.0)/(t1-t0)); */

    for(int i = 0; i < 10; i++){
        close(connfd[i]);
    }
	//printf("pkt count: %d\n", count);
	exit(0);
}

int main(int argc, char *argv[]) {
    
	signal(SIGINT,  handler);
	signal(SIGTERM, handler);

    bzero(&serv, sizeof(serv));
    bzero(&recv_addr, sizeof(recv_addr));
    cmd_fd = socket(AF_INET, SOCK_STREAM, 0);

	int flag = 1;
	
	char* ip = argv[1];
    serv.sin_addr.s_addr = inet_addr(ip);
    serv.sin_port = htons(atoi(argv[2]));
    serv.sin_family = AF_INET;

    recv_addr.sin_addr.s_addr = inet_addr(ip);
    recv_addr.sin_port = htons(atoi(argv[2])+1);
    recv_addr.sin_family = AF_INET;
    //printf("argv[3] = %s\n", argv[3]);
    //setsockopt(cmd_fd, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));
    int is_connect = connect(cmd_fd, (struct sockaddr*)&serv, sizeof(serv));
	//printf("%d\n", is_connect);

	//printf("%d\n", so);
    char msg[] = "/reset\n";
    write(cmd_fd, msg, strlen(msg));
	gettimeofday(&_t0, NULL);
    read(cmd_fd, buf, sizeof(buf));
    buf[strlen(buf)-1] = '\0';
    printf("%s\n", buf);
    close(cmd_fd);

	//struct timeval start, end;
	int size = 1e6;
    //printf("size: %d\n", size);
	char message[size];
	//printf("%d\n", size);
    //printf("argc: %d\n", argc);
	memset(message, 'a', sizeof(message));

    for(int i = 0; i < 10; i++){
        connfd[i] = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(connfd[i], IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));
        connect(connfd[i], (struct sockaddr*)&recv_addr, sizeof(recv_addr));
    }   
    
	while(1) {
		
		//gettimeofday(&start, NULL);
        for(int i = 0; i < 10; i++){
            ssize_t nbytes = send(connfd[i], message, sizeof(message), 0);
            bytesent += nbytes;
        }
		//gettimeofday(&end, NULL);

		//int t_len = (end.tv_sec-start.tv_sec)*1e9 + (end.tv_usec-start.tv_usec)*1e3;
		//printf("%d\n", t_len);

		struct timespec t = { 0, 1e7};
		//count++;
		//printf("%d\n", count);
		nanosleep(&t, NULL);
	}

	return 0;
}