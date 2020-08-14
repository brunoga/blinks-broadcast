#include <shared/blinkbios_shared_functions.h>
#include <string.h>

#include "debug.h"
#include "manager.h"
#include "message.h"

#define MESSAGE_COUNT_BLINKS 1
#define MESSAGE_REPORT_BLINKS_COUNT 2

static byte message_payload_[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

Color displayColor = OFF;

byte blinks_count = 0;

void rcv_message_handler(byte message_id, byte* payload) {
  if (message_id == MESSAGE_REPORT_BLINKS_COUNT) {
    blinks_count = *payload;

    displayColor = ORANGE;

    LOGF("Number of blinks (reported): ");
    LOGLN(payload[0]);

    return;
  }

  // We do not need to do anything. Just change our color to
  // note we forwarded a message.
  for (byte i = 0; i < MESSAGE_PAYLOAD_BYTES; ++i) {
    if (message_payload_[i] != payload[i]) {
      BLINKBIOS_ABEND_VECTOR(1);
    }
  }

  displayColor = YELLOW;

  // Silence unused variable warnings.
  (void)message_id;
}

byte fwd_message_handler(byte message_id, byte src_face, byte dst_face,
                         byte* payload) {
  if (message_id == MESSAGE_COUNT_BLINKS) {
    for (byte i = 0; i < MESSAGE_PAYLOAD_BYTES; ++i) {
      if (message_payload_[i] != payload[i]) {
        BLINKBIOS_ABEND_VECTOR(1);
      }
    }
  }

  return MESSAGE_PAYLOAD_BYTES;
}

byte sum = 1;

void rcv_reply_handler(byte message_id, const byte* payload) {
  if (message_id == MESSAGE_COUNT_BLINKS) {
    // Add the amount we just got from a neighbor to our local sum.
    sum += payload[0];

    for (byte i = 1; i < MESSAGE_PAYLOAD_BYTES; ++i) {
      if (payload[i] != 0) {
        BLINKBIOS_ABEND_VECTOR(2);
      }
    }
  }

  displayColor = CYAN;
}

byte fwd_reply_handler(byte message_id, byte* payload) {
  if (message_id == MESSAGE_COUNT_BLINKS) {
    // All faces reported. Set payload to our sum.
    payload[0] = sum;

    // Reset local sum.
    sum = 1;

    displayColor = BLUE;

    for (byte i = 1; i < MESSAGE_PAYLOAD_BYTES; ++i) {
      if (payload[i] != 0) {
        BLINKBIOS_ABEND_VECTOR(3);
      }
    }

    return MESSAGE_PAYLOAD_BYTES;
  }

  displayColor = MAGENTA;

  return MESSAGE_PAYLOAD_BYTES;
}

broadcast::Message count_blinks;
broadcast::Message report_blinks;

void setup() {
  broadcast::message::Initialize(&count_blinks, MESSAGE_COUNT_BLINKS, false);
  memcpy(count_blinks.payload, message_payload_, MESSAGE_PAYLOAD_BYTES);

  broadcast::message::Initialize(&report_blinks, MESSAGE_REPORT_BLINKS_COUNT,
                                 true);

  broadcast::manager::Setup(rcv_message_handler, fwd_message_handler,
                            rcv_reply_handler, fwd_reply_handler);
}

bool reported_blinks = false;

void loop() {
  broadcast::manager::Process();

  setColor(displayColor);

  if (buttonSingleClicked()) {
    reported_blinks = false;
    if (!broadcast::manager::Send(&count_blinks)) {
      displayColor = RED;
    }
  }

  broadcast::Message result;
  if (!broadcast::manager::Receive(&result)) return;

  displayColor = GREEN;

  LOGF("Number of blinks (counted): ");
  LOGLN(result.payload[0]);

  if (reported_blinks) return;

  report_blinks.payload[0] = result.payload[0];
  if (!broadcast::manager::Send(&report_blinks)) {
    displayColor = RED;
  }

  reported_blinks = true;
}
