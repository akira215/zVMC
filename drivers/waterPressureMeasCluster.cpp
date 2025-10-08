/*
  zTank 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h> // TODEL development purpose

#include "waterPressureMeasCluster.h"
#include "adsDriver.h"

#include <string>
#include <math.h> // for round


// Static handler when conversion is received
void WaterPressureMeasCluster::adc_event_handler(double value)
{
    int16_t attr = (int16_t)(round(value * _Kfactor));

    ESP_LOGV(ZCLUSTER_TAG, "WaterPressure ch %d ADC Callback - value: %f - attr: %d", 
                _channel, value, attr);

    setPressureMeasuredValue(attr);
}


// pFactor = 187.97 with shunt = 133R and sensor max 0.5 MPa:
// Imax = 20mA => Umax = 2.66V (=> 500 kPa) 500 / 2.66 = 187.97
// measuredValue is send in kPa 
WaterPressureMeasCluster::WaterPressureMeasCluster(uint8_t channel):
                        ZbPressureMeasCluster(false, 0, 0, ESP_ZB_ZCL_VALUE_U16_NONE - 1),
                        _channel(channel),
                        //_Kfactor(std::string("pFactor").append(std::to_string(channel)), 187.97f)
                        _Kfactor(std::string("pFactor").append(std::to_string(channel)), 187.97f)
{
    
    // PersistedValue::getValue() is required in the macro ESP_LOGX otherwise the PersistedValue obj is destroyed
    // and built again
    ESP_LOGV(ZCLUSTER_TAG, 
                "WaterPressureMeas Constructor channel %d - pFactor%d = %f", 
                channel, channel, _Kfactor.getValue()); 
    
    // setup the embedded pFactor cluster (analog value)
    _kfactorCluster = new ZbAnalogValueCluster(false, false, _Kfactor);

    _kfactorCluster->registerEventHandler(&WaterPressureMeasCluster::setKfactor, this);

    // Register the callback handler for ADC converter
    // WARNING, do not start the ADC driver prior to start the zigbee stack
    // Otherwise it will try to set an attribute and will fail
    AdsDriver::getInstance().registerAdsHandler(
                    &WaterPressureMeasCluster::adc_event_handler, this, channel);
}

ZbCluster* WaterPressureMeasCluster::getKfactorCluster()
{
    return _kfactorCluster;
}

void WaterPressureMeasCluster::setKfactor(clusterEvent_t event, std::vector<attribute_t> attrs)
{
    if (event != ATTR_UPDATED_REMOTELY)
        return;
    
    for (auto & el : attrs){
        uint16_t attrId = el.attrId;
        void* value = el.value;
        if (attrId == ESP_ZB_ZCL_ATTR_ANALOG_VALUE_PRESENT_VALUE_ID){
            ESP_LOGV(ZCLUSTER_TAG, "WaterPressure setKfactor event : %d", 
                event);

            float_t currentFactor = *(static_cast<float_t*>(value));
            ESP_LOGV(ZCLUSTER_TAG, "WaterPressure setKfactor currentFactor : : %f", 
                currentFactor);
            
            //_kfactorCluster.setAttribute(ESP_ZB_ZCL_ATTR_ANALOG_VALUE_PRESENT_VALUE_ID,
            //                                (void*)&currentFactor);
            _Kfactor = currentFactor;
            _Kfactor.save(); // Save will write in the NVS
        }
    }
    
}

void WaterPressureMeasCluster::setPressureMeasuredValue(int16_t newValue)
{
    setAttribute(ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_ID,
                &newValue);
}