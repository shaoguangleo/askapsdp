/// @file MetadataSource.h
///
/// @copyright (c) 2010 CSIRO
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

#ifndef ASKAP_CP_METADATASOURCE_H
#define ASKAP_CP_METADATASOURCE_H

// ASKAPsoft includes
#include "boost/shared_ptr.hpp"
#include "tosmetadata/MetadataReceiver.h"

// CP Ice interfaces
#include "CommonTypes.h"
#include "TypedValues.h"

// Local package includes
#include "ingestpipeline/sourcetask/IMetadataSource.h"
#include "ingestpipeline/sourcetask/CircularBuffer.h"

namespace askap {
    namespace cp {

        class MetadataSource :
            virtual public askap::cp::MetadataReceiver, public askap::cp::IMetadataSource
        {
            public:
                MetadataSource(const std::string& locatorHost,
                        const std::string& locatorPort,
                        const std::string& topicManager,
                        const std::string& topic,
                        const std::string& adapterName,
                        const unsigned int bufSize);
                ~MetadataSource();

                virtual void receive(const askap::interfaces::TimeTaggedTypedValueMap& msg);

                // Blocking
                boost::shared_ptr<askap::interfaces::TimeTaggedTypedValueMap> next(void);

            private:
                // Circular buffer of metadata payloads
                askap::cp::CircularBuffer< askap::interfaces::TimeTaggedTypedValueMap > itsBuffer;
        };

    };
};

#endif
