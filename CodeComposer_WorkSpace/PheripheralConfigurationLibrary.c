#include "PheripheralConfigurationLibrary.h"


/*-------------Definicion de variables--------------*/
//----------- Necesario para uso de pantalla
char chipid = 0;                        //Almacena el ID leido de la pantalla FT800
unsigned long cmdBufferRd = 0x00000000;         // Almacena el valor leido por el registro de lectura
unsigned long cmdBufferWr = 0x00000000;         // Almacena el valor leido por el registro de escritura
unsigned long POSX, POSY, BufferXY; //Variables para localizaciÃ³n de la posicion del pulso en la pantalla
unsigned int CMD_Offset = 0;

//----------- Necesario para la calibracion de pantalla

#ifdef AGL_pantalla
//Pantalla Alvaro
const int32_t REG_CAL[6]={21696,-78,-614558,498,-17021,15755638}; //Pantalla 3.5 pulgadas
const int32_t REG_CAL5[6]={32146, -1428, -331110, -40, -18930, 18321010}; //Pantalla 5 pulgadas
#endif

#ifdef SLD_pantalla
//Pantalla Sergio
const int32_t REG_CAL[6]={24816,397,-1325076,99,-16775,16138575}; //Pantalla 3.5 pulgadas
const int32_t REG_CAL5[6]={32146, -1428, -331110, -40, -18930, 18321010}; //Pantalla 5 pulgadas
#endif

//---------Necesario para SensorsBoostPack
// BME280
int g_s32ActualTemp   = 0;
unsigned int g_u32ActualPress  = 0;
unsigned int g_u32ActualHumity = 0;

// BMI160/BMM150
uint8_t returnValue;
struct bmi160_gyro_t        s_gyroXYZ;
struct bmi160_accel_t       s_accelXYZ;
struct bmi160_mag_xyz_s32_t s_magcompXYZ;

//Calibration off-sets
int8_t accel_off_x;
int8_t accel_off_y;
int8_t accel_off_z;
int16_t gyro_off_x;
int16_t gyro_off_y;
int16_t gyro_off_z;

//---------Necesario para Servomotor
unsigned int Max_pos = 4200; //3750 (2 ms de 20 ms == 10% del periodo --> 0.1 * 37500 = 3750)
unsigned int Min_pos = 1300; //1875 (1 ms de 20 ms == 5% del periodo --> 0.05 * 37500 = 1875)
unsigned int posicion;


/*------------Declaracion comportamiento funciones-------------*/

uint32_t ConfiguraReloj120MHz(void)
{
    uint32_t RELOJ = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
    return RELOJ;
}

void HabilitaPerifericos(unsigned int parameters_number, ...)
{
    va_list ap;
    int i;
    va_start(ap,parameters_number);
    for(i=0;i<parameters_number;i++)
    {
        SysCtlPeripheralEnable(va_arg(ap, uint32_t));
    }
    va_end(ap);
}

void defineTipoPines(unsigned int parameters_number, ...)
{
    va_list ap;
    int i;
    va_start(ap,parameters_number);
    for(i=0;i<parameters_number/3;i++)
    {
        bool tipo = va_arg(ap, uint32_t);
        uint32_t puerto = va_arg(ap, uint32_t);
        uint8_t pines = va_arg(ap, uint32_t);
        if(tipo)
        {
            GPIOPinTypeGPIOOutput(puerto, pines);
        }
        else GPIOPinTypeGPIOInput(puerto, pines);
    }
    va_end(ap);
}

void ConfiguraTimer(int numTimer, unsigned int periodo, void (*ptr)(void))
{
    uint32_t base;
    uint32_t interruptTimer;
    switch (numTimer) {
        case 0:
            base = TIMER0_BASE;
            interruptTimer = INT_TIMER0A;
            break;
        case 1:
            base = TIMER1_BASE;
            interruptTimer = INT_TIMER1A;
            break;
        case 2:
            base = TIMER2_BASE;
            interruptTimer = INT_TIMER2A;
            break;
        case 3:
            base = TIMER3_BASE;
            interruptTimer = INT_TIMER3A;
            break;
        case 4:
            base = TIMER4_BASE;
            interruptTimer = INT_TIMER4A;
            break;
        case 5:
            base = TIMER5_BASE;
            interruptTimer = INT_TIMER5A;
            break;
    }

    TimerClockSourceSet(base, TIMER_CLOCK_PIOSC);   //Fijar reloj que usara T1: 16MHz
    TimerConfigure(base, TIMER_CFG_SPLIT_PAIR| TIMER_CFG_A_PERIODIC | TIMER_CFG_B_PERIODIC);    //T1 periodico y dividido (16b)
    TimerPrescaleSet(base, TIMER_A, 255);    //T1 preescalado a 256: 16M/256=62.5 KHz
    uint32_t PeriodoTimer = 62.5*periodo;
    TimerLoadSet(base, TIMER_A, PeriodoTimer -1); //Fijar periodo
    TimerIntRegister(base,TIMER_A,ptr); //Definicion de interrupcion asociada al Timer
    IntEnable(interruptTimer); //Habilitar interrupcion global del Timer
    TimerIntEnable(base, TIMER_TIMA_TIMEOUT);
    TimerEnable(base, TIMER_A);  //Habilitar Timer1
}

void ModoBajoConsumo(unsigned int parameters_number, ...)
{
    va_list ap;
    int i;
    va_start(ap,parameters_number);
    for(i=0;i<parameters_number;i++)
    {
        SysCtlPeripheralSleepEnable(va_arg(ap, uint32_t));
    }
    va_end(ap);
    SysCtlPeripheralClockGating(true);
}

void ConfiguraPantalla(unsigned char boosterpack_number, uint32_t RELOJ)
{
    HAL_Init_SPI(boosterpack_number, RELOJ);  //Boosterpack a usar, Velocidad del MC
        Inicia_pantalla();       //Arranque de la pantalla
        int i;

        //Pantalla 3.5 pulgadas
    #ifdef VM800B35
        for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
    #endif
        //Pantalla 5 pulgadas
    #ifdef VM800B50
        for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL5[i]);
    #endif
}

void ConfiguraEEPROM()
{
    EEPROMInit(); //Se recupera si el ultimo apagado fue durante escritura
}

void ConfiguraSensorsBoosterpack(unsigned char boosterpack_number, uint32_t RELOJ)
{
Conf_Boosterpack(boosterpack_number, RELOJ); //Indicar que el sensors boosterpack esta conectado a los pines del boosterpack 2
//Configurar sensor de temperatura, presion y humedad relativa
bme280_data_readout_template();
bme280_set_power_mode(BME280_NORMAL_MODE);
//Configurar acelerometro y giroscopio
bmi160_initialize_sensor();
bmi160_config_running_mode(APPLICATION_NAVIGATION);
//Configurar sensor de luz
OPT3001_init();
}

void ConfiguraCCM_SHA256()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CCM0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_CCM0))
    {
    }
    SHAMD5Reset(SHAMD5_BASE);
    SHAMD5ConfigSet(SHAMD5_BASE, SHAMD5_ALGO_SHA256);
}


void ConfiguraPWM(uint32_t base,uint32_t PuertoPin, uint32_t puerto, uint8_t pin, unsigned int frecuenciaPWM)
{
    PWMGenConfigure(base, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC); //Configurar el pwm0, generador 2, contador descendente y sin sincronizacion (actualizacion automatica)
    PWMClockSet(base,PWM_SYSCLK_DIV_64);   // Configuracion del reloj: 1.875MHz
    GPIOPinConfigure(PuertoPin);          // Configurar el pin a PWM
    GPIOPinTypePWM(puerto, pin); // Configurar el pin a PWM

    unsigned int PeriodoPWM=1875000/frecuenciaPWM;
    PWMGenPeriodSet(base, PWM_GEN_2, PeriodoPWM-1); //Definicion del periodo de la PWM
    PWMOutputState(base, PWM_OUT_4_BIT , true);    //Habilitar la salida PWM
    PWMGenEnable(base, PWM_GEN_2);     //Habilita el generador 2
    posicion=(Max_pos+Min_pos)/2;
    PWMPulseWidthSet(base, PWM_OUT_4, posicion);   //Inicialmente, 1ms --> Posicion media
}
