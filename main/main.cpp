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

#include "zbEndpoint.h" // to del


#include "AldesModbus.h"

static const char *TAG = "Main_app";
BlinkTask* Main::_ledBlinking = nullptr;
GpioOutput Main::_led { (gpio_num_t)CONFIG_PIN_LED }; //TODO led pin number in config file

Main App;

Main::Main()
{
    // Setting the log level for each module
    esp_log_level_set("Main_app", ESP_LOG_DEBUG);  // Put verbose to check available stack
    esp_log_level_set("Modbus", ESP_LOG_INFO); 
    esp_log_level_set("AldesDriver", ESP_LOG_INFO);
    esp_log_level_set("ZbNode", ESP_LOG_DEBUG); 
    esp_log_level_set("ZbEndpoint", ESP_LOG_VERBOSE);
    esp_log_level_set("ZbCluster", ESP_LOG_INFO);
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
            // Wait for the clock to be synchronized (retry every seconds)
            _checkRTCSync = new PeriodicSoftTask(&Main::checkRTCSync, this, 500);
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

        ESP_LOGV(TAG,"Clock is sync - triggering action on RTC");

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

    // switch off led on UART1 as it is not used
    _tx1.on();
    _rx1.enablePullup();
    _rts1.off();

    _aldes = new AldesDriver();


    ESP_LOGI(TAG,"Creating Zigbee device");
    _zbDevice = ZbNode::getInstance();
    _zbDevice->registerNodeEventHandler(&Main::zbDeviceEventHandler, this);

    // Endpoints Construct ////////////////////////////////////////////////
    ZbEndPoint* generalEp = new ZbEndPoint(CONFIG_GENERAL_EP, 
                            ESP_ZB_HA_METER_INTERFACE_DEVICE_ID);
    
    ZbEndPoint* intakeOutEp = new ZbEndPoint(CONFIG_INTAKE_OUT_EP, 
                            ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID);

    ZbEndPoint* extractInEp = new ZbEndPoint(CONFIG_EXTRACT_IN_EP, 
                            ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID);
    
    ZbEndPoint* supplyInEp = new ZbEndPoint(CONFIG_SUPPLY_IN_EP, 
                            ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID);
    
    ZbEndPoint* exhaustOutEp = new ZbEndPoint(CONFIG_EXHAUST_OUT_EP, 
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
    

    // Temperatures
    _TIntakeOut     = new TSensorCluster();
    _TExtractIn     = new TSensorCluster();
    _TSupplyIn      = new TSensorCluster();
    _TExhaustOut    = new TSensorCluster();

    _filterTimerCluster     = new FilterTimerCluster(_aldes);
    _tBypassCluster         = new BypassCluster(_aldes);
    _setPointCluster        = new SetPointCluster(_aldes);
    
    _airflowMVECluster      = new AirflowCluster();
    _airflowMVICluster      = new AirflowCluster();

    _flowSettingsCluster    = new FlowSettingsCluster(_aldes);

    //_aldes->registerAldesHandler(&SetPointCluster::setDemandPoint, 
    //                    _setPointCluster,AldesModbus::REG_DEMAND_USER);
    _aldes->registerAldesHandler(&SetPointCluster::setCurrentLevel, 
                        _setPointCluster,AldesModbus::REG_CURRENT_LEVEL);

    _aldes->registerAldesHandler(&TSensorCluster::setTemperatureMeasuredValue, 
                        _TIntakeOut, AldesModbus::REG_T_INTAKE_AIR_OUT);
    _aldes->registerAldesHandler(&TSensorCluster::setTemperatureMeasuredValue, 
                        _TExtractIn, AldesModbus::REG_T_EXTRACT_AIR_IN);
    _aldes->registerAldesHandler(&TSensorCluster::setTemperatureMeasuredValue, 
                        _TSupplyIn, AldesModbus::REG_T_SUPPLY_AIR_IN);
    _aldes->registerAldesHandler(&TSensorCluster::setTemperatureMeasuredValue, 
                        _TExhaustOut, AldesModbus::REG_T_EXHAUST_AIR_OUT);
    
    _aldes->registerAldesHandler(&FilterTimerCluster::setFilterTempo, 
                        _filterTimerCluster,AldesModbus::REG_TEMPO_FILTER);
    _aldes->registerAldesHandler(&FilterTimerCluster::setFilterTimerValue, 
                        _filterTimerCluster,AldesModbus::REG_FILTER_STATE_DAYS);
    
    _aldes->registerAldesHandler(&BypassCluster::setBypassTemperature, 
                        _tBypassCluster,AldesModbus::REG_T_BYPASS_SUMMER);
    _aldes->registerAldesHandler(&BypassCluster::setBypassPosition, 
                        _tBypassCluster,AldesModbus::REG_BYPASS_POSITION);
    _aldes->registerAldesHandler(&BypassCluster::setSeasonDetected, 
                        _tBypassCluster,AldesModbus::REG_SEASON_DETECTION);
    
    _aldes->registerAldesHandler(&AirflowCluster::setAirflow, 
                        _airflowMVECluster,AldesModbus::REG_AIRFLOW_MVE);
    _aldes->registerAldesHandler(&AirflowCluster::setAirflow, 
                        _airflowMVICluster,AldesModbus::REG_AIRFLOW_MVI);
    
    _aldes->registerAldesHandler(&FlowSettingsCluster::setVacationLevel, 
                        _flowSettingsCluster,AldesModbus::REG_SETTING_MVE_VACATION);
    _aldes->registerAldesHandler(&FlowSettingsCluster::setDailyLevel, 
                        _flowSettingsCluster,AldesModbus::REG_SETTING_MVE_DAILY);
    _aldes->registerAldesHandler(&FlowSettingsCluster::setPushButtonLevel, 
                        _flowSettingsCluster,AldesModbus::REG_SETTING_MVE_PUSH_BUTTON);
    _aldes->registerAldesHandler(&FlowSettingsCluster::setBoostLevel, 
                        _flowSettingsCluster,AldesModbus::REG_SETTING_MVE_BOOST);
    _aldes->registerAldesHandler(&FlowSettingsCluster::setMaxSpeedLevel, 
                        _flowSettingsCluster,AldesModbus::REG_SETTING_MVE_MAX_SPEED); 


    _timeClient = new ZbTimeClusterClient();

    ESP_LOGI(TAG,"---------------- Register ------------------------");
    
    ZbIdentifyCluster* identifyServer = new ZbIdentifyCluster();
    //ZbIdentifyCluster* identifyClient = new ZbIdentifyCluster(true);

    
    // Attaching Clusters  ////////////////////////////////////////////////
    generalEp->addCluster(identifyServer);
    generalEp->addCluster(basicCl);
    generalEp->addCluster(_timeClient);
    generalEp->addCluster(_filterTimerCluster);
    generalEp->addCluster(_flowSettingsCluster);
    generalEp->addCluster(_tBypassCluster);
    generalEp->addCluster(_setPointCluster);
    //generalEp->addCluster(_otaCluster);

    //intakeOutEp->addCluster(identifyServer2);
    intakeOutEp->addCluster(_TIntakeOut);

    //extractInEp->addCluster(identifyServer3);
    extractInEp->addCluster(_TExtractIn);
    extractInEp->addCluster(_airflowMVECluster);


    //supplyInEp->addCluster(identifyServer4);
    supplyInEp->addCluster(_TSupplyIn);
    supplyInEp->addCluster(_airflowMVICluster);
    
    //exhaustOutEp->addCluster(identifyServer5);
    exhaustOutEp->addCluster(_TExhaustOut);


    // Attaching Enpoints  ////////////////////////////////////////////////
    _zbDevice->addEndPoint(*generalEp);
    _zbDevice->addEndPoint(*intakeOutEp);
    _zbDevice->addEndPoint(*extractInEp);
    _zbDevice->addEndPoint(*supplyInEp);
    _zbDevice->addEndPoint(*exhaustOutEp);
    
    // Get task handles from tasks names
    _eventLoopHandle = xTaskGetHandle( "ZbEventLoop" );
    _xButtonHandle = xTaskGetHandle( "button_task" ); 
    _mbHandle = xTaskGetHandle( "mbc_ser_master" ); 
    
    ESP_LOGI(TAG,"---------------- Starting ZbDevice ------------------------");
    //_zbDevice->setReadyCallback(initWhenJoined);
    
    //ZbApsData* inst = ZbApsData::getInstance();
    _zbDevice->start();

    vTaskDelay(pdMS_TO_TICKS(7000));

    _aldes->start(CONFIG_VMC_POLLING_MS, CONFIG_VMC_POLLING_GEN_DATA);

}



void Main::run(void)
{
    ESP_LOGV(TAG,"Modbus task high water mark %d", 
                            uxTaskGetStackHighWaterMark(_mbHandle ));
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


