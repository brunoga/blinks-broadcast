#ifndef DEBUG_H_
#define DEBUG_H_

// Debug support for Blinks programs. This allows the addition of debug calls
// whenever needed without having to keep commenting/uncommenting everything.
// When DEBUG_ENABLED is not define, everything compiles out to nothing so it
// was as if it was never there.
//
// Usage: Add LOG calls whenever you want them. This will directly translate to
// calls to ServicePortSerial::print. The ServicePortSerial object is fully
// managed by this code (no need to explicitly create it or to call begin() on
// anything).

// Uncomment the line below to enable debugging.
//#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
#include <Serial.h>

#define LOG(x) debug::Serial()->print(x)
#define LOGLN(x) debug::Serial()->println(x)
#define LOGF(x) debug::Serial()->print(F(x))
#define LOGFLN(x) debug::Serial()->println(F(x))

namespace debug {

ServicePortSerial* Serial();

}  // namespace debug

#else

#define LOG(x)
#define LOGLN(x)
#define LOGF(x)
#define LOGFLN(x)

#endif  // DEBUG_ENABLED

#endif  // DEBUG_H_
