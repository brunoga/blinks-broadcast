#ifndef HANDLER_H_
#define HANDLER_H_

#include <blinklib.h>

#include "message.h"

namespace broadcast {

namespace message {

struct Handler {
  byte message_id;

  // This will be called whenever a new externally handled message reaches the
  // local Blink. The handler should do whatever is needed to process this
  // message right away as it will be consumed when this returns. It is not a
  // good idea to try to send messages here.
  void (*consume)(const Message*);

  // Should return true if there is a message available to be propagated of
  // false othwerwise. If there is a message, it should be copied to the given
  // parameter. This message must be of message_id type and is most likelly
  // based on messages that were consumed by consume(). Note that consume might
  // be called an arbitrary number of times before propagate() is so code must
  // make provisions for this to work.
  bool (*propagate)(Message*);

  // This is called whenever a message is successfuly propagated so the handler
  // can update its internal state.
  void (*propagated)();
};

namespace handler {

void Set(const Handler& handler);

bool Consume(const Message* message);
bool Propagate(Message* message);
void Propagated();

}  // namespace handler

}  // namespace message

}  // namespace broadcast

#endif