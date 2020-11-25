#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <blinklib.h>

#if __has_include(<broadcast_config.h>)
#include <broadcast_config.h>
#else
// Default message payload size is the maximum size.
#define MESSAGE_PAYLOAD_BYTES 15
#endif

// This should not be changed.
#define MESSAGE_HEADER_BYTES 1

#define MESSAGE_DATA_BYTES MESSAGE_PAYLOAD_BYTES + MESSAGE_HEADER_BYTES

// Make sure we do not try to use more data than we can.
#if MESSAGE_DATA_BYTES > IR_DATAGRAM_LEN
#error MESSAGE_DATA_BYTES must not be greater than DATAGRAM_BYTES.
#endif

// Message id 0 is used for fire and forget reset messages.
#define MESSAGE_RESET 0

namespace broadcast {

union MessageHeader {
  struct {
    bool is_fire_and_forget : 1;
    bool is_reply : 1;
    byte sequence : 3;
    byte id : 3;
  };

  byte as_byte;
};

struct Message {
  MessageHeader header;

  byte payload[MESSAGE_PAYLOAD_BYTES];
};

namespace message {

void Initialize(Message* message, byte id, bool is_fire_and_forget);

void ClearPayload(Message* message);

}  // namespace message

}  // namespace broadcast

#endif  // MESSAGE_H_
