// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/TypeTraits.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/SegmentsReductionKernels/DefaultKernel.h>
#include "MatrixBase.h"
#include "SparseMatrixRowView.h"

namespace noa::TNL::Matrices {

/**
 * \brief Implementation of sparse matrix view.
 *
 * It serves as an accessor to \ref SparseMatrix for example when passing the
 * matrix to lambda functions. SparseMatrix view can be also created in CUDA
 * kernels.
 *
 * \tparam Real is a type of matrix elements. If \e Real equals \e bool the
 *         matrix is treated as binary and so the matrix elements values are
 *         not stored in the memory since we need to remember only coordinates
 *         of non-zero elements (which equal one).
 * \tparam Device is a device where the matrix is allocated.
 * \tparam Index is a type for indexing of the matrix elements.
 * \tparam MatrixType specifies a symmetry of matrix. See \ref MatrixType.
 *         Symmetric matrices store only lower part of the matrix and its
 *         diagonal. The upper part is reconstructed on the fly.  GeneralMatrix
 *         with no symmetry is used by default.
 * \tparam Segments is a structure representing the sparse matrix format.
 *         Depending on the pattern of the non-zero elements different matrix
 *         formats can perform differently especially on GPUs. By default
 *         \ref Algorithms::Segments::CSR format is used. See also
 *         \ref Algorithms::Segments::Ellpack,
 *         \ref Algorithms::Segments::SlicedEllpack,
 *         \ref Algorithms::Segments::ChunkedEllpack, and
 *         \ref Algorithms::Segments::BiEllpack.
 * \tparam ComputeReal is the same as \e Real mostly but for binary matrices it
 *         is set to \e Index type. This can be changed by the user, of course.
 */
template< typename Real, typename Device, typename Index, typename MatrixType, typename SegmentsView, typename ComputeReal >
class SparseMatrixBase : public MatrixBase< Real, Device, Index, MatrixType, SegmentsView::getOrganization() >
{
   static_assert(
      ! MatrixType::isSymmetric() || ! std::is_same_v< Device, Devices::Cuda >
         || (std::is_same_v< std::decay_t< Real >, float > || std::is_same_v< std::decay_t< Real >, double >
             || std::is_same_v< std::decay_t< Real >, int > || std::is_same_v< std::decay_t< Real >, long long int >
             || std::is_same_v< std::decay_t< Real >, bool >),
      "Given Real type is not supported by atomic operations on GPU which are necessary for symmetric operations." );

   using Base = MatrixBase< Real, Device, Index, MatrixType, SegmentsView::getOrganization() >;

public:
   // TODO: add documentation for this type
   using ColumnIndexesViewType =
      Containers::VectorView< typename noa::TNL::copy_const< Index >::template from< Real >::type, Device, Index >;

   /**
    * \brief Type of the kernel used for parallel reductions on segments.
    *
    * We are assuming that the default segments reduction kernel provides
    * a *static* reduceAllSegments method, and thus it does not have to be
    * instantiated and initialized. If the user wants to use a more
    * complicated kernel, such as CSRAdaptive, it must be instantiated and
    * initialized by the user and the object must be passed to the
    * vectorProduct or reduceRows method.
    */
   using DefaultSegmentsReductionKernel = typename Algorithms::SegmentsReductionKernels::DefaultKernel< SegmentsView >::type;

   /**
    * \brief The type of matrix elements.
    */
   using RealType = typename Base::RealType;

   using ComputeRealType = ComputeReal;

   /**
    * \brief The device where the matrix is allocated.
    */
   using DeviceType = Device;

   /**
    * \brief The type used for matrix elements indexing.
    */
   using IndexType = Index;

   /**
    * \brief Type of segments view used by this matrix. It represents the sparse matrix format.
    */
   using SegmentsViewType = SegmentsView;

   /**
    * \brief Type for accessing matrix rows.
    */
   using RowView =
      SparseMatrixRowView< typename SegmentsViewType::SegmentViewType, typename Base::ValuesViewType, ColumnIndexesViewType >;

   /**
    * \brief Type for accessing constant matrix rows.
    */
   using ConstRowView = typename RowView::ConstRowView;

   /**
    * \brief Constructor with no parameters.
    */
   __cuda_callable__
   SparseMatrixBase() = default;

   /**
    * \brief Constructor with all necessary data and views.
    *
    * \param rows is a number of matrix rows.
    * \param columns is a number of matrix columns.
    * \param values is a vector view with matrix elements values.
    * \param columnIndexes is a vector view with matrix elements column indexes.
    * \param segments is a segments view representing the sparse matrix format.
    */
   __cuda_callable__
   SparseMatrixBase( IndexType rows,
                     IndexType columns,
                     typename Base::ValuesViewType values,
                     ColumnIndexesViewType columnIndexes,
                     SegmentsViewType segments );

   /**
    * \brief Copy constructor.
    *
    * \param matrix is an input sparse matrix view.
    */
   __cuda_callable__
   SparseMatrixBase( const SparseMatrixBase& matrix ) = default;

   /**
    * \brief Move constructor.
    *
    * \param matrix is an input sparse matrix view.
    */
   __cuda_callable__
   SparseMatrixBase( SparseMatrixBase&& matrix ) noexcept = default;

   /**
    * \brief Copy-assignment operator.
    *
    * It is a deleted function, because sparse matrix assignment in general
    * requires reallocation.
    */
   SparseMatrixBase&
   operator=( const SparseMatrixBase& ) = delete;

   /**
    * \brief Move-assignment operator.
    */
   SparseMatrixBase&
   operator=( SparseMatrixBase&& ) = delete;

   /**
    * \brief Returns string with serialization type.
    *
    * The string has a form `Matrices::SparseMatrix< RealType,  [any_device], IndexType, General/Symmetric, Format,
    * [any_allocator] >`.
    *
    * \return \ref String with the serialization type.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_getSerializationType.cpp
    * \par Output
    * \include SparseMatrixExample_getSerializationType.out
    */
   [[nodiscard]] static std::string
   getSerializationType();

   /**
    * \brief Computes number of non-zeros in each row.
    *
    * \param rowLengths is a vector into which the number of non-zeros in each row
    * will be stored.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_getCompressedRowLengths.cpp
    * \par Output
    * \include SparseMatrixExample_getCompressedRowLengths.out
    */
   template< typename Vector >
   void
   getCompressedRowLengths( Vector& rowLengths ) const;

   /**
    * \brief Compute capacities of all rows.
    *
    * The row capacities are not stored explicitly and must be computed.
    *
    * \param rowCapacities is a vector where the row capacities will be stored.
    */
   template< typename Vector >
   void
   getRowCapacities( Vector& rowCapacities ) const;

   /**
    * \brief Returns capacity of given matrix row.
    *
    * \param row index of matrix row.
    * \return number of matrix elements allocated for the row.
    */
   [[nodiscard]] __cuda_callable__
   IndexType
   getRowCapacity( IndexType row ) const;

   /**
    * \brief Returns number of non-zero matrix elements.
    *
    * This method really counts the non-zero matrix elements and so
    * it returns zero for matrix having all allocated elements set to zero.
    *
    * \return number of non-zero matrix elements.
    */
   [[nodiscard]] IndexType
   getNonzeroElementsCount() const override;

   /**
    * \brief Constant getter of simple structure for accessing given matrix row.
    *
    * \param rowIdx is matrix row index.
    *
    * \return RowView for accessing given matrix row.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixViewExample_getConstRow.cpp
    * \par Output
    * \include SparseMatrixViewExample_getConstRow.out
    *
    * See \ref SparseMatrixRowView.
    */
   [[nodiscard]] __cuda_callable__
   ConstRowView
   getRow( IndexType rowIdx ) const;

   /**
    * \brief Non-constant getter of simple structure for accessing given matrix row.
    *
    * \param rowIdx is matrix row index.
    *
    * \return RowView for accessing given matrix row.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixViewExample_getRow.cpp
    * \par Output
    * \include SparseMatrixViewExample_getRow.out
    *
    * See \ref SparseMatrixRowView.
    */
   [[nodiscard]] __cuda_callable__
   RowView
   getRow( IndexType rowIdx );

   /**
    * \brief Sets element at given \e row and \e column to given \e value.
    *
    * This method can be called from the host system (CPU) no matter
    * where the matrix is allocated. If the matrix is allocated on GPU this method
    * can be called even from device kernels. If the matrix is allocated in GPU device
    * this method is called from CPU, it transfers values of each matrix element separately and so the
    * performance is very low. For higher performance see. \ref SparseMatrix::getRow
    * or \ref SparseMatrix::forElements and \ref SparseMatrix::forAllElements.
    * The call may fail if the matrix row capacity is exhausted.
    *
    * \param row is row index of the element.
    * \param column is columns index of the element.
    * \param value is the value the element will be set to.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixViewExample_setElement.cpp
    * \par Output
    * \include SparseMatrixViewExample_setElement.out
    */
   __cuda_callable__
   void
   setElement( IndexType row, IndexType column, const RealType& value );

   /**
    * \brief Add element at given \e row and \e column to given \e value.
    *
    * This method can be called from the host system (CPU) no matter
    * where the matrix is allocated. If the matrix is allocated on GPU this method
    * can be called even from device kernels. If the matrix is allocated in GPU device
    * this method is called from CPU, it transfers values of each matrix element separately and so the
    * performance is very low. For higher performance see. \ref SparseMatrix::getRow
    * or \ref SparseMatrix::forElements and \ref SparseMatrix::forAllElements.
    * The call may fail if the matrix row capacity is exhausted.
    *
    * \param row is row index of the element.
    * \param column is columns index of the element.
    * \param value is the value the element will be set to.
    * \param thisElementMultiplicator is multiplicator the original matrix element
    *   value is multiplied by before addition of given \e value.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_addElement.cpp
    * \par Output
    * \include SparseMatrixExample_addElement.out
    */
   __cuda_callable__
   void
   addElement( IndexType row, IndexType column, const RealType& value, const RealType& thisElementMultiplicator = 1.0 );

   /**
    * \brief Returns value of matrix element at position given by its row and column index.
    *
    * This method can be called from the host system (CPU) no matter
    * where the matrix is allocated. If the matrix is allocated on GPU this method
    * can be called even from device kernels. If the matrix is allocated in GPU device
    * this method is called from CPU, it transfers values of each matrix element separately and so the
    * performance is very low. For higher performance see. \ref SparseMatrix::getRow
    * or \ref SparseMatrix::forElements and \ref SparseMatrix::forAllElements.
    *
    * \param row is a row index of the matrix element.
    * \param column i a column index of the matrix element.
    *
    * \return value of given matrix element.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_getElement.cpp
    * \par Output
    * \include SparseMatrixExample_getElement.out
    *
    */
   [[nodiscard]] __cuda_callable__
   RealType
   getElement( IndexType row, IndexType column ) const;

   /**
    * \brief Method for performing general reduction on matrix rows for constant instances.
    *
    * \tparam Fetch is a type of lambda function for data fetch declared as
    *
    * ```
    * auto fetch = [] __cuda_callable__ ( IndexType rowIdx, IndexType& columnIdx, RealType& elementValue ) -> FetchValue
    * { ... };
    * ```
    *
    * The return type of this lambda can be any non void.
    * \tparam Reduce is a type of lambda function for reduction declared as
    *
    * ```
    * auto reduce = [] __cuda_callable__ ( const FetchValue& v1, const FetchValue& v2 ) -> FetchValue { ... };
    * ```
    *
    * \tparam Keep is a type of lambda function for storing results of reduction in each row. It is declared as
    *
    * ```
    * auto keep = [=] __cuda_callable__ ( IndexType rowIdx, const RealType& value ) { ... };
    * ```
    *
    * \tparam FetchValue is type returned by the Fetch lambda function.
    *
    * \param begin defines beginning of the range `[begin, end)` of rows to be processed.
    * \param end defines ending of the range `[begin, end)` of rows to be processed.
    * \param fetch is an instance of lambda function for data fetch.
    * \param reduce is an instance of lambda function for reduction.
    * \param keep in an instance of lambda function for storing results.
    * \param identity is the [identity element](https://en.wikipedia.org/wiki/Identity_element)
    *                 for the reduction operation, i.e. element which does not
    *                 change the result of the reduction.
    * \param kernel is an instance of the segments reduction kernel to be used
    *               for the operation.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_reduceRows.cpp
    * \par Output
    * \include SparseMatrixExample_reduceRows.out
    */
   template< typename Fetch,
             typename Reduce,
             typename Keep,
             typename FetchValue,
             typename SegmentsReductionKernel = DefaultSegmentsReductionKernel >
   void
   reduceRows( IndexType begin,
               IndexType end,
               Fetch& fetch,
               const Reduce& reduce,
               Keep& keep,
               const FetchValue& identity,
               const SegmentsReductionKernel& kernel = SegmentsReductionKernel{} ) const;

   /**
    * \brief Method for performing general reduction on all matrix rows for constant instances.
    *
    * \tparam Fetch is a type of lambda function for data fetch declared as
    *
    * ```
    * auto fetch = [] __cuda_callable__ ( IndexType rowIdx, IndexType& columnIdx, RealType& elementValue ) -> FetchValue
    * { ... };
    * ```
    *
    * The return type of this lambda can be any non void.
    * \tparam Reduce is a type of lambda function for reduction declared as
    *
    * ```
    * auto reduce = [] __cuda_callable__ ( const FetchValue& v1, const FetchValue& v2 ) -> FetchValue { ... };
    * ```
    *
    * \tparam Keep is a type of lambda function for storing results of reduction in each row. It is declared as
    *
    * ```
    * auto keep = [=] __cuda_callable__ ( IndexType rowIdx, const RealType& value ) { ... };
    * ```
    *
    * \tparam FetchValue is type returned by the Fetch lambda function.
    *
    * \param fetch is an instance of lambda function for data fetch.
    * \param reduce is an instance of lambda function for reduction.
    * \param keep in an instance of lambda function for storing results.
    * \param identity is the [identity element](https://en.wikipedia.org/wiki/Identity_element)
    *                 for the reduction operation, i.e. element which does not
    *                 change the result of the reduction.
    * \param kernel is an instance of the segments reduction kernel to be used
    *               for the operation.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_reduceAllRows.cpp
    * \par Output
    * \include SparseMatrixExample_reduceAllRows.out
    */
   template< typename Fetch,
             typename Reduce,
             typename Keep,
             typename FetchValue,
             typename SegmentsReductionKernel = DefaultSegmentsReductionKernel >
   void
   reduceAllRows( Fetch& fetch,
                  const Reduce& reduce,
                  Keep& keep,
                  const FetchValue& identity,
                  const SegmentsReductionKernel& kernel = SegmentsReductionKernel{} ) const;

   /**
    * \brief Method for iteration over all matrix rows for constant instances.
    *
    * \tparam Function is type of lambda function that will operate on matrix elements. It should have form like
    *
    * ```
    * auto function = [] __cuda_callable__
    *                 ( IndexType rowIdx, IndexType localIdx, const IndexType& columnIdx, const RealType& value )
    * { ... };
    * ```
    *
    *  The \e localIdx parameter is a rank of the non-zero element in given row.
    *
    * \param begin defines beginning of the range `[begin, end)` of rows to be processed.
    * \param end defines ending of the range `[begin, end)` of rows to be processed.
    * \param function is an instance of the lambda function to be called in each row.
    */
   template< typename Function >
   void
   forElements( IndexType begin, IndexType end, Function&& function ) const;

   /**
    * \brief Method for iteration over all matrix rows for non-constant instances.
    *
    * \tparam Function is type of lambda function that will operate on matrix elements. It should have form like
    *
    * ```
    * auto function = [] __cuda_callable__ ( IndexType rowIdx, IndexType localIdx, IndexType& columnIdx, RealType& value )
    * { ... };
    * ```
    *
    *  The \e localIdx parameter is a rank of the non-zero element in given row.
    *
    * \param begin defines beginning of the range `[begin, end)` of rows to be processed.
    * \param end defines ending of the range `[begin, end)` of rows to be processed.
    * \param function is an instance of the lambda function to be called in each row.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_forElements.cpp
    * \par Output
    * \include SparseMatrixExample_forElements.out
    */
   template< typename Function >
   void
   forElements( IndexType begin, IndexType end, Function&& function );

   /**
    * \brief This method calls \e forElements for all matrix rows (for constant instances).
    *
    * See \ref SparseMatrix::forElements.
    *
    * \tparam Function is a type of lambda function that will operate on matrix elements.
    * \param function  is an instance of the lambda function to be called in each row.
    */
   template< typename Function >
   void
   forAllElements( Function&& function ) const;

   /**
    * \brief This method calls \e forElements for all matrix rows.
    *
    * See \ref SparseMatrix::forElements.
    *
    * \tparam Function is a type of lambda function that will operate on matrix elements.
    * \param function  is an instance of the lambda function to be called in each row.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_forAllElements.cpp
    * \par Output
    * \include SparseMatrixExample_forAllElements.out
    */
   template< typename Function >
   void
   forAllElements( Function&& function );

   /**
    * \brief Method for parallel iteration over matrix rows from interval `[begin, end)`.
    *
    * In each row, given lambda function is performed. Each row is processed by at most one thread unlike the method
    * \ref SparseMatrixBase::forElements where more than one thread can be mapped to each row.

    *
    * \tparam Function is type of the lambda function.
    *
    * \param begin defines beginning of the range `[begin, end)` of rows to be processed.
    * \param end defines ending of the range `[begin, end)` of rows to be processed.
    * \param function is an instance of the lambda function to be called for each row.
    *
    * ```
    * auto function = [] __cuda_callable__ ( RowView& row ) { ... };
    * ```
    *
    * \e RowView represents matrix row - see \ref noa::TNL::Matrices::SparseMatrixBase::RowView.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_forRows.cpp
    * \par Output
    * \include SparseMatrixExample_forRows.out
    */
   template< typename Function >
   void
   forRows( IndexType begin, IndexType end, Function&& function );

   /**
    * \brief Method for parallel iteration over matrix rows from interval `[begin, end)` for constant instances.
    *
    * In each row, given lambda function is performed. Each row is processed by at most one thread unlike the method
    * \ref SparseMatrixBase::forElements where more than one thread can be mapped to each row.
    *
    * \tparam Function is type of the lambda function.
    *
    * \param begin defines beginning of the range `[begin, end)` of rows to be processed.
    * \param end defines ending of the range `[begin, end)` of rows to be processed.
    * \param function is an instance of the lambda function to be called for each row.
    *
    * ```
    * auto function = [] __cuda_callable__ ( const ConstRowView& row ) { ... };
    * ```
    *
    * \e ConstRowView represents matrix row - see \ref noa::TNL::Matrices::SparseMatrixBase::ConstRowView.
    */
   template< typename Function >
   void
   forRows( IndexType begin, IndexType end, Function&& function ) const;

   /**
    * \brief Method for parallel iteration over all matrix rows.
    *
    * In each row, given lambda function is performed. Each row is processed by at most one thread unlike the method
    * \ref SparseMatrixBase::forAllElements where more than one thread can be mapped to each row.
    *
    * \tparam Function is type of the lambda function.
    *
    * \param function is an instance of the lambda function to be called for each row.
    *
    * ```
    * auto function = [] __cuda_callable__ ( RowView& row ) { ... };
    * ```
    *
    * \e RowView represents matrix row - see \ref noa::TNL::Matrices::SparseMatrixBase::RowView.
    *
    * \par Example
    * \include Matrices/SparseMatrix/SparseMatrixExample_forRows.cpp
    * \par Output
    * \include SparseMatrixExample_forRows.out
    */
   template< typename Function >
   void
   forAllRows( Function&& function );

   /**
    * \brief Method for parallel iteration over all matrix rows for constant instances.
    *
    * In each row, given lambda function is performed. Each row is processed by at most one thread unlike the method
    * \ref SparseMatrixBase::forAllElements where more than one thread can be mapped to each row.
    *
    * \tparam Function is type of the lambda function.
    *
    * \param function is an instance of the lambda function to be called for each row.
    *
    * ```
    * auto function = [] __cuda_callable__ ( const ConstRowView& row ) { ... };
    * ```
    *
    * \e ConstRowView represents matrix row - see \ref noa::TNL::Matrices::SparseMatrixBase::ConstRowView.
    */
   template< typename Function >
   void
   forAllRows( Function&& function ) const;

   /**
    * \brief Method for sequential iteration over all matrix rows for constant instances.
    *
    * \tparam Function is type of lambda function that will operate on matrix elements. It should have form like
    *
    * ```
    * auto function = [] __cuda_callable__ ( const ConstRowView& row ) { ... };
    * ```
    *
    * \e ConstRowView represents matrix row - see \ref noa::TNL::Matrices::SparseMatrixBase::ConstRowView.
    *
    * \param begin defines beginning of the range `[begin, end)` of rows to be processed.
    * \param end defines ending of the range `[begin, end)` of rows to be processed.
    * \param function is an instance of the lambda function to be called in each row.
    */
   template< typename Function >
   void
   sequentialForRows( IndexType begin, IndexType end, Function&& function ) const;

   /**
    * \brief Method for sequential iteration over all matrix rows for non-constant instances.
    *
    * \tparam Function is type of lambda function that will operate on matrix elements. It should have form like
    *
    * ```
    * auto function = [] __cuda_callable__ ( RowView& row ) { ... };
    * ```
    *
    * \e RowView represents matrix row - see \ref noa::TNL::Matrices::SparseMatrixBase::RowView.
    *
    * \param begin defines beginning of the range `[begin, end)` of rows to be processed.
    * \param end defines ending of the range `[begin, end)` of rows to be processed.
    * \param function is an instance of the lambda function to be called in each row.
    */
   template< typename Function >
   void
   sequentialForRows( IndexType begin, IndexType end, Function&& function );

   /**
    * \brief This method calls \e sequentialForRows for all matrix rows (for constant instances).
    *
    * See \ref SparseMatrixBase::sequentialForRows.
    *
    * \tparam Function is a type of lambda function that will operate on matrix elements.
    * \param function  is an instance of the lambda function to be called in each row.
    */
   template< typename Function >
   void
   sequentialForAllRows( Function&& function ) const;

   /**
    * \brief This method calls \e sequentialForRows for all matrix rows.
    *
    * See \ref SparseMatrixBase::sequentialForAllRows.
    *
    * \tparam Function is a type of lambda function that will operate on matrix elements.
    * \param function  is an instance of the lambda function to be called in each row.
    */
   template< typename Function >
   void
   sequentialForAllRows( Function&& function );

   /**
    * \brief Computes product of matrix and vector.
    *
    * More precisely, it computes:
    *
    * ```
    * outVector = matrixMultiplicator * ( * this ) * inVector + outVectorMultiplicator * outVector
    * ```
    *
    * \tparam InVector is type of input vector. It can be
    *         \ref noa::TNL::Containers::Vector, \ref noa::TNL::Containers::VectorView,
    *         \ref noa::TNL::Containers::Array, \ref noa::TNL::Containers::ArrayView,
    *         or similar container.
    * \tparam OutVector is type of output vector. It can be
    *         \ref noa::TNL::Containers::Vector, \ref noa::TNL::Containers::VectorView,
    *         \ref noa::TNL::Containers::Array, \ref noa::TNL::Containers::ArrayView,
    *         or similar container.
    *
    * \param inVector is input vector.
    * \param outVector is output vector.
    * \param matrixMultiplicator is a factor by which the matrix is multiplied. It is one by default.
    * \param outVectorMultiplicator is a factor by which the outVector is multiplied before added
    *    to the result of matrix-vector product. It is zero by default.
    * \param begin is the beginning of the rows range for which the vector product
    *    is computed. It is zero by default.
    * \param end is the end of the rows range for which the vector product
    *    is computed. It is number if the matrix rows by default.
    * \param kernel is an instance of the segments reduction kernel to be used
    *               for the operation.
    */
   template< typename InVector, typename OutVector, typename SegmentsReductionKernel = DefaultSegmentsReductionKernel >
   void
   vectorProduct( const InVector& inVector,
                  OutVector& outVector,
                  ComputeRealType matrixMultiplicator = 1.0,
                  ComputeRealType outVectorMultiplicator = 0.0,
                  IndexType begin = 0,
                  IndexType end = 0,
                  const SegmentsReductionKernel& kernel = SegmentsReductionKernel{} ) const;

   template< typename InVector,
             typename OutVector,
             typename SegmentsReductionKernel,
             typename...,
             std::enable_if_t< ! std::is_convertible_v< SegmentsReductionKernel, ComputeRealType >, bool > = true >
   void
   vectorProduct( const InVector& inVector, OutVector& outVector, const SegmentsReductionKernel& kernel ) const;

   /**
    * \brief Computes product of transposed matrix and vector.
    *
    * \tparam InVector is type of input vector. It can be
    *    \ref noa::TNL::Containers::Vector, \ref noa::TNL::Containers::VectorView,
    *    \ref noa::TNL::Containers::Array, \ref noa::TNL::Containers::ArrayView,
    *   or similar container.
    * \tparam OutVector is type of output vector. It can be
    *    \ref noa::TNL::Containers::Vector, \ref noa::TNL::Containers::VectorView,
    *    \ref noa::TNL::Containers::Array, \ref noa::TNL::Containers::ArrayView,
    *  or similar container.
    *
    * \param inVector is input vector.
    * \param outVector  is output vector.
    * \param matrixMultiplicator is a factor by which the matrix is multiplied. It is one by default.
    * \param outVectorMultiplicator is a factor by which the outVector is multiplied before added
    * \param begin is the beginning of the rows range for which the vector product
    *    is computed. It is zero by default.
    * \param end is the end of the rows range for which the vector product
    *    is computed. It is number if the matrix rows by default.
    */
   template< typename InVector, typename OutVector >
   void
   transposedVectorProduct( const InVector& inVector,
                            OutVector& outVector,
                            ComputeReal matrixMultiplicator = 1.0,
                            ComputeReal outVectorMultiplicator = 0.0,
                            Index begin = 0,
                            Index end = 0 ) const;

   /**
    * \brief Comparison operator with another arbitrary matrix type.
    *
    * \param matrix is the right-hand side matrix.
    * \return \e true if the RHS matrix is equal, \e false otherwise.
    */
   template< typename Matrix >
   [[nodiscard]] bool
   operator==( const Matrix& matrix ) const;

   /**
    * \brief Comparison operator with another arbitrary matrix type.
    *
    * \param matrix is the right-hand side matrix.
    * \return \e false if the RHS matrix is equal, \e true otherwise.
    */
   template< typename Matrix >
   [[nodiscard]] bool
   operator!=( const Matrix& matrix ) const;

   /**
    * \brief Sort matrix elements in each row by column indexes in ascending order.
    */
   void
   sortColumnIndexes();

   /**
    * \brief Finds element in the matrix and returns its position in the arrays with \e values and \e columnIndexes.
    *
    * If the element is not found, the method returns the padding index.
    * \param row is the row index of the element.
    * \param column is the column index of the element.
    * \return the position of the element in the arrays with \e values and \e columnIndexes or the padding index if the element
    * is not found.
    */
   __cuda_callable__
   IndexType
   findElement( IndexType row, IndexType column ) const;

   /**
    * \brief Method for printing the matrix to output stream.
    *
    * \param str is the output stream.
    */
   void
   print( std::ostream& str ) const;

   /**
    * \brief Getter of segments for non-constant instances.
    *
    * \e Segments are a structure for addressing the matrix elements columns and values.
    * In fact, \e Segments represent the sparse matrix format.
    *
    * \return Non-constant reference to segments.
    */
   [[nodiscard]] SegmentsViewType&
   getSegments();

   /**
    * \brief Getter of segments for constant instances.
    *
    * \e Segments are a structure for addressing the matrix elements columns and values.
    * In fact, \e Segments represent the sparse matrix format.
    *
    * \return Constant reference to segments.
    */
   [[nodiscard]] const SegmentsViewType&
   getSegments() const;

   /**
    * \brief Getter of column indexes for constant instances.
    *
    * \return Constant reference to a vector with matrix elements column indexes.
    */
   [[nodiscard]] const ColumnIndexesViewType&
   getColumnIndexes() const;

   /**
    * \brief Getter of column indexes for nonconstant instances.
    *
    * \return Reference to a vector with matrix elements column indexes.
    */
   [[nodiscard]] ColumnIndexesViewType&
   getColumnIndexes();

protected:
   ColumnIndexesViewType columnIndexes;

   SegmentsViewType segments;

   /**
    * \brief Re-initializes the internal attributes of the base class.
    *
    * Note that this function is \e protected to ensure that the user cannot
    * modify the base class of a matrix. For the same reason, in future code
    * development we also need to make sure that all non-const functions in
    * the base class return by value and not by reference.
    */
   __cuda_callable__
   void
   bind( IndexType rows,
         IndexType columns,
         typename Base::ValuesViewType values,
         ColumnIndexesViewType columnIndexes,
         SegmentsViewType segments );
};

/**
 * \brief Overloaded insertion operator for printing a matrix to output stream.
 *
 * \tparam Real is a type of the matrix elements.
 * \tparam Device is a device where the matrix is allocated.
 * \tparam Index is a type used for the indexing of the matrix elements.
 *
 * \param str is a output stream.
 * \param matrix is the matrix to be printed.
 *
 * \return a reference to the output stream \ref std::ostream.
 */
template< typename Real, typename Device, typename Index, typename MatrixType, typename SegmentsView, typename ComputeReal >
std::ostream&
operator<<( std::ostream& str, const SparseMatrixBase< Real, Device, Index, MatrixType, SegmentsView, ComputeReal >& matrix )
{
   matrix.print( str );
   return str;
}

}  // namespace noa::TNL::Matrices

#include "SparseMatrixBase.hpp"
