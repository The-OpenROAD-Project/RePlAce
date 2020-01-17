#ifndef __REPLACE_NESTEROV_PLACE__
#define __REPLACE_NESTEROV_PLACE__

namespace replace
{

class PlacerBase;
class Instance;

class NesterovPlace {
public:
  NesterovPlace();
  NesterovPlace(PlacerBase* placerBase);
  ~NesterovPlace();

  void init();
  void doNesterovPlace();

private:
  PlacerBase* pb_;

};
}

#endif
