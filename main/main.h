#pragma once

#include "zbNode.h"
#include "cppgpio.h" // to del
#include "buttonTask.h" // to del
#include "blinkTask.h" // to del

#include "tempCluster.h"

#include "waterFlowMeasCluster.h" 
#include "waterPressureMeasCluster.h"
#include "waterLevelMeasCluster.h"

#include "zbOtaClusterClient.h"

#include "zbHaCluster.h"
#include "zbTimeClusterClient.h" 


#include "modbusMaster.h" 

/* Attribute values in ZCL string format
 * The string should be started with the length of its own.
 */
#define MANUFACTURER_NAME               "\x09""AkiraCorp" //TODO del
#define MODEL_IDENTIFIER                "\x10""WaterTankMonitor"  //TODO del


// Main class used for testing only
class Main final
{
    GpioInput               _button {(gpio_num_t)CONFIG_PIN_BUTTON, true};
    ButtonTask*             _buttonTask = nullptr;

    static BlinkTask*       _ledBlinking;
    static GpioOutput       _led;
    
    //inline static Main* _this = nullptr;

public:
    Main();
    void run(void);
    void setup(void);

    void zbDeviceEventHandler(ZbNode::nodeEvent_t event);
    static void shortPressHandler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data);
    static void longPressHandler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data);

    static void identifyHandler(uint16_t attrId, void* value);

private:
    /// @brief Helper to flash led
    /// @param speed flash cycle in ms. if 0, led will be set to off, 
    /// if -1 led will be switch on
    static void ledFlash(uint64_t speed);
    
    // Check RTC Sync and trigger all operation that require to be RTC sync
    void checkRTCSync();

    void setupModbus();

    ZbNode*                     _zbDevice           = nullptr;
    ZbTimeClusterClient*        _timeClient         = nullptr;

    TaskHandle_t                _xButtonHandle      = nullptr;
    TaskHandle_t                _eventLoopHandle    = nullptr;

    WaterFlowMeasCluster*       _fMeter             = nullptr;
    WaterPressureMeasCluster*   _upstreamPressure   = nullptr;
    WaterPressureMeasCluster*   _downstreamPressure = nullptr;
    WaterLevelMeasCluster*      _waterLevel         = nullptr;
    TempCluster*                _tempMeasurement    = nullptr;

    ZbOtaClusterClient*         _otaCluster         = nullptr;

    PeriodicSoftTask*           _checkRTCSync       = nullptr;

    ModbusMaster*               _mb_master          = nullptr;

}; // Main Class