/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "zbFlowMeasCluster.h"
#include "aldesDriver.h"

// Store remaining days prior to a filter change is required
class FilterTimerCluster: public ZbFlowMeasCluster
{
    typedef enum {
        ZB_VMC_ATTR_FILTER_STATE_ID        = 0xff00, // Filter state in days
        ZB_VMC_ATTR_TEMPO_FILTER_ID        = 0xff01, // Filter Tempo in month
    } zb_vmc_filters_attr_t;

    AldesDriver* _aldes = nullptr;

public:
    FilterTimerCluster(AldesDriver* aldes):ZbFlowMeasCluster(),  _aldes(aldes) {

        // Adding custom attribute for tempo filter (read write required)
        uint16_t value = 0;

        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_FILTER_STATE_ID,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | 
                                                ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                                            &value);
        
        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_TEMPO_FILTER_ID,
                                            CONFIG_MANUF_CODE,
                                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
                                            &value);

        registerEventHandler(&FilterTimerCluster::onAttrChange, this);

    };

    void setFilterTimerValue(int16_t newRemainingDays) {
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_FILTER_STATE_ID,
                &newRemainingDays);
    }

	void setFilterTempo(int16_t tempoMonths) {

        setAttribute(ZB_VMC_ATTR_TEMPO_FILTER_ID,
                &tempoMonths, CONFIG_MANUF_CODE); 
    }

    void onAttrChange(clusterEvent_t event, 
                std::vector<attribute_t> attrs) {
        
        if (event != ZbCluster::ATTR_UPDATED_REMOTELY)
            return;
    
        for (auto & el : attrs){
            uint16_t attrId = el.attrId;
            void* value = el.value;

            switch(attrId) {
                case ZB_VMC_ATTR_FILTER_STATE_ID: {
                    uint16_t newVal = *(static_cast<uint16_t*>(value)) * 24;
                    ESP_LOGD(ZCLUSTER_TAG, "New filter state : %d hours", 
                        newVal);
                    _aldes->setFilterTimer(newVal);
                    break; }
                case ZB_VMC_ATTR_TEMPO_FILTER_ID: {
                    uint16_t newTempo = *(static_cast<uint16_t*>(value));
                    ESP_LOGD(ZCLUSTER_TAG, "New tempo : %d", 
                        newTempo);
                    _aldes->setFilterTempo(newTempo);
                    break; }
                default: {
                    ESP_LOGW(ZCLUSTER_TAG, 
                        "FilterTimerCluster - Unknown remotly changed attribute id: %d", 
                        attrId);
                    break; }
            }
        }

    }

}; // FilterTimerCluster
