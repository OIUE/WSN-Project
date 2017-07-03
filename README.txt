Sender and receiver can pair with two modes:
Default is by the best RSSI (nearest sender/receiver).
If you want to pair by a fixed mote id, compile the sender code with a defined >PAIR_BY_ID X<, with X being the desired receiver id.

Connection with the sink is implemented in sink_conn.h

Maybe change the contiki path in the Makefile
