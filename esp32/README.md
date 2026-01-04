# Parts Required 
- [ESP32](https://www.amazon.com/dp/B0D8T53CQ5?ref=ppx_yo2ov_dt_b_fed_asin_title).
- [Thermal Printer](https://www.amazon.com/dp/B09YGVPPWV).
- [Buck Converter](https://www.amazon.com/UCTRONICS-Converter-Transformer-Voltage-Regulator/dp/B07XXWQ49N).
- [Arcade Button](https://www.amazon.com/dp/B07XYV5NNP)
- Power Supply. I used 9v 3a.
- 6mm diameter x 1 mm width magnets (for box v3).
- Misc. Wires

# Wiring Directions
All you need is one power rail and one ground rail wired in a clear manner, positive out of the buck converter to one, 
ground of the buck converter to the other. The power wires of the printer are connected to these rails as well as the VIN
pin of the ESP32 to power and it's ground to the rail. The wires that depend on specific pins are configured as follows:

- Printer TX to pin 25.
- Printer RX to pin 26.
- Button press terminal to pin 19.
- Button positive LED to pin 18.
- Button negative LED to pin 5.

If you are using the parts linked above, physical descriptions of the relevant wires/terminals are in [globals.h](src/globals.h).

# Usage Instructions
## Startup
When the box is plugged in, it will try to connect to the internet. If it succeeds, it will blink 3 times. If it fails,
it will open up a WiFi network for configuring settings. You can 

## Main Usage
- Press and release button: print the selected mini crossword.
- Press and hold button for 5 seconds: open up WiFi network for configuration settings.

## Wifi Configuration
- Set WiFi network and password
- Add NYT-S token (see section)
- Set a time for automatic printing
- Switch between NYT Mini and Puzzmo Mini

### NYT-S Token
- In order to use the NYT mini, you need to provide an NYT-S cookie. You can aquire it by inspecting element 
on the NYT Mini site, going to storage, then cookies. And there should be an NYT-S cookie which is a base-64 string.
Make sure you just provide the string without any quotes or "NYT-S" prepended to it.
