/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "zbMultistateValueCluster.h"
#include "aldesDriver.h"


/// @brief Store user demand and read current state
/// @brief MultistateValueCluster
/// @param PresentValue current mode set by the user
class SetPointCluster: public ZbMultistateValueCluster
{
    
    typedef enum {
        ZB_VMC_ATTR_CURRENT_LEVEL_ID   = 0xff00,
    } zb_vmc_setpoint_attr_t;

    AldesDriver* _aldes = nullptr;

public:
    SetPointCluster(AldesDriver* aldes):ZbMultistateValueCluster(false, 6),  _aldes(aldes) {
        
        uint16_t value = 0;
        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_CURRENT_LEVEL_ID,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY |
                                                ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                                            &value);

        registerEventHandler(&SetPointCluster::onAttrChange, this);

    };

    // Called when Modbus is sending data
    void setDemandPoint(int16_t mode) {
        
        ESP_LOGV(ZCLUSTER_TAG, "setDemand : %d", 
                        mode);
        // will be read as uint16_t
      	setAttribute(ESP_ZB_ZCL_ATTR_MULTI_VALUE_PRESENT_VALUE_ID,
                &mode);
    }

    // Called when Modbus is sending data
    void setCurrentLevel(int16_t lvl) {
        
        ESP_LOGV(ZCLUSTER_TAG, "current Level : %d", 
                        lvl);
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_CURRENT_LEVEL_ID,
                &lvl, CONFIG_MANUF_CODE); 
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
                case ESP_ZB_ZCL_ATTR_MULTI_VALUE_PRESENT_VALUE_ID: {
                    uint16_t newVal = *(static_cast<uint16_t*>(value));
                    ESP_LOGV(ZCLUSTER_TAG, "Setpoint set to : %d", 
                        newVal);
                    _aldes->setDemandPoint(newVal);
                    break; }
                default: {
                    ESP_LOGW(ZCLUSTER_TAG, 
                        "SetPointCluster - Unknown remotly changed attribute id: %d", 
                        attrId);
                    break; }
            }
        }

    }

}; // SetPointCluster
