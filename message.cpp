#include "message.h"

#include "bits.h"

#define MESSAGE_BIT_REPLY 7
#define MESSAGE_BIT_FIRE_AND_FORGET 6

namespace broadcast {

namespace message {

void Set(Message message, byte id, const byte* payload, bool is_reply,
         bool fire_and_forget) {
  if (is_reply) {
    support::SetBit(&id, MESSAGE_BIT_REPLY);
  }

  if (fire_and_forget) {
    support::SetBit(&id, MESSAGE_BIT_FIRE_AND_FORGET);
  }

  message[0] = id;

  // Initialize with either the payload given or zeroes if the payload is
  // nullptr.
  for (byte i = 0; i < MESSAGE_PAYLOAD_BYTES; ++i) {
    message[MESSAGE_HEADER_BYTES + i] = (payload == nullptr ? 0 : payload[i]);
  }
}

byte ID(const Message message) {
  byte id = message[0];

  // Clear reserved bits.
  support::UnsetBit(&id, MESSAGE_BIT_REPLY);
  support::UnsetBit(&id, MESSAGE_BIT_FIRE_AND_FORGET);

  return id;
}

const byte* Payload(const Message message) {
  return &message[MESSAGE_HEADER_BYTES];
}

byte* MutablePayload(Message message) { return &message[MESSAGE_HEADER_BYTES]; }

bool IsReply(const Message message) {
  // Check if the reply bit is set.
  return support::IsBitSet(message[0], MESSAGE_BIT_REPLY);
}

bool IsFireAndForget(const Message message) {
  // Check if the fire-and-forget bit is set.
  return support::IsBitSet(message[0], MESSAGE_BIT_FIRE_AND_FORGET);
}

}  // namespace message

}  // namespace broadcast
