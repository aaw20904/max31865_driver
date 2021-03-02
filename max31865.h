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
	float zeroRTD;
	float etalonR;
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

/*this class has hardware dependencises - you 
can change it in according to your platform:
Arduino, MSC51, x86, Z80 e.t.c.
It using  to communicate with MAX31865 chip*/ 
class HAL_plug_MAX31865 {
public:
	/**  <<interfaces>> for ConfigurationMgr_MAX31865 */
        /**TARGET PLATFORM: STM32*/ 
	/*1)write a data to specific adress (addr)*/
	void wrToAddr(uint8_t addr, uint8_t data);
	/*2) read a data from a specific adress. Return uint8_t*/
	uint8_t rdFromAddr(uint8_t addr);
	/*a constructor*/
	HAL_plug_MAX31865();
protected:
	
        static void delayMaker(uint32_t delay); 
        

};


class WriteConfigToIC {
public:
	/*a constructor*/
	WriteConfigToIC ();
	/*init a pointer for access to interface*/
	void bindInterface (HAL_plug_MAX31865* );
	/*clear a map*/
	void zeroMap (void);
	/*assign 1 to bias bit*/
	void biasMap (void);
	void convModeMap (void);
	void shootMap (void);
	void wire3Map (void);
	void clearFaultMap(void);
	void filterMap(void);
	void templateMap (uint8_t);
	void sendToIC(void);
	uint8_t readMap(void);
protected:
	uint8_t map;
	HAL_plug_MAX31865* pInterface;
};

class FaultChecker {
public:
	FaultChecker();
	/*uint8_t : are bits of config register*/
	errors_max31865 checkManually (uint8_t);
	errors_max31865 checkAuto (uint8_t);
	void bindToInterface (HAL_plug_MAX31865*);
protected:
	HAL_plug_MAX31865* pInterface;
	void delay(uint32_t);
	void clearErr(void);
	static errors_max31865 getErrors(HAL_plug_MAX31865* pSPI);
};

class ADCmgr {
public:
	ADCmgr();
	uint16_t readADC(uint8_t* errCode);
	void setHighThr (uint16_t);
	void setLowThr(uint16_t);
	errors_max31865 getError (void);
	void bindInterface (HAL_plug_MAX31865*);
protected:
	void clearError (void);
	HAL_plug_MAX31865* pInterface;
};

class DriverMAX31865 {
	public:
	DriverMAX31865 ();
	errors_max31865 checkAuto (initForMAX31865* initStruct);
	errors_max31865 checkManually (initForMAX31865* initStruct);
	uint16_t readADC (uint8_t *err);
	errors_max31865 getADCerror (void);
	void oneShootStart (void);
	void continuousStart (void);
	void oneShootModeInit(initForMAX31865* initStruct);
	void continuousModeInit(initForMAX31865* initStruct);
         float resistanceToGreeds( float R);
         float toResistance(uint16_t adc_val);
        void clearBias (void);
protected:
	HAL_plug_MAX31865 drvInst;
	FaultChecker faultCheckerInst;
	WriteConfigToIC writeConfInst;
	ADCmgr ADCmgrInst;
        initForMAX31865 innerStr;
	void delay(uint32_t val);
};
