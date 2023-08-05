#include "USBHost_t36.h"
#include <cassert>
#include <array>

class DJControlMP3LE final : USBDriver {
public:

  using ControlCallback = void (*)(
    const char *name,
    uint8_t controlType,
    uint8_t controlIndex,
    uint8_t oldValue,
    uint8_t newValue);

  DJControlMP3LE(USBHost &host) {
    init();
  }

  void ctrl_transfer(Device_t *dev, uint32_t bmRequestType, uint32_t bRequest, uint32_t wValue, uint32_t wIndex, uint32_t wLength = 0) {
    setup_t setup;
    mk_setup(setup, bmRequestType, bRequest, wValue, wIndex, wLength);

    queue_Control_Transfer(dev, &setup, nullptr, this);
  }

  void set_callback(ControlCallback callback) {
    _callback = callback;
  }

  void send_init_sequence(Device_t *dev) {

    ctrl_transfer(dev, 0xc0, 0x2c, 0x0000, 0x0000, 2);  // => 4040
    ctrl_transfer(dev, 0xc0, 0x29, 0x0300, 0x0000, 2);  // => 0c0c
    ctrl_transfer(dev, 0xc0, 0x29, 0x0400, 0x0000, 2);  // => f2f2
    ctrl_transfer(dev, 0xc0, 0x29, 0x0500, 0x0000, 2);  // => eded
    ctrl_transfer(dev, 0xc0, 0x29, 0x0600, 0x0000, 2);  // => 7373
    ctrl_transfer(dev, 0xc0, 0x2c, 0x0000, 0x0000, 2);  // => 4040
    ctrl_transfer(dev, 0xc0, 0x2c, 0x0000, 0x0000, 2);  // => 4040
    ctrl_transfer(dev, 0xc0, 0x29, 0x0300, 0x0000, 2);  // => 0c0c
    ctrl_transfer(dev, 0xc0, 0x29, 0x0400, 0x0000, 2);  // => f2f2
    ctrl_transfer(dev, 0xc0, 0x29, 0x0500, 0x0000, 2);  // => eded
    ctrl_transfer(dev, 0xc0, 0x29, 0x0600, 0x0000, 2);  // => 7373
    ctrl_transfer(dev, 0xc0, 0x29, 0x0200, 0x0000, 2);  // => 0000
    ctrl_transfer(dev, 0x02, 0x01, 0x0000, 0x0082);     // =>    0
    ctrl_transfer(dev, 0x40, 0x27, 0x0000, 0x0000);     // =>    0
  }

  void update_state(Device_t *dev) {
    queue_Data_Transfer(_rx_pipe, _current_state.data(), _current_state.size(), this);
  }

  void on_state_update() {
    int controlIndex = 0;

    for (const auto &mapping : Mappings) {
      auto oldVal = _old_state[mapping.byteOffset] & mapping.byteMask;
      auto newVal = _current_state[mapping.byteOffset] & mapping.byteMask;

      if (mapping.controlType == 0) {
        oldVal = (int)oldVal > 0;
        newVal = (int)newVal > 0;
      }

      if (newVal != oldVal) {
        Serial.printf("State changed: %s from %d to %d\n", mapping.name, oldVal, newVal);

        if (_callback) {
          _callback(mapping.name, mapping.controlType, controlIndex, oldVal, newVal);
        }
      }

      controlIndex++;
    }

    _old_state = _current_state;
  }

  static void on_rx_pipe_transfer(const Transfer_t *transfer) {
    ((DJControlMP3LE *)transfer->driver)->on_state_update();
  }

  bool claim(Device_t *dev, int type, const uint8_t *descriptors, uint32_t len) override {

    if (type != 0) return false;

    if (dev->idVendor == 0x06f8 && dev->idProduct == 0xb105) {
      Serial.print("Sending init sequence...");
      send_init_sequence(dev);
      Serial.print("Done\n");

      Serial.print("Creating rx pipe...");
      _rx_pipe = new_Pipe(dev, 2, 1, 1, 64);

      if (!_rx_pipe) {
        Serial.println("new_Pipe() failed");
        return false;
      }

      _rx_pipe->callback_function = on_rx_pipe_transfer;

      Serial.print("Done\n");

      return true;
    }

    return false;
  }

  void Task() override {
    update_state(device);
  }

  void control(const Transfer_t *transfer) override {}

  void disconnect() override {}

  void init() {
    Serial.begin(9600);
    Serial.println("Contributing driver memory...");

    contribute_Pipes(_pipes, sizeof(_pipes) / sizeof(Pipe_t));
    contribute_Transfers(_transfers, sizeof(_transfers) / sizeof(Transfer_t));
    driver_ready_for_device(this);
  }

private:

  Pipe_t _pipes[2] __attribute__((aligned(32)));

  //Reserve ~15 ctrl transfers for init sequence with each 3 transfer blocks
  Transfer_t _transfers[15 * 3] __attribute__((aligned(32)));

  Pipe_t *_rx_pipe{ nullptr };

  using device_state = std::array<unsigned char, 38>;

  device_state _current_state{};
  device_state _old_state{};

  ControlCallback _callback{ nullptr };

  struct control_mapping {
    const char *name;
    uint8_t byteOffset;
    uint8_t byteMask;
    uint8_t controlType;
  };

  static constexpr control_mapping Mappings[] = {
    { "PitchReset_A", 4, 0x80, 0 },
    { "PitchBendMinus_A", 0, 0x02, 0 },
    { "PitchBendPlus_A", 0, 0x04, 0 },
    { "Sync_A", 4, 0x20, 0 },
    { "Shift_A", 0, 0x01, 0 },
    { "Shifted_A", 3, 0x10, 0 },
    { "N1_A", 4, 0x40, 0 },
    { "N2_A", 0, 0x10, 0 },
    { "N3_A", 0, 0x20, 0 },
    { "N4_A", 0, 0x40, 0 },
    { "N5_A", 5, 0x01, 0 },
    { "N6_A", 5, 0x02, 0 },
    { "N7_A", 5, 0x04, 0 },
    { "N8_A", 5, 0x08, 0 },
    { "RWD_A", 0, 0x08, 0 },
    { "FWD_A", 0, 0x80, 0 },
    { "CUE_A", 1, 0x02, 0 },
    { "Play_A", 1, 0x04, 0 },
    { "Listen_A", 1, 0x01, 0 },
    { "Load_A", 1, 0x08, 0 },
    { "PitchReset_B", 4, 0x02, 0 },
    { "PitchBendMinus_B", 3, 0x02, 0 },
    { "PitchBendPlus_B", 3, 0x04, 0 },
    { "Sync_B", 4, 0x08, 0 },
    { "Shift_B", 3, 0x01, 0 },
    { "Shifted_B", 3, 0x20, 0 },
    { "N1_B", 4, 0x04, 0 },
    { "N2_B", 2, 0x10, 0 },
    { "N3_B", 2, 0x20, 0 },
    { "N4_B", 2, 0x40, 0 },
    { "N5_B", 5, 0x10, 0 },
    { "N6_B", 5, 0x20, 0 },
    { "N7_B", 5, 0x40, 0 },
    { "N8_B", 5, 0x80, 0 },
    { "RWD_B", 3, 0x08, 0 },
    { "FWD_B", 2, 0x80, 0 },
    { "CUE_B", 2, 0x02, 0 },
    { "Play_B", 2, 0x04, 0 },
    { "Listen_B", 2, 0x01, 0 },
    { "Load_B", 2, 0x08, 0 },
    { "Vinyl", 4, 0x10, 0 },
    { "Magic", 4, 0x01, 0 },
    { "Up", 1, 0x10, 0 },
    { "Down", 1, 0x80, 0 },
    { "Folders", 1, 0x20, 0 },
    { "Files", 1, 0x40, 0 },

    // Dials and sliders, range 0x00 to 0xff

    { "Treble_A", 7, 0xff, 1 },
    { "Medium_A", 8, 0xff, 1 },
    { "Bass_A", 9, 0xff, 1 },
    { "Vol_A", 6, 0xff, 1 },
    { "Treble_B", 12, 0xff, 1 },
    { "Medium_B", 13, 0xff, 1 },
    { "Bass_B", 14, 0xff, 1 },
    { "Vol_B", 11, 0xff, 1 },
    { "XFader", 10, 0xff, 1 },

    // Jog-Dials, rotary encoders, range 0x00 to 0xff with wrap-around

    { "Jog_A", 15, 0xff, 2 },
    { "Pitch_A", 17, 0xff, 2 },
    { "Jog_B", 16, 0xff, 2 },
    { "Pitch_B", 18, 0xff, 2 }
  };
};

USBHost myusb;
USBHub hub1(myusb);
DJControlMP3LE driver(myusb);

constexpr int MidiChannel = 1;

void setup() {

  if (CrashReport) {
    Serial.print(CrashReport);
  }

  driver.set_callback(
    [](const char *control, uint8_t controlType, uint8_t controlIndex, uint8_t oldState, uint8_t newState) {
      if (controlType == 0) {

        auto note = controlIndex;  //TODO: Custom MIDI mapping?

        if (newState == 1) {
          Serial.printf("MIDI: Note %d ON\n", note);
          usbMIDI.sendNoteOn(note, 100, MidiChannel);
        } else {
          Serial.printf("MIDI: Note %d OFF\n", note);
          usbMIDI.sendNoteOff(note, 0, MidiChannel);
        }

      } else {
        //MIDI can only represent 0-128
        newState /= 2;

        Serial.printf("MIDI: Control change: %d -> %u\n", controlIndex, newState);
        usbMIDI.sendControlChange(controlIndex, newState, 1);
      }
    });

  Serial.println("Starting usb host...");
  myusb.begin();
}

void loop() {
  myusb.Task();

  // MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) {}
}
