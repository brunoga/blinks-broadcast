#ifndef MESSAGE_TRACKER_H_
#define MESSAGE_TRACKER_H_

#include "message.h"

namespace broadcast {

namespace message {

namespace tracker {

struct ExternalTracker {
  byte message_id;

  void (*track)(const broadcast::Message*);
  bool (*tracked)(const broadcast::Message*);
};

void Track(broadcast::MessageHeader header);
void Track(const broadcast::Message* message);

bool Tracked(broadcast::MessageHeader header);
bool Tracked(const broadcast::Message* message);

void SetExternalTracker(const ExternalTracker& external_tracker);

byte LastSequence();

}  // namespace tracker

}  // namespace message

}  // namespace broadcast

#endif