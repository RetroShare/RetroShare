# Compilation on Windows

The preferred build method on Windows is by using MSYS2 which is a collection
of packages providing unix-like tools to build native Windows software.

This guide contains information about how to setup your build environment in an automated way by scripts.
If you prefer to setup your environment manually, check this guide: 
[WindowsMSys2_InstallGuide.md](WindowsMSys2_InstallGuide.md)

Setting up the build environment automatically on a 32 bit OS is not possible anymore. 
You can download an older 32 bit [MSYS2 installer](https://sourceforge.net/projects/msys2/files/Base/i686/msys2-base-i686-20180531.tar.xz/download) and follow the manual setup instructions.
Building 32 bit RetroShare from the 64 bit build environment is still possible.

You have to clone this repository (with [git for windows](https://gitforwindows.org/)) to a local folder, then start it in a terminal.


## Automatic building

Run the following script:

    <sourcefolder>\build_scripts\Windows-msys2\build.bat

It will install all necessary tools to build RetrosShare, and build it with the default configuration.

After the script is finished, you can find the resulting .7z package here:

    <sourcefolder>-msys2\deploy\

## Advanced building

You can specify extra build options if you use the scripts under:

    <sourcefolder>\build_scripts\Windows-msys2\build\

You have to specify the build options after each command.

Run the scripts in this order:
1. build.bat: builds RetroShare
2. pack.bat: makes a .7z archive with all the needed files to run the program
3. build-installer: makes a self extracting installer (optional)

### Build options

**Always delete the build artifacts folder if you enable or disable extra features after the build command: &lt;sourcefolder&gt;-msys2\deploy\builds**

* Mandatory
  * 32 or 64: 32 or 64 bit version
  * release or debug:   normally you would like to use the release option
* Extra features (optional)
  * autologin:          enable autologin
  * plugins:            build plugins
  * webui:              enable remoting features and pack webui files
  * indexing:           build with deep channel and file indexing support
  * tor:                include tor in the package
  * "CONFIG+=..."       enable other extra compile time features, you can find the almost complete list in file *&lt;sourcefolder&gt;\retroshare.pri*
* For fixing compile problems (optional)
  * singlethread:       use only 1 thread for building, slow but useful if you don't find the error message in the console
  * clang:              use clang compiler instead of GCC
  * noupdate:           skip the msys2 update step, sometimes some msys2 packages are broken, you can manually switch back to the older package, and this option will prevent updating to the broken version again

Example:

```batch
build.bat 64 release autologin webui "CONFIG+=rs_use_native_dialogs" "CONFIG+=rs_gui_cmark"
pack.bat 64 release autologin webui tor
build-installer.bat 64 release autologin
```

## Troubleshooting
* Run the command again, sometimes it works the second time, specially if it complains about *restbed* during building
* Delete the build artifacts: *&lt;sourcefolder&gt;-msys2\deploy\builds*
* Update msys2 manually:
  1. Open the terminal: *&lt;sourcefolder&gt;-msys2\msys2\msys64\msys2.exe*
  2. pacman -Sy
  3. pacman -Su
  4. Close the terminal
  5. Jump to 1. until it doesn't find more updates
* Start with a clean path environment variable, run *&lt;sourcefolder&gt;\build_scripts\Windows-msys2\start-clean-env.bat*, you will get a terminal with cleaned path

### Errors during MSYS2 update
MSYS2 developers recently introduced some breaking changes.
If you get PGP related errors during updating the system from pacman, then follow [their guide](https://www.msys2.org/news/#2020-06-29-new-packagers) to resolve the problems.

## Updating webui

The scripts don't update the webui source code automatically once it is checked out.
You have to do it manually with your favourite git client.

You can find the webui source code here:

    <sourcefolder>-webui
