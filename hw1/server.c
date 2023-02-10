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

struct User{
    char nickname[10];
    char channel[20];
    char* username;
    char* hostname;
    char* servername;
    char* realname;
};

struct Channel{
    char name[20];
    char topic[100];
};

char MOTD[] = "Hello\nWelcome to mircd\nStart chatting with friends\n";
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

    struct User users[FD_SETSIZE-1]; 
    bool uflag[FD_SETSIZE-1] = {false};
    bool nflag[FD_SETSIZE-1] = {false};

    int channel_cnt = 0;
    struct Channel channels[1000];
    char server[] = "mircd";

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
                    uflag[i] = false;
                    nflag[i] = false;
                    struct User new_user;
                    users[i] = new_user;

                } else{

                    char msg[MAXLINE];
                    buf[n-1] = '\0';
                    if(buf[n-2] == '\r')
                        buf[n-2] = '\0';
                    //printf("%s", buf);
                    char* cmd = strtok(buf, " ");

                    if(strcmp(cmd, "NICK") == 0){
                        char* name = strtok(NULL, " ");

                        if(name == NULL){
                            sprintf(msg, ":%s 431 :No nickname given\n", server);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        nflag[i] = true;
                        for(int c = 0; c <= maxi; c++){
                            if(client[c] < 0)
                                continue;
                            if(strcmp(users[c].nickname, name) == 0){
                                sprintf(msg, ":%s 436 %s :Nickname collision KILL\n", server, name);
                                write(sockfd, msg, strlen(msg));
                                nflag[i] = false;
                                break;
                            }
                        }
                        
                        if(!nflag[i])
                            continue;

                        //printf("aaa\n");
                        strcpy(users[i].nickname, name);
                        
                        


                    }else if(strcmp(cmd, "USER") == 0){
                        char* username = strtok(NULL, " ");
                        char* hostname = strtok(NULL, " ");
                        char* servername = strtok(NULL, " ");
                        char* realname = strtok(NULL, " ");
                        
                        if(username == NULL || hostname == NULL || servername == NULL || realname == NULL){
                            sprintf(msg, ":%s 461 %s USER :Not enough parameters\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        users[i].username = username;
                        users[i].hostname = hostname;
                        users[i].servername = servername;
                        users[i].realname = realname;

                        if(!nflag[i]){
                            //printf("Not register yet\n");
                            sprintf(msg, ":%s 451 %s :You have not registered\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                        }else{
                            printf("nickname: %s\n", users[i].nickname);
                            uflag[i] = true;

                            /* MOTD and Welcome */
                            sprintf(msg, ":%s 001 %s :Welcome to the minimized IRC daemon!\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            sprintf(msg, ":%s 251 %s :There are %d users and %d invisible on %d server\n", server, users[i].nickname, client_counter, 0, 1);
                            write(sockfd, msg, strlen(msg));
                            sprintf(msg, ":%s 375 %s :- %s Message of the day\n", server, users[i].nickname, server);
                            write(sockfd, msg, strlen(msg));

                            char motd[100];
                            strcpy(motd, MOTD);
                            //printf("MOTD: %s\n", motd);
                            char* token = strtok(motd, "\n");
                            while(token != NULL){
                                sprintf(msg, ":%s 372 %s :- %s\n", server, users[i].nickname, token);
                                write(sockfd, msg, strlen(msg));
                                token = strtok(NULL, "\n");
                                //printf("token: %s\n", token);
                            }
                            sprintf(msg, ":%s 376 %s :End of message of the day\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                        
                            /*********************/
                        }


                    }else if(strcmp(cmd, "PING") == 0){
                        /* if(!nflag[i] || uflag[i]){
                            sprintf(msg, ":You have not registered\n");
                            write(sockfd, msg, strlen(msg));
                            continue;
                        } */
                        char* server1 = strtok(NULL, " ");
                        
                        if(server1 == NULL){
                            sprintf(msg, ":%s 409 %s :No origin specified\n", server, users[i].nickname);
                            write(connfd, msg, strlen(msg));
                            continue;
                        }


                        char* server2 = strtok(NULL, " ");
                        if(server2 != NULL){
                            sprintf(msg, "PONG %s\n", server1);
                            write(sockfd, msg, strlen(msg));
                        }else{
                            sprintf(msg, "PONG %s\n", server1);
                            write(sockfd, msg, strlen(msg));
                        }
                        
                    }else if(strcmp(cmd, "LIST") == 0){
                        if(!nflag[i] || !uflag[i]){
                            sprintf(msg, ":%s 451 %s :You have not registered\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        char* channel = strtok(NULL, " ");
                        sprintf(msg, ":%s 321 %s Channel :Users Name\n", server, users[i].nickname);
                        write(sockfd, msg, strlen(msg));
                       
                        if(channel == NULL){
                            int n_user[channel_cnt];
                            for(int u = 0; u < channel_cnt; u++)
                                n_user[u] = 0;

                            /* count how much client is in each channel */
                            
                            for(int c = 0; c <= maxi; c++){
                                if(client[c] < 0)
                                    continue;
                                    
                                char chan[100];
                                strcpy(chan, users[c].channel);
                                printf("channel: %s\n", chan);

                                for(int ch = 0; ch < channel_cnt; ch++){
                                    if(strcmp(channels[ch].name, chan) == 0){
                                        n_user[ch]++;
                                        break;
                                    }
                                }
                            }
                            /********************************************/

                            for(int c = 0; c < channel_cnt; c++){
                                sprintf(msg, ":%s 322 %s %s %d :%s\n", server, users[i].nickname, channels[c].name, n_user[c], channels[c].topic);
                                write(sockfd, msg, strlen(msg));
                            }

                        }else{
                            /* count how much client is a channel */
                            int cnt = 0;
                            for(int c = 0; c <= maxi; c++){
                                if(client[c] < 0)
                                    continue;
                                if(strcmp(users[c].channel, channel) == 0)
                                    cnt++;
                            }
                            /**************************************/

                            int ch = 0;
                            for(int ch = 0; ch < channel_cnt; ch++){
                                if(strcmp(channels[ch].name, channel) == 0)
                                    break;             
                            }

                            sprintf(msg, ":%s 322 %s %s %d :%s\n", server, users[i].nickname, channel, cnt, channels[ch].topic);
                            write(sockfd, msg, strlen(msg));
                        }

                        sprintf(msg, ":%s 323 %s :End of List\n", server, users[i].nickname);
                        write(sockfd, msg, strlen(msg));
                        
                    }else if(strcmp(cmd, "JOIN") == 0){
                        if(!nflag[i] || !uflag[i]){
                            sprintf(msg, ":%s 451 %s :You have not registered\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        char* channel = strtok(NULL, " ");
                        int c;

                        /* No need error, if channel not exist, create one */
                        for(c = 0; c < channel_cnt; c++){
                            if(strcmp(channels[c].name, channel) == 0)
                                break;                                                          
                        }

                        if(c == channel_cnt){
                            channel_cnt++;

                            strcpy(channels[c].name, channel);                                                   
                            strcpy(channels[c].topic, "");                           
                        }
                        
                        strcpy(users[i].channel, channels[c].name);
                        sprintf(msg, ":%s JOIN %s\n", users[i].nickname, channel);  

                        for(int u = 0; u <= maxi; u++){
                            if(client[u] < 0 || strcmp(users[u].channel, channel) != 0)
                                continue;
                            write(client[u], msg, strlen(msg));                    
                        }

                        if(strlen(channels[c].topic) == 0){
                            sprintf(msg, ":%s 331 %s %s :No topic is set\n", server, users[i].nickname, channels[c].name);
                        }else{
                            sprintf(msg, ":%s 332 %s %s :%s\n", server, users[i].nickname, channels[c].name, channels[c].topic);
                            printf("msg: %s", msg);
                        }
                        
                        write(sockfd, msg, strlen(msg));
                        /* Finish create channel */

                        /* Print the user in the channel include your self */
                        char nameList[200];
                        memset(nameList, 0, 200);
                        for(int k = 0; k <= maxi; k++){
                            if(client[k] < 0)
                                continue;
                            
                            //printf("fd: %d\n", k);
                            if(strcmp(users[k].channel, channel) == 0){
                                strcat(nameList, users[k].nickname);
                                strcat(nameList, " ");
                            }
                            printf("nameList: %s\n", nameList);
                        }

                        //printf("nameList: %s\n", nameList);
                        sprintf(msg, ":%s 353 %s %s :%s\n", server, users[i].nickname, channel, nameList);
                        write(sockfd, msg, strlen(msg));
                        sprintf(msg, ":%s 366 %s %s :End of Names List\n", server, users[i].nickname, users[i].channel);
                        write(sockfd, msg, strlen(msg));
                        /***************************************************/

                    }else if(strcmp(cmd, "TOPIC") == 0){
                        if(!nflag[i] || !uflag[i]){
                            sprintf(msg, ":%s 451 %s :You have not registered\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        char* channel = strtok(NULL, " ");

                        int c = 0;
                        char topic[100];
                        char* token = strtok(NULL, " ");

                        if(strcmp(channel, users[i].channel) != 0){
                            sprintf(msg, ":%s 442 %s %s :You are not on that channel\n", server, users[i].nickname, channel);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        for(c = 0; c < channel_cnt; c++){
                            if(strcmp(channels[c].name, channel) == 0){
                                break;
                            }
                        }

                        if(token == NULL){
                            //printf("channel topic: %s\n", channels[c].topic);
                            if(strcmp(channels[c].topic, "\0") == 0)
                                sprintf(msg, ":%s 331 %s %s :No topic is set\n", server, users[i].nickname, channel);
                            else sprintf(msg, ":%s 332 %s %s :%s\n", server, users[i].nickname, channel, channels[c].topic);
                        }else{
                            if(token[0] == ':'){
                                token = token+1;
                                //printf("topic: %s\n", topic);
                            }
                            
                            //printf("token: %s\n", token);
                            strcpy(topic, token);

                            char* token = strtok(NULL, " ");
                            while(token != NULL){
                                strcat(topic, " ");
                                strcat(topic, token);
                                token = strtok(NULL, " ");
                            }

                            strcpy(channels[c].topic, topic);
                            //printf("new topic: %s\n", channels[c].topic);
                            sprintf(msg, ":%s 332 %s %s %s\n", server, users[i].nickname, channel, channels[c].topic);
                        }
                        write(sockfd, msg, strlen(msg));
                            
                    }else if(strcmp(cmd, "NAMES") == 0){
                        if(!nflag[i] || !uflag[i]){
                            sprintf(msg, ":%s 451 %s :You have not registered\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        char* channel = strtok(NULL, " ");
                        if(channel != NULL){
                            char nameList[200];
                            for(int k = 0; k <= maxi; k++){
                                if(client[k] < 0)
                                    continue;
                            
                                if(strcmp(users[k].channel, channel) == 0){
                                    strcat(nameList, users[k].nickname);
                                    strcat(nameList, " ");
                                }
                            }

                            sprintf(msg, ":%s 353 %s %s :%s\n", server, users[i].nickname, channel, nameList);
                            write(sockfd, msg, strlen(msg));

                            sprintf(msg, ":%s 366 %s %s :End of Names List\n", server, users[i].nickname, users[i].channel);
                            write(sockfd, msg, strlen(msg));
                        }else{
                            for(int c = 0; c < channel_cnt; c++){
                                char nameList[200];
                                for(int k = 0; k <= maxi; k++){
                                    if(client[k] < 0)
                                        continue;
                                
                                    if(strcmp(users[k].channel, channels[c].name) == 0){
                                        strcat(nameList, users[k].nickname);
                                        strcat(nameList, " ");
                                    }
                                }

                                sprintf(msg, ":%s 353 %s %s :%s\n", server, users[i].nickname, channels[c].name, nameList);
                                write(sockfd, msg, strlen(msg));

                                sprintf(msg, ":%s 366 %s %s :End of Names List\n", server, users[i].nickname, channels[c].name);
                                write(sockfd, msg, strlen(msg));
                            }
                        }

                    }else if(strcmp(cmd, "PART") == 0){
                        if(!nflag[i] || !uflag[i]){
                            sprintf(msg, ":%s 451 %s :You have not registered\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        char* channel = strtok(NULL, " ");

                        int c = 0;
                        for(c = 0; c < channel_cnt; c++){
                            if(strcmp(channels[c].name, channel) == 0){
                                break;
                            }
                        }

                        if(c != channel_cnt && strcmp(channel, users[i].channel) == 0){
                            sprintf(msg, ":%s PART :%s\n", users[i].nickname, channel);
                            for(int u = 0; u <= maxi; u++){
                                if(client[u] < 0 || strcmp(users[u].channel, channel) != 0)
                                    continue;
                                write(client[u], msg, strlen(msg));
                            }
                            strcpy(users[i].channel, "\0");
                        }else if(c == channel_cnt){
                            sprintf(msg, ":%s 403 %s %s :No such channel\n", server, users[i].nickname, channel);
                            write(sockfd, msg, strlen(msg));
                        }else if(strcmp(channel, users[i].channel) != 0){
                            sprintf(msg, ":%s 442 %s %s :You are not on that channel\n", server, users[i].nickname, channel);
                            write(sockfd, msg, strlen(msg));
                        }
                        
                    }else if(strcmp(cmd, "USERS") == 0){
                        if(!nflag[i] || !uflag[i]){
                            sprintf(msg, ":%s 451 %s :You have not registered\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        sprintf(msg, ":%s 392 %s :User ID   Terminal  Host\n", server, users[i].nickname);
                        write(sockfd, msg, strlen(msg));

                        for(int c = 0; c <= maxi; c++){
                            if(client[c] < 0)
                                continue;
                            
                            sprintf(msg, ":%s 393 %s :%-8s %-9s %-8s\n", server, users[i].nickname, users[c].nickname, "-", inet_ntoa(client_addr[c].sin_addr));
                            write(sockfd, msg, strlen(msg));
                        }
                        sprintf(msg, ":%s 394 %s :End of users\n", server, users[i].nickname);

                    }else if(strcmp(cmd, "PRIVMSG") == 0){
                        if(!nflag[i] || !uflag[i]){
                            sprintf(msg, ":%s 451 %s :You have not registered\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        char* channel = strtok(NULL, " ");
                        //printf("channel: %s\n", channel);
                        if(channel == NULL){
                            sprintf(msg, ":%s 411 %s :No recipient given (PRIVMSG)\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        int c = 0;
                        for(c = 0; c < channel_cnt; c++){
                            if(strcmp(channels[c].name, channel) == 0){
                                break;
                            }
                        }

                        if(c == channel_cnt){
                            sprintf(msg, ":%s 401 %s %s :No such nick/channel\n", server, users[i].nickname, channel);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        char message[100];
                        char* token = strtok(NULL, " ");

                        if(token == NULL){
                            sprintf(msg, ":%s 412 %s :No text to send\n", server, users[i].nickname);
                            write(sockfd, msg, strlen(msg));
                            continue;
                        }

                        token = token + 1;
                        strcpy(message, token);
                        token = strtok(NULL, " ");
                        //printf("token: %s\n", token);
                        while(token != NULL){
                            strcat(message, " ");
                            printf("message: %s\n", message);
                            printf("token: %s\n", token);
                            strcat(message, token);
                            printf("message: %s\n", message);
                            token = strtok(NULL, " ");
                        }

                        for(int u = 0; u <= maxi; u++){
                            if(client[u] < 0 || u == i)
                                continue;
                            
                            if(strcmp(channel, users[u].channel) == 0){
                                sprintf(msg, ":%s PRIVMSG %s :%s\n", users[i].nickname, channel, message);
                                write(client[u], msg, strlen(msg));
                            }
                        }

                    }else if(strcmp(cmd, "QUIT") == 0){
                        uflag[i] = false;
                        nflag[i] = false;
                        struct User new_user;
                        users[i] = new_user;
                    }else if(cmd[0] == ':'){
                        /* Do nothing*/
                    }else{
                        sprintf(msg, ":%s 421 %s %s :Unknown command\n", server, users[i].nickname, cmd);
                        write(sockfd, msg, strlen(msg));
                    }
                }

                if (--nready <= 0)
                    break;                          /* no more readable descriptors */
            }
        }
        
    }
}
