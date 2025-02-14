import FWCore.ParameterSet.Config as cms
from DQM.SiPixelPhase1Heterogeneous.siPixelPhase1MonitorClusterSoA_cfi import *
from DQM.SiPixelPhase1Heterogeneous.siPixelPhase1MonitorDigiSoA_cfi import *
from DQM.SiPixelPhase1Heterogeneous.siPixelPhase1MonitorTrackSoA_cfi import *
from DQM.SiPixelPhase1Heterogeneous.siPixelPhase1MonitorVertexSoA_cfi import *

monitorpixelSoASource = cms.Sequence(siPixelPhase1MonitorDigiSoA * siPixelPhase1MonitorClusterSoA * siPixelPhase1MonitorTrackSoA * siPixelPhase1MonitorVertexSoA)
