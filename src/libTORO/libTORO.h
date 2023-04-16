#ifndef TORO_ESP32_BASE
#define TORO_ESP32_BASE

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
#define CREAR_HILO(func,nombre,tam,priv,nucleo) xTaskCreatePinnedToCore(func,nombre,tam,NULL,priv,NULL,nucleo);
#define TORO_inline __attribute__((always_inline)) inline

namespace toro_stnd{
  struct TORO_parlante{
    uint8_t pin,canal;
    uint16_t calidad;
    float freq;
  };
  struct TORO_configs{
    camera_config_t camara;
    sensor_t sensor_camara;
    TORO_parlante parlante;
    uint32_t t_para_conect;
    const char* ssid_wifi,*con_wifi,*ssid_ap,*con_ap;
  };
  class TORO{
    private:
      //Variable bandera que representa el estado de TORO
      //1ºb:Estado general: 1 para fallo, 0 para estado correcto
      //2ºb:Estado WIFI: 1 para desconectado, 0 para conectado
      //3ºb:Tipo de conexion WIFI: 1 para punto de acceso, 0 para red WIFI
      //4ºb:Estado cliente: 1 para ninguno, 0 para conectado
      //5ºb:Estado de la tarjeta micro Sd: 0 para fallo, 0 para correcto
      //6ºb:Estado de la camera: 1 para fallo, 0 para correcto
      //7ºb:Estado del Arduino: 1 fallo, 0 para correcto
      uint8_t band_est;
      //Velocidad motores
      uint16_t vel_motores;
      //Wifi
      WiFiClass wifi;
      //Web Server
      AsyncWebServer *servidor;
      //Web Sockets
      AsyncWebSocket *ws_camara;
      AsyncWebSocket *ws_motores;
      AsyncWebSocket *ws_archivos;
      AsyncWebSocket *ws_sensores;
      //Para los threads
      TORO_inline void crearhilo(TaskFunction_t,const char*const,const uint32_t,UBaseType_t,const BaseType_t);
      //Controlar motores
      void controlmotor();
      //Configurar parlante
      void config_parlante();
      //Reproducir musica
      void reproducir();
      //Conectar Wifi
      void conect_wifi(uint32_t,const char*,const char*,const char*,const char*);
      //
      void crear_Web(AsyncWebServer**,AsyncWebSocket**,AsyncWebSocket**,AsyncWebSocket**,AsyncWebSocket**);
      //Funciones para los sockets
      static void func_ws_camara(AsyncWebSocket* socket,AsyncWebSocketClient* cliente,AwsEventType tipo,void* args,uint8_t* datos,size_t tam){

      }
      static void func_ws_motores(AsyncWebSocket* socket,AsyncWebSocketClient* cliente,AwsEventType tipo,void* args,uint8_t* datos,size_t tam){

      }
      static void func_ws_archivos(AsyncWebSocket* socket,AsyncWebSocketClient* cliente,AwsEventType tipo,void* args,uint8_t* datos,size_t tam){

      }
      static void func_ws_sensores(AsyncWebSocket* socket,AsyncWebSocketClient* cliente,AwsEventType tipo,void* args,uint8_t* datos,size_t tam){

      }
      //Funciones para el Web Server
      static void enviarHTTP(AsyncWebServerRequest* requiso){
        
      }
      static void enviarError(AsyncWebServerRequest* requiso){

      }
    public:
      TORO(TORO_configs*);
      ~TORO();
      void Iniciar();
  };
} 
#endif