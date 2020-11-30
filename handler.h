#ifndef HANDLER_H_
#define HANDLER_H_

#include <blinklib.h>

#include "message.h"

#define MESSAGE_HANDLER_MODE_CONSUME 0
#define MESSAGE_HANDLER_MODE_PROPAGATE 1
#define MESSAGE_HANDLER_MODE_PROPAGATED 2

namespace broadcast {

namespace message {

struct Handler {
  byte message_id;

  bool (*handle)(Message*, byte mode);
};

namespace handler {

void Set(Handler handler);

bool Consume(const Message* message);
bool Propagate(Message* message);
void Propagated();

}  // namespace handler

}  // namespace message

}  // namespace broadcast

#endif