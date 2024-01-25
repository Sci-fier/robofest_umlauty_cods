// подключение библиотек
#include <SPI.h> // библиотека, позволяющая Arduino Nano общаться с периферийными устройствами
#include <RF24.h> // библиотека, необходимая для взаиодействия с радиомодулем NRF24L01

// присваивание каждому порту кодовое название. Это необходимо для облегчения понимания кода 
#define PIN_VRX A5 // VELOCITY X
#define PIN_VRY A4 // VELOCITY Y
#define PIN_BUTTON 8 // PIN BUTTON
#define PIN_SPL A6 // SPEED LEFT
#define PIN_SPR A7 // SPEED RIGHT
#define PIN_ARM A3 // ARM POS
#define PIN_CC A2 // CLAW CLOSE
#define PIN_CO A1 // CLAW OPEN

#define DEBUG

// первичная настройка радиомодуля
RF24 radio(9, 10); // порты D9, D10: CE (SS - выбор ведомого)
const uint32_t pipe = 111156789; // адрес рабочей трубы

//объявление переменных
int serv_y = 117; // переменная, хранящая позицию камеры по оси y
int serv_x = 16;  // переменная, хранящая позицию камеры по оси x
int b;           // буферная (дополнительная) переменная
int fix_poz_code = 0; // переменная, хранящая сокращенный код позиции камеры 
bool reserve_battery = 0; // переменная, отвечающая за резервное питание
int past_camera_poz = 0; // переменная, сохраняющая прошлую позицию камеры
int past_reserve_battery = 0; // переменная, сохраняющая состояние кнопки резервного питания
long int data[6]; // массив передачи

int calc(int x){
  x = map(x, 345, 908, 0, 1023);
  if(x < 0){x = 0;}
  if(x > 1023){x = 1023;}
  return(x);
}

void setup () {
  Serial.begin(9600);
  radio.begin();                // инициализация радиомодуля
  delay(2000);                  // задержка необходима для корректной работы программы
  radio.setDataRate(RF24_1MBPS); // скорость обмена данными RF24_1MBPS (1 МБ/с) или RF24_2MBPS (2 МБ/с)
  radio.setCRCLength(RF24_CRC_16); // размер контрольной суммы 8 bit или 16 bit
  radio.setPALevel(RF24_PA_MAX); // уровень питания усилителя RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
  radio.setChannel(0x6f);         // установка канала
  radio.setAutoAck(false);       // выключение автоответа
  radio.powerUp();               // включение или пониженное потребление powerDown - powerUp
  radio.stopListening();  // радиоэфир не слушаем, только передача
  radio.openWritingPipe(pipe);   // открыть трубу на отправку
}

void loop () {
  // считывание сигнала с позицией камеры по оси y
  b = 0-(map(analogRead(PIN_VRY),0,1024,-5,6));
  // преобразование сигнала заключается в игнорировании достаточно маленького наклона джойстика
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
  // программно изменяем минимальное и максимальное значение наклона по оси Y (от 15 до 180)
  if(serv_y > 180){
  serv_y = 180;
  }
  if(serv_y < 15){
  serv_y = 15;
  }
  // считывание и обработка сигнала с позицией камеры по оси x
  b = 0-(map(analogRead(PIN_VRX),0,1024,-5,6));
  // преобразование сигнала заключается в игнорировании достаточно маленького наклона джойстика
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
  // программно изменяем минимальное и максимальное значение наклона по оси X(от 0 до 180)
  if(serv_x < 0){
  serv_x = 0;
  }
  if(serv_x > 180){
  serv_x = 180;
  }
  //обработка фиксированных позиций камеры
  b = digitalRead(PIN_BUTTON);
  Serial.print(b);
  Serial.print(" ");
  // взаимодействие с кнопкой джойстика
  if(b == 0){
    // проверка на зажатие кнопки джойстика
    if(past_camera_poz == 0){
      // выбор фиксированной позиции
      if(fix_poz_code == 1){
        // на захват
        serv_x = 95;
        serv_y = 107;
        fix_poz_code = 0;
      }
      else{
        // на движение
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
  // 5 - положение мотора в подьемном механизме от 0 до 1023
  // заполнение массива отправки
  data[0] = serv_x; // положение камеры по оси х
  data[1] = serv_y; // положение камеры по оси у
  b = digitalRead(PIN_CC) + 1;
  data[4] = b - digitalRead(PIN_CO); // подсчет скорости и направления сжатия клешни
  data[5] = calc(analogRead(PIN_ARM)); // подсчет угла манипулятора
  data[2] = map(analogRead(PIN_SPL), 706, 1023, 511, 0); // подсчет скорости и направления  вращения моторов левой стороны
  data[3] = map(analogRead(PIN_SPR), 0, 1023, 0, 511); // подсчет скорости и направления  вращения моторов правой стороны
  
  radio.write(data, sizeof(data));
#ifdef DEBUG
  for(int i=0; i < 6; ++i){
    Serial.print(data[i]);
    Serial.print(" ");
  }
#endif
  Serial.println("");
  delay (5);
}
