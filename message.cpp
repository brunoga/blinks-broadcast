#include "message.h"

#include <string.h>  // For memcpy.

namespace broadcast {

namespace message {

void Initialize(Message* message, byte id, bool is_fire_and_forget) {
  message->header.id = id;
  message->header.sequence = 0;

  message->header.is_reply = false;
  message->header.is_fire_and_forget = is_fire_and_forget;

  ClearPayload(message);
}

void ClearPayload(Message* message) {
  for (byte i = 0; i < MESSAGE_PAYLOAD_BYTES; ++i) {
    message->payload[i] = 0;
  }
}

}  // namespace message

}  // namespace broadcast
