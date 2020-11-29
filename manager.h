#ifndef MANAGER_H_
#define MANAGER_H_

#include "message.h"

namespace broadcast {

namespace manager {

// Prototype for functions that want to change the payload of a message as soon
// as it reaches a Blink. It is always called once per message that arrives and
// any changes in the payload will be seen by upstream Blinks (unless modified
// by a ForwardMessageHandler. See below). The message_id parameter can be used
// to differentiate between messages so specific code can be executed. The
// src_face parameter is the face the message arrived on. If the loop parameter
// is true, this means that this handler was called because a message that
// caused a loop was received. In this case, payload will be nullptr and any
// actions should be based only on message_id and src_face. This is also called
// for fire-and-forget messages.
typedef void (*ReceiveMessageHandler)(byte message_id, byte src_face,
                                      byte *payload, bool loop);

// Prototype for functions that want to change the payload of a message just
// before it is forwarded to the next Blink. This is called once per connected
// face so it might never be called for a specific message (in case there are no
// connected faces) or might be called multiple times. Each call gets a copy of
// the payload so modifications by other ForwardMessageHandlers that might
// happen to run first are not seem. Any modifications to the payload will be
// seen by upstream Blinks. This can be used to, for example, add some type of
// routing information. The message_id parameter can be used to differentiate
// between messages so specific code can be executed. the src_face and dst_face
// are, respectively, the face the message arrived from and the face it is being
// forwarded to. This is also called for fire-and-forget messages.
// Implementations should return the actual message payload size (which might be
// smaller than MESSAGE_PAYLOAD_BYTES).
typedef byte (*ForwardMessageHandler)(byte message_id, byte src_face,
                                      byte dst_face, byte *payload);

#ifndef BROADCAST_DISABLE_REPLIES
// Prototype for functions that want to take action on  a reply as soon as
// it reaches a Blink. It is always called once per reply and as there are
// possibly multiple replies arriving, payload is read-only. The message_id
// parameter can be used to differentiate between messages so specific code can
// be executed. The src_face parameter is the face the reply arrived on. This is
// never called for fire-and-forget messages.
typedef void (*ReceiveReplyHandler)(byte message_id, byte src_face,
                                    const byte *payload);

// Prototype for functions that want to change the payload of a reply just
// before it is sent back to the parent Blink. It will be called only once after
// all replies have been processed so its main function is to  make sense of all
// the replies and set the payload to something meaningful. The message_id
// parameter can be used to differentiate between different messages so specific
// code can be executed. The dst_face parameter is the face the reply will be
// forwarded to. This is never called for fire-and-forget messages.
// Implementations should return the actual reply payload size (which might be
// smaller than MESSAGE_PAYLOAD_BYTES).
typedef byte (*ForwardReplyHandler)(byte message_id, byte dst_face,
                                    byte *payload);
#endif

// Configures the message manager with the given handlers.
#ifdef BROADCAST_DISABLE_REPLIES
void Setup(ReceiveMessageHandler rcv_message_handler,
           ForwardMessageHandler fwd_message_handler);
#else
void Setup(ReceiveMessageHandler rcv_message_handler,
           ForwardMessageHandler fwd_message_handler,
           ReceiveReplyHandler rcv_reply_handler,
           ForwardReplyHandler fwd_reply_handler);
#endif
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
