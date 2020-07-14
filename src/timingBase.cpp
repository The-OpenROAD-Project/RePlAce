#include "timingBase.h"
#include <algorithm>
#include <cmath>

namespace replace {

// TimingBaseVars
TimingBaseVars::TimingBaseVars() {
  addTimingUpdateIter(79);
  addTimingUpdateIter(64);
  addTimingUpdateIter(49);
  addTimingUpdateIter(29);
  addTimingUpdateIter(21);
  addTimingUpdateIter(15);
}

void
TimingBaseVars::addTimingUpdateIter(int overflow) {
  std::vector<int>::iterator it 
    = std::find(timingUpdateIter_.begin(), 
        timingUpdateIter_.end(), 
        overflow);

  // only push overflow when the overflow is not in vector.
  if( it == timingUpdateIter_.end() ) {
    timingUpdateIter_.push_back(overflow);
  }

  // do sort in reverse order
  std::sort(timingUpdateIter_.begin(), 
      timingUpdateIter_.end(),
      std::greater <int> ());
}

void
TimingBaseVars::deleteTimingUpdateIter(int overflow) {
  std::vector<int>::iterator it 
    = std::find(timingUpdateIter_.begin(), 
        timingUpdateIter_.end(), 
        overflow);
  // only erase overflow when the overflow is in vector.
  if( it != timingUpdateIter_.end() ) {
    timingUpdateIter_.erase(it);
  }
}

void
TimingBaseVars::clearTimingUpdateIter() {
  timingUpdateIter_.clear();
  timingUpdateIter_.shrink_to_fit();
}

void 
TimingBaseVars::reset() {
  clearTimingUpdateIter();
}


// TimingBase
TimingBase::TimingBase() 
  : tbVars_(), db_(nullptr), nb_(nullptr), log_(nullptr) {}

TimingBase::TimingBase(
    TimingBaseVars tbVars,
    odb::dbDatabase* db, 
    std::shared_ptr<NesterovBase> nb, 
    std::shared_ptr<Logger> log)
  : TimingBase() {
    tbVars_ = tbVars;
    db_ = db;
    nb_ = nb;
    log_ = log;

    initTimingIterChk();
  }

void
TimingBase::initTimingIterChk() {
  timingIterChk_.clear();
  timingIterChk_.shrink_to_fit();
  timingIterChk_.resize(tbVars_.timingUpdateIter_.size(), false);
}


void
TimingBase::reset() {
  tbVars_.reset();
  timingIterChk_.clear();
  timingIterChk_.shrink_to_fit();
}


bool
TimingBase::isTimingUpdateIter(float overflow) {
  // exception case handling
  if( tbVars_.timingUpdateIter_.size() == 0 ) {
    return false;
  }

  int intOverflow = std::round(overflow);
  if( intOverflow > tbVars_.timingUpdateIter_[0] ) { 
    return false;
  } 

  bool needTdRun = false;
  for(int i=0; i<tbVars_.timingUpdateIter_.size(); i++) {
    if( tbVars_.timingUpdateIter_[i] > intOverflow ) {
      if( timingIterChk_[i] == false ) {
        timingIterChk_[i] = true;
        needTdRun = true; 
      }
      continue;
    }
    if( needTdRun ) {
      return true;
    }
    else {
      return false;
    }
  }
  return false;
}

// TODO: update corresponding net weights
void
TimingBase::updateNetWeight() {
  // 
}




}
