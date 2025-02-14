/*
 *  See header file for a description of this class.
 *
 *  $Date: 2012/03/02 19:47:32 $
 *  $Revision: 1.11 $
 *  \author S. Maselli - INFN Torino
 *          A. Vilela Pereira
 */

#include "DTTTrigCorrection.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "Geometry/Records/interface/MuonGeometryRecord.h"
#include "Geometry/DTGeometry/interface/DTGeometry.h"
#include "Geometry/DTGeometry/interface/DTSuperLayer.h"

#include "CondFormats/DTObjects/interface/DTTtrig.h"
#include "CondFormats/DataRecord/interface/DTTtrigRcd.h"

#include "CalibMuon/DTCalibration/interface/DTCalibDBUtils.h"

#include "CalibMuon/DTCalibration/interface/DTTTrigCorrectionFactory.h"
#include "CalibMuon/DTCalibration/interface/DTTTrigBaseCorrection.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"

#include <iostream>
#include <fstream>

using namespace edm;
using namespace std;

DTTTrigCorrection::DTTTrigCorrection(const ParameterSet& pset)
    : correctionAlgo_{DTTTrigCorrectionFactory::get()->create(pset.getParameter<string>("correctionAlgo"),
                                                              pset.getParameter<ParameterSet>("correctionAlgoConfig"),
                                                              consumesCollector())} {
  ttrigToken_ =
      esConsumes<edm::Transition::BeginRun>(edm::ESInputTag("", pset.getUntrackedParameter<string>("dbLabel")));
  dtGeomToken_ = esConsumes<edm::Transition::BeginRun>();
  LogVerbatim("Calibration") << "[DTTTrigCorrection] Constructor called" << endl;
}

DTTTrigCorrection::~DTTTrigCorrection() {
  LogVerbatim("Calibration") << "[DTTTrigCorrection] Destructor called" << endl;
}

void DTTTrigCorrection::beginRun(const edm::Run& run, const edm::EventSetup& setup) {
  // Get tTrig record from DB
  ESHandle<DTTtrig> tTrig = setup.getHandle(ttrigToken_);
  tTrigMap_ = &*tTrig;
  LogVerbatim("Calibration") << "[DTTTrigCorrection]: TTrig version: " << tTrig->version() << endl;

  // Get geometry from Event Setup
  muonGeom_ = setup.getHandle(dtGeomToken_);

  // Pass EventSetup to correction Algo
  correctionAlgo_->setES(setup);
}

void DTTTrigCorrection::endJob() {
  // Create the object to be written to DB
  DTTtrig* tTrigNewMap = new DTTtrig();

  for (vector<const DTSuperLayer*>::const_iterator sl = muonGeom_->superLayers().begin();
       sl != muonGeom_->superLayers().end();
       ++sl) {
    // Get old value from DB
    float tTrigMean, tTrigSigma, kFactor;
    int status = tTrigMap_->get((*sl)->id(), tTrigMean, tTrigSigma, kFactor, DTTimeUnits::ns);

    //Compute new ttrig
    try {
      dtCalibration::DTTTrigData tTrigCorr = correctionAlgo_->correction((*sl)->id());
      float tTrigMeanNew = tTrigCorr.mean;
      float tTrigSigmaNew = tTrigCorr.sigma;
      float kFactorNew = tTrigCorr.kFactor;
      tTrigNewMap->set((*sl)->id(), tTrigMeanNew, tTrigSigmaNew, kFactorNew, DTTimeUnits::ns);

      LogVerbatim("Calibration") << "New tTrig for: " << (*sl)->id() << " mean from " << tTrigMean << " to "
                                 << tTrigMeanNew << " sigma from " << tTrigSigma << " to " << tTrigSigmaNew
                                 << " kFactor from " << kFactor << " to " << kFactorNew << endl;
    } catch (cms::Exception& e) {
      LogError("Calibration") << e.explainSelf();
      // Set db to the old value, if it was there in the first place
      if (!status) {
        tTrigNewMap->set((*sl)->id(), tTrigMean, tTrigSigma, kFactor, DTTimeUnits::ns);
        LogVerbatim("Calibration") << "Keep old tTrig for: " << (*sl)->id() << " mean " << tTrigMean << " sigma "
                                   << tTrigSigma << " kFactor " << kFactor << endl;
      }
      continue;
    }
  }  //End of loop on superlayers

  //Write object to DB
  LogVerbatim("Calibration") << "[DTTTrigCorrection]: Writing ttrig object to DB!" << endl;
  string record = "DTTtrigRcd";
  DTCalibDBUtils::writeToDB<DTTtrig>(record, tTrigNewMap);
}
