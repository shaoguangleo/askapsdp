/// @file
///
/// Provides base class for handling the creation of FITS files
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>
///
#ifndef ASKAP_SIMS_FITS_H_
#define ASKAP_SIMS_FITS_H_

#include <askap_simulations.h>

#include <wcslib/wcs.h>

#include <Common/ParameterSet.h>
#include <casa/aipstype.h>
#include <casa/Quanta/Unit.h>
#include <casa/Arrays/Slice.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <images/Images/ImageInfo.h>


#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <duchamp/Utils/Section.hh>

#include <modelcomponents/Spectrum.h>
#include <modelcomponents/BeamCorrector.h>
#include <modelcomponents/ModelFactory.h>

#include <vector>
#include <utility>
#include <string>
#include <math.h>

using namespace askap::analysisutilities;

namespace askap {

namespace simulations {

namespace FITS {

/// @brief Convert the name of a FITS file to the name for the
/// equivalent casa image
/// @details Takes the name of a fits file and produces
/// the equivalent CASA image name. This simply involves
/// removing the ".fits" extension if it exists, or, if it
/// doesn't, adding a ".casa" extension.
/// @param fitsName The name of the fits file
/// @return The name of the casa image.
std::string casafy(std::string fitsName);

/// @details A utility function to combine a keyword and a
/// value, to produce a relevant FITS keyword for a given
/// axis. For example numerateKeyword(CRPIX,1) returns CRPIX1.
char *numerateKeyword(std::string key, int num);

/// @brief A class to create new FITS files
/// @details This class handles the creation of FITS files, as
/// well as WCS handling, adding point or Gaussian components, adding
/// noise, and convolving with a beam. It is driven by
/// parameterset input.
class FITSfile {
    public:
        /// Default constructor does not allocate anything
        FITSfile();

        /// Default destructor, frees the WCS structs
        virtual ~FITSfile();

        /// @brief Constructor, using an input parameter set
        /// @details Constructor that reads in the necessary
        /// definitions from the parameterset. All FITSfile members
        /// are read in. The conversion factors for the source fluxes
        /// are also defined using the WCSLIB wcsunits function (using
        /// the sourceFluxUnits parameter: if this is not specified,
        /// the fluxes are assumed to be the same units as those of
        /// BUNIT). The pixel array is allocated here.
        FITSfile(const LOFAR::ParameterSet& parset, bool allocateMemory = true);

        /// @brief Copy constructor
        FITSfile(const FITSfile& f);

        /// @brief Copy operator
        FITSfile& operator=(const FITSfile& f);

        /// @brief Define the world coordinate system
        /// @details Defines a world coordinate system from an
        /// input parameter set. This looks for parameters that
        /// define the various FITS header keywords for each
        /// axis (ctype, cunit, crval, cdelt, crpix, crota), as
        /// well as the equinox, then defines a WCSLIB wcsprm
        /// structure and assigns it to either FITSfile::itsWCS
        /// or FITSfile::itsWCSsources depending on the isImage
        /// parameter.
        /// @param isImage If true, the FITSfile::itsWCS
        /// structure is defined, else it is the
        /// FITSfile::itsWCSsources.
        /// @param parset The input parset to be examined.
        void setWCS(bool isImage, const LOFAR::ParameterSet& parset);

        /// @brief Return the WCS structure
        struct wcsprm *getWCS() {return itsWCS;};

        /// @brief Get and set individual values in the flux array
        /// @{
        float array(int pos) {return itsArray[pos];};
        float array(unsigned int x, unsigned int y)
        {
            size_t pos = x + itsAxes[0] * y; return itsArray[pos];
        };
        float array(unsigned int x, unsigned int y, unsigned int z)
        {
            size_t pos = x + itsAxes[0] * y + z * itsAxes[0] * itsAxes[1]; return itsArray[pos];
        };
        void setArray(unsigned int pos, float val)
        {
            itsArray[pos] = val;
        };
        void setArray(unsigned int x, unsigned int y, float val)
        {
            size_t pos = x + itsAxes[0] * y; itsArray[pos] = val;
        };
        void setArray(unsigned int x, unsigned int y, unsigned int z, float val)
        {
            size_t pos = x + itsAxes[0] * y + z * itsAxes[0] * itsAxes[1]; itsArray[pos] = val;
        };
        /// @}
        /// @brief Get the vector of axis dimensions
        std::vector<unsigned int> getAxes() {return itsAxes;};
        /// @brief Get the size of the X-axis
        unsigned int getXdim() {return itsAxes[itsWCS->lng];};
        /// @brief Get the size of the Y-axis
        unsigned int getYdim() {return itsAxes[itsWCS->lat];};
        /// @brief Get the size of the Z-axis
        unsigned int getZdim() {return itsAxes[itsWCS->spec];};
        /// @brief Get the index of the spectral axis
        int getSpectralAxisIndex() {return itsWCS->spec;};
        /// @brief Return the number of pixels
        size_t getSize() {return itsNumPix;};
        /// @brief Get the size of the Stokes axis
        int getNumStokes();
        /// @brief Get the size of the spectral axis
        size_t getNumChan();

        /// @brief Is the requested database a spectral-line one?
        bool databaseSpectral();

        /// @brief Make a flux array with just noise in it.
        /// @details Fills the pixel array with fluxes sampled from a
        /// normal distribution ~ N(0,itsNoiseRMS) (i.e. the mean of
        /// the distribution is zero). Note that this overwrites the array.
        void makeNoiseArray();

        /// @brief Add noise to the flux array
        /// @details Adds noise to the array. Noise values are
        /// distributed as N(0,itsNoiseRMS) (i.e. with mean zero).
        void addNoise();

        /// @brief Add sources to the flux array
        /// @details Adds sources to the array. If the source list
        /// file has been defined, it is read one line at a time, and
        /// each source is added to the array. If it is a point source
        /// (i.e. major_axis = 0) then its flux is added to the
        /// relevant pixel, assuming it lies within the boundaries of
        /// the array. If it is a Gaussian source (major_axis>0), then
        /// the function addGaussian is used. The WCSLIB functions are
        /// used to convert the ra/dec positions to pixel positions.
        void processSources();
        void processSource(std::string line) {};
        casa::Slicer getFootprint(std::string line) {return casa::Slicer();};

        /// @brief Convolve the flux array with a beam
        /// @brief The array is convolved with the Gaussian beam
        /// specified in itsBeamInfo. The GaussSmooth2D class from the
        /// Duchamp library is used. Note that this is only done if
        /// itsHaveBeam is set true.
        void convolveWithBeam();

        /// @brief Save the array to a FITS file
        /// @details Creates a FITS file with the appropriate headers
        /// and saves the flux array into it. Uses the CFITSIO library
        /// to do so.
        void writeFITSimage(bool creatFile = true, bool saveData = true, bool useOffset = true);

        /// @brief Save the array to a CASA image
        /// @details Writes the data to a casa image. The WCS is
        /// converted to a casa-format coordinate system using
        /// the analysis package function, the brightness units
        /// and restoring beam are saved to the image, and the
        /// data array is written using a casa::Array class. No
        /// additional memory allocation is done in saving the
        /// data array (the casa::SHARE flag is used in the
        /// array constructor). The name of the casa image is
        /// determined by the casafy() function.
        void writeCASAimage(bool creatFile = true, bool saveData = true, bool useOffset = true);

        double maxFreq();
        double minFreq();

        bool createTaylorTerms() {return itsCreateTaylorTerms;};
        void createTaylorTermImages(std::string nameBase,
                                    casa::CoordinateSystem csys,
                                    casa::IPosition shape,
                                    casa::IPosition tileshape,
                                    casa::Unit bunit,
                                    casa::ImageInfo iinfo);
        void defineTaylorTerms();
        void writeTaylorTermImages(std::string nameBase, casa::IPosition location);


    protected:

        /// @brief The name of the file to be written to
        std::string itsFileName;
        /// @brief Whether to write to a FITS-format image
        bool itsFITSOutput;
        /// @brief Whether to write to a CASA-format image
        bool itsCasaOutput;
        /// @brief Whether to write the CASA image channel-by-channel
        bool itsFlagWriteByChannel;
        /// @brief Whether to write the full cube (in addition to Taylor terms)
        bool itsWriteFullImage;
        /// @brief Whether to write Taylor term images matching the spectral cube
        bool itsCreateTaylorTerms;
        /// @brief The maximum Taylor term to be created
        unsigned int itsMaxTaylorTerm;
        /// @brief What percentage of the spectral fitting to log when doing Taylor terms?
        int itsTTlogevery;
        /// @brief The file containing the list of sources
        std::string itsSourceList;
        /// @brief The type of input list: either "continuum" or "spectralline"
        std::string itsSourceListType;
        /// @brief How often to record progress when adding sources
        int itsSourceLogevery;
        /// @brief The origin of the database: either "S3SEX" or
        /// "S3SAX" - used for spectralline case
        std::string itsDatabaseOrigin;
        /// @brief Should disc components be replaced with Gaussian components?
        bool itsUseGaussians;
        /// @brief Should we be verbose about information about sources?
        bool itsFlagVerboseSources;
        /// @brief The factory class used to generate model components.
        ModelFactory itsModelFactory;
        /// @brief The format of the source positions: "deg"=decimal degrees; "dms"= dd:mm:ss
        std::string itsPosType;
        /// @brief The minimum value for the minor axis for the
        /// sources in the catalogue. Only used when major axis > 0,
        /// to prevent infinite axial ratios
        float itsMinMinorAxis;
        /// @brief The units of the position angle for the sources in
        /// the catalogue: either "rad" or "deg"
        casa::Unit itsPAunits;
        /// @brief The flux units for the sources in the catalogue
        casa::Unit itsSourceFluxUnits;
        /// @brief The units of the major & minor axes for the sources in the catalogue
        casa::Unit itsAxisUnits;
        /// @brief Whether to integrate gaussians over pixels to find the flux in a pixel
        bool itsFlagIntegrateGaussians;

        /// @brief The array of pixel fluxes
        std::vector<float> itsArray;
        /// @brief The arrays holding the Taylor term maps
        std::vector<casa::Array<float> > itsTTmaps;
        /// @brief The RMS of the noise distribution
        float itsNoiseRMS;

        /// @brief The dimensionality of the image
        unsigned int itsDim;
        /// @brief The axis dimensions
        std::vector<unsigned int> itsAxes;
        /// @brief The number of pixels in the image
        size_t itsNumPix;
        /// @brief The section of the image in which to
        /// place sources - defaults to the null section
        /// of the appropriate dimensionality, and needs
        /// to be set explicitly via setSection()
        duchamp::Section itsSourceSection;

        /// @brief Do we have information on the beam size?
        bool itsHaveBeam;
        /// @brief The beam specifications: major axis, minor axis, position angle
        std::vector<float> itsBeamInfo;
        /// @brief How we correct source fluxes for the beam
        BeamCorrector itsBeamCorrector;

        /// @brief The base frequency (used only for Continuum sources)
        float itsBaseFreq;
        /// @brief The rest frequency for emission-line sources:
        /// stored as RESTFREQ in the FITS header
        float itsRestFreq;

        /// @brief Whether sources should be added
        bool itsAddSources;
        /// @brief Whether to just count the sources that would be added rather than add them
        bool itsDryRun;

        /// @brief The EQUINOX keyword
        float itsEquinox;
        /// @brief The BUNIT keyword: units of flux
        casa::Unit itsBunit;

        /// @brief How to convert source fluxes to the correct units
        /// for the image
        ///@{
        double itsUnitScl;
        double itsUnitOff;
        double itsUnitPwr;
        /// @}

        /// @brief The world coordinate information
        struct wcsprm *itsWCS;
        /// @brief Has the memory for the image's WCS been allocated?
        bool itsWCSAllocated;

        /// @brief The world coordinate information that the sources
        /// use, if different from itsWCS
        struct wcsprm *itsWCSsources;
        /// @brief Has the memory for the sources' WCS been allocated?
        bool itsWCSsourcesAllocated;
        /// @brief If the sources have a different WCS defined, and we
        /// need to transform to the image WCS.
        bool itsFlagPrecess;
        /// @brief Whether to save the source list with new positions
        bool itsFlagOutputList;
        /// @brief Whether to save the source list with new positions
        /// for only the sources in the image
        bool itsFlagOutputListGoodOnly;
        /// @brief The file to save the new source list to.
        std::string itsOutputSourceList;

};

}

}
}

#endif
