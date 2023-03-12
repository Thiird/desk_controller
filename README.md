# Desk Controller

Desk controller is a gadget that connects to my pc and can:
- power on/off the pc
- reset the pc
- select the booting OS
- control a bias lighting system
- control a desk led strip


The power on/off functionality is achieved by shorting the two respective pc signals to pc ground via optocouplers.

The booting OS selection is done by interacting with GRUB via serial port.

The bias lighting is the most complex part of this whole system.
The setup consists of a WS2815 12V led strip mounted around my main monitor, which is controlled by the STM32C0 of the desk controller by bit-banging the RGB data.
The RGB data is computed by a Visual C++ application running on windows and sent to the desk controller via serial port.
The main challenge here was to make a fast enough algorithm to to compute the RGB data, for the bias lighting to be reactive I wanted to achieve a 30Hz refresh rate of the RGB data, which means updating the leds color at most every 0.33.. seconds.
Without getting into GPU shaders and such, the wastest way to capture a screenshot of the screen and calculate the average color for the sections of the screen around the border, was to use the Desktop Duplication Windows API.

I inted too use this feature only during gaming or watching films, which I only do on windows. A Linux-equivalent application can always be made at a later moment,