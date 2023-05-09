#include <iostream>
#include <TNL/Algorithms/parallelFor.h>
#include <TNL/Matrices/SparseMatrix.h>
#include <TNL/Devices/Host.h>
#include <TNL/Devices/Cuda.h>
#include <TNL/Pointers/SharedPointer.h>
#include <TNL/Pointers/SmartPointersRegister.h>

template< typename Device >
void setElements()
{
   auto rowCapacities = { 1, 1, 1, 1, 1 };
   using MatrixType = TNL::Matrices::SparseMatrix< double, Device >;
   TNL::Pointers::SharedPointer< MatrixType > matrix( rowCapacities, 5 );

   /***
    * Calling the method setElements from host (CPU).
    */
   for( int i = 0; i < 5; i++ )
      matrix->setElement( i, i, i );

   std::cout << "Matrix set from the host:" << std::endl;
   std::cout << *matrix << std::endl;

   /***
    * This lambda function will run on the native device of the matrix which can be CPU or GPU.
    */
   MatrixType* matrix_device = &matrix.template modifyData< Device >();
   auto f = [=] __cuda_callable__ ( int i ) mutable {
      matrix_device->setElement( i, i, -i );
   };

   /***
    * For the case when Device is CUDA device we need to synchronize smart
    * pointers. To avoid this you may use SparseMatrixView. See
    * SparseMatrixView::getRow example for details.
    */
   TNL::Pointers::synchronizeSmartPointersOnDevice< Device >();
   TNL::Algorithms::parallelFor< Device >( 0, 5, f );

   std::cout << "Matrix set from its native device:" << std::endl;
   std::cout << *matrix << std::endl;
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
