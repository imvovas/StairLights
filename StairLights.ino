/*
 * 
 * Программно-аппаратный модуль StairLights
 * (с) Vladimir Shvedchenko 
 * 
 * Version 2019.07.19.001
 * 
 */

#include <Bounce2.h> // избавляемся от дребезга срабатываний сенсоров (https://github.com/thomasfredericks/Bounce2)
#include <Chrono.h>  // добавляем несколько таймеров для управления событиями (http://github.com/SofaPirate/Chrono)
#include <SoftPWM.h> // для получения ШИМ (программного) на всех выходах (https://github.com/bhagman/SoftPWM)

const int lightMinBrightness = 0; // минимальная яркость свечения LED (0-255)

const int effectsVariants = 11; // количество вариантов гашения LED

const int lightOnDelay = 100; // задержка между включениями LED
const int lightOffDelay = 1000; // задержка между выключениями LED

const int luminosityLowValue = 100; // граница уровня освещенности, ниже которого активируется освещение

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

  int luminositySensorPin = A0; // аналоговый вход, куда подключен фоторезистор
  int lightMaxBrightnessLevelPin = A1; // аналоговый вход, куда подключен потенциометр для управления максимальной яркостью свечения LED-ленты

  const int lightDelay = 1000; // длительность включения LED для тестовой платы (5 сек)
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

  int luminositySensorPin = A0; // аналоговый вход, куда подключен фоторезистор
  int lightMaxBrightnessLevelPin = A1; // аналоговый вход, куда подключен потенциометр для управления максимальной яркостью свечения LED-ленты

  const int lightDelay = 5000; // длительность включения LED для продуктовой платы (30 сек)
#endif

int lightMaxBrightness = 125; // максимальная яркость свечения LED (0-255), временное значение до чтения уровня заданного потенциометром на lightMaxBrightnessLevelPin

int luminositySensorValue = 0; // переменная для хранения считанного сенсором освещенности значения
bool luminositySensorLock = false; // признак блокировки реакции на сенсор освещенности (необходимо для продления действия подсветки, если сенсор засвечивается ей же)

bool valueTopSensorPin = false; // устанавливаем признак отсутствия срабатывания от сенсора верхней части лестницы
bool valueBottomSensorPin = false; // устанавливаем признак отсутствия срабатывания от сенсора нижней части лестницы

Bounce debouncerTopSensorPin = Bounce(); // создаём объект обработчика уровня для сенсора верхней части лестницы
Bounce debouncerBottomSensorPin = Bounce(); // создаём объект обработчика уровня для сенсора нижней части лестницы

Chrono chronoTop; // создаём объект таймера для событий сенсора верхней части лестницы
Chrono chronoBottom; // создаём объект таймера для событий сенсора нижней части лестницы 

bool cycleTopDone = true; // признак завершения цикла "включить-выключить" для LED при инициировании цикла с верхней части лестницы
bool cycleBottomDone = true; // признак завершения цикла "включить-выключить" для LED при инициировании цикла с нижней части лестницы

void setup() {
  SoftPWMBegin(); // инициализируем програмную эмуляцию ШИМ
  
  SoftPWMSet(ledPin1, lightMinBrightness); // устанавливаем на всех выводах для LED минимальный уровень свечения lightMinBrightness
  SoftPWMSet(ledPin2, lightMinBrightness);
  SoftPWMSet(ledPin3, lightMinBrightness);
  SoftPWMSet(ledPin4, lightMinBrightness);
  SoftPWMSet(ledPin5, lightMinBrightness);
  SoftPWMSet(ledPin6, lightMinBrightness);
  SoftPWMSet(ledPin7, lightMinBrightness);

  SoftPWMSetFadeTime(ledPin1, 100, 2500); // устанавливаем время перехода lightMinBrightness-lightMaxBrightness (включение) и время перехода lightMaxBrightness-lightMinBrightness (выключение) для всех выводов LED
  SoftPWMSetFadeTime(ledPin2, 100, 2500);
  SoftPWMSetFadeTime(ledPin3, 100, 2500);
  SoftPWMSetFadeTime(ledPin4, 100, 2500);
  SoftPWMSetFadeTime(ledPin5, 100, 2500);
  SoftPWMSetFadeTime(ledPin6, 100, 2500);
  SoftPWMSetFadeTime(ledPin7, 100, 2500);

  pinMode(topSensorPin,INPUT_PULLUP); // активируем внутренний подтягивающий резистор для выхода сенсора верхней части лестницы
  pinMode(bottomSensorPin,INPUT_PULLUP); // активируем внутренний подтягивающий резистор для выхода сенсора нижней части лестницы

  debouncerTopSensorPin.attach(topSensorPin); // определяем вход от обработчика дребезга срабатываний сенсора верхней части лестницы и границу отсечения ложных срабатываний (мкс).
  debouncerTopSensorPin.interval(5);
  
  debouncerBottomSensorPin.attach(bottomSensorPin); // определяем вход от обработчика дребезга срабатываний сенсора нижней части лестницы и границу отсечения ложных срабатываний (мкс).
  debouncerBottomSensorPin.interval(5);

  randomSeed(analogRead(3)); // инициализируем генератор псевдо-случайных чисел, запуская его от текущего сигнала на аналоговом входе
}

void loop() {
  doDebounce(); // обновляем состояние обработчиков дребезга сенсоров
  doSensorsRead(); // читаем состояние сенсоров
  doSomeLogic(); // выполняем логику управления LED
}

// управление логикой работы LED
void doSomeLogic() {
  if (((valueTopSensorPin == true)&&(luminositySensorValue<luminosityLowValue))||((valueTopSensorPin == true)&&(luminositySensorLock == true))) { // если пришло событие от сенсора верхней части лестницы и уровень освещенности упал ниже luminosityLowValue
    switchOnTopLEDs(); // включаем LED в направлении сверху вниз
    cycleTopDone=false; // устанавливаем признак срабатывания от сенсора верхней части лестницы
    chronoTop.restart(); // перезапускаем таймер для событий верхней части лестницы
  }

  if (((valueBottomSensorPin == true)&&(luminositySensorValue<luminosityLowValue))||((valueBottomSensorPin == true)&&(luminositySensorLock == true))) { // если пришло событие от сенсора нижней части лестницы и уровень освещенности упал ниже luminosityLowValue
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
}

// обновление состояний обработчиков дребезга сенсоров
void doDebounce() {
  debouncerTopSensorPin.update(); // обновляем состояние обработчика событий сенсора верхней части лестницы
  debouncerBottomSensorPin.update(); // обновляем состояние обработчика событий сенсора нижней части лестницы
}

// чтение сенсоров
void doSensorsRead() {
  lightMaxBrightness = map(analogRead(lightMaxBrightnessLevelPin), 0, 1023, 0, 255); // читаем значение максимальной допустимой яркости LED-ленты
  luminositySensorValue = analogRead(luminositySensorPin); // читаем значение уровня освещенности сенсора
  valueTopSensorPin = debouncerTopSensorPin.read(); // читаем состояние сенсора верхней части лестницы
  valueBottomSensorPin = debouncerBottomSensorPin.read(); // читаем состояние сенсора нижней части лестницы
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
}  

// включаем LED в направлении снизу вверх
void switchOnBottomLEDs() {
    luminositySensorLock=true; // блокируем реакцию на уровень освещения    

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
      break;
    }
    case 2: {  
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      break;
    }
    case 3: {
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
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
      break;
    }
    case 6: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      break;
    }
    case 7: {
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
      break;
    }
    case 8: {
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin7, lightMinBrightness);
      break;
    }
    case 9: {
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
      break;
    }
    case 10: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      break;
    }    
    case 11: {
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
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
      break;
    }
  }  
  luminositySensorLock = false; // разблокируем реакцию на уровень освещения
}

// выключаем LED в направлении снизу вверх в соответствии с выбранным сценарием effectVariant
void switchOffBottomLEDs(int effectVariant) {
  switch (effectVariant) {
    case 1: {
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
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      break;
    }
    case 3: {
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      break;
    }
    case 4: {
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
      break;
    }    
    case 5: {
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin1, lightMinBrightness);
      break;
    }
    case 6: {
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin4, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      break;
    }
    case 7: {
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin2, lightMinBrightness);
      break;
    }
    case 8: {
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
      break;
    }
    case 9: {
      SoftPWMSet(ledPin7, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin6, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      break;
    }
    case 10: {
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      SoftPWMSet(ledPin4, lightMinBrightness);
      break;
    }
    case 11: {
      SoftPWMSet(ledPin7, lightMinBrightness);
      SoftPWMSet(ledPin1, lightMinBrightness);
      delay(lightOffDelay);      
      SoftPWMSet(ledPin6, lightMinBrightness);
      SoftPWMSet(ledPin2, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin5, lightMinBrightness);
      SoftPWMSet(ledPin3, lightMinBrightness);
      delay(lightOffDelay);
      SoftPWMSet(ledPin4, lightMinBrightness);
      break;
    }
    default: {
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
      break;
    }
  }
  luminositySensorLock = false; // разблокируем реакцию на уровень освещения
}
