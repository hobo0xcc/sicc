#include <stdlib.h>
#include <stdio.h>

int main() {
    int *a = malloc(sizeof(int) * 3);
    *a = 2;
    *(a+1) = 3;
    *(a+2) = 4;
    printf("%d, %d, %d\n", *(a), *(a + 4), *(a + 8));
    return 0;
}
