/// @file UVChannelConstDataSource.cc
/// @brief Implementation of IConstDataSource
///
/// @copyright (c) 2011 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "UVChannelConstDataSource.h"

// System includes
#include <string>

// ASPAKsoft includes
#include "boost/shared_ptr.hpp"
#include "askap/AskapError.h"
#include "Common/ParameterSet.h"
#include "dataaccess/BasicDataConverter.h"
#include "dataaccess/DataAccessError.h"
#include "dataaccess/IConstDataSource.h"
#include "dataaccess/IConstDataIterator.h"

// Local package includes
#include "uvchannel/uvdataaccess/UVChannelDataConverter.h"
#include "uvchannel/uvdataaccess/UVChannelDataSelector.h"
#include "uvchannel/uvdataaccess/UVChannelConstDataIterator.h"

// Using
using namespace askap;
using namespace askap::accessors;
using namespace askap::cp::channels;

UVChannelConstDataSource::UVChannelConstDataSource(const LOFAR::ParameterSet& parset,
        const std::string& channelName)
        : itsChannelConfig(parset), itsChannelName(channelName)
{
}

IDataConverterPtr UVChannelConstDataSource::createConverter() const
{
    return IDataConverterPtr(new UVChannelDataConverter());
}

IDataSelectorPtr UVChannelConstDataSource::createSelector() const
{
    return IDataSelectorPtr(new UVChannelDataSelector());
}

boost::shared_ptr<IConstDataIterator>
UVChannelConstDataSource::createConstIterator(
    const IDataSelectorConstPtr &sel,
    const IDataConverterConstPtr &conv) const
{
    // cast input selector to "implementation" interface
    boost::shared_ptr<UVChannelDataSelector const> implSel =
        boost::dynamic_pointer_cast<UVChannelDataSelector const>(sel);
    boost::shared_ptr<UVChannelDataConverter const> implConv =
        boost::dynamic_pointer_cast<UVChannelDataConverter const>(conv);

    if (!implSel || !implConv) {
        ASKAPTHROW(DataAccessLogicError, "Incompatible selector and/or " <<
                   "converter was passed to the createConstIterator method");
    }

    return boost::shared_ptr<IConstDataIterator>(new UVChannelConstDataIterator(
                itsChannelConfig, itsChannelName, implSel, implConv));
}