set(SOURCES main.cpp)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . ../drivers/include
                    REQUIRES periodicTask zigbee cppgpio drivers)
                    