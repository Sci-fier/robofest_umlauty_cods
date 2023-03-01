#include <SPI.h>
#include <RF24.h>

#define PIN_VRX A4 // VELOCITY X
#define PIN_VRY A5 // VELOCITY Y
#define PIN_BUTTON 7 // PIN BUTTON
#define PIN_SPL A6 // SPEED LEFT
#define PIN_SPR A7 // SPEED RIGHT
#define PIN_CC 5 // CLAW CLOSE
#define PIN_CO 6 // CLAW OPEN
#define PIN_AD 3 // ARM DOWN
#define PIN_AU 4 // ARM UP
#define PIN_NITRO 2 // BUTTON THAT SWITCHING RESERVE POWER
#define PIN_LED 8 // LED OF RESERVE POWER



RF24 radio(9, 10); // порты D9, D10: CSN CE
const uint32_t pipe = 111156789; // адрес рабочей трубы;

int serv_y = 90; //переменная, хранящая позицию камеры по оси y
int serv_x = 0;  //переменная, хранящая позицию камеры по оси x
int b;           //буферная (дополнительная) переменная
int fix_poz_code = 0; //переменная, хранящая сокращенный код позиции камеры 
bool reserve_battery = 0; //переменная, отвечающая за резервное питание
int past_camera_poz = 0; //переменная, сохраняющая прошлую позицию камеры
int past_reserve_battery = 0; //переменная, сохраняющая состояние кнопки резервного питания
long int data[7];

void setup () {
  Serial.begin(9600);
  radio.begin();                // инициализация
  delay(2000);
  radio.setDataRate(RF24_1MBPS); // скорость обмена данными RF24_1MBPS или RF24_2MBPS
  radio.setCRCLength(RF24_CRC_16); // размер контрольной суммы 8 bit или 16 bit
  radio.setPALevel(RF24_PA_MAX); // уровень питания усилителя RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
  radio.setChannel(0x6f);         // установка канала
  radio.setAutoAck(false);       // автоответ
  radio.powerUp();               // включение или пониженное потребление powerDown - powerUp
  radio.stopListening();  //радиоэфир не слушаем, только передача
  radio.openWritingPipe(pipe);   // открыть трубу на отправку
}

void loop () {
  // считывание и обработка сигнала с позицией камеры по оси y
  b = 0-(map(analogRead(PIN_VRY),0,1024,-5,6));
  if(b<=1 and b>=-1){
    b = 0;
  }
  else{
    if(b>1){
      b = b-1;
    }
    else{
      if(b<1){
        b = b+1;
      }
    }
  }
  serv_y = serv_y + b;
  if(serv_y > 180){
  serv_y = 180;
  }
  if(serv_y < 15){
  serv_y = 15;
  }
  // считывание и обработка сигнала с позицией камеры по оси x
  b = 0-(map(analogRead(PIN_VRX),0,1024,-5,6));
  if(b<=1 and b>=-1){
    b = 0;
  }
  else{
    if(b>1){
      b = b-1;
    }
    else{
      if(b<1){
        b = b+1;
      }
    }
  }
  serv_x = serv_x + b;
  if(serv_x > 180){
  serv_x = 180;
  }
  if(serv_x < 0){
  serv_x = 0;
  }
  b = digitalRead(PIN_BUTTON);
  
  Serial.print(b);
  Serial.print(" ");
  if(b == 1){
    if(past_camera_poz == 0){
      if(fix_poz_code == 1){
        serv_x = 95;
        serv_y = 107;
        fix_poz_code = 0;
      }
      else{
        serv_x = 16;
        serv_y = 117;
        fix_poz_code = 1;
      }
    }
    past_camera_poz = 1;
  }
  else{
    past_camera_poz = 0;
  }
  // data[]
  // 0 - положение камеры х от 0 до 180
  // 1 - положение камеры у от 0 до 180
  // 2 - скорость левых колес в формате от 0 до 511
  // 3 - скорость правых колес в формате от 0 до 511
  // 4 - скорость врашения мотора в клешне от 0 до 2
  // 5 - скорость врашения мотора в подьемном механизме от 0 до 2
  // 6 - включение резервного питания ( 0 - не включать, 1 - включать )
  data[0] = serv_x;
  data[1] = serv_y;
  b = digitalRead(PIN_CC) + 1;
  data[4] = b - digitalRead(PIN_CO);
  b = digitalRead(PIN_AU) + 1;
  data[5] = b - digitalRead(PIN_AD);
  data[2] = map(analogRead(PIN_SPL), 0, 1023, 0, 511);
  data[3] = map(analogRead(PIN_SPR), 0, 1023, 0, 511);
  
  b = digitalRead(PIN_NITRO);
  Serial.print(b);
  Serial.print(" ");
  if(b == 1){
    if(past_reserve_battery == 0){
      if(reserve_battery == 1){
        reserve_battery = 0;
      }
      else{
        reserve_battery = 1;
      }
    }
    past_reserve_battery = 1;
  }
  else{
    past_reserve_battery = 0;
  }
  data[6] = reserve_battery;
  digitalWrite(PIN_LED, data[6]);
  radio.write(data, sizeof(data));
  for(int i=0; i < 7; ++i){
    Serial.print(data[i]);
    Serial.print(" ");
  }
  Serial.println("");
  delay (40);
}