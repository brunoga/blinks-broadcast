#ifndef BROADCAST_CONFIG_H_
#define BROADCAST_CONFIG_H_

// Sample configuration for the Blinks Broadcast library.

// Maximum allowed payload size.
//#define BROADCAST_MESSAGE_PAYLOAD_BYTES 15

// Enable message handler support. Allows sending messages that can be
// externally tracked. This is just to test this specific code path.
//#define BROADCAST_ENABLE_MESSAGE_HANDLER

// Disable message reply support. Saves a lot of storage space for programs that
// only need fire and forget messages.
//#define BROADCAST_DISABLE_REPLIES

// Define message handlers.

// Prototype for functions that want to handle external messages. These are
// messages that are not tracked or directly processed by the broadcast message
// handler (they will be processed by the external message handler).
// Implementations should return true if the message was processed by it and
// false otherwise.
//
// bool external_message_handler(byte face, const Message* message);
//
// #define BROADCAST_EXTERNAL_MESSAGE_HANDLER external_message_handler

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
//
// void rcv_message_handler(byte message_id, byte src_face, byte *payload,
//      bool loop);
//
//#define BROADCAST_RCV_MESSAGE_HANDLER rcv_message_handler

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
//
// byte fwd_message_handler(byte message_id, byte src_face, byte dst_face, byte
//      *payload);
//
//#define BROADCAST_FWD_MESSAGE_HANDLER fwd_message_handler

// Prototype for functions that want to take action on  a reply as soon as
// it reaches a Blink. It is always called once per reply and as there are
// possibly multiple replies arriving, payload is read-only. The message_id
// parameter can be used to differentiate between messages so specific code can
// be executed. The src_face parameter is the face the reply arrived on. This is
// never called for fire-and-forget messages.
//
// void rcv_reply_handler(byte message_id, byte src_face, const byte *payload);
//
//#define BROADCAST_RCV_REPLY_HANDLER rcv_reply_handler

// Prototype for functions that want to change the payload of a reply just
// before it is sent back to the parent Blink. It will be called only once after
// all replies have been processed so its main function is to  make sense of all
// the replies and set the payload to something meaningful. The message_id
// parameter can be used to differentiate between different messages so specific
// code can be executed. The dst_face parameter is the face the reply will be
// forwarded to. This is never called for fire-and-forget messages.
// Implementations should return the actual reply payload size (which might be
// smaller than MESSAGE_PAYLOAD_BYTES).
//
// byte fwd_reply_handler(byte message_id, byte dst_face, byte *payload);
//
//#define BROADCAST_FWD_REPLY_HANDLER fwd_reply_handler

#endif  // BROADCAST_CONFIG_H_