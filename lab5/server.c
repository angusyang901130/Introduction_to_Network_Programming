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

struct Channel{
    char* name;
    char* topic;
};

char Users[FD_SETSIZE-1][10];
// username[10]

int main(int argc, char **argv){
    int                     i, maxi, maxfd, listenfd, connfd, sockfd;
    int                     nready, client[FD_SETSIZE-1];
    ssize_t                 n;
    fd_set                  rset, allset;
    char                    buf[MAXLINE], time_buf[80];
    socklen_t               clilen;
    struct sockaddr_in      cliaddr, servaddr;
    time_t                  rawtime;
    struct tm* timeinfo;
    char* client_names[FD_SETSIZE-1], msg[2000];
    struct sockaddr_in client_addr[FD_SETSIZE-1];

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

    maxfd = listenfd;                       /* initialize */
    maxi = -1;                              /* index into client[] array */
    for (i = 0; i < FD_SETSIZE-1; i++)
        client[i] = -1;                     /* -1 indicates available entry */

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    int client_counter = 0;

    for ( ; ; ) {
        rset = allset;          /* structure assignment */
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset)) {        /* new client connection */
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

            client_counter++;
            FD_SET(connfd, &allset);        /* add new descriptor to set */

            if (connfd > maxfd)
                maxfd = connfd;             /* for select */

            if (i > maxi)
                maxi = i;                   /* max index in client[] array */

            //time(&rawtime);
            //timeinfo = localtime(&rawtime);

            //char* client_name = "aa";
            //char client_name[5];
            //sprintf(client_name, "%d", i);
            //strftime(time_buf, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
            
            //printf("* client connected from %s:%u\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
            //sprintf(msg, "%s *** Welcome to the simple CHAT server\n", time_buf);
            //write(connfd, msg, strlen(msg));
            //sprintf(msg, "%s *** Total %d users online now. Your name is <%s>\n", time_buf, client_counter, client_name);
            //write(connfd, msg, strlen(msg));

            //client_names[i] = (char*)calloc(strlen(client_name)+1, sizeof(char));
            //strcpy(client_names[i], client_name);

            client_addr[i] = cliaddr;

            if (--nready <= 0)
                continue;                   /* no more readable descriptors */

        }

        //dc_client = 0;

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

                    client_counter--;

                    for(int j = 0; j <= maxi; j++){
                        if(client[j] < 0)
                            continue;

                        //dup2(client[j], 1);
                        sprintf(msg, "*** User <%s> has left the server\n", client_names[i]);
                        //printf("aaa\n");
                        n = write(client[j], msg, strlen(msg));

                        /* if(n == -1 && !dc_queue[j]){
                            dc_queue[j] = true;
                            dc_client++;
                        } */
                    }
                    
                } else{
                    //printf("%s", buf);
                    buf[strlen(buf)-1] = '\0';
                    //printf("%s", buf);
                    char* cmd = strtok(buf, " ");

                    if(strcmp(cmd, "NICK") == 0){
                        char* name = strtok(NULL, " ");
                    }else if(strcmp(cmd, "USER") == 0){
                        char* username = strtok(NULL, " ");
                        char* hostname = strtok(NULL, " ");
                        char* servername = strtok(NULL, " ");
                        char* realname = strtok(NULL, " ");
                        
                    }else if(strcmp(cmd, "PING") == 0){
                        char* server1 = strtok(NULL, " ");
                        char* server2 = strtok(NULL, " ");
                        if(server2 == NULL)
                            /* Not sure what to do yet */;
                    }else if(strcmp(cmd, "LIST") == 0){
                        
                    }else if(strcmp(cmd, "JOIN") == 0){

                    }else if(strcmp(cmd, "TOPIC") == 0){
                        
                    }else if(strcmp(cmd, "NAMES") == 0){
                        
                    }else if(strcmp(cmd, "PART") == 0){
                        
                    }else if(strcmp(cmd, "USERS") == 0){
                        
                    }else if(strcmp(cmd, "PRIVMSG") == 0){
                        
                    }else if(strcmp(cmd, "QUIT") == 0){
                        
                    }
                }

                if (--nready <= 0)
                    break;                          /* no more readable descriptors */
            }
        }
        
    }
}
