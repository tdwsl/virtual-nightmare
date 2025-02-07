#include <stdio.h>

void test() {
    int n = printf("hello, %s x%d\n", "world", 10);
    int n = printf("hello, %s x%d\n", "world", 10);
    for(int i = 1; i < n; i++) printf("%c", '#');
    printf("\n");
}

int g;

struct test { int a, b; };

struct test tarr[20];

int test1(int a, int b);
int test1(int a, int b);
int test1(int a, int b) {}

int main() {
    printf("%d\n", 1017);
    printf("%X\n", 0xa10);
    test();
    char buf[20];
    sprintf(buf, "%s, w%drld\n", "hi", 0);
    FILE *fp = fopen("test.txt", "w");
    fprintf(fp, "Hello, hi, buf=%x\n", buf);
    fclose(fp);
    //tarr[1].a = 10;
}

