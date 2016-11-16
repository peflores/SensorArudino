#include <SPI.h>
#include <Ethernet.h>
#include "DHT.h"


#define DHTPIN 2     // Pin digital del sensor temp
#define DHTTYPE DHT22


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,111); // IP
byte gateway[] = { 192, 168, 1, 1 }; // IP de router
byte subnet[] = { 255, 255, 255, 0 }; // subnet
EthernetServer server(80);  // Arrancamos el servidor en el puerto estandar 80
DHT dht(DHTPIN, DHTTYPE);


// TODO: Modificar pins
int led_corte = 9; //view corte energy detected
int led_tl = 3; // temp low
int led_tm = 4; // temp medium
int led_th = 5; // temp high
int led_sd = 6; // smoke detected

int ventilador = 7; //activar rele
int corte = 8; //detectar corte energia

int smoke = A5; //detectar humo

String st_ventilador = "NO"; // estado ventilador (SI encendido)
String st_energia = "SI"; // estado energia (NO cortado)
String st_humo= "NO";// estado humo (SI con humo)

int sensorThres = 250;

void setup() 
{
    Serial.begin(9600);
    while (!Serial) ;

    Ethernet.begin(mac, ip, gateway, subnet);
    server.begin();

    Serial.print("Servidor Web en la direccion: ");
    Serial.println(Ethernet.localIP());

    pinMode(ventilador, OUTPUT);//pin7
    pinMode(led_corte, OUTPUT);//pin9
    pinMode(led_tl, OUTPUT);//pin3
    pinMode(led_tm, OUTPUT);//pin4
    pinMode(led_th, OUTPUT);//pin5
    pinMode(led_sd, OUTPUT);//pin6
    pinMode(corte, INPUT); //pin8
    pinMode(smoke, INPUT); //pinA5

    dht.begin();
}


void loop() 
{
    EthernetClient client = server.available();  // Buscamos entrada de clientes
    if (client) 
    {
        Serial.println("new client");
        boolean currentLineIsBlank = true;  // Las peticiones HTTP finalizan con linea en blanco
        String cadena = "";
        while (client.connected()) 
        {
            if (client.available()) 
            {
                char c = client.read();
                Serial.write(c);
                cadena.concat(c);

                
                    
                    // Comando Ventilador:
                    int posicion = cadena.indexOf("RELE=") ;
                    if(cadena.substring(posicion)=="RELE=ON")
                    {
                        digitalWrite(ventilador, HIGH);
                        st_ventilador = "SI";
                    }
                    else if(cadena.substring(posicion)=="RELE=OFF")
                    {
                        digitalWrite(ventilador, LOW);
                        st_ventilador = "NO";
                    }
                
                if (c == '\n' && currentLineIsBlank)
                {
                    
                    // Comando LED: (se prende solo en caso de corte)
                    if(digitalRead(corte) == LOW)
                    {
                        digitalWrite(led_corte, HIGH);
                        st_energia = "NO";
                    }
                    else {                      
                          digitalWrite(led_corte, LOW);
                          st_energia = "SI";
                          }


                    float hum = dht.readHumidity();
                    float temp = dht.readTemperature();

                    
                    // LED segun temperatura
                    if (temp < 20)
                    {
                        digitalWrite(led_tl, HIGH);
                        digitalWrite(led_tm, LOW);
                        digitalWrite(led_th, LOW);
                    }
                    else if (temp >= 20 && temp <= 35)
                    {
                        digitalWrite(led_tm, HIGH);
                        digitalWrite(led_tl, HIGH);
                        digitalWrite(led_th, LOW);
                    }
                    else if (temp > 35)
                    {
                        digitalWrite(led_th, HIGH);
                        digitalWrite(led_tl, HIGH);
                        digitalWrite(led_tm, HIGH);
                    }
                    
                    //lectura sensor de humo
                    int analogSensor = analogRead(smoke);
                    // LED cuando se detecta humo
                    if (analogSensor > sensorThres)
                    {
                        st_humo="SI";
                        digitalWrite(led_sd, HIGH);    
                    }
                    else if (analogSensor < 100)
                    {
                      digitalWrite(led_sd, LOW);
                       st_humo="NO";
                    }
                    else if (analogSensor >= 100 && analogSensor <150)
                    {
                      analogWrite(led_sd, 30);
                      st_humo="NO";
                    }
                    else if (analogSensor >= 150 && analogSensor <200)
                    {
                     analogWrite(led_sd, 75);
                     st_humo="NO";
                    }  
                    else if (analogSensor >= 200 && analogSensor <250)
                    {
                      analogWrite(led_sd, 120);
                      st_humo="NO";
                    }  
                    
                    

                    // Envio de JSON al cliente
                    client.println("HTTP/1.1 200 OK");  // Encabezado Protocolo HTTP
                    client.println("Content-Type: application/json;charset=utf-8");
                    client.println("Connection: close");  
                    client.println("Refresh: 5");  // Actualizar cada 10 segs
                    client.println();
                
                    

                    if (isnan(hum) || isnan(temp) )
                    {
                        Serial.println("Falla en la lectura del sensor!");
                    }
                    else 
                    {
                        Serial.println(temp);
                        Serial.println(hum);
                        Serial.println(digitalRead(corte));
                    }

                    // JSON que recibe el cliente:
                    client.print ("{\"ventilador\":\"");
                    client.print (st_ventilador);
                    client.print ("\", \"energia\":\"");
                    client.print (st_energia);
                    client.print ("\", \"temp\":\"");
                    client.print (temp);
                    client.print ("\", \"humedad\":\"");
                    client.print (hum);
                    client.print ("\", \"humo\":\"");
                    client.print (st_humo);
                    client.println ("\"}");

                    break;
                    Serial.print(cadena);
                    cadena = "";    // Una vez procesado, limpia el string
                }
                if (c == '\n') 
                    currentLineIsBlank = true;
                else if (c != '\r') 
                    currentLineIsBlank = false;
            }
        }
        delay(10);
        client.stop();
        Serial.println("client disonnected");
    }      
}
