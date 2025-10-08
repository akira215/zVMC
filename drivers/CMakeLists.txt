set(SOURCES     waterLevelMeasCluster.cpp
                waterPressureMeasCluster.cpp 
                waterFlowMeasCluster.cpp
                tempCluster.cpp
                adsDriver.cpp)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS include 
                    REQUIRES cppgpio persistedValue zigbee periodicSoftTask
                            cppi2c ads1115 esp_driver_tsens scheduledTask)
                    