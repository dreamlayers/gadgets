## What is this?

This is a collection of code for various one of a kind hardware devices built
by Boris Gjenero. It is all together in one Git repository due to
interdependencies. Gadget microcontroller firmware is distributed separately.
See the links below.

While the devices themselves and their interface libraries are one of a
kind, some parts of this may be interesting to others. I think the most
interesting part is the [RGB lamp music visualizer](rgblamp/vis_rgb/). You
may also be interested in the [serial interfacing library](serio/).
Some other parts are ancient and probably uninteresting.

## Contents

* [build](build/) : Makefile parts included from various places
* [common](common/) : Files used by multiple components
* [serio](serio/) : The serial port library used by all direct interfacing
  libraries for using the serial port in a platform-independent way. It is the
  oldest component here. Internally, global variables are used, so one copy of
  serio can only be used by one application at a time. Serio supports Windows
  and Unix serial ports, and now it also supports TCP connections in Unix.
* [pluginapi/winamp](pluginapi/winamp) : Winamp API headers
* [signd](signd/) : Common TCP daemon code for signs, used by lsd and vfdd.
  In Windows it runs as a tray application via tray42 by Michael T. Smith.

* [ledsign](ledsign/) : Code for
  [my LED sign](https://dreamlayers.blogspot.ca/2011/11/my-led-sign.html)
  * [libledsign](ledsign/libledsign/) : Direct interfacing library for the
    LED sign
  * [lsd](ledsign/lsd/) : TCP daemon for controlling LED sign and client
    library for using the daemon
  * [gen_led](ledsign/gen_led/) : Winamp plugin showing titles on the LED sign
  * [lsp](ledsign/lsp/) : CGI program with a web interface for displaying
    text messages on the LED sign.
    Requires [cgic library](http://www.boutell.com/cgic/)
  * [JSDraw](ledsign/JSDraw/) : Old JavaScript program for drawing on the
    LED sign. Very inefficient, and there is no program for uploading images.

* [rgblamp](rgblamp/) : Code for
  [my RGB lamp](https://dreamlayers.blogspot.ca/2011/10/my-msp430-based-rgb-light.html)
  * [librgblamp](rgblamp/librgblamp/) : Direct interfacing library for RGB lamp
  * [vis_rgb](rgblamp/vis_rgb/) : Music visualization program for RGB lamp
    It can be built for Winamp, Audacious, and stand-alone. There is a version
    which changes the colour of a window on the screen instead of using the
    light. Ruby is required for building. One file is built via GNU Octave,
    but the output file is in the repository so GNU Octave is not required.
  * [winrgbchoose](rgblamp/winrgbchoose/) : Windows colour chooser for the RGB
    lamp.
  * [rgbmacpick](rgblamp/rgbmacpick/) : Mac OS colour chooser for the RGB lamp.
    This does not include other Xcode generated stuff needed to build.

* [vfd](vfd/) : Code for my VFD display
  * [libvfd](vfd/libvfd/) : Direct interfacing library for VFD display
  * [vfd_plugin](vfd/vfd_plugin/) : Winamp and Audacious plugin showing music
    player and system info. Supports system info for both Linux and Windows.
  * [vfdd](vfd/vfdd) : TCP daemon for controlling VFD display
