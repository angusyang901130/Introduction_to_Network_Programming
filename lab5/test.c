#include <stdio.h>
#include <string.h>

int main(){
    char str[12] = "111 222 333";
    char* token, *tok;
    token = strtok(str, " ");
    printf("%s\n", token);
    tok = strtok(NULL, " ");
    printf("%s\n", tok);
    char* a = strtok(NULL, " ");
    char* b = strtok(NULL, " ");
    if(b == NULL)
        printf("%s\n", a);
}