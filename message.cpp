#include "message.h"

namespace broadcast {

namespace message {

void Set(Message message, byte id, const byte* payload, bool reply) {
  message[0] = reply ? (id | 128) : id;

  // Initialize with either the payload given or zeroes if the payload is
  // nullptr.
  for (byte i = 0; i < MESSAGE_PAYLOAD_BYTES; ++i) {
    message[MESSAGE_HEADER_BYTES + i] = (payload == nullptr ? 0 : payload[i]);
  }
}

byte ID(const Message message) {
  // ID without the reply bit.
  return message[0] & ~128;
}

const byte* Payload(const Message message) {
  return &message[MESSAGE_HEADER_BYTES];
}

byte* MutablePayload(Message message) { return &message[MESSAGE_HEADER_BYTES]; }

bool IsReply(const Message message) {
  // Check if the reply bit is set.
  return ((message[0] & 128) > 0);
}

}  // namespace message

}  // namespace broadcast
