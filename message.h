#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <blinklib.h>

#if __has_include(<broadcast_config.h>)
#include <broadcast_config.h>
#else
// Default message payload size is the maximum size.
#define BROADCAST_MESSAGE_PAYLOAD_BYTES 15
#endif

// This should not be changed.
#define BROADCAST_MESSAGE_HEADER_BYTES 1

#define BROADCAST_MESSAGE_DATA_BYTES \
  BROADCAST_MESSAGE_PAYLOAD_BYTES + BROADCAST_MESSAGE_HEADER_BYTES

// Make sure we do not try to use more data than we can.
#if BROADCAST_MESSAGE_DATA_BYTES > IR_DATAGRAM_LEN
#error MESSAGE_DATA_BYTES must not be greater than DATAGRAM_BYTES.
#endif

// Message id 0 is used for fire and forget reset messages.
#define MESSAGE_RESET 0

#define MESSAGE_MAX_SEQUENCE 15

namespace broadcast {

#ifdef BROADCAST_DISABLE_REPLIES
union MessageHeader {
  struct {
    byte id : 4;
    byte sequence : 4;
  };

  byte as_byte;
};
#else
union MessageHeader {
  struct {
    byte id : 3;
    byte sequence : 3;
    bool is_reply : 1;
    bool is_fire_and_forget : 1;
  };

  byte as_byte;
};
#endif

struct Message {
  MessageHeader header;

  byte payload[BROADCAST_MESSAGE_PAYLOAD_BYTES];
};

namespace message {

#ifdef BROADCAST_DISABLE_REPLIES
void Initialize(Message* message, byte id);
#else
void Initialize(Message* message, byte id, bool is_fire_and_forget);
#endif

void ClearPayload(Message* message);

}  // namespace message

}  // namespace broadcast

#endif  // MESSAGE_H_
