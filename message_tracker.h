#ifndef MESSAGE_TRACKER_H_
#define MESSAGE_TRACKER_H_

#include "message.h"

namespace broadcast {

namespace message {

namespace tracker {

void Track(broadcast::MessageHeader header);
bool Tracked(broadcast::MessageHeader header);
byte NextSequence();

}  // namespace tracker

}  // namespace message

}  // namespace broadcast

#endif