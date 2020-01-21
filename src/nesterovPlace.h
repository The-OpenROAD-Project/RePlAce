#ifndef __REPLACE_NESTEROV_PLACE__
#define __REPLACE_NESTEROV_PLACE__

#include <memory>

namespace replace
{

class PlacerBase;
class Instance;
class NesterovBase;

class NesterovPlace {
public:
  NesterovPlace();
  NesterovPlace(std::shared_ptr<PlacerBase> pb,
      std::shared_ptr<NesterovBase> nb);
  ~NesterovPlace();

  void doNesterovPlace();

private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;

  void init();
  void reset();

};
}

#endif
