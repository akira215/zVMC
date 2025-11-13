/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "zbCustomCluster.h"
#include "zbFlowMeasCluster.h"
/*
# define ZB_VMC_CLUSTER_ID_FILTER_TIMER 0xfc00 // to move in an enum

// Store remaining days prior to a filter change is required
class FilterTimerCluster: public ZbCustomCluster
{
  typedef enum {
    ZB_VMC_ATTR_FILTER_STATE_VALUE_ID        = 0x0000, // Current elapsed days
    ZB_VMC_ATTR_TEMPO_FILTER_VALUE_ID        = 0x0002, // Filter Tempo in month
  } zb_vmc_filters_attr_t;


public:
    FilterTimerCluster():ZbCustomCluster(false,ZB_VMC_CLUSTER_ID_FILTER_TIMER) {
        int16_t defaultVal = 0;
        addCustomAttribute(ZB_VMC_ATTR_FILTER_STATE_VALUE_ID,
                        &defaultVal,
                        ESP_ZB_ZCL_ATTR_TYPE_16BIT,
                        ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING );
        addCustomAttribute(ZB_VMC_ATTR_TEMPO_FILTER_VALUE_ID,
                        &defaultVal,
                        ESP_ZB_ZCL_ATTR_TYPE_16BIT,
                        ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE);
    };


    void setFilterTimerValue(int16_t newRemainingDays) {
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_FILTER_STATE_VALUE_ID,
                &newRemainingDays);
    }

	void setFilterTempo(int16_t maxDays) {
        // will be read as uint16_t
      	setAttribute(ZB_VMC_ATTR_TEMPO_FILTER_VALUE_ID,
                &maxDays);
    }

}; // FilterTimerCluster

*/


// Store remaining days prior to a filter change is required
class FilterTimerCluster: public ZbFlowMeasCluster
{
    typedef enum {
        ZB_VMC_ATTR_FILTER_STATE_VALUE_ID        = 0x0000, // Current elapsed days
        ZB_VMC_ATTR_TEMPO_FILTER_VALUE_ID        = 0xff00, // Filter Tempo in month
    } zb_vmc_filters_attr_t;

public:
    FilterTimerCluster():ZbFlowMeasCluster() {

        // Adding custom attribute for tempo filter (read write required)
        int16_t value = 0;

        esp_zb_cluster_add_manufacturer_attr(_attr_list, 
                                            getId(),
                                            ZB_VMC_ATTR_TEMPO_FILTER_VALUE_ID,
                                            0x1234,
                                            ESP_ZB_ZCL_ATTR_TYPE_S16,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
                                            &value);


    };
    void setFilterTimerValue(int16_t newRemainingDays) {
        // will be read as uint16_t
      	setAttribute(ESP_ZB_ZCL_ATTR_FLOW_MEASUREMENT_VALUE_ID,
                &newRemainingDays);
    }

	void setFilterTempo(int16_t maxDays) {
        // will be read as uint16_t
      	//setAttribute(ESP_ZB_ZCL_ATTR_FLOW_MEASUREMENT_MAX_VALUE_ID,
        //        &maxDays);
        
        setAttribute(ZB_VMC_ATTR_TEMPO_FILTER_VALUE_ID,
                &maxDays, 0x1234);
    }

}; // FilterTimerCluster
