#include "Layr.h"

__global__ 
void on_device(){ basic_complex<float>::test() ; } 

void on_host(){   basic_complex<float>::test() ; } 

int main()
{
#ifdef WITH_THRUST
    printf("on_device launch WITH_THRUST\n"); 
    on_device<<<1,1>>>();
    cudaDeviceSynchronize();
#endif

#ifdef WITH_THRUST
    printf("on_host WITH_THRUST\n"); 
#else
    printf("on_host NOT WITH_THRUST\n"); 
#endif
    on_host(); 

    return 0 ; 
}

