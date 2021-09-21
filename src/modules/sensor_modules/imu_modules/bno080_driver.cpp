#include "KraftKontrol/modules/sensor_modules/imu_modules/bno080_driver.h"



volatile int64_t BNO080Driver::_newDataTimestamp = 0;
volatile bool BNO080Driver::_newDataInterrupt = false;
Task_Abstract* BNO080Driver::driverTask_ = nullptr;



BNO080Driver::BNO080Driver(int interruptPin, TwoWire& i2cBus, const Barometer_Interface* baro, const GNSS_Interface* gnss): Task_Abstract("BNO080 Driver", 200, eTaskPriority_t::eTaskPriority_VeryHigh), pinInterrupt_(interruptPin, _interruptRoutine, false, true), i2c_(i2cBus) {
    if (baro != nullptr) baroSubr_.subscribe(baro->getBaroTopic());
    if (gnss != nullptr) gnssSubr_.subscribe(gnss->getGNSSTopic());
}


void BNO080Driver::prediction() {

    
    float dTime = (float)(NOW() - navigationData_.timestamp)/SECONDS;

    //Predict current state and its error
    navigationData_.velocity += navigationData_.linearAcceleration*dTime;
    navigationData_.velocityError += navigationData_.accelerationError*dTime;

    navigationData_.position += navigationData_.velocity*dTime;
    navigationData_.absolutePosition.height = navigationData_.position.z + navigationData_.homePosition.height;
    navigationData_.positionError += navigationData_.velocityError*dTime;
    
    
}


void BNO080Driver::getGNSSData() {

    const GNSSData& data = gnssSubr_.getItem();

    ValueError<Vector<>> posBuf = ValueError<Vector<>>(data.positionValueTimestamp.sensorData.getPositionVectorFrom(navigationData_.homePosition), data.positionError);
    ValueError<Vector<>> velBuf = ValueError<Vector<>>(data.velocityValueTimestamp.sensorData, data.velocityError);

    posBuf = ValueError<Vector<>>(navigationData_.position, navigationData_.positionError).weightedAverage(posBuf);
    velBuf = ValueError<Vector<>>(navigationData_.velocity, navigationData_.velocityError).weightedAverage(velBuf);

    navigationData_.position.x = posBuf.value.x;
    navigationData_.position.y = posBuf.value.y;
    navigationData_.position.z = posBuf.value.z;

    navigationData_.positionError.x = posBuf.error.x;
    navigationData_.positionError.y = posBuf.error.y;
    navigationData_.positionError.z = posBuf.error.z;

    navigationData_.velocity.x = velBuf.value.x;
    navigationData_.velocity.y = velBuf.value.y;
    navigationData_.velocity.z = velBuf.value.z;

    navigationData_.velocityError.x = velBuf.error.x;
    navigationData_.velocityError.y = velBuf.error.y;
    navigationData_.velocityError.z = velBuf.error.z;

    

}


void BNO080Driver::getBaroData() {

    /*SensorTimestamp<float> sensorTime;

    do {    

        baroSub_.takeBack(sensorTime);

        float& baroPressure_ = sensorTime.sensorData;
        int64_t& timestamp = sensorTime.sensorTimestamp;

        //calculate height from new pressure value
        float heightAbsolute = getHeightFromPressure(baroPressure_, sealevelPressure_);
        //Serial.println(String("H: ") + heightAbsolute);
        //float heightRelative = heightAbsolute - navigationData_.absolutePosition.height;
        //calculate z velocity from new height value
        float zVelocity = (heightAbsolute - _lastHeightValue)/dt;
        _lastHeightValue = heightAbsolute;

        //Update buffers
        baroHeightBuf_.placeFront(heightAbsolute, true);
        baroVelBuf_.placeFront(zVelocity, true);

        float heightMedian = baroHeightBuf_.getMedian();
        float heightError = baroHeightBuf_.getStandardError();

        float velMedian = baroVelBuf_.getMedian();
        float velError = baroVelBuf_.getStandardError();

        //Serial.println(String("Height: ") + heightMedian + " +- " + String(heightError,5) + ",\tvel: " + velMedian + "+-" + velError);
        //Serial.println(String("") + (heightMedian - navigationData_.homePosition.height) + "  " + velMedian);

        //correct dead reckoning values with new ones.
        ValueError<> heightVel = ValueError<>(navigationData_.velocity.z, navigationData_.velocityError.z).weightedAverage(ValueError<>(zVelocity, velError));
        navigationData_.velocity.z = heightVel.value;
        navigationData_.velocityError.z = heightVel.error;

        ValueError<> height = ValueError<>(navigationData_.absolutePosition.height, navigationData_.positionError.z).weightedAverage(ValueError<>(heightAbsolute, heightError));
        navigationData_.absolutePosition.height = height.value;
        navigationData_.positionError.z = height.error;

        //Update position z
        navigationData_.position.z = navigationData_.absolutePosition.height - navigationData_.homePosition.height;
    
    } while(baroSubr_.available() > 0);*/

}


void BNO080Driver::getIMUData() {

    //i2c_.setClock(400000);
    _imu.getReadings();

    uint8_t accuracy;

    float angleAccuracy;

    Quaternion<> transform = Quaternion<>(Vector<>(0,0,1), -90*DEG_TO_RAD)*Quaternion<>(Vector<>(0,0,1), 90*DEG_TO_RAD)*Quaternion<>(Vector<>(1,0,0), 180*DEG_TO_RAD);

    SensorTimestamp<Quaternion<>> quat(0, _newDataTimestamp);
    _imu.getQuat(quat.sensorData.x, quat.sensorData.y, quat.sensorData.z, quat.sensorData.w, angleAccuracy, accuracy);
    navigationData_.attitude = quat.sensorData*transform;

    SensorTimestamp<Vector<>> bufVec(0, _newDataTimestamp);
    _imu.getGyro(bufVec.sensorData.x, bufVec.sensorData.y, bufVec.sensorData.z, accuracy);
    navigationData_.angularRate = navigationData_.attitude.rotateVector(bufVec.sensorData);

    _imu.getAccel(bufVec.sensorData.x, bufVec.sensorData.y, bufVec.sensorData.z, accuracy);
    navigationData_.acceleration = navigationData_.attitude.rotateVector(bufVec.sensorData);

    _imu.getLinAccel(bufVec.sensorData.x, bufVec.sensorData.y, bufVec.sensorData.z, accuracy);
    navigationData_.linearAcceleration = navigationData_.attitude.rotateVector(bufVec.sensorData);
    

}


void BNO080Driver::thread() {


    if (moduleStatus_ == eModuleStatus_t::eModuleStatus_Running) {

        prediction();

        if (_newDataInterrupt) { //If true then data is ready in the imu FIFO
            _newDataInterrupt = false;

            getIMUData();
            

        }

        if (baroSubr_.available() > 0) getBaroData();

        if (gnssSubr_.isDataNew()) getGNSSData();
        
        navigationData_.timestamp = NOW();

        navigationDataTopic_.publish(navigationData_);


    } else if (moduleStatus_ == eModuleStatus_t::eModuleStatus_NotStarted || moduleStatus_ == eModuleStatus_t::eModuleStatus_RestartAttempt) {
        
        init();

    } else if (false/*moduleStatus_ == eModuleStatus_t::MODULE_CALIBRATING*/) {

        //################## Following is Temporary #################
        Serial.println("CALIBRATING IMU");
        //imu.calibrateGyro();
        //imu.calibrateMag();
        for (byte n = 0; n < 6; n++) {
            if (n != 0) Serial.println("SWITCH");
            //_imu.calibrateAccel();
        }

        moduleStatus_ = eModuleStatus_t::eModuleStatus_Running; 
        //else imuStatus = DeviceStatus::DEVICE_FAILURE; 

    } else { //This section is for device failure or a wierd mode that should not be set, therefore assume failure

        moduleStatus_ = eModuleStatus_t::eModuleStatus_Failure;
        driverTask_ = nullptr;
        stopTaskThreading();

    }

}



void BNO080Driver::_interruptRoutine() {
    _newDataInterrupt = true;
    _newDataTimestamp =  NOW();
    //if (driverTask_ != nullptr) driverTask_->startTaskThreading();
}



void BNO080Driver::init() {

    i2c_.begin();
    i2c_.setClock(100000);

    //Serial.println("Test1");
    bool startCode = _imu.begin(BNO080_DEFAULT_ADDRESS, i2c_);
    //Serial.println("Test2");

    i2c_.setClock(400000);


    if (startCode) {

        _imu.enableAccelerometer(5);
        _imu.enableGyro(5);
        _imu.enableGameRotationVector(5);

        //pinInterrupt_.setEnable(true);
        
        //imuStatus = DeviceStatus::DEVICE_CALIBRATING;
        moduleStatus_ = eModuleStatus_t::eModuleStatus_Running;

        driverTask_ = this;

    } else {
        moduleStatus_ = eModuleStatus_t::eModuleStatus_RestartAttempt; 
        Serial.println("IMU Start Fail. Code: " + String(startCode));
    }

    startAttempts_++;

    if (startAttempts_ >= 5 && moduleStatus_ == eModuleStatus_t::eModuleStatus_RestartAttempt) moduleStatus_ = eModuleStatus_t::eModuleStatus_Failure;

}