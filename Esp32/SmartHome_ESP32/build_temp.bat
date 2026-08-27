@echo off
set PATH=D:\Esp32\Espressif\tools\xtensa-esp32s3-elf\esp-12.2.0_20230208\xtensa-esp32s3-elf\bin;D:\Esp32\Espressif\tools\ninja\1.10.2;D:\Esp32\Espressif\python_env\idf5.1_py3.11_env\Scripts;D:\Esp32\Espressif\tools\idf-git\2.39.2\cmd;%PATH%
set IDF_PATH=D:\Esp32\Espressif\frameworks\esp-idf-v5.1.2
cd /d D:\Project\SmartHome\Esp32\SmartHome_ESP32
if exist build rmdir /s /q build
D:\Esp32\Espressif\tools\cmake\3.24.0\bin\cmake.exe -G Ninja -DPYTHON_DEPS_CHECKED=1 -DESP_PLATFORM=1 -B build -S . -DSDKCONFIG=D:\Project\SmartHome\Esp32\SmartHome_ESP32\sdkconfig > D:\Project\SmartHome\Esp32\SmartHome_ESP32\build_output.log 2>&1
echo EXIT_CODE=%ERRORLEVEL%
