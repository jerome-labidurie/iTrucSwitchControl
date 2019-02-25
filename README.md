Simple Device to control an iPhone/iPad with a simple Button.

It uses the  [Switch Control accessibility](https://support.apple.com/en-us/HT201370) feature from iOS

# Hardware
* a Wemos LoLin32 (esp32 based board)

# Firmware

Tested dependencies :
* Arduino IDE 1.8.5
* esp32 by Espressif v1.0.0

Based on SampleHIDKeyboard by chegewara
* https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLETests/SampleHIDKeyboard.cpp

# Bibliography
* http://forum.fablab-lannion.org/viewtopic.php?f=5&p=5236
* http://atmakers.org/2016/10/ios-switch-control-on-a-budget-using-bluetooth-kbd/

# TODO
* 1 or more input button
* French keyboard
* configurable keycode (Serial itf?)
* deepsleep ?

