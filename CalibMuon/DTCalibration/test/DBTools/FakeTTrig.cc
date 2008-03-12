 /*
 *  See header file for a description of this class.
 *
 *  $Date: 2008/01/18 17:48:39 $
 *  $Revision: 1.3 $
 *  \author G. Mila - INFN Torino
 */


#include "CalibMuon/DTCalibration/test/DBTools/FakeTTrig.h"

// Framework
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "CalibMuon/DTCalibration/test/DBTools/DTCalibrationMap.h"
#include "CalibMuon/DTCalibration/interface/DTCalibDBUtils.h"

// Geometry
#include "Geometry/Records/interface/MuonGeometryRecord.h"
#include "Geometry/DTGeometry/interface/DTGeometry.h"
#include "Geometry/DTGeometry/interface/DTSuperLayer.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "Geometry/DTGeometry/interface/DTTopology.h"
#include "CondFormats/DTObjects/interface/DTTtrig.h"

//Random generator
#include "FWCore/Utilities/interface/RandomNumberGenerator.h"
#include <CLHEP/Random/RandGaussQ.h>

// DTDigitizer
#include "CalibMuon/DTDigiSync/interface/DTTTrigSyncFactory.h"
#include "CalibMuon/DTDigiSync/interface/DTTTrigBaseSync.h"


using namespace std;
using namespace edm;



FakeTTrig::FakeTTrig(const ParameterSet& pset) {
 
  cout << "[FakeTTrig] Constructor called! " << endl;

  // further configurable smearing
  smearing=pset.getUntrackedParameter<double>("smearing");

  // get random engine
  edm::Service<edm::RandomNumberGenerator> rng;
  if ( ! rng.isAvailable()) {
    throw cms::Exception("Configuration")
      << "RandomNumberGeneratorService for DTFakeTTrigDB missing in cfg file";
  }
  theGaussianDistribution = new CLHEP::RandGaussQ(rng->getEngine()); 

  ps = pset;
  
}


FakeTTrig::~FakeTTrig(){
  cout << "[FakeTTrig] Destructor called! " << endl;
}


void FakeTTrig::beginJob(const edm::EventSetup& context){

  cout << "[FakeTTrig] entered into beginJob! " << endl;
  context.get<MuonGeometryRecord>().get(muonGeom);

}


void FakeTTrig::endJob() {
 
  cout << "[FakeTTrig] entered into endJob! " << endl;
 
  // Get the superlayers and layers list
  vector<DTSuperLayer*> dtSupLylist = muonGeom->superLayers();
  // Create the object to be written to DB
  DTTtrig* tTrigMap = new DTTtrig();

  for (vector<DTSuperLayer*>::const_iterator sl = dtSupLylist.begin();
       sl != dtSupLylist.end(); sl++) {

    // get the time of fly
    double timeOfFly = tofComputation(*sl);
    // get the time of wire propagation
    double timeOfWirePropagation = wirePropComputation(*sl);
    // get the gaussian smearing
    double gaussianSmearing = theGaussianDistribution->fire(0.,smearing);

    DTSuperLayerId slId = (*sl)->id();
    double fakeTTrig = 500 + timeOfFly + timeOfWirePropagation + gaussianSmearing;
    tTrigMap->set(slId, fakeTTrig, 0, DTTimeUnits::ns);

  }

  // Write the object in the DB
  cout << "[FakeTTrig] Writing ttrig object to DB!" << endl;
  string record = "DTTtrigRcd";
  DTCalibDBUtils::writeToDB<DTTtrig>(record, tTrigMap);
  
}

				      



double FakeTTrig::tofComputation(const DTSuperLayer* superlayer) {

  double tof=0;
  const double cSpeed = 29.9792458; // cm/ns

  if(ps.getUntrackedParameter<bool>("useTofCorrection", true)){
    LocalPoint localPos(0,0,0);
    double flight = superlayer->surface().toGlobal(localPos).mag();
    tof = flight/cSpeed;
  }

  return tof;

}



double FakeTTrig::wirePropComputation(const DTSuperLayer* superlayer) {

  double delay = 0;
  double theVPropWire = ps.getUntrackedParameter<double>("vPropWire", 24.4); // cm/ns

  if(ps.getUntrackedParameter<bool>("useWirePropCorrection", true)){
    DTLayerId lId = DTLayerId(superlayer->id(), 1);
    float halfL  =  superlayer->layer(lId)->specificTopology().cellLenght()/2;
    delay = halfL/theVPropWire;
  }

  return delay;

}

