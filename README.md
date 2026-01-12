# Installation notes

Just remember to pull submodule

```
git submodule update --init
```

Use vcpkg in manifest mode if you can
```
vcpkg install
cmake --preset vcpkg
cmake --build --preset vcpkg
```

If you don't want to use vcpkg just follow comands below. You will be prompted to install nuget and webview if not already done
```
cmake --preset default
cmake --build --preset default
```

Builded and tested on Windows 11 with MSVC 14.50.35717, WSL - Ubuntu 24.04 with GCC 13.3.0 and Ubuntu 25.04 with GCC 14.2.0
