#include "message_tracker.h"

// Must be at most one less than the maximum sequence number (currently 7).
#define MESSAGE_TRACKER_NUM_TRACKED 6

namespace broadcast {

namespace message {

namespace tracker {

static MessageHeader tracked_message_header_[MESSAGE_TRACKER_NUM_TRACKED];
static byte tracked_message_header_index_;

static byte last_sequence_;

void Track(broadcast::MessageHeader header) {
  tracked_message_header_[tracked_message_header_index_] = header;
  tracked_message_header_index_ =
      (tracked_message_header_index_ + 1) % MESSAGE_TRACKER_NUM_TRACKED;

  last_sequence_ = header.sequence;
}

bool __attribute__((noinline)) Tracked(broadcast::MessageHeader header) {
  for (byte i = 0; i < MESSAGE_TRACKER_NUM_TRACKED; ++i) {
    if (tracked_message_header_[i].as_byte == header.as_byte) {
      return true;
    }
  }

  return false;
}

byte LastSequence() { return last_sequence_; }

}  // namespace tracker

}  // namespace message

}  // namespace broadcast
