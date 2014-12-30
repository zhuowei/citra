Tested with Android NDK r10c.
You need to copy externals/libiconv-1.14 from Dolphin's source.
Build with
```
cmake -DANDROID=True -DANDROID_NATIVE_API_LEVEL=android-14 -DGIT_EXECUTABLE=/usr/bin/git -DCMAKE_TOOLCHAIN_FILE=../src/android/android.toolchain.cmake -DENABLE_GLFW=False -DENABLE_QT=False -DANDROID_ABI="armeabi-v7a with NEON" -DANDROID_TOOLCHAIN_NAME="arm-linux-androideabi-clang3.5" -DANDROID_STL="c++_static" ..
```
