#include "handler.h"

namespace broadcast {

namespace message {

Handler handler_;

namespace handler {

void Set(Handler handler) { handler_ = handler; }

bool Consume(const Message* message, byte face) {
  if ((handler_.consume == nullptr) ||
      (message->header.id != handler_.message_id)) {
    return false;
  }

  handler_.consume(message, face);

  return true;
}

}  // namespace handler

}  // namespace message

}  // namespace broadcast
