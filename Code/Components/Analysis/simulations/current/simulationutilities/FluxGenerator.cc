/// @file
///
/// Provides mechanism for calculating flux values of a set of spectral channels.
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
#include <askap_simulations.h>

#include <simulationutilities/FluxGenerator.h>
#include <modelcomponents/Spectrum.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <wcslib/wcs.h>
#include <duchamp/Utils/utils.hh>

#include <vector>

ASKAP_LOGGER(logger, ".fluxgen");

namespace askap {

namespace simulations {

FluxGenerator::FluxGenerator()
{
    this->itsNChan = 0;
    this->itsNStokes = 1;
}

FluxGenerator::FluxGenerator(size_t numChan, size_t numStokes)
{
    ASKAPASSERT(numChan >= 0);
    ASKAPASSERT(numStokes >= 1);
    this->itsNChan = numChan;
    this->itsNStokes = numStokes;
    this->itsFluxValues = std::vector< std::vector<float> >(numStokes);
    for (size_t s = 0; s < numStokes; s++) {
        this->itsFluxValues[s] = std::vector<float>(numChan, 0.);
    }
}

FluxGenerator::FluxGenerator(const FluxGenerator& f)
{
    operator=(f);
}

FluxGenerator& FluxGenerator::operator= (const FluxGenerator& f)
{
    if (this == &f) return *this;

    this->itsNChan      = f.itsNChan;
    this->itsNStokes    = f.itsNStokes;
    this->itsFluxValues = f.itsFluxValues;
    return *this;
}

void FluxGenerator::setNumChan(size_t numChan)
{
    ASKAPASSERT(numChan >= 0);
    this->itsNChan = numChan;
    this->itsFluxValues = std::vector< std::vector<float> >(itsNStokes);
    for (size_t s = 0; s < this->itsNStokes; s++) {
        this->itsFluxValues[s] = std::vector<float>(numChan, 0.);
    }
}

void FluxGenerator::setNumStokes(size_t numStokes)
{
    ASKAPASSERT(numStokes >= 1);
    this->itsNStokes = numStokes;
    this->itsFluxValues = std::vector< std::vector<float> >(numStokes);
    if (this->itsNChan > 0) {
        for (size_t s = 0; s < this->itsNStokes; s++) {
            this->itsFluxValues[s] = std::vector<float>(this->itsNChan, 0.);
        }
    }
}

void FluxGenerator::zero()
{
    for (size_t s = 0; s < this->itsNStokes; s++) {
        for (size_t c = 0; c < this->itsNChan; c++) {
            this->itsFluxValues[s][c] = 0.;
        }
    }
}


void FluxGenerator::addSpectrum(boost::shared_ptr<Spectrum> &spec,
                                double &x, double &y, struct wcsprm *wcs)
{
    if (this->itsNChan <= 0)
        ASKAPTHROW(AskapError,
                   "FluxGenerator: Have not set the number of channels in the flux array.");

    double pix[3 * this->itsNChan];
    double wld[3 * this->itsNChan];
    for (size_t z = 0; z < this->itsNChan; z++) {
        pix[3 * z + 0] = x;
        pix[3 * z + 1] = y;
        pix[3 * z + 2] = double(z);
    }

    pixToWCSMulti(wcs, pix, wld, this->itsNChan);

    for (size_t istokes = 0; istokes < this->itsNStokes; istokes++) {
        for (size_t z = 0; z < this->itsNChan; z++) {
            this->itsFluxValues[istokes][z] += spec->flux(wld[3 * z + 2], istokes);
        }
    }

}

void FluxGenerator::addSpectrumInt(boost::shared_ptr<Spectrum> &spec,
                                   double &x, double &y, struct wcsprm *wcs)
{

    if (this->itsNChan <= 0) {
        ASKAPTHROW(AskapError,
                   "FluxGenerator: Have not set the number of channels in the flux array.");
    }

    double pix[3 * this->itsNChan];
    double wld[3 * this->itsNChan];

    for (size_t i = 0; i < this->itsNChan; i++) {
        pix[3 * i + 0] = x;
        pix[3 * i + 1] = y;
        pix[3 * i + 2] = double(i);
    }

    pixToWCSMulti(wcs, pix, wld, this->itsNChan);

    size_t i;
    double df;

    for (size_t istokes = 0; istokes < this->itsNStokes; istokes++) {
        i = 2;
        for (size_t z = 0; z < this->itsNChan; z++) {

            if (z < this->itsNChan - 1) df = fabs(wld[i] - wld[i + 3]);
            else df = fabs(wld[i] - wld[i - 3]);

//     ASKAPLOG_DEBUG_STR(logger,"addSpectrumInt: freq="<<wld[i]<<", df="<<df<<", getting flux between "<<wld[i]-df/2.<<" and " <<wld[i]+df/2.);
            this->itsFluxValues[istokes][z] += spec->fluxInt(wld[i] - df / 2., wld[i] + df / 2.,
                                               istokes);

            i += 3;
        }
    }

}

}


}
