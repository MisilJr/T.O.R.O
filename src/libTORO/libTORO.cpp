#ifndef LIB_TORO
#define LIB_TORO

#include "libTORO.h"

namespace toro_stnd{
  TORO::TORO(TORO_configs *config){
    band_est=0;
    band_est|=esp_camera_init(&(config->camara))*4;
    if(!SD_MMC.begin()){
      band_est|=8;
    }
    conect_wifi(config->t_para_conect,config->ssid_wifi,config->con_wifi,
                config->ssid_ap,config->con_ap);
    crear_Web(&servidor,&ws_camara,&ws_motores,&ws_archivos,&ws_sensores);
    ws_camara->onEvent(func_ws_camara);
    ws_motores->onEvent(func_ws_motores);
    ws_archivos->onEvent(func_ws_archivos);
    ws_sensores->onEvent(func_ws_sensores);
    servidor->addHandler(ws_camara);
    servidor->addHandler(ws_motores);
    servidor->addHandler(ws_archivos);
    servidor->addHandler(ws_sensores);
    servidor->on("/",HTTP_GET,enviarHTTP);
    servidor->onNotFound(enviarError);
    servidor->begin();
  }
  void TORO::Iniciar(){
    
  }
  TORO::~TORO(){
    SD_MMC.end();
    servidor->end();
  }
  TORO_inline void TORO::crearhilo(TaskFunction_t func,const char*const nombre,
                             const uint32_t tam,UBaseType_t priv,const BaseType_t nucleo){
    CREAR_HILO(func,nombre,tam,priv,nucleo);
  }
  void TORO::conect_wifi(uint32_t t_para_conect,const char* ssid_wifi,
                         const char* con_wifi,const char* ssid_ap,const char* con_ap){
    wifi.begin(ssid_wifi,con_wifi);
    const uint32_t tiempo=millis();
    while(wifi.status()!=WL_CONNECTED and millis()-tiempo>t_para_conect){}
    if(wifi.status()!=WL_CONNECTED){//Si no se pudo conectar a la red, cambia a modo Access Point
      if(wifi.softAP(ssid_ap,con_ap)){
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
  void TORO::crear_Web(AsyncWebServer** serv,AsyncWebSocket** cam,
                       AsyncWebSocket** mot,AsyncWebSocket** arc,AsyncWebSocket** sen){
      AsyncWebServer _servidor(80);
      AsyncWebSocket _ws_camara("/Camara");
      AsyncWebSocket _ws_motores("/Motores");
      AsyncWebSocket _ws_archivos("/Archivos");
      AsyncWebSocket _ws_sensores("Sensores");
      *serv=&_servidor;
      *cam=&_ws_camara;
      *mot=&_ws_motores;
      *arc=&_ws_archivos;
      *sen=&_ws_sensores;
  }

}

#endif