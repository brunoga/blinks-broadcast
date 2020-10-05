#include "manager.h"

#include <blinklib.h>
#include <string.h>

#include "message.h"
#include "message_tracker.h"
#include "src/blinks-support/bits.h"

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

static void send_reply(broadcast::Message *reply) {
  // Send a reply to our parent.

  // Set message as reply and clear payload.
  reply->header.is_reply = true;
  message::ClearPayload(reply);

  byte len = MESSAGE_PAYLOAD_BYTES;
  if (fwd_reply_handler_ != nullptr) {
    len = fwd_reply_handler_(reply->header.id, parent_face_, reply->payload);
  }

  // Should never fail.
  sendDatagramOnFace((const byte *)reply, len + MESSAGE_HEADER_BYTES,
                     parent_face_);

  // Reset parent face.
  parent_face_ = FACE_COUNT;
}

void maybe_send_reply_or_set_result(Message *message) {
  // Nothing to do for fire and forget messages.
  if (message->header.is_fire_and_forget) return;

  if (sent_faces_ == 0) {
    // We are not waiting on any faces anymore.
    if (parent_face_ == FACE_COUNT) {
      // We do not have a parent, so surface the result here.
      if (fwd_reply_handler_ != nullptr) {
        // We can ignore the return value here as it is irrelevant.
        fwd_reply_handler_(message->header.id, parent_face_, message->payload);
      }

      result_ = message;
    } else {
      // This was the last face we were waiting on and we have a parent.
      // Send reply back.
      send_reply(message);
    }
  }
}

static void broadcast_message(byte src_face, broadcast::Message *message) {
  // Broadcast message to all connected blinks (except the parent one).

  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f)) {
      // No one seem to be connected to this face. Not necessarily true but
      // there is not much we can do here. Even if a face is connected but show
      // as expired, the way messages are routed should make up for it anyway.
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

    if (!message->header.is_fire_and_forget) SET_BIT(sent_faces_, f);
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

static bool pending_send() {
  FOREACH_FACE(face) {
    if (isDatagramPendingOnFace(face)) return true;
  }

  return false;
}

static bool handle_message(byte f, Message *message) {
  // Are we already tracking this message?
  bool tracked = message::tracker::Tracked(message->header);

  // Keep track of detected loops.
  bool loop = false;

  if (message->header.is_fire_and_forget) {
    if (tracked) {
      // We are already tracking this message. This is a fire-and-forget message
      // loop.
      loop = true;
    }
  } else {
    if (IS_BIT_SET(sent_faces_, f)) {
      // We already sent to this face, so this is a non fire-and-forget loop.
      // Mark face as not sent.
      UNSET_BIT(sent_faces_, f);

      // Record loop.
      loop = true;
    } else {
      if (tracked) {
        // We got a message that we are already tracking after we got replies
        // from all faces. This is a late propagation message so we can not
        // simply ignore if it and have to tell the sender not to wait on us (by
        // forcing a loop). We can send only the header back as we just want the
        // peer to stop waiting on us. Note this is *NOT* a loop.

        // Should never fail.
        sendDatagramOnFace((const byte *)message, MESSAGE_HEADER_BYTES, f);

        return false;
      }

      // Set our parent face.
      parent_face_ = f;
    }
  }

  if (loop) {
    // We detected a message loop. Call the receive handler to take care
    // of it.
    if (rcv_message_handler_ != nullptr) {
      rcv_message_handler_(message->header.id, f, nullptr, true);
    }

    return true;
  }

  message::tracker::Track(message->header);

  if (rcv_message_handler_ != nullptr) {
    rcv_message_handler_(message->header.id, f, message->payload, false);
  }

  // Broadcast message.
  broadcast_message(f, message);

  return true;
}

static void handle_reply(byte f, Message *reply) {
  if (rcv_reply_handler_ != nullptr) {
    rcv_reply_handler_(reply->header.id, f, reply->payload);
  }

  // Mark face as not pending anymore.
  UNSET_BIT(sent_faces_, f);
}

void Process() {
  // Invalidate result as it most likelly is not the same data as when it was
  // received.
  result_ = nullptr;

  if (pending_send()) {
    // We wait until any pending messages are cleared before we move on with
    // processing. This guarantees that no sendDatagramOnFace() calls after
    // this will fail (assuming we do not try to send more than one time in a
    // single face, that is, which this code never does currently).
    return;
  }

  // Receive any pending data on all faces.

  FOREACH_FACE(f) {
    if (getDatagramLengthOnFace(f) == 0) {
      // No data available. Nothing to do.
      continue;
    }

    // Receive data.
    const byte *rcv_datagram = getDatagramOnFace(f);

    // We are removing the constness here but this is fine in this case and it
    // is worth to avoid copies. Also, we might receive a message that is
    // smaller than Message but it is ok as the underlying buffer is of the
    // right size.
    broadcast::Message *message = (broadcast::Message *)rcv_datagram;

    if (message->header.is_reply) {
      // Got a reply.
      handle_reply(f, message);
    } else {
      // Got a message.
      if (!handle_message(f, message)) continue;
    }

    maybe_send_reply_or_set_result(message);
  }
}

bool Send(broadcast::Message *message) {
  // TODO(bga): Revisit this check.
  if (sent_faces_ != 0 && !message->header.is_fire_and_forget) return false;

  if (pending_send()) {
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
  if (result_ == nullptr) return false;

  if (reply != nullptr) {
    memcpy(reply, result_, MESSAGE_DATA_BYTES);
  }

  return true;
}

bool Processing() { return sent_faces_ != 0; }

}  // namespace manager

}  // namespace broadcast
