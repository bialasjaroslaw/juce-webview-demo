# Installation notes

Use vcpkg in manifest mode if you can
```
vcpkg install
cmake --preset debug
cmake --build --preset debug
```

If you don't want to use vcpkg just follow comands below. You will be prompted to install nuget and webview if not already done
```
mkdir build
cd build
cmake -S .. -B .
cmake --build .
```
