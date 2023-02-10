#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
    int *a;
    a = calloc(3, sizeof(int));
    a[0] = 0;
    a[1] = 1;
    a[2] = 4;

    a = realloc(a, sizeof(int) * 5);
    a[3] = 0;
    for(int i = 0; i < 4; i++)
        printf("%d\n", a[i]);
}