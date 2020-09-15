#include "debug.h"

#ifdef DEBUG_ENABLED

namespace debug {

static bool started_ = false;
static ServicePortSerial sp_;

ServicePortSerial* Serial() {
  if (!started_) {
    sp_.begin();
    started_ = true;
  }

  return &sp_;
}

}  // namespace debug

#endif