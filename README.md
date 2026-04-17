# Simple Video CMS

A managed video sharing platform inspired by [VideoHub](https://github.com/sharadbhat/VideoHub).
A back and front architecture video content management system with FFmpeg and SQLite.

[![Latest Release](https://gitlab.devau.co/theo/simple-video-cms/-/badges/release.svg)](https://gitlab.devau.co/theo/simple-video-cms/-/releases) [![pipeline status](https://gitlab.devau.co/theo/simple-video-cms/badges/develop/pipeline.svg)](https://gitlab.devau.co/theo/simple-video-cms/-/commits/develop)

### Third Parties

All third party files are available offline except for FFmpeg, zlib and libpng submodules.

- [C++ third parties](common/third-party/Readme.md)
- [CSS third parties](back/static/css/third-party/Readme.md)
- [JavaScript third parties](back/static/js/third-party/Readme.md)

### Local

For development purpose, files can be compiled with `clang LLVM` version 21.

Install [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-bash#1---set-up-vcpkg).

Install development requirements:

```bash
sudo apt install cmake clang-21 yasm ninja-build
```

Then build (zlib, libpng, ffmpeg and servers):

```bash
cmake --preset vcpkg
cmake --build --preset vcpkg-release --parallel
```

This will generates server binaries in `build/vcpkg/front/Release/front` and `build/vcpkg/back/Release/back`. Build with `vcpkg-debug` preset to make debug binaries.

Servers can be run using VS Code and CMake extension or with both commands `just start-front` and `just start-back`.

### Docker

To build docker images, install a release build in `dist`:

```bash
cmake --preset vcpkg -DCMAKE_INSTALL_PREFIX=dist
cmake --build --preset vcpkg-release --parallel --target install
```

Then build images:

```bash
docker build -f docker/front.containerfile -t simple-video-cms:front-latest dist
docker build -f docker/back.containerfile -t simple-video-cms:back-latest dist
```

### Environment

To personalize front and backends, there is few environment variables to know:

- Super administrator (cannot be deleted):
  - SUPER_ADMIN_USERNAME (default admin)
- Server addresses:
  - BACK_HOST (default 0.0.0.0)
  - BACK_PORT (default 5000)
  - FRONT_HOST (default 0.0.0.0)
  - FRONT_PORT (default 8080)
  - BACK_SERVER_URL (default localhost:5000)
- Name of your hosted website:
  - WEBSITE_NAME (default Simple Video CMS)
- Browser icon:
  - ICON_PATH (default /static/img/icon.svg), mount a volume with your custom icon and set this variable to the path

# Screenshots

## Normal user

#### Homepage

![Homepage](./images/homepage.png "Homepage")

#### Login / Signup Page

![Login Page](./images/login.png "Login Page")

#### Video Search Page

![Search Page](./images/search.png "Search Page")

#### Upload Page

![Upload Page](./images/upload.png "Upload Page")

#### Video Page

![Video Page](./images/video.jpg "Video Page")

#### Video Page Full Screen

![Video Page Full Screen](./images/video_full.jpg "Video Page Full Screen")

#### Dashboard

![Dashboard](./images/user_dash.png "Dashboard")

### Administrator

#### Flagged Video List

![Flagged Page](./images/flagged.png "Flagged Video List")

#### Users List

![Users page](./images/user_list.png "Login Page")
