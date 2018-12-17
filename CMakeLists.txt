cmake_minimum_required (VERSION 2.6)
project (I2cLedScreen)
add_executable(I2cLed src/led_i2c_command.cpp src/log.cpp )
include_directories(inc)
target_link_libraries (I2cLed pthread)

