/*
  zTank 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#include "waterFlowMeasCluster.h"
#include <time.h>
#include <esp_log.h> 

//Static
void WaterFlowMeasCluster::impulsionHandler(void *handler_args, esp_event_base_t base, 
                                    int32_t id, void *event_data)
{
    WaterFlowMeasCluster* instance = *(static_cast<WaterFlowMeasCluster**>(event_data));

    // This interrupt is triggered on rising and falling edge (whatever)
    //if(instance->_irqMeter.read()){
       instance->_pulseCount += 1; 
       instance->setFlowMeasuredValue(instance->_pulseCount);
    //}
    ESP_LOGV(ZCLUSTER_TAG, "WaterFlow impulsion,  _pulseCount %d ", 
                        instance->_pulseCount);


}


// Device sent its tick without regarding kFactor. It only sent the number of tick
// and reset its counter to 0 each time the attribute is reported
// we use kFactor here only to save in to device NVS so
// that z2m will know this value whatever it happpen (restart, device quit network, ...)
WaterFlowMeasCluster::WaterFlowMeasCluster():
                                ZbFlowMeasCluster(false, 0, 0, ESP_ZB_ZCL_VALUE_U16_NONE - 1),
                                _Kfactor("Kfactor", 1.0f),
                                _pulseCount(0)
{
    // setup the embedded Kfactor cluster (analog value)
    _kfactorCluster = new ZbAnalogValueCluster(false, false, _Kfactor);
      
    
    _kfactorCluster->registerEventHandler(&WaterFlowMeasCluster::setKfactor, this);

    _irqMeter.enableInterrupt(GPIO_INTR_NEGEDGE);

    // Create a pointer of Pointer to be able to access to this in the Event Handler
    WaterFlowMeasCluster** ptr = new WaterFlowMeasCluster*(this);

    // System Event Loop
    esp_event_loop_create_default();    // Create System Event Loop
    _irqMeter.setEventHandler(&impulsionHandler, ptr);
    
}

void WaterFlowMeasCluster::setupResetTask(uint64_t secondsFromMidnight)
{
    _secondsFromMidnight = secondsFromMidnight;
    // get current local time using C (as tz is set for C only)
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    uint64_t currentSecFromMidnight = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;

    uint64_t delayToReset;
    if (secondsFromMidnight > currentSecFromMidnight)
        delayToReset =  ( secondsFromMidnight - currentSecFromMidnight ) * 1000;
    else
        delayToReset = ( secondsFromMidnight - currentSecFromMidnight + 86400 ) *1000; // add 24h

    ESP_LOGV(ZCLUSTER_TAG, "WaterFlowMeasCluster -setupResetTask -  current from midnight: %lld sec - target: %lld sec - delay : %lld ms", 
                                    currentSecFromMidnight, secondsFromMidnight, delayToReset);

    if(_resetTask)
        _resetTask->startTimer(delayToReset);
    else
        _resetTask = new ScheduledTask(&WaterFlowMeasCluster::resetCounter, 
                                        this,
                                        delayToReset,
                                        std::string("WaterFlowResetTask"),
                                        false);

}

ZbCluster* WaterFlowMeasCluster::getKfactorCluster()
{
    return _kfactorCluster;
}


void WaterFlowMeasCluster::resetCounter()
{
    ESP_LOGV(ZCLUSTER_TAG, "WaterFlow resetting to 0", 
                        _pulseCount);

    _pulseCount = 0; 
    setFlowMeasuredValue(_pulseCount);

    setupResetTask(_secondsFromMidnight);

}


void WaterFlowMeasCluster::setKfactor(clusterEvent_t event, std::vector<attribute_t> attrs)
{
    if (event != ATTR_UPDATED_REMOTELY)
        return;
    
    for (auto & el : attrs){
        uint16_t attrId = el.attrId;
        void* value = el.value;
        if (attrId == ESP_ZB_ZCL_ATTR_ANALOG_VALUE_PRESENT_VALUE_ID){
            ESP_LOGV(ZCLUSTER_TAG, "WaterFlowMeas setKfactor event : %d", 
                event);

            float_t currentFactor = *(static_cast<float_t*>(value));
            ESP_LOGV(ZCLUSTER_TAG, "WaterFlowMeas setKfactor currentFactor : : %f", 
                currentFactor);
            
            //_kfactorCluster.setAttribute(ESP_ZB_ZCL_ATTR_ANALOG_VALUE_PRESENT_VALUE_ID,
            //                                (void*)&currentFactor);
            _Kfactor = currentFactor;
            _Kfactor.save(); // Save will write in the NVS
        }
    }
    
}

void WaterFlowMeasCluster::setFlowMeasuredValue(uint16_t newValue)
{
    setAttribute(ESP_ZB_ZCL_ATTR_FLOW_MEASUREMENT_VALUE_ID,
                &newValue);
}