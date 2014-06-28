# The USB configirable rubber ducky firmware.
Tries to be fully configurable throgh usb storage on the device.

## Usage
Flash the firware to the ducky.
The ducky will should now show up as a composite USB device AKA a dual device.
It should show up as a usb storage that is connected to the sd card, and a usb keyboard.

### When the device boots it will read
/config/keymap.map
A file that contains information about which keyboard key gives which character when pressed.

/config/speed.conf
A file that tells the ducky how fast to type.

/texts/main.txt
The text that the ducky should type.

These files will be pharsed and saved in to memmory.
After that the computer host will be given control over the SD card throgh the usb storage device.

If the /texts/main.txt existed and it was successfylly loaded the ducky will now type the contents of the file.
