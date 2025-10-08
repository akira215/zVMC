/*
  zTank 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h> // TODEL development purpose

#include "waterLevelMeasCluster.h"
#include "adsDriver.h"

#include <math.h> // for round


// Static handler when conversion is received
void WaterLevelMeasCluster::adc_event_handler(double value)
{
    uint16_t attr = (uint16_t)(round(value * _Kfactor));

    ESP_LOGV(ZCLUSTER_TAG, "WaterLevel ch %d ADC Callback - value: %f - attr: %d", 
                _channel, value, attr);

    setLevelMeasuredValue(attr);
}


// pFactor = 37.594 with shunt = 133R and sensor max 0-2m:
// Imax = 20mA => Umax = 2.66V (=> 2m) 100 / 2.66 = 37.594
// measuredValue is send in kPa 
WaterLevelMeasCluster::WaterLevelMeasCluster(uint8_t channel):
                        ZbHumidityMeasCluster(false, 0, 0, ESP_ZB_ZCL_VALUE_U16_NONE - 1),
                        _channel(channel),
                        _Kfactor("levFactor", 37.594f)
{
    
    // PersistedValue::getValue() is required in the macro ESP_LOGX otherwise the PersistedValue obj is destroyed
    // and built again
    ESP_LOGV(ZCLUSTER_TAG, 
                "WaterLevelMeas Constructor channel %d - levFactor = %f", 
                channel, _Kfactor.getValue()); 
    
    // setup the embedded pFactor cluster (analog value)
    _kfactorCluster = new ZbAnalogValueCluster(false, false, _Kfactor);

    _kfactorCluster->registerEventHandler(&WaterLevelMeasCluster::setKfactor, this);

    // Register the callback handler for ADC converter
    // WARNING, do not start the ADC driver prior to start the zigbee stack
    // Otherwise it will try to set an attribute and will fail
    AdsDriver::getInstance().registerAdsHandler(
                    &WaterLevelMeasCluster::adc_event_handler, this, channel);
}

ZbCluster* WaterLevelMeasCluster::getKfactorCluster()
{
    return _kfactorCluster;
}

void WaterLevelMeasCluster::setKfactor(clusterEvent_t event, std::vector<attribute_t> attrs)
{
    if (event != ATTR_UPDATED_REMOTELY)
        return;
    
    for (auto & el : attrs){
        uint16_t attrId = el.attrId;
        void* value = el.value;
        if (attrId == ESP_ZB_ZCL_ATTR_ANALOG_VALUE_PRESENT_VALUE_ID){
            ESP_LOGV(ZCLUSTER_TAG, "WaterLevel setKfactor event : %d", 
                event);

            float_t currentFactor = *(static_cast<float_t*>(value));
            ESP_LOGV(ZCLUSTER_TAG, "WaterLevel setKfactor currentFactor : : %f", 
                currentFactor);
            
            _Kfactor = currentFactor;
            _Kfactor.save(); // Save will write in the NVS
        }
    }
    
}

void WaterLevelMeasCluster::setLevelMeasuredValue(uint16_t newValue)
{
    setAttribute(ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_ID,
                &newValue);
}