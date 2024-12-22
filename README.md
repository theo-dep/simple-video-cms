# Simple Video CMS

A managed video sharing platform inspired by [VideoHub](https://github.com/sharadbhat/VideoHub).
A back and front architecture video content management system with FFmpeg and SQLite.

###### All third party files are available offline.

### Docker

Install Docker and Docker Compose then,

```bash
docker compose up -d
```

### Local

For development purpose, files can be compiled with `gcc` and `g++`.

Install development requirements:
```bash
sudo apt install make gcc yasm g++
```

Then build (zlib, libpng, ffmpeg and servers):
```bash
./scripts/build.sh
```

This will generates server binaries in `front/server` and `back/server`.

Servers can be run using VS Code or with both scripts `./scripts/start_front.sh` and `./scripts/start_back.sh`.

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
