#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <blinklib.h>

#include "payload_bytes.h"

// Set this to the number of bytes you need to send as payload. Anything up to
// 15 bytes is fair game.
#ifndef MESSAGE_PAYLOAD_BYTES
#error You must define MESSAGE_PAYLOAD_BYTES.
#endif

// This should not be changed.
#define MESSAGE_HEADER_BYTES 1

#define MESSAGE_DATA_BYTES MESSAGE_PAYLOAD_BYTES + MESSAGE_HEADER_BYTES

// Make sure we do not try to use more data than we can.
#if MESSAGE_DATA_BYTES > IR_DATAGRAM_LEN
#error MESSAGE_DATA_BYTES must not be greater than DATAGRAM_BYTES.
#endif

// Message id 0 is reserved to indicate an invalid message.
#define MESSAGE_INVALID 0

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
