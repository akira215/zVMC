set(SOURCES     waterLevelMeasCluster.cpp
                waterPressureMeasCluster.cpp 
                tempCluster.cpp
                aldesDriver.cpp)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS include 
                    REQUIRES cppgpio persistedValue zigbee periodicSoftTask
                             esp_driver_tsens scheduledTask modbus)
                    