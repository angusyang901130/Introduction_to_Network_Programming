#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#define UNIXSTR_PATH "./sudoku.sock"
#define MAXLINE 1024

int table[9][9];
int space[30][2];
int solution[30];

int solve(int pos){
    if(pos == 30)
        return 1;

    int x = space[pos][0];
    int y = space[pos][1];

    int num[10];
    for(int i = 0; i < 10; i++)
        num[i] = 0;

    for(int i = 0; i < 9; i++){
        num[table[x][i]] = 1;
        num[table[i][y]] = 1;
    }

    int blk_x = x / 3;
    int blk_y = y / 3;

    for(int i = blk_x*3; i < blk_x*3+3; i++){
        for(int j = blk_y*3; j < blk_y*3+3; j++){
            num[table[i][j]] = 1;
        }
    }

    for(int i = 1; i < 10; i++){
        if(num[i])
            continue;
        else{
            //printf("current solve: (x, y)=(%d, %d), value=%d\n", x, y, i);
            table[x][y] = i;
            solution[pos] = i;
            int sol = solve(pos+1);
            if(sol)
                return 1;
        }
    }
    table[x][y] = 0;
    return 0;
}

int main(int argc, char **argv){
    int                     sockfd;
    struct sockaddr_un      servaddr;
    char msg[MAXLINE], buf[MAXLINE];

    sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, UNIXSTR_PATH);

    connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    //str_cli(stdin, sockfd);         /* do it all */

    //read(sockfd, buf, MAXLINE);
    //printf("Received from server: %s\n", buf);
    strcpy(buf, "S\n");
    //printf("My command: %s", buf);
    write(sockfd, buf, strlen(buf));
    read(sockfd, msg, MAXLINE);
    //printf("Serial number: %s", msg);

    
    int pos = 4, space_pos = 0;
    
    for(int i = 0; i < 9; i++){
        for(int j = 0; j < 9; j++){
            if(msg[pos] == '.'){
                table[i][j] = 0;
                space[space_pos][0] = i;
                space[space_pos][1] = j;
                //printf("current pos: %d -> (%d, %d)", space_pos, space[space_pos][0], space[space_pos][1]);
                space_pos++;       
            }else{
                table[i][j] = (int)msg[pos] - '0';
                //printf("current pos: (%d, %d) = %d", i, j, table[i][j]);
            }
            pos++;
        }
    }

    int sol = solve(0);
    if(sol == 0)
        printf("No solution!!\n");

    for(int i = 0; i < 30; i++){
        sprintf(buf, "V %d %d %d\n", space[i][0], space[i][1], solution[i]);
        write(sockfd, buf, strlen(buf));
        read(sockfd, msg, MAXLINE);
        //usleep(100000);
    }

    sprintf(buf, "C");
    write(sockfd, buf, strlen(buf));
}