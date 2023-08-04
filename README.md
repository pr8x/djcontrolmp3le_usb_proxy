## What is this?

The Hercules DJControl MP3 LE Controller is not compatible with modern versions of Windows or Mac anymore. This is due to an error in the USB endpoint descriptor sent by the device. To fix this I created some kind of USB "proxy" device using a Teensy 4.1 that communicates with the controller via its USB Host port and reinterprets its custom binary protocol into class-compliant MIDI. Conveniently, this removes the needs for installing any Hercules drivers on the host machine.

![Image](https://github.com/pr8x/djcontrolmp3le_usb_proxy/assets/4670166/51ed6309-ec8c-4355-80b9-cd6cc035ebd3)

## How to install 

- Follow the [official instructions](https://www.pjrc.com/teensy/td_download.html) for installing Teensyduino
- Clone this repro and open it in the Arduino IDE
- Make sure to select "USB Type: MIDI" (or optionally "USB Type: MIDI + Serial" for serial debug) in the Tools menu
- Compile and upload the sketch to your Teensy 4.1

## MIDI Mapping

Since the MIDI notes/controls sent by the proxy device do not follow any standard it will be required to create a custom mapping for it in your DJ software. There should be plenty of tutorials out there on how to do that with (for instance) rekordbox, VirtualDJ or Traktor PRO.

## Credits

- [djcontrol](https://github.com/foomatic/djcontrol) project for reverse-engineering the proprietary(?) usb communication protocol used by the controller. This project would've been much harder without this.
- PJRC/Teensy and particuarly its fantastic [USB Host Library](https://github.com/PaulStoffregen/USBHost_t36) library
