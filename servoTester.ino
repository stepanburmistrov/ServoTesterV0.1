#include <Servo.h>  // Подключаем библиотеку Servo для управления сервоприводами

Servo part0;  // Создаем объекты сервоприводов
Servo part1;
Servo part2;
Servo part3;  // Создаем объекты сервоприводов
Servo part4;
#define BUTTON_PIN 10
#define DISP_CLK 9
#define DISP_DIO 8
#include "GyverTM1637.h"
GyverTM1637 disp(DISP_CLK, DISP_DIO);
int activeServo = 1;
int activeAngle = 0;
uint32_t dispTimer = 0;

Servo servos[] = { part0, part1, part2, part3, part4 };               // Массив объектов сервоприводов для удобства управления
int servosCurrentPos[] = { 90, 90, 90, 90, 90 };                      // Массив текущих позиций сервоприводов
int servosTargetPos[] = { 90, 90, 90, 90, 90 };                       // Массив целевых позиций сервоприводов
uint32_t servosTimer[] = { 0, 0, 0, 0, 0 };                           // Таймеры для контроля времени обновления положения каждого сервопривода
int sDelay = 5;                                                       // Задержка между обновлениями положения
uint32_t servosDelay[] = { sDelay, sDelay, sDelay, sDelay, sDelay };  // Массив задержек для каждого сервопривода
int lastTargetPos[] = { 90, 90, 90, 90, 90 };                         // Изначально центральные положения

#define NUM_READINGS 10
struct SmoothReading {
  int readings[NUM_READINGS];
  int readIndex = 0;
  long total = 0;
  int average = 0;
};

// Создаем индивидуальный фильтр для каждого потенциометра
SmoothReading smoothFilters[5];

// Глобальные переменные для отслеживания состояния кнопки и времени последнего нажатия
uint8_t buttonState = 0;    // Текущее состояние кнопки (0 - не нажата, 1 - нажата)
uint8_t buttonPressed = 0;  // Флаг, указывающий была ли кнопка нажата (0 - нет, 1 - да)
uint32_t buttonTimer = 0;   // Таймер для отслеживания времени последнего изменения состояния кнопки

void servoPosControl() {
  // Функция для плавного управления положением сервоприводов
  for (int i = 0; i <= 4; i++) {
    // Проверяем, прошло ли достаточно времени с последнего обновления положения
    if (millis() - servosTimer[i] > servosDelay[i]) {
      // Вычисляем разницу между текущим и целевым положением
      int delta = servosCurrentPos[i] == servosTargetPos[i] ? 0 : (servosCurrentPos[i] < servosTargetPos[i] ? 1 : -1);
      // Обновляем текущее положение
      servosCurrentPos[i] += delta;
      // Обновляем таймер
      servosTimer[i] = millis();
      // Устанавливаем новое положение сервопривода
      servos[i].write(servosCurrentPos[i]);
    }
  }
}

// Функция для переключения состояния светодиода
void buttonFunction() {
  activeServo++;
  if (activeServo > 5) activeServo = 1;
}

// Функция для управления состоянием кнопки и вызова функции buttonFunction при ее нажатии
void buttonControl() {
  // Читаем состояние кнопки (с учетом INPUT_PULLUP, нажатие будет 0)
  buttonState = !digitalRead(BUTTON_PIN);

  // Проверяем, прошло ли достаточно времени с момента последнего изменения состояния кнопки
  if (millis() - buttonTimer > 50) {
    // Если кнопка была нажата и ранее не была учтена
    if (!buttonPressed and buttonState) {
      buttonTimer = millis();
      buttonFunction();   // Вызываем функцию для переключения светодиода
      buttonPressed = 1;  // Устанавливаем флаг, что кнопка была нажата
    }
    // Если кнопка была отпущена
    else if (buttonPressed and !buttonState) {
      buttonPressed = 0;  // Сбрасываем флаг нажатия кнопки
    }
  }
}

void setup() {
  disp.clear();
  disp.brightness(2);
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  part0.attach(3);
  part1.attach(4);
  part2.attach(5);
  part3.attach(6);
  part4.attach(7);
  readPot();
}

int smoothAnalogReading(int pin, SmoothReading *filter) {
  filter->total -= filter->readings[filter->readIndex];        // вычитаем старое значение
  filter->readings[filter->readIndex] = analogRead(pin);       // читаем новое значение
  filter->total += filter->readings[filter->readIndex];        // добавляем новое значение к сумме
  filter->readIndex = (filter->readIndex + 1) % NUM_READINGS;  // переходим к следующему индексу
  filter->average = filter->total / NUM_READINGS;              // вычисляем среднее значение
  return filter->average;
}

void readPot() {
  servosTargetPos[0] = map(smoothAnalogReading(A4, &smoothFilters[0]), 0, 1023, 0, 180);
  servosTargetPos[1] = map(smoothAnalogReading(A3, &smoothFilters[1]), 0, 1023, 0, 180);
  servosTargetPos[2] = map(smoothAnalogReading(A2, &smoothFilters[2]), 0, 1023, 0, 180);
  servosTargetPos[3] = map(smoothAnalogReading(A1, &smoothFilters[3]), 0, 1023, 0, 180);
  servosTargetPos[4] = map(smoothAnalogReading(A0, &smoothFilters[4]), 0, 1023, 0, 180);
}

void checkServoChanges() {
  for (int i = 0; i < 5; i++) {
    int currentTarget = servosTargetPos[i];
    // Проверяем изменение больше 2 градусов между старым и новым целевым положением
    if (abs(currentTarget - lastTargetPos[i]) > 10) {
      lastTargetPos[i] = currentTarget;  // Обновляем последнюю известную позицию
      activeServo = 5 - i;               // Смещение для индексации с 1
    }
  }
}

void loop() {
  readPot();
  buttonControl();
  servoPosControl();  // Управляем положением сервоприводов

  if (millis() - dispTimer > 100) {
    dispTimer = millis();
    checkServoChanges();
    activeAngle = servosTargetPos[5 - activeServo];
    disp.display(activeServo, activeAngle > 99 ? activeAngle / 100 : 255, activeAngle > 9 ? activeAngle / 10 % 10 : 255, activeAngle % 10);
  }
}