# VitaShell-stripped

This is a stripped version of VitaShell, with *basic* file management features.
The goal is to be as small as possible, while still letting you edit text files and install VPKs.

## Changelog
See [CHANGELOG.md](CHANGELOG.md)

## Building
Install [VitaSDK](https://github.com/vitasdk), then install the necessary packages:
```
vdpm install libarchive libftpvita libvita2d
```

Now you can build using the following command:
```
mkdir build && cd build && cmake .. && make
```

## Credits
* Team Molecule for HENkaku
* xerpi for ftpvitalib and vita2dlib
* wololo for the Revitalize contest
* sakya for Lightmp3
* Everybody who contributed on vitasdk
