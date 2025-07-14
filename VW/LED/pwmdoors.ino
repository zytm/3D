#include <SPI.h>
#include <mcp2515.h>
#include <EEPROM.h>

struct can_frame canMsg;
MCP2515 mcp2515(10);

// Светодиоды (PWM пины)
const int ledPins[4] = {3, 5, 6, 9}; // Водитель, пассажир, зад.лев, зад.прав
int currentBrightness[4] = {0, 0, 0, 0};
int targetBrightness[4]  = {0, 0, 0, 0};

// Настройки плавности
const int fadeStep = 3;
const int fadeDelay = 10; // Затухание и розжиг ≈ 1 сек
unsigned long lastFadeTime = 0;

// EEPROM адреса
const int EEPROM_MODE_ADDR = 0;
const int EEPROM_BRIGHTNESS_ADDR = 1;

// CAN IDs
#define DOOR_STATUS_ID  0x470
#define BUTTONS_ID      0x5BF
#define OK_CODE         0x28
#define NEXT_CODE       0x02
#define PREV_CODE       0x03

// Режимы
byte currentMode = 1;
byte selectedMode = 1;
byte currentBrightnessLevel = 3;  // 1 = 10%, 2 = 50%, 3 = 100%
byte selectedBrightnessLevel = 3;
const int brightnessValues[4] = {0, 25, 127, 255}; // 0 индекс не используется

// Состояния
bool inConfigMode = false;
bool inBrightnessConfig = false;
bool manualLightActive = false;

bool okIsPressed = false;
unsigned long okPressedTime = 0;

byte okClickCount = 0;
unsigned long lastOkClickTime = 0;

bool okPressedInConfig = false;
unsigned long lastOkPressInConfig = 0;

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    analogWrite(ledPins[i], 0);
  }

  currentMode = EEPROM.read(EEPROM_MODE_ADDR);
  if (currentMode < 1 || currentMode > 4) currentMode = 1;

  currentBrightnessLevel = EEPROM.read(EEPROM_BRIGHTNESS_ADDR);
  if (currentBrightnessLevel < 1 || currentBrightnessLevel > 3) currentBrightnessLevel = 3;

  mcp2515.reset();
  mcp2515.setBitrate(CAN_100KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("CAN Door Light Controller with Menu");
  Serial.print("Mode: ");
  Serial.println(currentMode);
  Serial.print("Brightness level: ");
  Serial.println(currentBrightnessLevel);
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {

    // OK
    if (canMsg.can_id == BUTTONS_ID && canMsg.data[0] == OK_CODE) {
      if (!okIsPressed) {
        okIsPressed = true;
        okPressedTime = millis();

        if (!inConfigMode) {
          if (millis() - lastOkClickTime > 600) {
            okClickCount = 1;
          } else {
            okClickCount++;
            if (okClickCount == 3) {
              manualLightActive = !manualLightActive;
              Serial.print("Manual light: ");
              Serial.println(manualLightActive ? "ON" : "OFF");
              setAllTargets(manualLightActive ? brightnessValues[currentBrightnessLevel] : 0);
              okClickCount = 0;
            }
          }
          lastOkClickTime = millis();
        } else {
          okPressedInConfig = true;
        }
      }
    }

    // Кнопка отпущена
    if (canMsg.can_id == BUTTONS_ID && canMsg.data[0] != OK_CODE && okIsPressed) {
      unsigned long held = millis() - okPressedTime;
      okIsPressed = false;

      if (!inConfigMode && held >= 3000) {
        inConfigMode = true;
        inBrightnessConfig = false;
        selectedMode = currentMode;
        selectedBrightnessLevel = currentBrightnessLevel;
        Serial.println("Entered configuration menu");
        flashEnterConfig();
        delay(500);
        flashMode(selectedMode);
        lastOkPressInConfig = millis();
      }
    }

    // Внутри меню
    if (inConfigMode) {
      if (canMsg.can_id == BUTTONS_ID) {
        byte code = canMsg.data[0];
        if (code == NEXT_CODE) {
          if (!inBrightnessConfig) {
            selectedMode = (selectedMode % 4) + 1;
            Serial.print("Mode selected: ");
            Serial.println(selectedMode);
            flashMode(selectedMode);
          } else {
            selectedBrightnessLevel = (selectedBrightnessLevel % 3) + 1;
            Serial.print("Brightness selected: ");
            Serial.println(selectedBrightnessLevel);
            showBrightnessLevel(selectedBrightnessLevel);
            delay(1000); // показываем 2 секунды
          }
          lastOkPressInConfig = millis();
        }
        if (code == PREV_CODE) {
          if (!inBrightnessConfig) {
            selectedMode = (selectedMode == 1) ? 4 : selectedMode - 1;
            Serial.print("Mode selected: ");
            Serial.println(selectedMode);
            flashMode(selectedMode);
          } else {
            selectedBrightnessLevel = (selectedBrightnessLevel == 1) ? 3 : selectedBrightnessLevel - 1;
            Serial.print("Brightness selected: ");
            Serial.println(selectedBrightnessLevel);
            showBrightnessLevel(selectedBrightnessLevel);
            delay(1000); // показываем 2 секунды
          }
          lastOkPressInConfig = millis();
        }
        if (code == OK_CODE && okPressedInConfig) {
          if (!inBrightnessConfig) {
            inBrightnessConfig = true;
            Serial.println("Switched to brightness config");
            showBrightnessLevel(selectedBrightnessLevel);
            delay(1+000); // показать яркость при входе в меню
          } else {
            // Сохраняем и выходим
            currentMode = selectedMode;
            currentBrightnessLevel = selectedBrightnessLevel;
            EEPROM.update(EEPROM_MODE_ADDR, currentMode);
            EEPROM.update(EEPROM_BRIGHTNESS_ADDR, currentBrightnessLevel);
            Serial.println("Settings saved.");
            exitConfigFlash();
            inConfigMode = false;
            inBrightnessConfig = false;
          }
          okPressedInConfig = false;
          lastOkPressInConfig = millis();
        }
      }
    }

    // Обработка дверей
    if (canMsg.can_id == DOOR_STATUS_ID && canMsg.can_dlc >= 2 && !manualLightActive && !inConfigMode) {
      byte doorByte = canMsg.data[1];
      updateLeds(doorByte);
    }
  }

  // Таймер выхода из меню
  if (inConfigMode && millis() - lastOkPressInConfig > 10000) {
    Serial.println("Menu timeout — saving and exiting.");
    currentMode = selectedMode;
    currentBrightnessLevel = selectedBrightnessLevel;
    EEPROM.update(EEPROM_MODE_ADDR, currentMode);
    EEPROM.update(EEPROM_BRIGHTNESS_ADDR, currentBrightnessLevel);
    exitConfigFlash();
    inConfigMode = false;
    inBrightnessConfig = false;
  }

  fadeLeds();
}

// Обновление яркости по режиму
void updateLeds(byte doorByte) {
  int value = brightnessValues[currentBrightnessLevel];
  switch (currentMode) {
    case 1:
      targetBrightness[0] = (doorByte & 0x01) ? value : 0;
      targetBrightness[1] = (doorByte & 0x02) ? value : 0;
      targetBrightness[2] = (doorByte & 0x04) ? value : 0;
      targetBrightness[3] = (doorByte & 0x08) ? value : 0;
      break;
    case 2: {
      bool frontOpen = (doorByte & 0x01) || (doorByte & 0x02);
      bool rearOpen = (doorByte & 0x04) || (doorByte & 0x08);
      targetBrightness[0] = targetBrightness[1] = frontOpen ? value : 0;
      targetBrightness[2] = targetBrightness[3] = rearOpen ? value : 0;
      break;
    }
    case 3: {
      bool any = doorByte & 0x0F;
      setAllTargets(any ? value : 0);
      break;
    }
    case 4:
      setAllTargets(0);
      break;
  }
}

// Плавное изменение яркости
void fadeLeds() {
  if (millis() - lastFadeTime < fadeDelay) return;
  lastFadeTime = millis();
  for (int i = 0; i < 4; i++) {
    if (currentBrightness[i] < targetBrightness[i]) {
      currentBrightness[i] += fadeStep;
      if (currentBrightness[i] > targetBrightness[i]) currentBrightness[i] = targetBrightness[i];
    } else if (currentBrightness[i] > targetBrightness[i]) {
      currentBrightness[i] -= fadeStep;
      if (currentBrightness[i] < targetBrightness[i]) currentBrightness[i] = targetBrightness[i];
    }
    analogWrite(ledPins[i], currentBrightness[i]);
  }
}

// Установить целевую яркость для всех
void setAllTargets(int val) {
  for (int i = 0; i < 4; i++) targetBrightness[i] = val;
}

// Вход в конфигурацию
void flashEnterConfig() {
  for (int i = 0; i < 3; i++) {
    analogWrite(ledPins[0], 255);
    delay(100);
    analogWrite(ledPins[0], 0);
    delay(100);
  }
}

// Индикация выбранного режима
void flashMode(byte mode) {
  delay(500);
  for (int i = 0; i < mode; i++) {
    analogWrite(ledPins[0], 255);
    delay(200);
    analogWrite(ledPins[0], 0);
    delay(200);
  }
  delay(500);
}

// Показ уровня яркости реальным значением
void showBrightnessLevel(byte level) {
  analogWrite(ledPins[0], brightnessValues[level]);
}

// Выход из меню
void exitConfigFlash() {
  analogWrite(ledPins[0], 255);
  delay(1000);
  analogWrite(ledPins[0], 0);
}
