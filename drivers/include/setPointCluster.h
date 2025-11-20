/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "zbMultistateValueCluster.h"
#include "aldesDriver.h"

// T° bypass summer of aldes VMC
class SetPointCluster: public ZbMultistateValueCluster
{
    AldesDriver* _aldes = nullptr;

public:
    SetPointCluster(AldesDriver* aldes):ZbMultistateValueCluster(false, 6),  _aldes(aldes) {

        registerEventHandler(&SetPointCluster::onAttrChange, this);

    };


    // Called when Modbus is sending data
    void setDemandPoint(int16_t mode) {
        
        ESP_LOGW(ZCLUSTER_TAG, "setDemand : %d", 
                        mode);
        // will be read as uint16_t
      	setAttribute(ESP_ZB_ZCL_ATTR_MULTI_VALUE_PRESENT_VALUE_ID,
                &mode);
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
