#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main()
{
    int bufsize = 10;
    char *buf = (char *) malloc(bufsize * sizeof(char));
    char *str = "Hello world!";

    strcpy(buf, str);
    
    printf("buf contains: %s [size: %d]\n", buf, (int) strlen(buf));
    printf("str contains: %s [size: %d]\n", str, (int) strlen(str));
    return 0;
}