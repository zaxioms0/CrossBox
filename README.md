# CrossBox
A printer for the NYT mini crossword

Thanks for visiting! [People on Reddit](https://www.reddit.com/r/crossword/comments/1oy7tg7/i_made_a_box_to_print_the_nyt_mini/) seemed to think this was cool, so I'm putting it on Github if anyone
else wants to make it. This currently *does* work without an NYT Games subscription. Hopefully it stays that way :] 

# Parts
Here is a list of parts I used (not including wires and stuff)

I included Amazon Links to the exact models of some of the parts that I used. You can probably get them
for cheaper on Ali Express or similar - I am fortunate to have a robotics club that stocks these for me :]

- 12v to 5v buck converter
- Breadboard
- 12v 3a power supply
- [58mm thermal printer](https://www.amazon.com/Maikrt-Embedded-Microcontroller-Secondary-Development/dp/B09YGVPPWV)
- [WEMOS D1 Mini](https://www.amazon.com/Hosyond-Wireless-Internet-Development-Compatible/dp/B09SPYY61L)

# Setup
## Software
I use PlatformIO to handle dependencies. The only thing in software that needs to be changed are your
wifi SSID and password, and ROOT CA certificate in `src/creds.h`.

The way it works is that a button resets the microcontroller and it reconnects to the internet.
To be honest this is not a great design because it takes a long time to connect to the internet every time.
I did this because I have no idea how the JSON parser is allocating memory and I'm worried about using too much
if it leaks or something. I want to improve this in the future.

## Hardware
Besides 3D printing the models, the wiring is fairly straightforward. 
Here are the connections neccesary:
- Buck Converter -> Breadboard: positive goes to positive, negative to negative
- Breadboard Power Rails -> D1 Mini: Positive to 5v, negative to ground
- Printer Power -> Power Rails: positive to positive, negative to negative
- Printer TTL -> Breadboard: ground to ground, RX to GPIO 16, TX to GPIO 14
- Big Button -> Breadboard: positive to reset, negative to ground

Your power supply should supply 12v into the buck converter and at least 3 amps. 
If you have a power supply with 5 volts and 3 (or more) amps, the buck converter can be skipped.

# Disclaimer
I am fairly new to this whole electronics thing, lots of things can (and probably will) be improved.
In particular, I want to handle JSON parsing without a library for space reasons. 
Also, the box design right now is too tight, I plan to change it to use magnets for the connection in v3.

