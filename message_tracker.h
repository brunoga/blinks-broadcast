#ifndef MESSAGE_TRACKER_H_
#define MESSAGE_TRACKER_H_

#include "message.h"

namespace broadcast {

namespace message {

namespace tracker {

void Track(broadcast::MessageHeader header);
void Track(const broadcast::Message* message);

bool Tracked(broadcast::MessageHeader header);
bool Tracked(const broadcast::Message* message);

byte LastSequence();

}  // namespace tracker

}  // namespace message

}  // namespace broadcast

#endif