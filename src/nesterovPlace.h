#ifndef __REPLACE_NESTEROV_PLACE__
#define __REPLACE_NESTEROV_PLACE__

namespace replace
{

class PlacerBase;
class Instance;

class Bin {
public:
  Bin();
  ~Bin();

private:
};

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
