#include <iostream>
#include <TNL/Algorithms/parallelFor.h>
#include <TNL/Matrices/MultidiagonalMatrix.h>
#include <TNL/Devices/Host.h>
#include <TNL/Devices/Cuda.h>
#include <TNL/Pointers/SharedPointer.h>
#include <TNL/Pointers/SmartPointersRegister.h>

template< typename Device >
void setElements()
{
   const int matrixSize( 5 );
   auto diagonalsOffsets = { -1, 0, 1 }; // offsets of tridiagonal matrix
   using Matrix = TNL::Matrices::MultidiagonalMatrix< double, Device >;
   Matrix matrix( matrixSize, matrixSize, diagonalsOffsets );
   auto view = matrix.getView();

   for( int i = 0; i < 5; i++ )
      view.setElement( i, i, i );

   std::cout << "Matrix set from the host:" << std::endl;
   std::cout << matrix << std::endl;

   auto f = [=] __cuda_callable__ ( int i ) mutable {
      if( i > 0 )
         view.setElement( i, i - 1, 1.0 );
      view.setElement( i, i, -i );
      if( i < matrixSize - 1 )
         view.setElement( i, i + 1, 1.0 );
   };

   TNL::Algorithms::parallelFor< Device >( 0, matrixSize, f );

   std::cout << "Matrix set from its native device:" << std::endl;
   std::cout << matrix << std::endl;
}

int main( int argc, char* argv[] )
{
   std::cout << "Set elements on host:" << std::endl;
   setElements< TNL::Devices::Host >();

#ifdef __CUDACC__
   std::cout << "Set elements on CUDA device:" << std::endl;
   setElements< TNL::Devices::Cuda >();
#endif
}
