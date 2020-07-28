#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <blinklib.h>

#include "payload_bytes.h"

// Set this to the number of bytes you need to send as payload. Anything up to
// 15 bytes is fair game.
#ifndef MESSAGE_PAYLOAD_BYTES
#error You must define MESSAGE_PAYLOAD_BYTES
#endif

// This should not be changed unless you need a different header size.
#define MESSAGE_HEADER_BYTES 2

#define MESSAGE_DATA_BYTES MESSAGE_PAYLOAD_BYTES + MESSAGE_HEADER_BYTES

// Make sure we do not try to use more data than we can.
#if MESSAGE_DATA_BYTES > IR_DATAGRAM_LEN
#error "MESSAGE_DATA_BYTES must not be greater than IR_DATAGRAM_LEN"
#endif

// Message id 255 is reserved to indicate an invalid message.
#define MESSAGE_INVALID 0

namespace broadcast {

namespace message {

// Message is just an array of bytes.
typedef byte Message[MESSAGE_DATA_BYTES];

// Sets the message with the given parameters. If fire_and_forget is true,
// there will be no reply generated to this message, eliminating the roundtrip
// time.
void Set(Message message, byte id, byte sequence, const byte* payload,
         bool is_reply, bool fire_and_forget = false);

// Sets the message with the given data buffer.
void Set(Message message, const byte* data);

// Returns the message ID.
byte ID(const Message message);

// Returns the message sequence number.
byte Sequence(const Message message);

// Immutable and mutable accessors to the message payload.
const byte* Payload(const Message message);
byte* MutablePayload(Message message);

// True if message is a reply message.
bool IsReply(const Message message);

// True if message is fire-and-forget (no reply will be generated).
bool IsFireAndForget(const Message message);

}  // namespace message

}  // namespace broadcast

#endif  // MESSAGE_H_
