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

//int count = 0;

double tv2s(struct timeval *ptv) {
	return 1.0*(ptv->tv_sec) + 0.000001*(ptv->tv_usec);
}

void handler(int s) {
	struct timeval _t1;
	double t0, t1;
	gettimeofday(&_t1, NULL);
	t0 = tv2s(&_t0);
	t1 = tv2s(&_t1);
	fprintf(stderr, "\n%lu.%06lu %llu bytes sent in %.6fs (%.6f Mbps; %.6f MBps)\n",
		_t1.tv_sec, _t1.tv_usec, bytesent, t1-t0, 8.0*(bytesent/1000000.0)/(t1-t0), (bytesent/1000000.0)/(t1-t0));

	//printf("pkt count: %d\n", count);
	exit(0);
}

int main(int argc, char *argv[]) {

	signal(SIGINT,  handler);
	signal(SIGTERM, handler);

	struct sockaddr_in serv;
    bzero(&serv, sizeof(serv));
    int s_fd = socket(AF_INET, SOCK_STREAM, 0);

	int flag = 1;
	//int mss = 10000;
	int so = setsockopt(s_fd, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));
	//int so = setsockopt(s_fd, IPPROTO_TCP, TCP_MAXSEG, (void*)&mss, sizeof(mss));
	//printf("so: %d\n", so);

	char* ip = "127.0.0.1";
    serv.sin_addr.s_addr = inet_addr(ip);
    serv.sin_port = htons(10003);
    serv.sin_family = AF_INET;

    int is_connect = connect(s_fd, (struct sockaddr*)&serv, sizeof(serv));
	//printf("%d\n", is_connect);

	//printf("%d\n", so);

	
	gettimeofday(&_t0, NULL);

	//struct timeval start, end;
	int size = 2 * 1e5 * atof(argv[1]) *  ((65549 - 66) / 65549.0);
	char message[size];
	//printf("%d\n", size);

	memset(message, 'a', sizeof(message));

	while(1) {
		
		//gettimeofday(&start, NULL);
		ssize_t nbytes = send(s_fd, message, sizeof(message), 0);
		bytesent += nbytes;
		//gettimeofday(&end, NULL);

		//int t_len = (end.tv_sec-start.tv_sec)*1e9 + (end.tv_usec-start.tv_usec)*1e3;
		//printf("%d\n", t_len);

		struct timespec t = { 0, 2 * 1e8};
		//count++;
		//printf("%d\n", count);
		nanosleep(&t, NULL);
	}

	return 0;
}
