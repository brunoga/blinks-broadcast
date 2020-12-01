#ifndef HANDLER_H_
#define HANDLER_H_

#include <blinklib.h>

#include "message.h"

namespace broadcast {

namespace message {

struct Handler {
  byte message_id;

  void (*consume)(const Message*, byte);
};

namespace handler {

void Set(Handler handler);

bool Consume(const Message* message, byte face);

}  // namespace handler

}  // namespace message

}  // namespace broadcast

#endif