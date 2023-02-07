/*Libreria para la configuración de periféricos del microcontrolador TM4C1294NCPDT. Creada por Sergio Leon Doncel (03 Noviembre 2022)*/
//Consideraciones: se hace uso de funciones variádicas sin comprobar ni el número de argumentos recibidos ni el tamaño de los argumentos obtenidos, lo que puede
//conllevar al bloqueo del micro si se hace uso incorrecto de dichas funciones, tener especial cuidado

#ifndef _PHERIPHERALCONFIGURATIONLIBRARY_H_
//-----------------------------------------------------------------------------------------------------------
#define _PHERIPHERALCONFIGRATIONLIBRARY_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h> //permite el uso de funciones variadicas (numero de parametros a una funcion variable)

#include "driverlib2.h"
#include "HAL_I2C.h"
#include "sensorlib2.h"
#include "FT800_TIVA.h"

/*-------------Definicion de variables--------------*/
//-----------Necesario para velocidad del procesador y pantalla
uint32_t RELOJ;

//------- Necesario para uso de pantalla
extern char chipid;                        //Almacena el ID leido de la pantalla FT800
extern unsigned long cmdBufferRd;         // Almacena el valor leido por el registro de lectura
extern unsigned long cmdBufferWr;         // Almacena el valor leido por el registro de escritura
extern unsigned long POSX, POSY, BufferXY; //Variables para localizacion de la posicion del pulso en la pantalla
extern unsigned int CMD_Offset;

//-------- Necesario para la calibracion de pantalla
#define AGL_pantalla
//#define SLD_pantalla
#define VM800B35

extern const int32_t REG_CAL[6]; //Pantalla 3.5 pulgadas
extern const int32_t REG_CAL5[6]; //Pantalla 5 pulgadas

//------Necesario para uso de SensorsBoosterPack
// BME280
extern int g_s32ActualTemp;
extern unsigned int g_u32ActualPress;
extern unsigned int g_u32ActualHumity;

// BMI160/BMM150
extern uint8_t returnValue;
extern struct bmi160_gyro_t        s_gyroXYZ;
extern struct bmi160_accel_t       s_accelXYZ;
extern struct bmi160_mag_xyz_s32_t s_magcompXYZ;

//Calibration off-sets
extern int8_t accel_off_x;
extern int8_t accel_off_y;
extern int8_t accel_off_z;
extern int16_t gyro_off_x;
extern int16_t gyro_off_y;
extern int16_t gyro_off_z;

//---------------Necesario para uso del servomotor
extern unsigned int Max_pos;
extern unsigned int Min_pos;
extern unsigned int posicion;

/*-----------Prototipo de funciones---------------------*/
uint32_t ConfiguraReloj120MHz(void);
//Si se desea otra frecuencia de trabajo para el micro, se puede modificar manualmente el comportamiento de esta funcion
void HabilitaPerifericos(unsigned int parameters_number, ...);
//1) Parametro de SYSCTL_PERIPH_?
void defineTipoPines(unsigned int parameters_number, ...);
//Pasar numero de argumentos y en bloques de 3 parametros lo siguiente:
//1) Tipo: 1-Salida | 0-Entrada
//2) Puerto: GPIO_PORT?_BASE
//3) Pines de dicho puerto: GPIO_PIN_?
void ConfiguraTimer(int numTimer, unsigned int periodo, void (*ptr)(void));
//1) Numero de Timer
//2) Periodo del timer en ms
//3) Puntero a rutina de interrupcion del timer
void ModoBajoConsumo(unsigned int parameters_number, ...);
//Pasar numero de argumentos y perifericos que mantener activos que puedan despertar el micro con sus interrupciones
void ConfiguraPantalla(unsigned char boosterpack_number, uint32_t RELOJ);
//Pasar numero del boosterpack en el que esta conectado y frecuencia de Reloj del micro
void ConfiguraSensorsBoosterpack(unsigned char boosterpack_number, uint32_t RELOJ);
//Pasar numero del boosterpack en el que esta conectado y frecuencia de Reloj del micro
void ConfiguraPWM(uint32_t base,uint32_t PuertoPin, uint32_t puerto, uint8_t pin, unsigned int frecuenciaPWM);
//1) base -> Salida PWM a usar: PWM?_BASE
//2) PuertoPin -> Conexion pin con señal PWM: GPIO_PG?_M0PWM? (mirar en tabla de diapositiva 47 tema 4)
//3) puerto: GPIO_PORT?_BASE
//4) pin: GPIO_PIN_?
//5) frecuenciaPWM
void ConfiguraEEPROM();
void ConfiguraCCM_SHA256();

//-----------------------------------------------------------------------------------------------------------
#endif
