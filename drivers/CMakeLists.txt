set(SOURCES aldesDriver.cpp)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS include 
                    REQUIRES cppgpio zigbee periodicSoftTask
                            scheduledTask modbus)
                    