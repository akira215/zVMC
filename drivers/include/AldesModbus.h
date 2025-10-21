/*
  Aldes Modbus params
  Repository: https://github.com/akira215/esp-ash-components
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/
#pragma once

namespace AldesModbus
{
// Enumeration of modbus device addresses accessed by master device
    enum {
        MB_ALDES_ADDR = 2 // ALDES INSPIRAIR Slave addr (add other slave addresses here)
    };

  

    // Enumeration of register from Aldes Inspirair Top device
    enum {
        REG_PRODUCT_CODE            = ( 0x0001 ),   // 2 WORD
        REG_SERIAL_NUMBER           = ( 0x0003 ),   // 4 WORD
        REG_SOFT_VERSION            = ( 0x000c ),   // 1 WORD
        REG_USER_LEVEL              = ( 0x0010 ),   // 1 WORD
        REG_REGULATION_MODE         = ( 0x0100 ),   // 1 WORD
        REG_DEMAND_USER             = ( 0x0101 ),   // 1 WORD
        REG_DEMAND_PROGRAMMER       = ( 0x0102 ),   // 1 WORD
        REG_BYPASS_MODE             = ( 0x0103 ),   // 1 WORD
        REG_TEMPO_FILTER            = ( 0x010b ),   // 1 WORD
        REG_UNBALANCED_COEF_MVI     = ( 0x0116 ),   // 1 WORD
        REG_FILTER_STATE_PERCENT    = ( 0x015a ),   // 1 WORD
        REG_FILTER_STATE_DAYS       = ( 0x015b ),   // 1 WORD
        REG_BYPASS_POSITION         = ( 0x015c ),   // 1 WORD
        REG_T_INTAKE_AIR_OUT        = ( 0x015e ),   // 1 WORD
        REG_T_EXTRACT_AIR_IN        = ( 0x015f ),   // 1 WORD
        REG_T_SUPPLY_AIR_IN         = ( 0x0160 ),   // 1 WORD
        REG_T_EXHAUST_AIR_OUT       = ( 0x0161 ),   // 1 WORD
        REG_SPEED_EXHAUST_FAN       = ( 0x0162 ),   // 1 WORD
        REG_SPEED_SUPPLY_FAN        = ( 0x0163 ),   // 1 WORD
        REG_AIRFLOW_MVE             = ( 0x0164 ),   // 1 WORD
        REG_AIRFLOW_MVI             = ( 0x0165 ),   // 1 WORD
        REG_PRESSURE                = ( 0x0166 ),   // 1 WORD
        REG_SEASON_DETECTION        = ( 0x0175 ),   // 1 WORD
        REG_ERROR_CODE              = ( 0x0180 ),   // 1 WORD
        REG_T_BYPASS_SUMMER         = ( 0x01ec ),   // 1 WORD
        REG_T_BYPASS_FULL_OPEN_CLOSE= ( 0x0300 ),   // 1 WORD
        REG_T_BYPASS_OPEN_ANGLE     = ( 0x0301 ),   // 1 WORD
        REG_T_BYPASS_CLOSE_ANGLE    = ( 0x0302 ),   // 1 WORD
        REG_FAN_CONFIG              = ( 0x0404 ),   // 1 WORD
        REG_SETTING_MVE_VACATION    = ( 0x0410 ),   // 1 WORD
        REG_SETTING_MVI_VACATION    = ( 0x0411 ),   // 1 WORD
        REG_SETTING_MVE_DAILY       = ( 0x0412 ),   // 1 WORD
        REG_SETTING_MVI_DAILY       = ( 0x0413 ),   // 1 WORD
        REG_SETTING_MVE_PUSH_BUTTON = ( 0x0414 ),   // 1 WORD
        REG_SETTING_MVI_PUSH_BUTTON = ( 0x0415 ),   // 1 WORD
        REG_SETTING_MVE_BOOST       = ( 0x0416 ),   // 1 WORD
        REG_SETTING_MVI_BOOST       = ( 0x0417 ),   // 1 WORD
        REG_SETTING_MVE_MAX_SPEED   = ( 0x0418 ),   // 1 WORD
        REG_SETTING_MVI_MAX_SPEED   = ( 0x0419 ),   // 1 WORD
        REG_CURRENT_LEVEL           = ( 0x0420 ),   // 1 WORD
        REG_REQUESTER               = ( 0x0421 ),   // 1 WORD
        REG_DATETIME32              = ( 0x0516 ),   // 2 WORD
        REG_DATE_YEAR               = ( 0x0518 ),   // 1 WORD
        REG_DATE_MONTH              = ( 0x0519 ),   // 1 WORD
        REG_DATE_DAY                = ( 0x051a ),   // 1 WORD
        REG_DATE_WEEKDAY            = ( 0x051b ),   // 1 WORD
        REG_TIME_HOURS              = ( 0x051c ),   // 1 WORD
        REG_TIME_MINUTES            = ( 0x051d ),   // 1 WORD
        REG_TIME_SECONDS            = ( 0x051e ),   // 1 WORD
    };

    enum {
        USR_LVL_NORMAL  = (     0 ), // 0x0000  
        USR_LVL_1       = (  2345 ), // 0x0929
        USR_LVL_2       = ( 12054 ), // 0x2F16
        USR_LVL_3       = ( 34102 )  // 0x8536
    };
    
    typedef struct {
        uint32_t code;
        const char *msg;
    } aldesDevice_t;

    static const aldesDevice_t aldesDevice_table[] = {
        {11023471, "VEX 40 T CLASSIC"           },
        {11023472, "VEX 40 T_PREMIUM"           },
        {11023473, "INSPIRAIR TOP 300 CLASSIC"  },
        {11023474, "INSPIRAIR TOP 300 PREMIUM"  },
        {11023475, "INSPIRAIR TOP 450 CLASSIC"  },
        {11023476, "INSPIRAIR TOP 450 PREMIUM"  },
        {11023477, "INSPIRAIR TOP 300 ERV"      },
        {11023478, "INSPIRAIR TOP 450 ERV"      },
    };

    static const char unknown_device[] = "UNKOWN";

}