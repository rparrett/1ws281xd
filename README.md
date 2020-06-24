1ws281xd
========

A daemon for running a single ws281x led on a raspberry pi.

Controlled by simple text protocol via a fifo.

`echo "0040:FF00FF" > fifo` blinks the led magenta every 64 milliseconds (40 hex).
`echo "0000:00FFFF" > fifo` displays a solid cyan.
`echo "0000:000000" > fifo` turns the led off.

Handy for motionEyeOS.

Hardware
========

See https://github.com/jgarff/rpi_ws281x
