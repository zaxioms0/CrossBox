# CrossBox
A printer for the NYT mini crossword

Thanks for visiting! [People on Reddit](https://www.reddit.com/r/crossword/comments/1oy7tg7/i_made_a_box_to_print_the_nyt_mini/) 
seemed to think this was cool, so I'm putting it on Github if anyone
else wants to make it. ~~This currently *does* work without an NYT Games subscription. Hopefully it stays that way :]~~ 
In order to use the NYT mini, you need to provide an NYT-S token which is availible to anyone with an NYT Games subscription. 
The boxes also can print the [Puzzmo Mini](https://www.puzzmo.com/today/)
if you do not have an NYT Games subscription. If you are interested in building this project for yourself,
I highly recommend using the version for the ESP32 as well as v3 of the box. 

The current version is in the `esp32` directory and all development going forward will be for it. Due to the changes to the NYT API 
and the lack of development, the `esp8266` version does not work and also will not be updated.
