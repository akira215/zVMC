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

logger = log.logger;
const e = exposes.presets;
const ea = exposes.access;

// To avoid import from Zcl
const DataType = {
    uint16: 0x21,
    int16:  0x29,
    enum8:  0x30,
    single: 0x39,
    double: 0x3a
}

// Global /////////////////////////

const GENERAL_EP        = 1;
const INTAKE_OUT_EP     = 2;
const EXTRACT_IN_EP     = 3;
const SUPPLY_IN_EP      = 4;
const EXHAUST_OUT_EP    = 5;

const manufCode = {
    zVmc : {manufacturerCode: 0x6796}
}

const ATTR_CURRENT_LEVEL_ID = 0xff00;  // ['Holidays', 'Daily', 'Boost', 'Guests', 'MaxSpeed'];

const ATTR_FILTER_STATE_ID = 0xff00;  // elapsed days  
const ATTR_TEMPO_FILTER_ID = 0xff01;  // Number of months

const ATTR_BYPASS_TEMPERATURE_ID    = 0xff00;  
const ATTR_SEASON_DETECTION_ID      = 0xff01;  

const ATTR_VACATION_LEVEL_ID        = 0xff00; // Airflow in m3/h
const ATTR_DAILY_LEVEL_ID           = 0xff01;
const ATTR_PUSHBUTTON_LEVEL_ID      = 0xff02;
const ATTR_BOOST_LEVEL_ID           = 0xff03;
const ATTR_MAXSPEED_LEVEL_ID        = 0xff04;

// GUI elements ///////////////////////////////////////////////////////////////////////////

const setpointEnum = ['Holidays', 'Daily', 'Boost', 'Guests', 'MaxSpeed', 'Ignore'];
const bypassPositionEnum = ['Undefined', 'Open', 'Closed 45°', 'Closed'];
const detectedSeasonEnum = ['Undefined', 'Winter', 'Summer'];

function genVMC() {
    // The name shall not contain space otherwise it is not reported to HA
    return [
            exposes.enum('Setpoint', ea.ALL, setpointEnum)
                .withDescription('Demand User'),
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
            exposes.numeric('Airflow_MVE', ea.STATE_GET)
                .withDescription('Airflow extraction Fan')
                .withUnit('m3/h'),
            exposes.numeric('Airflow_MVI', ea.STATE_GET)
                .withDescription('Airflow supply Fan')
                .withUnit('m3/h'),
            exposes.enum('Bypass_Position', ea.STATE_GET, bypassPositionEnum)
                .withDescription('Current Bypass Position'),
            exposes.enum('Detected_Season', ea.STATE_GET, detectedSeasonEnum)
                .withDescription('If Tavg > 19°C or Tmax > 28°C and Tmin > 7° : Summer / If Tavg <19°C or Tmax<28°C : Winter (24h)'),  
            exposes.numeric('Filter_Elapsed_Days', ea.ALL)
                .withDescription('Elapsed days since filters replacement')
                .withUnit('days'),
            exposes.numeric('Tempo_Filter', ea.ALL)
                .withValueMin(3)
                .withValueMax(12)
                .withDescription('Number of month prior to change filter')
                .withUnit('months')
        ];
};

// Options ///////////////////////////////////////////////////////////////////////////

// Option T° bypass Summer
function optionTbypassSetting() {
    return  exposes.numeric('Temperature_Bypass_Summer', ea.ALL)
                .withValueMin(19)
                .withValueMax(28)
                .withValueStep(0.1)
                .withDescription('Temperature of Exhaust Extracted Air that trigger the opening of bypass')
                .withUnit('°C')
};

// Option Airflow for mode
function optionVacationSetting() {
    return  exposes.numeric('Vacation_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for Vacation level')
                .withUnit('m3/h')
};

// Option Airflow for mode
function optionDailySetting() {
    return  exposes.numeric('Daily_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for Daily level')
                .withUnit('m3/h')
};

// Option Airflow for mode
function optionPushButtonSetting() {
    return  exposes.numeric('Push_Button_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for PushButton level')
                .withUnit('m3/h')
};

// Option Airflow for mode
function optionBoostSetting() {
    return  exposes.numeric('Boost_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for Boost level')
                .withUnit('m3/h')
};

// Option Airflow for mode
function optionMaxSpeedSetting() {
    return  exposes.numeric('Max_Speed_Setting', ea.ALL)
                .withValueMin(10)
                .withValueMax(450)
                .withDescription('Airflow for Max Speed level')
                .withUnit('m3/h')
};


// toZigbee ///////////////////////////////////////////////////////////////////////////
const  tz_VMC = {
    key: ['Setpoint', 'Bypass_Position', 'Detected_Season', 'Filter_Elapsed_Days', 'Tempo_Filter',
        'Airflow_MVE', 'Airflow_MVI',
        'Air_Intake_Out_T', 'Air_Extract_In_T', 'Air_Supply_In_T', 'Air_Exhaust_Out_T'],
    
    // convertSet will be called when user update a value from GUI, 
    convertSet: async (entity, key, value, meta) => {
        let payload = {};

        switch(key) {
            case 'Setpoint':
                //logger.info(`Setpoint new value = [${value}]`);
                let newValue = 255;

                if (value == 'Ignore'){
                    newValue = 255 // refer to Aldes datasheet
                } else {
                    newValue = setpointEnum.indexOf(value);
                }
                await entity.write('genMultistateValue', { presentValue: newValue });
                break;

            case 'Filter_Elapsed_Days':
                payload[ATTR_FILTER_STATE_ID] = {'value': value, 'type': DataType.uint16};
                await entity.write('msFlowMeasurement', payload, manufCode.zVmc);
                break;

            case 'Tempo_Filter':
                payload[ATTR_TEMPO_FILTER_ID] = {'value': value, 'type': DataType.uint16};
                await entity.write('msFlowMeasurement', payload, manufCode.zVmc);
                break;

            default:
               break;
       }

       result = {state: {[key]: value}};
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

        // If actualization button has been pressed
        switch(key) {
            case 'Setpoint':
                // read current level instead of user demand
                await endpointGeneral.read('genMultistateValue', [ATTR_CURRENT_LEVEL_ID], manufCode.zVmc);
                break;        
            
            case 'Air_Intake_Out_T':
                await endpointIntakeOut.read('msTemperatureMeasurement', ['measuredValue']);
                break;
            
            case 'Air_Extract_In_T':
                await endpointExtractIn.read('msTemperatureMeasurement', ['measuredValue']);
                break;
            
            case 'Air_Supply_In_T':
                await endpointSupplyIn.read('msTemperatureMeasurement', ['measuredValue']);
                break;

            case 'Air_Exhaust_Out_T':
                await endpointExhaustOut.read('msTemperatureMeasurement', ['measuredValue']);
                break;
            
            case 'Bypass_Position':
                await endpointGeneral.read('genAnalogInput', ['presentValue']);
                break;

            case 'Detected_Season':
                await endpointGeneral.read('genAnalogInput', [ATTR_SEASON_DETECTION_ID], manufCode.zVmc);
                break;

            case 'Filter_Elapsed_Days':
                await endpointGeneral.read('msFlowMeasurement', [ATTR_TEMPO_FILTER_ID, ATTR_FILTER_STATE_ID], manufCode.zVmc);
                break;

            case 'Tempo_Filter':
                await endpointGeneral.read('msFlowMeasurement', [ATTR_TEMPO_FILTER_ID], manufCode.zVmc); 
                break;

            case 'Airflow_MVE':
                await endpointExtractIn.read('genAnalogValue', ['presentValue']); 
                break;
            
            case 'Airflow_MVI':
                await endpointSupplyIn.read('genAnalogValue', ['presentValue']); 
                break;
           
            default:
               break;
        }

    },

};


// fromZigbee ///////////////////////////////////////////////////////////////////////////
const fz_Setpoint = {
    
    cluster: 'genMultistateValue',
    type: ['attributeReport', 'readResponse'],
    options: [],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        
        // When remote device send the attribute of current level
        if (msg.data.hasOwnProperty(ATTR_CURRENT_LEVEL_ID)) {  

            logger.info(`*********zVMC.js Setpoint read response =${msg.data[ATTR_CURRENT_LEVEL_ID]}`);
            if (msg.data[ATTR_CURRENT_LEVEL_ID] > 4){
                result['Setpoint'] = 'Ignore';
            } else {
                result['Setpoint'] = setpointEnum[msg.data[ATTR_CURRENT_LEVEL_ID]];
            }
        }

        return result;
    },
}

const fz_Filters = {
    
    cluster: 'msFlowMeasurement',
    type: ['attributeReport', 'readResponse'],
    options: [/*genWaterMeterOptions()*/],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        //logger.info(`*********zVMC.js convert data =${JSON.stringify(msg.data)}`);
        // deviceAddr is also available with options.ID or options.friendly_name
        // const deviceIeeAdd = msg.device.getEndpoint(GENERAL_EP).deviceIeeeAddress;
        
        // When remote device send the attribute tempo filter (triggered by a read on filter elapsed)
        if (msg.data.hasOwnProperty(ATTR_TEMPO_FILTER_ID)) {        
            result['Tempo_Filter'] = msg.data[ATTR_TEMPO_FILTER_ID];
        }

        // Occured when a read from Zigbee occured. Device only send number of tick that occured,
        // and reset to 0 its tick count each day at 00:00
        if (msg.data.hasOwnProperty(ATTR_FILTER_STATE_ID)) {
            result[`Filter_Elapsed_Days`] = msg.data[ATTR_FILTER_STATE_ID];
        }

        return result;
    },
}

const fz_Tbypass = {
    
    cluster: 'genAnalogInput',
    type: ['attributeReport', 'readResponse'],
    options: [/*genWaterMeterOptions()*/],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        const deviceIeeAdd = msg.device.getEndpoint(GENERAL_EP).deviceIeeeAddress;

        // When remote device send the attribute for T° bypass summer (triggered by a read only on reconfigure)
        if (msg.data.hasOwnProperty(ATTR_BYPASS_TEMPERATURE_ID)) {      
            settings.changeEntityOptions(deviceIeeAdd, { Temperature_Bypass_Summer: (msg.data[ATTR_BYPASS_TEMPERATURE_ID] / 100) });
        }

        if (msg.data.hasOwnProperty('presentValue')) {      
           result['Bypass_Position'] = bypassPositionEnum[Math.round(msg.data['presentValue'])];
        }

        if (msg.data.hasOwnProperty(ATTR_SEASON_DETECTION_ID)) {      
            result['Detected_Season'] = detectedSeasonEnum[msg.data[ATTR_SEASON_DETECTION_ID]];
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
        if(msg.endpoint.ID == GENERAL_EP)
        {
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
        }

        if(msg.endpoint.ID == EXTRACT_IN_EP)
        {
            // When remote device send the attribute tempo filter (triggered by a read on filter remaining)
            if (msg.data.hasOwnProperty('presentValue')) {        
                result['Airflow_MVE'] = msg.data['presentValue'];
            }
        }

        if(msg.endpoint.ID == SUPPLY_IN_EP)
        {
            // When remote device send the attribute tempo filter (triggered by a read on filter remaining)
            if (msg.data.hasOwnProperty('presentValue')) {        
                result['Airflow_MVI'] = msg.data['presentValue'];
            }
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



// Events ///////////////////////////////////////////////////////////////////////////

async function onEventCallback(event) {

    // Catch deviceOptionsChanged event, triggered by user changing device specific settings on GUI
    if(event.type == 'deviceOptionsChanged'){
        const endpointGeneral= event.data.device.getEndpoint(GENERAL_EP);
        //logger.info(`zVMC.js deviceOptionsChanged event.data =${JSON.stringify(event.data)}`)
        
        // Warning this event is trigger twice, the 2nd with from field = to field
        // Return if there is no from or no to field
        if ((!event.data.hasOwnProperty('from'))||(!event.data.hasOwnProperty('to')))
            return; 

        if (event.data.to.hasOwnProperty('Temperature_Bypass_Summer'))
        {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['Temperature_Bypass_Summer'] != event.data.to['Temperature_Bypass_Summer'])
            {
                const newVal = event.data.to['Temperature_Bypass_Summer'];
                logger.info(`*******zVMC.js deviceOptionsChanged Temperature_Bypass_Summer =${newVal}` )
                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['Temperature_Bypass_Summer'] = newVal;
                else
                    event.data.state.options = { Temperature_Bypass_Summer: newVal };

                let payload={};
                payload[ATTR_BYPASS_TEMPERATURE_ID] = {'value': newVal * 100, 'type': DataType.uint16};
    
                await endpointGeneral.write('genAnalogInput',  payload, manufCode.zVmc);

            } // from != to
        }

        if (event.data.to.hasOwnProperty('Vacation_Setting'))
        {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['Vacation_Setting'] != event.data.to['Vacation_Setting'])
            {
                const newVal = event.data.to['Vacation_Setting'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['Vacation_Setting'] = newVal;
                else
                    event.data.state.options = { Vacation_Setting: newVal };
    
                let payload={};
                payload[ATTR_VACATION_LEVEL_ID] = {'value': newVal, 'type': DataType.uint16};
                
                await endpointGeneral.write('genAnalogValue', payload, manufCode.zVmc);

            } // from != to
        }

        if (event.data.to.hasOwnProperty('Daily_Setting'))
        {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['Daily_Setting'] != event.data.to['Daily_Setting'])
            {
                const newVal = event.data.to['Daily_Setting'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['Daily_Setting'] = newVal;
                else
                    event.data.state.options = { Daily_Setting: newVal };
    
                let payload={};
                payload[ATTR_DAILY_LEVEL_ID] = {'value': newVal, 'type': DataType.uint16};
                
                await endpointGeneral.write('genAnalogValue', payload, manufCode.zVmc);

            } // from != to
        } 

        if (event.data.to.hasOwnProperty('Push_Button_Setting'))
        {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['Push_Button_Setting'] != event.data.to['Push_Button_Setting'])
            {
                const newVal = event.data.to['Push_Button_Setting'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['Push_Button_Setting'] = newVal;
                else
                    event.data.state.options = { Push_Button_Setting: newVal };

                let payload={};
                payload[ATTR_PUSHBUTTON_LEVEL_ID] = {'value': newVal, 'type': DataType.uint16};
                
                await endpointGeneral.write('genAnalogValue', payload, manufCode.zVmc);

            } // from != to
        } 

        if (event.data.to.hasOwnProperty('Boost_Setting'))
        {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['Boost_Setting'] != event.data.to['Boost_Setting'])
            {
                const newVal = event.data.to['Boost_Setting'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['Boost_Setting'] = newVal;
                else
                    event.data.state.options = { Boost_Setting: newVal };

                let payload={};
                payload[ATTR_BOOST_LEVEL_ID] = {'value': newVal, 'type': DataType.uint16};
                
                await endpointGeneral.write('genAnalogValue', payload, manufCode.zVmc);

            } // from != to
        } 

        if (event.data.to.hasOwnProperty('Max_Speed_Setting'))
        {
            // Event seems to be trigger twice, the second one with same value 'from' and 'to'
            if (event.data.from['Max_Speed_Setting'] != event.data.to['Max_Speed_Setting'])
            {
                const newVal = event.data.to['Max_Speed_Setting'];

                if(event.data.state.hasOwnProperty('options'))
                    event.data.state.options['Max_Speed_Setting'] = newVal;
                else
                    event.data.state.options = { Max_Speed_Setting: newVal };

                let payload={};
                payload[ATTR_MAXSPEED_LEVEL_ID] = {'value': newVal, 'type': DataType.uint16};
                
                await endpointGeneral.write('genAnalogValue', payload, manufCode.zVmc);

            } // from != to
        } 
        

    }  //deviceOptionsChanged
}

// Definition ///////////////////////////////////////////////////////////////////////////

const definition = {
    zigbeeModel: ['AldesMonitor'],
    model: 'AldesMonitor',
    vendor: 'AkiraCorp',
    description: 'VMC ALDES Inspirair Top Monitor Device https://github.com/akira215/zVMC',
    fromZigbee: [ fz_Setpoint, fz_Filters, fz_Tbypass, fz_FlowSettings, fz_Temperatures ],
    toZigbee: [ tz_VMC ],
    exposes: [ ...genVMC() ],
    options:[ optionTbypassSetting(), optionVacationSetting(), optionDailySetting(), 
            optionPushButtonSetting(),optionBoostSetting(), optionMaxSpeedSetting(), ],
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
    

        // Setting reportings when hitting configure button ///////////////////////////////////////////////////////////////////
        
        //Current Level
        const payloadCurrentLevel = [
                {
                    attribute: { ID: ATTR_CURRENT_LEVEL_ID, type: DataType.uint16 },
                    minimumReportInterval: 5,
                    maximumReportInterval: constants.repInterval.HOUR, 
                    reportableChange: 1, // unit
                },
        ];
        await endpointGeneral.configureReporting('genMultistateValue', payloadCurrentLevel, manufCode.zVmc);
        
        // Reporting Bypass 
        const payloadSeasonDetection = [
                {
                    attribute: { ID: ATTR_SEASON_DETECTION_ID, type: DataType.uint16 },
                    minimumReportInterval: constants.repInterval.MINUTE,
                    maximumReportInterval: constants.repInterval.MAX, //cannot exceed 65535
                    reportableChange: 1, // unit
                },
        ];
        
        //await reporting.bind(endpointGeneral, coordinatorEndpoint, ['genAnalogInput']);
        await endpointGeneral.configureReporting('genAnalogInput', payloadSeasonDetection, manufCode.zVmc);
        await endpointGeneral.configureReporting('genAnalogInput', [{
                    attribute: {ID: 0x0055, type: DataType.single},
                    minimumReportInterval: 2,
                    maximumReportInterval: constants.repInterval.HOUR,
                    reportableChange: 1,
                }]);

        // Reporting Temperatures
        const payloadTemperature = [
                {
                    attribute: {ID: 0, type: DataType.int16},
                    //attribute: 'measuredValue',
                    minimumReportInterval: 10,
                    maximumReportInterval: constants.repInterval.HOUR,
                    reportableChange: 10, // temperature are x100 so 10 is 0.1°C
                },
        ];
        await endpointIntakeOut.configureReporting('msTemperatureMeasurement', payloadTemperature);
        await endpointExtractIn.configureReporting('msTemperatureMeasurement', payloadTemperature);
        await endpointSupplyIn.configureReporting('msTemperatureMeasurement', payloadTemperature);
        await endpointExhaustOut.configureReporting('msTemperatureMeasurement', payloadTemperature);

        // Airflow
        const payloadAirflow = [
                {
                    attribute: {ID: 0x0055, type: DataType.single},
                    minimumReportInterval: 5,
                    maximumReportInterval: constants.repInterval.HOUR,
                    reportableChange: 5,
                },
        ];
        await endpointExtractIn.configureReporting('genAnalogValue', payloadAirflow);
        await endpointSupplyIn.configureReporting('genAnalogValue', payloadAirflow);

        //Filters
        const payloadFilter = [
                {
                    attribute: { ID: ATTR_FILTER_STATE_ID, type: DataType.uint16 },
                    minimumReportInterval: constants.repInterval.MINUTES_10,
                    maximumReportInterval: constants.repInterval.MAX, //cannot exceed 65535
                    reportableChange: 1, // unit
                },
        ];
        await endpointGeneral.configureReporting('msFlowMeasurement', payloadFilter, manufCode.zVmc);


        // Read values when hitting configure button ///////////////////////////////////////////////////////////////////
       
        // Current level
        await endpointGeneral.read('genMultistateValue', [ATTR_CURRENT_LEVEL_ID], manufCode.zVmc);
        
        // Filters
        await endpointGeneral.read('msFlowMeasurement', [ATTR_TEMPO_FILTER_ID, ATTR_FILTER_STATE_ID], manufCode.zVmc);

        // Speed settings
        await endpointGeneral.read('genAnalogValue', [ATTR_VACATION_LEVEL_ID, ATTR_DAILY_LEVEL_ID, 
                        ATTR_PUSHBUTTON_LEVEL_ID, ATTR_BOOST_LEVEL_ID, ATTR_MAXSPEED_LEVEL_ID ], manufCode.zVmc); 

        
        // Bypass
        await endpointGeneral.read('genAnalogInput', ['presentValue']);
        await endpointGeneral.read('genAnalogInput', [ATTR_SEASON_DETECTION_ID, ATTR_BYPASS_TEMPERATURE_ID], manufCode.zVmc);

        // Temperatures
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