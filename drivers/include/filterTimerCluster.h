/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "zbFlowMeasCluster.h"


// Store remaining days prior to a filter change is required
class FilterTimerCluster: public ZbFlowMeasCluster
{

public:
    FilterTimerCluster() {};
    void setFilterTimerValue(int16_t newRemainingDays) {
      	setAttribute(ESP_ZB_ZCL_ATTR_FLOW_MEASUREMENT_VALUE_ID,
                &newRemainingDays);
    }

	void setFilterTempo(int16_t maxDays) {
      	setAttribute(ESP_ZB_ZCL_ATTR_FLOW_MEASUREMENT_MAX_VALUE_ID,
                &maxDays);
    }

}; // FilterTimerCluster