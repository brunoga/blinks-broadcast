#include "manager.h"

#include <blinklib.h>
#include <string.h>

#include "bits.h"
#include "message.h"
#include "message_tracker.h"

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

static MessageHeader pending_clear_face_headers_[FACE_COUNT];

static Message result_;
static bool has_result_;

static void maybe_send_reply_or_set_result(Message *message) {
  if (sent_faces_ == 0) {
    // We are not waiting on any faces anymore.
    Message *fwd_reply = message;
    if (parent_face_ == FACE_COUNT) {
      // No parent. Set our local result and mark it as available.
      fwd_reply = &result_;
      has_result_ = true;
    }

    message::Initialize(fwd_reply, message->header.id, false);
    message::ClearPayload(fwd_reply);

    message->header.is_reply = true;

    byte len = MESSAGE_PAYLOAD_BYTES;
    if (fwd_reply_handler_ != nullptr) {
      len = fwd_reply_handler_(fwd_reply->header.id, parent_face_,
                               fwd_reply->payload);
    }

    if (parent_face_ != FACE_COUNT) {
      // This was the last face we were waiting on and we have a parent.
      // Send reply back.

      // Should never fail.
      sendDatagramOnFace((const byte *)fwd_reply, len + MESSAGE_HEADER_BYTES,
                         parent_face_);

      // Reset parent face.
      parent_face_ = FACE_COUNT;
    }
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
    memcpy(&fwd_message, message, MESSAGE_DATA_BYTES);

    byte len = MESSAGE_PAYLOAD_BYTES;
    if (fwd_message_handler_ != nullptr) {
      len = fwd_message_handler_(fwd_message.header.id, src_face, f,
                                 fwd_message.payload);
    };

    // Should never fail.
    sendDatagramOnFace((const byte *)&fwd_message, len + MESSAGE_HEADER_BYTES,
                       f);

    if (!message->header.is_fire_and_forget) {
      SET_BIT(sent_faces_, f);
    } else if (message->header.id == MESSAGE_RESET) {
      // This is a reset message. Clear relevant data.
      sent_faces_ = 0;
      parent_face_ = FACE_COUNT;
    }
  }
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

static bool handle_reply(byte face, Message *reply) {
  // Mark face as not pending anymore.
  UNSET_BIT(sent_faces_, face);

  if ((sent_faces_ == 0) && (parent_face_ != FACE_COUNT) &&
      isDatagramPendingOnFace(face)) {
    // Processing this message would result on us forwarding a reply to the
    // parent face, which would fail as there is already a datagram pending to
    // be sent on it. Reset the sent faces bit and return without consuming
    // the message.
    SET_BIT(sent_faces_, face);

    return false;
  }

  if (rcv_reply_handler_ != nullptr) {
    rcv_reply_handler_(reply->header.id, face, reply->payload);
  }

  maybe_send_reply_or_set_result(reply);

  return true;
}

static bool handle_fire_and_forget(byte face, Message *message, bool tracked) {
  if (tracked) {
    // We detected a message loop. Call the receive handler to take care
    // of it.
    if (rcv_message_handler_ != nullptr) {
      rcv_message_handler_(message->header.id, face, nullptr, true);
    }

    return true;
  }

  // Check if all the faces we are going to send to are available.
  FOREACH_FACE(out_face) {
    if (out_face == face) continue;

    if (isDatagramPendingOnFace(out_face)) {
      // At least one is not. Do not consume message and try again later.
      return false;
    }
  }

  // We are clear to go!
  message::tracker::Track(message->header);

  if (rcv_message_handler_ != nullptr) {
    rcv_message_handler_(message->header.id, face, message->payload, false);
  }

  // Broadcast message.
  broadcast_message(face, message);

  return true;
}

static bool handle_message(byte face, Message *message, bool tracked) {
  if (tracked) {
    if (IS_BIT_SET(sent_faces_, face)) {
      // We already sent to this face, so this is a non fire-and-forget loop.
      // Mark face as not sent.
      UNSET_BIT(sent_faces_, face);

      if (rcv_message_handler_ != nullptr) {
        rcv_message_handler_(message->header.id, face, nullptr, true);
      }
    } else {
      // Indicate we need to clear this face and do it when possible. This
      // avoids sending a message right now and tying up a face.
      pending_clear_face_headers_[face] = message->header;

      return true;
    }
  } else {
    // Check if all the faces we are going to send to are available.
    FOREACH_FACE(out_face) {
      if (out_face == face) continue;

      if (isDatagramPendingOnFace(out_face)) {
        // At least one is not. Do not consume message and try again later.
        return false;
      }
    }

    parent_face_ = face;

    // We are clear to go!
    message::tracker::Track(message->header);

    if (rcv_message_handler_ != nullptr) {
      rcv_message_handler_(message->header.id, face, message->payload, false);
    }

    // Broadcast message.
    broadcast_message(face, message);
  }

  maybe_send_reply_or_set_result(message);

  return true;
}

void Process() {
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

    bool message_consumed = false;

    // Now we try to consume the message. We do this in the simplest way
    // possible by procerssing the message and if we reach a point where it
    // would result in messages being sent, we try to send it. If sending
    // fails, we do not consume the message. If it does not fail or we do not
    // send, we consume it.
    if (message->header.is_reply) {
      // Reply message.
      message_consumed = handle_reply(face, message);
    } else {
      bool tracked = message::tracker::Tracked(message->header);

      if (message->header.is_fire_and_forget) {
        // Fire and forget message.
        message_consumed = handle_fire_and_forget(face, message, tracked);
      } else {
        // Normal message.
        message_consumed = handle_message(face, message, tracked);
      }
    }

    if (message_consumed) {
      markDatagramReadOnFace(face);
    }
  }

  // Try to clear pending faces.
  FOREACH_FACE(face) {
    if (pending_clear_face_headers_[face].as_byte != 0) {
      if (sendDatagramOnFace(&pending_clear_face_headers_[face], 1, face)) {
        pending_clear_face_headers_[face].as_byte = 0;
      }
    }
  }
}

bool Send(broadcast::Message *message) {
  // TODO(bga): Revisit this check.
  if (sent_faces_ != 0 && !message->header.is_fire_and_forget) return false;

  if (isDatagramPendingOnAnyFace()) {
    // There are currently pending transfers in progress so sending would fail
    // silently if we tried to continue.
    return false;
  }

  // Setup tracking for this message.
  message->header.sequence = message::tracker::LastSequence() + 1;
  message::tracker::Track(message->header);

  broadcast_message(FACE_COUNT, message);

  return true;
}

bool Receive(broadcast::Message *reply) {
  if (!has_result_) return false;

  has_result_ = false;

  if (reply != nullptr) {
    memcpy(reply, &result_, MESSAGE_DATA_BYTES);
  }

  return true;
}

bool Processing() { return sent_faces_ != 0; }

}  // namespace manager

}  // namespace broadcast
