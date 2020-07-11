#include<stdio.h>
int getint()
{
    int a;
    scanf("%d",&a);
    return a;
}
int getchar()
{
    char a;
    scanf("%c",&a);
    int b = a;
    return b;
}

int putint(int x)
{
    printf("%d",x);
    return 0;
}


// int putchar(int o)
// {
//     // char o = i;
//     printf("%c", o);
//     // printf("hello\n");
//     //printf("%c", i);
//     return 0;
// }

// int main()
// {
//     int r = getchar();
//     return 0;
// }