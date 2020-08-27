/*
 * 
 * Программно-аппаратный модуль StairLights
 * (с) Vladimir Shvedchenko 
 * 
 * Version 2020.08.27.002
 * 
 */

#include <Chrono.h>  // добавляем несколько таймеров для управления событиями (http://github.com/SofaPirate/Chrono)
#include <SoftPWM.h> // для получения ШИМ (программного) на всех выходах (https://github.com/bhagman/SoftPWM)

const int lightMinBrightness = 0; // минимальная яркость свечения LED (0-255)
const int effectsVariants = 10; // количество вариантов гашения LED

const bool debug = false; // переменная, активирующая вывод отладочной информации в последовательный порт

/*
#if defined(ARDUINO_AVR_UNO) // устанавливаем номера управляющих выходов для Arduino UNO (тестовая плата)
  const int ledPin1 =  4; 
  const int ledPin2 =  5;
  const int ledPin3 =  6;
  const int ledPin4 =  7;
  const int ledPin5 =  8;
  const int ledPin6 =  9;
  const int ledPin7 =  10;

  const byte topSensorPin = 3; // устанавливаем номер входа от сенсора верхней части лестницы
  const byte bottomSensorPin = 2; // устанавливаем номер входа от сенсора нижней части лестницы

  int lightMinLuminosityLevelPin = A0; // аналоговый вход, куда подключен потенциометр для управления минимальной освещенностью
  int lightMaxBrightnessLevelPin = A1; // аналоговый вход, куда подключен потенциометр для управления максимальной яркостью свечения LED-ленты
  int luminositySensorPin = A2; // аналоговый вход, куда подключен фоторезистор
  int lightChangingSpeedPin = A3; // аналоговый вход, куда подключен потенциометр для управления скоростью работы эффектов

  const int lightDelay = 1000; // длительность включения LED для тестовой платы (1 сек)
#endif

#if defined(ARDUINO_AVR_PRO) // устанавливаем номера управляющих выходов для Arduino Pro Mini (продуктовая плата)
  const int ledPin1 =  12;
  const int ledPin2 =  11;
  const int ledPin3 =  10;
  const int ledPin4 =  13;
  const int ledPin5 =  8;
  const int ledPin6 =  7;
  const int ledPin7 =  6;

  const byte topSensorPin = 3; // устанавливаем номер входа от сенсора верхней части лестницы
  const byte bottomSensorPin = 2; // устанавливаем номер входа от сенсора нижней части лестницы

  int lightMinLuminosityLevelPin = A0; // аналоговый вход, куда подключен потенциометр для управления минимальной освещенностью
  int lightMaxBrightnessLevelPin = A1; // аналоговый вход, куда подключен потенциометр для управления максимальной яркостью свечения LED-ленты
  int luminositySensorPin = A2; // аналоговый вход, куда подключен фоторезистор
  int lightChangingSpeedPin = A3; // аналоговый вход, куда подключен потенциометр для управления скоростью работы эффектов

  const int lightDelay = 30000; // длительность включения LED для продуктовой платы (30 сек)
#endif
*/

#if defined(ARDUINO_AVR_LEONARDO) // устанавливаем номера управляющих выходов для Arduino Leonardo
  const int ledPin1 =  9;
  const int ledPin2 =  8;
  const int ledPin3 =  7;
  const int ledPin4 =  6;
  const int ledPin5 =  10;
  const int ledPin6 =  16;
  const int ledPin7 =  14;
  const int ledPin8 =  15;

  const byte topSensorPin = 5; // устанавливаем номер входа от сенсора верхней части лестницы
  const byte bottomSensorPin = 4; // устанавливаем номер входа от сенсора нижней части лестницы

  int lightMinLuminosityLevelPin = A0; // аналоговый вход, куда подключен потенциометр для управления минимальной освещенностью
  int lightMaxBrightnessLevelPin = A1; // аналоговый вход, куда подключен потенциометр для управления максимальной яркостью свечения LED-ленты
  int luminositySensorPin = A2; // аналоговый вход, куда подключен фоторезистор
  int lightChangingSpeedPin = A3; // аналоговый вход, куда подключен потенциометр для управления скоростью работы эффектов

  const int lightDelay = 30000; // длительность включения LED для продуктовой платы (30 сек)
#endif


int lightOnDelay = 100; // задержка между включениями LED
int lightOffDelay = 1000; // задержка между выключениями LED

int lightMinLuminosity = 0; // значение минимального уровня освещения, после которого включается освещение
int lightChangingSpeed = 125; // значение для скорости отработки эффектов
int lightMaxBrightness = 125; // максимальная яркость свечения LED (0-255), временное значение до чтения уровня заданного потенциометром на lightMaxBrightnessLevelPin
int luminositySensorValue = 0; // переменная для хранения считанного сенсором освещенности значения

bool luminositySensorLock = false; // признак блокировки реакции на сенсор освещенности (необходимо для продления действия подсветки, если сенсор засвечивается ей же)

int valueTopSensorPin = LOW; // устанавливаем признак отсутствия срабатывания от сенсора верхней части лестницы
int valueBottomSensorPin = LOW; // устанавливаем признак отсутствия срабатывания от сенсора нижней части лестницы

Chrono chronoTop; // создаём объект таймера для событий сенсора верхней части лестницы
Chrono chronoBottom; // создаём объект таймера для событий сенсора нижней части лестницы 

bool cycleTopDone = true; // признак завершения цикла "включить-выключить" для LED при инициировании цикла с верхней части лестницы
bool cycleBottomDone = true; // признак завершения цикла "включить-выключить" для LED при инициировании цикла с нижней части лестницы


void setup() {
  if (debug) {  
    Serial.begin(57600);
  }
  
  SoftPWMBegin(); // инициализируем програмную эмуляцию ШИМ
  
  SoftPWMSet(ledPin1, lightMinBrightness); // устанавливаем на всех выводах для LED минимальный уровень свечения lightMinBrightness
  SoftPWMSet(ledPin2, lightMinBrightness);
  SoftPWMSet(ledPin3, lightMinBrightness);
  SoftPWMSet(ledPin4, lightMinBrightness);
  SoftPWMSet(ledPin5, lightMinBrightness);
  SoftPWMSet(ledPin6, lightMinBrightness);
  SoftPWMSet(ledPin7, lightMinBrightness);
  SoftPWMSet(ledPin8, lightMinBrightness);

  SoftPWMSetFadeTime(ledPin1, 100, 2500); // устанавливаем время перехода lightMinBrightness-lightMaxBrightness (включение) и время перехода lightMaxBrightness-lightMinBrightness (выключение) для всех выводов LED
  SoftPWMSetFadeTime(ledPin2, 100, 2500);
  SoftPWMSetFadeTime(ledPin3, 100, 2500);
  SoftPWMSetFadeTime(ledPin4, 100, 2500);
  SoftPWMSetFadeTime(ledPin5, 100, 2500);
  SoftPWMSetFadeTime(ledPin6, 100, 2500);
  SoftPWMSetFadeTime(ledPin7, 100, 2500);
  SoftPWMSetFadeTime(ledPin8, 100, 2500);

  pinMode(topSensorPin,INPUT); // устанавливаем режим работы входа для сигнала сенсора верхней части лестницы
  pinMode(bottomSensorPin,INPUT); // устанавливаем режим работы входа для сигнала сенсора нижней части лестницы

  randomSeed(analogRead(3)); // инициализируем генератор псевдо-случайных чисел, запуская его от текущего сигнала на аналоговом входе
}


void loop() {
  doSensorsRead(); // читаем состояние сенсоров
  doSomeLogic(); // выполняем логику управления LED
}


// чтение сенсоров
void doSensorsRead() {
  lightMinLuminosity = map(analogRead(lightMinLuminosityLevelPin), 0, 1023, 0, 150); // читаем значение минимального уровня освещения
  luminositySensorValue = map(analogRead(luminositySensorPin), 0, 1023, 0, 255); // читаем значение уровня освещенности сенсора
  lightMaxBrightness = map(analogRead(lightMaxBrightnessLevelPin), 0, 1023, 0, 250); // читаем значение максимальной допустимой яркости LED-ленты
  lightChangingSpeed = map(analogRead(lightChangingSpeedPin), 0, 1023, 0, 255); // читаем значение для скорости отработки эффектов

  lightOnDelay = lightChangingSpeed;
  lightOffDelay = lightChangingSpeed * 10;

  valueTopSensorPin = digitalRead(topSensorPin); // читаем состояние сенсора верхней части лестницы
  valueBottomSensorPin = digitalRead(bottomSensorPin); // читаем состояние сенсора нижней части лестницы    
}



// управление логикой работы LED
void doSomeLogic() {
  if (((valueTopSensorPin == HIGH)&&(luminositySensorValue<lightMinLuminosity))||((valueTopSensorPin == HIGH)&&(luminositySensorLock == true))) { // если пришло событие от сенсора верхней части лестницы и уровень освещенности упал ниже luminosityLowValue
    switchOnTopLEDs(); // включаем LED в направлении сверху вниз
    cycleTopDone=false; // устанавливаем признак срабатывания от сенсора верхней части лестницы
    chronoTop.restart(); // перезапускаем таймер для событий верхней части лестницы
  }

  if (((valueBottomSensorPin == HIGH)&&(luminositySensorValue<lightMinLuminosity))||((valueBottomSensorPin == HIGH)&&(luminositySensorLock == true))) { // если пришло событие от сенсора нижней части лестницы и уровень освещенности упал ниже luminosityLowValue
    switchOnBottomLEDs(); // включаем LED в направлении снизу вверх
    cycleBottomDone=false; // устанавливаем признак срабатывания от сенсора нижней части лестницы
    chronoBottom.restart(); // перезапускаем таймер для событий нижней части лестницы
  }  

  if (chronoTop.hasPassed(lightDelay)&&(cycleTopDone==false)) { // если сработал таймер события с верхней части лестницы и у нас активен признак срабатывания верхней части лестницы
    cycleTopDone=true; // устанавливаем признак завершенности цикла "включить-выключить" для LED, при инициировании цикла с верхней части лестницы
    chronoTop.restart(); // перезапускаем таймер для событий верхней части лестницы
    switchOffTopLEDs(random(effectsVariants)); // выключаем LED в направлении сверху вниз по случайному сценарию
  }
  
  if (chronoBottom.hasPassed(lightDelay)&&(cycleBottomDone==false)) { // если сработал таймер события с нижней части лестницы и у нас активен признак срабатывания нижней части лестницы
    cycleBottomDone=true; // устанавливаем признак завершенности цикла "включить-выключить" для LED, при инициировании цикла с нижней части лестницы
    chronoBottom.restart(); // перезапускаем таймер для событий нижней части лестницы
    switchOffBottomLEDs(random(effectsVariants)); // выключаем LED в направлении снизу вверх по случайному сценарию
  }

  if (debug) {
    Serial.print("lightMinLuminosity: ");
    Serial.println(lightMinLuminosity);
    Serial.print("luminositySensorValue: ");
    Serial.println(luminositySensorValue);
    Serial.println();
    Serial.print("lightMaxBrightness: ");
    Serial.println(lightMaxBrightness);
    Serial.print("lightChangingSpeed: ");
    Serial.println(lightChangingSpeed);
    Serial.println();
    Serial.print("valueTopSensorPin: ");
    Serial.println(valueTopSensorPin);
    Serial.print("valueBottomSensorPin: ");
    Serial.println(valueBottomSensorPin);
    Serial.println("==============================");

    delay(1000);
  }
}

// включаем LED в направлении сверху вниз
void switchOnTopLEDs() {
    luminositySensorLock=true; // блокируем реакцию на уровень освещения    

    SoftPWMSet(ledPin1, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin2, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin3, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin4, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin5, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin6, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin7, lightMaxBrightness);    
    delay(lightOnDelay);
    SoftPWMSet(ledPin8, lightMaxBrightness);    
    delay(lightOnDelay);
}  

// включаем LED в направлении снизу вверх
void switchOnBottomLEDs() {
    luminositySensorLock=true; // блокируем реакцию на уровень освещения    

    SoftPWMSet(ledPin8, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin7, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin6, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin5, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin4, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin3, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin2, lightMaxBrightness);
    delay(lightOnDelay);
    SoftPWMSet(ledPin1, lightMaxBrightness);    
    delay(lightOnDelay);
}

// выключаем LED в направлении сверху вниз в соответствии с выбранным сценарием effectVariant
void switchOffTopLEDs(int effectVariant) {
  switch (effectVariant) {
    case 1: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin8, lightMinBrightness);
      break;
    }
    case 2: {  
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 3: {
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 4: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 5: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 6: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 7: {
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 8: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 9: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);      
      break;
    }    
    case 10: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);      
      break;
    }
    default: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
  }  
  luminositySensorLock = false; // разблокируем реакцию на уровень освещения
}

// выключаем LED в направлении снизу вверх в соответствии с выбранным сценарием effectVariant
void switchOffBottomLEDs(int effectVariant) {
  switch (effectVariant) {
    case 1: {
      SoftPWMSet(ledPin8, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      break;
    }
    case 2: {  
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 3: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 4: {
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 5: {
      SoftPWMSet(ledPin8, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 6: {
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 7: {
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 8: {
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
    case 9: {
      SoftPWMSet(ledPin8, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);      
      break;
    }    
    case 10: {
      SoftPWMSet(ledPin8, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);      
      break;
    }
    default: {
      SoftPWMSet(ledPin8, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }
  }  
  luminositySensorLock = false; // разблокируем реакцию на уровень освещения
}
