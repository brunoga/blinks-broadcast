#include "message_tracker.h"

// This determines the number of messages that can be in flight without issue.
#define MESSAGE_TRACKER_NUM_TRACKED 4

namespace broadcast {

namespace message {

namespace tracker {

static MessageHeader header_[MESSAGE_TRACKER_NUM_TRACKED];
static byte header_index_;

static byte next_sequence_;

void Track(broadcast::MessageHeader header) {
  header_[header_index_] = header;
  header_index_ = (header_index_ + 1) % MESSAGE_TRACKER_NUM_TRACKED;

  next_sequence_ =
      (header.sequence == MESSAGE_MAX_SEQUENCE) ? 0 : header.sequence + 1;
}

bool Tracked(broadcast::MessageHeader header) {
  for (byte i = 0; i < MESSAGE_TRACKER_NUM_TRACKED; ++i) {
    if (header_[i].as_byte == header.as_byte) {
      return true;
    }
  }

  return false;
}

byte NextSequence() { return next_sequence_; }

}  // namespace tracker

}  // namespace message

}  // namespace broadcast
