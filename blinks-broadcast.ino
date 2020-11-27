#include <shared/blinkbios_shared_functions.h>
#include <string.h>

#include "manager.h"
#include "message.h"
#include "src/blinks-debug/debug.h"

#define MESSAGE_COUNT_BLINKS 1
#define MESSAGE_REPORT_BLINKS_COUNT 2

static byte message_payload_[] = {0, 1, 2,  3,  4,  5,  6, 7,
                                  8, 9, 10, 11, 12, 13, 14};

byte blinks_count = 0;

void rcv_message_handler(byte message_id, byte src_face, byte* payload,
                         bool loop) {
  if (loop) {
    // On a loop, we just want to turn the face led off.
    setColorOnFace(OFF, src_face);

    return;
  }

  // Reset all leds.
  setColor(OFF);

  if (message_id == MESSAGE_REPORT_BLINKS_COUNT) {
    blinks_count = *payload;

    setColorOnFace(ORANGE, src_face);

    LOGF("Number of blinks (reported): ");
    LOGLN(payload[0]);

    return;
  }

  // We do not need to do anything. Just change our color to
  // note we forwarded a message.
  for (byte i = 0; i < BROADCAST_MESSAGE_PAYLOAD_BYTES; ++i) {
    if (message_payload_[i] != payload[i]) {
      BLINKBIOS_ABEND_VECTOR(1);
    }
  }

  setColorOnFace(CYAN, src_face);

  // Silence unused variable warnings.
  (void)message_id;
}

byte fwd_message_handler(byte message_id, byte src_face, byte dst_face,
                         byte* payload) {
  if (message_id == MESSAGE_COUNT_BLINKS) {
    for (byte i = 0; i < BROADCAST_MESSAGE_PAYLOAD_BYTES; ++i) {
      if (message_payload_[i] != payload[i]) {
        BLINKBIOS_ABEND_VECTOR(1);
      }
    }

    setColorOnFace(CYAN, dst_face);
  } else {
    setColorOnFace(ORANGE, dst_face);
  }

  (void)src_face;

  return BROADCAST_MESSAGE_PAYLOAD_BYTES;
}

byte sum = 1;

void rcv_reply_handler(byte message_id, byte src_face, const byte* payload) {
  if (message_id == MESSAGE_COUNT_BLINKS) {
    // Add the amount we just got from a neighbor to our local sum.
    sum += payload[0];

    for (byte i = 1; i < BROADCAST_MESSAGE_PAYLOAD_BYTES; ++i) {
      if (payload[i] != 0) {
        BLINKBIOS_ABEND_VECTOR(2);
      }
    }
  }

  setColorOnFace(OFF, src_face);
}

byte fwd_reply_handler(byte message_id, byte dst_face, byte* payload) {
  if (message_id == MESSAGE_COUNT_BLINKS) {
    // All faces reported. Set payload to our sum.
    payload[0] = sum;

    // Reset local sum.
    sum = 1;

    for (byte i = 1; i < BROADCAST_MESSAGE_PAYLOAD_BYTES; ++i) {
      if (payload[i] != 0) {
        BLINKBIOS_ABEND_VECTOR(3);
      }
    }
  }

  if (dst_face != FACE_COUNT) {
    setColorOnFace(OFF, dst_face);
  }

  return BROADCAST_MESSAGE_PAYLOAD_BYTES;
}

broadcast::Message count_blinks;
broadcast::Message report_blinks;

void setup() {
  broadcast::message::Initialize(&count_blinks, MESSAGE_COUNT_BLINKS, false);
  memcpy(count_blinks.payload, message_payload_,
         BROADCAST_MESSAGE_PAYLOAD_BYTES);

  broadcast::message::Initialize(&report_blinks, MESSAGE_REPORT_BLINKS_COUNT,
                                 true);

  broadcast::manager::Setup(rcv_message_handler, fwd_message_handler,
                            rcv_reply_handler, fwd_reply_handler);
}

bool reported_blinks = false;
byte step = 0;
broadcast::Message result;
byte count = 0;

void loop() {
  broadcast::manager::Process();

  if (buttonSingleClicked()) {
    step = 1;
    count = 1;
  }

  switch (step) {
    case 0:
      return;
    case 1:
      if (!broadcast::manager::Send(&count_blinks)) return;
      break;
    case 2:
      if (!broadcast::manager::Receive(&result)) return;
      break;
    case 3:
      LOGF("Number of blinks (counted): ");
      LOGLN(result.payload[0]);
      break;
    case 4:
      report_blinks.payload[0] = result.payload[0];
      if (!broadcast::manager::Send(&report_blinks)) return;
      break;
  }

  step++;

  if (step > 4) {
    count++;
    if (count > 3) {
      step = 0;
      count = 0;
    } else {
      step = 1;
    }
  }
}
