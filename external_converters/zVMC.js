/*
  zTank 
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

const WATERMETER_EP = 1;
const UPSTREAM_EP   = 2;
const DOWNSTREAM_EP = 3;
const WATERLEVEL_EP = 4;
const TEMPERATURE_EP = 5;

// GUI elements ///////////////////////////////////////////////////////////////////////////

function genZtank() {
    // The name shall not contain space otherwise it is not reported to HA
    return [
            exposes.numeric('water_consumed', ea.STATE_GET)
                .withDescription('Water meter record, reset to 0 each day at 00:00')
                .withUnit('L'),
                //.withEndoint('Name'),
            exposes.numeric('upstream_pressure', ea.STATE_GET)
                .withDescription('Pressure on pump side')
                .withUnit('bars'),
            exposes.numeric('downstream_pressure', ea.STATE_GET)
                .withDescription('Pressure on network side')
                .withUnit('bars'),
            exposes.numeric('water_level', ea.STATE_GET)
                .withDescription('Water Level in the tank')
                .withUnit('%'),
        ];
};

// Options ///////////////////////////////////////////////////////////////////////////

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

const toZigbee_zTank = {
    key: ['water_consumed', 'upstream_pressure', 'downstream_pressure', 'water_level'],
    convertSet: async (entity, key, value, meta) => {
        // just for debug, to see any incoming request
        logger.debug(`zTank.js convertSet key=[${key}] doing nothing`);
    },
    // convertGet will be call by pressing the button on the GUI to read value on the device, 
    convertGet: async (entity, key, meta) => {
        //logger.info(`zTank.js convertGet entity=[${JSON.stringify(entity)}]`);
        const endpointWaterMeter    = meta.device.getEndpoint(WATERMETER_EP);
        const endpointUpstream      = meta.device.getEndpoint(UPSTREAM_EP);
        const endpointDownstream    = meta.device.getEndpoint(DOWNSTREAM_EP);
        const endpointWaterLevel    = meta.device.getEndpoint(WATERLEVEL_EP);

        // If Upstream Pressure actualization button has been pressed
        if(key == 'water_consumed') {
            await endpointWaterMeter .read('msFlowMeasurement', ['measuredValue'/*,'unitOfMeasure','multiplier','divisor','summaFormatting'*/]);
        }

        if(key == 'upstream_pressure') {
            await endpointUpstream.read('msPressureMeasurement', ['measuredValue']);
        }

        if(key == 'downstream_pressure') {
            await endpointDownstream.read('msPressureMeasurement', ['measuredValue']);
        }

        if(key == 'water_level') {
            await endpointWaterLevel.read('msRelativeHumidity', ['measuredValue']);
        }

    },

};

// fromZigbee ///////////////////////////////////////////////////////////////////////////

const fromZigbee_Metering = {
    
    cluster: 'msFlowMeasurement',
    type: ['attributeReport', 'readResponse'],
    options: [genWaterMeterOptions()],
    convert: (model, msg, publish, options, meta) => {
        const result = {};
        // logger.info(`zTank.js convert meta =${JSON.stringify(meta)}`);

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

        //logger.info(`zTank.js convert msg =${JSON.stringify(msg)}`);

        // Occured when a read from Zigbee occured. Device send int16, so calibration factor
        // is required only on device side. We just use z2m GUI to adjust calbration factor
        // value is send in kPa, adjusting to get bars
        if (msg.data.hasOwnProperty('measuredValue')) {

            // Get endpoint list of device to read oposite property to compute Delta
            const ep_list = msg.device._endpoints;

            if(msg.endpoint.ID == UPSTREAM_EP)
            {
                const up_value = msg.data['measuredValue'];
                
                // Get the oposite endpoint to compute detlaP
                const down_ep = ep_list.find(x => x.ID === DOWNSTREAM_EP);
                const down_value = down_ep?.clusters?.msPressureMeasurement?.attributes?.measuredValue;

                result[`upstream_pressure`] = up_value / 100;
                
            }

            if(msg.endpoint.ID == DOWNSTREAM_EP)
            {
                const down_value = msg.data['measuredValue'];

                // Get the oposite endpoint to compute detlaP
                const up_ep = ep_list.find(x => x.ID === UPSTREAM_EP);
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
        // logger.info(`zTank.js convert meta =${JSON.stringify(meta)}`);

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
        //logger.info(`zTank.js fromZigbee_kFactor convert msg =${JSON.stringify(msg)}`);

        //Multiplier is read only on configure device, we use this to set up the optional setting from the state
        if (msg.data.hasOwnProperty('presentValue')) {
            
            // deviceAddr is also available with options.ID or options.friendly_name
            const deviceIeeAdd = msg.device.getEndpoint(WATERMETER_EP).deviceIeeeAddress;

            if(msg.endpoint.ID == WATERMETER_EP)
            {
                
                let newK = 1;
                if (options.hasOwnProperty('k_factor'))
                    newK = options.k_factor;
                else
                    newK =msg.data['presentValue'];
            
                settings.changeEntityOptions(deviceIeeAdd, { k_factor: newK });
            }

            if(msg.endpoint.ID == UPSTREAM_EP)
            {
                let newK = 1;
                if (options.hasOwnProperty('upstream_pressure_calibration'))
                    newK = options.upstream_pressure_calibration;
                else
                    newK = msg.data['presentValue'];
            
                settings.changeEntityOptions(deviceIeeAdd, { upstream_pressure_calibration: newK });
            }

            if(msg.endpoint.ID == DOWNSTREAM_EP)
            {
                let newK = 1;
                if (options.hasOwnProperty('downstream_pressure_calibration'))
                    newK = options.downstream_pressure_calibration;
                else
                    newK = msg.data['presentValue'];

                settings.changeEntityOptions(deviceIeeAdd, { downstream_pressure_calibration: newK });
            }

            if(msg.endpoint.ID == WATERLEVEL_EP)
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
        
        //logger.info(`zTank.js deviceOptionsChanged event.data =${JSON.stringify(event.data)}`)
        
        // Warning this event is trigger twice, the 2nd with from field = to field
        // Return if there is no from or no to field
        if ((!event.data.hasOwnProperty('from'))||(!event.data.hasOwnProperty('to')))
            return; 

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
                const endpointWaterMeter = event.data.device.getEndpoint(WATERMETER_EP);
                await endpointWaterMeter.write('genAnalogValue',  {presentValue: newK});
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
                const endpointUpstream = event.data.device.getEndpoint(UPSTREAM_EP);
                await endpointUpstream.write('genAnalogValue',  {presentValue: newCalibration});
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
                const endpointDownstream    = event.data.device.getEndpoint(DOWNSTREAM_EP);
                await endpointDownstream.write('genAnalogValue',  {presentValue: newCalibration});
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
                const endpointWaterLevel    = event.data.device.getEndpoint(WATERLEVEL_EP);
                await endpointWaterLevel.write('genAnalogValue',  {presentValue: newCalibration});
            } // from != to

        } // has k_factor

    }  //deviceOptionsChanged
}

// Definition ///////////////////////////////////////////////////////////////////////////

const definition = {
    zigbeeModel: ['WaterTankMonitor'],
    model: 'WaterTankMonitor',
    vendor: 'AkiraCorp',
    description: 'Water Tank Monitor Device https://github.com/akira215/zTank',
    fromZigbee: [fromZigbee_Metering, fromZigbee_kFactor, fromZigbee_Pressure, fromZigbee_Level, fz.temperature],
    toZigbee: [toZigbee_zTank],
    exposes: [ ...genZtank(), e.temperature()],
    options:[genWaterMeterOptions(), genUpstreamPressureOptions(), 
                    genDownstreamPressureOptions(), genWaterLevelOptions()],
    configure: async (device, coordinatorEndpoint, logger) => {
        
        // Get endpoints
        const endpointWaterMeter    = device.getEndpoint(WATERMETER_EP);
        const endpointUpstream      = device.getEndpoint(UPSTREAM_EP);
        const endpointDownstream    = device.getEndpoint(DOWNSTREAM_EP);
        const endpointWaterLevel    = device.getEndpoint(WATERLEVEL_EP);
        const endpointTemperature   = device.getEndpoint(TEMPERATURE_EP);
        
        // Bind cluster
        //await reporting.bind(endpointWaterMeter,    coordinatorEndpoint, ['msFlowMeasurement']     );
        //await reporting.bind(endpointUpstream,      coordinatorEndpoint, ['msPressureMeasurement'] );
        //await reporting.bind(endpointDownstream,    coordinatorEndpoint, ['msPressureMeasurement'] );
        //await reporting.bind(endpointWaterLevel,    coordinatorEndpoint, ['msRelativeHumidity']);

        await reporting.temperature(endpointTemperature);

        const payloadWatermeter = [
                {
                    attribute: {ID: 0, type: DataType.uint16},
                    //attribute: 'measuredValue',
                    minimumReportInterval: 2,
                    maximumReportInterval: constants.repInterval.HOUR,
                    reportableChange: 2,
                },
            ];

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

        await endpointWaterMeter.configureReporting('msFlowMeasurement', payloadWatermeter);
        await endpointUpstream.configureReporting('msPressureMeasurement', payloadPressure);
        await endpointDownstream.configureReporting('msPressureMeasurement', payloadPressure);
        await endpointWaterLevel.configureReporting('msRelativeHumidity', payloadWaterLevel);

        // trigger a read k_factor at startup to update the k_factor from the saved state
        await endpointWaterMeter.read('genAnalogValue', ['presentValue']);
        await endpointUpstream.read('genAnalogValue', ['presentValue']);
        await endpointDownstream.read('genAnalogValue', ['presentValue']);
        await endpointWaterLevel.read('genAnalogValue', ['presentValue']);

        // read value on start up
        await endpointWaterMeter.read('msFlowMeasurement', ['measuredValue']);
        await endpointUpstream.read('msPressureMeasurement', ['measuredValue']);
        await endpointDownstream.read('msPressureMeasurement', ['measuredValue']);
        await endpointWaterLevel.read('msRelativeHumidity', ['measuredValue']);
        await endpointTemperature.read('msTemperatureMeasurement', ['measuredValue']);

        device.powerSource = 'Mains (single phase)';
        device.save();
    },
    onEvent: onEventCallback
    //ota: ota.zigbeeOTA
};

module.exports = definition;