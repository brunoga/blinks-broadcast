#include "message_tracker.h"

#define MESSAGE_TRACKER_NUM_FIRE_AND_FORGET 3

namespace broadcast {

namespace message {

namespace tracker {

static MessageHeader tracked_message_header_;

static MessageHeader
    tracked_fire_and_forget_header_[MESSAGE_TRACKER_NUM_FIRE_AND_FORGET];
static byte tracked_fire_and_forget_header_index_;

static byte last_sequence_;

void Track(broadcast::MessageHeader header) {
  if (header.is_fire_and_forget) {
    tracked_fire_and_forget_header_[tracked_fire_and_forget_header_index_] =
        header;
    tracked_fire_and_forget_header_index_ =
        (tracked_fire_and_forget_header_index_ + 1) %
        MESSAGE_TRACKER_NUM_FIRE_AND_FORGET;
  } else {
    tracked_message_header_ = header;
  }

  last_sequence_ = header.sequence;
}

bool Tracked(broadcast::MessageHeader header) {
  if (header.is_fire_and_forget) {
    for (byte i = 0; i < MESSAGE_TRACKER_NUM_FIRE_AND_FORGET; ++i) {
      if (tracked_fire_and_forget_header_[i].value == header.value) {
        return true;
      }
    }

    return false;
  }

  return tracked_message_header_.value == header.value;
}

byte LastSequence() { return last_sequence_; }

}  // namespace tracker

}  // namespace message

}  // namespace broadcast
