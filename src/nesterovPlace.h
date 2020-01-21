#ifndef __REPLACE_NESTEROV_PLACE__
#define __REPLACE_NESTEROV_PLACE__

namespace replace
{

class PlacerBase;
class Instance;
class NesterovBase;

class NesterovPlace {
public:
  NesterovPlace();
  NesterovPlace(PlacerBase* placerBase, NesterovBase* nesterovBase);
  ~NesterovPlace();

  void doNesterovPlace();

private:
  PlacerBase* pb_;
  NesterovBase* nb_;
  
  void init();
  void reset();

};
}

#endif
