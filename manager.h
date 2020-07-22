#ifndef MANAGER_H_
#define MANAGER_H_

#include "message.h"

namespace broadcast {

namespace manager {

// Prototype for functions that want to change the payload of a message as soon
// as it reaches a node. It is always called once per message and any changes in
// the payload will be seen by upstream nodes (unless modified by a
// ForwardMessageHandler. See below). The message_id parameter can be used to
// differentiate between messages so specific code can be executed.
typedef void (*ReceiveMessageHandler)(byte message_id, byte *payload);

// Prototype for functions that want to change the payload of a message just
// before it is forwarded to the next node. This is called once per connected
// face so it might never be called for a specific message (in case there are no
// connected faces) or might be called multiple times. Each call gets a copy of
// the payload so modifications by other ForwardMessageHandlers that might
// happen to run first are not seem. Any modifications to the payload will be
// seen by upstream nodes. This can be used to, for example, add some type of
// routing information. The message_id parameter can be used to differentiate
// between messages so specific code can be executed.
typedef void (*ForwardMessageHandler)(byte message_id, byte src_face,
                                      byte dst_face, byte *payload);

// Prototype for functions that want to take action on  a reply as soon as
// it reaches a node. It is always called once per reply and as there are
// possibly multiple replies arriving, payload is read-only. The message_id
// parameter can be used to differentiate between messages so specific code can
// be executed.
typedef void (*ReceiveReplyHandler)(byte message_id, const byte *payload);

// Prototype for functions that want to change the payload of a reply just
// before it is sent back to the parent. It will be called only once after all
// replies have been processed so its main function is to  make sense of all the
// replies and set the payload to something meaningful. The message_id parameter
// can be used to differentiate between different messages so specific code can
// be executed.
typedef void (*ForwardReplyHandler)(byte message_id, byte *payload);

// Configures the message manager with the given handlers.
void Setup(ReceiveMessageHandler rcv_message_handler,
           ForwardMessageHandler fwd_message_handler,
           ReceiveReplyHandler rcv_reply_handler,
           ForwardReplyHandler fwd_reply_handler);

// Updates the status of the MessageManager. If it returns true (this should
// ever only happen in the node that originally sent the message), then result
// will contain the actual result of aggregating the message payloads from all
// nodes. If it returns false, then the result value is meaningless. This should
// be called at every loop iteration.
void Process();

// Sends the given message to all available neighboors so it can be propagated
// through the network. The forward_handler callback is called whenever a new
// message being routed reaches the current node and the reply_handler callback
// is called for any replies that reaches it. Returns true if the message was
// sent and false otherwise.
bool Send(const broadcast::message::Message message);

// Tries to receive the result of a sent message. This will only ever return
// true at the same node that sent the message. Returns true if a result was
// available and false otherwise.
bool Receive(broadcast::message::Message result);

}  // namespace manager

}  // namespace broadcast

#endif  // MANAGER_H_
