#include "handler.h"

namespace broadcast {

namespace message {

Handler handler_;

namespace handler {

void Set(const Handler& handler) { handler_ = handler; }

bool Consume(const Message* message) {
  if ((handler_.consume != nullptr) &&
      (message->header.id == handler_.message_id)) {
    handler_.consume(message);
    return true;
  }

  return false;
}

bool Propagate(Message* message) {
  if (handler_.propagate != nullptr) {
    return handler_.propagate(message);
  }

  return false;
}

void __attribute__((noinline)) Propagated() {
  if (handler_.propagated != nullptr) handler_.propagated();
}

}  // namespace handler

}  // namespace message

}  // namespace broadcast
