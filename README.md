# IPSWEx

IPSWEx is a suite of tools to extract wallpapers from iOS/iPadOS firmware files (IPSW), used to build SniperGER/iOS-Wallpapers.  
The toolset is based on malus-security/iextractor, but heavily modified to be specialized on wallpaper extraction.

For wallpaper collections introduced in iOS 16, IPSWEx also includes CAMLViewer, an iOS Simulator app that loads these bundles and saves them as a HEIF image.

IPSWEx is built for macOS (tested extensively with Big Sur, but should also work on other versions), and the extraction scripts include Linux support (although **incomplete and untested**).  

## Tools
### vfdecrypt

`vfdecrypt`is used to decrypt encrypted RootFS DMGs.  
IPSWEx uses [this build](https://github.com/bizonix/vfdecrypt) of `vfdecrypt`, with differences of [iExctractor's fork](https://github.com/malus-security/iextractor/tree/master/tools/vfdecrypt) patched in.

Install dependencies (requires [Homebrew](https://brew.sh)):

```
brew install openssl cmake
```

Apply the patch:

```
patch tools/vfdecrypt/vfdecrypt.c < tools/vfdecrypt.patch
```

Compile `vfdecrypt`:
```
cd tools/vfdecrypt
cc -Wall -I/usr/local/include -DMAC_OSX -L/usr/local/lib  vfdecrypt.c  -lcrypto -o vfdecrypt
```

### manifesto

`manifesto` is a custom tool to extract build information from either `Restore.plist` (old IPSWs) or `BuildManifest.plist` (current IPSWs).

```
Usage: manifesto <path/to/BuildManifest.plist> [OPTIONS] 

Parses information from Restore.plist or BuildManifest.plist

OPTIONS:
  -b, --build             Prints the build version string of an IPSW
  -V, --version           Prints the build version of an IPSW
  -b, --build-train       Prints the build train string of an IPSW
  -p, --product           Prints the (first, if multiple) supported product identifier
  -r, --rootfs            Prints the name of the RootFS .dmg
  -l, --legacy            Sets the "legacy" flag for old firmware files containing Restore.plist
```

Compile `manifesto`:
```
cd tools/manifesto
make
```

### CAMLViewer

CAMLViewer allows you to view wallpaper bundles included in iOS 16 and export them to a static image.  
The app itself runs inside the iOS Simulator and loads whichever wallpaper is defined in `CAMLViewer/Import/Wallpaper.plist` (which usually comes with an extracted wallpaper bundle).

To set the wallpaper name and the associated iOS/iPadOS version, see `CAMLViewer/ExportOptions.plist`.

To run CAMLViewer, open the Xcode project, select a Simulator to run on (device type probably doesn't matter, but using iOS 16 is recommended) and click "Run".  
If the screen is plain white, either there's no wallpaper to be found or there was an error loading it.

Once the wallpaper is loaded, you can click the "Save" button, and it will be saved to `WALLPAPER_OUTPUT` (variable defined in project settings), including the correct directory structure for each device defined by the wallpaper resolution.

## Extract wallpapers

To extract wallpapers from a given firmware, run the following command:

```
./run_extract_wallpapers path/to/Firmware.ipsw
```

The process is fully automatic. You may be asked to enter a RootFS key manually (if such cannot be found) or your root password for mounting the RootFS.  
Extracted wallpapers will be stored in the project directory.

You can also set certain environment variables to specify output directories, the IPSW mount point or logging behavior:

```
ROOT                    Sets the root directory (Default: ./)
KEYS_PATH               Sets the to store firmware keys in (Default: ./ipsw/keys)
IPSW_PATH               Sets the to store extracted firmware files in (Default: ./ipsw/extracted)
MOUNT_POINT             Sets the mount point of the RootFS DMG (Default: ./ipsw/mnt)
OUT                     Sets the directory to copy the Wallpaper folder to (Default: ./)
INFO                    Toggles info messages (Default: 1)
DEBUG                   Toggles debug messages (Default: 0)
WAIT_AFTER_EXTRACT      Waits for user input after mounting the RootFS DMG and extracting the Wallpaper directory (Default: 0)
```
