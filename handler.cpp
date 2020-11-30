#include "handler.h"

namespace broadcast {

namespace message {

Handler handler_;

namespace handler {

static bool maybe_call_handle(Message* message, byte mode) {
  return (handler_.handle != nullptr) && handler_.handle(message, mode);
}

void Set(Handler handler) { handler_ = handler; }

bool Consume(const Message* message) {
  return (message->header.id == handler_.message_id) &&
         maybe_call_handle((Message*)message, MESSAGE_HANDLER_MODE_CONSUME);
}

bool Propagate(Message* message) {
  return maybe_call_handle(message, MESSAGE_HANDLER_MODE_PROPAGATE);
}

void Propagated() {
  maybe_call_handle(nullptr, MESSAGE_HANDLER_MODE_PROPAGATED);
}

}  // namespace handler

}  // namespace message

}  // namespace broadcast
