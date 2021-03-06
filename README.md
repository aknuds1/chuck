# ChucK [![Build Status](https://travis-ci.org/aknuds1/chuck.svg?branch=master)](https://travis-ci.org/aknuds1/chuck)
## Strongly-timed, Concurrent, and On-the-fly Music Programming Language  

Welcome to ChucK!

what is it? : ChucK is a programming language for real-time sound synthesis and music creation. It is open-source and freely available on MacOS X, Windows, and Linux. ChucK presents a unique time-based, concurrent programming model that's precise and expressive (we call this strongly-timed), dynamic control rates, and the ability to add and modify code on-the-fly. In addition, ChucK supports MIDI, OpenSoundControl, HID device, and multi-channel audio. It's fun and easy to learn, and offers composers, researchers, and performers a powerful programming tool for building and experimenting with complex audio synthesis/analysis programs, and real-time interactive music.

For more information, including documentation, research publications, and community resources, please check out the ChucK website:
http://chuck.stanford.edu/

## JavaScript
ChucK has a JavaScript port, generated by the conversion tool [Emscripten](http://emscripten.org/).
The JavaScript version consists of a single file, chuck.js, which is generated by the make target 'emscripten'.

This library requires Web Audio API to function, so be sure to combine it with a compliant browser.
We have a test HTML page to demonstrate chuck.js, src/emscripten/chuck.html; just open this in a Web Audio
enabled browser, and you should hear a short sine tone.

## BUILDING
If you wish to build for debugging, make sure you export the environment variable CHUCK_DEBUG equal to 1
(`export CHUCK_DEBUG=1`) before building. This will disable optimization and enable debug symbols. It will
also make the the JavaScript version of ChucK easier to work with, due to not being optimized.

### JavaScript
To build the JavaScript port of ChucK, you should use the [Gradle](http://gradle.org) build tool. Since
a wrapper is included with ChucK, you'll only need to make sure the Java runtime (JRE) is installed first.
Then, you must install [Emscripten](http://emscripten.org/) and make sure it's activated in your shell
by [sourcing](http://superuser.com/questions/46139/what-does-source-do) the Emscripten SDK. On OS X I
accomplish this with the following command:
`pushd ~/Applications/emsdk_portable/ && source ./emsdk_env.sh && popd`. Now, from the root directory run
`./gradlew emscripten`. This will generate a JavaScript library with the help of Emscripten:
'build/js/chuck.js'.

### JavaScript Demos
To build the JavaScript demo HTML pages, first make sure you have [NPM](https://www.npmjs.org/) installed.
Also, install grunt (`npm install -g grunt-cli`) if you don't have it. Then, enter the js directory, and
if you haven't done so already, run `npm install` to install the JavaScript dependencies.

To build the demo pages, in js/examples/, enter the js/ directory and execute `grunt`. This'll build a
number of standalone HTML pages in the js/examples/ directory, each of which you can open to see/hear a
certain demo of ChucK.

It is recommended to launch the demo pages via a Web server, this is actually required if chuck.js has
been built with optimizations, for technical reasons. If you've got Python installed, you can start
a Web server as simple as this: `python -m SimpleHTTPServer`.
