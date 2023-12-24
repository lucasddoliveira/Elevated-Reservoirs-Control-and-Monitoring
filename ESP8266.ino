#include <LiquidCrystal_I2C.h>
#include "UbidotsESPMQTT.h"

//#define WIFINAME "LAC"  
//#define WIFIPASS "2021UlFaPcB" 
//#define WIFINAME "JABRE2_2G"  
//#define WIFIPASS "casaamarela"  
#define WIFINAME "TP-LINK_9291E8"
#define WIFIPASS ""
//#define WIFINAME "Lucas Dantas"  
//#define WIFIPASS "lucas123"

#define hTotal 5.2
#define hTubo 3.5
#define hCaixa 1.7

#define TOKEN "YOUR_UBIDOTS_TOKEN"   
#define intervalo 45000//Intervalo de tempo para publicação no Ubidots
#define intervaloLCD 15000

Ubidots client(TOKEN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

//------------------------------------------------------------------------------------------------------------/
int alert = 0;
int estadoBomba = 0;
int mudancaEstadoBomba = estadoBomba;

int estadoControlador = 0;
int bombaManual = 0;

int type = 2;

int posi[20],pos,aux;
float ten, erro, nivel, li, ls;

float setpoint = 0.85;
float hi=0.15;                       
float hs=0.15;

float hMedido;
float capacidade;

int tOld = millis();
int tOld1 = millis();
int tOld2 = millis();
//------------------------------------------------------------------------------------------------------------/

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  
  String strMSG = String((char*)payload);
  String strTOP = String(topic);
  
  if (strTOP=="/v1.6/devices/supervisorio/setpoint/lv"){        //Tópico para modificar setpoint
    setpoint = strMSG.toFloat()/100.0;
  } else if(strTOP=="/v1.6/devices/supervisorio/hs/lv") { //Tópico para modificar limite da histerese
    hs = strMSG.toFloat()/100.0;
  } else if(strTOP=="/v1.6/devices/supervisorio/hi/lv") { //Tópico para modificar limite interior da histerese
    hi = strMSG.toFloat()/100.0;
  } else if(strTOP=="/v1.6/devices/supervisorio/tipo/lv") {         // Tópico para modificar tipo de ligação
    type = strMSG.toInt();
  } else if(strTOP=="/v1.6/devices/supervisorio/bombamanual/lv") {         // Tópico para modificar tipo de ligação
    bombaManual = strMSG.toInt();
  } else if(strTOP=="/v1.6/devices/supervisorio/estadocontrolador/lv") {         // Tópico para modificar tipo de ligação
    estadoControlador = strMSG.toInt();
  } 
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    
  }
  
  Serial.println();
}

//------------------------------------------------------------------------------------------------------------//

void setup() {
  Serial.begin(115200);

  pinMode(D0, OUTPUT);
  pinMode(A0, INPUT);
  
  client.ubidotsSetBroker("industrial.api.ubidots.com");
  client.setDebug(true); 
  client.wifiConnection(WIFINAME, WIFIPASS);
  client.begin(callback);

  client.ubidotsSubscribe("supervisorio", "hi");
  client.ubidotsSubscribe("supervisorio", "hs");
  client.ubidotsSubscribe("supervisorio", "setpoint");
  client.ubidotsSubscribe("supervisorio", "tipo");
  client.ubidotsSubscribe("supervisorio", "estadoControlador");
  client.ubidotsSubscribe("supervisorio", "bombaManual");

  lcd.init();                     
  //lcd.backlight();
  lcd.clear();

}

//------------------------------------------------------------------------------------------------------------//

void lerADC(){
  for (int k=0; k<20; k++) {
   posi[k] = analogRead(A0);
   delay(50); 
  }
  for (int j=0; j<20; j++) {    //Ajustar vetor para colocar os mumeros em ordem crescente
   for (int l=0; l<20; l++) {
      if (posi[j]<posi[l]) {
        aux = posi[l];
        posi[l] = posi[j];
        posi[j] = aux;
      }
   }
  }
  pos = (posi[9]+posi[10])/2;
}

//------------------------------------------------------------------------------------------------------------//

void controlador () {
  if(estadoControlador == 1){
      switch (type) {
        case 1:
        //TIPO ON-OFF
          if (erro>0) {
            digitalWrite(D0, HIGH);
            estadoBomba = 1;
          } else {
            digitalWrite(D0, LOW);
            estadoBomba = 0;
          }
          break;
          
        case 2:
        //TIPO HISTERESE
          li=(setpoint-hi);                      //Limite inferior
          ls=(setpoint+hs);                      //Limite superior
          
          if (nivel<li) {
            digitalWrite(D0, HIGH);
            estadoBomba = 1;
          } 
          else if (nivel>ls){
            digitalWrite(D0, LOW);
            estadoBomba = 0;
          }
          break;
       
        default:
          break;
      }
      
  } else if(estadoControlador == 0){
      if(bombaManual==1){
          Serial.println("BOMBA ATIVADA");
          digitalWrite(D0, HIGH);
          estadoBomba = 1;
      } else {
          Serial.println("BOMBA DESATIVADA");
          digitalWrite(D0, LOW);  
          estadoBomba = 0;
      }  
  }
}

//------------------------------------------------------------------------------------------------------------//

void calcularNivel(){

  //tensao 3.14V --> 5.2mca
  //tensao 0V --> 0mca
  //hTotal 5.2m
  //hTubo 3.5m
  //hCaixa 1.7m
  
  ten = (0.0030791894922*pos)-0.011103016; 
 
  hMedido = (ten/3.14)*hTotal;
  nivel = hMedido-hTubo;

  capacidade = (nivel/hCaixa)*100;
  
  erro = setpoint-nivel;
}

//------------------------------------------------------------------------------------------------------------//

void displayLCD(){

  if(millis()- tOld1 < intervaloLCD){
      //tOld1 = millis();
      
      lcd.setCursor(0, 0);
      lcd.print("Capacidade:");
    
      lcd.print(capacidade);
      lcd.setCursor(15,0);
      lcd.print("%");
      
      lcd.setCursor(0, 1);
      
      lcd.print("Bomba:");
      
      if(estadoBomba==1){
          lcd.setCursor(6, 1);
          lcd.print(" ATIVADA    ");  
      }else{
          //lcd.clear();
          lcd.setCursor(6, 1);
          lcd.print("DESATIVADA   ");  
      
      }
 
  }if(millis()- tOld1 > intervaloLCD){
      
      lcd.setCursor(0, 0);
      lcd.print("Controle:");

      if(estadoControlador==1){
          lcd.print(" AUTO     ");

          lcd.setCursor(0, 1);
          lcd.print("Tipo: ");

          if(type==1){
              lcd.print("ON-OFF     ");
          }
          if(type==2){
              lcd.print("Histerese");
          }
          
      }else{
        lcd.print(" MANUAL  ");  
      }
    
  }if(millis()- tOld1 > intervaloLCD*2){
    tOld1 = millis();  
  }
}

//------------------------------------------------------------------------------------------------------------//

void publicarUbidots(){
  
  if(millis()- tOld2 > 300000){
      tOld2 = millis();
      
      client.reconnect();
        
      client.ubidotsSubscribe("supervisorio", "hi");
      client.ubidotsSubscribe("supervisorio", "hs");
      client.ubidotsSubscribe("supervisorio", "setpoint");
      client.ubidotsSubscribe("supervisorio", "tipo");
      client.ubidotsSubscribe("supervisorio", "estadoControlador");
      client.ubidotsSubscribe("supervisorio", "bombaManual");
  }    
     
  
  if(millis()- tOld > intervalo){
      tOld = millis();
      
      if(!client.connected()){
         
        client.reconnect();
        
        client.ubidotsSubscribe("supervisorio", "hi");
        client.ubidotsSubscribe("supervisorio", "hs");
        client.ubidotsSubscribe("supervisorio", "setpoint");
        client.ubidotsSubscribe("supervisorio", "tipo");
        client.ubidotsSubscribe("supervisorio", "estadoControlador");
        client.ubidotsSubscribe("supervisorio", "bombaManual");
      }

      client.add("Nivel", nivel);
      
      if(alert == 0){
        alert = 1;
      }else if(alert == 1){
        alert = 0;
      }
      
      client.add("Alerta", alert);
      
      if(mudancaEstadoBomba!=estadoBomba){

          client.add("Estado", estadoBomba);
          
          mudancaEstadoBomba = estadoBomba;
      }
      
      client.ubidotsPublish("ESP8266");
  }

  

  client.loop();
  
}

//------------------------------------------------------------------------------------------------------------//

void loop() {
   
  lerADC();
  
  calcularNivel();
  
  controlador();  
  
  displayLCD();

  publicarUbidots();

}
