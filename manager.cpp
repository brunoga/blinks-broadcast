#include "manager.h"

#include <blinklib.h>
#include <string.h>

#include "bits.h"
#include "message.h"
#include "message_tracker.h"

#ifdef BROADCAST_ENABLE_MESSAGE_HANDLER
#include "handler.h"
#endif

#ifndef BGA_CUSTOM_BLINKLIB
#error \
    "This code requires a custom blinklib. See https://github.com/brunoga/blinklib/releases/latest"
#endif

namespace broadcast {

namespace manager {

static ReceiveMessageHandler rcv_message_handler_ = nullptr;
static ForwardMessageHandler fwd_message_handler_ = nullptr;
static ReceiveReplyHandler rcv_reply_handler_ = nullptr;
static ForwardReplyHandler fwd_reply_handler_ = nullptr;

static byte parent_face_ = FACE_COUNT;
static byte sent_faces_;

static Message *result_;

// Return true if we generated a result (as opposed to not doing anything or
// forwarding a reply back to the parent).
static void maybe_fwd_reply_or_set_result(Message *message) {
  if (sent_faces_ != 0) return;

  message->header.is_reply = true;
  message::ClearPayload(message);

  byte len = BROADCAST_MESSAGE_PAYLOAD_BYTES;
  if (fwd_reply_handler_ != nullptr) {
    len =
        fwd_reply_handler_(message->header.id, parent_face_, message->payload);
  }

  if (parent_face_ != FACE_COUNT) {
    // This was the last face we were waiting on and we have a parent.
    // Send reply back.

    // Should never fail.
    sendDatagramOnFace((const byte *)message,
                       len + BROADCAST_MESSAGE_HEADER_BYTES, parent_face_);

    // Reset parent face.
    parent_face_ = FACE_COUNT;
  } else {
    // Generated a result. Note that the code will mark the datagram as read.
    // This is fine though as a result is only supposed to be valid in the
    // same loop() iteration it was generated.
    result_ = message;
  }
}

static void broadcast_message(byte src_face, broadcast::Message *message) {
  // Broadcast message to all connected blinks (except the parent one).

  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f)) {
      // No one seem to be connected to this face. Not necessarily true but
      // there is not much we can do here. Even if a face is connected but
      // show as expired, the way messages are routed should make up for it
      // anyway.
      continue;
    }

    if (f == src_face) {
      // Do not send message back to parent.
      continue;
    }

    broadcast::Message fwd_message;
    memcpy(&fwd_message, message, BROADCAST_MESSAGE_DATA_BYTES);

    byte len = BROADCAST_MESSAGE_PAYLOAD_BYTES;
    if (fwd_message_handler_ != nullptr) {
      len = fwd_message_handler_(fwd_message.header.id, src_face, f,
                                 fwd_message.payload);
    };

    // Should never fail.
    sendDatagramOnFace((const byte *)&fwd_message,
                       len + BROADCAST_MESSAGE_HEADER_BYTES, f);

    if (!message->header.is_fire_and_forget) {
      SET_BIT(sent_faces_, f);

      // We do not need to set parent_face_ on every loop iteration, but this
      // actually saves us some storage space.
      parent_face_ = src_face;
    }
  }

  if (message->header.id == MESSAGE_RESET) {
    // This was a reset message. Clear relevant data.
    sent_faces_ = 0;
    parent_face_ = FACE_COUNT;
  }
}

static bool would_forward_reply_and_fail(byte face) {
  // Processing the message on this face would clear its sent_face_ bit.
  UNSET_BIT(sent_faces_, face);

  if ((sent_faces_ == 0) && (parent_face_ != FACE_COUNT) &&
      isDatagramPendingOnFace(parent_face_)) {
    // Processing this message would result in us forwarding a reply to the
    // parent face, which would fail as there is already a datagram pending
    // to be sent on it. Reset the sent faces bit and let the caller know about
    // that.
    SET_BIT(sent_faces_, face);

    return true;
  }

  // No reply would be forwarded or it would not fail.
  return false;
}

static bool would_broadcast_fail(byte src_face) {
  // Check if all faces we would broadcast to arer available.
  FOREACH_FACE(dst_face) {
    // TODO(bga): We might want to check for face expiration here but doing that
    // results in an extra 32 bytes of storage being used. So we are not doing
    // it now and will revisit if needed.

    if (dst_face == src_face) {
      // We do not broadcast to the parent face. Skip it.
      continue;
    }

    if (isDatagramPendingOnFace(dst_face)) {
      // We would broadcast to this face but there is a datagram pending on it.
      // We would fail if we tried to broadcast.
      return true;
    }
  }

  // All faces are available.
  return false;
}

static bool handle_reply(byte face, Message *reply) {
  if (would_forward_reply_and_fail(face)) {
    // Do not even try processing this message.
    return false;
  }

  // Note the call above already cleared the sent_faces_ bit for face.

  if (rcv_reply_handler_ != nullptr) {
    rcv_reply_handler_(reply->header.id, face, reply->payload);
  }

  maybe_fwd_reply_or_set_result(reply);

  return true;
}

static bool maybe_broadcast(byte face, Message *message) {
  if (would_broadcast_fail(face)) {
    // Do not try to process this message and broadcast it. Note that this might
    // prevent us from making progress and creating a deadlock but there is only
    // so much we can do about this.
    return false;
  }

  // We are clear to go. Track message.
  message::tracker::Track(message->header);

  if (face != FACE_COUNT && rcv_message_handler_ != nullptr) {
    rcv_message_handler_(message->header.id, face, message->payload, false);
  }

  // Broadcast message.
  broadcast_message(face, message);

  return true;
}

static bool handle_message(byte face, Message *message) {
  if (message::tracker::Tracked(message->header)) {
    if (!message->header.is_fire_and_forget) {
      if (IS_BIT_SET(sent_faces_, face)) {
        if (would_forward_reply_and_fail(face)) {
          // Do not even try processing this message.
          return false;
        }

        // Note the call above already cleared the sent_faces_ bit for face.
      } else {
        // Late propagation message. Send header back to the other Blink so it
        // will not wait on us.
        return sendDatagramOnFace(message, 1, face);
      }
    }

    // Call receive message handler to process loop.
    if (rcv_message_handler_ != nullptr) {
      rcv_message_handler_(message->header.id, face, nullptr, true);
    }
  } else {
    if (!maybe_broadcast(face, message)) return false;
  }

  if (!message->header.is_fire_and_forget) {
    maybe_fwd_reply_or_set_result(message);
  }

  return true;
}

void Setup(ReceiveMessageHandler rcv_message_handler,
           ForwardMessageHandler fwd_message_handler,
           ReceiveReplyHandler rcv_reply_handler,
           ForwardReplyHandler fwd_reply_handler) {
  rcv_message_handler_ = rcv_message_handler;
  fwd_message_handler_ = fwd_message_handler;
  rcv_reply_handler_ = rcv_reply_handler;
  fwd_reply_handler_ = fwd_reply_handler;
}

void Process() {
  // Results are only valid in the same loop iteration they were generated.
  result_ = nullptr;

  // We might be dealing with multiple messages propagating here so we need to
  // try very hard to make progress in processing messages or things may stall
  // (as we always try to wait on all local message to be sent before trying
  // to process anything). The general idea here is that several messages we
  // receive (usually most of them) might be absorbed locally (replies other
  // than the last one we are waiting for and message loops) so the strategy
  // will be to simply try to process everything and only consume messages we
  // processed. This is the best we can do and although it mitigates issues,
  // there can always be pathological cases where we might stall (say, 6
  // different new messages arriving at the same loop iteration in all faces).
  // Ideally we would have enought memory for a message queue, but we do not
  // have this luxury.
  FOREACH_FACE(face) {
    if (getDatagramLengthOnFace(face) == 0) {
      // No datagram waiting on this face. Move to the next one.
      continue;
    }

    // Get a pointer to the available data. Notice this does not actually
    // consume it. We will do it when we are sure it has been handled. We
    // cheat a bit and cast directly to a message. Note that the payload on
    // the message we received might be smaller than MESSAGE_PAYLOAD_SIZE. but
    // this does not matter because the underlying receive buffer will always
    // be big enough so no illegal memory access should happen.
    broadcast::Message *message = (broadcast::Message *)getDatagramOnFace(face);

    bool message_consumed;

    // Now we try to consume the message. We do this in the simplest way
    // possible by procerssing the message and if we reach a point where it
    // would result in messages being sent, we try to detect this and check
    // beforehand if sending would fail. If it would, we abort and do not
    // consume the message. If it would not or no messages would be sent, we
    // consume it.
#ifdef BROADCAST_ENABLE_MESSAGE_HANDLER
    if (message::handler::Consume(message)) {
      // Message was consumed by external handler.
      message_consumed = true;
    } else if (message->header.is_reply) {
#else
    if (message->header.is_reply) {
#endif
      // Reply.
      message_consumed = handle_reply(face, message);
    } else {
      // Message.
      message_consumed = handle_message(face, message);
    }

    if (message_consumed) {
      markDatagramReadOnFace(face);
    }
  }

#ifdef BROADCAST_ENABLE_MESSAGE_HANDLER
  // We tried to process all incoming messages. Now check if there is any
  // handler message availabe to propagate.
  Message message;
  if (message::handler::Propagate(&message)) {
    // Yep, there is. Try to send one message.
    if (Send(&message)) {
      // Send was successfull. Mark message as propagated.
      message::handler::Propagated();
    }
  }
#endif
}

bool Send(broadcast::Message *message) {
  // Setup tracking for this message.
  message->header.sequence = message::tracker::LastSequence() + 1;

  return maybe_broadcast(FACE_COUNT, message);
}

bool Receive(broadcast::Message *reply) {
  if (result_ == nullptr) return false;

  if (reply != nullptr) {
    memcpy(reply, result_, BROADCAST_MESSAGE_DATA_BYTES);
  }

  return true;
}

bool Processing() { return sent_faces_ != 0; }

}  // namespace manager

}  // namespace broadcast
