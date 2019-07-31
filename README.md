# BleController
### Bluetooth controller using Adafruit Bluefruit Feather 32u4 Bluefruit LE

This project uses an [Adafruit Feather 32u4 Bluefruit LE](https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le) to act as a HID mouse for an android tablet.  For movement input it uses the [Adafruit LIS3DH Triple-Axis Accelerometer](https://learn.adafruit.com/adafruit-lis3dh-triple-axis-accelerometer-breakout) attached to an ALS patients foot.  There are two simple arcade buttons used for left click and a scroll toggle. 

## Project Purpose

Several years ago my mother was diagnosed with ALS.  Since then she has slowly lost the function of her limbs.  She loves the Android game Fishdom and at first she could play it normally.  As she lost function she could no longer use the touch screen to control the game.  At that time I made a wired version of this controller using an arcade joystick to control the mouse movement, and the same two buttons to act as a click and a scroll toggle.  As time when on she lost more function and the joystick became hard to use.  She still has pretty good movement of her foot when in her chair so I decided to try and use an accelerometer to control the mouse movement.  Once I got that working I wanted to make it wireless to the tablet so that she wouldn't have to stop playing to charge the tablet.  

The code by this time was getting a little large and I didn't want to break something while trying to implement a new feature or while tweking a setting so I figured it was time to finally learn Git.  The bulk of the code was written in the Arduino IDE, I then discovered VSCode and its Git integration and have starting using it for my development.  

With any luck this project may help another disabled person have some entertainment.  

## Wiring

Wiring is fairly simple.  You will need four wires connecting the Feather to the Accelerometer (two if you want to give the Accelerometer its own power supply) and four wires to connect the buttons to the Feather (two for each button).  I used a Cat5 cable between the main housing box (which has the buttons and the Feather) and the foot control box (which has the Accelerometer) because I had keystones I could fasten to the boxes and I wanted a quick way to disconnect the Accelerometer from the rest of the system.  

* Connect the SCL pin of the Feather to the SCL pin of the Accelerometer
* Connect the SCA pin of the Feather tot he SCA pin of the Accelerometer
* Connect the Vin pin of the Accelerometer to 3V or 5V
* Connect the Gnd pin of the Accelerometer to Ground
* Connect one lead of the main button to pin 5 on the Feather (this can be changed in code)
* Connect the other lead to Ground
* Connect one lead of the scroll toggle button to pin 12 on the Feather (this can be changed in code)
* Connect the other lead to Ground

![Wiring Diagram](images/wiring.png)

## Coding
### Concept
The Accelerometer is acting much like a d-pad.  I'm not concerned with the amount of movement or the specific angle, just that it is tilted left/right or up/down.  There also needs to be a resting position that the user can keep the Accelerometer at in order to stop cursor movement.  The user also needs a way simulate a finger drag.  This can be done with just the main button and moving the Accelerometer but it is slow.  To facilitate faster scrolling of long forums and web pages (Facebook) a scroll mode is implemented. While in this mode the cursor stops moving and the entire page is moved up/down or left/right with the movement of the Accelerometer. 

### Includes
In addition to the Adafruit libraries for the Feather and the Accelerometer I used the PushButton library from [here](https://github.com/kristianklein/PushButton).  This is a convenient library for debouncing and detecting clicks, double clicks, and holds.  I had (and still do) have a hard time getting the bluetooth to initialize and connect,  `BluefruitRoutines.h` comes from [this](https://github.com/cyborg5/iOS_switch_control) project and includes an `initializeBluefruit()` function that works wonderfully so its included here.  I would like to eventually integrate this into my main ino.  

### Defines
There are several `#define` statements I use to help fine tune the project.  As movement is limited and will continue to deteriorate I wanted an easy way to adjust cursor speed, deadzone size, etc.  

```c++
#define RANGE 7  //How far the mouse moves
#define RESPONSEDELAY 5  //How often the loop runs
#define AVERAGEFACTOR 20  //How many captures per movement check
#define CALIBRATIONFACTOR 200  //How many captures for calibration
#define DEADZONE 200  //How far before movement registered
```

The `RANGE` constant determines how far the mouse moves with each move command (essentially how fast the cursor moves).  `RESPONSEDELAY` is a little delay at the end of the loop before it reruns this affects how fast the program is overall (may not be strickly necessary at this point).  The `AVERAGEFACTOR` constant determines how many data points to get from the Accelerometer before checking for movement.  The Accelerometer is quite noisy so several captures are taken and averaged out before determining its location.  `CALIBRATIONFACTOR` is how many times we poll the Accelerometer to determine a resting position (see `calibration()` function).  Finally `DEADZONE` is a threshhold the Accelerometer must pass before we register a movement.  This is needed so that when the user is at rest the cursor will stay in one location instead of jumping around.  

### Global Objects and Variables
I tried to minimize the use of global objects and variables to avoid any unintended changes in the code.  The way the Arduino `loop()` works necessitates some though.

```c++
PushButton scrollButton(5);  //Scroll button
PushButton mainButton(12);  //Main button
Adafruit_LIS3DH lis = Adafruit_LIS3DH();  //Create acc object
int xCalibrated = 0;  //Base value for x
int yCalibrated = 0;  //Base value for y
bool bolScroll = false;  //Is scroll mode active
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);  //Bluetooth object
```

There are two buttons for the user to manipulate `scrollButton` and `mainbutton`.  `mainButton` acts like left click on a mouse, while `scrollButton` is used to toggle in and out of scroll mode.  A switch would also work well for `scrollButton` but my user doesn't have the coordination to manipulate a switch.  `lis` is the Accelerometer object.  This is used to get all the movement data from the Accelerometer.  The `xCalibrated` and `yCalibrated` variables are used to hold the resting (no cursor movement) Accelerometer values.  `bolScroll` keeps track of whether the user is in scroll mode or not.  Finally the `ble` object is created in `BluefruitRoutines.h` and it is used to actually send the commands to the tablet. 

### Functions
In order to facilitate easier future transitions as movement degrades and new ways of controlling the cursor have to be devised, I've tried to keep the code modular and use functions when I can.  

```c++
int averageX();
int averageY();
void calibrate();
int checkXmovement();
int checkYmovement();
String convertMovement(int x, int y);
```

The `averageX()` and `averageY()` functions pull data from the `lis` object multiple times (`AVERAGEFACTOR`) and then average that data to determine the Accelerometers position.  This is done to reduce the noise of the Accelerometer data. The `calibrate()` function pulls data from the `lis` object `CALIBRATIONFACTOR` number of times and averages that all together to determine a resting position for the sensor.  It is called during `setup()` and can be called again by pressing `mainButton` and `scrollbutton` simultaneously.  The user may want to recalibrate if the cursor starts to drift while in a resting position.  The `checkXmovement()` and `checkYmovement()` functions are called to determine if there is enough movement in the Accelerometer to warrant a cursor movement.  It does this by calling the `averageX()` or `averageY()` function to get the current Accelerometer position.  It then compares that to `xCalibrated` or `yCalibrated` (the resting position) and if the difference between the new location and the calibrated location is greater than `DEADZONE` it returns a 1 or -1 depending on the direction of the movement, or a 0 if no movement.  In the main `loop()` that returned value is then multiplied by `RANGE` to determine the amount of cursor movement.  Finally the `convertMovement()` function takes the amount of movement wanted for the cursor and converts it into a string to be used by the bluetooth object (`ble`) and sent to the tablet to make the cursor actually move.  

### `setup()`

There isn't a lot to do in the setup.  First the pins for the buttons are setup as pull up inputs.  The buttons are set to active low logic for the PushButton library.  The Accelerometer is started and its sensitivity range is set.  It is then calibrated and lastly the bluetooth object is initialized. 

```c++
//Assign button pins
pinMode(5, INPUT_PULLUP);
pinMode(12, INPUT_PULLUP);
//Set buttons to active low
scrollButton.setActiveLogic(LOW);
mainButton.setActiveLogic(LOW);
//Begin acc
lis.begin(0x18);  
//Set RANGE for Acc
lis.setRange(LIS3DH_RANGE_16_G);   // 2, 4, 8 or 16 G!
//Calibrate X and Y axis
calibrate();
//Start bluetooth
initializeBluefruit();
```

### `loop()`

The main `loop()` consists of severl steps.  First it checks the status of the buttons.  If one or more of them are pressed it does the corresponding action.  Then it checks for movement and if it finds any it moves the cursor.  Rinse and repeat.  

```c++
//Update buttons
scrollButton.update();
mainButton.update();

//Check for button events
if (scrollButton.isActive() && mainButton.isActive())  //Click both to calibrate
{
  calibrate();
}
else
{
  if (scrollButton.isClicked())  //Check if scroll mode change
  {
    bolScroll = !bolScroll;
  }
  if (mainButton.isActive())  //Check for main button
  {
    ble.println("AT+BleHidMouseButton=L");  //Press but don't release to allow for dragging
  }
  if (mainButton.isReleased())
  {
    ble.println("AT+BleHidMouseButton=0");  //Release the button
  }
}

//checkmovement functions return 0,1, or -1, then multiply by the RANGE
int xDistance = checkXmovement() * RANGE;
int yDistance = checkYmovement() * RANGE;

//If not zero move
if ((xDistance != 0) || (yDistance != 0))
{
  //If not in scroll mode
  if (bolScroll == false)
  {
    String distance = convertMovement(xDistance,yDistance);  //Convert movement to string
    ble.print("AT+BleHidMouseMove=");  //Scroll mouse
    ble.println(distance);
  }
  //If in scroll mode
  else
  {
    String distance = convertMovement(-yDistance/2,-xDistance/2); //Convert to string reversed for scroll
    ble.print("AT+BleHidMouseMove=0,0,");  //Scroll mouse
    ble.println(distance);
    delay(150);
  }
}
  
delay(RESPONSEDELAY);
```

After the button status is updated the code checks for both buttons being pressed and then if they aren't it checks each individually, this is done to prevent an accidental click or scroll mode change.  Depending on if the user presses and releases both in a timely fashion this doesn't alway work but most of the time it works fine.  In the future I would like to refine this.  The way the Feather sends bluetooth commands is via a string that starts with "AT+" then whatever it is you're trying to do.  Documentation on this can be found [here](https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le/at-commands).  I am also looking for a better way to accomplish this.  I've seen some programs use `ble.atcommand()` but I can't find documentation on how this differs from `ble.print()`.  Also the last bit of code you send via bluetooth should use the `ble.println()` command.  I had crashing and erratic behavior when I only used `ble.print()`.
