/// @file : testing ways to access Measurement Sets and related information
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
#include <askap_analysis.h>

#include <analysisutilities/CasaImageUtil.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <casa/aipstype.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <images/Images/ImageOpener.h>
#include <images/Images/FITSImage.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <casa/Arrays/IPosition.h>
#include <casa/Containers/RecordInterface.h>
#include <tables/Tables/Table.h>
#include <tables/Tables/TableDesc.h>
#include <lattices/Lattices/LatticeBase.h>
#include <images/Images/ImageOpener.h>
#include <casa/Utilities/Assert.h>

#include <string>
#include <iostream>

#include <wcslib/wcs.h>

using namespace casa;
using namespace askap;
using namespace askap::analysis;

ASKAP_LOGGER(logger, "tDataAccess.log");

int main(int argc, char *argv[])
{

  try {
    std::string imageName;
    if(argc==1) imageName = "$ASKAP_ROOT/Code/Components/Synthesis/testdata/trunk/simulation/stdtest/image.i.10uJy_clean_stdtest";
    else imageName = argv[1];

    std::cout << "Loading " << imageName << " using casa::LatticeBase\n";
    ImageOpener::registerOpenImageFunction(ImageOpener::FITS, FITSImage::openFITSImage);
    LatticeBase* lattPtr = ImageOpener::openImage (imageName);
    ASKAPASSERT (lattPtr);      // to be sure the image file could be opened
    ImageInterface<Float>* imagePtr = dynamic_cast<ImageInterface<Float>*>(lattPtr);
    //   ASSERT (imagePtr);     // to be sure its data type is Float
    CoordinateSystem coords2=imagePtr->coordinates();
    Record hdr2;
    IPosition shape2 = imagePtr->shape();
    Bool worked2 = coords2.toFITSHeader(hdr2,shape2,true,'c',true);
    if(worked2) std::cout << "Success!\n";

    std::cout << hdr2<< "\n"
	      << "beam = " << imagePtr->imageInfo().restoringBeam() << "\n";
    Vector<Quantum<Double> > beam = imagePtr->imageInfo().restoringBeam();
    if(beam.size()>0)
      std::cout << beam[0].getValue("deg") << "\n" << beam[1].getValue("deg") << "\n" << beam[2].getValue("deg") << "\n";

    std::cout << "Loading " << imageName << " using askap::analysis::casaImageToWCS()\n";
    wcsprm *wcs = casaImageToWCS(imageName);
    std::cout << "Success! wcsprt gives:\n";
    wcsprt(wcs);

    std::cout << "Success!\n";

  }
  catch (askap::AskapError& x)
    {
      ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << x.what());
      std::cerr << "Askap error in " << argv[0] << ": " << x.what() << std::endl;
      exit(1);
    }
  catch (std::exception& x)
    {
      ASKAPLOG_FATAL_STR(logger, "Unexpected exception in " << argv[0] << ": " << x.what());
      std::cerr << "Unexpected exception in " << argv[0] << ": " << x.what() << std::endl;
      exit(1);
    }
  exit(0);
}
