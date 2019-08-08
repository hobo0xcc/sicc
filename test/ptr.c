#include <stdlib.h>
#include <stdio.h>

int main() {
    int *a = malloc(sizeof(int) * 3);
    *a = 2;
    *(a+1) = 3;
    *(a+2) = 4;
    printf("%d, %d, %d\n", *(a), *(a + 1), *(a + 2));
    int b[3] = { 1, 2 ,3 };
    printf("%d\n", b[2]);
    printf("%d, %d, %d\n", *(b), *(b + 1), *(b + 2));
    return 0;
}
