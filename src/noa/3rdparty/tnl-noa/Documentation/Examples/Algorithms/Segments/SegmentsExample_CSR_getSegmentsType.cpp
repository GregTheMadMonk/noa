#include <iostream>
#include <functional>
#include <TNL/Algorithms/Segments/CSR.h>
#include <TNL/Devices/Host.h>
#include <TNL/Devices/Cuda.h>

template< typename Device >
void SegmentsExample()
{
   using SegmentsType = typename TNL::Algorithms::Segments::CSR< Device, int >;

   /***
    * Create segments and print the segments type.
    */
   SegmentsType segments;
   std::cout << "The segments type is: " << segments.getSegmentsType() << std::endl;
}

int main( int argc, char* argv[] )
{
   std::cout << "Example of CSR segments on host: " << std::endl;
   SegmentsExample< TNL::Devices::Host >();

#ifdef __CUDACC__
   std::cout << "Example of CSR segments on CUDA GPU: " << std::endl;
   SegmentsExample< TNL::Devices::Cuda >();
#endif
   return EXIT_SUCCESS;
}
