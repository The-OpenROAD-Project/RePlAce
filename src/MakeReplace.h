
#ifndef MAKE_REPLACE
#define MAKE_REPLACE

namespace ord {

class OpenRoad;

// All of replace's state is global, so there is nothing to make;
void *
makeReplace();

void
initReplace(OpenRoad *openroad);

void
deleteReplace(void *replace);

}

#endif
