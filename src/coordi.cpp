#include "coordi.h"

namespace replace {

FloatCoordi::FloatCoordi() : x(0), y(0) {}
FloatCoordi::FloatCoordi(float inputX, float inputY) {
  x = inputX; 
  y = inputY;
}

IntCoordi::IntCoordi() : x(0), y(0) {}
IntCoordi::IntCoordi(int inputX, int inputY) {
  x = inputX;
  y = inputY;
}
}

