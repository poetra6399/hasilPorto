#include <PCD8544.h>
#include <SPI.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
SoftwareSerial s(10, 11);
#include <DS3231.h> //mengincludekan library DS3231
DS3231  rtc(SDA, SCL); // inisialisasi penggunaan i2c
PCD8544 lcd;
//library suhu
#include <OneWire.h>
#include <DallasTemperature.h>
#define sensor_ds18b20 45 //pada pin 45 untuk sensor ds18b20
OneWire oneWire(sensor_ds18b20);
DallasTemperature sensors(&oneWire);
//sensor pH
#include "DFRobot_PH.h" //library yang diinstal seperti instruksi diatas
#define sensor_ph A0 //pembacaan sensor ph menggunakan ADC
unsigned int volt = 50;
//lib SD card
#include <SD.h>
File myFile; //deklarasi variabel untuk save di sd
//sensor kekeruhan
#include<Wire.h>
#define Addr 0x4A
int luminance = 0; //untuk deklarasi variabel sd luminance
float suhunilai, phnilai, keruhnilai;
float tegangan, nilaiph, suhu;
DFRobot_PH ph;
unsigned int data[2];
//deklarasi keypad
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[] = {29, 31, 33, 35};
byte colPins[] = {37, 39, 41, 43};

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
unsigned long waktuCount ;
//buzzer
#define bunyi 52
// menu
int8_t menuLevel = 0;
int8_t menuLevelSebelumnya = -1;
int8_t menuLevelSekarang = -1;
void setup() {
  s.begin(115200);
  lcd.begin(84, 48);
  lcd.setContrast(35);
  Serial.begin(115200);
  rtc.begin();
  sensors.begin();//suhu
  ph.begin(); //mulai untuk pembacaan sensor ph meter
  //waktuCount = millis();
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print("Initializing SD card...");
  if (!SD.begin(53)) {
    Serial.println("initialization failed!");
    while (1);
  }
  //Serial.println("initialization done.");
  Wire.beginTransmission(Addr);
  Wire.write(0x02);
  Wire.write(0x40);
  Wire.endTransmission();


}
void loop() {
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  char key = keypad.getKey();
  //menuLevel = 0;
  //if (key==NO_KEY){
  //lcd.clear();
  lcd.setCursor(10, 0);
  lcd.print(rtc.getDOWStr(2));
  Serial.println(rtc.getDOWStr(2));
  lcd.setCursor(10, 1);
  lcd.print(rtc.getDateStr());
  Serial.println(rtc.getDateStr());
  lcd.setCursor(10, 2);
  lcd.print(rtc.getTimeStr());
  Serial.println(rtc.getTimeStr());
  //baca pH
  Suhu_dan_pH();
  keruh();
  temperatur();
  sd();
  root["data1"] = suhunilai;
  Serial.println(suhunilai);
  root["data2"] = phnilai;
  Serial.println(phnilai);
  root["data3"] = keruhnilai;
  Serial.println(keruhnilai);
  if (s.available() > 0)
  {
    root.printTo(s);
  }
  //}


  //else if (key =! NO_KEY){
  //char key = keypad.getKey();
  switch (key)
  {
    case 'A': //atur ph
      //waktuCount = millis();
      A();
      break;

    case 'B': //atur suhu
      B();
      break;

    case 'C': //atur keruh
      C();
      break;

    default:
      break;
  }
}
//}

//menu keypad
void A() {
  waktuCount = millis();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ket. Suhu");
  lcd.setCursor(0, 1);
  lcd.print("Suhu = ");
  lcd.print(suhu);
  lcd.setCursor(0, 2);
  if (sensors.getTempCByIndex(00) < 24) {
    lcd.print("Suhu Dingin");
    lcd.println("Tambahkan air hangat");
  }
  else if (sensors.getTempCByIndex(00) > 28) {
    lcd.print("Suhu Hangat");
    lcd.println(" ");
    lcd.println("Tambahkan air dingin");
  }
  else if (24 < sensors.getTempCByIndex(00) < 28) {
    lcd.print("Suhu Normal");
  }
  delay(9000);
  lcd.clear();
}

void B() {
  waktuCount = millis();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ket. pH");
  lcd.setCursor(0, 1);
  lcd.print("pH = ");
  lcd.print(nilaiph);
  lcd.setCursor(0, 2);
  if (nilaiph < 6.0) {
    lcd.print("pH Asam");
    lcd.println("   ");
    lcd.println("Tambahkan larutan basa");
  }
  else if (nilaiph > 8.0) {
    lcd.print("pH Basa");
    lcd.println(" ");
    lcd.println("Tambahkan larutan asam");
  }
  else if (6.0 < nilaiph < 8.0) {
    lcd.print("pH Normal");
  }
  delay(9000);
  lcd.clear();
}

void C() {
  waktuCount = millis();
  //ambil data keruh
  int exponent = (data[0] & 0xF0) >> 4;
  int mantissa = ((data[0] & 0x0F) << 4) | (data[1] & 0x0F);
  float luminance = pow(2, exponent) * mantissa * 0.045;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ket. Keruh");
  lcd.setCursor(0, 1);
  lcd.print("TDS = ");
  lcd.print(luminance);
  lcd.setCursor(0, 2);
  if (luminance < 93.00 ) {
    lcd.print("Air Keruh");
    lcd.println("Kuras Kolam");
    }
    else if (luminance > 94.00) {
    lcd.print("Keruh Normal");
    }
  delay(9000);
  lcd.clear();
}

//All prosedur
//void suhu ds
void temperatur() {
  //baca suhu
  sensors.requestTemperatures();
  suhu = sensors.getTempCByIndex(00);
  suhunilai = suhu;
  lcd.setCursor(10, 3);
  lcd.print("Suhu: ");
  lcd.setCursor(40, 3);
  lcd.print(suhu);
  if(suhu < 24.0){
    digitalWrite(bunyi, HIGH);
    delay(600);
    digitalWrite(bunyi, LOW);
    delay(600);
  }
  else if(suhu > 28.0){
    digitalWrite(bunyi,HIGH);
    delay(1000);
    digitalWrite(bunyi,LOW);
    delay(1000);
  }
  else if(24.0 < suhu < 28.0){
    digitalWrite(bunyi, LOW);
  }
}


void Suhu_dan_pH() {
  sensors.requestTemperatures();
  static unsigned long timepoint = millis();
  if (millis() - timepoint > 1000U) {            //time interval: 1s
    timepoint = millis();
    tegangan = analogRead(sensor_ph) * 1024.0 * 25.0 / 9000; //mengubah tegangan analog menjadi digital dan menjadi tegangan
    suhu = sensors.getTempCByIndex(00);

    nilaiph = ph.readPH(tegangan, volt); //konversi tegangan menjadi ph meter dengan kompensasi suhu
    phnilai = nilaiph;
  }
  ph.calibration(tegangan, volt);
  lcd.setCursor(10, 4);
  lcd.print("pH :");
  lcd.setCursor(40, 4);
  lcd.print(nilaiph); //nilai pembacaan ph meter
  if(nilaiph < 6.0){
    digitalWrite(bunyi, HIGH);
    delay(500);
    digitalWrite(bunyi, LOW);
    delay(500);
  }
  else if(nilaiph > 8.0){
    digitalWrite(bunyi,HIGH);
    delay(1000);
    digitalWrite(bunyi,LOW);
    delay(1000);
  }
  else if(6.0 < nilaiph <8.0){
    digitalWrite(bunyi, LOW);
  }
}

//VOID Kekeruhan
void keruh() {

  Wire.beginTransmission(Addr);
  Wire.write(0x03);
  Wire.endTransmission();
  // Request 2 bytes of data
  Wire.requestFrom(Addr, 2);
  // Read 2 bytes of data luminance msb, luminance lsb
  if (Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
  }
  // Convert the data to lux
  int exponent = (data[0] & 0xF0) >> 4;
  int mantissa = ((data[0] & 0x0F) << 4) | (data[1] & 0x0F);
  float luminance = pow(2, exponent) * mantissa * 0.045;
  lcd.setCursor(10, 5);
  lcd.print("TDS:");
  lcd.setCursor(40, 5);
  lcd.print (luminance);
  keruhnilai = luminance;
  if(luminance < 93.0){
    digitalWrite(bunyi, HIGH);
    delay(700);
    digitalWrite(bunyi, LOW);
    delay(700);
  }
  else if(luminance > 94.0){
    digitalWrite(bunyi, LOW);
  }
}

void sd() {
  //temperatur();
  //Suhu_dan_pH();
  //keruh();
  //ambil data keruh
  int exponent = (data[0] & 0xF0) >> 4;
  int mantissa = ((data[0] & 0x0F) << 4) | (data[1] & 0x0F);
  float luminance = pow(2, exponent) * mantissa * 0.045;
  //mulai catat
  myFile = SD.open("test3.txt", FILE_WRITE);
  static unsigned long timepoint = millis();
  if (millis() - timepoint > 5000U) {            //time interval: 5s, 300000 = 5 menit, 60000 = 1 menit, 3600000 = 1 jam
    timepoint = millis();
    if (myFile) {
     Serial.println("Writing to test3.txt");
      myFile.print(rtc.getDOWStr(2));
      myFile.print(" ");
      myFile.print(rtc.getDateStr());
      myFile.print(" ");
      myFile.print(rtc.getTimeStr());
      myFile.print(",");
      myFile.print("");
      myFile.print(" Suhu : ");
      myFile.print(suhu);
      myFile.print(" C");
      myFile.print(",");
      myFile.print("");
      myFile.print(" pH : ");
      myFile.print(nilaiph);
      myFile.print(",");
      myFile.print("");
      myFile.print(" TDS : ");
      myFile.print(luminance);
      myFile.print(" lux");
      myFile.println("");
      myFile.close();// close the file:
      //Serial.println("done.")
      //waktu_sebelumnya = waktu_sekarang;;
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening test.txt");
    }
  }
}

