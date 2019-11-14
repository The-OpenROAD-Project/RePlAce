#ifndef __REPLACE_TIMING_STA__
#define __REPLACE_TIMING_STA__ 0

#include "Machine.hh"
#include "Liberty.hh"
#include "StringUtil.hh"
#include "Vector.hh"
#include "Sta.hh"
#include "Sdc.hh"
#include "StaMain.hh"
#include "Stats.hh"
#include "Report.hh"
#include "StringUtil.hh"
#include "PatternMatch.hh"
#include "PortDirection.hh"
#include "FuncExpr.hh"
#include "Units.hh"
#include "MinMax.hh"
#include "Transition.hh"
#include "TimingRole.hh"
#include "TimingArc.hh"
#include "InternalPower.hh"
#include "LeakagePower.hh"
#include "Liberty.hh"
#include "EquivCells.hh"
#include "MinMax.hh"
#include "Network.hh"
#include "Clock.hh"
#include "PortDelay.hh"
#include "ExceptionPath.hh"
#include "Graph.hh"
#include "GraphDelayCalc.hh"
#include "Parasitics.hh"
#include "Wireload.hh"
#include "DelayCalc.hh"
#include "DcalcAnalysisPt.hh"
#include "Corner.hh"
#include "Tag.hh"
#include "PathVertex.hh"
#include "PathRef.hh"
#include "PathEnd.hh"
#include "PathGroup.hh"
#include "CheckTiming.hh"
#include "CheckMinPulseWidths.hh"
#include "Levelize.hh"
#include "Bfs.hh"
#include "Search.hh"
#include "SearchPred.hh"
#include "PathAnalysisPt.hh"
#include "DisallowCopyAssign.hh"
#include "Debug.hh"
#include "Error.hh"
#include "Stats.hh"
#include "PortDirection.hh"
#include "Liberty.hh"
#include "Network.hh"
#include "VerilogNamespace.hh"
#include "VerilogReader.hh"
#include "CheckMinPulseWidths.hh"
#include "CheckMinPeriods.hh"
#include "CheckMaxSkews.hh"
#include "Search.hh"
#include "PathExpanded.hh"
#include "Latches.hh"
#include "Corner.hh"
#include "ReportPath.hh"
#include "VisitPathGroupVertices.hh"
#include "WorstSlack.hh"
#include "ParasiticsClass.hh"
#include "ParseBus.hh"

#ifdef YY_INPUT
#undef YY_INPUT
#endif

// to import Swig
extern "C" {
extern int Sta_Init(Tcl_Interp *interp);
}

// to import TCL functions
namespace sta {
extern const char *tcl_inits[];
}

#endif
