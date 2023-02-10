/* include fig01 */
#include "unp.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>


struct channel{
    char *name;
	char topic[100];
} ;
struct user{
	char nickname[10];
	char *username;
	char *hostname;
	char *servername;
	char *realname;
	char channel[20];
	char *ip;
	int	 port;
};
char motd[] = "Welcome to the server\nThis is the message of the day\nAnd this is the third line\n";
int flag_checker(int i);
bool user_flag[FD_SETSIZE] = {false};
bool nick_flag[FD_SETSIZE] = {false};
int main(int argc, char **argv)
{
	int					i, maxi, maxfd, listenfd, connfd, sockfd, total_user = 0, total_channel = 0;
	int					nready, client[FD_SETSIZE];
	ssize_t				n;
	fd_set				rset, allset;
	char				buf[MAXLINE];
	memset(buf, 0, MAXLINE);
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
	signal(SIGPIPE, SIG_IGN);

	struct user Users[FD_SETSIZE];
	struct channel *Channels;
	char ServerName[20] = "Server";

	printf("Init done\n");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);

	printf("Listening on port %s\n", argv[1]);

	maxfd = listenfd;
	maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	for ( ; ; ) {
		rset = allset;
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(listenfd, &rset)) {
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (SA *) &cliaddr, &clilen);
			printf("* client connected from %s:%d\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd;
					Users[i].ip = inet_ntoa(cliaddr.sin_addr);
					Users[i].port = htons(cliaddr.sin_port);
					total_user++;
					break;
				}
			if (i == FD_SETSIZE)
				perror("too many clients");
			FD_SET(connfd, &allset);
			if (connfd > maxfd)
				maxfd = connfd;
			if (i > maxi)
				maxi = i;
			if (--nready <= 0)
				continue;
		}

		for (i = 0; i <= maxi; i++) {
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ( (n = read(sockfd, buf, MAXLINE)) == 0) {
					close(sockfd);
					FD_CLR(sockfd, &allset);
					printf("* client %s:%d disconnected\n", Users[i].ip, Users[i].port);
					client[i] = -1;
					total_user--;
					Users[i].ip = NULL;
					Users[i].port = 0;
					Users[i].nickname[0] = '\0';
					Users[i].username = NULL;
					Users[i].hostname = NULL;
					Users[i].servername = NULL;
					Users[i].realname = NULL;
					Users[i].channel[0] = '\0';
					user_flag[i] = false;
					nick_flag[i] = false;
				} else{
					printf("cmd: %s", buf);
					char msg[MAXLINE];
                    buf[n-1] = '\0';
                    char *cmd = strtok(buf, " ");
					if( cmd == NULL){
						continue;
                    }if( strcmp(cmd, "NICK") == 0 ){
                        char *nickname = strtok(NULL, " ");
						if(nickname == NULL){			// no nickname given 431
							sprintf(msg, ":%s 431 :No nickname given\n", ServerName);
							write(sockfd, msg, strlen(msg));
						}else{
							int a;
							for(a=0; a<total_user-1; a++){
								if(strcmp(nickname, Users[a].nickname) == 0){	// nickname in use 436
									sprintf(msg, ":%s 436 %s :Nickname collision KILL\n", ServerName, nickname);
									write(sockfd, msg, strlen(msg));
									break;
								}
							}
							if(a == total_user-1){
								strcpy(Users[i].nickname, nickname);
								printf("nickname[%d]: %s\n", i, Users[i].nickname);
								nick_flag[i] = true;
								sprintf(msg, ":%s 001 %s :Welcome to the minimized IRC daemon\n", ServerName, nickname);
								write(sockfd, msg, strlen(msg));
								sprintf(msg, ":%s 251 %s : There are %d users and %d invisible on %d servers\n", ServerName, nickname, total_user, 0, 1);
								write(sockfd, msg, strlen(msg));
								sprintf(msg, ":%s 375 %s : - %s Message of the day - \n", ServerName, nickname, ServerName);
								write(sockfd, msg, strlen(msg));

								char *motd_split = strtok(motd, "\n");
								while(motd_split != NULL){
									sprintf(msg, ":%s 372 %s : %s\n", ServerName, nickname, motd_split);
									write(sockfd, msg, strlen(msg));
									motd_split = strtok(NULL, "\n");
								}
								sprintf(msg, ":%s 376 %s :End of message of the day\n", ServerName, nickname);
								write(sockfd, msg, strlen(msg));
							}
						}
                    }else if( strcmp(cmd, "USER") == 0 ){
                        char *username = strtok(NULL, " ");
                        char *hostname = strtok(NULL, " ");
                        char *servername = strtok(NULL, " ");
                        char *realname = strtok(NULL, " ");
						if(username == NULL || hostname == NULL || servername == NULL || realname == NULL){	// not enough parameters 461
							sprintf(msg, ":%s 461 %s USER :Not enough parameters\n", ServerName, Users[i].nickname);
							write(sockfd, msg, strlen(msg));
						}else{
							if(nick_flag[i] == false)
								printf("Not registered yet\n"); // to be implemented something else
							else{
								Users[i].username = username;
								Users[i].hostname = hostname;
								Users[i].servername = servername;
								Users[i].realname = realname;
								user_flag[i] = true;
							}
						}
                    }else if( strcmp(cmd, "PING") == 0 ){
						if( flag_checker(i) == false ){
							printf("Not registered yet\n"); // to be implemented something else
							continue;
						}
                        char *server1 = strtok(NULL, " ");
                        char *server2 = strtok(NULL, " ");
                        if (server2 == NULL){
                            //PING server1
                            printf("PING %s\n", server1);
							sprintf(msg, ":%s PONG %s :%s\n", ServerName, ServerName, server1); //unsure command
							write(sockfd, msg, strlen(msg));
                        }else{
                            //PING server1 server2
                            printf("PING %s %s\n", server1, server2);							//unsure command

                        }
                    }else if( strcmp(cmd, "LIST") == 0 ){
                        char *channel = strtok(NULL, " ");
                        if (channel == NULL){
                            //LIST
							;
                        }else{
                            //LIST channel
							;
                        }
                    }else if( strcmp(cmd, "JOIN") == 0 ){
						if( flag_checker(i) == false ){
							printf("Not registered yet\n"); // to be implemented something else
							continue;
						}
                        char *channel = strtok(NULL, " ");
						int c;
						for(c=0; c<total_channel; c++){
							if(strcmp(channel, Channels[c].name) == 0){
								break;
							}
						}
						if(c == total_channel){
							total_channel++;
							Channels = (struct channel *)realloc(Channels, total_channel*sizeof(struct channel));
							struct channel tmp_channel;
							tmp_channel.name = channel;
							strcpy(tmp_channel.topic, "");
							Channels[c] = tmp_channel;
						}
						strcat(Users[i].channel, channel);
						sprintf(msg, ":%s JOIN %s\n", Users[i].nickname, channel);
						write(sockfd, msg, strlen(msg));
						if(Channels[c].topic != ""){
							sprintf(msg, ":%s 332 %s %s :%s\n", ServerName, Users[i].nickname, channel, Channels[c].topic);
							write(sockfd, msg, strlen(msg));
						}else{
							sprintf(msg, ":%s 331 %s %s :No topic is set\n", ServerName, Users[i].nickname, channel);
							write(sockfd, msg, strlen(msg));
						}

						char *names = (char *)malloc(100*sizeof(char));

						printf("total_user: %d\n", total_user);
						for(int j=0; j<total_user; j++){
							printf("Users[%d].channel: %s\n", j, Users[j].channel);
							printf("Users[%d].nickname: %s\n", j, Users[j].nickname);
							if(Users[j].channel == channel){
								strcat(names, Users[j].nickname);
								strcat(names, " ");
								printf("names: %s\n", names);
							}
						}
						sprintf(msg, ":%s 353 %s %s :%s\n", ServerName, Users[i].nickname, channel, names);
						write(sockfd, msg, strlen(msg));
						sprintf(msg, ":%s 366 %s %s :End of NAMES list\n", ServerName, Users[i].nickname, channel);
						write(sockfd, msg, strlen(msg));
                    }else if( strcmp(cmd, "TOPIC") == 0 ){
						if( flag_checker(i) == false ){
							printf("Not registered yet\n"); // to be implemented something else
							continue;
						}
                        char *channel = strtok(NULL, " ");
						printf("channel: %s\n", channel);

                        char *topic;
						char *tmp = strtok(NULL, " ");
						while( tmp != NULL ){
							strcat(topic, tmp);
							strcat(topic, " ");
							tmp = strtok(NULL, " ");
						}

						
						printf("topic: %s\n", topic);
						int c;
						for(c=0; c<total_channel; c++){
							if(strcmp(channel, Channels[c].name) == 0){
								break;
							}
						}
						printf("Checking same channel or not\n");
						if ( strcmp(Users[i].channel, channel) != 0 ){
								sprintf(msg, ":%s 442 %s %s :You're not on that channel\n", ServerName, Users[i].nickname, channel);
								write(sockfd, msg, strlen(msg));
								continue;
						}
						printf("in the same channel\n");
                        if (topic == NULL){
							printf("Reading topic\n");
							printf("Users[%d].nickname: %s\n", i, Users[i].nickname);
							printf("channel: %s\n", channel);
							printf("Channels[%d].topic: %s\n", c, Channels[c].topic);
							if( strcmp(Channels[c].topic, "") == 0 ){ 	// channel has no topic
								sprintf(msg, ":%s 331 %s %s :No topic is set\n", ServerName, Users[i].nickname, channel);
								write(sockfd, msg, strlen(msg));
								
							}else{										// channel has topic
								sprintf(msg, ":%s 332 %s %s :%s\n", ServerName, Users[i].nickname, channel, Channels[c].topic);
								write(sockfd, msg, strlen(msg));
							}
                        }else{
                            //set topic
							strcpy(Channels[c].topic, topic);
							sprintf(msg, ":%s TOPIC %s :%s\n", Users[i].nickname, channel, topic);
							write(sockfd, msg, strlen(msg));
                        }
                    }else if( strcmp(cmd, "NAMES") == 0 ){
						if( flag_checker(i) == false ){
							printf("Not registered yet\n"); // to be implemented something else
							continue;
						}
                        char *channel = strtok(NULL, " ");
                        if (channel == NULL){
                            //NAMES
                            printf("NAMES\n");
                        }else{
                            //NAMES channel
                            printf("NAMES %s\n", channel);
                        }
                    }else if( strcmp(cmd, "PRIVMSG") == 0 ){
						if( flag_checker(i) == false ){
							printf("Not registered yet\n"); // to be implemented something else
							continue;
						}
                        char *receiver = strtok(NULL, " ");
                        char *message = strtok(NULL, " ");
                        //PRIVMSG receiver message
                        printf("PRIVMSG %s %s\n", receiver, message);
                    }else if( strcmp(cmd, "QUIT") == 0 ){
                        //QUIT
                        printf("QUIT\n");
                    }else{
                        //Unknown command
						;
                    }
				    memset(buf, 0, MAXLINE);
				    if (--nready <= 0)
					    break;
                }
			}
		}
	}
}

// check if the user is registered
int flag_checker(int i){
	if( nick_flag[i] == 0 || user_flag[i] == 0 )
		return 0;
	return 1;
}
