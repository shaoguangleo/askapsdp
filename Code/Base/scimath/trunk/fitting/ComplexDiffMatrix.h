/// @file
/// 
/// @brief A matrix of ComplexDiff classes
/// @details The calibration code constructs normal equations for each
/// row of the data accessor, i.e. a matrix with dimensions nchan x npol.
/// When a design matrix is constructed, all elements of this matrix are
/// treated independently. However, it is better to retain a basic matrix
/// algebra to ensure the code is clear. This class also treats well a
/// possible degenerate dimension (polarisation). Theoretically, 
/// casa::Matrix<ComplexDiff> could be used instead of this class. However,
/// having a separate class allows, in principle, to handle maps of the 
/// parameters at the matrix level and don't duplicate the map search 
/// unnecessarily. Such functionality is in the future plans, but it is
/// hidden behind the interface of this class.
///
/// @copyright (c) 2007 CONRAD, All Rights Reserved.
/// @author Max Voronkov <maxim.voronkov@csiro.au>


#ifndef COMPLEXDIFFMATRIX_H
#define COMPLEXDIFFMATRIX_H

// casa includes
#include <casa/BasicSL/Complex.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>

// std includes
#include <vector>
#include <string>

// own includes
#include <fitting/ComplexDiff.h>
#include <conrad/ConradError.h>

namespace conrad {

namespace scimath {

/// @brief A matrix of ComplexDiff classes
/// @details The calibration code constructs normal equations for each
/// row of the data accessor, i.e. a matrix with dimensions nchan x npol.
/// When a design matrix is constructed, all elements of this matrix are
/// treated independently. However, it is better to retain a basic matrix
/// algebra to ensure the code is clear. This class also treats well a
/// possible degenerate dimension (polarisation). Theoretically, 
/// casa::Matrix<ComplexDiff> could be used instead of this class. However,
/// having a separate class allows, in principle, to handle maps of the 
/// parameters at the matrix level and don't duplicate the map search 
/// unnecessarily. Such functionality is in the future plans, but it is
/// hidden behind the interface of this class.
struct ComplexDiffMatrix {
   /// @brief constant iterator type used to access flattened storage
   typedef std::vector<ComplexDiff>::const_iterator const_iterator;
   
   /// @brief iterator corresponding to the origin of the sequence
   /// @return iterator corresponding to the origin of the sequence
   inline const_iterator begin() const { return itsElements.begin(); }
   
   /// @brief iterator corresponding to the end of the sequence
   /// @return iterator corresponding to the end of the sequence
   inline const_iterator end() const { return itsElements.end(); }
   
   /// @brief constructor of an empty matrix with the given dimensions
   /// @param[in] nrow number of rows
   /// @param[in] ncol number of columns
   explicit inline ComplexDiffMatrix(size_t nrow, size_t ncol = 1) : itsNRows(nrow),
               itsNColumns(ncol), itsElements(nrow*ncol) {}
   
   /// @brief constructor of an initialized matrix with the given dimensions
   /// @param[in] nrow number of rows
   /// @param[in] ncol number of columns
   /// @param[in] val value
   inline ComplexDiffMatrix(size_t nrow, size_t ncol, const ComplexDiff &val) : 
         itsNRows(nrow), itsNColumns(ncol), itsElements(nrow*ncol, val) {}
   
   /// @brief constructor of an initialized vector with the given length
   /// @param[in] nrow number of rows
   /// @param[in] val value
   inline ComplexDiffMatrix(size_t nrow, const ComplexDiff &val) : 
         itsNRows(nrow), itsNColumns(1), itsElements(nrow, val) {}
         
   /// @brief constructor from casa::Matrix
   /// @param[in] matr input matrix
   inline ComplexDiffMatrix(const casa::Matrix<casa::Complex> &matr);    

   /// @brief constructor from casa::Vector
   /// @param[in] vec input matrix
   inline ComplexDiffMatrix(const casa::Vector<casa::Complex> &vec);    
   
   /// @brief access to given matrix element
   /// @param[in] row row
   /// @param[in] col column
   /// @return element
   inline const ComplexDiff& operator()(size_t row, size_t col) const;
   
   /// @brief read/write access to given matrix element
   /// @param[in] row row
   /// @param[in] col column
   /// @return element
   inline ComplexDiff& operator()(size_t row, size_t col);
   
   /// @brief matrix multiplication
   /// @details
   /// @param[in] in1 first matrix
   /// @param[in] in2 second matrix
   /// @return product of the first and the second matrices
   friend inline ComplexDiffMatrix operator*(const ComplexDiffMatrix &in1,
                const ComplexDiffMatrix &in2);

   /// @brief matrix addition
   /// @details
   /// @param[in] in1 first matrix
   /// @param[in] in2 second matrix
   /// @return a sum of the first and the second matrices
   friend inline ComplexDiffMatrix operator+(const ComplexDiffMatrix &in1,
                const ComplexDiffMatrix &in2);
  
   /// @brief obtain number of rows
   /// @return number of rows
   inline size_t nRow() const { return itsNRows;}
   
   /// @brief obtain number of columns
   /// @return number of columns
   inline size_t nColumn() const {return itsNColumns;}
    
private:
   /// @brief number of rows (channels in the calibration framework)
   size_t itsNRows;
   /// @brief number of columns (polarisations in the calibration framework)
   size_t itsNColumns; 
   /// @brief flattened storage for the matrix elements
   std::vector<ComplexDiff> itsElements;
};


/// @brief access to given matrix element
/// @param[in] row row
/// @param[in] col column
/// @return element
inline const ComplexDiff& ComplexDiffMatrix::operator()(size_t row, size_t col) const
{
   CONRADDEBUGASSERT(row<itsNRows && col<itsNColumns);
   return itsElements[itsNRows*col+row];
}

/// @brief access to given matrix element
/// @param[in] row row
/// @param[in] col column
/// @return element
inline ComplexDiff& ComplexDiffMatrix::operator()(size_t row, size_t col)
{
   CONRADDEBUGASSERT(row<itsNRows && col<itsNColumns);
   return itsElements[itsNRows*col+row];
}

/// @brief constructor from casa::Matrix
/// @param[in] matr input matrix
inline ComplexDiffMatrix::ComplexDiffMatrix(const casa::Matrix<casa::Complex> &matr) :
     itsNRows(matr.nrow()), itsNColumns(matr.ncolumn()), 
     itsElements(matr.nrow()*matr.ncolumn())    
{
   std::vector<ComplexDiff>::iterator it = itsElements.begin();
   for (casa::uInt col=0; col<itsNColumns; ++col) {
        for (casa::uInt row=0; row<itsNRows; ++row,++it) {
             *it = matr(row,col);
        }
   }
}

/// @brief constructor from casa::Vector
/// @param[in] vec input vector
inline ComplexDiffMatrix::ComplexDiffMatrix(const casa::Vector<casa::Complex> &vec) :
     itsNRows(vec.nelements()), itsNColumns(1), itsElements(vec.nelements())    
{
   std::vector<ComplexDiff>::iterator it = itsElements.begin();
   for (casa::uInt row=0; row<itsNRows; ++row,++it) {
        *it = vec[row];
   }
}

/// @brief matrix multiplication
/// @details
/// @param[in] in1 first matrix
/// @param[in] in2 second matrix
/// @return product of the first and the second matrices
inline ComplexDiffMatrix operator*(const ComplexDiffMatrix &in1,
                const ComplexDiffMatrix &in2)
{
   CONRADDEBUGASSERT(in1.nColumn() == in2.nRow());
   ComplexDiffMatrix result(in1.nRow(), in2.nColumn());
   size_t curCol = 0, curRow = 0;
   for (std::vector<ComplexDiff>::iterator it = result.itsElements.begin();
        it != result.itsElements.end(); ++it,++curRow) {
        *it = ComplexDiff(casa::Complex(0.,0.));
        if (curRow >= result.nRow()) {
            curRow = 0;
            ++curCol;
        }
        for (size_t index=0; index<in1.nColumn(); ++index) {
             *it += in1(curRow,index)*in2(index,curCol);
        }
   }
   
   return result;
}

/// @brief matrix addition
/// @details
/// @param[in] in1 first matrix
/// @param[in] in2 second matrix
/// @return a sum of the first and the second matrices
inline ComplexDiffMatrix operator+(const ComplexDiffMatrix &in1,
                const ComplexDiffMatrix &in2)
{
  CONRADDEBUGASSERT(in1.nColumn() == in2.nColumn());
  CONRADDEBUGASSERT(in1.nRow() == in2.nRow());
  ComplexDiffMatrix result(in1);
  ComplexDiffMatrix::const_iterator ci = in2.begin();
  for (std::vector<ComplexDiff>::iterator it = result.itsElements.begin();
             it != result.itsElements.end(); ++it,++ci) {
       *it += *ci;
  }
}

} // namepace scimath

} // namespace conrad

#endif // #ifndef COMPLEXDIFFMATRIX_H