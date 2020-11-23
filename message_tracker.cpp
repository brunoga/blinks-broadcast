#include "message_tracker.h"

// Must be at most one less than the number of different sequence numbers
// (currently 8).
#define MESSAGE_TRACKER_NUM_TRACKED 7

namespace broadcast {

namespace message {

namespace tracker {

#ifdef BROADCAST_ENABLE_EXTERNAL_TRACKER
ExternalTracker external_tracker_;
#endif

static MessageHeader tracked_message_header_[MESSAGE_TRACKER_NUM_TRACKED];
static byte tracked_message_header_index_;

static byte last_sequence_ = 7;

static void track(broadcast::MessageHeader header) {
  tracked_message_header_[tracked_message_header_index_] = header;
  tracked_message_header_index_ =
      (tracked_message_header_index_ + 1) % MESSAGE_TRACKER_NUM_TRACKED;

  last_sequence_ = header.sequence;
}

static bool tracked(broadcast::MessageHeader header) {
  for (byte i = 0; i < MESSAGE_TRACKER_NUM_TRACKED; ++i) {
    if (tracked_message_header_[i].as_byte == header.as_byte) {
      return true;
    }
  }

  return false;
}

void Track(const broadcast::Message* message) {
#ifdef BROADCAST_ENABLE_EXTERNAL_TRACKER
  if (external_tracker_.track != nullptr &&
      external_tracker_.message_id == message->header.id) {
    external_tracker_.track(message);
  } else {
    track(message->header);
  }
#else
  track(message->header);
#endif
}

bool Tracked(const broadcast::Message* message) {
#ifdef BROADCAST_ENABLE_EXTERNAL_TRACKER
  if (external_tracker_.tracked != nullptr &&
      external_tracker_.message_id == message->header.id) {
    return external_tracker_.tracked(message);
  }
#endif
  return tracked(message->header);
}

#ifdef BROADCAST_ENABLE_EXTERNAL_TRACKER
void SetExternalTracker(const ExternalTracker& external_tracker) {
  external_tracker_ = external_tracker;
}
#endif

byte LastSequence() { return last_sequence_; }

}  // namespace tracker

}  // namespace message

}  // namespace broadcast
