#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>

#define MAXLINE 1000

static struct timeval _t, last_reset_time;

double tv2s(struct timeval *ptv) {
	return 1.0*(ptv->tv_sec) + 0.000001*(ptv->tv_usec);
}

int main(int argc, char **argv){
    int                     i, maxi, maxfd, listenfd, connfd, sockfd, recvfd, cmdfd = -1;
    int                     nready, client[FD_SETSIZE-1];
    long long               n, byte_count;
    fd_set                  rset, allset;
    char                    buf[MAXLINE], time_buf[80];
    socklen_t               clilen;
    struct sockaddr_in      cliaddr, servaddr, recvaddr;
    //time_t                  rawtime,  elapsed_time;
    double t, elapsed_time;
    struct tm* timeinfo;
    char  msg[2000];
    struct sockaddr_in client_addr[FD_SETSIZE-1];
    in_port_t server_port[FD_SETSIZE-1];

    //bool dc_queue[FD_SETSIZE];
    //int dc_client;

    //printf("%u\n", FD_SETSIZE);
    signal(SIGPIPE, SIG_IGN);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(atoi(argv[1]));

    bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    listen(listenfd, MAXLINE);

    recvfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&recvaddr, sizeof(recvaddr));
    recvaddr.sin_family      = AF_INET;
    recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    recvaddr.sin_port        = htons(atoi(argv[1])+1);

    bind(recvfd, (struct sockaddr *) &recvaddr, sizeof(recvaddr));
    listen(recvfd, MAXLINE);


    maxfd = listenfd > recvfd ? listenfd : recvfd;                       /* initialize */
    maxi = -1;                              /* index into client[] array */
    for (i = 0; i < FD_SETSIZE-1; i++)
        client[i] = -1;                     /* -1 indicates available entry */

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    FD_SET(recvfd, &allset);

    int client_counter = 0;
    byte_count = 0;

    for ( ; ; ) {
        rset = allset;          /* structure assignment */
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(recvfd, &rset)) {        /* new client connection */
            clilen = sizeof(cliaddr);
            connfd = accept(recvfd, (struct sockaddr *) &cliaddr, &clilen);

            for (i = 0; i < FD_SETSIZE-1; i++){
                if (client[i] < 0) {
                    client[i] = connfd;     /* save descriptor */
                    break;
                }
            }

            if (i == FD_SETSIZE-1)
                printf("too many clients\n");

            client_counter++;
            FD_SET(connfd, &allset);        /* add new descriptor to set */

            if (connfd > maxfd)
                maxfd = connfd;             /* for select */

            if (i > maxi)
                maxi = i;                   /* max index in client[] array */
            
            printf("* client connected from %s:%u\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);

            client_addr[i] = cliaddr;
            server_port[i] = htons(atoi(argv[1])+1);

            if (--nready <= 0)
                continue; 
                                  /* no more readable descriptors */
            
        
        }else if(FD_ISSET(listenfd, &rset)){
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

            for (i = 0; i < FD_SETSIZE-1; i++){
                if (client[i] < 0) {
                    client[i] = connfd;     /* save descriptor */
                    break;
                }
            }

            if (i == FD_SETSIZE-1)
                printf("too many clients\n");

            //client_counter++;
            FD_SET(connfd, &allset);        /* add new descriptor to set */

            if (connfd > maxfd)
                maxfd = connfd;             /* for select */

            if (i > maxi)
                maxi = i;                   /* max index in client[] array */
            
            printf("* client connected from %s:%u\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);

            client_addr[i] = cliaddr;
            server_port[i] = htons(atoi(argv[1]));

            if (--nready <= 0)
                continue;

        }


        for (i = 0; i <= maxi; i++) {   /* check all clients for data */
            if ( (sockfd = client[i]) < 0)
                continue;

            if (FD_ISSET(sockfd, &rset)) {
                memset(buf, 0, MAXLINE);
                if ( (n = read(sockfd, buf, MAXLINE)) == 0) {
                    /*4connection closed by client */

                    printf("* client is %s:%u disconnected\n", inet_ntoa(client_addr[i].sin_addr), client_addr[i].sin_port);
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;

                    if(server_port[i] == htons(atoi(argv[1])+1))
                        client_counter--;
                    
                }else if(server_port[i] == htons(atoi(argv[1])+1)){
                    buf[strlen(buf)-1] = '\0';
                    byte_count += strlen(buf);
                }else{
        
                    buf[strlen(buf)-1] = '\0';
                    gettimeofday(&_t, NULL);

                    if(strcmp(buf, "/ping") == 0){
                        sprintf(msg, "%s PONG\n", time_buf);
                        write(connfd, msg, strlen(msg));
                    }else if(strcmp(buf, "/reset") == 0){
                        sprintf(msg, "%lf RESET %lld\n", tv2s(&_t), byte_count);
                        byte_count = 0;
                        last_reset_time = _t;
                        write(connfd, msg, strlen(msg));
                    }else if(strcmp(buf, "/report") == 0){
                        t = tv2s(&_t);
                        elapsed_time = t - tv2s(&last_reset_time);
                        double bit_rate = 8.0 * byte_count / 1e6 / elapsed_time;
                        //printf(msg, "%s REPORT %lds %lfMbps\n", time_buf, elapsed_time, bit_rate);
                        sprintf(msg, "%lf REPORT %lfs %lfMbps\n", t, elapsed_time, bit_rate);
                        write(connfd, msg, strlen(msg));
                    }else if(strcmp(buf, "/clients") == 0){
                        sprintf(msg, "%s CLIENTS %d\n", time_buf, client_counter);
                        write(connfd, msg, strlen(msg));
                    }
                }

                if (--nready <= 0)
                    break;                          /* no more readable descriptors */
            }
        }
        
    }
}