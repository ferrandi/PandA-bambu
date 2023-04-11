# AppImage generation
AppImage is a format for distributing portable software on Linux without needing superuser permissions to install the application.<br>
For further details see https://appimage.org/.

The PandA Bambu AppImage will contain the whole PandA HLS suite of tools. 
The default executable is Bambu HLS, the actual synthesis tool, but other PandA applications may be launched by exploiting symlinks. Other tools like Spider, Eucalyptus, the bundled FloPoCo executable, and others can be used by creating a symlink to the AppImage file with the name of the executable. As an example, to launch the bundled Clang compiler within the bambu-x86_64.AppImage file, one could create a link named clang-6.0 as follows: 
```
ln -s bambu-x86_64.AppImage clang-6.0
```
Now ./clang-6.0 will result in launching the bundled clang-6.0 executable.<br>
Be aware that not all bundled tools are completely relocatable, thus it may be necessary to specify absolute paths when dealing with I/O files to avoid issues.

## Generating with Vagrant
In this directory we have a set of Vagrant scripts which will produce an AppImage of bambu.

## Generating with Docker
To generate the PandA Bambu AppImage using docker please see the github action under *.github/actions/generate-appimage* and in particular the [*entrypoint.sh*](/.github/actions/generate-appimage/entrypoint.sh) script.<br>
If needed you may find all the custom compiled GCC distributions we are testing against at https://release.bambuhls.eu/compiler/gcc-&lt;version&gt;-bambu-Ubuntu_16.04.tar.xz. <br>
Currently supported versions: 
- gcc-4.5
- gcc-4.6
- gcc-4.7
- gcc-4.8
- gcc-4.9
- gcc-5
- gcc-6
- gcc-7
- gcc-8

Clang supported versions and their pre-compiled binaries download links are reported inside the build-appimage github workflow under *.github/workflows/build-appimage.yml*.
