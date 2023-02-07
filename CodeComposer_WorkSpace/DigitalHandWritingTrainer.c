#include <PheripheralConfigurationLibrary.h>
#include "stdio.h"
#include <string.h>


/*--------------------------------------------------------------*/
// DEFINICIONES
/*--------------------------------------------------------------*/

#define SLEEP SysCtlSleep()
//#define SLEEP SysCtlSleepFake()

#define N_IMAGENES 11

#define TIME 25

//BOTONES
#define CLEAR Boton(230, 5, 80, 40, 22, "CLEAR")
#define NEXT Boton(230, 55, 80, 40, 22, "NEXT")
#define PREVIUS Boton(230, 105, 80, 40, 22, "PREV")
#define SCORE Boton(230, 155, 80, 40, 22, "SCORE")
#define CONFIG Boton(230, 205, 80, 30, 21, "CONFIG")
#define PROGRESS Boton(100, 180, 120, 40, 22, "PROGRESS")
#define BACK Boton(100, 180, 120, 40, 22, "BACK")
#define RESET Boton(160, 85, 70, 40, 22, "RESET")
#define CHANGE Boton(160, 130, 70, 40, 22, "CHANGE")

/*--------------------------------------------------------------*/
// ESTRUCTURAS Y ENUMERACIONES
/*--------------------------------------------------------------*/

struct dimensiones
{
    int x;
    int y;
};

struct caracteristicasImagen
{
    unsigned long addr;
    int format;
    struct dimensiones dim;
};

enum formatoImagen
{
    L1 = 1,
    RGB332 = 4
};

enum estados
{
    pantalla_inicio,
    espera_login,
    login,
    espera_soltar,
    principal,
    resultados,
    next_digito,
    previus_digito,
    progreso,
    configuracion,
    cambia_pwd
};
enum estados estado = pantalla_inicio;

enum teclaEspecial
{
    DEL=-1,SPACE=-10,NO_PULSADO=0,ENTER=-100
};

/*--------------------------------------------------------------*/
// VARIABLES GLOBALES
/*--------------------------------------------------------------*/
bool Flag_ints = false;
int t = 0;
struct caracteristicasImagen atrImagenes[N_IMAGENES];
int matrizDibujoRealizado[64][64];
volatile long int ticks = 0;

unsigned char vector_codificado[4096];
int matriz_imagen_original[64][64];
int matriz_imagen_din_RGB[64][64][3];

// SONIDO
int xilofono = 0x08;
int nota_xilofono = 50;
int alarm = 0x010;
int nota_alarm = 100;
int scoreSound = 0x045;
int nota_score = 50;
int notify = 0x057;
int nota_notify = 50;
unsigned int volumen;
unsigned int volumenAnt;

char string[20];
unsigned int radioTrazo=0;
unsigned int best_scores[10];
unsigned int progreso_total;

/*--------------------------------------------------------------*/
// PROTOTIPOS DE FUNCIONES
/*--------------------------------------------------------------*/
void IntTick(void)
{
    ticks++;
}
void IntTimer1(void);
void SysCtlSleepFake(void);
extern void SysCtlSleep(void);
void cargar_imagenes(void);
void ejercicio_dibujar(void);
void dibuja_imagen(struct caracteristicasImagen parametrosImagen, float factor_escala, int xUpperCorner, int yUpperCorner);
void LeeImagenFormatoL1(struct caracteristicasImagen atrib_imagen);
void traduceMatriz2RGB(void);
void reseteaMatrizTrazoPintado(void);
void codificaMatrizFormatoRGB332(void);
void guardaImagenDin(void);
void resetImagenDin(void);
float calculaScore(void);
void ComSlider(int x, int y, int w, int h, int options, unsigned int *val, int range);
void progress_list(void);
void ComProgressBar(int x, int y, int w, int h, int options, int val, int range);
int teclado(void);
void ComKeys(int16_t x, int16_t y, int16_t w, int16_t h, int16_t font, uint16_t options, const char *s);
int Keys(int16_t x, int16_t y, int16_t w, int16_t h, int16_t font, const char *s);

/*--------------------------------------------------------------*/
// FUNCION PRINCIPAL
/*--------------------------------------------------------------*/

int main(void)
{
    /*VARIABLES LOCALES MAIN*/
    int i;
    int digitoActual = 0;
    float scoreRes = 0;
    char stringScore[20];
    bool flagClear = false;
    bool flagReset = false;
    bool flagChange = false;
    bool flagChangePWD = false;
    bool flagTime = false;
    bool flagDerecha = true;
    char desplaz = 0;
    int teclaPuls, teclaPulsAnt=NO_PULSADO,indTecla=0;
    char stringTeclado[50] = "Write password";
    unsigned int hashGen[8];
    unsigned int expectedHash[8];
    bool flagValida;

    /*CONFIGURACION DE PERIFERICOS*/
    RELOJ = ConfiguraReloj120MHz();  // Fijar la velocidad del reloj
    HabilitaPerifericos(3, SYSCTL_PERIPH_TIMER1, SYSCTL_PERIPH_EEPROM0, SYSCTL_PERIPH_CCM0); //LISTA DE TODOS LOS PERIFERICOS UTILIZADOS
    ConfiguraTimer(1, TIME, IntTimer1);
    ModoBajoConsumo(1, SYSCTL_PERIPH_TIMER1);
    ConfiguraPantalla(1, RELOJ);
    ConfiguraEEPROM();
    ConfiguraCCM_SHA256();     //Configuracion del modulo criptografico de aceleracion por hardware


    // Depuracion de tiempo en ejecucion
    SysTickIntRegister(IntTick);
    SysTickPeriodSet(12000);
    SysTickIntEnable();
    SysTickEnable();

    IntMasterEnable(); // Habilitacion global de interrupciones


    // CARGAR IMAGENES EN RAM DE PANTALLA
    cargar_imagenes();


    /*--------------------------------------------------------------*/
    //EJECUTAR PREVIAMENTE PARA INICIALIZAR MEMORIA ROM LA PRIMERA VEZ CON LOS VALORES PREDETERMINADOS DE FABRICA
    /*--------------------------------------------------------------*/
    //    unsigned int borrar=0;
    //    EEPROMProgram(&borrar, 0x0, sizeof(borrar)); //Escribir EEPROM
    //    EEPROMProgram(&borrar, 0x4, sizeof(borrar));
    //    int i;
    //    for(i=0;i<10;i++){
    //        EEPROMProgram(&borrar, 0x8+i*4, sizeof(borrar));
    //    }
    //    char stringAux[50] = "BETIS";
    //    SHAMD5DataProcess(SHAMD5_BASE, (uint32_t *) stringAux, 5, hashGen); //calcular hash a partir de algoritmo SHA256
    //    for(i=0;i<8;i++)
    //    {
    //        EEPROMProgram(&(hashGen[i]), 0x30+i*4, sizeof(hashGen[i]));
    //    }
    /*--------------------------------------------------------------*/

    //Leer EEPROM al comienzo del programa
    EEPROMRead(&volumen, 0x0, sizeof(volumen));
    volumenAnt=volumen;
    EEPROMRead(&progreso_total, 0x4, sizeof(progreso_total));
    for(i=0;i<10;i++){
        EEPROMRead(&best_scores[i], 0x8+i*4, sizeof(best_scores[i]));
    }

    // Configuracion inicial sonido y pantalla
    VolNota(volumen);
    TocaNota(xilofono, nota_xilofono);
    dibuja_imagen(atrImagenes[10], desplaz, desplaz, 30);

    /*--------------------------------------------------------------*/
    // BUCLE PRINCIPAL
    /*--------------------------------------------------------------*/
    while (1)
    {
        SLEEP; // dormir y despertar a los 50 ms para hacer sincrono el funcionamiento
        Lee_pantalla();
        switch (estado)
        {

        case pantalla_inicio:
            Nueva_pantalla(0, 0, 0);
            //ANIMACION
            ComColor(255, 255, 255);
            ComTXT(160, 15, 22, OPT_CENTER, "DIGITAL HANDWRITING TRAINER");
            if(flagDerecha){
                desplaz++;
                if(desplaz>=240)
                    flagDerecha = false;
            }
            else{
                desplaz --;
                if(desplaz<=0)
                    flagDerecha = true;
            }
            dibuja_imagen(atrImagenes[10], (2-((float)desplaz*0.005)), desplaz, 30);
            Dibuja();

            if (POSY != 0x8000)
            {
                LeeImagenFormatoL1(atrImagenes[digitoActual]);
                resetImagenDin();
                estado = espera_login;
            }

            break;

        case espera_login:
            if (POSY == 0x8000){
                FinNota();
                estado = login;
            }
            break;

        case login:
            Nueva_pantalla(0, 0, 0);
            teclaPuls=teclado();
            if(teclaPuls != NO_PULSADO && teclaPuls != teclaPulsAnt){
                if(teclaPuls == ENTER)
                {
                    if(indTecla>0)
                    {
                        SHAMD5DataProcess(SHAMD5_BASE, (uint32_t *) stringTeclado, indTecla-1, hashGen);
                        flagValida=true;
                        for(i=0;i<8;i++)
                        {
                            EEPROMRead(&(expectedHash[i]), 0x30+i*4, sizeof(expectedHash[i]));

                            if(hashGen[i]!=expectedHash[i])
                                flagValida=false;
                        }
                        if(flagValida)
                        {
                            estado = espera_soltar;
                            indTecla=0;
                        }

                    }

                }
                else if(teclaPuls == DEL){
                    stringTeclado[indTecla-1]='\0';
                    indTecla --;
                    if(indTecla<=0) indTecla=0;
                }
                else{
                    stringTeclado[indTecla]=(char)teclaPuls;
                    stringTeclado[indTecla+1]='\0';
                    indTecla ++;
                }
            }
            teclaPulsAnt = teclaPuls;
            ComColor(255, 255, 255);
            ComTXT(160, 20, 21, OPT_CENTER, stringTeclado);
            Dibuja();

            break;

        case espera_soltar:
            if (POSY == 0x8000){
                FinNota();
                if(flagChangePWD)
                {
                    strcpy(stringTeclado,"Write new password");
                    estado = cambia_pwd;
                }
                else
                {
                    strcpy(stringTeclado,"Write current password");
                    estado =principal;
                }
            }
            break;

        case principal:
            Nueva_pantalla(0, 0, 0);
            ejercicio_dibujar();
            Dibuja();

            //Borrar dibujado
            if(CLEAR){
                flagClear=true;
            }
            if(!CLEAR && flagClear){
                flagClear=false;
                resetImagenDin();
                TocaNota(notify, nota_notify);
            }
            //Siguiente digito
            if(NEXT){
                digitoActual++;
                if(digitoActual>9)
                    digitoActual=0;
                TocaNota(notify, nota_notify);
                estado =next_digito;
            }
            //Anterior digito
            if(PREVIUS){
                digitoActual--;
                if(digitoActual<0)
                    digitoActual=9;
                TocaNota(notify, nota_notify);
                estado = previus_digito;
            }
            //Resultados
            if(SCORE){
                scoreRes = calculaScore();
                // Actualiza progreso
                if(best_scores[digitoActual] < (int)(scoreRes*100)){
                    best_scores[digitoActual] = scoreRes*100;
                    EEPROMProgram(&best_scores[digitoActual], 0x8+digitoActual*4, sizeof(best_scores[digitoActual]));
                }
                progreso_total = 0;
                for(i=0;i<10;i++) progreso_total += (int)(best_scores[i]/100);
                EEPROMProgram(&progreso_total, 0x4, sizeof(progreso_total));
                resetImagenDin();
                TocaNota(scoreSound, nota_score);
                estado = resultados;
            }
            //Configuracion
            if(CONFIG){
                estado = configuracion;
            }

            break;

        case next_digito:
            if(!NEXT){
                LeeImagenFormatoL1(atrImagenes[digitoActual]);
                resetImagenDin();
                estado = principal;
            }
            break;

        case previus_digito:
            if(!PREVIUS){
                LeeImagenFormatoL1(atrImagenes[digitoActual]);
                resetImagenDin();
                estado = principal;
            }
            break;


        case resultados:
            if(!SCORE){
                Nueva_pantalla(0, 153, 153);
                //Score
                sprintf(stringScore, "SCORE = %.2f", scoreRes);
                ComColor(255, 255, 255);
                ComTXT(150, 110, 31, OPT_CENTER, stringScore);
                //Comentario
                ComColor(255, 255, 255);
                ComTXT(150, 50, 31, OPT_CENTER, string);
                //Boton Acceso Panel Progreso
                PROGRESS;
                Dibuja();
                if(POSY != 0x8000){
                    estado = espera_soltar;
                }
                if(PROGRESS){
                    resetImagenDin();
                    estado = progreso;
                }
            }
            break;

        case progreso:
            if(!PROGRESS){
                Nueva_pantalla(200, 0, 100);
                progress_list();
                ComFgcolor(50,50,255);
                ComTXT(272, 80, 18, OPT_CENTER, "TOTAL");
                ComProgressBar(260, 105, 25, 100, 0, 100-progreso_total, 100);
                Dibuja();

                if(POSY != 0x8000){
                    estado = espera_soltar;
                }
            }
            break;

        case configuracion:
            if(!CONFIG){
                Nueva_pantalla(160, 160, 160);
                ComTXT(160, 20, 28, OPT_CENTER, "CONFIGURATION PANEL");
                // Volumen
                ComSlider(140, 50, 150, 15, 0, &volumen, 255);
                if(volumen != volumenAnt){
                    volumenAnt = volumen;
                    EEPROMProgram(&volumen, 0x0, sizeof(volumen)); //Write EEPROM
                    VolNota(volumen);
                }
                ComFgcolor(255,255,255);
                ComTXT(60, 55, 23, OPT_CENTER, "Volume:");
                // Reseteo calificaciones
                ComTXT(80, 105, 23, OPT_CENTER, "Reset Progress:");
                ComTXT(80, 140, 23, OPT_CENTERX, "Change Key:");
                ComFgcolor(0,0,0);
                RESET;
                if(RESET && !flagReset)
                    flagReset = true;
                if(!RESET && flagReset){
                    flagReset = false;
                    flagTime = true;
                    for(i=0;i<10;i++){
                        best_scores[i]=0;
                        EEPROMProgram(&best_scores[i], 0x8+i*4, sizeof(best_scores[i]));
                    }
                    progreso_total = 0;
                    t=0;
                }
                if(t<((1000/TIME)*3) && flagTime){
                    ComTXT(270, 100, 23, OPT_CENTERX, "DONE");
                }
                else if(t>=(200*2) && flagTime)
                    flagTime = false;

                //Boton cambio de password
                ComFgcolor(0,0,0);
                if(CHANGE && !flagChange)
                {
                    flagChange=true;
                }
                if(!CHANGE && flagChange)
                {
                    flagChange=false;
                    flagChangePWD = true;
                    estado = login;
                }
                // Salida a principal
                ComFgcolor(0,0,0);
                BACK;
                Dibuja();

                if(BACK)
                    estado = espera_soltar;
            }
            break;

        case cambia_pwd:
            Nueva_pantalla(0, 0, 0);
            teclaPuls=teclado();
            if(teclaPuls != NO_PULSADO && teclaPuls != teclaPulsAnt){
                if(teclaPuls == ENTER)
                {
                    if(indTecla>0)
                    {
                        SHAMD5DataProcess(SHAMD5_BASE, (uint32_t *) stringTeclado, indTecla-1, hashGen);
                        flagValida=true;
                        for(i=0;i<8;i++)
                        {
                            EEPROMProgram(&(hashGen[i]), 0x30+i*4, sizeof(hashGen[i]));
                        }
                        flagChangePWD=false;
                        indTecla=0;
                        estado =espera_soltar;
                    }
                }
                else if(teclaPuls == DEL){
                    stringTeclado[indTecla-1]='\0';
                    indTecla --;
                    if(indTecla<=0) indTecla=0;
                }
                else{
                    stringTeclado[indTecla]=(char)teclaPuls;
                    stringTeclado[indTecla+1]='\0';
                    indTecla ++;
                }
            }
            teclaPulsAnt = teclaPuls;
            ComColor(255, 255, 255);
            ComTXT(160, 20, 21, OPT_CENTER, stringTeclado);
            Dibuja();


            break;

        default:
            break;
        }
    }
}


/*--------------------------------------------------------------*/
// FUNCIONES PROPIAS Y SUBRUTINAS AUXILIARES
/*--------------------------------------------------------------*/

void SysCtlSleepFake(void)
{
    // rutina de modo dormir falsa

    while (!Flag_ints);
    Flag_ints = false;
}

void IntTimer1(void)
{
    // rutina de interrupcion del timer 1

    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    Flag_ints = true;
    t++;
    SysCtlDelay(10);
}

void ejercicio_dibujar(void)
{
    // Representacion de imagenes y tratamiento de datos de la pantalla principal donde se realiza la actividad

    float factor_escala = 3.2;
    int xUpperCorner = 10;
    int yUpperCorner = 5;
    int dimX = 64;
    int dimY = 64;
    char stringWidth[20];
    long int tiempo = ticks;
    int x_ind, y_ind;
    int dirAsociada;
    int radio,fila,col;
    bool flagSonido = false;

    radio = 0.3*radioTrazo; //Radio del trazo para pintar
    // Comprobar coordenada de localizacion, marcar fallo o acierto, actualizar imagen dinamica y reproducir nota
    if ((POSY != 0x8000) && (POSX > xUpperCorner && POSX < (dimX * factor_escala + xUpperCorner) && POSY > yUpperCorner && POSY < (dimY * factor_escala + yUpperCorner)))
    {

        x_ind = (int)(POSX - xUpperCorner) / factor_escala;
        y_ind = (int)(POSY - yUpperCorner) / factor_escala;
        for(fila=y_ind-radio;fila<=y_ind+radio;fila++){
            for(col=x_ind-radio;col<=x_ind+radio;col++){
                if(fila>=0 && fila<dimY && col>=0 && col<dimX){
                    if (matriz_imagen_original[fila][col]==1)
                    {
                        matrizDibujoRealizado[fila][col] = 1; // Zona blanca = Verde (1)
                        matriz_imagen_din_RGB[fila][col][0] = 0;
                        matriz_imagen_din_RGB[fila][col][1] = 7;
                        matriz_imagen_din_RGB[fila][col][2] = 0;
                    }
                    else
                    {
                        matrizDibujoRealizado[fila][col] = -1; // Zona negra = Roja (-1)
                        matriz_imagen_din_RGB[fila][col][0] = 7;
                        matriz_imagen_din_RGB[fila][col][1] = 0;
                        matriz_imagen_din_RGB[fila][col][2] = 0;
                        flagSonido = true;
                    }
                }

            }
        }
        // Actualizo imagen dinamica
        for(fila=y_ind-radio;fila<=y_ind+radio;fila++){
            for(col=x_ind-radio;col<=x_ind+radio;col++){
                if(fila>=0 && fila<dimY && col>=0 && col<dimX){
                    dirAsociada = (col + fila * 64);
                    vector_codificado[dirAsociada] = (matriz_imagen_din_RGB[fila][col][0] << 5) + (matriz_imagen_din_RGB[fila][col][1] << 2) + (matriz_imagen_din_RGB[fila][col][2]);
                    int imagen = (vector_codificado[dirAsociada + 3] << 24) | (vector_codificado[dirAsociada + 2] << 16) | (vector_codificado[dirAsociada + 1] << 8) | vector_codificado[dirAsociada];
                    Esc_Reg(RAM_G + 10 * 512 + 10000 + dirAsociada, imagen);
                }
            }
        }
        if(flagSonido)
            TocaNota(alarm, nota_alarm);
    }
    else
        FinNota();


    dibuja_imagen((struct caracteristicasImagen){(RAM_G) + (10 * 512)+10000, RGB332, {dimX, dimY}}, factor_escala, xUpperCorner, yUpperCorner);

    // Dibujo rectangulo en blanco
    ComColor(255, 255, 255);
    ComRect(xUpperCorner, yUpperCorner, (dimX * factor_escala + xUpperCorner), (dimY * factor_escala + yUpperCorner), 0);

    // Boton de borrado
    ComFgcolor(0,150,150);
    CLEAR;

    // Boton siguiente digito
    ComFgcolor(255,0,127);
    NEXT;

    // Boton previo digito
    ComFgcolor(255,0,127);
    PREVIUS;

    // Boton resultados
    ComFgcolor(100,200,30);
    SCORE;

    // Slider grosor trazos
    ComSlider(20, 220, 100, 15, 0, &radioTrazo, 10);
    ComFgcolor(255,255,255);
    sprintf(stringWidth, "PEN SIZE : %d", radio+1);
    ComTXT(175, 225, 20, OPT_CENTER, stringWidth);

    // Boton configuracion
    ComFgcolor(160,160,160);
    CONFIG;

}

void dibuja_imagen(struct caracteristicasImagen parametrosImagen, float factor_escala, int xUpperCorner, int yUpperCorner)
{
    // Representacion de imagenes contenidas en RAM de pantalla

    ComColor(255, 255, 255);

    int linestride;
    switch (parametrosImagen.format)
    {
    case L1:
        linestride = parametrosImagen.dim.x / 8;
        break;
    case RGB332:
        linestride = parametrosImagen.dim.x / 1;
        break;
    }

    Comando(CMD_BITMAP_SOURCE + RAM_G + parametrosImagen.addr);
    switch (parametrosImagen.format)
    {
    case L1:
        Comando(CMD_BITMAP_LAYOUT + FORMAT_L1 + (linestride << 9) + parametrosImagen.dim.y);
        break;
    case RGB332:
        Comando(CMD_BITMAP_LAYOUT + FORMAT_RGB332 + (linestride << 9) + parametrosImagen.dim.y);
        break;
    }
    Comando(CMD_BITMAP_TRANSFORM_A + (int)(1 / factor_escala * 256));
    Comando(CMD_BITMAP_TRANSFORM_E + (int)(1 / factor_escala * 256));
    Comando(CMD_BITMAP_SIZE + (0 << 20) + (0 << 19) + (0 << 18) + ((int)(parametrosImagen.dim.x * factor_escala) << 9) + (int)(parametrosImagen.dim.y * factor_escala));
    Comando(CMD_BEGIN_BMP);
    Comando(CMD_VERTEX2II + (xUpperCorner << 21) + (yUpperCorner << 12) + (0 << 7) + 0);
    Comando(CMD_BITMAP_TRANSFORM_A + 256);
    Comando(CMD_BITMAP_TRANSFORM_E + 256);
}

void cargar_imagenes(void)
{
    // Almacenar imagenes propias en memoria RAM de pantalla

    unsigned long imagen;
    int i, j;

    // DIGITOS
    const unsigned char imagenDigitos[10][512] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 240, 0, 0, 0, 0, 0, 0, 31, 255, 128, 0, 0, 0, 0, 0, 127, 255, 224, 0, 0, 0, 0, 0, 255, 255, 240, 0, 0, 0, 0, 3, 255, 255, 248, 0, 0, 0, 0, 7, 255, 255, 252, 0, 0, 0, 0, 7, 252, 7, 254, 0, 0, 0, 0, 15, 240, 1, 255, 0, 0, 0, 0, 31, 224, 0, 255, 0, 0, 0, 0, 31, 192, 0, 127, 128, 0, 0, 0, 63, 128, 0, 63, 128, 0, 0, 0, 63, 128, 0, 63, 192, 0, 0, 0, 63, 0, 0, 31, 192, 0, 0, 0, 127, 0, 0, 31, 192, 0, 0, 0, 127, 0, 0, 31, 192, 0, 0, 0, 127, 0, 0, 31, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 254, 0, 0, 15, 224, 0, 0, 0, 255, 0, 0, 31, 192, 0, 0, 0, 127, 0, 0, 31, 192, 0, 0, 0, 127, 0, 0, 31, 192, 0, 0, 0, 127, 0, 0, 31, 128, 0, 0, 0, 127, 128, 0, 63, 128, 0, 0, 0, 63, 128, 0, 63, 128, 0, 0, 0, 63, 192, 0, 127, 0, 0, 0, 0, 31, 224, 0, 255, 0, 0, 0, 0, 31, 240, 1, 254, 0, 0, 0, 0, 15, 248, 7, 252, 0, 0, 0, 0, 15, 255, 255, 252, 0, 0, 0, 0, 7, 255, 255, 248, 0, 0, 0, 0, 3, 255, 255, 240, 0, 0, 0, 0, 0, 255, 255, 192, 0, 0, 0, 0, 0, 63, 255, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 7, 252, 0, 0, 0, 0, 0, 0, 15, 252, 0, 0, 0, 0, 0, 0, 63, 252, 0, 0, 0, 0, 0, 0, 255, 252, 0, 0, 0, 0, 0, 1, 255, 252, 0, 0, 0, 0, 0, 7, 255, 252, 0, 0, 0, 0, 0, 15, 255, 252, 0, 0, 0, 0, 0, 63, 249, 252, 0, 0, 0, 0, 0, 127, 225, 252, 0, 0, 0, 0, 0, 127, 193, 252, 0, 0, 0, 0, 0, 127, 1, 252, 0, 0, 0, 0, 0, 124, 1, 252, 0, 0, 0, 0, 0, 112, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 31, 255, 255, 255, 128, 0, 0, 0, 63, 255, 255, 255, 192, 0, 0, 0, 127, 255, 255, 255, 192, 0, 0, 0, 127, 255, 255, 255, 192, 0, 0, 0, 127, 255, 255, 255, 192, 0, 0, 0, 63, 255, 255, 255, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 254, 0, 0, 0, 0, 0, 0, 255, 255, 128, 0, 0, 0, 0, 3, 255, 255, 224, 0, 0, 0, 0, 15, 255, 255, 240, 0, 0, 0, 0, 31, 255, 255, 248, 0, 0, 0, 0, 31, 255, 255, 252, 0, 0, 0, 0, 31, 224, 15, 252, 0, 0, 0, 0, 31, 128, 3, 254, 0, 0, 0, 0, 30, 0, 1, 254, 0, 0, 0, 0, 24, 0, 1, 254, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 3, 248, 0, 0, 0, 0, 0, 0, 3, 248, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 15, 240, 0, 0, 0, 0, 0, 0, 15, 224, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 127, 128, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 1, 254, 0, 0, 0, 0, 0, 0, 3, 252, 0, 0, 0, 0, 0, 0, 7, 248, 0, 0, 0, 0, 0, 0, 15, 240, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 127, 128, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 1, 254, 0, 0, 0, 0, 0, 0, 3, 252, 0, 0, 0, 0, 0, 0, 7, 248, 0, 0, 0, 0, 0, 0, 15, 240, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 63, 255, 255, 255, 0, 0, 0, 0, 63, 255, 255, 255, 192, 0, 0, 0, 63, 255, 255, 255, 192, 0, 0, 0, 63, 255, 255, 255, 192, 0, 0, 0, 63, 255, 255, 255, 192, 0, 0, 0, 63, 255, 255, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 254, 0, 0, 0, 0, 0, 0, 127, 255, 192, 0, 0, 0, 0, 1, 255, 255, 224, 0, 0, 0, 0, 7, 255, 255, 248, 0, 0, 0, 0, 15, 255, 255, 252, 0, 0, 0, 0, 31, 254, 63, 252, 0, 0, 0, 0, 31, 224, 7, 254, 0, 0, 0, 0, 31, 128, 3, 254, 0, 0, 0, 0, 30, 0, 1, 255, 0, 0, 0, 0, 8, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 1, 254, 0, 0, 0, 0, 0, 0, 3, 252, 0, 0, 0, 0, 0, 0, 3, 252, 0, 0, 0, 0, 0, 0, 15, 248, 0, 0, 0, 0, 0, 0, 63, 240, 0, 0, 0, 0, 0, 255, 255, 224, 0, 0, 0, 0, 1, 255, 255, 0, 0, 0, 0, 0, 1, 255, 255, 0, 0, 0, 0, 0, 1, 255, 255, 224, 0, 0, 0, 0, 1, 255, 255, 248, 0, 0, 0, 0, 0, 127, 255, 252, 0, 0, 0, 0, 0, 0, 15, 254, 0, 0, 0, 0, 0, 0, 1, 255, 0, 0, 0, 0, 0, 0, 0, 255, 128, 0, 0, 0, 0, 0, 0, 127, 128, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 56, 0, 0, 127, 128, 0, 0, 0, 62, 0, 0, 255, 0, 0, 0, 0, 63, 192, 3, 255, 0, 0, 0, 0, 63, 255, 255, 254, 0, 0, 0, 0, 63, 255, 255, 252, 0, 0, 0, 0, 31, 255, 255, 248, 0, 0, 0, 0, 15, 255, 255, 224, 0, 0, 0, 0, 3, 255, 255, 128, 0, 0, 0, 0, 0, 63, 252, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 1, 255, 128, 0, 0, 0, 0, 0, 3, 255, 128, 0, 0, 0, 0, 0, 7, 255, 128, 0, 0, 0, 0, 0, 7, 255, 128, 0, 0, 0, 0, 0, 15, 255, 128, 0, 0, 0, 0, 0, 15, 255, 128, 0, 0, 0, 0, 0, 31, 191, 128, 0, 0, 0, 0, 0, 31, 191, 128, 0, 0, 0, 0, 0, 63, 63, 128, 0, 0, 0, 0, 0, 127, 63, 128, 0, 0, 0, 0, 0, 126, 63, 128, 0, 0, 0, 0, 0, 252, 63, 128, 0, 0, 0, 0, 0, 252, 63, 128, 0, 0, 0, 0, 1, 248, 63, 128, 0, 0, 0, 0, 3, 248, 63, 128, 0, 0, 0, 0, 3, 240, 63, 128, 0, 0, 0, 0, 7, 224, 63, 128, 0, 0, 0, 0, 7, 224, 63, 128, 0, 0, 0, 0, 15, 192, 63, 128, 0, 0, 0, 0, 31, 192, 63, 128, 0, 0, 0, 0, 31, 128, 63, 128, 0, 0, 0, 0, 63, 128, 63, 128, 0, 0, 0, 0, 63, 0, 63, 128, 0, 0, 0, 0, 126, 0, 63, 128, 0, 0, 0, 0, 126, 0, 63, 128, 0, 0, 0, 0, 252, 0, 63, 128, 0, 0, 0, 1, 252, 0, 63, 128, 0, 0, 0, 1, 248, 0, 63, 128, 0, 0, 0, 3, 240, 0, 63, 128, 0, 0, 0, 3, 240, 0, 63, 128, 0, 0, 0, 7, 224, 0, 63, 128, 0, 0, 0, 15, 224, 0, 63, 128, 0, 0, 0, 15, 192, 0, 63, 128, 0, 0, 0, 15, 192, 0, 63, 192, 0, 0, 0, 31, 255, 255, 255, 255, 0, 0, 0, 31, 255, 255, 255, 255, 128, 0, 0, 31, 255, 255, 255, 255, 128, 0, 0, 31, 255, 255, 255, 255, 0, 0, 0, 15, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 255, 255, 254, 0, 0, 0, 0, 7, 255, 255, 255, 0, 0, 0, 0, 7, 255, 255, 255, 0, 0, 0, 0, 7, 255, 255, 254, 0, 0, 0, 0, 7, 255, 255, 254, 0, 0, 0, 0, 7, 255, 255, 252, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 255, 254, 0, 0, 0, 0, 0, 7, 255, 255, 192, 0, 0, 0, 0, 7, 255, 255, 240, 0, 0, 0, 0, 7, 255, 255, 252, 0, 0, 0, 0, 7, 255, 255, 254, 0, 0, 0, 0, 7, 224, 127, 255, 0, 0, 0, 0, 0, 0, 3, 255, 128, 0, 0, 0, 0, 0, 0, 255, 128, 0, 0, 0, 0, 0, 0, 127, 192, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 127, 128, 0, 0, 0, 0, 0, 0, 255, 128, 0, 0, 0, 60, 0, 1, 255, 0, 0, 0, 0, 63, 0, 3, 254, 0, 0, 0, 0, 63, 240, 127, 254, 0, 0, 0, 0, 63, 255, 255, 252, 0, 0, 0, 0, 63, 255, 255, 240, 0, 0, 0, 0, 31, 255, 255, 224, 0, 0, 0, 0, 7, 255, 255, 128, 0, 0, 0, 0, 0, 255, 252, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 255, 224, 0, 0, 0, 0, 0, 15, 255, 252, 0, 0, 0, 0, 0, 63, 255, 254, 0, 0, 0, 0, 0, 255, 255, 254, 0, 0, 0, 0, 1, 255, 255, 254, 0, 0, 0, 0, 3, 255, 192, 254, 0, 0, 0, 0, 7, 254, 0, 14, 0, 0, 0, 0, 7, 248, 0, 0, 0, 0, 0, 0, 15, 240, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 126, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 252, 0, 0, 0, 0, 0, 0, 0, 252, 15, 255, 128, 0, 0, 0, 0, 252, 127, 255, 224, 0, 0, 0, 0, 253, 255, 255, 248, 0, 0, 0, 1, 255, 255, 255, 252, 0, 0, 0, 1, 255, 255, 255, 254, 0, 0, 0, 1, 255, 248, 7, 254, 0, 0, 0, 1, 255, 192, 1, 255, 0, 0, 0, 1, 255, 0, 0, 255, 0, 0, 0, 1, 252, 0, 0, 127, 128, 0, 0, 1, 252, 0, 0, 63, 128, 0, 0, 1, 252, 0, 0, 63, 128, 0, 0, 0, 252, 0, 0, 63, 128, 0, 0, 0, 252, 0, 0, 63, 128, 0, 0, 0, 254, 0, 0, 63, 128, 0, 0, 0, 254, 0, 0, 31, 128, 0, 0, 0, 254, 0, 0, 63, 128, 0, 0, 0, 254, 0, 0, 63, 128, 0, 0, 0, 254, 0, 0, 63, 128, 0, 0, 0, 254, 0, 0, 63, 128, 0, 0, 0, 127, 0, 0, 63, 128, 0, 0, 0, 127, 0, 0, 63, 128, 0, 0, 0, 127, 0, 0, 127, 0, 0, 0, 0, 127, 128, 0, 255, 0, 0, 0, 0, 63, 192, 0, 254, 0, 0, 0, 0, 63, 192, 1, 254, 0, 0, 0, 0, 31, 240, 7, 252, 0, 0, 0, 0, 31, 254, 127, 248, 0, 0, 0, 0, 15, 255, 255, 240, 0, 0, 0, 0, 7, 255, 255, 224, 0, 0, 0, 0, 3, 255, 255, 192, 0, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 0, 0, 31, 248, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 255, 255, 255, 192, 0, 0, 0, 127, 255, 255, 255, 224, 0, 0, 0, 127, 255, 255, 255, 224, 0, 0, 0, 127, 255, 255, 255, 224, 0, 0, 0, 127, 255, 255, 255, 224, 0, 0, 0, 127, 255, 255, 255, 224, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 3, 252, 0, 0, 0, 0, 0, 0, 3, 248, 0, 0, 0, 0, 0, 0, 3, 248, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 15, 224, 0, 0, 0, 0, 0, 0, 15, 224, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 1, 254, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 3, 252, 0, 0, 0, 0, 0, 0, 3, 248, 0, 0, 0, 0, 0, 0, 3, 248, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 7, 240, 0, 0, 0, 0, 0, 0, 15, 240, 0, 0, 0, 0, 0, 0, 15, 224, 0, 0, 0, 0, 0, 0, 31, 224, 0, 0, 0, 0, 0, 0, 31, 192, 0, 0, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 127, 128, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 252, 0, 0, 0, 0, 0, 0, 127, 255, 128, 0, 0, 0, 0, 1, 255, 255, 224, 0, 0, 0, 0, 7, 255, 255, 248, 0, 0, 0, 0, 15, 255, 255, 252, 0, 0, 0, 0, 31, 254, 31, 252, 0, 0, 0, 0, 31, 224, 3, 254, 0, 0, 0, 0, 63, 192, 1, 254, 0, 0, 0, 0, 63, 128, 0, 255, 0, 0, 0, 0, 127, 0, 0, 127, 0, 0, 0, 0, 127, 0, 0, 127, 0, 0, 0, 0, 127, 0, 0, 127, 0, 0, 0, 0, 127, 0, 0, 127, 0, 0, 0, 0, 127, 0, 0, 127, 0, 0, 0, 0, 127, 0, 0, 127, 0, 0, 0, 0, 127, 0, 0, 127, 0, 0, 0, 0, 127, 128, 0, 126, 0, 0, 0, 0, 63, 128, 0, 254, 0, 0, 0, 0, 63, 192, 1, 252, 0, 0, 0, 0, 31, 224, 3, 252, 0, 0, 0, 0, 15, 248, 7, 248, 0, 0, 0, 0, 15, 252, 15, 240, 0, 0, 0, 0, 7, 255, 63, 224, 0, 0, 0, 0, 1, 255, 255, 128, 0, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 0, 0, 63, 254, 0, 0, 0, 0, 0, 0, 63, 255, 0, 0, 0, 0, 0, 0, 127, 255, 192, 0, 0, 0, 0, 1, 255, 255, 224, 0, 0, 0, 0, 7, 254, 127, 248, 0, 0, 0, 0, 15, 248, 31, 252, 0, 0, 0, 0, 31, 240, 7, 254, 0, 0, 0, 0, 63, 192, 1, 255, 0, 0, 0, 0, 127, 128, 0, 255, 0, 0, 0, 0, 127, 0, 0, 127, 128, 0, 0, 0, 254, 0, 0, 63, 128, 0, 0, 0, 254, 0, 0, 63, 192, 0, 0, 1, 252, 0, 0, 31, 192, 0, 0, 1, 252, 0, 0, 31, 192, 0, 0, 1, 252, 0, 0, 31, 192, 0, 0, 1, 252, 0, 0, 31, 192, 0, 0, 1, 252, 0, 0, 31, 192, 0, 0, 1, 254, 0, 0, 31, 192, 0, 0, 0, 254, 0, 0, 63, 192, 0, 0, 0, 255, 0, 0, 63, 128, 0, 0, 0, 255, 0, 0, 127, 128, 0, 0, 0, 127, 192, 0, 255, 0, 0, 0, 0, 127, 240, 7, 255, 0, 0, 0, 0, 63, 255, 255, 254, 0, 0, 0, 0, 31, 255, 255, 252, 0, 0, 0, 0, 15, 255, 255, 248, 0, 0, 0, 0, 3, 255, 255, 224, 0, 0, 0, 0, 0, 127, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 248, 0, 0, 0, 0, 0, 0, 63, 255, 128, 0, 0, 0, 0, 0, 255, 255, 192, 0, 0, 0, 0, 3, 255, 255, 240, 0, 0, 0, 0, 7, 255, 255, 248, 0, 0, 0, 0, 15, 255, 255, 252, 0, 0, 0, 0, 31, 248, 7, 252, 0, 0, 0, 0, 31, 224, 1, 254, 0, 0, 0, 0, 63, 192, 0, 254, 0, 0, 0, 0, 63, 128, 0, 255, 0, 0, 0, 0, 127, 0, 0, 127, 0, 0, 0, 0, 127, 0, 0, 127, 128, 0, 0, 0, 127, 0, 0, 63, 128, 0, 0, 0, 254, 0, 0, 63, 128, 0, 0, 0, 254, 0, 0, 63, 128, 0, 0, 0, 254, 0, 0, 31, 192, 0, 0, 0, 254, 0, 0, 31, 192, 0, 0, 0, 254, 0, 0, 31, 192, 0, 0, 0, 254, 0, 0, 31, 192, 0, 0, 0, 254, 0, 0, 31, 192, 0, 0, 0, 254, 0, 0, 31, 192, 0, 0, 0, 255, 0, 0, 31, 192, 0, 0, 0, 127, 0, 0, 31, 192, 0, 0, 0, 127, 0, 0, 31, 192, 0, 0, 0, 127, 128, 0, 31, 192, 0, 0, 0, 63, 192, 0, 127, 192, 0, 0, 0, 63, 240, 3, 255, 192, 0, 0, 0, 31, 255, 255, 255, 192, 0, 0, 0, 15, 255, 255, 255, 192, 0, 0, 0, 7, 255, 255, 255, 192, 0, 0, 0, 3, 255, 255, 159, 192, 0, 0, 0, 0, 255, 254, 31, 192, 0, 0, 0, 0, 15, 224, 31, 192, 0, 0, 0, 0, 0, 0, 31, 128, 0, 0, 0, 0, 0, 0, 31, 128, 0, 0, 0, 0, 0, 0, 31, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 128, 0, 0, 0, 0, 0, 0, 63, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 0, 254, 0, 0, 0, 0, 0, 0, 1, 252, 0, 0, 0, 0, 0, 0, 3, 252, 0, 0, 0, 0, 0, 0, 7, 248, 0, 0, 0, 0, 48, 0, 31, 240, 0, 0, 0, 0, 63, 0, 255, 224, 0, 0, 0, 0, 127, 255, 255, 192, 0, 0, 0, 0, 63, 255, 255, 128, 0, 0, 0, 0, 63, 255, 255, 0, 0, 0, 0, 0, 63, 255, 252, 0, 0, 0, 0, 0, 7, 255, 224, 0, 0, 0, 0, 0, 0, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    for (j = 0; j < 10; j++)
    {
        atrImagenes[j].addr = (RAM_G) + (j * 512);
        atrImagenes[j].format = L1;
        atrImagenes[j].dim.x = 64;
        atrImagenes[j].dim.y = 64;
        for (i = 0; i < 512; i += 4)
        {
            imagen = (imagenDigitos[j][i + 3] << 24) | (imagenDigitos[j][i + 2] << 16) | (imagenDigitos[j][i + 1] << 8) | imagenDigitos[j][i];
            Esc_Reg(atrImagenes[j].addr + i, imagen);
        }
    }

    // LOGO
    const unsigned char imagenLogo[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 36, 41, 41, 41, 41, 41, 41, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 45, 82, 86, 123, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 123, 118, 86, 45, 36, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 45, 86, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 123, 77, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 45, 123, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 82, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 41, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 36, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 0, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 77, 4, 68, 168, 32, 45, 123, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 136, 32, 0, 45, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 122, 4, 100, 68, 0, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 41, 32, 136, 236, 168, 0, 86, 159, 159, 123, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 82, 81, 0, 204, 236, 100, 0, 4, 82, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 68, 204, 236, 32, 4, 159, 159, 159, 159, 159, 159, 159, 159, 127, 82, 4, 68, 168, 236, 236, 136, 4, 123, 159, 159, 159, 159, 118, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 123, 159, 127, 4, 100, 236, 236, 204, 100, 0, 4, 86, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 168, 236, 236, 204, 0, 41, 159, 159, 159, 159, 159, 159, 118, 41, 32, 136, 236, 236, 236, 236, 68, 41, 159, 159, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 82, 159, 159, 159, 159, 45, 32, 236, 236, 236, 236, 204, 68, 0, 41, 118, 159, 159, 159, 159, 159, 123, 4, 100, 236, 236, 236, 236, 168, 0, 82, 159, 159, 159, 127, 82, 0, 68, 168, 236, 236, 236, 236, 204, 32, 82, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 118, 159, 159, 159, 159, 159, 123, 0, 136, 236, 236, 236, 236, 236, 168, 32, 0, 45, 123, 159, 159, 159, 45, 32, 204, 236, 236, 236, 236, 236, 100, 0, 86, 159, 123, 41, 32, 100, 236, 236, 236, 236, 236, 236, 168, 0, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 123, 159, 159, 159, 159, 159, 159, 159, 41, 32, 236, 236, 236, 236, 236, 236, 236, 136, 32, 0, 45, 123, 86, 0, 136, 236, 236, 236, 236, 236, 236, 236, 68, 0, 45, 0, 68, 168, 236, 236, 236, 236, 236, 236, 236, 100, 4, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 127, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 168, 236, 236, 236, 236, 236, 236, 236, 236, 136, 0, 0, 0, 68, 236, 236, 236, 236, 236, 236, 236, 236, 236, 32, 0, 136, 236, 236, 236, 236, 236, 236, 236, 236, 236, 68, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 4, 68, 236, 236, 236, 236, 236, 236, 236, 236, 236, 204, 100, 32, 204, 236, 236, 236, 236, 236, 236, 236, 236, 236, 204, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 204, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 204, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 136, 0, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 77, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 100, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 100, 36, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 77, 32, 204, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 32, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 136, 236, 236, 236, 236, 236, 236, 236, 204, 168, 204, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 168, 0, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 32, 236, 236, 236, 236, 236, 204, 68, 68, 68, 32, 32, 136, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 136, 4, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 168, 236, 236, 236, 236, 100, 168, 236, 236, 236, 136, 32, 68, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 68, 0, 82, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 36, 68, 236, 236, 236, 236, 204, 236, 236, 236, 236, 236, 204, 32, 68, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 136, 32, 0, 0, 4, 45, 118, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 41, 0, 204, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 204, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 204, 104, 68, 68, 100, 204, 236, 204, 168, 68, 32, 0, 4, 41, 82, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 45, 0, 32, 136, 136, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 168, 36, 68, 168, 204, 204, 168, 172, 236, 236, 236, 236, 168, 100, 32, 0, 0, 4, 45, 86, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 81, 4, 32, 136, 204, 236, 204, 32, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 136, 32, 100, 236, 236, 236, 236, 236, 204, 236, 236, 236, 236, 236, 236, 236, 204, 168, 68, 0, 0, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 82, 4, 32, 100, 204, 236, 236, 236, 236, 100, 36, 68, 204, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 136, 136, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 100, 0, 45, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 77, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 86, 41, 0, 100, 204, 236, 236, 236, 236, 236, 204, 72, 212, 140, 36, 68, 136, 204, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 204, 68, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 0, 136, 236, 236, 236, 236, 236, 236, 236, 172, 104, 248, 248, 212, 140, 68, 32, 68, 136, 204, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 168, 32, 4, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 86, 0, 68, 236, 236, 236, 236, 236, 236, 236, 168, 104, 248, 248, 248, 248, 176, 146, 146, 73, 36, 36, 68, 136, 204, 204, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 136, 32, 41, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 68, 236, 236, 236, 236, 236, 236, 168, 104, 248, 248, 248, 248, 140, 219, 255, 255, 182, 104, 140, 72, 36, 36, 36, 68, 100, 136, 136, 168, 204, 204, 204, 204, 204, 204, 204, 204, 204, 236, 236, 236, 236, 100, 0, 45, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 4, 68, 204, 236, 236, 236, 236, 172, 72, 248, 248, 248, 244, 109, 255, 255, 255, 182, 144, 248, 248, 140, 182, 219, 146, 72, 104, 104, 36, 36, 68, 68, 36, 68, 72, 68, 68, 136, 236, 236, 204, 68, 4, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 86, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 32, 204, 236, 236, 236, 204, 68, 244, 248, 248, 212, 109, 255, 255, 255, 182, 176, 248, 248, 108, 255, 255, 255, 109, 248, 248, 140, 182, 255, 255, 109, 244, 248, 176, 136, 236, 236, 168, 32, 40, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 32, 204, 236, 236, 236, 68, 212, 248, 248, 244, 72, 255, 255, 255, 146, 176, 248, 244, 109, 255, 255, 255, 109, 244, 248, 108, 219, 255, 255, 109, 212, 248, 104, 204, 236, 204, 32, 41, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 41, 32, 200, 236, 236, 136, 108, 248, 248, 248, 140, 109, 219, 255, 146, 176, 248, 212, 109, 255, 255, 255, 109, 244, 248, 108, 219, 255, 255, 146, 208, 212, 104, 236, 236, 236, 68, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 100, 236, 236, 204, 36, 212, 248, 248, 248, 176, 104, 109, 104, 212, 248, 244, 72, 219, 255, 255, 141, 244, 248, 108, 182, 255, 255, 182, 176, 140, 136, 236, 236, 236, 168, 0, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 45, 32, 204, 236, 236, 236, 104, 104, 248, 248, 248, 248, 248, 212, 244, 248, 248, 248, 176, 72, 109, 146, 140, 248, 248, 212, 72, 146, 182, 109, 176, 104, 204, 236, 236, 236, 236, 68, 4, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 77, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 86, 0, 136, 236, 236, 236, 236, 236, 68, 140, 248, 248, 248, 248, 244, 208, 172, 168, 140, 208, 244, 208, 176, 244, 248, 248, 248, 244, 176, 176, 176, 140, 104, 236, 236, 236, 236, 236, 168, 0, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 159, 159, 159, 159, 159, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 68, 236, 236, 236, 236, 236, 236, 204, 36, 172, 248, 248, 208, 168, 164, 192, 224, 192, 160, 140, 244, 212, 176, 172, 208, 248, 248, 248, 248, 212, 72, 204, 236, 236, 236, 236, 236, 236, 68, 4, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 12, 86, 159, 159, 159, 159, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 32, 204, 236, 236, 236, 236, 236, 236, 236, 204, 68, 104, 172, 164, 192, 224, 224, 224, 224, 224, 160, 132, 164, 192, 192, 160, 136, 212, 248, 212, 104, 168, 236, 236, 236, 236, 236, 236, 236, 204, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 16, 25, 16, 86, 159, 159, 159, 159, 4, 0, 0, 0, 0, 0, 0, 0, 4, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 100, 32, 128, 192, 224, 224, 224, 224, 224, 224, 192, 224, 224, 224, 224, 224, 132, 176, 104, 136, 236, 236, 236, 236, 236, 236, 236, 236, 236, 68, 4, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 16, 25, 25, 25, 12, 86, 159, 159, 159, 45, 0, 0, 0, 0, 0, 0, 0, 41, 159, 159, 159, 127, 109, 77, 159, 159, 159, 159, 159, 159, 159, 114, 73, 123, 159, 159, 159, 159, 159, 159, 118, 4, 100, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 204, 100, 64, 64, 128, 160, 192, 192, 192, 224, 224, 224, 224, 192, 160, 96, 68, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 168, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 86, 16, 25, 25, 25, 25, 21, 40, 127, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 45, 159, 159, 127, 109, 192, 160, 77, 159, 159, 159, 159, 159, 114, 164, 192, 105, 123, 159, 159, 159, 159, 123, 41, 68, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 172, 136, 100, 68, 68, 64, 64, 64, 96, 64, 68, 100, 168, 236, 236, 236, 236, 236, 236, 236, 204, 168, 136, 100, 68, 32, 0, 4, 159, 159, 159, 159, 123, 118, 159, 159, 159, 159, 159, 82, 16, 25, 25, 25, 25, 25, 13, 86, 159, 159, 159, 118, 0, 0, 0, 0, 0, 0, 0, 82, 159, 127, 109, 160, 224, 224, 160, 77, 159, 159, 159, 114, 164, 224, 224, 192, 105, 123, 159, 159, 159, 45, 32, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 204, 204, 204, 204, 236, 236, 236, 236, 236, 236, 168, 100, 68, 32, 32, 0, 0, 4, 41, 45, 82, 86, 159, 159, 159, 123, 45, 12, 86, 159, 159, 159, 86, 16, 25, 25, 25, 25, 25, 17, 86, 159, 159, 159, 159, 123, 0, 0, 0, 0, 0, 0, 0, 86, 159, 82, 160, 224, 224, 224, 224, 160, 77, 159, 114, 164, 224, 224, 224, 224, 192, 77, 159, 159, 86, 0, 136, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 204, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 100, 0, 41, 77, 82, 118, 123, 127, 159, 159, 159, 159, 159, 159, 123, 45, 20, 25, 16, 82, 159, 82, 16, 25, 25, 25, 25, 25, 17, 86, 159, 159, 159, 159, 159, 123, 0, 0, 0, 0, 0, 0, 0, 118, 159, 127, 105, 192, 224, 224, 224, 224, 160, 73, 164, 224, 224, 224, 224, 192, 105, 123, 159, 123, 4, 100, 236, 236, 236, 236, 236, 236, 204, 168, 100, 68, 32, 0, 0, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 100, 4, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 45, 20, 25, 25, 25, 16, 45, 16, 25, 25, 25, 25, 25, 17, 86, 159, 159, 159, 159, 159, 159, 159, 4, 0, 0, 0, 0, 0, 0, 123, 159, 159, 127, 105, 192, 224, 224, 224, 224, 192, 224, 224, 224, 224, 192, 105, 123, 159, 159, 41, 32, 168, 204, 168, 136, 100, 68, 32, 0, 0, 4, 41, 45, 45, 0, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 100, 36, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 12, 25, 25, 25, 25, 25, 20, 25, 25, 25, 25, 25, 17, 86, 159, 159, 159, 159, 159, 159, 159, 159, 4, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 127, 105, 192, 224, 224, 224, 224, 224, 224, 224, 192, 105, 123, 159, 159, 82, 0, 0, 32, 0, 0, 4, 4, 41, 81, 86, 123, 127, 159, 159, 123, 0, 168, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 68, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 12, 25, 25, 25, 25, 25, 25, 25, 25, 25, 17, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 40, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 159, 127, 105, 192, 224, 224, 224, 224, 224, 192, 105, 123, 159, 159, 123, 4, 4, 41, 45, 82, 118, 123, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 136, 236, 236, 236, 236, 236, 236, 236, 236, 236, 136, 100, 236, 236, 236, 236, 236, 236, 236, 236, 236, 68, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 122, 12, 25, 25, 25, 25, 25, 25, 25, 17, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 159, 159, 114, 160, 224, 224, 224, 224, 224, 192, 73, 159, 159, 159, 127, 123, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 136, 236, 236, 236, 236, 236, 236, 236, 236, 136, 0, 0, 68, 204, 236, 236, 236, 236, 236, 236, 236, 68, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 12, 25, 25, 25, 25, 25, 17, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 159, 114, 164, 224, 224, 224, 224, 224, 224, 224, 160, 77, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 136, 236, 236, 236, 236, 236, 236, 236, 136, 0, 45, 118, 4, 32, 168, 236, 236, 236, 236, 236, 236, 68, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 12, 25, 25, 25, 17, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 114, 164, 224, 224, 224, 224, 224, 224, 224, 224, 224, 160, 77, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 136, 236, 236, 236, 236, 236, 236, 168, 32, 45, 127, 159, 127, 41, 0, 136, 236, 236, 236, 236, 236, 68, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 86, 16, 25, 17, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 36, 0, 0, 0, 0, 0, 0, 123, 159, 159, 114, 164, 224, 224, 224, 224, 192, 132, 192, 224, 224, 224, 224, 160, 77, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 136, 236, 236, 236, 236, 236, 168, 32, 45, 127, 159, 159, 159, 159, 45, 0, 100, 236, 236, 236, 236, 32, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 86, 12, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 4, 0, 0, 0, 0, 0, 0, 118, 159, 82, 132, 224, 224, 224, 224, 192, 105, 118, 105, 192, 224, 224, 224, 224, 160, 81, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 136, 236, 236, 236, 236, 168, 32, 41, 127, 159, 159, 159, 159, 159, 159, 82, 0, 68, 236, 236, 236, 32, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 0, 0, 0, 0, 0, 0, 86, 159, 122, 100, 224, 224, 224, 192, 105, 123, 159, 127, 105, 192, 224, 224, 224, 132, 114, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 136, 236, 236, 236, 200, 32, 41, 123, 159, 159, 159, 159, 159, 159, 159, 159, 118, 4, 32, 204, 236, 32, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 118, 100, 224, 192, 105, 123, 159, 159, 159, 127, 105, 192, 224, 132, 114, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 136, 236, 236, 204, 32, 41, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 36, 0, 136, 32, 77, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 0, 0, 0, 0, 0, 0, 41, 159, 159, 159, 118, 100, 105, 123, 159, 159, 159, 159, 159, 127, 105, 132, 114, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 100, 236, 204, 32, 41, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 45, 0, 0, 77, 159, 159, 159, 127, 123, 123, 118, 118, 82, 82, 82, 77, 45, 45, 41, 41, 41, 41, 41, 41, 41, 45, 45, 77, 82, 86, 123, 127, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 36, 159, 159, 159, 159, 123, 123, 159, 159, 159, 159, 159, 159, 159, 127, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 4, 100, 204, 68, 36, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 123, 86, 4, 0, 4, 41, 41, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 41, 127, 159, 159, 159, 77, 0, 0, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 4, 68, 68, 36, 123, 159, 159, 159, 159, 159, 159, 127, 118, 86, 82, 77, 41, 40, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 5, 37, 37, 37, 37, 38, 38, 38, 38, 37, 0, 0, 37, 38, 38, 38, 38, 37, 0, 0, 82, 159, 159, 159, 41, 0, 0, 0, 0, 0, 0, 0, 0, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 4, 0, 4, 118, 127, 123, 118, 82, 77, 41, 36, 4, 0, 0, 0, 0, 0, 0, 0, 1, 1, 5, 37, 38, 38, 38, 38, 42, 42, 43, 43, 43, 43, 43, 43, 43, 43, 42, 38, 38, 43, 43, 38, 0, 0, 38, 43, 43, 43, 43, 43, 37, 0, 4, 159, 159, 127, 4, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 0, 4, 41, 4, 0, 0, 0, 0, 0, 0, 0, 1, 5, 37, 38, 38, 38, 42, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 38, 0, 0, 0, 0, 38, 43, 38, 0, 0, 37, 43, 43, 43, 43, 43, 38, 0, 0, 123, 159, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 123, 82, 45, 36, 4, 0, 0, 0, 0, 0, 0, 1, 37, 38, 38, 38, 42, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 42, 38, 38, 38, 37, 5, 1, 0, 0, 0, 0, 0, 38, 43, 38, 0, 0, 37, 43, 43, 43, 43, 43, 38, 0, 0, 118, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 118, 82, 41, 4, 0, 0, 0, 0, 0, 0, 5, 37, 38, 38, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 42, 38, 38, 38, 37, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 43, 38, 0, 0, 5, 43, 43, 43, 43, 38, 5, 0, 0, 118, 159, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 118, 82, 41, 4, 0, 0, 0, 0, 0, 1, 37, 38, 38, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 42, 38, 38, 38, 37, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 43, 42, 0, 0, 1, 43, 43, 38, 5, 0, 0, 0, 0, 118, 123, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 86, 45, 4, 0, 0, 0, 0, 0, 1, 37, 38, 42, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 38, 37, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 37, 37, 37, 0, 0, 1, 37, 42, 43, 38, 0, 0, 0, 38, 1, 0, 0, 0, 0, 4, 82, 159, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 82, 45, 4, 0, 0, 0, 0, 1, 38, 38, 42, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 37, 37, 38, 38, 42, 43, 43, 43, 43, 43, 42, 43, 42, 38, 38, 1, 0, 0, 0, 0, 0, 0, 0, 4, 45, 86, 127, 159, 159, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 45, 4, 0, 0, 0, 0, 0, 0, 0, 38, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 37, 38, 38, 42, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 42, 38, 37, 1, 0, 0, 0, 0, 0, 0, 0, 4, 45, 86, 127, 159, 159, 159, 159, 123, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 159, 159, 159, 159, 159, 159, 159, 86, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 38, 5, 5, 37, 38, 38, 42, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 42, 38, 37, 1, 0, 0, 0, 0, 0, 0, 0, 0, 41, 77, 122, 127, 159, 159, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 118, 159, 159, 159, 159, 86, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 42, 38, 38, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 36, 45, 86, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 159, 159, 123, 45, 0, 36, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 42, 42, 38, 38, 37, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 45, 82, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 122, 123, 4, 36, 146, 182, 73, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 42, 42, 38, 38, 38, 37, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 41, 45, 86, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 82, 0, 36, 109, 146, 73, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 42, 42, 38, 38, 38, 38, 37, 37, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 45, 82, 118, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 86, 82, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 5, 5, 5, 5, 5, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 40, 45, 82, 118, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 123, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 159, 159, 123, 86, 82, 41, 40, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 36, 41, 45, 82, 86, 123, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 86, 45, 36, 4, 0, 0, 77, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 127, 123, 123, 118, 86, 82, 82, 45, 45, 41, 41, 41, 41, 41, 40, 36, 36, 4, 36, 36, 36, 36, 41, 41, 41, 41, 45, 45, 82, 82, 86, 118, 123, 123, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 122, 45, 4, 0, 0, 0, 4, 41, 41, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 123, 159, 159, 159, 159, 159, 159, 123, 41, 82, 159, 159, 159, 123, 41, 0, 0, 0, 4, 86, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 45, 41, 159, 159, 159, 159, 159, 159, 86, 0, 41, 159, 159, 77, 4, 0, 4, 45, 0, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 36, 4, 159, 159, 159, 159, 159, 159, 82, 0, 45, 159, 86, 0, 0, 77, 123, 123, 0, 41, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 86, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 82, 45, 41, 41, 41, 41, 86, 127, 159, 159, 159, 159, 159, 127, 4, 41, 159, 159, 159, 159, 159, 159, 45, 0, 82, 159, 123, 77, 118, 159, 159, 122, 0, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 0, 0, 0, 0, 0, 0, 0, 4, 86, 159, 159, 159, 159, 123, 0, 45, 159, 159, 159, 159, 159, 159, 41, 0, 118, 159, 159, 159, 159, 159, 159, 86, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 45, 0, 0, 118, 123, 118, 41, 0, 0, 118, 159, 159, 159, 86, 0, 82, 159, 159, 159, 159, 159, 159, 4, 4, 127, 159, 159, 159, 159, 159, 159, 82, 0, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 4, 127, 159, 159, 159, 82, 0, 4, 159, 159, 159, 82, 0, 118, 159, 159, 159, 159, 159, 123, 0, 4, 159, 159, 159, 159, 159, 159, 159, 45, 0, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 0, 41, 159, 159, 159, 159, 159, 45, 0, 123, 159, 159, 41, 0, 123, 159, 159, 159, 127, 86, 41, 0, 4, 127, 159, 159, 159, 159, 159, 159, 41, 4, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 118, 0, 45, 159, 159, 159, 159, 159, 118, 0, 86, 159, 159, 36, 4, 127, 159, 122, 77, 4, 0, 0, 0, 45, 159, 159, 159, 159, 159, 159, 159, 4, 4, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 82, 0, 82, 159, 159, 159, 159, 159, 82, 0, 118, 159, 123, 4, 4, 81, 40, 0, 0, 0, 41, 4, 0, 118, 159, 159, 159, 159, 159, 159, 123, 0, 41, 159, 159, 159, 159, 159, 159, 159, 159, 123, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 81, 0, 86, 159, 159, 159, 159, 127, 36, 4, 123, 159, 123, 0, 0, 0, 0, 4, 77, 123, 159, 41, 0, 123, 159, 159, 159, 159, 159, 159, 123, 0, 45, 159, 159, 159, 159, 159, 159, 159, 118, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 45, 0, 122, 159, 159, 159, 127, 45, 0, 45, 159, 159, 82, 0, 0, 41, 82, 127, 159, 159, 159, 40, 0, 123, 159, 159, 159, 159, 159, 159, 118, 0, 77, 159, 159, 159, 159, 159, 159, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 41, 0, 123, 159, 159, 123, 45, 0, 4, 123, 159, 159, 77, 0, 82, 159, 159, 159, 159, 159, 159, 4, 0, 123, 159, 159, 159, 159, 159, 159, 123, 4, 82, 159, 159, 159, 159, 123, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 77, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 36, 4, 127, 159, 86, 40, 0, 4, 118, 159, 159, 159, 41, 4, 123, 159, 159, 159, 159, 159, 159, 4, 0, 123, 159, 159, 159, 159, 159, 159, 159, 127, 159, 159, 159, 159, 86, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 86, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 4, 4, 82, 41, 0, 0, 41, 123, 159, 159, 159, 159, 36, 36, 159, 159, 159, 159, 159, 159, 159, 41, 41, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 118, 159, 159, 159, 159, 159, 159, 159, 123, 77, 0, 0, 0, 0, 4, 82, 127, 159, 159, 159, 159, 159, 4, 41, 159, 159, 159, 159, 159, 159, 159, 159, 127, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 45, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 118, 159, 159, 159, 159, 123, 4, 0, 0, 0, 41, 82, 127, 159, 159, 159, 159, 159, 159, 159, 86, 118, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 82, 123, 159, 159, 77, 77, 86, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 86, 41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 45, 82, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 127, 86, 77, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 77, 86, 123, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 123, 118, 82, 41, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 41, 45, 82, 86, 118, 118, 123, 123, 123, 122, 118, 86, 82, 77, 41, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    atrImagenes[10].addr = (RAM_G) + (10 * 512);
    atrImagenes[10].format = RGB332;
    atrImagenes[10].dim.x = 100;
    atrImagenes[10].dim.y = 100;

    for (i = 0; i < 10000; i += 4)
    {
        imagen = (imagenLogo[i + 3] << 24) | (imagenLogo[i + 2] << 16) | (imagenLogo[i + 1] << 8) | imagenLogo[i];
        Esc_Reg(atrImagenes[10].addr + i, imagen);
    }
}

void LeeImagenFormatoL1(struct caracteristicasImagen atrib_imagen)
{
    int dir, col, fila, orden;
    unsigned long pix_0_31_des, pix_32_63_des;
    unsigned long pix_0_31 = 0;
    unsigned long pix_32_63 = 0;
    unsigned long mask_ult_bit = 0x01;
    unsigned long mask_ult_byte = 0xff;

    for (dir = 0, fila = 0; dir < 512; dir += 8, fila++)
    {
        // Lectura de nueva fila (64 bits)
        pix_0_31_des = Lee_Reg(atrib_imagen.addr + dir);
        pix_32_63_des = Lee_Reg(atrib_imagen.addr + (dir + 4));
        // Reordeno
        for (orden = 3; orden >= 0; orden--)
        {
            pix_0_31 += (pix_0_31_des & mask_ult_byte) << (orden * 8);
            pix_0_31_des = pix_0_31_des >> 8;
            pix_32_63 += (pix_32_63_des & mask_ult_byte) << (orden * 8);
            pix_32_63_des = pix_32_63_des >> 8;
        }
        // Almaceno fila en matriz de comparacion
        for (col = 63; col >= 32; col--)
        {
            matriz_imagen_original[fila][col] = (pix_32_63 & mask_ult_bit);
            pix_32_63 = pix_32_63 >> 1;
        }
        for (col = 31; col >= 0; col--)
        {
            matriz_imagen_original[fila][col] = (pix_0_31 & mask_ult_bit);
            pix_0_31 = pix_0_31 >> 1;
        }
    }
}

void decodificaFormatoL1()
{
    // Expresar matriz original de digito concreto usado como vector codificado en formato L1 para almacenar en RAM de pantalla y posterior representacion

    int i, j;
    for (i = 0; i < 64; i++)
    {
        for (j = 0; j < 64; j++)
        {
            vector_codificado[(j + i * 64) / 8] = matriz_imagen_original[i][j] + vector_codificado[(j + i * 64) / 8];
            if ((j + i * 64) % 8 != 7)
                vector_codificado[(j + i * 64) / 8] = vector_codificado[(j + i * 64) / 8] << 1;
        }
    }
}

void codificaMatrizFormatoRGB332()
{
    // Expresar matriz dinamica de imagen digitos pintados como vector en formato RGB332 para para almacenar en RAM de pantalla y posterior representacion
    int i, j;
    for (i = 0; i < 64; i++)
    {
        for (j = 0; j < 64; j++)
        {
            vector_codificado[j + i * 64] = (matriz_imagen_din_RGB[i][j][0] << 5) + (matriz_imagen_din_RGB[i][j][1] << 2) + (matriz_imagen_din_RGB[i][j][2]);
        }
    }
}

void traduceMatriz2RGB()
{
    // Paso de matriz original de 0 y 1 del digito concreto usado en cada instante a formato RGB332

    int i,j;
    for (i = 0; i < 64; i++)
    {
        for (j = 0; j < 64; j++)
        {
            if (matriz_imagen_original[i][j] == 1)
            {
                matriz_imagen_din_RGB[i][j][0] = 7;
                matriz_imagen_din_RGB[i][j][1] = 7;
                matriz_imagen_din_RGB[i][j][2] = 3;
            }
            else
            {
                matriz_imagen_din_RGB[i][j][0] = 0;
                matriz_imagen_din_RGB[i][j][1] = 0;
                matriz_imagen_din_RGB[i][j][2] = 0;
            }
        }
    }
}

void guardaImagenDin()
{
    // Escritura en RAM de pantalla de la matriz dinamica que contiene datos de imagen de digito pintados

    unsigned long buffer;
    int i;
    for (i = 0; i < 4096; i += 4)
    {
        buffer = (vector_codificado[i + 3] << 24) + (vector_codificado[i + 2] << 16) + (vector_codificado[i + 1] << 8) + vector_codificado[i];
        Esc_Reg(RAM_G + 10 * 512 + 10000 + i, buffer);
    }
}

void reseteaMatrizTrazoPintado()
{
    // Reinicio de la matriz que contiene informacion sobre la cantidad de pixeles rojos(fallos), verdes(aciertos), blancos(zona digito sin pintar) y negros(zona fondo sin pintar)

    int i, j;
    for (i = 0; i < 64; i++)
    {
        for (j = 0; j < 64; j++)
        {
            matrizDibujoRealizado[i][j] = 2*matriz_imagen_original[i][j];
        }
    }
}

void resetImagenDin(){

    // Llamada a todas las funciones necesarias para reinicio correcto digito nuevo para pintar y ser evaluado

    traduceMatriz2RGB();
    codificaMatrizFormatoRGB332();
    guardaImagenDin();
    reseteaMatrizTrazoPintado();
}

float calculaScore(){

    // Calculo de la puntuacion obtenida tras terminar y confirmar ejercicio de pintado de un digito concreto

    float scoreRes=0;
    int cont_rojo = 0;
    int cont_verde = 0;
    int cont_blanco = 0;
    int i,j;
    float relacionPintadoDigito;
    int umbralValido;

    for (i = 0; i < 64; i++)
    {
        for (j = 0; j < 64; j++)
        {
            if(matrizDibujoRealizado[i][j] == 1)
                cont_verde ++;
            if(matrizDibujoRealizado[i][j] == -1)
                cont_rojo ++;
            if(matrizDibujoRealizado[i][j] == 2)
                cont_blanco ++;
        }
    }

    // MENSAJES
    relacionPintadoDigito = (float)(cont_verde+cont_rojo)/(float)(cont_verde+cont_blanco)*100;
    umbralValido = 20*(0.3*radioTrazo)+5;
    if(relacionPintadoDigito<umbralValido){
        sprintf(string, "TEST ERROR");
    }

    else{
        if ((cont_verde+cont_rojo) != 0)
            scoreRes=((float)cont_verde/(float)(cont_verde+abs(cont_rojo)))*10;

        if(scoreRes>=10){
            sprintf(string, "PERFECT !!");
        }
        else if(scoreRes>=9.5){
            sprintf(string, "AMAZING");
        }
        else if(scoreRes>=8.5){
            sprintf(string, "WELL DONE");
        }
        else if(scoreRes>=7){
            sprintf(string, "NICE");
        }
        else if(scoreRes>=5){
            sprintf(string, "MORE OR LESS");
        }
        else
            sprintf(string, "TRY AGAIN");
    }

    return scoreRes;
}

void ComSlider(int x, int y, int w, int h, int options, unsigned int *val, int range){
    //x,y: posicion de esquina lateral izquierda del widget
    //w: anchura de la barra en pixeles
    //h: altura de la barra (grosor) en pixeles
    //options: efecto 3D o plano
    //*val: valor a rellenar en la barra (se pasa como puntero para pasar parametro por referencia y poder modificar su valor
    //range: rango que puede tomar el valor a representar

#ifdef VM800B35
#ifdef ESCALADO
    x=(x*2)/3;
    y=(y*15)/17;
    w=(w*2)/3;
    h=(h*15)/17;
#endif
#endif
    int markerx_c = (w*(*val)/(float)range)+x; //posicion estimada del indicador del slider
    if( POSX > markerx_c-h && POSX < markerx_c+h  && POSY > y-h && POSY < y+h ){ //Comprobar si se esta pulsando el indicador
        if(POSX<x) //saturar por la izquierda
            POSX=x;
        else if(POSX>x+w) //saturar por la derecha
            POSX=x+w;
        *val = (int)((float)range/(float)w*(POSX-x)); //actualizar valor del valor asociado al slider
    }
    EscribeRam32(CMD_SLIDER);
    EscribeRam16(x);
    EscribeRam16(y);
    EscribeRam16(w);
    EscribeRam16(h);
    EscribeRam16(options);
    EscribeRam16(*val);
    EscribeRam16(range);
    EscribeRam16(0);
}

void progress_list(){

    // Lista con las mejores marcas obtenidas para cada digito, progreso alcanzado

    int dig;
    char string[30];

    ComColor(255, 255, 255);
    ComTXT(160, 20, 30, OPT_CENTER, "BEST SCORES");
    ComColor(0, 0, 0);
    ComTXT(160, 50, 24, OPT_CENTER, "--------- Current Progress ---------");

    for(dig=0;dig<10;dig++){
        sprintf(string, "DIGITO %d - - - - > %.2f", dig, ((float)best_scores[dig]/(float)100));
        if(dig%2) ComColor(0, 0, 0);
        else ComColor(255, 255, 255);
        ComTXT(120, 75+dig*16+1, 18, OPT_CENTER, string);
    }

    ComTXT(160, 230, 24, OPT_CENTER, "-------------------------------------------------------");
}

void ComProgressBar(int x, int y, int w, int h, int options, int val, int range){
    //x,y: posicion de esquina lateral izquierda del widget
    //w: anchura de la barra en p�xeles
    //h: altura de la barra (grosor) en p�xeles
    //options: efecto 3D o plano
    //val: valor a rellenar en la barra
    //range: rango que puede tomar el valor a representar

#ifdef VM800B35
#ifdef ESCALADO
    x=(x*2)/3;
    y=(y*15)/17;
    w=(w*2)/3;
    h=(h*15)/17;
#endif
#endif
    EscribeRam32(CMD_PROGRESS);
    EscribeRam16(x);
    EscribeRam16(y);
    EscribeRam16(w);
    EscribeRam16(h);
    EscribeRam16(options);
    EscribeRam16(val);
    EscribeRam16(range);
    EscribeRam16(0);
}

int teclado(void)
{
    // Manejo de teclado para introducir y cambiar clave del sistema

    int teclaPulsada=NO_PULSADO;
    ComKeys(10,38,300,35,28,0,"0123456789");
    ComKeys(10,77,300,35,28,0,"QWERTYUIOP");
    ComKeys(10,116,300,35,28,0,"ASDFGHIJKL");
    ComKeys(10, 155, 300, 35, 28, 0, "ZXCVBNM");
    teclaPulsada=Keys(10,38,300,35,28,"0123456789");
    if(teclaPulsada==NO_PULSADO)
        teclaPulsada=Keys(10,77,300,35,28,"QWERTYUIOP");
    if(teclaPulsada==NO_PULSADO)
        teclaPulsada=Keys(10,116,300,35,28,"ASDFGHIJKL");
    if(teclaPulsada==NO_PULSADO)
        teclaPulsada=Keys(10, 155, 300, 35, 28, "ZXCVBNM");
    if(Boton(100, 194, 157, 40, 28, ""))
        teclaPulsada=' ';
    if(Boton(260, 194, 50, 40, 28, "DEL"))
        teclaPulsada=DEL;
    if(Boton(10,194,87,40,28,"ENTER"))
        teclaPulsada=ENTER;
    return teclaPulsada;
}

void ComKeys(int16_t x, int16_t y, int16_t w, int16_t h, int16_t font, uint16_t options, const char *s){

    // Representacion de teclas de teclado

#ifdef VM800B35
#ifdef ESCALADO
    x=(x*2)/3;
    y=(y*15)/17;
    w=(w*2)/3;
    h=(h*15)/17;
#endif
#endif

    EscribeRam32(CMD_KEYS);
    EscribeRam16(x);
    EscribeRam16(y);
    EscribeRam16(w);
    EscribeRam16(h);
    EscribeRam16(font);
    EscribeRam16(options);
    while(*s) EscribeRam8(*s++);
    EscribeRam8(0);
    PadFIFO();

}

int Keys(int16_t x, int16_t y, int16_t w, int16_t h, int16_t font, const char *s)
{

    // Deteccion tecla pulsada de teclado

#ifdef VM800B35
#ifdef ESCALADO
    x=(x*2)/3;
    y=(y*15)/17;
    w=(w*2)/3;
    h=(h*15)/17;
#endif
#endif
    int i, longitud=0, resul = 0;
    char cadena[50];
    while(*(s+longitud))
    {
        cadena[longitud]=*(s+longitud);
        longitud++;
    }
    cadena[longitud]='\0';


    Lee_pantalla();
    for(i=0;i<longitud;i++)
    {
        if(POSX>x+i*(w/longitud) && POSX<x+i*(w/longitud)+w/longitud && POSY>y && POSY<(y+h)){
            ComKeys(x,y,w,h,font,cadena[i],cadena);
            resul=*(s+i);
            break;
        }
    }
    return resul;
}
