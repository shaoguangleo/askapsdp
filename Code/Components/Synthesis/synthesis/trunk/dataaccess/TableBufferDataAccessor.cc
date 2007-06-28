/// @file
/// @brief an implementation of IDataAccessor for buffers
///
/// @details TableDataAccessor is an implementation of the
/// DataAccessor working with TableDataIterator. It deals with
/// writable buffers only. Another class TableDataAccessor is
/// intended to write to the original visibility data.
///
/// @copyright (c) 2007 CONRAD, All Rights Reserved.
/// @author Max Voronkov <maxim.voronkov@csiro.au>
///

/// own includes
#include <dataaccess/TableBufferDataAccessor.h>
#include <dataaccess/DataAccessError.h>
#include <dataaccess/TableDataIterator.h>

using namespace conrad;
using namespace synthesis;

/// construct an object linked with the given const accessor
/// @param iter a reference to the associated accessor
TableBufferDataAccessor::TableBufferDataAccessor(const TableConstDataAccessor &acc) :
                 MetaDataAccessor(acc) {}

/// Read-only visibilities (a cube is nRow x nChannel x nPol; 
/// each element is a complex visibility)
///
/// @return a reference to nRow x nChannel x nPol cube, containing
/// all visibility data
///
const casa::Cube<casa::Complex>& TableBufferDataAccessor::visibility() const
{
  // active buffer should be returned
  fillBufferIfNeeded();
  return itsScratchBuffer.vis;  
}

/// read the information into the buffer if necessary
void TableBufferDataAccessor::fillBufferIfNeeded() const
{
  if (itsScratchBuffer.needsRead) {
      CONRADDEBUGASSERT(!itsScratchBuffer.needsFlush);

      // a call to iterator method will be here
      //
      itsScratchBuffer.needsRead=false;
  }
}

/// Read-write access to visibilities (a cube is nRow x nChannel x nPol;
/// each element is a complex visibility)
///
/// @return a reference to nRow x nChannel x nPol cube, containing
/// all visibility data
///
casa::Cube<casa::Complex>& TableBufferDataAccessor::rwVisibility()
{    
  // active buffer should be returned      
  fillBufferIfNeeded();
  itsScratchBuffer.needsFlush=true;
  return itsScratchBuffer.vis;      
 
  /*
  // original visibility is requested
  itsVisNeedsFlush=true;
  throw DataAccessLogicError("rwVisibility() for original visibilities is "
                                 "not yet implemented");  
  return const_cast<casa::Cube<casa::Complex>&>(getROAccessor().visibility());
  */
}


/// set needsFlush fkag to false (i.e. used after the visibility scratch 
/// buffer is synchronized with the disk)
void TableBufferDataAccessor::notifySyncCompleted() throw()
{
  itsScratchBuffer.needsFlush=false;  
}

/// @return True if the visibilities need to be written back
bool TableBufferDataAccessor::needSync() const throw()
{
  return itsScratchBuffer.needsFlush;
}

/// set needsRead flag to true (i.e. used following an iterator step
/// to force updating the cache on the next data request
void TableBufferDataAccessor::notifyNewIteration() throw()
{
  itsScratchBuffer.needsRead=true;
}