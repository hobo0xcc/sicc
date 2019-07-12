#include <stdio.h>

int main() {
    int *a = malloc(sizeof(int) * 3);
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    printf("%d, %d, %d\n", *a, *(a+1), *(a+2));
    return 0;
}
