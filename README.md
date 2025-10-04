# zTank
Tank survey, embedded sofware on dedicated PCB using ESP32C6 µC

# Description

This repository contain all the development for an electronic device that will monitor a water tank (such as rain collect tank) using zigbee to transmit data to a coordinator. It contains: 
 * A PCB design [PCB design](PCB/README.md)
 * An enclosure design to be printed in 3D [Enclosure design](3Dprint/README.md)
 * Source code of the embedded software on the board (in an ESP32C6 µC)
 * An External converter for Zigbee2MQTT service [external converter](external_converters)

# Principle

The board will collect data from various sensors surch as:
 * Upstream static pressure (pump side)
 * Downstream static pressure (home side) - difference between upstream and downstream static pressure is used to estimate filters clogging
 * Water level sensor, to estimate the available water volume in the tank
 * water pulse meter to count the consumption in the home
 * Additionally on board temperature sensor could be monitor

 ![zTank connections](<img/zTank connections.png>)

 The 3 first sensors shall be 4-20mA sensor directly connected to the board. The water counter shall be a dry contact meter, connected to the board

 All these sensors have a calibration factor that can be updated directly form Zigbee2MQTT GUI. To avoid losing calibration, the device itself will keep updated theses factor in its Non Volatile Storage.

![z2m settings (specific)](<img/zTank z2m settings specific.png>)

A 4-20mA input is available for any other futher sensor requirement. Adding a sensor will require to :
 * Update the embedded software
 * Update the external converter on Zigbee2MQTT side

## Usage

  * Short press button to join the zigbee network (coordinator shall grant pairing) -> LED shall flash, until network is joined, it then shutoff
  * Long press button to leave network. When the LED is fixed on, the device is not connected to the network neither is trying to join

## Sensor selection

### Upstream and downstream static pressure

  Regular pressure transmitter are not so expensive and efficient for that purpose. These 2 sensors shall have a measured range according to installation pressure. In regular network, pressure is limited to 3 bars, so selecting a sensor 0-5 bars seems a good choice. If the upper value is higher, the resolution of the measurement will be less as the ADC converter resolution is fixed.

  The shunt to convert current to tension for the ADC converter is 133R, and Imax for the sensor @max range will be 20mA so Umax for ADS1115 will be 2.66V (U=R*I). 5 bars = 500 kPa, so the calibration factor shall be 500 kPa / 2.66V = 187.97 kPa/V. Of course this factor shall be adjusted by measuring at the same location of one sensor the pressure, and correcting the reading of the sensor, but this short computation can help to start setting up calibration factor

### Water level

  Whatever technology of sensor could be used as soon as its output is 4-20mA current. Tests have been done using submerged differential pressure sensor, with the balance pipe to get the atmospheric pressure that run with the electrical cable, they worked properly. The range of this sensor shall be in accordance with the depth of the tank. The output value of Zigbee2MQTT will be a % of depth (meaning 100% when tank is full). Depending on the shape and cross section of the tank, this could be converted to a volume directly in the Z2M external converter, or outside Z2M in another software.

  The shunt to convert current to tension for the ADC converter is 133R, and Imax for the sensor @max range will be 20mA so Umax for ADS1115 will be 2.66V (U=R*I). fFr 100% the calibration factor shall be 100% / 2.66V = 37.594 %/V. Of course this factor shall be adjusted when tank is full to read 100% in the Zigbee2MQTT output.

### Water pulse counter

  The water meter shall send a pulse each XX L, whatever XX is. It is expected to have a dry contact that switch on and off each pulse. Hardware debouncer has been implemented for a particular counter, measuring bouncing using an oscilloscope. Computation of this RC filter has been provided here:
  [Spreadsheet](<PCB/2024-06-29 Comput.ods>)
    
  The kFactor to be set up in Zigbee2MQTT is only the L per pulse of the counter.

## Zigbee2MQTT integration

### External converter

External converter shall be loaded according to Zigbee2MQTT instructions:

[Zigbee2MQTT external converters](https://www.zigbee2mqtt.io/advanced/more/external_converters.html)

Restart Zigbee2MQTT `Webpage > Settings > Tools > Restart Zigbee2MQTT`

### Pairing zTank device

If the LED is fixed on, short press one time on the button, this will put the device in pairing mode.
Device is in Pairing Mode when the LED flahes quickly. 

Put Coordinator (or other device) in pairing mode using Zigbee2MQTT GUI (antenna symbol on the upper right of the screen with mention `All`)

After a short time, LED shall stop flashing. When LED is off, device is paired. You can check on Zigbee2MQTT device list

### Device icon

To add an icon in Zigbee2MQTT GUI, create a folder named `device_icon` in the root of Zigbee2MQTT docker installation (folder containing `configuration.yml`). In that foler, put the `zTank_icon.png` file contained in the `img` directory of this repository.

On Zigbee2MQTT GUI, go to `Devices` list and click on the device. Go to `Settings` tab and scroll down to the `icon` box.
Put `device_icons/zTank_icon.png` in that box. Icon shall be visible on Zigbee2MQTT GUI.

### Reporting

To develop*******************************************************************

## Build Instructions

The embedded software required `esp-ash-components`:

```
git submodule add https://github.com/akira215/esp-ash-components.git components
```

Create a partition.csv for zigbee and configure menuconfig with custom partition table (shall be done by default)

## Configuration

Configuration is available in `menuconfig` in the `Project setting` section:

### Pin Layout

  This section refer to hardware connection of LED button and volume meter. <span style="color:red">**These settings shall not be changed unless new hardware is developped.**</span>

![pin layout](<img/zTank config pin layout.png>)


### Zigbee endpoints

  This section refer to the endpoint for each sensor of the device. Any change in this number require to update the external converter accordingly

![Zigbee endpoints](<img/zTank config zigbee endpoints.png>)

  And in Zigbee2MQTT external converter:

![External converter](<img/zTank ext converter zigbee endpoints.png>)

### Zigbee device

  Zigbee stack has been sized doing extensive tests. <span style="color:red">**This setting shall not be changed.**</span>

![Zigbee device](<img/zTank config zigbee stack.png>)

### ADS Pin Layout

  This section refer to hardware connection of ADS1115 converter to µC pin. <span style="color:red">**These settings shall not be changed unless new hardware is developped.**</span>

![ADS pin layout](<img/zTank config ADS pin layout.png>)

### ADS Converter config

  The first 3 inputs refer to the channel in which each sensor is connected. Take care that channel are not implemented on the connector in a logical order, so please refer to pictures that describe which pins of the connector is connected to which ADC channel (and polarity for sensors)

  The delay between 2 queries of ADS is the update rate of the measurement. Between 2 measurement, ADS is put in sleep to optimize consumption. Take care that driver will trigger one conversion for each channel, so there is a minimal time between 2 queries, depending on the resolution set for the ADS.

  The delay between 2 queries of temperature is the update rate of the measurement of the internal µC temperature sensor.


![ADS converter config](<img/zTank config ADS converter config.png>)

## Current Status

Still work in progress, but is completely running. All bugs 
could be reported to try to improve it.

## PCB design

[PCB design](PCB/README.md)

## Enclosure design design

[Enclosure design](3Dprint/README.md)

## Caveats

### To be developped

OTA update


## Usage and Examples



## Changes

### New in version 0.0.0


## Credits

 * [Zigbee2MQTT](https://github.com/Koenkk/zigbee2mqtt)

