#include <stdio.h>
#include <stdlib.h>

int main() {
    int *a = malloc(sizeof(int) * 3);
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    printf("%d, %d, %d\n", a[0], a[1], a[2]);
    int b[3];
    b[0] = 4;
    b[1] = 5;
    b[2] = 6;
    printf("%d, %d, %d\n", b[0], b[1], b[2]);
    int c[3] = { 7, 8, 9 };
    printf("%d, %d, %d\n", c[0], c[1], c[2]);
    int d[] = { 10, 11, 12 };
    printf("%d, %d, %d\n", d[0], d[1], d[2]);
    int e[5][5];
    e[0][2] = 10;
    e[1][4] = 42;
    printf("e[0][2] == %d\n", e[0][2]);
    printf("e[1][4] == %d\n", e[1][4]);
    printf("size of e[0] == %ld\n", sizeof e[0]);
    int f[2][2][2];
    f[0][1][0] = 10;
    printf("f[0][1][0] == %d\n", f[0][1][0]);
    return 0;
}
