/// @file
/// 
/// @brief Calibration effect: polarisation leakage
/// @details This is a simple effect which can be used in conjunction
/// with the CalibrationME template (as its template argument)
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

#ifndef LEAKAGE_TERM_H
#define LEAKAGE_TERM_H

// own includes
#include <fitting/ComplexDiffMatrix.h>
#include <fitting/ComplexDiff.h>
#include <fitting/Params.h>
#include <dataaccess/IConstDataAccessor.h>
#include <askap/AskapError.h>
#include <measurementequation/ParameterizedMEComponent.h>
#include <utils/PolConverter.h>

// std includes
#include <string>
#include <utility>

namespace askap {

namespace synthesis {

/// @brief Calibration effect: polarisation leakage
/// @details This is a simple effect which can be used in conjunction
/// with the CalibrationME template (as its template argument)
/// @ingroup measurementequation
struct LeakageTerm : public ParameterizedMEComponent {
   
   /// @brief constructor, store reference to paramters
   /// @param[in] par shared pointer to parameters
   inline explicit LeakageTerm(const scimath::Params::ShPtr &par) : 
                              ParameterizedMEComponent(par) {}
   
   /// @brief main method returning Mueller matrix and derivatives
   /// @details This method has to be overloaded (in the template sense) for
   /// all classes representing various calibration effects. CalibrationME
   /// template will call it when necessary. It returns 
   /// @param[in] chunk accessor to work with
   /// @param[in] row row of the chunk to work with
   /// @return ComplexDiffMatrix filled with Mueller matrix corresponding to
   /// this effect
   inline scimath::ComplexDiffMatrix get(const IConstDataAccessor &chunk, 
                                casa::uInt row) const;
protected:
   
   /// @brief obtain a name of the parameter
   /// @details This method produces a parameter name in the form d12.x.y or d21.x.y depending on the
   /// polarisation index (12 or 21), antenna (x) and beam (y) ids.
   /// @param[in] ant antenna number (0-based)
   /// @param[in] beam beam number (0-based)
   /// @param[in] pol12 index of the polarisation product (12 == true or 21 == false)
   /// @return name of the parameter
   static std::string paramName(casa::uInt ant, casa::uInt beam, bool pol12);
   
};

} // namespace synthesis

} // namespace askap

#include <measurementequation/LeakageTerm.tcc>

#endif // #ifndef LEAKAGE_TERM_H
