#ifndef BROADCAST_CONFIG_H_
#define BROADCAST_CONFIG_H_

// Maximum allowed payload size.
#define BROADCAST_MESSAGE_PAYLOAD_BYTES 15

// Enable message handler support. Allows sending messages that can be
// externally tracked. This is just to test this specific code path.
#define BROADCAST_ENABLE_MESSAGE_HANDLER

// Disable message reply support. Saves a lot of storage space for programs that
// only need fire and forget messages.
//#define BROADCAST_DISABLE_REPLIES

#endif