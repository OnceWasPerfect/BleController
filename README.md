# BleController
### Bluetooth controller using Adafruit Bluefruit Feather 32u4

This project uses an [Adafruit Feather 32u4](https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le) to act as a HID mouse for an android tablet.  For movement input it uses the [Adafruit LIS3DH Triple-Axis Accelerometer](https://learn.adafruit.com/adafruit-lis3dh-triple-axis-accelerometer-breakout) attached to an ALS patients foot.  There are two simple arcade buttons used for left click and a scroll toggle. 

## Wiring
The Feather communicates with the Accelerometer via the 12C connection.

The buttons use two digital outputs from the Feather.
