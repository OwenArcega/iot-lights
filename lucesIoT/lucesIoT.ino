#define USE_ARDUINO_INTERRUPTS false    // Prepara interruptores de bajo nivel para leturas mas precisas.

///////////////////////////////////////////////////////////////////JSON/////////////////////////////////////////////////////////////////////////
#include <ArduinoJson.h>


//////////////////////////////////////////////////////////////////////MQTT//////////////////////////////////////////////////////////////////
#include <WiFi.h>
#include <PubSubClient.h>

// Reemplaza con tus credenciales de WiFi
const char* ssid = "owen";
const char* password = "12345678";

// Reemplaza con la dirección IP y el puerto de tu broker Mosquitto
const char* mqtt_server = "192.168.0.34";
const int mqtt_port = 1883;
const char* topic = "/se/luces";
const char* topicSubscription = "/se/web";

WiFiClient espClient;
PubSubClient client(espClient);


/////////////////////////////////////////////////////////////////////////////Sensor de Corazon//////////////////////////////////////////////////////////////////////////
#include <PulseSensorPlayground.h>     // Incluye la librería PulseSensorPlayground.   


//  Variables  //
#define sensorCardiaco 35           // Sensor de ritmo cardiaco
#define relevadorCardiaco 13         // Pin conectado al relevador
#define umbralCardiaco 550         // Determina cuál señar tomar como pulso cardiaco

PulseSensorPlayground pulseSensor;  //Crea la instancia del objeto de PulseSensorPlayground

int UpperThreshold = 518;
int LowerThreshold = 490;

int reading = 35;

float BPM = 0.0;

bool IgnoreReading = false;
bool FirstPulseDetected = false;

unsigned long FirstPulseTime = 0;
unsigned long SecondPulseTime = 0;
unsigned long PulseInterval = 0;


/////////////////////////////////////////////////////////////////////////////Sensor RFID/////////////////////////////////////////////////////////////////////////////////
#include <SPI.h>            // Incluye las librerías para usar el sensor
#include <MFRC522.h>

#define RST_PIN 0        // Configuramos el pin del reset del módulo RC522
#define SS_PIN 5         // Configuramos el pin del SS del módulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Creamos instancia del módulo MFRC522

#define relevadorRFID 21                   //Definimos el pin 5 de arduino como contacto para el relevador

////////////////////////////////////////////////////////PIR//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define PIR 17             // señal de PIR a pin 16
#define relevadorPIR 22    //Pin de conexión del relevador

//////////////////////////////////////////////////////////////////HUMO////////////////////////////////////////////////////////////////////////////////////////

#define relevadorHumo 25               // Pin del relevador del humo
#define sensorHumo 32 // Pin analógico del sensor de llama
int flameValue = 0; // Variable para guardar el valor analógico del sensor
int threshold = 700; // Umbral para detectar humo

//----------------------------------------------------------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////SETUP/////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);                       //Iniciamos la velocidad de la comunicación serial

  ///////////////////////////////////////////////////////////////Conexion a internet/////////////////////////////////////////////////////////////////
  WiFi.begin(ssid, password); //Configura  la red wifi con el nombre de la red y la contraseña
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conectado a internet");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP()); //Muestra la direccion IP del ESP32
pinMode(sensorHumo, INPUT);
  /////////////////////////////////////////////////////////////////MQTT//////////////////////////////////////////////////////////////////////////////
  client.setServer(mqtt_server, mqtt_port);                     //Configura el servidor del broker
  client.setCallback(callback);                                 //Configura la funcion de callback

  
  /////////////////////////////////////////////////////////////RFID///////////////////////////////////////////////////////////////////////////////////
  
  pinMode (relevadorRFID, OUTPUT);                //Definimos el modo del pin asignado para la lámpara como salida
  SPI.begin();                              //Iniciamos la libreria SPI
  mfrc522.PCD_Init();                       //Iniciamos la instancia del módulo RC522 creado anteriormente
  Serial.println("Esperando tarjeta");      //Mandamos información al monitor serie para saber que el dispositivo
  //esta en operación

  ////////////////////////////////////Sensor de Corazon//////////////////////////////////////////////////////////////////////////
  
  pulseSensor.analogInput(sensorCardiaco);            // La función analogInput configura un pin para leer lecturas analógicas, usamos el pin del
  // sensor para configurarlo
  // cada qu se detecte un pulso cardiaco
  pulseSensor.setThreshold(umbralCardiaco);               // La función setThreshold crea el umbral para determinar qué lecturas tomar como latidos cardiacos

  pinMode(relevadorCardiaco, OUTPUT);                //Define el pin del relevador para el sensor cardiaco
  
  ///////////////////////////////////////////////////////////////PIR///////////////////////////////////////////////////////////////////////////////////////////////////////
  pinMode(PIR, INPUT);           // Define el pin del PIR para la entrada de datos del sensor

  pinMode(relevadorPIR, OUTPUT);  // Define el pin del relevador como salida
  
  //////////////////////////////////////////////////////////////////HUMO////////////////////////////////////////////////////////////////////////////////////////////
  pinMode(sensorHumo, INPUT);      // Define el pin del microfono como entrada de datos
  pinMode (relevadorHumo, OUTPUT);         // Define el pin del relevador como salida de datos

  delay(2000);                    // Demora para estabilizar los sensores
  
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void encenderApagar(bool pir, bool rfid, bool humo, bool cardiaco){
  if(pir == true){
    digitalWrite(relevadorPIR, HIGH);
  } else{
    digitalWrite(relevadorPIR, LOW);  
  }

  if(rfid == true){
    digitalWrite(relevadorRFID, HIGH);
  } else{
    digitalWrite(relevadorRFID, LOW);  
  }

  if(humo == true){
    digitalWrite(relevadorHumo, HIGH);
  } else{
    digitalWrite(relevadorHumo, LOW);  
  }

  if(cardiaco == true){
    digitalWrite(relevadorCardiaco, HIGH);
  } else{
    digitalWrite(relevadorCardiaco, LOW);  
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el tema: ");
  Serial.println(topic);

  String payload_str;
  for (int i = 0; i < length; i++) {
    payload_str += (char)payload[i];
  }
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, payload_str);
  
  encenderApagar(doc["pir"], doc["rfid"], doc["humo"], doc["cardiaco"]);

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conectar al broker MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("conectado");
      // Suscríbete a los temas que desees aquí
      client.subscribe(topicSubscription);
    } else {
      Serial.print("Error, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando nuevamente en 5 segundos");
      delay(5000);
    }
  }
}

//////////////////////////////////////////////////CLASE//////////////////////////////////////////////////////////////////////
class sensoresActuadores {
  public:
    bool activarRelevadorRFID(uint8_t);           //Definimos las clases
    bool activarRelevadorPIR(uint8_t, uint8_t);
    uint16_t activarRelevadorHumo(uint8_t, uint8_t);
  private:
    bool PIREncendido = false; //definimos las variables a utilizar dentro de cada método
    bool estadoRFID = false;
    bool estadoHumo = false;
    uint8_t lecturaPIR = 0;
    String cadenalampara[2] = {"Lampara encendida", "Lampara Apagada"};
    uint8_t indexCadena = 0;
    uint8_t lecturaHumo = 0;
    uint8_t umbralHumo = 70;
    uint8_t estadoPIR = 0;
};

///////////////////////////////////Método para RFID//////////////////////////////////////////////////////////////
bool sensoresActuadores::activarRelevadorRFID(uint8_t relRFID) {
  Serial.println(cadenalampara[indexCadena]); // Muestra el estado de la lampara
  if (estadoRFID == true) {                   // Cambia el estado del relevador dependiendo de su estado
    digitalWrite(relRFID, LOW);
    estadoRFID = false;
    indexCadena = 1;                        // Cambia la cadena a mostrar del estado de la lámpara
    return true;
  } else {
    digitalWrite(relRFID, HIGH);
    estadoRFID = true;
    indexCadena = 0;
    return false;
  }
};

///////////////////////////////////////////////Método del PIR/////////////////////////////////////////////////////////////
bool sensoresActuadores::activarRelevadorPIR(uint8_t relPIR, uint8_t pinPIR) {
  estadoPIR = digitalRead(pinPIR);                              // lectura de estado de señal
  delay(200);
  if (estadoPIR == HIGH) {                                 // Si la señal esta en alto indicando movimiento
    Serial.println("Se ha encendido el PIR");
    if (PIREncendido) {                                 // Si el relevador está activado lo apaga
      digitalWrite(relPIR, LOW);
      PIREncendido = false;                             // Ahora el relevador está apagado
      return false;
    } else {
      digitalWrite(relPIR, HIGH);
      PIREncendido = true;                              // Si el relevador está apagado lo enciende
    }
    return true;
  } 
};

//////////////////////////////////////////////////Método del sensor de Humo////////////////////////////////////////////////////
uint16_t sensoresActuadores::activarRelevadorHumo(uint8_t relHumo, uint8_t pinHumo) {
/*  lecturaHumo = analogRead(pinHumo);                       // Obtiene la lectura del sensor de Humo
  delay(50);
  if (lecturaHumo > umbralHumo) {                                // Si la lectura es mayor al umbral definido, se manda a llamar la función para activar el relevador
    Serial.println("Se detecto humo");
    if (estadoHumo) {                           // Si esta encendido apaga el relevador y resetea la bandera
      digitalWrite(relHumo, LOW);
      estadoHumo = false;
      return lecturaHumo;
    } else {
      digitalWrite(relHumo, HIGH);                         // Si esta apagado, enciendo el rele y resetea la bandera
      estadoHumo = true;
      return lecturaHumo;
    }
  } */
};

sensoresActuadores activador; // Crea el objeto de la clase sensoresActuadores


//---------------------------------------------------------------------------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////LOOP///////////////////////////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------------------------------------

long int lecturaCardiaco = 0;
unsigned long lastMessage = 0;
void loop() {
   if (!client.connected()) {
    reconnect();
  }
  client.loop();

//--------------------------------------------------------------------------------------------------------------------------------------------------------
///////////////////////////////////Pagina web//////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------------------------------------

  StaticJsonDocument<200> doc;
  char jsonMessage[512];
  JsonObject sensors = doc.createNestedObject("sensors");
  
  ////////////////////////////////////////////////////////////////RFID////////////////////////////////////////////////////////////////////////////////////////////////
  
  // Verifica si hay una tarjeta RFID cerca
  if (mfrc522.PICC_IsNewCardPresent()) {
    Serial.print("Tarjeta RFID detectada");
    digitalWrite(relevadorRFID, HIGH);
    sensors["rfid"] = true;
    // Realiza aquí las acciones necesarias con la tarjeta leída

    mfrc522.PICC_HaltA(); // Termina la comunicación con la tarjeta RFID
  } else{
    digitalWrite(relevadorRFID, LOW);
    sensors["rfid"] = false;
  }
  
  /////////////////////////////////////////////////////////////////////////////Sensor de Corazon//////////////////////////////////////////////////////////////////////////
  if(pulseSensor.sawNewSample()){      
    lecturaCardiaco = analogRead(sensorCardiaco) / 60;
    Serial.print(lecturaCardiaco);
    Serial.println(" BPM");
    sensors["cardiaco"] = lecturaCardiaco;
    if(lecturaCardiaco >= 30){
      Serial.println("Sucedio un pulso cardiaco");
      digitalWrite(relevadorCardiaco, HIGH);
    } else{
      digitalWrite(relevadorCardiaco, LOW);
    }
  }

  ////////////////////////////////////////////////////////PIR//////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  //sensors["pir"] = activador.activarRelevadorPIR(relevadorPIR, PIR);           // Mandamos a llamar al método para activar el relevador del PIR
  if(digitalRead(PIR)){
    Serial.println("Movimiento");
    sensors["pir"] = true;  
  } else{
    Serial.println("Sin movimiento");
    sensors["pir"] = false;
  }

  
  //////////////////////////////////////////////////////////////////Humo////////////////////////////////////////////////////////////////////////////////////////////
   flameValue = analogRead(sensorHumo); // Leer el valor analógico del sensor
   sensors["humo"] = flameValue;
  Serial.print("Valor del sensor: ");
  Serial.println(flameValue); // Imprimir el valor del sensor
  if (flameValue > threshold) { // Si el valor supera el umbral
    digitalWrite(relevadorHumo, HIGH);
    Serial.println("Humo detectado!"); // Imprimir un mensaje de alerta
  }
  else { // Si no
     digitalWrite(relevadorHumo, LOW);
    Serial.println("Todo normal"); // Imprimir un mensaje de normalidad
  }
  delay(1000); // Esperar un segundo

  
  unsigned long now = millis();
  if(now - lastMessage > 2000){
    lastMessage = now;
    serializeJson(doc, jsonMessage);
    client.publish(topic, jsonMessage);  
  } 
};
