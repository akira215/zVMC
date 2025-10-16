/*
  Aldes Modbus params
  Repository: https://github.com/akira215/esp-ash-components
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#include "modbusMaster.h"

// Enumeration of modbus device addresses accessed by master device
enum {
    MB_ALDES_ADDR = 2 // ALDES INSPIRAIR Slave addr (add other slave addresses here)
};


// Enumeration of all supported CIDs for device (used in parameter definition table)
enum {
    CID_T_OUTDOOR_AIR = 0,
    CID_T_EXTRACT_AIR,
    CID_BYPASS_POSITION,
    CID_HOLD_DATA_1,
    CID_INP_DATA_2,
    CID_HOLD_DATA_2,
    CID_HOLD_CUSTOM1,
    CID_HOLD_TEST_REG,
    CID_RELAY_P1,
    CID_RELAY_P2,
    CID_DISCR_P1,
};



// Example Data (Object) Dictionary for Modbus parameters:
// The CID field in the table must be unique.
// Modbus Slave Addr field defines slave address of the device with correspond parameter.
// Modbus Reg Type - Type of Modbus register area (Holding register, Input Register and such).
// Reg Start field defines the start Modbus register number and Reg Size defines the number of registers for the characteristic accordingly.
// The Instance Offset defines offset in the appropriate parameter structure that will be used as instance to save parameter value.
// Data Type, Data Size specify type of the characteristic and its data size.
// Parameter Options field specifies the options that can be used to process parameter value (limits or masks).
// Access Mode - can be used to implement custom options for processing of characteristic (Read/Write restrictions, factory mode values and etc).
const mb_parameter_descriptor_t device_parameters[] = {
    // { CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size, Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
    { CID_T_OUTDOOR_AIR, STR("T_outdoor_air"), STR("°C"), MB_ALDES_ADDR, MB_PARAM_HOLDING,
            0x15E, 1, INPUT_OFFSET(input_data0), PARAM_TYPE_I16_AB, 2,
            OPTS( -50, 100, 0 ), PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_T_EXTRACT_AIR, STR("T_extract_air"), STR("°C"), MB_ALDES_ADDR, MB_PARAM_HOLDING,
            TEST_HOLD_REG_START(holding_data0), TEST_HOLD_REG_SIZE(holding_data0),
            HOLD_OFFSET(holding_data0), PARAM_TYPE_I16_AB, 2,
            OPTS( TEST_HUMI_MIN, TEST_HUMI_MAX, 0 ), PAR_PERMS_READ_WRITE_TRIGGER },
    { CID_BYPASS_POSITION,STR("Bypass_position"), STR("°"), MB_ALDES_ADDR, MB_PARAM_INPUT,
            0x15C, 1, INPUT_OFFSET(input_data0), PARAM_TYPE_U16, 2,
            OPTS( 0, 4, 0 ), PAR_PERMS_READ_WRITE_TRIGGER }
    };
