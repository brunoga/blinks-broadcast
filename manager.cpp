#include "manager.h"

#include <blinklib.h>
#include <string.h>

#include "bits.h"
#include "debug.h"
#include "message.h"
#include "message_tracker.h"

#ifndef BGA_CUSTOM_BLINKLIB
#error "This code requires BGA's Custom Blinklib"
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
  sendDatagramOnFace((const byte *)reply, len + 1, parent_face_);

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

static void broadcast_message(broadcast::Message *message) {
  // Broadcast message to all connected blinks (except the parent one).

  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f)) {
      // No one seem to be connected to this face. Not necessarily true but
      // there is not much we can do here. Even if a face is connected but show
      // as expired, the way messages are routed should make up for it anyway.
      continue;
    }

    if (f == parent_face_) {
      // Do not send message back to parent.
      continue;
    }

    broadcast::Message fwd_message;
    memcpy(&fwd_message, message, MESSAGE_DATA_BYTES);

    byte len = MESSAGE_PAYLOAD_BYTES;
    if (fwd_message_handler_ != nullptr) {
      len = fwd_message_handler_(fwd_message.header.id, parent_face_, f,
                                 fwd_message.payload);
    };

    // Should never fail.
    sendDatagramOnFace((const byte *)&fwd_message, len + 1, f);

    SET_BIT(sent_faces_, f);
  }

  if (message->header.is_fire_and_forget) {
    // Fire and forget message. Reset everything relevant.
    parent_face_ = FACE_COUNT;
    sent_faces_ = 0;
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
    byte len = getDatagramLengthOnFace(f);

    if (len == 0) {
      // No data available. Nothing to do.
      continue;
    }

    const byte *rcv_datagram = getDatagramOnFace(f);
    markDatagramReadOnFace(f);

    // We are removing the constness here but this is fine in this case and it
    // is worth to avoid copies. Also, we might receive a message that is
    // smaller than Message but it is ok as the underlying buffer is of the
    // right size.
    broadcast::Message *message = (broadcast::Message *)rcv_datagram;

    if (!message->header.is_reply) {
      // We allow a fire and forget message to reset state unless it is a
      // message we already saw (so this would be a fire and forget loop, not an
      // original message).
      bool seen_fire_and_forget = message->header.is_fire_and_forget &&
                                  message::tracker::Tracked(message->header);
      // Got a message.
      if (IS_BIT_SET(sent_faces_, f) && !seen_fire_and_forget) {
        // We already sent to this face, so this is a loop. Mark face as not
        // sent.
        UNSET_BIT(sent_faces_, f);

        // Call receive handler to take action on loop if needed.
        rcv_message_handler_(message->header.id, f, nullptr, true);
      } else {
        if (message::tracker::Tracked(message->header)) {
          // We got another message identical to the last one we processed after
          // we got replies from all faces. This is a late propagation message
          // so we can not simply ignore if it is not a fire-and-forget message
          // and have to tell the sender not to wait on us (by forcing a loop).
          if (!message->header.is_fire_and_forget) {
            // We can send a single byte back with our header as we just want
            // the peer to stop waiting on us.

            // Should never fail.
            sendDatagramOnFace((const byte *)message, 1, f);
          } else {
            // Fire-and-forget message loops simply mean a Blink got the same
            // message more than once.
            rcv_message_handler_(message->header.id, f, nullptr, true);
          }

          continue;
        }

        // Set our parent face.
        parent_face_ = f;

        message::tracker::Track(message->header);

        if (rcv_message_handler_ != nullptr) {
          rcv_message_handler_(message->header.id, f, message->payload, false);
        }

        // Broadcast message.
        broadcast_message(message);
      }
    } else {
      // Got a reply.

      if (rcv_reply_handler_ != nullptr) {
        rcv_reply_handler_(message->header.id, f, message->payload);
      }

      // Mark face as not pending anymore.
      UNSET_BIT(sent_faces_, f);
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
  message->header.sequence = (message::tracker::LastSequence() % 7) + 1;
  message::tracker::Track(message->header);

  broadcast_message(message);

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