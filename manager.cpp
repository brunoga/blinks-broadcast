#include "manager.h"

#include <string.h>

#include "bits.h"
#include "datagram.h"
#include "debug.h"
#include "message.h"

namespace broadcast {

namespace manager {

static ReceiveMessageHandler rcv_message_handler_ = nullptr;
static ForwardMessageHandler fwd_message_handler_ = nullptr;
static ReceiveReplyHandler rcv_reply_handler_ = nullptr;
static ForwardReplyHandler fwd_reply_handler_ = nullptr;

static byte parent_face_ = FACE_COUNT;
static byte sent_faces_;

static MessageHeader last_message_header_;

static Message *result_;

static void send_reply(broadcast::Message *reply) {
  // Send a reply to our parent.

  // Set message as reply and clear payload.
  reply->header.is_reply = true;
  message::ClearPayload(reply);

  byte len = MESSAGE_DATA_BYTES - 1;
  if (fwd_reply_handler_ != nullptr) {
    len = fwd_reply_handler_(reply->header.id, reply->payload);
  }

  if (!datagram::Send(parent_face_, (const byte *)reply, len + 1)) {
    // Should never happen.
  }

  // Reset parent face.
  parent_face_ = FACE_COUNT;
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

    byte len = MESSAGE_DATA_BYTES - 1;
    if (fwd_message_handler_ != nullptr) {
      len = fwd_message_handler_(fwd_message.header.id, parent_face_, f,
                                 fwd_message.payload);
    };

    if (datagram::Send(f, (const byte *)&fwd_message, len + 1)) {
      // Mark this face as having data sent to it.
      SET_BIT(sent_faces_, f);
    }
  }

  if (message->header.is_fire_and_forget) {
    parent_face_ = FACE_COUNT;
    sent_faces_ = 0;

    return;
  }

  if (sent_faces_ == 0 && parent_face_ != FACE_COUNT) {
    // We did not send data to any faces and we have a parent, so we are most
    // likelly a leaf Blink. Send reply back.
    send_reply(message);
  }
}

void send_reply_or_set_result(Message *message) {
  if (sent_faces_ == 0) {
    // We are not waiting on any faces anymore.
    if (parent_face_ == FACE_COUNT) {
      // We do not have a parent, so surface the result here.
      if (fwd_reply_handler_ != nullptr) {
        // We can ignore the return value here as it is irrelevant.
        fwd_reply_handler_(message->header.id, message->payload);
      }

      result_ = message;
    } else {
      // This was the last face we were waiting on and we have a parent.
      // Send reply back.
      send_reply(message);
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

void Process() {
  datagram::Process();

  // Invalidate result as it most likelly is not the same data as when it was
  // received.
  result_ = nullptr;

  // Receive any pending data on all faces.

  FOREACH_FACE(f) {
    byte len;
    const byte *rcv_datagram = datagram::Receive(f, &len);

    if (rcv_datagram == nullptr) {
      // No data available. Nothing to do.
      continue;
    }

    // We are removing the constness here but this is fine in this case and it
    // is worth to avoid copies. Also, we might receive a message that is
    // smaller than Message but it is ok as the underlying buffer is of the
    // right size.
    broadcast::Message *message = (broadcast::Message *)rcv_datagram;

    if (!message->header.is_reply) {
      // Got a message.
      if (IS_BIT_SET(sent_faces_, f)) {
        // We already sent to this face, so this is a loop. Mark face as not
        // sent.
        UNSET_BIT(sent_faces_, f);

        send_reply_or_set_result(message);

        continue;
      }

      if (message->header.value == last_message_header_.value) {
        if (!message->header.is_fire_and_forget) {
          // We can send a single byte back with our header as we just want the
          // peer to stop waiting on us.
          if (!datagram::Send(f, (const byte *)message, 1)) {
            // Should never happen.
          }
        }

        continue;
      }

      // Set our parent face.
      parent_face_ = f;

      last_message_header_ = message->header;

      if (rcv_message_handler_ != nullptr) {
        rcv_message_handler_(message->header.id, message->payload);
      }

      // Broadcast message.
      broadcast_message(message);
    } else {
      // Got a reply.

      if (rcv_reply_handler_ != nullptr) {
        rcv_reply_handler_(message->header.id, message->payload);
      }

      // Mark face as not pending anymore.
      UNSET_BIT(sent_faces_, f);

      send_reply_or_set_result(message);
    }
  }
}

bool Send(broadcast::Message *message) {
  if (sent_faces_ != 0) return false;

  if (message == nullptr) return false;

  message->header.sequence = (last_message_header_.sequence % 7) + 1;

  last_message_header_ = message->header;

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

}  // namespace manager

}  // namespace broadcast