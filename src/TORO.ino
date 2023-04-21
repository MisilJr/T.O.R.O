#include<stddef.h>
#include<stdint.h>
#include<esp_camera.h>
#include<Arduino.h>
#include<WiFi.h>
#include<AsyncTCP.h>
#include<ESPAsyncWebServer.h>
#include<FS.h>
#include<SD_MMC.h>
#include<sd_defines.h>

//Pines de la camara
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//Algunas macros
#define CrearHilo(func,nombre,tam,priv,nucleo) xTaskCreatePinnedToCore(func,nombre,tam,NULL,priv,NULL,nucleo)
#define TORO_inline __attribute__((always_inline)) inline

struct TORO_motor{
  uint8_t pin,canal;
};
struct TORO_parlante{
  uint8_t pin,canal;
  uint16_t calidad;
  float freq;
};
enum TORO_proceso{
  Fin_Nulo,//Nada, solo para decirle a IniciarTORO() que no hay mas procesos
  arduino,//Datos desde y para el Arduino 
  archivo,//Tx y Rx de archivos
  camara,//Obvio
  motores,//Obvio
  registro,//Info. varia hacia el cliente
  reproduccion,//Sonido y musica
  wifi//Conexion Wifi, Access Point y verificacion
};
struct TORO_thread{
  TORO_proceso proceso;
  uint32_t memoria;
  BaseType_t privilegio,nucleo;
};
struct TORO_configs{
  camera_config_t camara;
  sensor_t sensor_camara;
  TORO_motor motor1,motor2;
  TORO_parlante parlante;
  uint32_t t_para_conect;//Segundos
  const char* ssid_wifi,*con_wifi,*ssid_ap,*con_ap;
  TORO_thread* threads;
};

//Variable bandera que representa el estado de TORO
//1ºb:Estado general: 1 para fallo, 0 para estado correcto
//2ºb:Estado WIFI: 1 para desconectado, 0 para conectado
//3ºb:Tipo de conexion WIFI: 1 para punto de acceso, 0 para red WIFI
//4ºb:Estado cliente: 1 para ninguno, 0 para conectado
//5ºb:Estado de la tarjeta micro Sd: 0 para fallo, 0 para correcto
//6ºb:Estado de la camera: 1 para fallo, 0 para correcto
//7ºb:Estado del Arduino: 1 fallo, 0 para correcto
//8ºb:Estado de la memoria RAM: 1 para fallo, 0 para correcto
uint8_t band_est;

//Para los motores
uint16_t vel_motores=0;

//Para la funcion reproducir()
uint8_t canalParlante;

//Para la funcion reproducirArchivo()
bool detenerReproduccion=false;

//Para manejar la SD
fs::FSImplPtr tmemoriaptr;
fs::FS tmemoria(tmemoriaptr);

//Wifi
WiFiClass TORO_wifi;

//El servidor y sockets
AsyncWebServer servidor(80);
AsyncWebSocket ws_camara("/Camara");
AsyncWebSocket ws_motores("/Motores");
AsyncWebSocket ws_archivos("/Archivos");
AsyncWebSocket ws_sensores("/Sensores");
AsyncWebSocket ws_reproduccion("/Sonido");
AsyncWebSocket consl_regist("/Registro");

void controlmotor(uint8_t);
//Configurar parlante
void config_parlante(TORO_parlante*);
//Reproducir musica
void reproducirArchivo(const char*,uint32_t);
void reproducir(uint16_t);
//Conectar Wifi
void conect_wifi(uint32_t,const char*,const char*,const char*,const char*);

//Funciones para el Server Web
void enviarHTTP(AsyncWebServerRequest*);
TORO_inline void enviarError(AsyncWebServerRequest*);

//Funciones de recepcion para los sockets
void func_ws_camara(AsyncWebSocket*,AsyncWebSocketClient*,AwsEventType,void*,uint8_t*,size_t);//Al ser una funcion de recepcion, casi no hace nada
void func_ws_motores(AsyncWebSocket*,AsyncWebSocketClient*,AwsEventType,void*,uint8_t*,size_t);//Regula velocidad
void func_ws_archivos(AsyncWebSocket*,AsyncWebSocketClient*,AwsEventType,void*,uint8_t*,size_t);
void func_ws_sensores(AsyncWebSocket*,AsyncWebSocketClient*,AwsEventType,void*,uint8_t*,size_t);
void func_ws_reproduccion(AsyncWebSocket*,AsyncWebSocketClient*,AwsEventType,void*,uint8_t*,size_t);
void func_consl_regist(AsyncWebSocket*,AsyncWebSocketClient*,AwsEventType,void*,uint8_t*,size_t);//El registro no recibe datos, esta func. no hace nada

//Funciones para el envio y proceso de datos atraves de los Web Sockets. Todas tienen un arg. del tipo void* ya que asi lo pide la funcion de crear threads
void func_camara(void*);
void func_motores(void*);
void func_archivos(void*);
void func_sensores(void*);
void func_reproduccion(void*);
void func_registro(const char*);//No es ejecutada en paralelo directamente pero si por las demas funciones

void IniciarTORO(TORO_configs*);

void setup(){
  TORO_configs config;
  //Camara
  config.camara.ledc_channel = LEDC_CHANNEL_0;
  config.camara.ledc_timer = LEDC_TIMER_0;
  config.camara.pin_d0 = Y2_GPIO_NUM;
  config.camara.pin_d1 = Y3_GPIO_NUM;
  config.camara.pin_d2 = Y4_GPIO_NUM;
  config.camara.pin_d3 = Y5_GPIO_NUM;
  config.camara.pin_d4 = Y6_GPIO_NUM;
  config.camara.pin_d5 = Y7_GPIO_NUM;
  config.camara.pin_d6 = Y8_GPIO_NUM;
  config.camara.pin_d7 = Y9_GPIO_NUM;
  config.camara.pin_xclk = XCLK_GPIO_NUM;
  config.camara.pin_pclk = PCLK_GPIO_NUM;
  config.camara.pin_vsync = VSYNC_GPIO_NUM;
  config.camara.pin_href = HREF_GPIO_NUM;
  config.camara.pin_sscb_sda = SIOD_GPIO_NUM;
  config.camara.pin_sscb_scl = SIOC_GPIO_NUM;
  config.camara.pin_pwdn = PWDN_GPIO_NUM;
  config.camara.pin_reset = RESET_GPIO_NUM;
  config.camara.xclk_freq_hz = 20000000;
  config.camara.pixel_format = PIXFORMAT_JPEG;
  config.camara.frame_size = FRAMESIZE_UXGA;
  config.camara.jpeg_quality = 12;
  config.camara.fb_count = 2;

  //Motores
  config.motor1.pin = 3;
  config.motor1.canal = 3;
  config.motor2.pin = 4;
  config.motor2.canal = 4;

  //Parlante
  config.parlante.pin = 5;
  config.parlante.calidad = 9;//Calidad 9 bits
  config.parlante.canal = 1;
  config.parlante.freq = 44100;

  //Wifi
  config.t_para_conect = 15;
  config.ssid_wifi = "SSID";
  config.con_ap = "contraseña";
  config.ssid_ap = "TORO";
  config.con_ap = "contraseña";

  //Threads
  TORO_thread threads[6];
  threads[0].proceso = TORO_proceso::wifi;
  threads[0].memoria = 1024;
  threads[0].privilegio = 1;
  threads[0].nucleo = 0;
  threads[1].proceso = TORO_proceso::arduino;
  threads[1].memoria = 256;
  threads[1].privilegio = 2;
  threads[1].nucleo = 0;
  threads[2].proceso = TORO_proceso::reproduccion;
  threads[2].memoria = 10000;
  threads[2].privilegio = 2;
  threads[2].nucleo = 0;
  threads[3].proceso = TORO_proceso::motores;
  threads[3].memoria = 256;
  threads[3].privilegio = 2;
  threads[3].nucleo = 1;
  threads[4].proceso = TORO_proceso::camara;
  threads[4].memoria = 100000;
  threads[4].privilegio = 3;
  threads[4].nucleo = 1;
  threads[5].proceso = TORO_proceso::Fin_Nulo;
  threads[5].memoria = 0;
  threads[5].privilegio = 0;
  threads[5].nucleo = 0;

  config.threads=threads;

  IniciarTORO(&config);
}

void loop(){
  
}

void config_parlante(TORO_parlante* config){
  ledcSetup(config->canal,config->freq,config->calidad);
  ledcAttachPin(config->pin,config->canal);
  canalParlante=config->canal;
}

void reproducirArchivo(const char* nombre_arch,uint32_t freq){
  if(band_est&8 or detenerReproduccion){
    return;
  }
  uint16_t* dato=nullptr;
  uint32_t tmuestras=(uint32_t)((float)1000000/(float)freq);  
  fs::File archivo=tmemoria.open(nombre_arch,FILE_READ);
  if(!archivo){
    archivo.close();
    return;
  }
  size_t tam_arch=archivo.size();
  archivo.readBytes((char*)dato,tam_arch);
  const uint32_t tiempo=micros();
  for(uint64_t actual=0;actual<floor(tam_arch/2) and micros()-tiempo>tmuestras //Dividido entre dos ya que vamos a la funcion lee 2 bytes a la vez
                        and !detenerReproduccion;actual++){
    reproducir(*dato);
    dato++;
  }
  archivo.close();
}

TORO_inline void reproducir(uint16_t valor){
  ledcWrite(canalParlante,valor>>7);//Valor dividido entre 128 debido a que su valor es de 16 bits y la resolucion es de 9 bits
}

void conect_wifi(uint32_t t_para_conect,const char* ssid_wifi,
                 const char* con_wifi,const char* ssid_ap,const char* con_ap){
  if(!(band_est&64))return;
  TORO_wifi.begin(ssid_wifi,con_wifi);
  const uint32_t tiempo=millis();
  while(TORO_wifi.status()!=WL_CONNECTED and millis()-tiempo>t_para_conect*1000){}
  if(TORO_wifi.status()!=WL_CONNECTED){//Si no se pudo conectar a la red, cambia a modo Access Point
    if(TORO_wifi.softAP(ssid_ap,con_ap)){
      band_est|=64;
    }else{
      band_est|=32;//Cambiamos a AP
      band_est&=191;//255-64 o B10111111, estado conectado
    }
  }else{
    band_est&=223;//255-32 o B11011111, cambiamos a WIFI
    band_est&=191;//255-64 o B10111111, estado conectado
  }
}

void enviarHTTP(AsyncWebServerRequest* requiso){
  if(band_est&8){
    enviarError(requiso);
    return;
  }
  char* texto=nullptr;
  fs::File archivo=tmemoria.open("TOROpaginahtml.txt",FILE_READ);
  if(!archivo){
    enviarError(requiso);
    archivo.close();
    return;
  }
  archivo.readBytes(texto,archivo.size());
  if(texto==nullptr){
    enviarError(requiso);
    archivo.close();
    return;
  }
  requiso->send_P(200,"text/html",texto);
  archivo.close();
}

TORO_inline void enviarError(AsyncWebServerRequest* requiso){
  requiso->send(404,"text/plain","Error en TORO.");
}

void func_ws_camara(AsyncWebSocket* socket,AsyncWebSocketClient* cliente,AwsEventType tipo,void* args,uint8_t* datos,size_t tam){

}

void func_ws_motores(AsyncWebSocket* socket,AsyncWebSocketClient* cliente,AwsEventType tipo,void* args,uint8_t* datos,size_t tam){

}

void func_ws_archivos(AsyncWebSocket* socket,AsyncWebSocketClient* cliente,AwsEventType tipo,void* args,uint8_t* datos,size_t tam){

}

void func_ws_sensores(AsyncWebSocket* socket,AsyncWebSocketClient* cliente,AwsEventType tipo,void* args,uint8_t* datos,size_t tam){

}

void func_consl_regist(AsyncWebSocket* socket,AsyncWebSocketClient* cliente,AwsEventType tipo,void* args,uint8_t* datos,size_t tam){

}

void func_camara(void* arg_nulo){
  if(band_est&84)return;
  ws_camara.cleanupClients();
}

void func_motores(void* arg_nulo){
  if(band_est&80)return;
  ws_motores.cleanupClients();
}

void func_archivos(void* arg_nulo){
  if(band_est&88)return;
  ws_archivos.cleanupClients();
}

void func_sensores(void* arg_nulo){
  if(band_est&82)return;
  ws_sensores.cleanupClients();
}

void func_registro(const char* mensaje){
  if(band_est&80)return;
  
}

void IniciarTORO(TORO_configs* config){
  band_est=64;//Empesamos sin conex. Wifi
  if(esp_camera_init(&(config->camara))!=ESP_OK){
    band_est|=4;
  }
  if(!SD_MMC.begin()){
    band_est|=8;
  }else{
    tmemoria=SD_MMC;
  }
  ledcSetup(config->motor1.canal,610,16);//610Hz es la maxima frequencia a la que puede funcionar la resolucion de 16 bits
  ledcAttachPin(config->motor1.pin,config->motor1.canal);
  ledcSetup(config->motor2.canal,610,16);
  ledcAttachPin(config->motor2.pin,config->motor2.canal);
  Serial.begin(115200);
  conect_wifi(config->t_para_conect,config->ssid_wifi,config->con_wifi,config->ssid_ap,config->con_ap);
  ws_camara.onEvent(func_ws_camara);
  ws_motores.onEvent(func_ws_motores);
  ws_archivos.onEvent(func_ws_archivos);
  ws_sensores.onEvent(func_ws_sensores);
  consl_regist.onEvent(func_consl_regist);
  servidor.addHandler(&ws_camara);
  servidor.addHandler(&ws_motores);
  servidor.addHandler(&ws_archivos);
  servidor.addHandler(&ws_sensores);
  servidor.addHandler(&consl_regist);
  servidor.on("/",HTTP_GET,enviarHTTP);
  servidor.onNotFound(enviarError);
  servidor.begin();
  TORO_thread *threads=config->threads;
  while(threads->proceso!=TORO_proceso::Fin_Nulo){
    switch(threads->proceso){
      case Fin_Nulo:break;
      case arduino:break;
      case camara:CrearHilo(func_camara,"camara",threads->memoria,threads->privilegio,threads->nucleo);break;
      case motores:CrearHilo(func_motores,"motores",threads->memoria,threads->privilegio,threads->nucleo);break;
      case archivo:CrearHilo(func_archivos,"registro",threads->memoria,threads->privilegio,threads->nucleo);break;
      default:break;
    }
    threads++;
  }
}
