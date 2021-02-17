// ConsoleApplication1.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <stdio.h>
#include <string.h>
#include "max31865.h"


	/*a constructor*/
	HAL_plug_MAX31865::HAL_plug_MAX31865() {
		/*configuration READ FROM*/
		arrayOfData[0x00] = 0x00;
		/*RTD MSB*/
		arrayOfData[0x01] = 0x00;
		/*RTD LSB*/
		arrayOfData[0x02] = 0x00;
		/*High Fault Threshold MSB*/
		arrayOfData[0x03] = 0xff;
		/*High Fault Threshold LSB*/
		arrayOfData[0x04] = 0xff;
		/*Low Fault Threshold MSB*/
		arrayOfData[0x05] = 0x00;
		/*Low Fault Threshold LSB*/
		arrayOfData[0x06] = 0x00;
		/* Fault status*/
		arrayOfData[0x07] = (1 << 7);
		/* configuration - write into**************/


	}



void HAL_plug_MAX31865::wrToAddr(uint8_t addr, uint8_t data) {
	this->arrayOfData[addr] = data;
	const char zero[] = { '0','\0' };
	const char one[] = { '1','\0' };
	char substr[32];
	substr[0] = '\0';
	for (int ind = 0; ind < 8; ind++) {
		if (data & (1 << (7 - ind))) {
			strcat_s(substr, one);
		}
		else {
			strcat_s(substr, zero);
		}
	}
	printf("DATA %x BINARY: %s send to ADDR %x \n", data, substr, addr);

}

uint8_t HAL_plug_MAX31865::rdFromAddr(uint8_t addr) {
	return arrayOfData[addr];
}

/****fault class***************************************************/

	/*a constructor*/
	FaultDetectionMgr31865::FaultDetectionMgr31865() {

	};

	


	/*set or clear a bit in a byte*/
	uint8_t FaultDetectionMgr31865::setBitInByte(uint8_t  data, uint8_t  mask) {
		data = data & (~mask);
		data |= mask;
		return data;
	}


void FaultDetectionMgr31865::bindInterface(HAL_plug_MAX31865* param) {
	this->pToInterface = param;
}

void  FaultDetectionMgr31865::runFaultDetectionAutoDelay(void) {
	/*read configuration*/
	uint8_t previous = this->pToInterface->rdFromAddr(0x00);
	/*save only bits 0 and 4*/
	previous &= 0x09;
	/*apply a pattern */
	previous |= 0x84;

	this->pToInterface->wrToAddr(0x80, previous);
}

void  FaultDetectionMgr31865::runFaultDetectionManualDelay(void) {
	/*read configuration*/
	uint8_t previous = this->pToInterface->rdFromAddr(0x00);
	/*save only bits 0 and 4*/
	previous &= 0x09;
	/*apply a pattern */
	previous |= 0x88;

	this->pToInterface->wrToAddr(0x80, previous);
}


void  FaultDetectionMgr31865::finishFaultDetectionManualDelay(void) {
	/*read configuration*/
	uint8_t previous = this->pToInterface->rdFromAddr(0x00);
	/*save only bits 0 and 4*/
	previous &= 0x09;
	/*apply a pattern */
	previous |= 0x8C;
	this->pToInterface->wrToAddr(0x80, previous);
}

uint8_t  FaultDetectionMgr31865::isFaultDetectionFinished(void) {
	/*read configuration*/
	uint8_t previous = this->pToInterface->rdFromAddr(0x00);
	uint8_t result = previous & 0x0C; //bits 2,3
	result = (result == 0) ? 1 : 0;
	return result;
}

uint8_t  FaultDetectionMgr31865::isFaultDetectionRunning(void) {
	/*read configuration*/
	uint8_t previous = this->pToInterface->rdFromAddr(0x00);
	/*ignore bits 0, 4*/
	uint8_t result = previous & 0xEE;
	/*compare  with a constant*/
	result = (result == 0x84) ? 1 : 0;
	return result;
}

uint8_t  FaultDetectionMgr31865::isManualCycle1Running(void) {
	/*read configuration*/
	uint8_t previous = this->pToInterface->rdFromAddr(0x00);
	/*ignore bits 0, 4*/
	uint8_t result = previous & 0xEE;
	/*compare  with a constant*/
	result = (result == 0x88) ? 1 : 0;
	return result;

}

uint8_t  FaultDetectionMgr31865::isManualCycle2Running(void) {
	/*read configuration*/
	uint8_t previous = this->pToInterface->rdFromAddr(0x00);
	/*ignore bits 0, 4*/
	uint8_t result = previous & 0xEE;
	/*compare  with a constant*/
	result = (result == 0x8C) ? 1 : 0;
	return result;
}

/****fault dstatus manager******************************
************************************************************/

	FaultStatusMgr31865::FaultStatusMgr31865() {

	};



void FaultStatusMgr31865::bindToInterface(HAL_plug_MAX31865* ptr) {
	this->pToInterface = ptr;
}

errors_max31865 FaultStatusMgr31865::getFaultStatus (void) {
	uint8_t hardResult = this->pToInterface->rdFromAddr(0x07);
	errors_max31865 result;
	memset(&result, 0, sizeof(errors_max31865));
	/*Overvoltage undervoltage fault*/
	if (hardResult & (1 << 2)) {
		result.UnderOROvervoltage = 1;
	} else  if (hardResult & (1 << 3)) {
		/*RTDIN- < 0.85 x VBIAS (FORCE- open)*/
		result.RTDIN_Less = 1;
	}
	else if (hardResult & (1 << 4)) {
		/*REFIN- < 0.85 x VBIAS (FORCE- open)*/
		result.REFIN_LessThat = 1;
	}
	else if (hardResult & (1 << 5)) {
		/*REFIN- > 0.85 x VBIAS*/
		result.REFIN_MoreThat = 1;
	}
	else if (hardResult & (1 << 6)) {
		/*RTD Low Threshold*/
		result.RTD_TooLow = 1;
	}
	else if (hardResult & (1 << 7)) {
		/*RTD High Threshold*/
		result.RTD_TooHigh= 1;
	}

	return result;
}



void FaultStatusMgr31865::setHighFaultThreshold(uint16_t value) {
	/*shift a data to left - because a LSB is an insignificant bit*/
	value <<= 1;
	/*extract and send low byte*/
	uint8_t parcel = (uint8_t)(value & 0xFF);
	this->pToInterface->wrToAddr(0x04, parcel);
	/*extract and send high byte*/
	parcel = (uint8_t)(value >> 8);
	this->pToInterface->wrToAddr(0x03, parcel);
}

void FaultStatusMgr31865::setLowFaultThreshold(uint16_t value) {
	/*shift a data to left - because a LSB is an insignificant bit*/
	value <<= 1;
	/*extract and send low byte*/
	uint8_t parcel = (uint8_t)(value & 0xFF);
	this->pToInterface->wrToAddr(0x06, parcel);
	/*extract and send high byte*/
	parcel = (uint8_t)(value >> 8);
	this->pToInterface->wrToAddr(0x05, parcel);

}

void FaultStatusMgr31865::clearStatus(void) {
	uint8_t parcel = this->pToInterface->rdFromAddr(0x00);
	/*exclude Fault-Detection Cycle Control Bits (page14 datasheet bottom) */
	parcel &= 0xF3;
	/*apply a clear bit*/
	parcel |= 0x02;
	this->pToInterface->wrToAddr(0x80, parcel);
}

/*****************************************************
a configuration manager class***************************/

	ConfigurationMgr_MAX31865::ConfigurationMgr_MAX31865() {

	};
	
	uint8_t ConfigurationMgr_MAX31865::setBitInByte(uint8_t  data, uint8_t  mask) {
		data = data & (~mask);
		data |= mask;
		return data;
	}


void ConfigurationMgr_MAX31865::bindToInterface(HAL_plug_MAX31865* ptr) {
	this->interfaceForCommunication = ptr;
}
/*methods implementation*/
void ConfigurationMgr_MAX31865::setFilter(uint8_t s) {
	/*read configuration*/
	uint8_t previous = this->interfaceForCommunication->rdFromAddr(0x00);
	uint8_t mask = (s & 0x01);
	/*correct in according to a parameter*/
	previous = this->setBitInByte(previous, mask);
	/*exclude Fault-Detection Cycle Control Bits (page14 datasheet bottom) */
	previous &= 0xF3;
	this->interfaceForCommunication->wrToAddr(0x80, previous);
}

uint8_t ConfigurationMgr_MAX31865::readFilter(void) {
	/*read configuration*/
	uint8_t previous = this->interfaceForCommunication->rdFromAddr(0x00);
	previous &= 0x01;
	return previous;
}

void ConfigurationMgr_MAX31865::setWireInterface(uint8_t par) {
	/*read configuration*/
	uint8_t previous = interfaceForCommunication->rdFromAddr(0x00);
	uint8_t mask = (par << 4);
	/*correct in according to a parameter*/
	previous = this->setBitInByte(previous, mask);
	/*exclude Fault-Detection Cycle Control Bits (page14 datasheet bottom) */
	previous &= 0xF3;
	this->interfaceForCommunication->wrToAddr(0x80, previous);
}

void ConfigurationMgr_MAX31865::shootStart(void) {
	/*read configuration*/
	uint8_t previous = this->interfaceForCommunication->rdFromAddr(0x00);
	uint8_t mask = (1 << 5);
	/*correct in according to a parameter*/
	previous = this->setBitInByte(previous, mask);
	/*exclude Fault-Detection Cycle Control Bits (page14 datasheet bottom) */
	previous &= 0xF3;
	this->interfaceForCommunication->wrToAddr(0x80, previous);
}

void ConfigurationMgr_MAX31865::setConversionMode(uint8_t mode) {
	/*read configuration*/
	uint8_t previous = this->interfaceForCommunication->rdFromAddr(0x00);
	uint8_t mask = (mode << 6);
	/*correct in according to a parameter*/
	previous = this->setBitInByte(previous, mask);
	/*exclude Fault-Detection Cycle Control Bits (page14 datasheet bottom) */
	previous &= 0xF3;
	this->interfaceForCommunication->wrToAddr(0x80, previous);
}

uint8_t ConfigurationMgr_MAX31865::readConversionMode(void) {
	/*read configuration*/
	uint8_t previous = this->interfaceForCommunication->rdFromAddr(0x00);
	previous &= (1 << 6);
	previous >>= 6;
	return previous;
}

uint8_t ConfigurationMgr_MAX31865::readBias(void) {
	/*read configuration*/
	uint8_t previous = this->interfaceForCommunication->rdFromAddr(0x00);
	/*set mask*/
	previous &= (1 << 7);
	previous >>= 7;
	return previous;
}

void ConfigurationMgr_MAX31865::setBias(uint8_t data) {
	/*read configuration*/
	uint8_t previous = this->interfaceForCommunication->rdFromAddr(0x00);
	uint8_t mask = (data << 7);
	/*correct in according to a parameter*/
	previous = this->setBitInByte(previous, mask);
	/*exclude Fault-Detection Cycle Control Bits (page14 datasheet bottom) */
	previous &= 0xF3;
	this->interfaceForCommunication->wrToAddr(0x80, previous);
}
/*read from  ADC*/
uint16_t ConfigurationMgr_MAX31865::readADC(void) {
	uint16_t result;
	/*MSB*/
	result = this->interfaceForCommunication->rdFromAddr(0x01);
	result <<= 8;
	/*LSB*/
	result |= this->interfaceForCommunication->rdFromAddr(0x02);
	return result;
}

/*************************A MAIN DRIVER CLASS*****
************************************************/


	
	 SensorDriverMAX31865::SensorDriverMAX31865() {
		faultDetector.bindInterface(&spiInterface);
		faultStatus.bindToInterface(&spiInterface);
		configManager.bindToInterface(&spiInterface);
	};

	 void SensorDriverMAX31865::delayMaker(uint32_t delay) {
		 while (delay > 0) {
			 delay--;
		 }
	 }

	
/*initializing an IC in according to initStruct in
 a single convertion mode*/
void SensorDriverMAX31865::singleConvertionInit(initForMAX31865* pInitStruct) {
	/*setting 2 or 4 wire interface*/
	this->configManager.setWireInterface(pInitStruct->wireSchematic);
	/*delay*/
	SensorDriverMAX31865::delayMaker(8192);
	/*set a 50-60Hz filter*/
	this->configManager.setFilter(pInitStruct->filter);
	SensorDriverMAX31865::delayMaker(8192);
	/*set a wire interface*/
	this->configManager.setWireInterface(pInitStruct->wireSchematic);
	/*set a single conversion mode*/
	this->configManager.setConversionMode(0);
}

/*automatic convertion init*/
void SensorDriverMAX31865::automaticConvertionInit(initForMAX31865* pInitStruct) {
	/*setting 2 or 4 wire interface*/
	this->configManager.setWireInterface(pInitStruct->wireSchematic);
	/*delay*/
	SensorDriverMAX31865::delayMaker(8192);
	/*set a 50-60Hz filter*/
	this->configManager.setFilter(pInitStruct->filter);
	SensorDriverMAX31865::delayMaker(8192);
	/*set a wire interface*/
	this->configManager.setWireInterface(pInitStruct->wireSchematic);
	/*turn on bias*/
	this->configManager.setBias(0x01);
	/*delay*/
	SensorDriverMAX31865::delayMaker(100000);
	/*set continuous mode*/
	this->configManager.setConversionMode(0x01);
}

void SensorDriverMAX31865::automaticStop (void) {
	this->configManager.setConversionMode(0);

}

void SensorDriverMAX31865::automaticContinue (void) {
	this->configManager.setConversionMode(1);
}

uint16_t SensorDriverMAX31865::getResultEconomy (errors_max31865* err) {
	uint16_t result = this->configManager.readADC();
	errors_max31865 faultError;
	/*was a fault happened?*/
	  if (result & 0x0001) {
	  	faultError = this->faultStatus.getFaultStatus();
		err[0] = faultError;
		/*clear fault bits*/
		this->faultStatus.clearStatus();
		return 0;
	  }
	/*othervise reading a result*/
	result >>= 1;
	/*read a fault status*/
	faultError = this->faultStatus.getFaultStatus();
	err[0] = faultError;
	/*clear fault bits*/
	this->faultStatus.clearStatus();
	/*clear bias*/
	this->configManager.setBias(0x00);
	return result;
}

uint16_t SensorDriverMAX31865::getResult(errors_max31865* err) {
	uint16_t result = this->configManager.readADC();
	errors_max31865 faultError;
	/*was a fault happened?*/
	if (result & 0x0001) {
		faultError = this->faultStatus.getFaultStatus();
		err[0] = faultError;
		/*clear fault bits*/
		this->faultStatus.clearStatus();
		return 0;
	}
	/*othervise reading a result*/
	result >>= 1;
	/*read a fault status*/
	faultError = this->faultStatus.getFaultStatus();
	err[0] = faultError;
	/*clear fault bits*/
	this->faultStatus.clearStatus();
	return result;
}

errors_max31865 SensorDriverMAX31865::checkFaultAutomaticaly(void) {
	errors_max31865 err;
	this->faultDetector.runFaultDetectionAutoDelay();
	/*wait 1ms*/
	SensorDriverMAX31865::delayMaker(60000);
	
	while ( !(this->faultDetector.isFaultDetectionFinished()) ) {
		/*wait until a FaultDetection completed*/
		SensorDriverMAX31865::delayMaker(10000);
	}
	/*read a starus*/
	err = this->faultStatus.getFaultStatus();
	/*clear an error status*/
	this->faultStatus.clearStatus();
	return err;
}

errors_max31865 SensorDriverMAX31865::checkFaultManually() {
	errors_max31865 err;
	/*turn on bias*/
	this->configManager.setBias(0x01);
	/*delay 5 times minimum*/
	SensorDriverMAX31865::delayMaker(1000000);
	this->faultDetector.runFaultDetectionManualDelay();
	/*delay 5 times minimum*/
	SensorDriverMAX31865::delayMaker(1000000);
	/*finish manual mode*/
	this->faultDetector.finishFaultDetectionManualDelay();
	/*a delay*/
	SensorDriverMAX31865::delayMaker(10000);
	 while (!(this->faultDetector.isFaultDetectionFinished())) {
		/*wait until a FaultDetection completed*/
		 SensorDriverMAX31865::delayMaker(10000);
	 }
	/*read a starus*/
	err = this->faultStatus.getFaultStatus();
	/*clear an error status*/
	this->faultStatus.clearStatus();
	return err;
}

void SensorDriverMAX31865::singleStartEconomy(void) {
	/*turn on a bias*/
	this->configManager.setBias(1);
	/*wait */
	SensorDriverMAX31865::delayMaker(1000000);
	/*start*/
	this->configManager.shootStart();
}

/*a bias can be turn on before!*/
void SensorDriverMAX31865::singleStart (void) {
	/*start*/
	this->configManager.shootStart();
}




int main()
{   
	initForMAX31865 initial31865;
	/*filter 50Hz*/
	initial31865.filter = 1;
	/*low barrier*/
	initial31865.low = 0x0000;
	/*high fault threshold*/
	initial31865.high = 0xffff;
	/*2 or 4 wire schema*/
	initial31865.wireSchematic = 0;

	HAL_plug_MAX31865  hal_drv;
	errors_max31865 errorcode;
	/*ConfigurationMgr_MAX31865 confMgr;
	confMgr.bindToInterface(&hal_drv);
	FaultDetectionMgr31865 faultM;
	faultM.bindInterface(&hal_drv);
	FaultStatusMgr31865  faultStat;
	faultStat.bindToInterface(&hal_drv);
	faultStat.clearStatus();*/
	SensorDriverMAX31865 myDriver;
	myDriver.singleConvertionInit(&initial31865);
	///errorcode = myDriver.checkFaultAutomaticaly();
	errorcode = myDriver.checkFaultManually();
	//faultStat.setHighFaultThreshold(0x02F4);
	//faultStat.setLowFaultThreshold(0x81CA);
	///printf("returned: %d \n", faultStat.isStatusRTD_H());
	///printf("returned: %d \n", faultStat.isStatusRTD_L());
	///printf("returned: %d \n", faultStat.isStatusREFIN_UP());
	///printf("returned: %d \n", faultStat.isStatusREFIN_DWN());
	///printf("returned: %d \n", faultStat.isStatusRTDIN_DWN());
	///printf("returned: %d \n", faultStat.isStatusOverOrUnderFault());
	//hal_drv.wrToAddr(0x00,0x04);
	//confMgr.setWireInterface(1);
	//confMgr.setConversionMode(1);
	//confMgr.readConversionMode();
	//confMgr.setBias(1);
	//confMgr.readBias();
	//faultM.isFaultDetectionFinished();
	//faultM.isFaultDetectionRunning();
	//faultM.isManualCycle1Running();
	//faultM.isManualCycle2Running();
	//faultM.runFaultDetectionAutoDelay();
	//faultM.runFaultDetectionManualDelay();
	//faultM.finishFaultDetectionManualDelay();
	//getchar();
	return 0;
}








// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
