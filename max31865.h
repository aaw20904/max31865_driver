#pragma once
/*this function implements a configuration
of registers MAX31865 (page 13 of datasheet).
It using 2 interfaces: wrToAddr (uint8_t addr, uint8_t data)
					   rdFromAddr (uiunt8_t addr)*/

struct initForMAX31865 {
	/*low fault threshold*/
	uint16_t low;
	/*high fault threshold*/
	uint16_t high;
	/*a schema of measuring - 2  or 3 wire*/
	uint8_t wireSchematic;
	/* AC filter 50/60Hz: 0-60, 1-50*/
	uint8_t filter;
};


struct errors_max31865 {
	uint8_t RTD_TooHigh : 1;
	uint8_t RTD_TooLow : 1;
	uint8_t REFIN_MoreThat : 1;
	uint8_t REFIN_LessThat : 1;
	uint8_t RFDIN_LessThat : 1;
	uint8_t RTDIN_Less : 1;
	uint8_t UnderOROvervoltage : 1;
	uint8_t HardwareError : 1;
};

class HAL_plug_MAX31865 {
public:
	/**  <<interfaces>> for ConfigurationMgr_MAX31865 */
	/*1)write a data to specific adress (addr)*/
	void wrToAddr(uint8_t addr, uint8_t data);
	/*2) read a data from a specific adress. Return uint8_t*/
	uint8_t rdFromAddr(uint8_t addr);
	/*a constructor*/
	HAL_plug_MAX31865();
protected:
	/*simulation registers of IC*/
	uint8_t arrayOfData[256];

};

class FaultDetectionMgr31865 {
public:
	/*a constructor*/
	FaultDetectionMgr31865();

	/* <<interfaces>>*/
	void runFaultDetectionAutoDelay(void);
	void runFaultDetectionManualDelay(void);
	void finishFaultDetectionManualDelay(void);
	uint8_t isFaultDetectionFinished(void);
	uint8_t isFaultDetectionRunning(void);
	uint8_t isManualCycle1Running(void);
	uint8_t isManualCycle2Running(void);
	void bindInterface(HAL_plug_MAX31865* param);

protected:
	HAL_plug_MAX31865* pToInterface;
	/*set or clear a bit in a byte*/
	uint8_t setBitInByte(uint8_t  data, uint8_t  mask);
};


class FaultStatusMgr31865 {
public:
	FaultStatusMgr31865();

	errors_max31865 getFaultStatus(void);
	void setHighFaultThreshold(uint16_t);
	void setLowFaultThreshold(uint16_t);
	void clearStatus(void);
	void bindToInterface(HAL_plug_MAX31865* ptr);

protected:
	HAL_plug_MAX31865* pToInterface;
	errors_max31865 faultStatus;
};

class ConfigurationMgr_MAX31865 {
public:
	/*a constructor - @pointer to an instance of HAL_plug_MAX31865*/
	ConfigurationMgr_MAX31865();
	/******  <<interfaces>>*/
	void setFilter(uint8_t s);
	uint8_t readFilter(void);
	void setWireInterface(uint8_t par);
	void shootStart(void);
	void setConversionMode(uint8_t mode);
	uint8_t readConversionMode(void);
	uint8_t readBias(void);
	void setBias(uint8_t data);
	void bindToInterface(HAL_plug_MAX31865* ptr);
	uint16_t readADC(void);
protected:
	/*assign an interface*/
	HAL_plug_MAX31865* interfaceForCommunication;
	/*set or clear a bit in a byte*/
	uint8_t setBitInByte(uint8_t  data, uint8_t  mask);
};


class SensorDriverMAX31865 {
protected:
	HAL_plug_MAX31865 spiInterface;
	static void delayMaker(uint32_t delay); 
public:
	FaultDetectionMgr31865 faultDetector;
	ConfigurationMgr_MAX31865 configManager;
	FaultStatusMgr31865 faultStatus;

	SensorDriverMAX31865(); 
	/*initializing an IC in according to initStruct in
	a single convertion mode*/
	void singleConvertionInit(initForMAX31865* pInitStruct);
	/*automatic convertion mode init*/
	void automaticConvertionInit(initForMAX31865* pInitStruct);
	/*start a single conversion: ON bias -> conversion -> OFF bias*/
	void singleStartEconomy(void);
	/*single start without power off*/
	void singleStart(void);

	/*halt / start during running (after init) */
	void automaticContinue(void);
	void automaticStop(void);
	/*callback for getting a result*/
	unsigned short getResultEconomy(errors_max31865* err);
	unsigned short getResult(errors_max31865* err);
	/*checking fault automaticaly -
	by a master. Return a structure with a result*/
	errors_max31865  checkFaultAutomaticaly(void);
	errors_max31865  checkFaultManually(void);

};