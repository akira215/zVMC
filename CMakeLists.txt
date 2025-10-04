# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)

set(EXTRA_COMPONENT_DIRS components drivers)
set(COMPONENTS cxx app bootloader bootloader_support
       app_trace app_update efuse esp32 esp_common 
       esp_event esp_rom esp_partition espcoredump esptool_py freertos heap log 
       pthread perfmon vfs spi_flash
       main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(zTank)
