#include "message.h"

#include <string.h>  // For memcpy.

namespace broadcast {

namespace message {

#ifdef BROADCAST_DISABLE_REPLIES
void Initialize(Message* message, byte id) {
#else
void Initialize(Message* message, byte id, bool is_fire_and_forget) {
#endif
  message->header.id = id;
  message->header.sequence = 0;

#ifndef BROADCAST_DISABLE_REPLIES
  message->header.is_reply = false;
  message->header.is_fire_and_forget = is_fire_and_forget;
#endif

  ClearPayload(message);
}

void ClearPayload(Message* message) {
  memset(message->payload, 0, BROADCAST_MESSAGE_PAYLOAD_BYTES);
}

}  // namespace message

}  // namespace broadcast
