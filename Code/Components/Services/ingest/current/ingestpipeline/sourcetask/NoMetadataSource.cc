/// @file NoMetadataSource.cc
///
/// @copyright (c) 2013 CSIRO
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
#include "NoMetadataSource.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <string>
#include <stdint.h>
#include <set>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "Common/ParameterSet.h"
#include "cpcommon/VisDatagram.h"
#include "utils/PolConverter.h"
#include "casa/Quanta/MVEpoch.h"
#include "casa/Quanta.h"
#include "casa/Arrays/Matrix.h"
#include "measures/Measures.h"
#include "measures/Measures/MeasFrame.h"
#include "measures/Measures/MCEpoch.h"
#include "measures/Measures/Stokes.h"

// Local package includes
#include "ingestpipeline/sourcetask/IVisSource.h"
#include "ingestpipeline/sourcetask/IMetadataSource.h"
#include "ingestpipeline/sourcetask/ChannelManager.h"
#include "ingestpipeline/sourcetask/InterruptedException.h"
#include "ingestpipeline/sourcetask/MergedSource.h"
#include "configuration/Configuration.h"
#include "configuration/BaselineMap.h"

ASKAP_LOGGER(logger, ".NoMetadataSource");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

NoMetadataSource::NoMetadataSource(const LOFAR::ParameterSet& params,
                                   const Configuration& config,
                                   IVisSource::ShPtr visSrc,
                                   int numTasks, int id) :
        itsConfig(config),
        itsVisSrc(visSrc),
        itsNumTasks(numTasks), itsId(id),
        itsChannelManager(params),
        itsBaselineMap(config.bmap()),
        itsInterrupted(false),
        itsSignals(itsIOService, SIGINT, SIGTERM, SIGUSR1),
        itsMaxNBeams(params.getUint32("maxbeams",0)),
        itsBeamsToReceive(params.getUint32("beams2receive",0)),
        itsCentreFreq(asQuantity(params.getString("centre_freq"))),
        itsTargetName(params.getString("target_name")),
        itsTargetDirection(asMDirection(params.getStringVector("target_direction"))),
        itsLastTimestamp(-1)
{
    // Trigger a dummy frame conversion with casa measures to ensure all caches are setup
    const casa::MVEpoch dummyEpoch(56000.);

    casa::MEpoch::Convert(casa::MEpoch(dummyEpoch, casa::MEpoch::Ref(casa::MEpoch::TAI)),
                          casa::MEpoch::Ref(casa::MEpoch::UTC))();

    parseBeamMap(params);

    itsCorrelatorMode = config.lookupCorrelatorMode(params.getString("correlator_mode"));

    // Setup a signal handler to catch SIGINT, SIGTERM and SIGUSR1
    itsSignals.async_wait(boost::bind(&NoMetadataSource::signalHandler, this, _1, _2));
}

NoMetadataSource::~NoMetadataSource()
{
    itsSignals.cancel();
}

VisChunk::ShPtr NoMetadataSource::next(void)
{
    // Get the next VisDatagram if there isn't already one in the buffer
    while (!itsVis) {
        itsVis = itsVisSrc->next(10000000); // 1 second timeout

        itsIOService.poll();
        if (itsInterrupted) throw InterruptedException();
    }

    // This is the BAT timestamp for the current integration being processed
    const casa::uLong currentTimestamp = itsVis->timestamp;

    // Protect against producing VisChunks with the same timestamp
    ASKAPCHECK(currentTimestamp != itsLastTimestamp,
            "Consecutive VisChunks have the same timestamp");
    itsLastTimestamp = currentTimestamp;

    // Now the streams are synced, start building a VisChunk
    VisChunk::ShPtr chunk = createVisChunk(currentTimestamp);

    // Determine how many VisDatagrams are expected for a single integration
    const casa::uInt nAntenna = itsConfig.antennas().size();
    const casa::uInt nChannels = itsChannelManager.localNChannels(itsId);
    ASKAPCHECK(nChannels % N_CHANNELS_PER_SLICE == 0,
               "Number of channels must be divisible by N_CHANNELS_PER_SLICE");
    const casa::uInt datagramsExpected = itsBaselineMap.size() * itsMaxNBeams * (nChannels / N_CHANNELS_PER_SLICE);
    const casa::uInt interval = itsCorrelatorMode.interval();
    const casa::uInt timeout = interval * 2;

    // Read VisDatagrams and add them to the VisChunk. If itsVisSrc->next()
    // returns a null pointer this indicates the timeout has been reached.
    // In this case assume no more VisDatagrams for this integration will
    // be recieved and move on
    casa::uInt datagramCount = 0;
    casa::uInt datagramsIgnored = 0;
    std::set<DatagramIdentity> receivedDatagrams;
    while (itsVis && currentTimestamp >= itsVis->timestamp) {
        itsIOService.poll();
        if (itsInterrupted) throw InterruptedException();

        if (currentTimestamp > itsVis->timestamp) {
            // If the VisDatagram is from a prior integration then discard it
            ASKAPLOG_WARN_STR(logger, "Received VisDatagram from past integration");
            itsVis = itsVisSrc->next(timeout);
            continue;
        }

        if (addVis(chunk, *itsVis, nAntenna, receivedDatagrams)) {
            ++datagramCount;
        } else {
            ++datagramsIgnored;
        }
        itsVis.reset();

        if (datagramCount == datagramsExpected) {
            // This integration is finished
            break;
        }

        itsVis = itsVisSrc->next(timeout);
    }

    ASKAPLOG_DEBUG_STR(logger, "VisChunk built with " << datagramCount <<
            " of expected " << datagramsExpected << " visibility datagrams");
    ASKAPLOG_DEBUG_STR(logger, "     - ignored " << datagramsIgnored
            << " successfully received datagrams");

    // Submit monitoring data
    itsMonitoringPointManager.submitPoint<int32_t>("PacketsLostCount",
            datagramsExpected - datagramCount);
    if (datagramsExpected != 0) {
        itsMonitoringPointManager.submitPoint<float>("PacketsLostPercent",
            (datagramsExpected - datagramCount) / static_cast<float>(datagramsExpected) * 100.);
    }
    itsMonitoringPointManager.submitMonitoringPoints(*chunk);

    return chunk;
}

VisChunk::ShPtr NoMetadataSource::createVisChunk(const casa::uLong timestamp)
{
    /*
    ASKAPLOG_DEBUG_STR(logger, "received chunk at bat="<<timestamp<<" == 0x"<<std::hex<<timestamp);
    ASKAPLOG_DEBUG_STR(logger, "diff: "<<std::setprecision(9)<<double(timestamp-0x11662f89a887f0)/4976640.<<" cycles ");
    */
    const casa::uInt nAntenna = itsConfig.antennas().size();
    ASKAPCHECK(nAntenna > 0, "Must have at least one antenna defined");
    const casa::uInt nChannels = itsChannelManager.localNChannels(itsId);
    const casa::uInt nPol = itsCorrelatorMode.stokes().size();
    const casa::uInt nBaselines = nAntenna * (nAntenna + 1) / 2;
    const casa::uInt nRow = nBaselines * itsMaxNBeams;
    const casa::uInt period = itsCorrelatorMode.interval(); // in microseconds

    VisChunk::ShPtr chunk(new VisChunk(nRow, nChannels, nPol, nAntenna));

    // Convert the time from integration start in microseconds to an
    // integration mid-point in seconds
    const uint64_t midpointBAT = static_cast<uint64_t>(timestamp + (period / 2ull));
    chunk->time() = bat2epoch(midpointBAT).getValue();
    // Convert the interval from microseconds (long) to seconds (double)
    const casa::Double interval = period / 1000.0 / 1000.0;
    chunk->interval() = interval;

    // All visibilities get flagged as bad, then as the visibility data
    // arrives they are unflagged
    chunk->flag() = true;
    chunk->visibility() = 0.0;

    // For now polarisation data is hardcoded.
    ASKAPCHECK(nPol == 4, "Only supporting 4 polarisation products");

    for (casa::uInt polIndex = 0; polIndex < nPol; ++polIndex) {
        // this way of creating the Stokes vectors ensures the canonical order of polarisation products
        // the last parameter of stokesFromIndex just defines the frame (i.e. linear, circular) and can be
        // any product from the chosen frame. We may want to specify the frame via the parset eventually.
        chunk->stokes()(polIndex) = scimath::PolConverter::stokesFromIndex(polIndex, casa::Stokes::XX);
    }

    // Add the scan index
    chunk->scan() = 0;

    chunk->targetName() = itsTargetName;

    // Determine and add the spectral channel width
    chunk->channelWidth() = itsCorrelatorMode.chanWidth().getValue("Hz");

    // Frequency vector is not of length nRows, but instead nChannels
    chunk->frequency() = itsChannelManager.localFrequencies(itsId,
            itsCentreFreq.getValue("Hz"),
            itsCorrelatorMode.chanWidth().getValue("Hz"),
            itsCorrelatorMode.nChan());

    chunk->directionFrame() = itsTargetDirection.getRef();

    casa::uInt row = 0;
    for (casa::uInt beam = 0; beam < itsMaxNBeams; ++beam) {
        for (casa::uInt ant1 = 0; ant1 < nAntenna; ++ant1) {
            for (casa::uInt ant2 = ant1; ant2 < nAntenna; ++ant2) {
                ASKAPCHECK(row < nRow, "Row index (" << row <<
                           ") should be less than nRow (" << nRow << ")");

                // TODO!!
                // The handling of pointing directions below is not handled per beam.
                // It just takes the field centre direction from the parset and uses
                // that for all beam pointing directions.
                chunk->antenna1()(row) = ant1;
                chunk->antenna2()(row) = ant2;
                chunk->beam1()(row) = beam;
                chunk->beam2()(row) = beam;
                chunk->beam1PA()(row) = 0;
                chunk->beam2PA()(row) = 0;
                chunk->phaseCentre1()(row) = itsTargetDirection.getAngle();
                chunk->phaseCentre2()(row) = itsTargetDirection.getAngle();
                chunk->uvw()(row) = 0.0;

                row++;
            }
        }
    }

    // Populate the per-antenna vectors
    for (casa::uInt i = 0; i < nAntenna; ++i) {
        chunk->targetPointingCentre()[i] = itsTargetDirection;
        chunk->actualPointingCentre()[i] = itsTargetDirection;
        chunk->actualPolAngle()[i] = 0.0;
    }

    return chunk;
}

bool NoMetadataSource::addVis(VisChunk::ShPtr chunk, const VisDatagram& vis,
                              const casa::uInt nAntenna,
                              std::set<DatagramIdentity>& receivedDatagrams)
{
    // 0) Map from baseline to antenna pair and stokes type
    if (itsBaselineMap.idToAntenna1(vis.baselineid) == -1 ||
        itsBaselineMap.idToAntenna2(vis.baselineid) == -1 ||
        itsBaselineMap.idToStokes(vis.baselineid) == casa::Stokes::Undefined) {
            ASKAPLOG_WARN_STR(logger, "Baseline id: " << vis.baselineid
                    << " has no valid mapping to antenna pair and stokes");
        return false;
    }

    const casa::uInt antenna1 = itsBaselineMap.idToAntenna1(vis.baselineid);
    const casa::uInt antenna2 = itsBaselineMap.idToAntenna2(vis.baselineid);
    const casa::Int beamid = itsBeamIDMap(vis.beamid);
    if (beamid < 0) {
        // this beam ID is intentionally unmapped
        return false;
    }
    ASKAPCHECK(beamid < static_cast<casa::Int>(itsMaxNBeams), 
               "Received beam id vis.beamid="<<vis.beamid<<" mapped to beamid="<<beamid<<
               " which is outside the beam index range, itsMaxNBeams="<<itsMaxNBeams);

    // 1) Map from baseline to stokes type and find the  position on the stokes
    // axis of the cube to insert the data into
    const casa::Stokes::StokesTypes stokestype = itsBaselineMap.idToStokes(vis.baselineid);
    // We could use scimath::PolConverter::getIndex here, but the following code allows more checks
    int polidx = -1;
    for (size_t i = 0; i < chunk->stokes().size(); ++i) {
        if (chunk->stokes()(i) == stokestype) {
            polidx = i;
        }
    }
    if (polidx < 0) {
        ASKAPLOG_WARN_STR(logger, "Stokes type " << casa::Stokes::name(stokestype)
                              << " is not configured for storage");
        return false;
    }

    // 2) Check the indexes in the VisDatagram are valid
    ASKAPCHECK(antenna1 < nAntenna, "Antenna 1 index is invalid");
    ASKAPCHECK(antenna2 < nAntenna, "Antenna 2 index is invalid");
    ASKAPCHECK(static_cast<casa::uInt>(beamid) < itsMaxNBeams, "Beam index " << beamid << " is invalid");
    ASKAPCHECK(polidx < 4, "Only 4 polarisation products are supported");
    ASKAPCHECK(vis.slice < 16, "Slice index is invalid");

    // 3) Detect duplicate datagrams
    const DatagramIdentity identity(vis.baselineid, vis.slice, vis.beamid);
    if (receivedDatagrams.find(identity) != receivedDatagrams.end()) {
        ASKAPLOG_WARN_STR(logger, "Duplicate VisDatagram - BaselineID: " << vis.baselineid
                << ", Slice: " << vis.slice << ", Beam: " << vis.beamid);
        return false;
    }
    receivedDatagrams.insert(identity);

    // 4) Find the row for the given beam and baseline
    const casa::uInt row = MergedSource::calculateRow(antenna1, antenna2, beamid, nAntenna);

    const std::string errorMsg = "Indexing failed to find row";
    ASKAPCHECK(chunk->antenna1()(row) == antenna1, errorMsg);
    ASKAPCHECK(chunk->antenna2()(row) == antenna2, errorMsg);
    ASKAPCHECK(chunk->beam1()(row) == static_cast<casa::uInt>(beamid), errorMsg);
    ASKAPCHECK(chunk->beam2()(row) == static_cast<casa::uInt>(beamid), errorMsg);

    // 5) Determine the channel offset and add the visibilities
    const casa::uInt chanOffset = (vis.slice) * N_CHANNELS_PER_SLICE;
    for (casa::uInt chan = 0; chan < N_CHANNELS_PER_SLICE; ++chan) {
        casa::Complex sample(vis.vis[chan].real, vis.vis[chan].imag);
        ASKAPCHECK((chanOffset + chan) <= chunk->nChannel(), "Channel index overflow");

        chunk->visibility()(row, chanOffset + chan, polidx) = sample;

        // Unflag the sample
        chunk->flag()(row, chanOffset + chan, polidx) = false;

        if (antenna1 == antenna2) {
            // For auto-correlations we duplicate cross-pols as index 2 should always be missing
            ASKAPDEBUGASSERT(polidx != 2);

            if (polidx == 1) {
                chunk->visibility()(row, chanOffset + chan, 2) = conj(sample);
                // Unflag the sample
                chunk->flag()(row, chanOffset + chan, 2) = false;
            }
        }
    }
    return true;
}

void NoMetadataSource::signalHandler(const boost::system::error_code& error,
                                     int signalNumber)
{
    if (signalNumber == SIGTERM || signalNumber == SIGINT || signalNumber == SIGUSR1) {
        itsInterrupted = true;
    }
}

void NoMetadataSource::parseBeamMap(const LOFAR::ParameterSet& params)
{                             
    const std::string beamidmap = params.getString("beammap","");
    if (beamidmap != "") {    
        ASKAPLOG_INFO_STR(logger, "Beam indices will be mapped according to <"<<beamidmap<<">");
        itsBeamIDMap.add(beamidmap);
    }   
    const casa::uInt nBeamsInConfig = itsConfig.feed().nFeeds();
    if (itsMaxNBeams == 0) {
        for (int beam = 0; beam < static_cast<int>(nBeamsInConfig) + 1; ++beam) {
             const int processedBeamIndex = itsBeamIDMap(beam);
             if (processedBeamIndex > static_cast<int>(itsMaxNBeams)) {
                 // negative values are automatically excluded by this condition
                 itsMaxNBeams = static_cast<casa::uInt>(processedBeamIndex);
             }
        }
        ++itsMaxNBeams;
    }   
    if (itsBeamsToReceive == 0) {
        itsBeamsToReceive = nBeamsInConfig;
    }        
    ASKAPLOG_INFO_STR(logger, "Number of beams: " << nBeamsInConfig << " (defined in configuration), "
            << itsBeamsToReceive << " (to be received), " << itsMaxNBeams << " (to be written into MS)");
    ASKAPDEBUGASSERT(itsMaxNBeams > 0);
    ASKAPDEBUGASSERT(itsBeamsToReceive > 0);
}
