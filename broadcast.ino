#include "debug.h"
#include "manager.h"
#include "message.h"

#define MESSAGE_COUNT_BLINKS 1
#define MESSAGE_REPORT_BLINKS_COUNT 2

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
  displayColor = YELLOW;

  // Silence unused variable warnings.
  (void)payload;
  (void)message_id;
}

byte sum = 1;

void rcv_reply_handler(byte message_id, const byte* payload) {
  if (message_id == MESSAGE_COUNT_BLINKS) {
    // Add the amount we just got from a neighbor to our local sum.
    sum += payload[0];
  }

  displayColor = CYAN;
}

void fwd_reply_handler(byte message_id, byte* payload) {
  if (message_id == MESSAGE_COUNT_BLINKS) {
    // All faces reported. Set payload to our sum.
    payload[0] = sum;

    // Reset local sum.
    sum = 1;

    displayColor = BLUE;

    return;
  }

  displayColor = MAGENTA;
}

broadcast::Message count_blinks;
broadcast::Message report_blinks;

void setup() {
  broadcast::message::Initialize(&count_blinks, MESSAGE_COUNT_BLINKS, false);
  broadcast::message::Initialize(&report_blinks, MESSAGE_REPORT_BLINKS_COUNT,
                                 true);

  broadcast::manager::Setup(rcv_message_handler, nullptr, rcv_reply_handler,
                            fwd_reply_handler);
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
