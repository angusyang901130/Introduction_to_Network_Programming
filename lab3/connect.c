#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(){

    struct sockaddr_in serv;
    bzero(&serv, sizeof(serv));
    int s_fd = socket(AF_INET, SOCK_STREAM, 0);

    serv.sin_addr.s_addr = inet_addr("140.113.213.213");
    serv.sin_port = htons(10002);
    serv.sin_family = AF_INET;

    int is_connect = connect(s_fd, (struct sockaddr*)&serv, sizeof(serv));

    char buff[1];
    char com[1000];
    bzero(com, sizeof(com));

    read(s_fd, com, sizeof(com));
    //printf("%s\n", buff);

    char msg[4] = "GO\n";
    //printf("%lu\n", sizeof(msg));
    write(s_fd, msg, 3);

    bzero(com, sizeof(com));
    read(s_fd, com, sizeof(com));
    bzero(com, sizeof(com));
    //printf("%s\n", buff);

    //int w_fd = open("test.txt", O_WRONLY | O_CREAT, S_IRWXU);

    int count = 0;
    while(read(s_fd, buff, sizeof(buff))){
        //ssize_t byte = write(w_fd, buff, 1);
        
        if(*buff == '\n')
            break;

        count++;
        //printf("%lu\n", byte);
        bzero(buff, sizeof(buff));
    }

    printf("%d\n", count);
    char* count_str = calloc(100, sizeof(char));
    sprintf(count_str, "%d", count);

    read(s_fd, com, sizeof(com));
    //printf("%s\n", com);
    bzero(com, sizeof(com));

    char* tmp = "\n";
    strcat(count_str, tmp);
    count_str = realloc(count_str, strlen(count_str));
    //printf("%lu\n", sizeof(count_str));
    //printf("%s\n", count_str);
    write(s_fd, count_str, sizeof(count_str));
    read(s_fd, com, sizeof(com));
    printf("%s\n", com);

    close(s_fd);

}