##Compilation on Windows

###Qt Installation

Install Qt via: [Qt Download](http://www.qt.io/download/)  

Use default options.  
Add to the PATH environment variable

       ;C:\Qt\5.5\mingw492_32\bin;C:\Qt\Tools\mingw492_32\bin;C:\Qt\Tools\mingw492_32\opt\bin  

Depends on wich version of Qt you use.  
Change build-all-mingw32make.bat with these values too if you don't use MSys2.  

###MSYS2 INSTALLATION

Choose your MSYS2 installer here: [MSYS2](http://msys2.github.io/)

Follow install procedure.  
Don't forget to sync & Update pacman.  

       pacman --needed -Sy bash pacman pacman-mirrors msys2-runtime  

Restart console  

       pacman -Su  

Install all default programms  

       pacman -S base-devel git mercurial cvs wget p7zip gcc perl ruby python2  

Choose only w64-i686 if you want only compilation in 32b architecture.  

       pacman -S mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain  

###Install other binutils:   
       pacman -S mingw-w64-i686-miniupnpc mingw-w64-x86_64-miniupnpc  
       pacman -S mingw-w64-i686-sqlite3 mingw-w64-x86_64-sqlite3  
       pacman -S mingw-w64-i686-speex mingw-w64-x86_64-speex  
       pacman -S mingw-w64-i686-opencv mingw-w64-x86_64-opencv  
       pacman -S mingw-w64-i686-ffmpeg mingw-w64-x86_64-ffmpeg  
       pacman -S mingw-w64-i686-libmicrohttpd mingw-w64-x86_64-libmicrohttpd  
       pacman -S mingw-w64-i686-libxslt mingw-w64-x86_64-libxslt  

Add MSYS2 to PATH environment variable depends your windows  

       ;C:\msys64\mingw32\bin
       ;C:\msys32\mingw32\bin


###Git Installation

Install Git Gui or other client: [Git Scm](https://git-scm.com/download/win)

Create a new directory named:  

       C:\Development\GIT  

Right-click on it and choose: *Git Bash Here*  

Paste this text on git console:  
git clone https://github.com/RetroShare/RetroShare.git  


###Last Settings


In QtCreator Option Git add its path: *C:\Program Files\Git\bin* 
and select "Pull" with "Rebase"  


Open an MSys2 32|64 shell  
Move to build_scripts:  

       cd /c/Development/GIT/RetroShare/msys2_build_libs/  

###Compile missing library
       make

You can now compile RS into Qt Creator  

For using, and debugging Plugins, you can use [Symlinker](http://amd989.github.io/Symlinker/) to link 
the files in  

       \build-RetroShare-Desktop_Qt_X_Y_Z_MinGW_32bit-Debug\plugins\PluginName\debug\*.dll  
to  
*%appdata%\RetroShare\extensions6*
