/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "zbAnalogValueCluster.h"
#include "aldesDriver.h"

// Store remaining days prior to a filter change is required
class FlowSettingsCluster: public ZbAnalogValueCluster
{
    typedef enum {
        ZB_VMC_ATTR_VACATION_LEVEL_ID       = 0xff00, // Airflow in m3/h
        ZB_VMC_ATTR_DAILY_LEVEL_ID          = 0xff01, // Airflow in m3/h
        ZB_VMC_ATTR_PUSHBUTTON_LEVEL_ID     = 0xff02, // Airflow in m3/h
        ZB_VMC_ATTR_BOOST_LEVEL_ID          = 0xff03, // Airflow in m3/h
        ZB_VMC_ATTR_MAXSPEED_LEVEL_ID       = 0xff04, // Airflow in m3/h
    } zb_vmc_filters_attr_t;

    AldesDriver* _aldes = nullptr;

public:
    FlowSettingsCluster(AldesDriver* aldes):ZbAnalogValueCluster(),  _aldes(aldes) {

        // Adding custom attribute for tempo filter (read write required)
        uint16_t value = 0;

        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_VACATION_LEVEL_ID,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
                                            &value);
        
        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_DAILY_LEVEL_ID,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
                                            &value);

        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_PUSHBUTTON_LEVEL_ID ,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
                                            &value);

        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_BOOST_LEVEL_ID,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
                                            &value);

        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_MAXSPEED_LEVEL_ID,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
                                            &value);
                             

        registerEventHandler(&FlowSettingsCluster::onAttrChange, this);

    };


    // Called when Modbus is sending data
    void setVacationLevel(int16_t flowrate) {
        ESP_LOGW(ZCLUSTER_TAG, "setVacationLevel : %d", 
                        flowrate);
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_VACATION_LEVEL_ID,
                &flowrate, CONFIG_MANUF_CODE);
    }

    // Called when Modbus is sending data
    void setDailyLevel(int16_t flowrate) {
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_DAILY_LEVEL_ID,
                &flowrate, CONFIG_MANUF_CODE);
    }

    // Called when Modbus is sending data
    void setPushButtonLevel(int16_t flowrate) {
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_PUSHBUTTON_LEVEL_ID,
                &flowrate, CONFIG_MANUF_CODE);
    }

    // Called when Modbus is sending data
    void setBoostLevel(int16_t flowrate) {
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_BOOST_LEVEL_ID,
                &flowrate, CONFIG_MANUF_CODE);
    }

    // Called when Modbus is sending data
    void setMaxSpeedLevel(int16_t flowrate) {
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_MAXSPEED_LEVEL_ID,
                &flowrate, CONFIG_MANUF_CODE);
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
                case ZB_VMC_ATTR_VACATION_LEVEL_ID: {
                    uint16_t newVal = *(static_cast<uint16_t*>(value)) * 24;
                    ESP_LOGD(ZCLUSTER_TAG, "New filter state : %d hours", 
                        newVal);
                    //_aldes->setFilterTimer(newVal);
                    break; }
                case ZB_VMC_ATTR_DAILY_LEVEL_ID: {
                    uint16_t newTempo = *(static_cast<uint16_t*>(value));
                    ESP_LOGD(ZCLUSTER_TAG, "New tempo : %d", 
                        newTempo);
                    //_aldes->setFilterTempo(newTempo);
                    break; }
                case ZB_VMC_ATTR_PUSHBUTTON_LEVEL_ID: {
                    uint16_t newTempo = *(static_cast<uint16_t*>(value));
                    ESP_LOGD(ZCLUSTER_TAG, "New tempo : %d", 
                        newTempo);
                   // _aldes->setFilterTempo(newTempo);
                    break; }
                case ZB_VMC_ATTR_BOOST_LEVEL_ID: {
                    uint16_t newTempo = *(static_cast<uint16_t*>(value));
                    ESP_LOGD(ZCLUSTER_TAG, "New tempo : %d", 
                        newTempo);
                    //_aldes->setFilterTempo(newTempo);
                    break; }
                case ZB_VMC_ATTR_MAXSPEED_LEVEL_ID: {
                    uint16_t newTempo = *(static_cast<uint16_t*>(value));
                    ESP_LOGD(ZCLUSTER_TAG, "New tempo : %d", 
                        newTempo);
                    //_aldes->setFilterTempo(newTempo);
                    break; }

                default: {
                    ESP_LOGW(ZCLUSTER_TAG, 
                        "FlowSettingsCluster - Unknown remotly changed attribute id: %d", 
                        attrId);
                    break; }
            }
        }

    }

}; // FilterTimerCluster
