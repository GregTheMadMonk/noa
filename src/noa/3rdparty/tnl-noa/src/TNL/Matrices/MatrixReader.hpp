// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <iomanip>
#include <sstream>
#include <map>

#include <noa/3rdparty/tnl-noa/src/TNL/String.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Vector.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Timer.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Matrices/MatrixReader.h>

namespace noa::TNL::Matrices {

template< typename Matrix, typename Device >
void
MatrixReader< Matrix, Device >::readMtx( const std::string& fileName, Matrix& matrix, bool verbose )
{
   HostMatrix hostMatrix;
   MatrixReader< HostMatrix >::readMtx( fileName, hostMatrix, verbose );
   matrix = hostMatrix;
}

template< typename Matrix, typename Device >
void
MatrixReader< Matrix, Device >::readMtx( std::istream& str, Matrix& matrix, bool verbose )
{
   HostMatrix hostMatrix;
   MatrixReader< HostMatrix >::readMtx( str, hostMatrix, verbose );
   matrix = hostMatrix;
}

// MatrixReader specialization for noa::TNL::Devices::Host.

// This is to prevent Doxygen warnings due to hidden class.
/// \cond
template< typename Matrix >
void
MatrixReader< Matrix, noa::TNL::Devices::Host >::readMtx( const std::string& fileName, Matrix& matrix, bool verbose )
{
   std::fstream file;
   file.open( fileName, std::ios::in );
   if( ! file )
      throw std::runtime_error( std::string( "I am not able to open the file " ) + fileName );
   readMtx( file, matrix, verbose );
}

template< typename Matrix >
void
MatrixReader< Matrix, noa::TNL::Devices::Host >::readMtx( std::istream& file, Matrix& matrix, bool verbose )
{
   IndexType rows = 0;
   IndexType columns = 0;
   bool symmetricSourceMatrix = false;

   readMtxHeader( file, rows, columns, symmetricSourceMatrix, verbose );

   if( Matrix::isSymmetric() && ! symmetricSourceMatrix )
      throw std::runtime_error( "Matrix is not symmetric, but flag for symmetric matrix is given. Aborting." );

   if( verbose )
      std::cout << "Matrix dimensions are " << rows << " x " << columns << std::endl;
   matrix.setDimensions( rows, columns );

   readMatrixElementsFromMtxFile( file, matrix, symmetricSourceMatrix, verbose );
}

template< typename Matrix >
void
MatrixReader< Matrix, noa::TNL::Devices::Host >::verifyMtxFile( std::istream& file, const Matrix& matrix, bool verbose )
{
   bool symmetricSourceMatrix = false;
   IndexType rows = 0;
   IndexType columns = 0;
   readMtxHeader( file, rows, columns, symmetricSourceMatrix, false );

   file.clear();
   file.seekg( 0, std::ios::beg );
   String line;
   bool dimensionsLine = false;
   IndexType processedElements = 0;
   Timer timer;
   timer.start();
   while( std::getline( file, line ) ) {
      if( line[ 0 ] == '%' )
         continue;
      if( ! dimensionsLine ) {
         dimensionsLine = true;
         continue;
      }
      IndexType row = 1;
      IndexType column = 1;
      RealType value;
      parseMtxLineWithElement( line, row, column, value );
      if( value != matrix.getElement( row - 1, column - 1 )
          || ( symmetricSourceMatrix && value != matrix.getElement( column - 1, row - 1 ) ) )
      {
         std::stringstream str;
         str << "*** !!! VERIFICATION ERROR !!! *** " << std::endl
             << "The elements differ at " << row - 1 << " row " << column - 1 << " column." << std::endl
             << "The matrix value is " << matrix.getElement( row - 1, column - 1 ) << " while the file value is " << value
             << "." << std::endl;
         throw std::runtime_error( str.str() );
      }
      processedElements++;
      if( symmetricSourceMatrix && row != column )
         processedElements++;
      if( verbose )
         std::cout << " Verifying the matrix elements ... " << processedElements << " / " << matrix.getNumberOfMatrixElements()
                   << "                       \r" << std::flush;
   }
   file.clear();
   long int fileSize = file.tellg();
   timer.stop();
   if( verbose )
      std::cout << " Verifying the matrix elements ... " << processedElements << " / " << matrix.getNumberOfMatrixElements()
                << " -> " << timer.getRealTime() << " sec. i.e. " << fileSize / ( timer.getRealTime() * ( 1 << 20 ) ) << "MB/s."
                << std::endl;
}

template< typename Matrix >
bool
MatrixReader< Matrix, noa::TNL::Devices::Host >::findLineByElement( std::istream& file,
                                                               const IndexType& row,
                                                               const IndexType& column,
                                                               String& line,
                                                               IndexType& lineNumber )
{
   file.clear();
   file.seekg( 0, std::ios::beg );
   bool symmetricSourceMatrix = false;
   bool dimensionsLine = false;
   lineNumber = 0;
   while( std::getline( file, line ) ) {
      lineNumber++;
      if( line[ 0 ] == '%' )
         continue;
      if( ! dimensionsLine ) {
         dimensionsLine = true;
         continue;
      }
      IndexType currentRow = 1;
      IndexType currentColumn = 1;
      RealType value;
      parseMtxLineWithElement( line, currentRow, currentColumn, value );
      if( ( currentRow == row + 1 && currentColumn == column + 1 )
          || ( symmetricSourceMatrix && currentRow == column + 1 && currentColumn == row + 1 ) )
         return true;
   }
   return false;
}

template< typename Matrix >
void
MatrixReader< Matrix, noa::TNL::Devices::Host >::checkMtxHeader( const String& header, bool& symmetric )
{
   std::vector< String > parsedLine = header.split( ' ', String::SplitSkip::SkipEmpty );
   if( (int) parsedLine.size() < 5 || parsedLine[ 0 ] != "%%MatrixMarket" )
      throw std::runtime_error(
         "Unknown format of the source file. We expect line like this: %%MatrixMarket matrix coordinate real general" );
   if( parsedLine[ 1 ] != "matrix" )
      throw std::runtime_error( std::string( "Keyword 'matrix' is expected in the header line: " ) + header.getString() );
   if( parsedLine[ 2 ] != "coordinates" && parsedLine[ 2 ] != "coordinate" )
      throw std::runtime_error( std::string( "Error: Only 'coordinates' format is supported now, not " )
                                + parsedLine[ 2 ].getString() );
   if( parsedLine[ 3 ] != "real" && parsedLine[ 3 ] != "integer" )
      throw std::runtime_error( std::string( "Only 'real' and 'integer' matrices are supported, not " )
                                + parsedLine[ 3 ].getString() );
   if( parsedLine[ 4 ] != "general" ) {
      if( parsedLine[ 4 ] == "symmetric" )
         symmetric = true;
      else
         throw std::runtime_error( std::string( "Only 'general' matrices are supported, not " ) + parsedLine[ 4 ].getString() );
   }
}

template< typename Matrix >
void
MatrixReader< Matrix, noa::TNL::Devices::Host >::readMtxHeader( std::istream& file,
                                                           IndexType& rows,
                                                           IndexType& columns,
                                                           bool& symmetric,
                                                           bool verbose )
{
   file.clear();
   file.seekg( 0, std::ios::beg );
   String line;
   bool headerParsed = false;
   std::vector< String > parsedLine;
   while( true ) {
      std::getline( file, line );
      if( ! headerParsed ) {
         checkMtxHeader( line, symmetric );
         headerParsed = true;
         if( verbose && symmetric )
            std::cout << "The matrix is SYMMETRIC ... ";
         continue;
      }
      if( line[ 0 ] == '%' )
         continue;

      parsedLine = line.split( ' ', String::SplitSkip::SkipEmpty );
      if( (int) parsedLine.size() != 3 )
         throw std::runtime_error( "Wrong number of parameters in the matrix header - should be 3." );
      rows = atoi( parsedLine[ 0 ].getString() );
      columns = atoi( parsedLine[ 1 ].getString() );
      if( verbose )
         std::cout << " The matrix has " << rows << " rows and " << columns << " columns. " << std::endl;

      if( rows <= 0 || columns <= 0 )
         throw std::runtime_error( "Row or column index is negative." );
      break;
   }
}

template< typename Matrix >
void
MatrixReader< Matrix, noa::TNL::Devices::Host >::readMatrixElementsFromMtxFile( std::istream& file,
                                                                           Matrix& matrix,
                                                                           bool symmetricMatrix,
                                                                           bool verbose )
{
   file.clear();
   file.seekg( 0, std::ios::beg );
   String line;
   bool dimensionsLine = false;
   IndexType processedElements = 0;
   Timer timer;
   timer.start();

   // Reading the elements first into an std::map and then copying to the matrix
   // has higher memory requirements, but avoids having to read the file twice.
   // Also the SparseMatrix::setElements method is more efficient than calling
   // SparseMatrix::setElement over and over...
   std::map< std::pair< IndexType, IndexType >, RealType > map;

   while( std::getline( file, line ) ) {
      if( line.empty() || line[ 0 ] == '%' )
         continue;
      if( ! dimensionsLine ) {
         dimensionsLine = true;
         continue;
      }
      IndexType row = 1;
      IndexType column = 1;
      RealType value;
      parseMtxLineWithElement( line, row, column, value );

      if( column > matrix.getColumns() || row > matrix.getRows() ) {
         std::stringstream str;
         str << "There is an element at position " << row << ", " << column << " out of the matrix dimensions "
             << matrix.getRows() << " x " << matrix.getColumns() << ".";
         throw std::runtime_error( str.str() );
      }

      if( ! Matrix::isSymmetric() || ( Matrix::isSymmetric() && row >= column ) )
         map[ { row - 1, column - 1 } ] = value;
      else if( Matrix::isSymmetric() && row < column )
         map[ { column - 1, row - 1 } ] = value;
      processedElements++;

      if( symmetricMatrix && row != column && ! Matrix::isSymmetric() ) {
         map[ { column - 1, row - 1 } ] = value;
         processedElements++;
      }

      if( verbose && processedElements % 1000 == 0 )
         std::cout << " Reading the matrix elements ... " << processedElements / 1000 << " thousands      \r" << std::flush;
   }

   file.clear();
   const long int fileSize = file.tellg();
   timer.stop();
   if( verbose )
      std::cout << " Reading the matrix elements ... " << processedElements << " -> " << timer.getRealTime() << " sec. i.e. "
                << fileSize / ( timer.getRealTime() * ( 1 << 20 ) ) << "MB/s." << std::endl;

   timer.reset();
   timer.start();

   if( verbose )
      std::cout << " Copying matrix elements from std::map to the matrix ... \r" << std::flush;
   matrix.setElements( map );

   timer.stop();
   const long int dataSize = processedElements * ( sizeof( RealType ) + sizeof( IndexType ) );
   if( verbose )
      std::cout << " Copying matrix elements from std::map to the matrix ... -> " << timer.getRealTime() << " sec. i.e. "
                << dataSize / ( timer.getRealTime() * ( 1 << 20 ) ) << "MB/s." << std::endl;
}

template< typename Matrix >
void
MatrixReader< Matrix, noa::TNL::Devices::Host >::parseMtxLineWithElement( const String& line,
                                                                     IndexType& row,
                                                                     IndexType& column,
                                                                     RealType& value )
{
   std::vector< String > parsedLine = line.split( ' ', String::SplitSkip::SkipEmpty );
   if( (int) parsedLine.size() != 3 ) {
      std::stringstream str;
      str << "Wrong number of parameters in the matrix row at line:" << line;
      throw std::runtime_error( str.str() );
   }
   row = atoi( parsedLine[ 0 ].getString() );
   column = atoi( parsedLine[ 1 ].getString() );
   value = (RealType) atof( parsedLine[ 2 ].getString() );
}
/// \endcond

}  // namespace noa::TNL::Matrices
