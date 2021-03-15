In this directory we have a set of Vagrant scripts creating an AppImage of bambu.
AppImage is a format for distributing portable software on Linux without needing superuser permissions to install the application.
For further details see https://appimage.org/.

These scripts need to be maintained with respect to the AppImage releases.
In particular, we need to periodically check if these links are fine:
 - https://github.com/niess/python-appimage/releases/download/python3.8/python3.8.5-cp38-cp38-manylinux1_x86_64.AppImage
 - https://github.com/AppImage/AppImageKit/releases/download/12/appimagetool-x86_64.AppImage
 - https://github.com/AppImage/AppImageKit/releases/download/12/AppRun-x86_64 
