# Generic Beat Detector (GBD)

The *Generic Beat Detector (GBD)* strives to deliver an industrial strength library and IoT framework for realtime beat 
detection (and soon other DSP analysis operations) for live music streams. 

## Description

GBD primarily consists of a library for music DSP analysis, and support plugins for 
audio signal routing between hosts.
<p align="center">
<img src="https://github.com/generic-beat-detector/GBD/blob/master/docs/img/gbd-iot-framework.jpg" alt="GBD IoT Framework" width="760" height="250"/>
</p>

This GBD release (v0.20) offers low-latency and realtime beat detection for
live/playing music streams:

* Bass beat (e.g. kick drum) detection (below 200Hz)
* Detection of snare-like hits (between 3KHz and 10KHz)
* Detection of cymbals-like sounds (above 15KHz)

## Targeted Users

The GBD framework is primarily designed for the __IoT maker__ industry and community. The 
target is to supply __robust__, industrial-strength __realtime__ music 
beat detection (and soon other music DSP analysis) engines for makers 
working with __commodity hardware__ and developing setups or products similar to,
for example, the Philips Hue Disco framework.

## Operation

The Raspberry Pi computer is used as the platform of choice for the GBD core given the board's popularity with the IoT maker community. The *GBD core* (PCM routing server and DSP library) is GNU/Linux based. The GBD client and/or audio applications
may execute on a remote host which need not be a GNU/Linux system. This is discussed at greater depth in [*GBD Architecture*](https://github.com/generic-beat-detector/GBD/wiki/The-GBD-IoT-Framework#gbd-achitecture).

<p align="center">
<img src="https://github.com/generic-beat-detector/GBD/blob/master/docs/img/generalized-gbd-arch.jpg" alt="Generalized GBD Arch" width="430" height="250"/></p>

With realtime beat detection, as an audio stream is routed through the GBD framework, the 
GBD music DSP analysis library delivers beat counts to Linux POSIX SHM. 
The IoT maker can then use this information to synchronize the control of light bulbs/leds, or even an emulation such as in this [*Youtube video*](https://youtu.be/1wmrO51TZqA), to the beats of the music.


Bear in mind that while GBD's realtime beat detection is sturdy, it still requires some "compliance" on the part of the user
if it is to function in the most generic manner possible, i.e. across the variety of music genres. For the best results, the quality of the music should be that of professional studio CD recordings: preferably WAV uncompressed PCM or, at least, an encoding by an industry-standard audio format converter. For example, pirated `.mp3` downloads from the Internet (i.e. poorly encoded or transcoded formats) or amateur recordings are likely to result in disappointment. These issues are discussed more fully
[*here*](https://github.com/generic-beat-detector/GBD/wiki#features).

## Licenses

### GBD

Released under the MIT license, GBD is partially open sourced. It is available as either freeware or in commercial package.
See [*GBD Releases and Directions*](https://github.com/generic-beat-detector/GBD/wiki/GBD-Releases-and-Directions). 

### Attribution/Infrastructure

* GBD Server

  __Freeware__ versions of the GBD server are __statically linked__ against the following libraries to enable secure communication
  with an external authentication service on the Internet.
  
	* [*libcurl*](https://curl.haxx.se/libcurl/), the multi-protocol transfer library ([*Curl license*](https://curl.haxx.se/docs/copyright.html) -- an MIT/X derivative license)
  
	* [*OpenSSL*](https://www.openssl.org/) crypto infrastructure ([*OpenSSL license*](https://www.openssl.org/source/license.html))

  __Commercial__ releases of the GBD server are __neither__ statically nor dynamically linked against libcurl or OpenSSL. Their
  authentication keys are stored locally on disk and no connection is made to an external service on the Internet. 
  
* GBD Library

  The binaries for both the freeware and commercial versions of the GBD DSP library incorporate the 
  [*KISS (Keep It Simple Stupid) FFT*](https://github.com/mborgerding/kissfft) source code which, by default, is released under a revised BSD license. 
  
  Nevertheless, if you intend to incorporate GBD in your commercial application and don't want to make mention of KISS FFT, a commercial license is available from the author of the KISS FFT library which removes the requirement for attribution. Check [*this*](https://github.com/mborgerding/kissfft) out.
  
## Documentation and Tutorials

Check the [*GBD wiki*](https://github.com/generic-beat-detector/GBD/wiki) for resources on
the GBD IoT framework, GBD HowTos, GBD maker guides, etc.



