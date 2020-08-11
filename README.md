# Broadcast Messages for the Move38 Blinks Platform.

https://www.move38.com

This code implements sending message broadcasts for all connected Blinks. Messages can either be sent and replied to or can be fire-and-forget (no replies). The code uses guaranted delivery datagrams to make sure messages are fully propagated.

# Usage.

The API is pretty simple:

* Call broadcast::manager::Process() unconditionaly at the top of loop().
* Call broadcast::manager::Send() to broadcast a message.
* Call broadcast::manager::Receive() to receive a reply for a message (fire-and-forget messages have no reply).

See the message.h file for the Message API.
