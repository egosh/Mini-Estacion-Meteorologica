/*
 #       _\|/_   A ver..., ¿que tenemos por aqui?
 #       (O-O)
 # ---oOO-(_)-OOo---------------------------------

 ####################################################
 # ************************************************ #
 # *   MAYORDOMO VIRTUAL CON ESP8266 Y TELEGRAM   * #
 # *         Aplicado a estacion miniMeteo        * #
 # *         Autor: Eulogio Lopez Cayuela         * #
 # *      Versión 0.1g     Fecha: 14/07/2021      * #
 # *                                              * #
 # ************************************************ #
 ####################################################


NOTA: Cambiar los ### por los datos que se vayan a usar
 */

#define __VERSION__        "mini METEO 0.1b"


/*
      =====  NOTAS DE LA VERSION  ===== 



  WeMos     ESP-8266    Functiones
  
  D0        GPIO16      DeepSleep out  
  D1        GPIO5       SCL
  D2        GPIO4       SDA
  D3        GPIO0       10k Pull-up
  D4        GPIO2       10k Pull-up, blue LED
  D5        GPIO14      SPI SCK
  D6        GPIO12      SPI MISO
  D7        GPIO13      SPI MOSI
  D8        GPIO15      SPI SSEL, 10k Pull-down
  Rx        GPIO03      Serial Rx
  Tx        GPIO01      Serial Tx
  A0        A0/ADC0     Entrada Analogica

 */
 


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        IMPORTACION DE LIBRERIAS
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/


#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>                  //https://github.com/tzapu/WiFiManager


#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>  
#include "SparkFunHTU21D.h" 


/* Inicializar Telegram BOT */
#define BOT_TOKEN " ####"   // COlocar token_minimeto_bot



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        ALGUNAS DEFINICIONES PERSONALES PARA MI COMODIDAD AL ESCRIBIR CODIGO
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

#define AND         &&
#define OR          ||
#define NOT          !
#define ANDbit       &
#define ORbit        |
#define XORbit       ^
#define NOTbit       ~

#define SERIAL_BEGIN        Serial.begin
#define PRINTLN             Serial.println
#define PRINT               Serial.print
#define SERIAL_BAUD_RATE    115200


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        SECCION DE DEFINICION DE PINES
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

#define PIN_LED_OnBoard             D4    // Led on Board Wemos
#define PIN_pulsador_config         D5    // para poner un pulsador pullup para configuracion (esto es revisable)
#define PIN_dht                     D6    //  



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        CONSTANTES DEL PROGRAMA
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

#define ID_STATION   "####"    // ID de estacion, codigo para distinguirlas (ha de ser una cadena)



#define ADMIN_USER   "###"  // ID usuario
#define LED_ON    0    //el led interno del wemos esta conectado a una salida Pull-up (enciende con LOW)
#define LED_OFF   1
#define ON        false
#define OFF       true


#define INTERVALO_ACCESO_SERVIDOR    1000     // tiempo entre busquedas de mensajes en el servidor de telegram
#define INTERVALO_LECTURA_SENSORES   10000    // tiempo entre refrescos automaticos de variables clima



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        SECCION DE DEFINICION DE VARIABLES GLOBALES
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

float tempHTU;
float humedad; 
float tempMed;

float temperatura; 
float temperatura_min = 1000; 
float temperatura_max = -1000; 
float presionABS;
float presionREL; 
String fecha_hora="";

uint32_t ultimo_acceso;
uint32_t ultima_lectura;
uint32_t proximo_mensaje = 0;
uint32_t momento_actual = 0;
uint32_t intervalo_mensaje = 60000; 

#define ALTITUD ###      	// Altitud de tu localidad en metros, para el calculo de la presion relativa
//#define ALTITUD 407.0      	// Altitud de "Sorbas" en metros, para el calculo de la presion relativa


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        CREACION DE OBJETOS
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/


X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
  
//Adafruit_BME280 sensor_bmx280;
Adafruit_BMP280 sensor_bmx280;  //
HTU21D sensorHTU; 



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
   ****************************************************************************************************** 
                                    FUNCION DE CONFIGURACION
   ****************************************************************************************************** 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

void setup()
{

  
  SERIAL_BEGIN(115200);
  PRINTLN();
  
  pinMode(PIN_LED_OnBoard, OUTPUT);
  digitalWrite(PIN_LED_OnBoard, OFF);
    
   if (!sensor_bmx280.begin(0x76)) {   
     PRINTLN("FALLO SENSOR BME280");
     //while (true);
   } 

  sensorHTU.begin(); 
   

  secured_client.setTrustAnchors(&cert); // certificado necesario apra la API de telegram (api.telegram.org)

  /* Establecer el modo WiFi */
  WiFi.mode(WIFI_STA);
 
  /* Iniciar el servicio de gestion de redes para intentar conectar a una red guardada en eeprom */
  WiFiManager wifiManager;
  
  digitalWrite(PIN_LED_OnBoard, LED_ON); 
//  /* SIN USO */ 493369 bytes ROM // 33484 bytes RAM
//  pinMode(PIN_pulsador_config, INPUT_PULLUP);   
//  /* disponemos de 5 segundos para forzar el modo de configuracion */ 
//  PRINTLN(F("MANTEN PULSADO PARA INICIAR CONFIGURACION"));
//  delay(5000);
// 
//  boolean FLAG_estado_pulsador = digitalRead(PIN_pulsador_config);
//
//
//  /* Si el pulsador esta activo al final de la pausa, se entra en el portal de configuracion */
//  if (FLAG_estado_pulsador == false){   
//    PRINTLN(F("< CONFIG MODE >"));  
//    wifiManager.resetSettings();  //reset de cualquier configuracion guardada, para forzar el modo AP
//  }
      
  /* Intentar conectar y si no... montar un AP para configuracion */ 
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,5,1), IPAddress(192,168,5,1), IPAddress(255,255,255,0));
  delay(1000);
  wifiManager.autoConnect("INFOTEC_IoT", "minimeteo");          // portal de configuracion si no podemos conectar


  digitalWrite(PIN_LED_OnBoard, LED_OFF);   //el led se apaga si hemos conseguido establecer conexion
      
  PRINT(F("\nWiFi conectada. IP address: "));
  PRINTLN(WiFi.localIP()); PRINTLN(F("\n\n"));

  PRINTLN(F("Actualizando fecha/hora: "));
  configTime("GMT-1", "pool.ntp.org", "time.nist.gov");   //
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    PRINT(".");
    delay(1000);
    now = time(nullptr);
  }

  PRINTLN("");
  update_time();
  PRINTLN(fecha_hora);
  PRINTLN("\n");
  
  enviar_cabecera(ADMIN_USER);
  eliminar_mensajes_viejos();   // eliminar mensajes que llegaron mientras estabamos desconectados
                                // para evitar responder a cosas que ya no son necesarias
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
   ****************************************************************************************************** 
                                  BUCLE PRINCIPAL DEL PROGRAMA
   ****************************************************************************************************** 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

void loop()
{
  
  uint32_t momento_actual = millis();
  
 
   

  if ( momento_actual - ultimo_acceso > INTERVALO_ACCESO_SERVIDOR) {
    int num_mensajes_pendientes = bot.getUpdates(bot.last_message_received + 1);
    
    while (num_mensajes_pendientes) {
      procesarTelegramas(num_mensajes_pendientes);
      num_mensajes_pendientes = bot.getUpdates(bot.last_message_received + 1);
    }
    ultimo_acceso = momento_actual; 
  }
  
  if ( momento_actual - ultima_lectura > INTERVALO_LECTURA_SENSORES) {
	static int contador;
	contador++;
	if ( contador > 5) {
	  contador = 0;
	  update_day();
	}
    update_sensores();
    ultima_lectura = momento_actual; 
  } 

  /*if(momento_actual-proximo_mensaje > intervalo_mensaje)
  {
    proximo_mensaje = momento_actual;
    bot.sendMessage(ADMIN_USER, fecha_hora);
    }*/
      

    
}




/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx 
        BLOQUE DE FUNCIONES: LECTURAS DE SENSORES, COMUNICACION SERIE, INTERRUPCIONES...
   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//    TELEGRAM
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
// ENVIAR CABECERA CON DATOS DE CONEXION
//========================================================

void enviar_cabecera(String usuario)
{
  String local_ip = String() + WiFi.localIP()[0] + "." + WiFi.localIP()[1] + "." + WiFi.localIP()[2] + "." + WiFi.localIP()[3];
  
  String cabecera="BOT ";
  cabecera +=__VERSION__;
  cabecera += "\n";
  cabecera += "Conectado a: ";
  cabecera +=  WiFi.SSID();
  cabecera += "\n";
  cabecera += "IP: ";
  cabecera += local_ip;
  cabecera += "\n";
  update_time();
  cabecera += fecha_hora;
  
  bot.sendMessage( usuario, cabecera );
}



//========================================================
// ENVIAR CABECERA CON DATOS DE CONEXION
//========================================================

void dar_hora(String usuario)
{
  update_time();
  fecha_hora = "Fecha/Hora:  " + fecha_hora;
  bot.sendMessage( usuario, fecha_hora );
}


//========================================================
// CONTROL DEL CAMBIO DE DIA
//========================================================

void update_day()
{                                         //          1         2
  time_t now = time(nullptr);             //0123456789012345678901234
  const char *date_time = ctime(&now);    //Wed Jul 14 18:43:14 2021  --> string.substring(from, to) 
                                          //Wed Jul 14 2021 / 18:43:14 
  String hoy = String(date_time);
  hoy = hoy.substring(0, 4);    //'wed '
  
  static String dia_actual="abcde";
 
  if (hoy.compareTo(dia_actual)!=0) { 
    dia_actual = hoy;
	reset_min_max();
  }

}

//========================================================
// ACTUALIZAR FECHA Y HORA
//========================================================

void update_time() 
{                                         //          1         2
  time_t now = time(nullptr);             //0123456789012345678901234
  const char *date_time = ctime(&now);    //Wed Jul 14 18:43:14 2021  --> string.substring(from, to) 
                                          //Wed Jul 14 2021 / 18:43:14 
  String fecha = String(date_time);
  
  fecha_hora="";
  
  fecha_hora += fecha.substring(0, 4);    //'wed '
  fecha_hora += fecha.substring(8, 11);   //'14 '
  fecha_hora += fecha.substring(4, 8);    //'Jul '
  fecha_hora += fecha.substring(20,24);   //'2021'
  fecha_hora += " / ";
  fecha_hora += fecha.substring(11, 19);  //'hora... '
}

//========================================================
// ENVIAR INFORMACION CLIMATICA
//========================================================

void enviar_informacion(String usuario)
{
  update_sensores();
  
  String   info_clima = "Temperatura : ";               
  info_clima += String(tempMed);
  info_clima += " ºC\n";
  //info_clima = "Temperatura BMP : "; //BMP
  //info_clima += String(temperatura);
  //info_clima += " ºC\n";
  //info_clima += "Temperatura HTU : ";        //HTU
  //info_clima += String(tempHTU);
  //info_clima += " ºC\n";
  info_clima += "Presion : ";
  info_clima += String(presionREL);
  info_clima += " mb\n";
  info_clima += "Humedad: ";                //HTU
  info_clima += String(humedad);
  info_clima += " %\n";


  bot.sendMessage( usuario, info_clima );
}

//========================================================
// ENVIAR INFORMACION DE MIN Y MAX
//========================================================

void enviar_min_max(String usuario)
{
  update_sensores();
  
  String info_temp = "Temperatura Actual : "; 
  info_temp += String(temperatura);
  info_temp += " ºC\n";
  info_temp += "Temperatura MIN : ";  
  info_temp += String(temperatura_min);
  info_temp += " ºC\n";
  info_temp += "Temperatura MAX : ";  
  info_temp += String(temperatura_max);
  info_temp += " ºC\n";
  bot.sendMessage( usuario, info_temp );
}

//========================================================
// RESET DE MIN Y MAX
//========================================================

void reset_min_max()
{
  temperatura_min = 1000;
  temperatura_max = -1000;
  update_sensores();
}


//========================================================
// ELIMINAR TELEGRAMAS VIEJOS AL REINICIAR
//========================================================

void eliminar_mensajes_viejos()
{
  /* funcion apra eliminar mensajes que han podido llegar mientras estamos desconectados */

  int num_mensajes_pendientes = bot.getUpdates(bot.last_message_received + 1);
  while (num_mensajes_pendientes) {
    num_mensajes_pendientes = bot.getUpdates(bot.last_message_received + 1);
  }
}



//========================================================
// PROCESAR TELEGRAMAS RECIBIDOS
//========================================================

void procesarTelegramas(int num_mensajes)
{
  for (int i = 0; i < num_mensajes; i++) {

    String from_name = bot.messages[i].from_name;  
    String chat_id = bot.messages[i].chat_id; 
    String text = bot.messages[i].text; 

    /* DEBUG */
    PRINT("   >>> USUARIO: "); 
    PRINTLN( chat_id );
    PRINT("   >>> NOMBRE:  "); 
    PRINTLN( from_name );    
    PRINT("   >>> COMANDO: "); 
    PRINTLN( text );
    PRINTLN("");

   

    if (text.equals("/red") && chat_id == ADMIN_USER) { 
      enviar_cabecera( chat_id );
    } 

    else if (text.equals("/hora")) { 
      dar_hora( chat_id );
    }

    else if (text.equals("/clima")) {  
      enviar_informacion( chat_id ); 
	  delay(100);
    }
     else if (text.equals("/minmax")) {  
      enviar_min_max( chat_id ); 
	  delay(100);
    }   
     else if (text.equals("/clear") && chat_id == ADMIN_USER) {  
      reset_min_max(); 
	  delay(100);
    } 	
    else if (text.equals("/on")) {  
      digitalWrite(PIN_LED_OnBoard, ON);   
      bot.sendMessage( chat_id, "Led ON" );  
    } 
     
    else if (text.equals("/off")) {  
      digitalWrite(PIN_LED_OnBoard, OFF);   
      bot.sendMessage( chat_id, "Led OFF" );  
    }
    
    else{
      bot.sendMessage( chat_id, "ok" );      
    }
  }
}



//========================================================
// ACTUALIZAR VARIABLES CLIMATICAS (funcion original)
//========================================================

void update_sensores()
{
  temperatura = sensor_bmx280.readTemperature();
  
  presionABS = sensor_bmx280.readPressure()/100.0;
 
  presionREL = (presionABS / pow(1.0-ALTITUD/44330, 5.255));       //calculada por formula 
  humedad = sensorHTU.readHumidity();
  if(humedad<0) { humedad=0; }
  if(humedad>100) { humedad=100; }
  tempHTU = sensorHTU.readTemperature();
  //tempMed = ((tempHTU + temperatura)/2);
  
  if (temperatura == 0){
    temperatura = tempHTU;
    tempMed = temperatura; 
  }
  else
  {
    tempMed = ((tempHTU + temperatura)/2);
  }

  if(temperatura>temperatura_max) { temperatura_max=temperatura; }
  if(temperatura<temperatura_min) { temperatura_min=temperatura; }
  
  /* DEBUG */
  PRINT(F("Temperatura BME280: "));PRINTLN(temperatura);

  PRINT(F("Temperatura HTU21: "));PRINTLN(tempHTU);
  PRINT(F("Humedad: "));PRINTLN(humedad);
  PRINT(F("PresionREL: "));PRINTLN(presionREL);

  }



//*******************************************************
//                    FIN DE PROGRAMA
//*******************************************************
