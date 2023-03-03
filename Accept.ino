#include <Servo.h>
#include <SPI.h>
#include "NRFLite.h"
#include "nRF24L01.h"
#include "RF24.h"
#include <Multiservo.h>
#include "ServoSmooth.h"

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

RF24 radio(7, 8); // порты D9, D10: CSN CE

const uint32_t pipe = 111156789; // адрес рабочей трубы;
long int data[7];
ServoSmooth servox;
ServoSmooth servoy;
Multiservo servo_arm;
Multiservo servo_claw;

void robot_move(int SL, int SR){
  if(SL>=256){
    digitalWrite(PIN_IN1L, LOW);
    digitalWrite(PIN_IN2L, HIGH);
    analogWrite(PIN_ENL, SL-256);
  }
  else{
    digitalWrite(PIN_IN2L, LOW);
    digitalWrite(PIN_IN1L, HIGH);
    analogWrite(PIN_ENL, 255-SL);
  }
  if(SR>=256){
    digitalWrite(PIN_IN1R, LOW);
    digitalWrite(PIN_IN2R, HIGH);
    analogWrite(PIN_ENR, SR-256);
  }
  else{
    digitalWrite(PIN_IN2R, LOW);
    digitalWrite(PIN_IN1R, HIGH);
    analogWrite(PIN_ENR, 255-SR);
  }
}

void setup() {
  Serial.begin(9600);

  radio.begin();  // инициализация
  delay(2000);
  radio.setDataRate(RF24_1MBPS); // скорость обмена данными RF24_1MBPS или RF24_2MBPS
  radio.setCRCLength(RF24_CRC_16); // размер контрольной суммы 8 bit или 16 bit
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(0x6f);         // установка канала
  radio.setAutoAck(false);       // автоответ
  radio.powerUp();
  radio.openReadingPipe(1, pipe); // открыть трубу на приём
  radio.startListening();        // приём
  servox.attach(PIN_SX);
  servoy.attach(PIN_SY);
  servo_arm.attach(PIN_SA, 1000, 2000);
  servo_claw.attach(PIN_SC, 1000, 2000);
  servox.setAccel(1.0);
  servoy.setAccel(1.0);
}

void loop() {
  if (radio.available()){
    radio.read(data, sizeof(data));
    // data[]
    // 0 - положение камеры х
    // 1 - положение камеры у
    // 2 - скорость левовых колес в формате от 0 до 511
    // 3 - скорость правых колес в формате от 0 до 511
    // 4 - скорость врашения мотора в клешне от 0 до 2
    // 5 - скорость врашения мотора в подьемном механизме от 0 до 2
    // 6 - показывает необходимость включения резервного питания от 0 до 1
    servox.setTargetDeg(data[0]);
    servox.tick();
    servoy.setTargetDeg(data[1]);
    servoy.tick();
    robot_move(data[2], data[3]);
    servo_claw.write(data[4] * 90);
    servo_arm.write(data[5] * 90);
    if(data[6] == 1){
      analogWrite(PIN_RELAY, 0); 
    }
    else{
      analogWrite(PIN_RELAY, 255); 
    }
    
    for(int i=0; i < 7; ++i){
      Serial.print(data[i]);
      Serial.print(" ");
    }
    Serial.println("");
  }

  delay(40);
}