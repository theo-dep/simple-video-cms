# Simple Video CMS

A managed video sharing platform inspired by [VideoHub](https://github.com/sharadbhat/VideoHub).
A preact web application video content management system and cpp-httplib, FFmpeg, SQLite backend c++26 server.

[![Latest Release](https://gitlab.devau.co/theo/simple-video-cms/-/badges/release.svg)](https://gitlab.devau.co/theo/simple-video-cms/-/releases) [![pipeline status](https://gitlab.devau.co/theo/simple-video-cms/badges/develop/pipeline.svg)](https://gitlab.devau.co/theo/simple-video-cms/-/commits/develop)

## Local

For development purpose, files can be compiled with `clang LLVM` version 22.

Install [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-bash#1---set-up-vcpkg).

Install development requirements:

```bash
sudo apt install npm cmake clang-22 yasm ninja-build
```

And Node.js dependencies:

```bash
npm install
```

Then build (zlib, libpng, ffmpeg and back server):

```bash
cmake --preset vcpkg
cmake --build --preset vcpkg-release --parallel
```

This will generates server binary in `build/vcpkg/Release/back`. Build with `vcpkg-debug` preset to make debug binaries.

Server can be run using VS Code and CMake extension or with commands `just start-dev` or `just start-prod`.

> [!NOTE]
> Configure CMake with `-DCMAKE_CXX_FLAGS_RELEASE=-DNO_SECURE` to define `NO_SECURE` cookie in Release mode.
> This allow to test the production bundle in local environment (like with a phone).

## Container

To build container images, install a release build in `dist`:

```bash
cmake --preset vcpkg -DCMAKE_INSTALL_PREFIX=dist
cmake --build --preset vcpkg-release --parallel --target install
```

Then build images:

```bash
docker build -f container/Containerfile -t simple-video-cms:latest dist
```

## Environment

To personalize front and backend, there is few environment variables to know:

- Super administrator (cannot be deleted):
  - SUPER_ADMIN_USERNAME (default admin)
- Back server address:
  - BACK_HOST (default 0.0.0.0)
  - BACK_PORT (default 8080)
- Name of your hosted website:
  - WEBSITE_NAME (default Simple Video CMS)
  - SHORT_WEBSITE_NAME (default Video CMS)

## Screenshots

TODO

## Third Parties

All third party files are available offline.

### C++

#### cpp-httplib

- https://github.com/yhirose/cpp-httplib
- MIT License

#### nlohmann/json

- https://github.com/nlohmann/json
- MIT License

#### sqlite3

- https://github.com/sqlite/sqlite
- Public Domain

#### sqlite_orm

- https://github.com/fnc12/sqlite_orm
- AGPL License

#### FFmpeg

- https://git.ffmpeg.org/gitweb/ffmpeg
- GNU LGPL v2 and GPL v2 for parts upgraded to v3

#### zlib

- https://github.com/madler/zlib/
- Copyright

#### libpng

- https://github.com/pnggroup/libpng
- PNG Reference Library License version 2

#### OpenSSL

- https://www.openssl.org/
- Apache-2.0

### Javascript and CSS

#### preact

- https://github.com/preactjs/preact
- MIT License

#### Fuse.js

- https://github.com/krisk/fuse
- Apache License Version 2.0

#### Pure.css

- https://github.com/pure-css/pure
- BSD Yahoo! Inc. License

#### Video.js

- https://github.com/videojs/video.js
- Apache License Version 2.0

#### videojs-mobile-ui

- https://github.com/mister-ben/videojs-mobile-ui
- MIT License

#### videojs-yt-style

- https://github.com/paidless/videojs-yt-style
- Unlicensed

#### Bootstrap Icons

- https://github.com/twbs/icons
- MIT License

#### MultiSelect

- https://github.com/codeshackio/multi-select-dropdown-js
- MIT License

#### SVG-Spinners

- https://github.com/n3r4zzurr0/svg-spinners
- MIT License
