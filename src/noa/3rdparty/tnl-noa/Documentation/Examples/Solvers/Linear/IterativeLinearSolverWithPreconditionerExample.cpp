#include <iostream>
#include <memory>
#include <TNL/Matrices/SparseMatrix.h>
#include <TNL/Devices/Host.h>
#include <TNL/Devices/Cuda.h>
#include <TNL/Solvers/Linear/TFQMR.h>
#include <TNL/Solvers/Linear/Preconditioners/Diagonal.h>

template< typename Device >
void iterativeLinearSolverExample()
{
   /***
    * Set the following matrix (dots represent zero matrix elements):
    *
    *   /  2.5 -1    .    .    .   \
    *   | -1    2.5 -1    .    .   |
    *   |  .   -1    2.5 -1.   .   |
    *   |  .    .   -1    2.5 -1   |
    *   \  .    .    .   -1    2.5 /
    */
   using MatrixType = TNL::Matrices::SparseMatrix< double, Device >;
   using Vector = TNL::Containers::Vector< double, Device >;
   const int size( 5 );
   auto matrix_ptr = std::make_shared< MatrixType >();
   matrix_ptr->setDimensions( size, size );
   matrix_ptr->setRowCapacities( Vector( { 2, 3, 3, 3, 2 } ) );

   auto f = [=] __cuda_callable__ ( typename MatrixType::RowView& row ) mutable {
      const int rowIdx = row.getRowIndex();
      if( rowIdx == 0 )
      {
         row.setElement( 0, rowIdx,    2.5 );    // diagonal element
         row.setElement( 1, rowIdx+1, -1 );      // element above the diagonal
      }
      else if( rowIdx == size - 1 )
      {
         row.setElement( 0, rowIdx-1, -1.0 );    // element below the diagonal
         row.setElement( 1, rowIdx,    2.5 );    // diagonal element
      }
      else
      {
         row.setElement( 0, rowIdx-1, -1.0 );    // element below the diagonal
         row.setElement( 1, rowIdx,    2.5 );    // diagonal element
         row.setElement( 2, rowIdx+1, -1.0 );    // element above the diagonal
      }
   };

   /***
    * Set the matrix elements.
    */
   matrix_ptr->forAllRows( f );
   std::cout << *matrix_ptr << std::endl;

   /***
    * Set the right-hand side vector.
    */
   Vector x( size, 1.0 );
   Vector b( size );
   matrix_ptr->vectorProduct( x, b );
   x = 0.0;
   std::cout << "Vector b = " << b << std::endl;

   /***
    * Solve the linear system using diagonal (Jacobi) preconditioner.
    */
   using LinearSolver = TNL::Solvers::Linear::TFQMR< MatrixType >;
   using Preconditioner = TNL::Solvers::Linear::Preconditioners::Diagonal< MatrixType >;
   auto preconditioner_ptr = std::make_shared< Preconditioner >();
   preconditioner_ptr->update( matrix_ptr );
   LinearSolver solver;
   solver.setMatrix( matrix_ptr );
   solver.setPreconditioner( preconditioner_ptr );
   solver.setConvergenceResidue( 1.0e-6 );
   solver.solve( b, x );
   std::cout << "Vector x = " << x << std::endl;
}

int main( int argc, char* argv[] )
{
   std::cout << "Solving linear system on host: " << std::endl;
   iterativeLinearSolverExample< TNL::Devices::Sequential >();

#ifdef __CUDACC__
   std::cout << "Solving linear system on CUDA device: " << std::endl;
   iterativeLinearSolverExample< TNL::Devices::Cuda >();
#endif
}
