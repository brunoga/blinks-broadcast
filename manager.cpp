#include "manager.h"

#include <string.h>

#include "bits.h"

namespace broadcast {

namespace manager {

static ReceiveMessageHandler rcv_message_handler_ = nullptr;
static ForwardMessageHandler fwd_message_handler_ = nullptr;
static ReceiveReplyHandler rcv_reply_handler_ = nullptr;
static ForwardReplyHandler fwd_reply_handler_ = nullptr;

static byte parent_face_ = FACE_COUNT;
static byte sent_faces_ = 0;

static bool has_result_ = false;
static message::Message result_;

static bool readDatagram(byte f, byte* datagram) {
  if (!isDatagramReadyOnFace(f)) return false;

  if (getDatagramLengthOnFace(f) != MESSAGE_DATA_BYTES) {
    markDatagramReadOnFace(f);
    return false;
  }

  const byte* data = getDatagramOnFace(f);
  memcpy(datagram, data, MESSAGE_DATA_BYTES);

  markDatagramReadOnFace(f);

  return true;
}

static bool processReply(byte f, const message::Message reply) {
  if (rcv_reply_handler_) {
    rcv_reply_handler_(message::ID(reply), message::Payload(reply));
  }

  support::UnsetBit(&sent_faces_, f);

  if (sent_faces_ != 0) return false;

  return true;
}

static bool processMessage(byte f, message::Message message) {
  if (support::IsBitSet(sent_faces_, f)) {
    // We already forwarded the message to this face. Ignore it and stop
    // waiting for a reply on it.
    support::UnsetBit(&sent_faces_, f);

    return false;
  }

  // We received a new message. Set our parent and forward the message.
  parent_face_ = f;

  if (rcv_message_handler_) {
    rcv_message_handler_(message::ID(message),
                         message::MutablePayload(message));
  }

  return true;
}

static void sendReply(message::Message reply) {
  if (fwd_reply_handler_) {
    fwd_reply_handler_(message::ID(reply), message::MutablePayload(reply));
  }

  sendDatagramOnFace(reply, MESSAGE_DATA_BYTES, parent_face_);

  parent_face_ = FACE_COUNT;
}

static void sendMessage(const message::Message message) {
  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f)) continue;

    if (f == parent_face_) continue;

    message::Message fwd_message;
    message::Set(fwd_message, message::ID(message), message::Payload(message),
                 false);

    if (fwd_message_handler_) {
      fwd_message_handler_(message::ID(fwd_message), parent_face_, f,
                           message::MutablePayload(fwd_message));
    };

    sendDatagramOnFace(fwd_message, MESSAGE_DATA_BYTES, f);

    support::SetBit(&sent_faces_, f);
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

  message::Set(result_, MESSAGE_INVALID, nullptr, false);
}

void Process() {
  byte datagram[MESSAGE_DATA_BYTES];
  FOREACH_FACE(f) {
    if (!readDatagram(f, datagram)) continue;

    has_result_ = false;
    message::Set(result_, MESSAGE_INVALID, nullptr, false);

    // Got what appears to be a valid message. Parse it.
    broadcast::message::Message message;
    message::Set(message, datagram[0], &datagram[MESSAGE_HEADER_BYTES], false);

    if (message::IsReply(message)) {
      if (processReply(f, message)) {
        if (parent_face_ == FACE_COUNT) {
          if (fwd_reply_handler_) {
            fwd_reply_handler_(message::ID(message),
                               message::MutablePayload(message));
          }

          has_result_ = true;
          message::Set(result_, message::ID(message), message::Payload(message),
                       true);
          break;
        }

        sendReply(message);
      }
    } else {
      if (processMessage(f, message)) {
        sendMessage(message);
      }

      if (sent_faces_ != 0) continue;

      message::Message reply;
      message::Set(reply, message::ID(message), message::Payload(message),
                   true);

      sendReply(reply);
    }
  }
}

bool Send(const broadcast::message::Message message) {
  if (sent_faces_ != 0) return false;

  sendMessage(message);

  return true;
}

bool Receive(broadcast::message::Message result) {
  if (!has_result_) return false;

  message::Set(result, message::ID(result_), message::Payload(result_),
               message::IsReply(result_));

  has_result_ = false;

  return true;
}

}  // namespace manager

}  // namespace broadcast