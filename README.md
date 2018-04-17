This is an example application making use of Glasgow-MicroStream and the BME280 sensor to calculate differences between two sensor readings from separate ESP32.

The sensor library is adapted from https://github.com/yanbe/bme280-esp-idf-i2c .  Modifications include making it compile with C++.

## Setup
Make sure to follow the instructions at https://esp-idf.readthedocs.io/en/v2.0/linux-setup.html.
There are some default compiler flags that need to be removed to use Glasgow-Microstream on the ESP32.  Namely the -fno-rtti flag should be removed from the CXXFLAGS in esp-idf's make/project.mk file.
```
git clone git@github.com:sbeanie/Glasgow-MicroStream-ESP32-BME280-Example.git
cd Glasgow-MicroStream-ESP32-BME280-Example
git submodule init
git submodule update --recursive
make -j8 flash
```

### Eclipse
Follow the instructions here https://esp-idf.readthedocs.io/en/v2.0/eclipse-setup.html.
However, replace the full command from the "CDT GCC Build Output Parser" step with "xtensa-esp32-elf-((g?cc)|([gc]++)|(clang))".
Then clean the project and build.  All headers should resolve.
