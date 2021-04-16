#ifndef MANAGER_H_
#define MANAGER_H_

#include "message.h"

namespace broadcast {

namespace manager {

// Processes all pending incoming and outgoing messages. This should be called
// at the top of every loop() iteration.
void Process();

// Sends the given message to all connected Blinks so it can be propagated
// through the network. Returns true if the message was sent and false
// otherwise.
bool Send(broadcast::Message *message);

#ifndef BROADCAST_DISABLE_REPLIES
// Tries to receive the result of a sent message. This will only ever return
// true at the same Blink that sent the message. Returns true if a result was
// available and false otherwise. Note that this will never return true for
// fire-and-forget messages.
bool Receive(broadcast::Message *result);

// Returns true if we are still waiting for replies for a message in progress.
// This can be used to prevent other messages being sent before we complete the
// current work.
bool Processing();
#endif

}  // namespace manager

}  // namespace broadcast

#endif  // MANAGER_H_
