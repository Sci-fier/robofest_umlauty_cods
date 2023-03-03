//подключение библиотек
#include <Servo.h> //библиотека для управления сервоприводами
#include <SPI.h> //библиотека, позволяющая Arduino Nano общаться с периферийными устройствами
#include "RF24.h" //библиотека, необходимая для взаиодействия с радиомодулем NRF24L01
#include <Multiservo.h> //библиотека для движения сервоприводов при помощи Multi Shield
#include "ServoSmooth.h" //библиотека для плавного движения сервоприводов

//присваивание каждому порту кодовое название. Это необходимо для облегчения понимания кода 
#define PIN_SX 10 // SERVO X
#define PIN_SY 9 // SERVO Y
#define PIN_SA 1 // SERVO ARM
#define PIN_SC 0 // SERVO CLAW
#define PIN_ENL 6 // ENABLE LEFT
#define PIN_IN1L 17 // INPUT 1 LEFT
#define PIN_IN2L 16 // INPUT 2 LEFT
#define PIN_ENR 5 // ENABLE RIGHT
#define PIN_IN1R 14 // INPUT 1 RIGHT
#define PIN_IN2R 15 // INPUT 2 RIGHT
#define PIN_RELAY 3 // RELAY OF RESERVE POWER

//первичная настройка радиомодуля
RF24 radio(7, 8); // порты D9, D10: CE (SS - выбор ведомого)
const uint32_t pipe = 111156789; // адрес рабочей трубы
long int data[7]; // массив передачи
ServoSmooth servox; // создание переменной, отвечающей за плавное движение по х класса ServoSmooth
ServoSmooth servoy; // создание переменной, отвечающей за плавное движение по у класса ServoSmooth
Multiservo servo_arm; // создание переменной, отвечающей за изменение угла наклона манипулятора, класса Multiservo
Multiservo servo_claw; // создание переменной, отвечающей за управление клешнёй манипулятора, класса Multiservo

// создание функции движения
void robot_move(int SL, int SR){
  // из-за особенности управления диапазон значений от 0 до 512 вместо от -256 до 256
  // если SL >= 256 - движение вперёд или стояние на месте (при SL == 256)
  if(SL>=256){
    // каждый мотор принимает два сигнала на вращение. 
    //Важно помнить, что если вращение мотора будет установлен уровень High обе стороны, то произойдёт короткое замыкание.
    digitalWrite(PIN_IN1L, LOW); // вращение левого мотора назад - выключить (именно поэтому сначала ставим Low сигнал)
    digitalWrite(PIN_IN2L, HIGH); // вращение левого мотора вперёд - включить
    analogWrite(PIN_ENL, SL-256); // установление скорости вращения при помощи Шим-сигнала
  }
  //иначе - движение назад 
  else{
    digitalWrite(PIN_IN2L, LOW); // вращение левого мотора вперёд - выключить
    digitalWrite(PIN_IN1L, HIGH); // вращение левого мотора назад - включить
    analogWrite(PIN_ENL, 255-SL); // установление скорости вращения при помощи Шим-сигнала
  }
  if(SR>=256){
    digitalWrite(PIN_IN1R, LOW); // вращение правого мотора вперёд - включить
    digitalWrite(PIN_IN2R, HIGH); // вращение правого мотора назад - выключить
    analogWrite(PIN_ENR, SR-256); // установление скорости вращения при помощи Шим-сигнала
  }
  else{
    digitalWrite(PIN_IN2R, LOW); // вращение правого мотора вперёд - выключить
    digitalWrite(PIN_IN1R, HIGH); // вращение правого мотора назад - включить
    analogWrite(PIN_ENR, 255-SR); // установление скорости вращения при помощи Шим-сигнала
  }
}

void setup() {
  Serial.begin(9600);
  radio.begin();  // инициализация радиомодуля
  delay(2000); // задержка необходима для корректной работы программы
  radio.setDataRate(RF24_1MBPS); // скорость обмена данными RF24_1MBPS (1 МБ/с) или RF24_2MBPS (2 МБ/с)
  radio.setCRCLength(RF24_CRC_16); // размер контрольной суммы 8 bit или 16 bit
  radio.setPALevel(RF24_PA_MAX); // уровень питания усилителя RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
  radio.setChannel(0x6f); // установка канала
  radio.setAutoAck(false); // выключение автоответа
  radio.powerUp();  // включение или пониженное потребление powerDown - powerUp
  radio.openReadingPipe(1, pipe); // открыть трубу на приём
  radio.startListening();  // включение режима приёма сигнала
  servox.attach(PIN_SX); // резервирование порта PIN_SX под servox
  servoy.attach(PIN_SY); // резервирование порта PIN_SY под servoy
  // моторы для манипулятора принимают другую длину импульса (1000 - минимальная длина импульса, 2000 - максимальная длина импульса)
  servo_arm.attach(PIN_SA, 1000, 2000); // резервирование порта PIN_SA под servo_arm 
  servo_claw.attach(PIN_SC, 1000, 2000); // резервирование порта PIN_SA под servo_claw
  servox.setAccel(1.0); //задаём максимальное ускорение для сервопривода оси Х
  servoy.setAccel(1.0); //задаём максимальное ускорение для сервопривода оси Y
}

void loop() {
  if (radio.available()){
    radio.read(data, sizeof(data)); // слушаем радиоканал и получаем data
    // data[]
    // 0 - положение камеры х
    // 1 - положение камеры у
    // 2 - скорость левовых колес в формате от 0 до 511
    // 3 - скорость правых колес в формате от 0 до 511
    // 4 - скорость врашения мотора в клешне от 0 до 2
    // 5 - скорость врашения мотора в подьемном механизме от 0 до 2
    // 6 - показывает необходимость включения резервного питания от 0 до 1
    servox.setTargetDeg(data[0]); // задаем целевой угол (угол в градусах) для оси Y
    servox.tick(); // начало движения камеры по оси X
    servoy.setTargetDeg(data[1]); // задаем целевой угол (угол в градусах) для оси Х
    servoy.tick(); // начало движения камеры по оси Y
    robot_move(data[2], data[3]); // движение робота на координаты data[2], data[3]
    servo_claw.write(data[4] * 90); // задаём вращение клешни
    servo_arm.write(data[5] * 90); // задаём вращение манипулятора
    // проверяем сигнал на включение резервного питания
    if(data[6] == 1){
      analogWrite(PIN_RELAY, 0); // включаем резервное питание (перемещаем реле в позицию 0)
    }
    else{
      analogWrite(PIN_RELAY, 255); // выключаем резервное питание (перемещаем реле в позицию 255)
    }
    //проверка выходных значений
    for(int i=0; i < 7; ++i){
      Serial.print(data[i]);
      Serial.print(" ");
    }
    Serial.println("");
  }

  delay(40);
}
