# Wii mini SD Patcher

Tool to aid in the installation of required software for restoring SD functionality on the Wii mini, based on [Aurelio's RVLoader Installer](https://github.com/Aurelio92/RVLoader/tree/main/installer)

Most of the code was written and documented by Aurelio for RVLoader, without whose help this project would not have been possible.
Priiloader code and binaries (priiloader.c/h/app) originally by DacoTaco for [Priiloader](https://github.com/DacoTaco/priiloader)

For information and assistance on how to use this software, please visit [the Wii mini Hacking Discord server](https://discord.gg/7jNHphfDQk)

### Features:

- Patch and install original Wii IOS (versions 36, 58 and 80):
	
	These "nowifi" patches allow the IOSes to work without the need for a Wireless network card, while keeping the SD modules that are not present on the Wii mini IOS, thus allowing for a soldered SD card to work almost fully on a Wii mini console.
- Install the Wii System Menu and Priiloader
	This just simplifies the installation of the System Menu WAD if present on the USB drive, while providing some safety if it fails by instantly installing Priiloader on top of it.
- Install the Open Homebrew Channel as LULZ to replace upside-down hbc (TBA)

### Usage:
To use this app, download the zip file, decompress it and place the contents inside the /apps folder of your USB drive.

You will also need to obtain the following files from NUS and place them in the root of your USB Drive:
```
IOS36-64-3608.wad
IOS58-64-6176.wad
IOS80-64-6944.wad
```

We recommend using [Sharpii NetCore](https://github.com/TheShadowEevee/Sharpii-NetCore/releases) for this as it is a cross-platform solution.
To download the above WADs, run `Sharpii.exe` in Windows (or `chmod +x Sharpii` in macOS/Linux and then `./Sharpii`)  with the following arguments (Replacing `Sharpii` with the filename that matches your OS as described above):

```
Sharpii nusd -wad -ios 36 -v 3608 -o IOS36-64-3608.wad
```
```
Sharpii nusd -wad -ios 58 -v 6176 -o IOS58-64-6176.wad
```
```
Sharpii nusd -wad -ios 80 -v 6944 -o IOS80-64-6944.wad
```

To install the System Menu, also place `SystemMenu4.3Y-vXXX.wad` in the root of the USB drive (`XXX` being `513` for NTSC consoles and `514` for PAL, `Y` being the region code: `U` for NTSC, `E` for PAL).

To download it run the following (replace `Sharpii` with your sharpii executable, `XXX` with the region version and `Y` with the region code,  as mentioned above):
```
Sharpii nusd -wad -id 0000000100000002 -v XXX -o SystemMenu4.3Y-vXXX.wad
```

THIS APPLICATION COMES WITH ABSOLUTELY NO WARRANTY, TO THE EXTENT PERMITTED BY APPLICABLE LAW.
USE AT YOUR OWN RISK. 
While the odds of it breaking your console are pretty low, we or anybody else assume no responsibilities in any harm caused to your or anyone else's property.