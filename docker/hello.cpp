#include <stdio.h>

int main(){
    printf("Hello ");
#ifdef __ARM_ARCH
    printf("ARM %d\n", __ARM_ARCH);
#endif
#ifdef __x86_64
    printf("x86_64\n");
#endif
}
