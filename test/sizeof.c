#include <stdio.h>

int main() {
    printf("int * = %d, char * = %d, int = %d, char = %d\n", sizeof(int *), sizeof(char *), sizeof(int), sizeof(char));
    int i = 0;
    printf("sizeof i = %d\n", sizeof i);
    return 0;
}
