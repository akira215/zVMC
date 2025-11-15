/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/


const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const log = require('zigbee-herdsman-converters/lib/logger');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const constants = require('zigbee-herdsman-converters/lib/constants');
//const utils = require('zigbee-herdsman-converters/lib/utils');
//const globalStore = require('zigbee-herdsman-converters/lib/store');

// settings from zigbee2mqtt to access changeEntityOptions
const settings = require('/app/dist/util/settings');

// To avoid import from Zcl
const DataType = {
    uint16: 0x21,
    int16: 0x29,
    enum8: 0x30,
}

const logger = log.logger;
const e = exposes.presets;
const ea = exposes.access;

// Global /////////////////////////

const GENERAL_EP        = 1;
const INTAKE_OUT_EP     = 2;
const EXTRACT_IN_EP     = 3;
const SUPPLY_IN_EP      = 4;
const EXHAUST_OUT_EP    = 5;

const manufCode = {
    zVmc : {manufacturerCode: 0x6796}
}

const ATTR_FILTER_STATE_ID = 0xff00;  // elapsed days  
const ATTR_TEMPO_FILTER_ID = 0xff01;  // Number of months

const ATTR_VACATION_LEVEL_ID   = 0xff00; // Airflow in m3/h
const ATTR_DAILY_LEVEL_ID           = 0xff01;
const ATTR_PUSHBUTTON_LEVEL_ID      = 0xff02;
const ATTR_BOOST_LEVEL_ID           = 0xff03;
const ATTR_MAXSPEED_LEVEL_ID        = 0xff04;

// GUI elements ///////////////////////////////////////////////////////////////////////////

function genVMC() {
    // The name shall not contain space otherwise it is not reported to HA
    return [
            exposes.numeric('Air_Intake_Out_T', ea.STATE_GET)
                .withDescription('Air intake temperature outside')
                .withUnit('°C'),
            exposes.numeric('Air_Extract_In_T', ea.STATE_GET)
                .withDescription('Air extract temperature inside')
                .withUnit('°C'),
            exposes.numeric('Air_Supply_In_T', ea.STATE_GET)
                .withDescription('Airsupply temperature inside')
                .withUnit('°C'),
            exposes.numeric('Air_Exhaust_Out_T', ea.STATE_GET)
                .withDescription('Air exhaust temperature outside')
                .withUnit('°C'),
            exposes.numeric('Filter_Remaining_Days', ea.ALL)
                .withDescription('Remaining days prior to replace filters')
                .withUnit('days'),
        ];
};

// Options ///////////////////////////////////////////////////////////////////////////

// Option nb month for filter change
function optionTempoFilter() {
    return  exposes.numeric('Tempo_Filter', ea.ALL)
                .withValueMin(3)
                .withValueMax(12)
                .withDescription('Number of month prior to change filter')
                .withUnit('months')
};

// Option nb month for filter change
function optionVacationSetting() {
    return  exposes.numeric('Vacation_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for Vacation level')
                .withUnit('m3/h')
};

// Option nb month for filter change
function optionDailySetting() {
    return  exposes.numeric('Daily_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for Daily level')
                .withUnit('m3/h')
};

// Option nb month for filter change
function optionPushButtonSetting() {
    return  exposes.numeric('Push_Button_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for PushButton level')
                .withUnit('m3/h')
};

// Option nb month for filter change
function optionBoostSetting() {
    return  exposes.numeric('Boost_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for Boost level')
                .withUnit('m3/h')
};

// Option nb month for filter change
function optionMaxSpeedSetting() {
    return  exposes.numeric('Max_Speed_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for Max Speed level')
                .withUnit('m3/h')
};

// Options for the k factor of the water meter (L/pule)
function genWaterMeterOptions() {
    return  exposes.numeric('k_factor', ea.ALL)
                .withValueMin(0)
                .withDescription('Water meter kFactor')
                .withUnit('L/pulse')
};

function genUpstreamPressureOptions() {
    return  exposes.numeric('upstream_pressure_calibration', ea.ALL)
                .withValueMin(0)
                .withDescription('Sensor calibrating factor for upstream pressure (Pascal per Volt)')
                .withUnit('Pa/V')
};

function genDownstreamPressureOptions() {
    return  exposes.numeric('downstream_pressure_calibration', ea.ALL)
                .withValueMin(0)
                .withDescription('Sensor calibrating factor for downstream pressure (Pascal per Volt)')
                .withUnit('Pa/V')
};

function genWaterLevelOptions() {
    return  exposes.numeric('water_level_calibration', ea.ALL)
                .withValueMin(0)
                .withDescription('Sensor calibrating factor for water level (% per Volt)')
                .withUnit('%/V')
};

// toZigbee ///////////////////////////////////////////////////////////////////////////
const  tz_VMC = {
    key: ['Filter_Remaining_Days', 'Air_Intake_Out_T', 'Air_Extract_In_T', 'Air_Supply_In_T', 'Air_Exhaust_Out_T'],
    
    // convertSet will be call when updating a value from GUI, 
    convertSet: async (entity, key, value, meta) => {
        let payload = {};
        let newValue = value;

        switch(key) {
            case 'Filter_Remaining_Days':
                logger.info(`************Filter_Remaining_Days new value =[${value}]`);
                payload[ATTR_FILTER_STATE_ID] = {'value': value, 'type': DataType.uint16};
                await entity.write('msFlowMeasurement', payload, manufCode.zVmc);
                break;

            case 'switch_actions':
                newValue = switchActionValues.indexOf(value);
                payload = {switchActions: newValue};
                await entity.write('genOnOffSwitchCfg', payload);
                break;

            case 'relay_mode':
                newValue = relayModeValues.indexOf(value);
                payload = {65281: {'value': newValue, 'type': DataType.enum8}};
                await entity.write('genOnOffSwitchCfg', payload, manufCode.zVmc);
                break;

            case 'max_pause':
                payload = {65282: {'value': value, 'type': DataType.uint16}};
                await entity.write('genOnOffSwitchCfg', payload, manufCode.zVmc);
                break;

           case 'min_long_press':
                payload = {65283: {'value': value, 'type': DataType.uint16}};
                await entity.write('genOnOffSwitchCfg', payload, manufCode.zVmc);
                break;

           default:
               break;
       }

       result = {state: {[key]: value}}
       return result;
    },

    // convertGet will be call by pressing the button on the GUI to read value on the device, 
    convertGet: async (entity, key, meta) => {
        logger.info(`zVMC.js convertGet entity=[${JSON.stringify(key)}]`);

        const endpointGeneral       = meta.device.getEndpoint(GENERAL_EP);
        const endpointIntakeOut     = meta.device.getEndpoint(INTAKE_OUT_EP);
        const endpointExtractIn     = meta.device.getEndpoint(EXTRACT_IN_EP);
        const endpointSupplyIn      = meta.device.getEndpoint(SUPPLY_IN_EP);
        const endpointExhaustOut    = meta.device.getEndpoint(EXHAUST_OUT_EP);

        // If temperature actualization button has been pressed
        if(key == 'Filter_Remaining_Days') {
            await endpointGeneral.read('msFlowMeasurement', [ATTR_TEMPO_FILTER_ID, ATTR_FILTER_STATE_ID], manufCode.zVmc);
            //await endpointGeneral.read('msFlowMeasurement', [ATTR_FILTER_STATE_ID], manufCode.zVmc);
        }

        if(key == 'Air_Intake_Out_T') {
            await endpointIntakeOut.read('msTemperatureMeasurement', ['measuredValue']);
        }

        if(key == 'Air_Extract_In_T') {
            await endpointExtractIn.read('msTemperatureMeasurement', ['measuredValue']);
        }

        if(key == 'Air_Supply_In_T') {
            await endpointSupplyIn.read('msTemperatureMeasurement', ['measuredValue']);
        }

        if(key == 'Air_Exhaust_Out_T') {
            await endpointExhaustOut.read('msTemperatureMeasurement', ['measuredValue']);
        }

    },

};

// fromZigbee ///////////////////////////////////////////////////////////////////////////
const fz_Filters = {
    
    cluster: 'msFlowMeasurement',
    type: ['attributeReport', 'readResponse'],
    options: [/*genWaterMeterOptions()*/],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        //logger.info(`*********zVMC.js convert data =${JSON.stringify(msg.data)}`);
        // deviceAddr is also available with options.ID or options.friendly_name
        const deviceIeeAdd = msg.device.getEndpoint(GENERAL_EP).deviceIeeeAddress;
        
        // When remote device send the attribute tempo filter (triggered by a read on filter remaining)
        if (msg.data.hasOwnProperty(ATTR_TEMPO_FILTER_ID)) {        
            const newTempo = msg.data[ATTR_TEMPO_FILTER_ID];
            settings.changeEntityOptions(deviceIeeAdd, { Tempo_Filter: newTempo });
        }

        // Occured when a read from Zigbee occured. Device only send number of tick that occured,
        // and reset to 0 its tick count each day at 00:00
        if (msg.data.hasOwnProperty(ATTR_FILTER_STATE_ID)) {
            let tempo = 12;
            
            if (options.hasOwnProperty('Tempo_Filter')){
                tempo = options.Tempo_Filter;
             }

            const elapsedDays = msg.data[ATTR_FILTER_STATE_ID];

            //Remaining days is the difference
            const currentState = Math.round(tempo * 30.5) - elapsedDays;
            //logger.info(`tempo days =${Math.round(tempo * 30.5)}  elapsed days =${elapsedDays} => current=${currentState}`)
            
            result[`Filter_Remaining_Days`] = currentState;
        }

        return result;
    },
}

const fz_FlowSettings = {
    
    cluster: 'genAnalogValue',
    type: ['readResponse'],
    options: [/*genWaterMeterOptions()*/],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        //logger.info(`*********zVMC.js convert data =${JSON.stringify(msg.data)}`);
        // deviceAddr is also available with options.ID or options.friendly_name
        const deviceIeeAdd = msg.device.getEndpoint(GENERAL_EP).deviceIeeeAddress;
        
        // When remote device send the attribute tempo filter (triggered by a read on filter remaining)
        if (msg.data.hasOwnProperty(ATTR_VACATION_LEVEL_ID)) {        
            settings.changeEntityOptions(deviceIeeAdd, { Vacation_Setting: msg.data[ATTR_VACATION_LEVEL_ID] });
        }

        if (msg.data.hasOwnProperty(ATTR_DAILY_LEVEL_ID)) {        
            settings.changeEntityOptions(deviceIeeAdd, { Daily_Setting: msg.data[ATTR_DAILY_LEVEL_ID] });
        }

        if (msg.data.hasOwnProperty(ATTR_PUSHBUTTON_LEVEL_ID)) {        
            settings.changeEntityOptions(deviceIeeAdd, { Push_Button_Setting: msg.data[ATTR_PUSHBUTTON_LEVEL_ID] });
        }

        if (msg.data.hasOwnProperty(ATTR_BOOST_LEVEL_ID)) {        
            settings.changeEntityOptions(deviceIeeAdd, { Boost_Setting: msg.data[ATTR_BOOST_LEVEL_ID] });
        }

        if (msg.data.hasOwnProperty(ATTR_MAXSPEED_LEVEL_ID)) {        
            settings.changeEntityOptions(deviceIeeAdd, { Max_Speed_Setting: msg.data[ATTR_MAXSPEED_LEVEL_ID] });
        }

        return result;
    },
}

const fz_Temperatures = {

    cluster: 'msTemperatureMeasurement',
    type: ['attributeReport', 'readResponse'],
    options: [/*genWaterMeterOptions()*/],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        //logger.info(`*********zVMC.js fz_Temperatures convert msg =${JSON.stringify(msg)}`);

        //Get only measuredValue from ALDES temperature sensors
        if (msg.data.hasOwnProperty('measuredValue')) {
            
            // deviceAddr is also available with options.ID or options.friendly_name
            const deviceIeeAdd = msg.device.getEndpoint(GENERAL_EP).deviceIeeeAddress;

            if(msg.endpoint.ID == INTAKE_OUT_EP)
            {
                //const data = msg.data['measuredValue'];
      
                result[`Air_Intake_Out_T`] = Number(msg.data['measuredValue']) / 100;
            }

            if(msg.endpoint.ID == EXTRACT_IN_EP)
            {
                result[`Air_Extract_In_T`] = Number(msg.data['measuredValue']) / 100;
            }

            if(msg.endpoint.ID == SUPPLY_IN_EP)
            {
                result[`Air_Supply_In_T`] = Number(msg.data['measuredValue']) / 100;
            }

             if(msg.endpoint.ID == EXHAUST_OUT_EP)
            {
                result[`Air_Exhaust_Out_T`] = Number(msg.data['measuredValue']) / 100;
            }
            
        }

        return result;
    },
}


const fromZigbee_Metering = {
    
    cluster: 'msFlowMeasurement',
    type: ['attributeReport', 'readResponse'],
    options: [genWaterMeterOptions()],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        // logger.info(`zVMC.js convert meta =${JSON.stringify(meta)}`);

        // Occured when a read from Zigbee occured. Device only send number of tick that occured,
        // and reset to 0 its tick count each day at 00:00
        if (msg.data.hasOwnProperty('measuredValue')) {
            const multiplier = options.k_factor;
            const data = msg.data['measuredValue'];

            //Multiply by the divisor and divide again to avoid BigInt rounding to 0
            const currentSummDisplayed = (Number(data) * multiplier);
            
            result[`water_consumed`] = currentSummDisplayed;
        }

        return result;
    },
}

const fromZigbee_Pressure = {
    
    cluster: 'msPressureMeasurement',
    type: ['attributeReport', 'readResponse'],
    options: [ genUpstreamPressureOptions(), genDownstreamPressureOptions() ],
    convert: (model, msg, publish, options, meta) => {
        const result = {};

        //logger.info(`zVMC.js convert msg =${JSON.stringify(msg)}`);

        // Occured when a read from Zigbee occured. Device send int16, so calibration factor
        // is required only on device side. We just use z2m GUI to adjust calbration factor
        // value is send in kPa, adjusting to get bars
        if (msg.data.hasOwnProperty('measuredValue')) {

            // Get endpoint list of device to read oposite property to compute Delta
            const ep_list = msg.device._endpoints;

            if(msg.endpoint.ID == INTAKE_OUT_EP)
            {
                const up_value = msg.data['measuredValue'];
                
                // Get the oposite endpoint to compute detlaP
                const down_ep = ep_list.find(x => x.ID === EXTRACT_IN_EP);
                const down_value = down_ep?.clusters?.msPressureMeasurement?.attributes?.measuredValue;

                result[`upstream_pressure`] = up_value / 100;
                
            }

            if(msg.endpoint.ID == EXTRACT_IN_EP)
            {
                const down_value = msg.data['measuredValue'];

                // Get the oposite endpoint to compute detlaP
                const up_ep = ep_list.find(x => x.ID === INTAKE_OUT_EP);
                const up_value = up_ep?.clusters?.msPressureMeasurement?.attributes?.measuredValue;

                result[`downstream_pressure`] = down_value / 100;
            }

        }

        return result;
    },
}

const fromZigbee_Level = {
    
    cluster: 'msRelativeHumidity',
    type: ['attributeReport', 'readResponse'],
    options: [genWaterLevelOptions()],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        // logger.info(`zVMC.js convert meta =${JSON.stringify(meta)}`);

        // Occured when a read from Zigbee occured. Device only send number of tick that occured,
        // and reset to 0 its tick count each time value is reported to z2m
        if (msg.data.hasOwnProperty('measuredValue')) {
            const data = msg.data['measuredValue'];
            result[`water_level`] = data;
        }

        return result;
    },
}


// This is triggered after device pairing (if loosing connection)
// Factors are saved on the device itself and read by z2m to update options the values
const fromZigbee_kFactor = {
    
    cluster: 'genAnalogValue',
    type: ['attributeReport', 'readResponse'],
    options: [genWaterMeterOptions()],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        //logger.info(`zVMC.js fromZigbee_kFactor convert msg =${JSON.stringify(msg)}`);

        //Multiplier is read only on configure device, we use this to set up the optional setting from the state
        if (msg.data.hasOwnProperty('presentValue')) {
            
            // deviceAddr is also available with options.ID or options.friendly_name
            const deviceIeeAdd = msg.device.getEndpoint(GENERAL_EP).deviceIeeeAddress;

            if(msg.endpoint.ID == GENERAL_EP)
            {
                
                let newK = 1;
                if (options.hasOwnProperty('k_factor'))
                    newK = options.k_factor;
                else
                    newK =msg.data['presentValue'];
            
                settings.changeEntityOptions(deviceIeeAdd, { k_factor: newK });
            }

            if(msg.endpoint.ID == INTAKE_OUT_EP)
            {
                let newK = 1;
                if (options.hasOwnProperty('upstream_pressure_calibration'))
                    newK = options.upstream_pressure_calibration;
                else
                    newK = msg.data['presentValue'];
            
                settings.changeEntityOptions(deviceIeeAdd, { upstream_pressure_calibration: newK });
            }

            if(msg.endpoint.ID == EXTRACT_IN_EP)
            {
                let newK = 1;
                if (options.hasOwnProperty('downstream_pressure_calibration'))
                    newK = options.downstream_pressure_calibration;
                else
                    newK = msg.data['presentValue'];

                settings.changeEntityOptions(deviceIeeAdd, { downstream_pressure_calibration: newK });
            }

            if(msg.endpoint.ID == SUPPLY_IN_EP)
            {
                let newK = 1;
                if (options.hasOwnProperty('water_level_calibration'))
                    newK = options.water_level_calibration;
                else
                    newK = msg.data['presentValue'];

                settings.changeEntityOptions(deviceIeeAdd, { water_level_calibration: newK });
            }
            
        }

        return result;
    },
}

// Events ///////////////////////////////////////////////////////////////////////////

async function onEventCallback(event) {

    // Catch deviceOptionsChanged event, triggered by user changing device specific settings on GUI
    if(event.type == 'deviceOptionsChanged'){
        
        //logger.info(`zVMC.js deviceOptionsChanged event.data =${JSON.stringify(event.data)}`)
        
        // Warning this event is trigger twice, the 2nd with from field = to field
        // Return if there is no from or no to field
        if ((!event.data.hasOwnProperty('from'))||(!event.data.hasOwnProperty('to')))
            return; 

        if (event.data.to.hasOwnProperty('Tempo_Filter'))
        {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['Tempo_Filter'] != event.data.to['Tempo_Filter'])
            {
                const newTempo = event.data.to['Tempo_Filter'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['Tempo_Filter'] = newTempo;
                else
                    event.data.state.options = {Tempo_Filter: newTempo};

                // Sent the new value to the device so it will be saved on nvm
                const endpointGeneral= event.data.device.getEndpoint(GENERAL_EP);
    
                let payload={};
                payload[ATTR_TEMPO_FILTER_ID] = {'value': newTempo, 'type': DataType.uint16};
                
                logger.info(`*********write to device =${JSON.stringify(payload)}`);
                await endpointGeneral.write('msFlowMeasurement', payload, manufCode.zVmc);

                // Update remaining days accordingly
                await endpointGeneral.read('msFlowMeasurement', [ATTR_FILTER_STATE_ID], manufCode.zVmc);

            } // from != to

        } 
        
        if (event.data.to.hasOwnProperty('k_factor'))
        {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['k_factor'] != event.data.to['k_factor'])
            {
                const newK = event.data.to['k_factor'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['k_factor'] = newK;
                else
                    event.data.state.options = {k_factor: newK};

                // Sent the new value to the device so it will be saved on nvm
                const endpointGeneral= event.data.device.getEndpoint(GENERAL_EP);
                await endpointGeneral.write('genAnalogValue',  {presentValue: newK});
            } // from != to

        } 
        
        if (event.data.to.hasOwnProperty('upstream_pressure_calibration')) {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['upstream_pressure_calibration'] != event.data.to['upstream_pressure_calibration'])
            {
                const newCalibration = event.data.to['upstream_pressure_calibration'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['upstream_pressure_calibration'] = newCalibration;
                else
                    event.data.state.options = {upstream_pressure_calibration: newCalibration};

                // Sent the new value to the device so it will be saved on nvm
                const endpointIntakeOut = event.data.device.getEndpoint(INTAKE_OUT_EP);
                await endpointIntakeOut.write('genAnalogValue',  {presentValue: newCalibration});
            } // from != to

        } // has k_factor

        if (event.data.to.hasOwnProperty('downstream_pressure_calibration')) {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['downstream_pressure_calibration'] != event.data.to['downstream_pressure_calibration'])
            {
                const newCalibration = event.data.to['downstream_pressure_calibration'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['downstream_pressure_calibration'] = newCalibration;
                else
                    event.data.state.options = {downstream_pressure_calibration: newCalibration};

                // Sent the new value to the device so it will be saved on nvm
                const endpointExtractIn    = event.data.device.getEndpoint(EXTRACT_IN_EP);
                await endpointExtractIn.write('genAnalogValue',  {presentValue: newCalibration});
            } // from != to

        } // has k_factor

        if (event.data.to.hasOwnProperty('water_level_calibration')) {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['water_level_calibration'] != event.data.to['water_level_calibration'])
            {
                const newCalibration = event.data.to['water_level_calibration'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['water_level_calibration'] = newCalibration;
                else
                    event.data.state.options = {water_level_calibration: newCalibration};

                // Sent the new value to the device so it will be saved on nvm
                const endpointSupplyIn    = event.data.device.getEndpoint(SUPPLY_IN_EP);
                await endpointSupplyIn.write('genAnalogValue',  {presentValue: newCalibration});
            } // from != to

        } // has k_factor

    }  //deviceOptionsChanged
}

// Definition ///////////////////////////////////////////////////////////////////////////

const definition = {
    zigbeeModel: ['AldesMonitor'],
    model: 'AldesMonitor',
    vendor: 'AkiraCorp',
    description: 'VMC ALDES Inspirair Top Monitor Device https://github.com/akira215/zVMC',
    fromZigbee: [fz_Filters, fz_FlowSettings, fz_Temperatures/*fromZigbee_Metering, fromZigbee_kFactor, fromZigbee_Pressure, fromZigbee_Level, fz.temperature*/],
    toZigbee: [tz_VMC/*toZigbee_zTank*/],
    exposes: [ ...genVMC()],
    options:[optionTempoFilter(), optionVacationSetting(), optionDailySetting(), optionPushButtonSetting(),optionBoostSetting(), optionMaxSpeedSetting(),
                    /*genWaterMeterOptions(), genUpstreamPressureOptions(), 
                    genDownstreamPressureOptions(), genWaterLevelOptions()*/],
    configure: async (device, coordinatorEndpoint, logger) => {

        //logger.info(`*********zVMC.js **************** Configure`);

        // Get endpoints
        const endpointGeneral       = device.getEndpoint(GENERAL_EP);
        const endpointIntakeOut     = device.getEndpoint(INTAKE_OUT_EP);
        const endpointExtractIn     = device.getEndpoint(EXTRACT_IN_EP);
        const endpointSupplyIn      = device.getEndpoint(SUPPLY_IN_EP);
        const endpointExhaustOut    = device.getEndpoint(EXHAUST_OUT_EP);
        
        // Bind cluster
        //await reporting.bind(endpointWaterMeter,    coordinatorEndpoint, ['msFlowMeasurement']     );
        //await reporting.bind(endpointIntakeOut,      coordinatorEndpoint, ['msPressureMeasurement'] );
        //await reporting.bind(endpointExtractIn,    coordinatorEndpoint, ['msPressureMeasurement'] );
        //await reporting.bind(endpointSupplyIn,    coordinatorEndpoint, ['msRelativeHumidity']);
        

        const payloadTemperature = [
                {
                    attribute: {ID: 0, type: DataType.int16},
                    //attribute: 'measuredValue',
                    minimumReportInterval: 10,
                    maximumReportInterval: constants.repInterval.HOUR,
                    reportableChange: 10, // temperature are x100 so 10 is 0.1°C
                },
        ];

        const payloadFilter = [
                {
                    attribute: {ID: ATTR_FILTER_STATE_ID, type: DataType.uint16, },
                    //attribute: 'measuredValue',
                    minimumReportInterval: constants.repInterval.HOUR,
                    maximumReportInterval: 65500, //cannot exceed 65535
                    reportableChange: 1, // days
                },
        ];
/*
        const payloadPressure = [
                {
                    attribute: {ID: 0, type: DataType.int16},
                    //attribute: 'measuredValue',
                    minimumReportInterval: 2,
                    maximumReportInterval: constants.repInterval.HOUR,
                    reportableChange: 10,
                },
            ];
        
        const payloadWaterLevel = [
                {
                    attribute: {ID: 0, type: DataType.uint16},
                    //attribute: 'measuredValue',
                    minimumReportInterval: 10,
                    maximumReportInterval: constants.repInterval.HOUR,
                    reportableChange: 1,
                },
            ];
*/
        //await endpointGeneral.configureReporting('msFlowMeasurement', payloadFilter, manufCode.zVmc);
        await endpointIntakeOut.configureReporting('msTemperatureMeasurement', payloadTemperature);
        await endpointExtractIn.configureReporting('msTemperatureMeasurement', payloadTemperature);
        await endpointSupplyIn.configureReporting('msTemperatureMeasurement', payloadTemperature);
        await endpointExhaustOut.configureReporting('msTemperatureMeasurement', payloadTemperature);

        // trigger a read k_factor at startup to update the k_factor from the saved state
        //await endpointGeneral.read('genAnalogValue', ['presentValue']);
        //await endpointIntakeOut.read('genAnalogValue', ['presentValue']);
        //await endpointExtractIn.read('genAnalogValue', ['presentValue']);
        //await endpointSupplyIn.read('genAnalogValue', ['presentValue']);

        // read value on start up
        //await endpointGeneral.read('msFlowMeasurement', ['measuredValue']);
        //await endpointIntakeOut.read('msPressureMeasurement', ['measuredValue']);
        //await endpointExtractIn.read('msPressureMeasurement', ['measuredValue']);
        //await endpointSupplyIn.read('msRelativeHumidity', ['measuredValue']);

        await endpointGeneral.read('msFlowMeasurement', [ATTR_TEMPO_FILTER_ID, ATTR_FILTER_STATE_ID], manufCode.zVmc); // Filters
        await endpointGeneral.read('genAnalogValue', [ATTR_VACATION_LEVEL_ID, ATTR_DAILY_LEVEL_ID, 
                        ATTR_PUSHBUTTON_LEVEL_ID, ATTR_BOOST_LEVEL_ID, ATTR_MAXSPEED_LEVEL_ID ], manufCode.zVmc); // Filters
        await endpointIntakeOut.read('msTemperatureMeasurement', ['measuredValue']);
        await endpointExtractIn.read('msTemperatureMeasurement', ['measuredValue']);
        await endpointSupplyIn.read('msTemperatureMeasurement', ['measuredValue']);
        await endpointExhaustOut.read('msTemperatureMeasurement', ['measuredValue']);

        device.powerSource = 'Mains (single phase)';
        device.save();
    },
    onEvent: onEventCallback
    //ota: ota.zigbeeOTA
};

module.exports = definition;