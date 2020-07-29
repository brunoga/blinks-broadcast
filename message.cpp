#include "message.h"

#include <string.h>  // For memcpy.

#include "bits.h"

#define MESSAGE_HEADER_ID_BYTE 0
#define MESSAGE_HEADER_SEQUENCE_BYTE 1

#define MESSAGE_HEADER_ID_BIT_REPLY 7
#define MESSAGE_HEADER_ID_BIT_FIRE_AND_FORGET 6

namespace broadcast {

namespace message {

void Set(Message message, byte id, byte sequence, const byte* payload,
         bool is_reply, bool fire_and_forget) {
  if (is_reply) {
    SET_BIT(id, MESSAGE_HEADER_ID_BIT_REPLY);
  }

  if (fire_and_forget) {
    SET_BIT(id, MESSAGE_HEADER_ID_BIT_FIRE_AND_FORGET);
  }

  message[MESSAGE_HEADER_ID_BYTE] = id;
  message[MESSAGE_HEADER_SEQUENCE_BYTE] =
      id == 0 ? 0 : (sequence == 0 ? random(254) + 1 : sequence);

  // Initialize with either the payload given or zeroes if the payload is
  // nullptr.
  for (byte i = 0; i < MESSAGE_PAYLOAD_BYTES; ++i) {
    message[MESSAGE_HEADER_BYTES + i] = (payload == nullptr ? 0 : payload[i]);
  }
}

void Set(Message message, const byte* data) {
  memcpy(message, data, MESSAGE_DATA_BYTES);
}

byte ID(const Message message) {
  byte id = message[MESSAGE_HEADER_ID_BYTE];

  // Clear reserved bits.
  UNSET_BIT(id, MESSAGE_HEADER_ID_BIT_REPLY);
  UNSET_BIT(id, MESSAGE_HEADER_ID_BIT_FIRE_AND_FORGET);

  return id;
}

byte Sequence(const Message message) {
  return message[MESSAGE_HEADER_SEQUENCE_BYTE];
}

const byte* Payload(const Message message) {
  return &message[MESSAGE_HEADER_BYTES];
}

byte* MutablePayload(Message message) { return &message[MESSAGE_HEADER_BYTES]; }

bool IsReply(const Message message) {
  // Check if the reply bit is set.
  return IS_BIT_SET(message[MESSAGE_HEADER_ID_BYTE],
                    MESSAGE_HEADER_ID_BIT_REPLY);
}

bool IsFireAndForget(const Message message) {
  // Check if the fire-and-forget bit is set.
  return IS_BIT_SET(message[MESSAGE_HEADER_ID_BYTE],
                    MESSAGE_HEADER_ID_BIT_FIRE_AND_FORGET);
}

}  // namespace message

}  // namespace broadcast
