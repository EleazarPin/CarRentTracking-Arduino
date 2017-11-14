#include <TinyGPS.h>
#include <SD.h>
#include <SPI.h>
#include <Audio.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#define GPSSerial Serial1
#define HC06 Serial3

//***************variables para el gps****************************INI*
bool a = true;
TinyGPS gps;//Usamos los serial1 pin  Tx1 y  Rx1
int codGPS;
float latitude, longitude,latitudeOld, longitudeOld,alturaOld;
float constanteVariacionLL = 0.0003; 
int year;
byte month, day, hour, minute, second, hundredths;
unsigned long chars;
unsigned short sentences, failed_checksum;
boolean gpsSincronizado;
boolean gpsNoCalibrado;
//***************variables para el gps******************************FIN*
//***************variables para el bluetooth************************INI*
boolean flagBT;
//char c;
char caracter;
int found;
boolean accionValida;
//***************variables para el bluetooth************************FIN*
//***************variables para la Foto Resistencia*****************INI*
boolean flagSensorFotoresistencia;
int sensorFotoresistenciaPin = 52;
int ledFotoresistenciaPin = 26;
int sensorFotoresistenciaValue = 0;
int contFotoresistencia;
boolean hardCodModoNocturno;
//***************variables para la Foto Resistencia*****************FIN*
//***************variables para el mq7******************************INI*
int sensorMQ7Pin=A0;
int ledSensorMQ7Pin = 24;
//int sensorMQ7DOPin = 53; //NO SE USA
boolean flagSensorMQ7;
int adc_MQ;
float sensorMQ7Voltaje=0;
float sensorMQ7VoltajeLimite;//=3.20;//2.00;
float sensorMQ7VoltajeLimiteInf;//=3.15;//1.85;
boolean confirmacionHumo,confirmacionNoHumo;
//***************variables para el mq7******************************FIN*
//***************variables para el SD*******************************INI*
File Archivo;
int shieldSDPin = 28; //pin correspondiente a CS.
const String fileNameTraking = "traking.txt";
const String fileNameEvento = "evento.txt";
const String fileNameParametros = "param.txt";
const String fileNameLog = "registro.log";
int ledArchivoPin = 30; 
//***************variables para el SD*******************************FIN*
//***************variables para el gy-88****************************INI*
MPU6050 accel;
int16_t ax, ay, az;
int16_t ax2, ay2, az2;
int16_t rx,ry,rz;
unsigned long antes;
unsigned long despues;
unsigned long transcurridos;
//#define OUTPUT_READABLE_ACCELGYRO
int ledGY88 = 34;
bool blinkState = true;
int limiteAc;// = 5;
//***************variables para el gy-88****************************FIN*
//***************variables para el cooler***************************INI*
int TIP120pin = 7; //for this project, I pick Arduino's PMW pin 11
int velCooler;
//***************variables para el cooler***************************FIN*
//***************variables para el reproductor de audio*************INI*
const int S = 1024; // Number of samples to read in block
short buffer[S];
const short vCero=0;
int volume = 1024;
const String excesoVel="ev.wav";
const String humoDetectado="humo.wav";
const String encenderLuces="sos.wav";
const String aceleracionBruzca="alarma.wav";
//***************variables para el reproductor de audio*************FIN*
//***************variables extras***********************************INI*
int limiteHoraInicialNoche = 19;
int limiteMinutosInicialNoche = 30;
int limiteHoraFinalNoche = 6;
int limiteMinutosFinalNoche = 30;
int hActual,mActual;
boolean flagExesoVel;
int limVel;
float maxVel;
int ledExcesoVelPin = 32;
int contadorH;
int contadorHD;
int ledONPin = 30;

//***************variables extras***********************************FIN*

void setearPines()
{
  pinMode(sensorFotoresistenciaPin, INPUT);
  pinMode(ledFotoresistenciaPin, OUTPUT);
  pinMode(ledSensorMQ7Pin, OUTPUT);
  pinMode(ledGY88, OUTPUT);
  pinMode(ledONPin, OUTPUT);
  pinMode(shieldSDPin, OUTPUT);
  digitalWrite(ledFotoresistenciaPin, LOW);
  pinMode(ledONPin, OUTPUT);
  pinMode(TIP120pin, OUTPUT); // Set pin for output to control TIP120 Base pin
}

void inicializar()
{
//  c=' ';
  //limVel = 60;
  flagBT=false;
  velCooler = 255;
  hardCodModoNocturno=false;
  gpsNoCalibrado = true;
  flagSensorFotoresistencia=true;
  gpsSincronizado = false;
  flagSensorMQ7 = false;
  flagExesoVel = false;
  confirmacionHumo = false;
  confirmacionNoHumo = false;
  contFotoresistencia=100;
  contadorHD=50;
  maxVel = -1;
  latitudeOld = 0;
  longitudeOld = 0;
  alturaOld = -1;
  hActual=7;
  mActual=0;
  //GPSSerial.begin(9600);//Iniciamos el puerto serie del gps
  //GPSSerial.flush();
  HC06.begin(9600);//Iniciamos el puerto serie del bluetooth
  HC06.flush();
  if (!SD.begin(shieldSDPin)) {
    //Serial.println(" failed!");
    digitalWrite(ledGY88, HIGH);
    while(true);
  }
  //SD.begin(shieldSDPin);
  do
  {
    Wire.begin();
    accel.initialize();
  }while(!accel.testConnection());

  SPI.setClockDivider(shieldSDPin);
   
}

void setup() {
  setearPines();
  inicializar();  
  //Imprimimos:
  Serial.begin(9600);
  //analogWrite(TIP120pin, 0);
  actualizarParametrosDesdeSD();
}

void Reproducir(String arch)
{
  // 44100kHz stereo => 88200 sample rate
  // 100 mSec of prebuffering.
  Audio.begin(88200, 100);
  File myFile = SD.open(arch); // open wave file from sdcard
  if (!myFile) 
    return;
  while (myFile.available()) // until the file is not finished
  { 
    myFile.read(buffer, sizeof(buffer)); // read from the file into buffer
    Audio.prepare(buffer, S, volume); // Prepare samples 
    Audio.write(buffer, S); // Feed samples to audio
  }
  myFile.close();
  Audio.end();
  
}

void MQ7()
{
  adc_MQ = analogRead(sensorMQ7Pin); //Lemos la salida analógica del MQ
  sensorMQ7Voltaje = adc_MQ * (5.0 / 1023.0); //Convertimos la lectura en un valor de voltaje
  //Serial.println(sensorMQ7Voltaje);
  if(!flagSensorMQ7&&sensorMQ7Voltaje>sensorMQ7VoltajeLimite)
  {
    if(confirmacionHumo)
    {
      contadorHD--;
      if(contadorHD<1)
      {
        digitalWrite(ledSensorMQ7Pin, HIGH);
        Reproducir(humoDetectado);
        velCooler = 255;
        analogWrite(TIP120pin, velCooler); // By changing values from 0 to 255 you can control motor speed
        flagSensorMQ7=true;
        escribirEventoEnSD("HumoOn",false);
        contadorH=100;
      }
    }
    else
    {
      confirmacionHumo=true;
      confirmacionNoHumo=false;
    }
  }
  if(flagSensorMQ7&&sensorMQ7Voltaje<sensorMQ7VoltajeLimite)
  {
    velCooler=128;
    analogWrite(TIP120pin, velCooler);
  }
  if(flagSensorMQ7&&sensorMQ7Voltaje<sensorMQ7VoltajeLimiteInf)
  {
    if(confirmacionNoHumo)
    {
      contadorH--;
      velCooler--;
      analogWrite(TIP120pin, velCooler);
      if(contadorH<1)
      {
        digitalWrite(ledSensorMQ7Pin, LOW);
        analogWrite(TIP120pin, 0);
        //analogWrite(DAC0, 0); // By changing values from 0 to 255 you can control motor speed
        flagSensorMQ7=false;
        escribirEventoEnSD("HumoOff",false);
        contadorHD=100;
      }
    }
    else
    {
      confirmacionHumo=false;
      confirmacionNoHumo=true;
    }
  }
}

int16_t Maximo(int16_t a, int16_t b)
{
  return (((a) > (b)) ?  (a) : (b));
}
boolean modoNocturno(int h, int m)
{
  /*Serial.println(h);
  Serial.println(limiteHoraInicialNoche);
  Serial.println(m);
  Serial.println(limiteMinutosInicialNoche);*/
  
  return (hardCodModoNocturno||((h>limiteHoraInicialNoche&&m>limiteMinutosInicialNoche)||(h<limiteHoraFinalNoche&&m<limiteMinutosFinalNoche)));
}

void Fotoresistencia()
{
  sensorFotoresistenciaValue = digitalRead(sensorFotoresistenciaPin);
  if(flagSensorFotoresistencia&&sensorFotoresistenciaValue==1&&modoNocturno(hActual,mActual))
  {
    escribirEventoEnSD("PrenderLuces",false);
    
    digitalWrite(ledFotoresistenciaPin, HIGH);
    Reproducir(encenderLuces);
    flagSensorFotoresistencia=false;
    
  }
  if(!flagSensorFotoresistencia&&sensorFotoresistenciaValue==0)
  {
    if(contFotoresistencia<1)
    {
      //escribirEventoEnSD("ApagarLuces",false);
      digitalWrite(ledFotoresistenciaPin, LOW);
      flagSensorFotoresistencia=true;
      contFotoresistencia=100;
    }
    else
      contFotoresistencia--;
  }
}

void actualizarExcesoVel(float v)
{
  if(v>maxVel)
    maxVel=v;
}

int cuantificarVelocidad(float v)
{
  if(v<8)
    return 0;
  if(v<12)
    return 10;
  if(v<18)
    return 15;
  if(v<26)
    return 20;
  if(v<36)
    return 30;
  if(v<46)
    return 40;
  if(v<52)
    return 50;
  if(v<58)
    return 55;
  if(v<62)
    return 60;
  if(v<68)
    return 65;
  if(v<72)
    return 70;
  if(v<78)
    return 75;
  if(v<82)
    return 80;
  if(v<88)
    return 85;
  if(v<96)
    return 90;
  if(v<106)
    return 100;
  if(v<112)
    return 110;
  if(v<118)
    return 115;
  if(v<122)
    return 120;
  if(v<128)
    return 125;
  if(v<136)
    return 130;
  if(v<146)
    return 140;
  if(v<156)
    return 150;
  if(v<166)
    return 160;
  if(v<176)
    return 170;
  if(v<186)
    return 180;
  if(v<196)
    return 190;
  if(v<206)
    return 200;
  if(v<216)
    return 210;
  return 222;  
  
}

void GPS()
{
  while(GPSSerial.available()) 
  {
    codGPS = GPSSerial.read(); 
    if(gps.encode(codGPS)) 
    {
      if(gpsSincronizado==false)
      {
        calibrarGPS();
        gpsSincronizado = true;
        escribirLineaDeTrakingEnSD();
      }
      else
      {
        gps.f_get_position(&latitude, &longitude);
        gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
        hActual=hour;
        mActual=minute;
        if(filtroGPS()==true&&movimiento()==true)
        {
          escribirLineaDeTrakingEnSD();
          if(cuantificarVelocidad(gps.f_speed_kmph())>limVel)
          {
            digitalWrite(ledExcesoVelPin, HIGH);
            if(!flagExesoVel)
              Reproducir(excesoVel);
            flagExesoVel = true;
          }
          if(flagExesoVel)
            actualizarExcesoVel(gps.f_speed_kmph());
          if(flagExesoVel&&cuantificarVelocidad(gps.f_speed_kmph())<=limVel)
          {
            flagExesoVel = false;
            escribirEventoEnSD("ExcesoVelocidad",true);
            digitalWrite(ledExcesoVelPin, LOW);
            maxVel=-1;
          }
        }
        gps.stats(&chars, &sentences, &failed_checksum);
        
      }
    }
  }
}
void calibrarGPS()
{
  float a1=-1,a2=-1,a3=-1,p;
  do
  {
    while(GPSSerial.available()) 
    {
      codGPS = GPSSerial.read(); 
      if(gps.encode(codGPS))
      {
        gps.f_get_position(&latitude, &longitude);
        gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
        gps.stats(&chars, &sentences, &failed_checksum);
        //Serial.println(gps.f_altitude());
        if(a1==-1)
          a1=gps.f_altitude();
        else 
        {
          if(a2==-1)
            a2=gps.f_altitude();
          else
          {
            a3=gps.f_altitude();
            if((a3-((a1+a2+a3)/3))<10)
              gpsNoCalibrado=false;
            else
            {
              a1=a2;
              a2=a3;
            }
          }
        }
      }
    }
  }while(gpsNoCalibrado); 
  alturaOld = a3;
  latitudeOld = latitude;
  longitudeOld = longitude; 
  hActual=hour;
  mActual=minute;
}

boolean filtroGPS()
{
  if(gps.f_altitude()>10000.00)
    return false;
  if(alturaOld!=-1)
    return abs(alturaOld - gps.f_altitude()) < 10;
  return true;
}

boolean movimiento()
{
  if(abs(latitudeOld - latitude) > constanteVariacionLL || abs(longitudeOld - longitude) > constanteVariacionLL)
  {
    alturaOld = gps.f_altitude();
    latitudeOld = latitude;
    longitudeOld = longitude;
    return true;
  }
  return false;
}

int16_t cuantificarAcelerometro(int16_t a)
{
  if(a<1000)
    return 0;
  if(a<2000)
    return 1;
  if(a<4000)
    return 2;
  if(a<6000)
    return 3;
  if(a<8000)
    return 4;
  if(a<10000)
    return 5;
  if(a<12000)
    return 6;
  if(a<14000)
    return 7;
  return 8;
    
}

void Acelerometro()
{
  antes=millis();
  accel.getAcceleration(&ax2, &ay2, &az2);
  accel.getAcceleration(&ax, &ay, &az);
  despues=millis();
  transcurridos=despues-antes;
  rx=abs(ax2-ax)/transcurridos;
  ry=abs(ay2-ay)/transcurridos;
  rz=abs(az2-az)/transcurridos;


  if(movimiento()&&(cuantificarAcelerometro(rx)>limiteAc||cuantificarAcelerometro(ry)>limiteAc))
  {
    digitalWrite(ledGY88, true);
    int16_t m=Maximo(cuantificarAcelerometro(rx),cuantificarAcelerometro(ry));
    String auxs="AceleracionBruzca(";
    auxs.concat(m);
    auxs.concat(")");
    escribirEventoEnSD(auxs,false);
    Reproducir(aceleracionBruzca);
  }
  else
  {
    digitalWrite(ledGY88, false);
  }
}

void escribirLogEnSD(String evento,boolean accionValida)
{
  Archivo = SD.open(fileNameLog, FILE_WRITE);
  if (Archivo) {  
    Archivo.print(evento);
    if(gpsSincronizado==true)
    {
      Archivo.print("#"); Archivo.print(day, DEC); Archivo.print("/"); Archivo.print(month, DEC); Archivo.print("/"); Archivo.print(year);
      Archivo.print("#"); Archivo.print(hour, DEC); Archivo.print(":"); Archivo.print(minute, DEC); Archivo.print(":"); Archivo.print(second, DEC); Archivo.print("."); Archivo.print(hundredths, DEC);
      
    }
    if(accionValida)
      Archivo.print("#Accion Valida");
    else
      Archivo.print("#Accion Invalida");
    Archivo.println();
    Archivo.close();  
  }
  else {
    return; //problemas abriendo dtxt, se muestra por pantalla.
  }
}
void escribirEventoEnSD(String evento,boolean exesoVel)
{
  
  Archivo = SD.open(fileNameEvento, FILE_WRITE);
  if (Archivo) {  
    Archivo.print(evento);
    if(gpsSincronizado==true)
    {
      Archivo.print("#"); Archivo.print(day, DEC); Archivo.print("/"); Archivo.print(month, DEC); Archivo.print("/"); Archivo.print(year);
      Archivo.print("#"); Archivo.print(hour, DEC); Archivo.print(":"); Archivo.print(minute, DEC); Archivo.print(":"); Archivo.print(second, DEC); Archivo.print("."); Archivo.print(hundredths, DEC);
      
      if(exesoVel==true)
      {
        Archivo.print("#"); Archivo.print(gps.satellites()); Archivo.print("Satelites");   
        Archivo.print("#"); Archivo.print(cuantificarVelocidad(maxVel)); Archivo.print("kmph"); 
      }
    }
    Archivo.println();
    Archivo.close();
    
  }
  else {
    return; //problemas abriendo dtxt, se muestra por pantalla.
  }
}
void escribirLineaDeTrakingEnSD()
{
  Archivo = SD.open(fileNameTraking, FILE_WRITE);
  if (Archivo) {
    if(gpsSincronizado==true)
    {
      
      Archivo.print("#"); Archivo.print(latitude,5); Archivo.print(","); Archivo.print(longitude,5);
      Archivo.print("#"); Archivo.print(day, DEC); Archivo.print("/"); Archivo.print(month, DEC); Archivo.print("/"); Archivo.print(year);
      Archivo.print("#"); Archivo.print(hour, DEC); Archivo.print(":"); Archivo.print(minute, DEC); Archivo.print(":"); Archivo.print(second, DEC); Archivo.print("."); Archivo.print(hundredths, DEC);
      Archivo.print("#"); Archivo.print(gps.f_altitude()); Archivo.print("metros |"); Archivo.print(gps.f_course()); Archivo.print("grados |"); Archivo.print(gps.satellites()); Archivo.print("Satelites");   
      Archivo.print("#"); Archivo.print(cuantificarVelocidad(gps.f_speed_kmph())); Archivo.print("kmph"); 
      Archivo.println();
    }
    Archivo.close();
  }
  else {
    return; //problemas abriendo dtxt, se muestra por pantalla.
  }
}

void actualizarParametrosEnSD()
{
  if(SD.exists(fileNameParametros))
    SD.remove(fileNameParametros);
  Archivo = SD.open(fileNameParametros, FILE_WRITE);
  if (Archivo) {
      Archivo.print(limVel);
      Archivo.print("#"); Archivo.print(limiteAc); 
      Archivo.print("*"); Archivo.print(sensorMQ7VoltajeLimite); 
      Archivo.print("|"); Archivo.print(sensorMQ7VoltajeLimiteInf); 
      Archivo.println();
    Archivo.close();
  }
  else {
    return; //problemas abriendo dtxt, se muestra por pantalla.
  }
}

int posCarac(String s,const char op)
{
  int pos=0;
  for(int i=0;i<s.length();i++)
    if(s[i]==op)
      pos=i;
  return pos;
}

void actualizarParametrosDesdeSD()
{
  Archivo = SD.open(fileNameParametros, FILE_READ);
  String parametros[4];
  String linea="";
  int found,found2;
  char caracter;
  if (Archivo) {
    while (Archivo.available()) {  
      caracter=Archivo.read();
      linea=linea+caracter;
    }
    if(linea!="")
    {
      found = posCarac(linea,'#');
      parametros[0]="";
      for(int i=0;i<found;i++)
        parametros[0]+=linea[i];
      found2 = posCarac(linea,'*');
      parametros[1]="";
      for(int i=found+1;i<found2;i++)
        parametros[1]+=linea[i];
      found = posCarac(linea,'|');
      parametros[2]="";
      for(int i=found2+1;i<found;i++)
        parametros[2]+=linea[i];
      parametros[3]="";
      for(int i=found+1;i<linea.length();i++)
        parametros[3]+=linea[i];
        
      Archivo.close();
  
      limVel=parametros[0].toInt();
      limiteAc=parametros[1].toInt();
      sensorMQ7VoltajeLimite=parametros[2].toFloat();
      sensorMQ7VoltajeLimiteInf=parametros[3].toFloat();      
    }
  }
  else {
    return; //problemas abriendo dtxt, se muestra por pantalla.
  }
}

void Bluetooth()
{
  String linea="",op="",operando="";
  accionValida=false;
  
  Serial.flush();
  HC06.flush();
  while (HC06.available()) {      
    char data=HC06.read();
    if(data != ' ')
    {
      linea=linea+data;
      flagBT=true;
    }
    Serial.flush();
    HC06.flush();
  }
  if(flagBT)
  {
    Serial.print(linea);
    found = posCarac(linea,'#');
    for(int i=0;i<found;i++)
      op+=linea[i];
    for(int i=found+1;i<linea.length();i++)
      operando+=linea[i];
  
    if(op.equals("setVel"))
    {
      limVel = operando.toInt();
      actualizarParametrosEnSD();
      accionValida=true;
    }
    else if(op.equals("setMQ7"))
    {
      sensorMQ7VoltajeLimite = operando.toFloat();
      sensorMQ7VoltajeLimiteInf = operando.toFloat()-0.20;
      actualizarParametrosEnSD();
      accionValida=true;
    }
    else if(op.equals("setModoNoc"))
    {
      if(operando.equals("true"))
      {
        hardCodModoNocturno=true;
        accionValida=true;
      }
      else if(operando.equals("false"))
      {
        hardCodModoNocturno=false;
        accionValida=true;
      }
    }
    else if(op.equals("clear"))
    {
      if(operando.equals("all"))
      {
        if(SD.exists(fileNameTraking))
          SD.remove(fileNameTraking);
        if(SD.exists(fileNameEvento))
          SD.remove(fileNameEvento);
        if(gpsSincronizado)
          escribirLineaDeTrakingEnSD();
        accionValida=true;
      }
      else if(operando.equals("traking"))
      {
        if(SD.exists(fileNameTraking))
          SD.remove(fileNameTraking);
        if(gpsSincronizado)
          escribirLineaDeTrakingEnSD();
        accionValida=true;
      }
      else if(operando.equals("evento"))
      {
        if(SD.exists(fileNameEvento))
          SD.remove(fileNameEvento);
        accionValida=true;
      }
    }
    Serial.flush();
    HC06.flush();
    if(accionValida)
      HC06.print('0');
    else
      HC06.print('1');
    escribirLogEnSD(linea,accionValida);
    flagBT=false;
    Serial.flush();
    HC06.flush();
  }
}

void loop() {
  //Serial.println("1");
  if(a)
  {
    Serial.println("Hola");
    a = false;
  }  
  // GPS();
  // Fotoresistencia();
  // MQ7();
  // Acelerometro();
  Bluetooth(); 
  // digitalWrite(ledONPin, true);
}
