// ConsoleApplication1.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <stdio.h>
#include <string.h>
#include "max31865.h"
#include "main.h"
/*extern variables - for low-level plugin */
extern SPI_HandleTypeDef hspi1;
extern uint8_t busy_flag;
	/*a constructor*/
	HAL_plug_MAX31865::HAL_plug_MAX31865() {
	


	}



void HAL_plug_MAX31865::wrToAddr(uint8_t addr, uint8_t data) {
 
   static uint8_t buff[2];
  
  /*clear CE to LOW*/
   GPIOA->BSRR = GPIO_BSRR_BR4;
   /*make a delay*/
   HAL_plug_MAX31865::delayMaker(50);
   /*turn on a spi*/
 // SPI1->CR1 |= SPI_CR1_SPE;
  buff[0] = addr;
  buff[1] = data;
 busy_flag = 0x02;
  HAL_SPI_Transmit_IT(&hspi1,buff,0x02);
   

}

 void HAL_plug_MAX31865::delayMaker(uint32_t delay ) {
  while (delay > 0) {
    delay--;
  }
}

uint8_t HAL_plug_MAX31865::rdFromAddr(uint8_t addr) {
	 static uint8_t tx;
         static uint8_t rx;
         /*clear CE to LOW*/
        GPIOA->BSRR = GPIO_BSRR_BR4;
        /*make a delay*/
        HAL_plug_MAX31865::delayMaker(50);
        /*turn on a spi*/
        //  SPI1->CR1 |= SPI_CR1_SPE;
         /*this flag is checked in an SPI ISR:
          when it equals zero - SPI becomes disable (NSS pin=1)*/
         busy_flag = 0x04;
         tx = addr;
         rx = 0x00;
       HAL_SPI_Transmit_IT(&hspi1, &tx, 0x01);
       while (busy_flag > 2){
         /*wait until previous transmission finished*/
        }
        HAL_SPI_Receive_IT(&hspi1, &rx, 0x01);
         while (busy_flag){
         /*wait until previous transmission finished*/
         }
         return rx;
         
}

/****fault class***************************************************/

WriteConfigToIC::WriteConfigToIC() {

}

void WriteConfigToIC::bindInterface(HAL_plug_MAX31865* p) {
	this->pInterface = p;
}

void WriteConfigToIC::templateMap (uint8_t m) {
	this->map = m;
}

void WriteConfigToIC::zeroMap (void) {
	this->map = 0x00;
}

void WriteConfigToIC::biasMap (void) {
	this->map |= (1 << 7);
}

void WriteConfigToIC::convModeMap (void) {
	this->map |= (1 << 6);
}

void WriteConfigToIC::shootMap (void) {
	this->map |= (1 << 5);
}

void WriteConfigToIC::wire3Map (void) {
	this->map |= (1 << 4);
}

void WriteConfigToIC::clearFaultMap (void) {
	this->map |= (1 << 1);
}

void WriteConfigToIC::filterMap (void) {
	this->map |= (0x01);
}

void WriteConfigToIC::sendToIC (void) {
	this->pInterface->wrToAddr (0x80, this->map);
}

uint8_t WriteConfigToIC::readMap (void) {
	return this->map;
}

 FaultChecker::FaultChecker() {

}

void FaultChecker::bindToInterface(HAL_plug_MAX31865* p) {
	this->pInterface = p;
}

void FaultChecker::delay(uint32_t pause) {
	static uint32_t p = 0;
	p = pause;
	while (pause > 0) {
		pause--;
	}

}


 errors_max31865 FaultChecker::getErrors(HAL_plug_MAX31865* pSPI) {
	errors_max31865 result;
	memset((void*)&result, 0x00, sizeof(errors_max31865));
	/*read a ststus*/
	uint8_t status = pSPI->rdFromAddr(0x07);
	if (status & (1 << 7)) {
		result.RTD_TooHigh = 1;
	}
	else if (status & (1 << 6)) {
		 result.RTD_TooLow = 1;
	}
	else if (status & (1 << 5))
	{
		 result.REFIN_MoreThat = 1;
	}
	else if (status & (1 << 4))
	{
		 result.REFIN_LessThat = 1;
	}
	else if (status & (1 << 3))
	{
		 result.RTDIN_Less = 1;
	}
	else if (status & (1 << 2))
	{
		 result.UnderOROvervoltage = 1;
	}
	return result;
}
 
 void FaultChecker::clearErr(void) {
	 uint8_t old_data;
	 old_data = this->pInterface->rdFromAddr(0x00);
	 this->delay(1000);
	 old_data |= (1 << 1);
	 this->pInterface->wrToAddr(0x80, old_data);
 }

errors_max31865 FaultChecker::checkAuto (uint8_t templ) {
	uint8_t tmp;
	errors_max31865 result;
	/*applying only  bit 4 and bit 0*/
	tmp = templ & ((1 << 4) | (0x01));
	/*logical OR with a command*/
	tmp |= ((1 << 7) | (1 << 2));
	/*send to IC*/
	this->pInterface->wrToAddr(0x80, tmp);
	/*set a delay - roughly 500 microSeconds*/
	this->delay(1000);
	/*has a check already done?*/
	tmp = this->pInterface->rdFromAddr(0x00);
	/*clear bits 4 and 0*/
	tmp &= ~((1 << 4) | (0x01));
	while (tmp != (1 << 7)) {
		/*has a check already done?*/
		tmp = this->pInterface->rdFromAddr(0x00);
		/*clear bits 4 and 0*/
		tmp &= ~((1 << 4) | (0x01));
		this->delay(100);
	}
	/*when check was done*/
	result = FaultChecker::getErrors(this->pInterface);
	this->delay(100);
	this->clearErr();
	return result;
}

ADCmgr::ADCmgr() {

}

void ADCmgr::clearError (void) {
	uint8_t temp;
	temp = this->pInterface->rdFromAddr(0x00);
	temp |= (1 << 1);
	this->pInterface->wrToAddr(0x80, temp);
}

errors_max31865 ADCmgr::getError (void) {
	errors_max31865 result;
	memset((void*)&result, 0x00, sizeof(errors_max31865));
	/*read a ststus*/
	uint8_t status = this->pInterface->rdFromAddr(0x07);
	if (status & (1 << 7)) {
		result.RTD_TooHigh = 1;
	}
	else if (status & (1 << 6)) {
		result.RTD_TooLow = 1;
	}
	else if (status & (1 << 5))
	{
		result.REFIN_MoreThat = 1;
	}
	else if (status & (1 << 4))
	{
		result.REFIN_LessThat = 1;
	}
	else if (status & (1 << 3))
	{
		result.RTDIN_Less = 1;
	}
	else if (status & (1 << 2))
	{
		result.UnderOROvervoltage = 1;
	}

	this->clearError();
	return result;

}

uint16_t ADCmgr::readADC (uint8_t* errCode) {
	uint16_t result;
	//high byte
	result = this->pInterface->rdFromAddr(0x01);
	result <<= 8;
	result |= this->pInterface->rdFromAddr(0x02);
	if (result & 0x0001) {
		errCode[0] = 0x01;
		return (result >> 1);
	}
	else {
		errCode[0] = 0x00;
		return (result >> 1);
	}

}

void ADCmgr::setHighThr(uint16_t value) {
	uint8_t temp;
	/*low byte*/
	temp = (value & 0x00ff);
	this->pInterface->wrToAddr(0x04, temp);
	/*high byte*/
	temp = (value >> 8);
	this->pInterface->wrToAddr(0x03, temp);
}

void ADCmgr::setLowThr(uint16_t value) {
	uint8_t temp;
	/*low byte*/
	temp = (value & 0x00ff);
	this->pInterface->wrToAddr(0x06, temp);
	/*high byte*/
	temp = (value >> 8);
	this->pInterface->wrToAddr(0x05, temp);
}

void ADCmgr::bindInterface(HAL_plug_MAX31865* p) {
	this->pInterface = p;
}

DriverMAX31865::DriverMAX31865 () {
	this->ADCmgrInst.bindInterface(&this->drvInst);
	this->faultCheckerInst.bindToInterface(&this->drvInst);
	this->writeConfInst.bindInterface(&this->drvInst);
}

void DriverMAX31865::delay (uint32_t value) {
	while (value > 0) {
		value--;
	}
}


void DriverMAX31865::continuousModeInit (initForMAX31865* initStr) {
         /*init an inner structure*/
        this->innerStr = initStr[0];
	/*setting limits*/
	ADCmgrInst.setHighThr(this->innerStr.high);
	ADCmgrInst.setHighThr(this->innerStr.low);
	/*clear a map and write to IC*/
	this->writeConfInst.zeroMap();
	this->writeConfInst.sendToIC();
	this->delay(1000);
	/*set bits in a bitmap template*/
	if (this->innerStr.filter) {
		this->writeConfInst.filterMap();
	}
	else if (this->innerStr.wireSchematic)
	{
		this->writeConfInst.wire3Map();
	}
	/*bias - on, conversion mode - 1 (continuous)*/
	this->writeConfInst.biasMap();
	/*send to IC*/
	this->writeConfInst.sendToIC();
	this->delay(1000);
	this->writeConfInst.convModeMap();
	this->writeConfInst.sendToIC();
}

void DriverMAX31865::oneShootModeInit(initForMAX31865* initStr) {
	 /*init an inner structure*/
        this->innerStr = initStr[0];
	/*setting limits*/
	ADCmgrInst.setHighThr(this->innerStr.high);
	ADCmgrInst.setHighThr(this->innerStr.low);
	/*clear a map and write to IC*/
	this->writeConfInst.zeroMap();
	this->writeConfInst.sendToIC();
	this->delay(2000);
	/*set bits in a bitmap template*/
	if (this->innerStr.filter) {
		this->writeConfInst.filterMap();
	}
	else if (this->innerStr.wireSchematic)
	{
		this->writeConfInst.wire3Map();
	}
	/*bias - on, conversion mode - 1 (continuous)*/
	this->writeConfInst.biasMap();
	/*send to IC*/
	this->writeConfInst.sendToIC();
	this->delay(3000);
}

void DriverMAX31865::oneShootStart (void) {

  /*clear a map*/
  this->writeConfInst.zeroMap();
  /*set bits in a bitmap template*/
	if (this->innerStr.filter) {
		this->writeConfInst.filterMap();
	}
	else if (this->innerStr.wireSchematic)
	{
		this->writeConfInst.wire3Map();
	}
  /*turn on bias*/
  this->writeConfInst.biasMap();
  /*send to IC*/
  this->writeConfInst.sendToIC();
  /*wait until a transition process will be finished*/
  this->delay(1500000);
  this->writeConfInst.shootMap();
  this->writeConfInst.sendToIC();
}

void DriverMAX31865::clearBias (void) {
  /*clear a map*/
  this->writeConfInst.zeroMap();
  /*set bits in a bitmap template*/
	if (this->innerStr.filter) {
		this->writeConfInst.filterMap();
	}
	else if (this->innerStr.wireSchematic)
	{
		this->writeConfInst.wire3Map();
	}
        /*send to IC*/
  this->writeConfInst.sendToIC();
}

errors_max31865 DriverMAX31865::checkAuto(initForMAX31865* initStruct) {
	errors_max31865 result;
	uint8_t temp;
        this->innerStr = initStruct[0];
	/*set bits in a bitmap template*/
	if (this->innerStr.filter) {
		this->writeConfInst.filterMap();
	}
	else if (this->innerStr.wireSchematic)
	{
		this->writeConfInst.wire3Map();
	}
	/*save options about a configurations to a variable*/
	temp = this->writeConfInst.readMap();
	/*pass these options to a  check-class*/
	result = this->faultCheckerInst.checkAuto(temp);
	return result;
}

float DriverMAX31865::toResistance (uint16_t adc_val) {
  float result, rtd;;
  result = this->innerStr.etalonR / 32767.000;
  rtd = adc_val;
  result = result * rtd;
  return result;
  
}

float DriverMAX31865::resistanceToGreeds ( float R) {
	float sensor = R;
	float result;
	float zeroRTD = this->innerStr.zeroRTD; //resistance of Pt100 sensor at 0 Deg Celsius
	/*coeficients for a CallendarVan Dusen equation*/
	float const Ah  = 0.00390830; 
	float const Bh = -0.000000577500;
	float const Ch = -0.00000000000418301;
	float squarePart =  powf(Ah, 2) - (4 * Bh * (1-(sensor/zeroRTD)) );
	squarePart = sqrtf(squarePart);
        squarePart = squarePart - Ah;
	squarePart = squarePart / (2 * Bh);
	return squarePart;
}


uint16_t DriverMAX31865::readADC (uint8_t* err) {
	return this->ADCmgrInst.readADC(err);
}


	




