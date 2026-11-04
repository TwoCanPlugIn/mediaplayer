Media Player plug-in for OpenCPN
==========================
A simple remote control for Fusion Media Players connected via NMEA 2000

Installation
------------
Install using the OpenCPN Plugin Manager.

Features
------------
It allows the user to select and play many of the input sources for a Fusion Media Player

| Control | Description|
|--------|---------------|
|On/Off| Switch the unit on or off|
|Play/Pause| Play or Pause the current media track|
|Save| Save the current radio station to the preset list|
|Next|Play the next track or scna to the next radio station|
|Previous| Play the previous track or scan to the previous radio station|
|Source| Change to the input source to USB, AM/FM, Aux etc.|
|Balance| Adjust the balance for Zone 0|
|Volume | Adjust the volume for Zones 0 or 1|
|Tone controls| adjust Bass, Midrange and Treble|

The settings which are available from the plugin preferences dialog allows the user to change some of teh device's settings

| Setting | Description|
|--------|---------------|
|Device Name| Change the device name|
|Zone Name| Change the zone name|
|Zone Volume| Set the maximum volume for the zone|
|Tuner| Change tuning steps, radio region and delete presets|

Obtaining the source code
-------------------------

git clone https://github.com/twocanplugin/mediaplayer.git


Build Environment
-----------------

Windows, Linux and Mac OSX are currently supported.

This plugin builds outside of the OpenCPN source tree

Refer to the OpenCPN developer manual for details regarding other requirements such as git, cmake and wxWidgets.

It can also be built using the CircleCI and Appveyor continuous integration services.


Build Commands
--------------

 git clone https://github.com/twocanplugin/mediplayer

 cd mediaplayer
 
 mkdir build

 cd build

 cmake ..

 cmake --build . --config Debug/Release/MinSizeRel/RelWithDebInfo

Creating an installation package
--------------------------------
 cmake --build . --config Debug/Release/MinSizeRel/RelWithDebInfo --target tarball

Installation
------------
For locally built versions, they can be installed using the Plugin Manager import tarball option.

Problems
--------
Untested with Apple iPod and Android as USB sources or DAB and SiriusXM radios.

Please send bug reports/questions/comments to the opencpn forum or via email to twocanplugin@hotmail.com

License
-------
The plugin code is licensed under the terms of the GPL v3 or, at your convenience, a later version.

NMEA2000® is a registered trademark of the National Marine Electronics Association.
Fusion® is a registered trademark of Garmin Ltd.
