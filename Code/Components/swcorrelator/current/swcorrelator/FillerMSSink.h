/// @file 
///
/// @brief Actual MS writer class doing the low-level dirty job
/// @details This class is heavily based on Ben's MSSink in the CP/ingestpipeline package.
/// I just copied the appropriate code from there. The basic approach is to set up as
/// much of the metadata as we can via the parset file. It is envisaged that we may
/// use this class also for the conversion of the DiFX output into MS. 
///
/// @copyright (c) 2007 CSIRO
/// Australia Telescope National Facility (ATNF)
/// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
/// PO Box 76, Epping NSW 1710, Australia
/// atnf-enquiries@csiro.au
///
/// This file is part of the ASKAP software distribution.
///
/// The ASKAP software distribution is free software: you can redistribute it
/// and/or modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of the License,
/// or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Max Voronkov <maxim.voronkov@csiro.au>


#ifndef ASKAP_SWCORRELATOR_FILLER_MS_SINK
#define ASKAP_SWCORRELATOR_FILLER_MS_SINK

// own includes
#include <swcorrelator/CorrProducts.h>
#include <swcorrelator/ISink.h>
#include <askap/IndexConverter.h>

// casa includes
#include "ms/MeasurementSets/MeasurementSet.h"
#include <casa/BasicSL.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Quanta.h>
#include <measures/Measures/Stokes.h>
#include <measures/Measures/MDirection.h>
#include <measures/Measures/MEpoch.h>


// other 3rd party
#include <Common/ParameterSet.h>

// boost includes
#include "boost/scoped_ptr.hpp"
#include <boost/utility.hpp>

// std includes
#include <string>

namespace askap {

namespace swcorrelator {

/// @brief Actual MS writer class doing the low-level dirty job
/// @details This class is heavily based on Ben's MSSink in the CP/ingestpipeline package.
/// I just copied the appropriate code from there. The basic approach is to set up as
/// much of the metadata as we can via the parset file. It is envisaged that we may
/// use this class also for the conversion of the DiFX output into MS. 
/// @ingroup swcorrelator
class FillerMSSink : public ISink {
public:
  /// @brief constructor, sets up  MS writer
  /// @details Configuration is done via the parset, a lot of the metadata are just filled
  /// via the parset.
  /// @param[in] parset parset file with configuration info
  explicit FillerMSSink(const LOFAR::ParameterSet &parset);

  /// @brief calculate uvw for the given buffer
  /// @param[in] buf products buffer
  /// @note The calculation is bypassed if itsUVWValid flag is already set in the buffer
  /// @return time epoch corresponding to the BAT of the buffer
  virtual casa::MEpoch calculateUVW(CorrProducts &buf) const;
  
  /// @brief write one buffer to the measurement set
  /// @details Current fieldID and dataDescID are assumed
  /// @param[in] buf products buffer
  /// @note This method could've received a const reference to the buffer. However, more
  /// workarounds would be required with casa arrays, so we don't bother doing this at the moment.
  /// In addition, we could call calculateUVW inside this method (but we still need an option to
  /// calculate uvw's ahead of writing the buffer if we implement some form of delay tracking).
  virtual void write(CorrProducts &buf);
  
  
  /// @brief obtain the number of channels in the current setup
  /// @details This method throws an exception if the number of channels has not been
  /// set up (normally it takes place when MS is initialised)
  /// @return the number of channels in the active configuration
  int nChan() const;
  
  /// @brief obtain number of defined data descriptors
  /// @return number of data descriptors
  int numDataDescIDs() const;
  
  /// @brief set new default data descriptor
  /// @details This will be used for all future write operations
  /// @param[in] desc new data descriptor
  void setDataDescID(const int desc);  

  /// @brief obtain number of beams in the current setup
  /// @details This method throws an exception if the number of beams has not been
  /// set up  (normally it takes place when MS is initialised)
  /// @return the number of beams
  int nBeam() const;

  /// @brief return baseline index for a given baseline
  /// @details The data are passed in CorrProducts structure gathering all baselines in a single
  /// matrix (for visibility data and for flags). There is a standard order (see also CorrProducts)
  /// of baselines. In the software correlator itself, the data are produced directly in the standard
  /// order, but this method is handy for other uses of this class (i.e. format converter). It
  /// returns an index for a given baseline
  /// @param[in] ant1 zero-based index of the first antenna  
  /// @param[in] ant2 zero-based index of the second antenna
  /// @return index into visibility of flag matrix (row of the matrix)
  /// @note a negative value is returned if the given baseline is not found, no substitution is performed
  /// with the recent changes to CorrProducts this methid could be made redundant, provide it for
  /// compatibility for the time being
  static int baselineIndex(const casa::uInt ant1, const casa::uInt ant2);
  
protected:
  /// @brief helper method to substitute antenna index
  /// @details This is required to be able to use 4th (or potentially even more) antennas 
  /// connected through beamformer of another antenna. The correlator is still running in 3-antenna mode,
  /// but records the given beam data as correlations with extra antennas (so a useful measurement set is
  /// produced). The method substitutes an index in the range of 0-2 to an index > 2 if the appropriate
  /// beam and antenna are selected
  /// @param[in] antenna input antenna index
  /// @param[in] beam beam index (controls whether and how the substitution is done)
  /// @return output antenna index into itsAntXYZ
  int substituteAntId(const int antenna, const int beam) const;

  /// @brief helper method to make a string out of an integer
  /// @param[in] in unsigned integer number
  /// @return a string padded with zero on the left size, if necessary
  static std::string makeString(const casa::uInt in);
    
  /// @brief Initialises ANTENNA and FEED tables
  /// @details This method extracts configuration from the parset and fills in the 
  /// compulsory ANTENNA and FEED tables. It also caches antenna positions and beam offsets 
  /// in the form suitable for calculation of uvw's.
  void initAntennasAndBeams();
  
  
  /// @brief read beam information, populate itsBeamOffsets
  void readBeamInfo();
  
  /// @brief initialises field information
  void initFields();
  
  /// @brief initialises spectral and polarisation info (data descriptor)
  void initDataDesc();
    
  /// @brief Create the measurement set
  void create();

  // methods borrowed from Ben's MSSink class (see CP/ingest)

  // Add observation table row
  casa::Int addObs(const casa::String& telescope,
             const casa::String& observer,
             const double obsStartTime,
             const double obsEndTime);

  // Add field table row
  casa::Int addField(const casa::String& fieldName,
             const casa::MDirection& fieldDirection,
             const casa::String& calCode);

  // Add feeds table rows
  void addFeeds(const casa::Int antennaID,
             const casa::Vector<double>& x,
             const casa::Vector<double>& y,
             const casa::Vector<casa::String>& polType);
  
  // Add antenna table row
  casa::Int addAntenna(const casa::String& station,
             const casa::Vector<double>& antXYZ,
             const casa::String& name,
             const casa::String& mount,
             const casa::Double& dishDiameter);

  // Add data description table row
  casa::Int addDataDesc(const casa::Int spwId, const casa::Int polId);
  
  // Add spectral window table row
  casa::Int addSpectralWindow(const casa::String& name,
            const int nChan,
            const casa::Quantity& startFreq,
            const casa::Quantity& freqInc);

  // Add polarisation table row
  casa::Int addPolarisation(const casa::Vector<casa::Stokes::StokesTypes>& stokesTypes);
  

  /// @brief guess the effective LO frequency from the current sky frequency, increment and the number of channels
  /// @details This code is BETA3 specific
  /// @return effective LO frequency in Hz
  double guessEffectiveLOFreq() const;

private:
  /// @brief parameters
  LOFAR::ParameterSet itsParset;
  
  /// @brief data descriptor ID used for all added rows
  casa::uInt itsDataDescID;

  /// @brief field ID used for all added rows
  casa::uInt itsFieldID;

  /// @brief dish pointing centre corresponding to itsFieldID
  casa::MDirection itsDishPointing;
  
  /// @brief true if uvw's are calculated for the centre of each beam (default)
  bool itsBeamOffsetUVW;
  
  /// @brief global (ITRF) coordinates of all antennas
  /// @details row is antenna number, column is X,Y and Z
  casa::Matrix<double> itsAntXYZ;
  
  /// @brief beam offsets in radians
  /// @details assumed the same for all antennas, row is antenna numbers, column is the coordinate
  casa::Matrix<double> itsBeamOffsets;
  
  /// @brief Measurement set
  boost::scoped_ptr<casa::MeasurementSet> itsMs;
    
  /// @brief cached number of channels
  /// @details We don't use it for the real-time correlator, but it is handy for the converter
  int itsNumberOfChannels;
  
  /// @brief number of data descriptor IDs
  /// @details It is equivalent to the number of rows in the appropriate table
  /// (indices go from 0 to itsNumberOfDataDesc)
  int itsNumberOfDataDesc;
  
  /// @brief number of beams defined in the FEED table
  /// @details It is equivalent to the number of rows of the FEED table.
  /// (indices go from 0 to itsNumberOfBeams)
  int itsNumberOfBeams;    

  /// @brief index converter to translate beams into extra antennas
  utility::IndexConverter itsExtraAntennas;

  /// @brief id of the antenna, the beamformer of which gets the extra signals
  /// @note, it should be a non-negative number
  int itsAntHandlingExtras;
  
  /// @brief effective LO frequency in Hz for phase tracking
  /// @note it can be both positive and negative depending on the sidebands used
  double itsEffectiveLOFreq;
  
  /// @brief true, if phase tracking is done
  bool itsTrackPhase;

  /// @brief true, if the LO frequency is derived automatically from the spectral window information (assuming BETA3)
  bool itsAutoLOFreq;

  /// @brief start frequency of the current frequency configuration
  mutable double itsCurrentStartFreq;

  /// @brief frequency increment for the current frequency configuration
  double itsCurrentFreqInc;

  // frequency control via epics - affects spectral window information and phase-tracking
  // the related code is not very general, some BETA3-related specifics are hard-coded

  /// @brief previous value of CONTROL word or -1 if it is not initialised
  mutable int itsPreviousControl;

  /// @brief true, if a change in epics control word passed with the data causes change in frequency
  bool itsControlFreq;

};

} // namespace swcorrelator

} // namespace askap

#endif // #ifndef ASKAP_SWCORRELATOR_FILLER_MS_SINK


