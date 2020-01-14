#ifndef __REPLACE_NESTEROV_PLACE__
#define __REPLACE_NESTEROV_PLACE__

namespace replace 
{

class PlacerBase;
class NesterovPlace {
public:
  NesterovPlace();
  NesterovPlace(PlacerBase* placerBase);
  ~NesterovPlace();

  void doNesterovPlace();
private:
  PlacerBase* pb_;
};
}

#endif
