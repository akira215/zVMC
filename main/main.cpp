//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "main.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <functional>

#include <iostream>

#include <vector> // todel

#include "zbEndpoint.h" // to del
#include "periodicSoftTask.h" // to del


#include "esp_check.h" // todel

static const char *TAG = "Main_app";
BlinkTask* Main::_ledBlinking = nullptr;
GpioOutput Main::_led { (gpio_num_t)CONFIG_PIN_LED }; //TODO led pin number in config file

Main App;

Main::Main()
{
    // Setting the log level for each module
    esp_log_level_set("Main_app", ESP_LOG_DEBUG);  // Put verbose to check available stack
    esp_log_level_set("ZbNode", ESP_LOG_DEBUG); 
    esp_log_level_set("ZbEndpoint", ESP_LOG_VERBOSE);
    esp_log_level_set("ZbNode", ESP_LOG_VERBOSE);
    esp_log_level_set("AdsDriver", ESP_LOG_INFO);
    esp_log_level_set("TempDriver", ESP_LOG_VERBOSE);
    esp_log_level_set("ZbCluster", ESP_LOG_VERBOSE);
    esp_log_level_set("EventLoop", ESP_LOG_DEBUG);
    esp_log_level_set("PersistedValue", ESP_LOG_VERBOSE); 
}

// static
void Main::ledFlash(uint64_t speed)
{
    if(speed == 0) 
    {
        if(_ledBlinking){
        delete _ledBlinking;
        _ledBlinking = nullptr;
        }
        _led.off();
    } else if(speed == -1) {
        if(_ledBlinking){
        delete _ledBlinking;
        _ledBlinking = nullptr;
        }
        _led.on();
    } else {
        if(!_ledBlinking)
            _ledBlinking = new BlinkTask(_led, speed); // very short flash
        else
            _ledBlinking->setBlinkPeriod(speed);
    }

}

//Static
void Main::shortPressHandler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    ESP_LOGV(TAG,"Short Press detected %ld -",id);
    if(ZbNode::getInstance()->isJoined())  
    {
        ESP_LOGI(TAG,"Short Press detected but device is join to zigbee network, nothing to do");
    } else {
         ZbNode::getInstance()->joinNetwork();
    }
}

//Static
void Main::longPressHandler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{

    ESP_LOGV(TAG,"Long Press detected %ld -",id);
    ZbNode::getInstance()->leaveNetwork();

}

// Static
void Main::identifyHandler(uint16_t attrId, void* value)
{
    if(attrId != ESP_ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID)
        return;
    
    ESP_LOGI(TAG,"Identifying time %d",*((uint16_t*)value));
    if (*((uint16_t*)value) !=0)
        ledFlash(50);
    else
        ledFlash(0);
}

//void Main::zbDeviceEventHandler(ZbNode::nodeEvent_t event)
void Main::zbDeviceEventHandler(ZbNode::nodeEvent_t event)
{
    switch(event){
        case ZbNode::JOINED:
            // Device just joined the network
            {
            ledFlash(0);
            // Synchronise RTC clock of the device
            _timeClient->syncRTC();

            _checkRTCSync = new PeriodicSoftTask(&Main::checkRTCSync, this, 500);

            // Wait for the clock to be synchronized (retry every seconds)
            //while(!(_timeClient->isSynchronized()))
            //    vTaskDelay( 1000 / portTICK_PERIOD_MS );
           
            //if (_fMeter) {
            //    ESP_LOGW(TAG,"Clock is finally sync!");
            //}
                

            }
            break;
        case ZbNode::JOINING:
            ledFlash(FAST_BLINK);
            break;
        case ZbNode::NLME_STATUS:
            if(_zbDevice->isJoined()) 
                ledFlash(0);
            else 
                ledFlash(50); //very short flash
            break;
        case ZbNode::LEAVING:
            ledFlash(50);
            break;
        case ZbNode::LEFT:
            ledFlash(-1);
            break; 
        default:
            ESP_LOGW(TAG,"Device Event Handler, event %d not registered occured", 
                        static_cast<uint>(event));
            break;
    }
  
}

void Main::checkRTCSync(){
    if(_timeClient->isSynchronized()){
        if (_fMeter) {
            ESP_LOGV(TAG,"Clock is sync - triggering action on RTC");
            _fMeter->setupResetTask(0);
        }

        delete _checkRTCSync;
        _checkRTCSync = nullptr;
    }
}

void Main::setup(void)
{
    ESP_LOGI(TAG,"---------- Setup ----------");

    _button.enablePullup();
    _buttonTask = new ButtonTask (_button);
    _buttonTask->setShortPressHandler(&shortPressHandler);
    _buttonTask->setLongPressHandler(&longPressHandler,(void*)this);

    AdsDriver::getInstance().setup();

    _mb_master = new ModbusMaster(1);


    ESP_LOGI(TAG,"Creating Zigbee device");
    _zbDevice = ZbNode::getInstance();
    _zbDevice->registerNodeEventHandler(&Main::zbDeviceEventHandler, this);

    // Endpoints Construct ////////////////////////////////////////////////
    ZbEndPoint* generalEp = new ZbEndPoint(CONFIG_GENERAL_EP, 
                            ESP_ZB_HA_METER_INTERFACE_DEVICE_ID);
    
    ZbEndPoint* inputEp = new ZbEndPoint(CONFIG_INPUT_EP, 
                            ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID);

    ZbEndPoint* interiorEp = new ZbEndPoint(CONFIG_INTERIOR_EP, 
                            ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID);
    
    ZbEndPoint* supplyEp = new ZbEndPoint(CONFIG_SUPPLY_EP, 
                            ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID);
    
    ZbEndPoint* exhaustEp = new ZbEndPoint(CONFIG_EXHAUST_EP, 
                            ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID);

    
    // Clusters Construct ////////////////////////////////////////////////
    ZbBasicCluster* basicCl = new ZbBasicCluster(false,
                        ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
                        (uint8_t)0x01);
    basicCl->addAttribute(ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                        (void*)MANUFACTURER_NAME);
    basicCl->addAttribute(ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                        (void*)MODEL_IDENTIFIER);


    _otaCluster = new ZbOtaClusterClient(CONFIG_OTA_TIMER_QUERY,
                                        CONFIG_HARDWARE_VERSION,
                                        CONFIG_MAX_DATA_SIZE);
    

    // Sensor clusters
    _fMeter = new WaterFlowMeasCluster();

    // Upstream channel 2
    _upstreamPressure = new WaterPressureMeasCluster(CONFIG_UPSTREAM_PRESSURE_CH);
    
    // Downstream channel 3
    _downstreamPressure = new WaterPressureMeasCluster(CONFIG_DOWNSTREAM_PRESSURE_CH);
    
    // WaterLevel channel 4
    _waterLevel = new WaterLevelMeasCluster(CONFIG_WATER_LEVEL_CH);
    
    // Temperature cluster
    /*
    _tempMeasurement  = new ZbTemperatureMeasCluster(false,
                                ESP_TEMP_SENSOR_MIN_VALUE,
                                ESP_TEMP_SENSOR_MAX_VALUE,
                                ESP_ZB_ZCL_TEMP_MEASUREMENT_MEASURED_VALUE_DEFAULT);
    */
    _tempMeasurement = new TempCluster();

    _timeClient = new ZbTimeClusterClient();

    ESP_LOGI(TAG,"---------------- Register ------------------------");
    
    ZbIdentifyCluster* identifyServer = new ZbIdentifyCluster();
    ZbIdentifyCluster* identifyClient = new ZbIdentifyCluster(true);
    ZbIdentifyCluster* identifyServer2 = new ZbIdentifyCluster(*identifyServer);
    ZbIdentifyCluster* identifyServer3 = new ZbIdentifyCluster(*identifyServer);
    ZbIdentifyCluster* identifyServer4 = new ZbIdentifyCluster(*identifyServer);
    ZbIdentifyCluster* identifyServer5 = new ZbIdentifyCluster(*identifyServer);
    
    // Attaching Clusters  ////////////////////////////////////////////////
    generalEp->addCluster(identifyServer);
    generalEp->addCluster(basicCl);
    generalEp->addCluster(_fMeter);
    generalEp->addCluster(_fMeter->getKfactorCluster());
    generalEp->addCluster(_otaCluster);

    inputEp->addCluster(identifyServer2);
    inputEp->addCluster(_upstreamPressure);
    inputEp->addCluster(_upstreamPressure->getKfactorCluster());

    interiorEp->addCluster(identifyServer3);
    interiorEp->addCluster(_downstreamPressure);
    interiorEp->addCluster(_downstreamPressure->getKfactorCluster());

    supplyEp->addCluster(identifyServer4);
    supplyEp->addCluster(_waterLevel);
    supplyEp->addCluster(_waterLevel->getKfactorCluster());
    
    exhaustEp->addCluster(identifyServer5);
    exhaustEp->addCluster(identifyClient);
    exhaustEp->addCluster(_tempMeasurement);
    exhaustEp->addCluster(_timeClient);

    // Attaching Enpoints  ////////////////////////////////////////////////
    _zbDevice->addEndPoint(*generalEp);
    _zbDevice->addEndPoint(*inputEp);
    _zbDevice->addEndPoint(*interiorEp);
    _zbDevice->addEndPoint(*supplyEp);
    _zbDevice->addEndPoint(*exhaustEp);
    
    
    // Get task handles from tasks names
    _eventLoopHandle = xTaskGetHandle( "ZbEventLoop" );
    _xButtonHandle = xTaskGetHandle( "button_task" ); 
    
    //driver_init();

    ESP_LOGI(TAG,"---------------- Starting ZbDevice ------------------------");
    //_zbDevice->setReadyCallback(initWhenJoined);
    
    //ZbApsData* inst = ZbApsData::getInstance();
    _zbDevice->start();

    vTaskDelay(pdMS_TO_TICKS(7000));

    // Start AdsDriver after zigbee stack as it will start to change attributes
    // 1000ms for each measures
    AdsDriver::getInstance().start(CONFIG_DELAY_ADS_DRIVER);

    _tempMeasurement->start(CONFIG_DELAY_TEMPERATURE_DRIVER);

}



void Main::run(void)
{
    
    ESP_LOGV(TAG,"Main task high water mark %d", 
                            uxTaskGetStackHighWaterMark(NULL));

    ESP_LOGV(TAG,"Event Loop task high water mark %d", 
                            uxTaskGetStackHighWaterMark(_eventLoopHandle));
    
    ESP_LOGV(TAG,"Button task high water mark %d", uxTaskGetStackHighWaterMark(_xButtonHandle));
    ESP_LOGV(TAG,"Zigbee task high water mark %d", 
                            uxTaskGetStackHighWaterMark(_zbDevice->getZbTask()));


    vTaskDelay(pdMS_TO_TICKS(2000));

    //ESP_LOGD(TAG,"App is running");
    
}


extern "C" void app_main(void)
{
    App.setup();

    while (true)
    {
        App.run();
    }    

    //should not reach here
}


