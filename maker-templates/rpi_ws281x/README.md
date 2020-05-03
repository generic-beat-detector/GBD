## Background

These template programs demonstrate use of GBD with Jeremy Garff's [*rpi_ws281x*](https://github.com/jgarff/rpi_ws281x) project. They build upon `../gbd-text-display.c` and assume local `gbdserver` shared memory IPC. If everything goes well, then:
* `simple` will toggle 125 LEDs (of a WS2812B) between RGB colors upon `gbdserver` kickdrum beat detection
* `segments` will mirror the [*GBD Youtube*](https://youtu.be/1wmrO51TZqA) OpenGL emulation -- but only for kickdrum beats and "bassline" sounds

The main advantage of controlling an LED strip directly from the RPi running `gbdserver` -- as opposed to, say, over WiFi via ESP8266/ESP32 -- is that jitter is practically eliminated.

## Setup

### PWM/DMA

By default, the template programs use PWM0 and DMA channel 10. According to [*RPi PWM Limitation*](https://github.com/jgarff/rpi_ws281x#pwm), use of [*rpi_ws281x*](https://github.com/jgarff/rpi_ws281x)'s PWM conflicts with the usage of the RPi's onboard audio. But this restriction doesn't really matter for GBD setups since RPi's onboard audio should not be used for GBD operation anyway. See [*GBD Setups for RPi*](https://github.com/generic-beat-detector/GBD/wiki/The-GBD-IoT-Framework#gbd-setups-for-the-raspberry-pi).

### LED Strip Hookup

These template programs have been tested on a Raspberry Pi 3B with a WS2812B strip. The data pin for the WS2812B was connected to the default RPi GPIO18.

## Download and Build

* Clone [*rpi_ws281x*](https://github.com/jgarff/rpi_ws281x) 

      $ git clone https://github.com/jgarff/rpi_ws281x/

* Install Scons -- according to [*build instructions*](https://github.com/jgarff/rpi_ws281x#build), i.e:
		
      $ apt-get install scons

* Copy `main.c` and `gbd.h` from the respective directory (i.e. `simple`, `segments`, etc) to `rpi_ws281x/` -- replacing the original `rpi_ws281x/main.c`. Then build:
	
      $ cd rpi_ws281x	
      $ scons

This will result in an executable file named `rpi_ws281x/test`. 

## Runtime

### Execution

Simply execute `rpi_ws281x/test` with:

	$ sudo ./test 
	BassBeat (0)
	BassBeat (1)
	BassBeat (2)
	BassBeat (3)
	BassBeat (4)

... where the `BassBeat (N)`s will appear as `gbdserver` detects bass beats in the audio stream.


* __IMPORTANT NOTE:__ 

	Since the `test` program has to be executed with `sudo` priviledges, it is important to note that if `/dev/shm/gbd` does not exist, then `test` will create it with `root:root` ownership. This will require `gbdserver` to either:
	* be executed with `sudo`, or 
	* to have already created `/dev/shm/gbd`  
	
	... since `gbdserver` creates the SHM file with default ownership e.g. `pi:pi`

	In short, if `gbdserver` reports

          init:1213:: Permission denied

	or if the GBD ALSA PCM plugin complains with:

          gbdclient.c:gbdclient_transfer:165:: WARNING!! GBD PCM cmd write failed!

	then it is likely that `gbdserver` does not have sufficient priviledges to write to `/dev/shm/gbd`

### System Metrics

CPU and memory usage can be inspected via:

    pi@raspberrypi:~ $ PIDLIST=$(for i in `pidof gbdserver` ; do echo -n "$i, "; done ; echo `pidof test`)
    pi@raspberrypi:~ $ top -d .4 -p $PIDLIST

On RPi 3B, CPU usage by `test` avaraged at around 6% while the signal analysis by `gbdserver` consumes about 5%. 

