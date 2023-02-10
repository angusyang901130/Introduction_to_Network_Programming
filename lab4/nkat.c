#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

void handler(int s){
    int stat;
    pid_t pid;
    //printf("reach handler\n");
    while( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
        printf("Child %d has terminated\n", pid);
    }
        
    return;
}

int main(int argc, char **argv){
	int                             listenfd, connfd;
	pid_t                           childpid;
	socklen_t                       clilen;
	struct sockaddr_in              cliaddr, servaddr;
	void                            sig_chld(int);

    signal(SIGCHLD, handler);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));
	
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, 100);
    
    //printf("%d\n", listenfd);

	for ( ; ; ) {
        clilen = sizeof(cliaddr);

        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
        //printf("%d\n", connfd);
        printf("New connection from %s:%d\n",inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
        
        childpid = fork();
        if ( childpid == 0) {        /* child process */
            //printf("%s", "aaa");
            //printf("%d\n", new_argc);
            close(listenfd);        /* close listening socket */
            // char* str = (char*)calloc(10, sizeof(char));

            // strcpy(str, argv[2]);
            //if(strcmp(str, ""))
            int new_out = dup(2);
            dup2(connfd, 1);
            dup2(connfd, 2);
            dup2(connfd, 0);
            int error = execvp(argv[2], argv+2);
            //printf("aaa\n");
            if(error == -1){
                dup2(new_out, 2);
                perror(argv[2]);
                exit(-1);
                //printf("The file is not executable!\n");
            }
            exit(0);
        }
        
        close(connfd);                  /* parent closes connected socket */
	}
}