# KnxForceDetector2Channel

ETS configurable force detector sensor based on the [Adafruit QT Py - SAMD21 Dev Board](https://www.adafruit.com/product/4600).

This sensor can be used to implement a presence detector for the bed to disable motion sensors.

![SAMD 21 board](/assets/images/Sensor.jpg "SAMD 21 board")

![Force sensor mounted on bed feet](/assets/images/ForceSensorMounted.jpg "Force sensor mounted on bed feet")

## Features

* 2 Force sensors
* One virtual summation sensor
* Percent group objects
* Switch group objects
* Switch group objects with delayed OFF
* Configurable in ETS

## Required hardware

* [Adafruit QT Py - SAMD21Dev Board](https://www.adafruit.com/product/4600)
* 2 pieces MD30-60 thin film force sensor
* 2 resistors 10kΩ
* [NanoBCU](https://shop.sirsydom.de/Busankoppler/NanoBCU-Standard.html), not tested, but also the Siemens BCU or other should work.
* button (used for the programming key)  

## Required software

* Visual Studio Code
* [PlatformioIO IDE](https://platformio.org) VS Code 
extension
* Arduino framework installed
* [CreateKnxProd](https://github.com/thelsing/CreateKnxProd) 
* ETS5 or ETS6

## Hardware construction

* Connect the 3,3V, GND to the BCU
* Connect the TX of the SAMD21 to the RX of the BCU
* Connect the RX of the SAMD21 to the TX of the BCU
* Connect one pin of the first 10kΩ to A0, the other pin to 3.3V
* Connect one pin of the second 10kΩ to A1, the other pin to 3.3V
* Connect one pin of the first force detector to A0, the other pin to GND
* Connect one pin of the first force detector to A1, the other pin to GND
* Connect the one pin of the button to A2, the other to GND

## Build and upload firmware

1) clone this repo including the git submodules

`git clone --recurse-submodules https://github.com/mgeramb/KnxForceDetector2Channel.git`

1) open the newly created folder KnxForceDetector2Channel in VS Code

1) Click on the platformio symbol on the left side

1) Connect the USB cable of the board to your computer and click on "Upload"

## Build forcedetector.knproj file

The forcedetector.knproj for the ETS application must be build with the [CreateKnxProd](https://github.com/thelsing/CreateKnxProd) tool.

Prerequirement is the installation of the [.NET Core 6 framework desktop framework](https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/runtime-desktop-6.0.3-windows-x64-installer) and ETS5 or ETS6 installed on the same computer.

After the framework installation download the CreateKnxProd tool from here https://github.com/thelsing/CreateKnxProd/releases.

Start the tool and load the [ForceDetector.xml](ForceDetector.xml) file of this repo in the tool. Click on `Export` to create the forcedetector.knproj file.

## ETS programming and commissioning

The ETS application provides 2 force channels and a virtual `Summe` channel which is calculate by adding the RAW values the channel `1` and `2`.
Also a virtual channel for OR joining of `1`, `2` and `Summe` is available to control your KNX devices.

### Steps for calibration:

1) Import the `forcedetector.knproj' created from the previous steps in the ETS product catalog.

2) Add the newly imported sensor application to your project

3) Connect the groups addresses needed for calibration
* Diagnose
* Druck RAW 1
* Druck Prozent 1
* Limit Setzten 1
* Untere Schwelle 1
* Obere Schwelle 1
* Druck RAW 2
* Druck Prozent 2
* Limit Setzten 2
* Untere Schwelle 2
* Obere Schwelle 2
* Limit Setzten Summe
You don't need to connect the GA's to any other things. We will use the ETS Group Monitor to calibrate the device.

4) Connect other groups addresses to connect your HW as you want

5) Place the force sensor where you want to measure the pressure, e.g. under the bed feets

6) Connect the BCU to the KNX bus

7) Press the which you have connected to the A2 pin to enter the address programming mode. The LED should be red now.

8) Program the sensor in ETS. The LED should be turned off

9) Open the 'group monitor' window and start the observation

10) Send 1 to the GA connected to the `Diagnose` object. The device will transfer now the measurement in the RAW objects. 

11) Send a value 10-20% below the RAW value to the GA connected to `Unter Schwelle X` objects (All values below this value will be interpreded as error). 

12) Burdon now the force sensors (e.g. lie down in the bed)

13) Send a value 10-20% above the RAW value to the GA connected to `Obere Schwelle X` objects (All values above this value will be interpreded as error)

14) Set the `Limit Setzten X` to a percent value which should force the switch of the `Schalten X` objects. 

**Note:** The values of `Untere Schwelle 1`, `Untere Schwelle 2`, `Limit Setzten 1`, `Limit Setzten 2`, `Limit Setzten Summe` will be persistent in the flash of the SAMD21. For this reason, the values must not be changed at runtime because the number of write cycles to the flash ared limited to ~10000. After this, the chip gets unusable.

# Credits

* [thelsing](https://github.com/thelsing) for the KNX library

* [sirsydom](https://shop.sirsydom.de) for the BCU

# License

KnxDistanceDoorFlowerSensor code is licensed under [MIT](LICENSE).