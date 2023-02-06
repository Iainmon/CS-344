#include <stdio.h>

int main()
{
    char *cptr = "Hello-world!";
    int  *iptr = (int *) cptr;
    int   cnt  = 0;
    
    printf("first: ");
    printf("%s\n", cptr);
    printf("second: ");
    printf("%s\n", iptr);
    
    printf("third: ");
    printf("%s\n", cptr+1);
    printf("fourth: ");
    printf("%s\n", iptr+1);
    
    for (cnt = 0; cnt < 13; ++cnt)
    {
        printf("[loop] %s\n", cptr+cnt);
        printf("[loop] %s\n", iptr+cnt);
    }
    
    return 0;
}