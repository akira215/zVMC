/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "zbAnalogInputCluster.h"
#include "aldesDriver.h"

// T° bypass summer of aldes VMC
class BypassCluster: public ZbAnalogInputCluster
{
    typedef enum {
        ZB_VMC_ATTR_BYPASS_POSITION_ID  = 0xff00, // "0:Undefined | 1:Open | 2:Closed 45° | 3:Closed"
        ZB_VMC_ATTR_SEASON_DETECTION_ID = 0xff01, // "0:Undefined | 1:Winter | 2:Summer"
    } zb_vmc_bypass_attr_t;

    AldesDriver* _aldes = nullptr;

public:
    BypassCluster(AldesDriver* aldes):ZbAnalogInputCluster(),  _aldes(aldes) {
        
        // Adding custom attribute for bypass current position
        uint16_t value = 0;

        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_BYPASS_POSITION_ID,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | 
                                                ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                                            &value);
        
        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_SEASON_DETECTION_ID,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | 
                                                ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                                            &value);

        registerEventHandler(&BypassCluster::onAttrChange, this);

    };


    // Called when Modbus is sending data
    void setBypassTemperature(int16_t temperature) {
        
        float_t temp = static_cast<float_t>(temperature);
        ESP_LOGV(ZCLUSTER_TAG, "setBypassTemperature : %f", 
                        temp);
        // will be read as uint16_t
      	setAttribute(ESP_ZB_ZCL_ATTR_ANALOG_VALUE_PRESENT_VALUE_ID,
                &temp);
    }

    // Called when Modbus is sending data
    void setBypassPosition(int16_t position) {
        
        ESP_LOGW(ZCLUSTER_TAG, "setBypassPosition : %d", 
                        position);
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_BYPASS_POSITION_ID,
                &position, CONFIG_MANUF_CODE); 
    }

    // Called when Modbus is sending data
    void setSeasonDetected(int16_t season) {
        
        ESP_LOGW(ZCLUSTER_TAG, "set season : %d", 
                        season);
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_SEASON_DETECTION_ID,
                &season, CONFIG_MANUF_CODE); 
    }

    // Called when Zigbee is sending data
    void onAttrChange(clusterEvent_t event, 
                std::vector<attribute_t> attrs) {
        
        if (event != ZbCluster::ATTR_UPDATED_REMOTELY)
            return;
    
        for (auto & el : attrs){
            uint16_t attrId = el.attrId;
            void* value = el.value;

            switch(attrId) {
                case ESP_ZB_ZCL_ATTR_ANALOG_VALUE_PRESENT_VALUE_ID: {
                    int16_t newVal = static_cast<int16_t>(*(static_cast<float_t*>(value)));
                    ESP_LOGV(ZCLUSTER_TAG, "New T° Bypass summer : %d °C", 
                        newVal);
                    _aldes->setBypassTemperature(newVal);
                    break; }
                default: {
                    ESP_LOGW(ZCLUSTER_TAG, 
                        "BypassTemperatureCluster - Unknown remotly changed attribute id: %d", 
                        attrId);
                    break; }
            }
        }

    }

}; // BypassCluster
