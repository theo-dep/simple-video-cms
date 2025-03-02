# Third Parties

## cpp-httplib

- v0.18.0
- https://github.com/yhirose/cpp-httplib
- MIT License

## inja

- v3.4.0
- https://github.com/pantor/inja/
- MIT License

## nlohmann/json

- v3.11.3
- https://github.com/nlohmann/json
- MIT License

## sqlite3

- v3.47.2
- https://github.com/sqlite/sqlite
- Public Domain

## sqlite_orm

- v1.9
- https://github.com/fnc12/sqlite_orm
- AGPL License

### Patch

- line 1320 with Ubuntu LLVM version 19.1.1
```cpp
  - template<size_t Pos, size_t... Idx>
  - SQLITE_ORM_CONSTEVAL size_t index_sequence_value_at(std::index_sequence<Idx...>) {
  + template<size_t Pos, size_t... Idx>
  + SQLITE_ORM_CONSTEVAL auto index_sequence_value_at(std::index_sequence<Idx...>) {
```

## rapidfuzz-cpp

- v3.1.1
- taken amalgamated header in "extras" folder
- https://github.com/rapidfuzz/rapidfuzz-cpp
- MIT License

## KDBindings

- v1.1.0
- https://github.com/KDAB/KDBindings
- MIT License

## Hash++

- v2.0.1
- https://github.com/D7EAD/HashPlusPlus
- MIT License

## FFmpeg

- v7.1.0
- https://git.ffmpeg.org/gitweb/ffmpeg
- GNU LGPL v2 and GPL v2 for parts upgraded to v3

## zlib

- v1.3.1
- https://github.com/madler/zlib/
- Copyright

## libpng

- v1.6.44
- https://github.com/pnggroup/libpng
- PNG Reference Library License version 2
