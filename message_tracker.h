#ifndef MESSAGE_TRACKER_H_
#define MESSAGE_TRACKER_H_

#include "message.h"

#if __has_include(<broadcast_config.h>)
#include <broadcast_config.h>
#endif

namespace broadcast {

namespace message {

namespace tracker {

#ifdef BROADCAST_ENABLE_EXTERNAL_TRACKER
struct ExternalTracker {
  byte message_id;

  void (*track)(const broadcast::Message*);
  bool (*tracked)(const broadcast::Message*);
};
#endif

void Track(const broadcast::Message* message);
bool Tracked(const broadcast::Message* message);

#ifdef BROADCAST_ENABLE_EXTERNAL_TRACKER
void SetExternalTracker(const ExternalTracker& external_tracker);
#endif

byte LastSequence();

}  // namespace tracker

}  // namespace message

}  // namespace broadcast

#endif