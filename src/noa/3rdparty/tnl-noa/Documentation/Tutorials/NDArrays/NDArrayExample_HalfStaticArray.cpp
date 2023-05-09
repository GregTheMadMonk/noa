#include <TNL/Containers/NDArray.h>

using namespace TNL::Containers;

int main()
{
   // compile-time constant number of rows
   constexpr int num_rows = 3;

   // dynamic (run-time specified) number of columns
   int num_cols = 10;

   using HalfStaticArray = NDArray< int,  // Value
                                    SizesHolder< int, num_rows, 0 >,   // SizesHolder
                                    std::index_sequence< 0, 1 >,       // Permutation
                                    TNL::Devices::Host >;              // Device
   HalfStaticArray a;

   // set array sizes: statically-sized axes must have 0
   a.setSizes( 0, num_cols );

   int value = 0;
   for( int i = 0; i < num_rows; i++ )
      for( int j = 0; j < num_cols; j++ )
         a( i, j ) = value++;

   std::cout << "a = " << a.getStorageArray() << std::endl;
}
