#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <NimBLEDevice.h>
#include <tuple>
#include <NimBLEHIDDevice.h>

#include <PixelFun.h>

#ifndef DATA_PIN
#define DATA_PIN GPIO_NUM_6
#endif
#ifndef WIDTH
#define WIDTH 8
#endif
#ifndef HEIGHT
#define HEIGHT 8
#endif
const int PIXEL_COUNT = WIDTH * HEIGHT;

Adafruit_NeoPixel strip(PIXEL_COUNT, DATA_PIN, NEO_GRB + NEO_KHZ800);

PixelFun<1024> pixelFun;

#define BLE_DEVICE_NAME "PixelFun"
#define BLE_PIXELFUN_SERVICE_UUID "565AA538-1311-41B8-BE4D-7018A7CF18AF"
#define BLE_PIXELFUN_PROGRAM_CHARACTERISTIC_UUID "ABC02BC7-123F-4DEC-98FF-3B7750A401DE"
#define BLE_PIXELFUN_BRIGHTNESS_CHARACTERISTIC_UUID "02307AFC-72B4-48DE-9FDF-EE26BA1A71C7"
#define BLE_PIXELFUN_FRAMERATE_CHARACTERISTIC_UUID "C8B74D1F-B691-4825-A60C-7D78D77A322E"
#define BLE_PIXELFUN_COLOR1_CHARACTERISTIC_UUID "EF598BF8-6CEC-4054-8926-990C5D46B1DA"
#define BLE_PIXELFUN_COLOR2_CHARACTERISTIC_UUID "4B95E86E-5207-4230-B838-ED361BDFC859"

NimBLEServer *pServer;
NimBLEService *pService;
NimBLECharacteristic *pProgramCharacteristic;
NimBLECharacteristic *pBrightnessCharacteristic;
NimBLECharacteristic *pFrameRateCharacteristic;
NimBLECharacteristic *pColor1Characteristic;
NimBLECharacteristic *pColor2Characteristic;
NimBLEAdvertising *pAdvertising;

char program[1024] = "sin(2*t-hypot(x-3.5,y-3.5))";
uint8_t brightness = 25;
uint8_t color1[3] = {251, 72, 196};
uint8_t color2[3] = {63, 255, 33};
uint8_t frameRate = 60;

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *characteristic) override {
        Serial.print("Write ");
        if (characteristic == pProgramCharacteristic) {
            Serial.println("Program");
            strncpy(program, (char *) characteristic->getValue().data(), sizeof(program) - 1);
            program[sizeof(program) - 1] = '\0';
            Serial.println(program);
            if (pixelFun.parse(program)) {
                Serial.println("parse succeeded");
                pixelFun.printAST();
            } else {
                Serial.println("parse failed");
            }
        } else if (characteristic == pBrightnessCharacteristic) {
            Serial.println("Brightness");
            brightness = characteristic->getValue().data()[0];
            Serial.println(brightness);
            strip.setBrightness(brightness);
        } else if (characteristic == pFrameRateCharacteristic) {
            Serial.println("Frame Rate");
            frameRate = characteristic->getValue().data()[0];
            if (frameRate == 0) {
                frameRate = 1;
            }
            Serial.println(frameRate);
        } else if (characteristic == pColor1Characteristic) {
            Serial.println("Color 1");
            if (characteristic->getValue().length() == 3) {
                memcpy(color1, characteristic->getValue().data(), 3);
                Serial.printf("%d %d %d\n", color1[0], color1[1], color1[2]);
            } else {
                Serial.println("Invalid length");
            }
        } else if (characteristic == pColor2Characteristic) {
            Serial.println("Color 2");
            if (characteristic->getValue().length() == 3) {
                memcpy(color2, characteristic->getValue().data(), 3);
                Serial.printf("%d %d %d\n", color2[0], color2[1], color2[2]);
            } else {
                Serial.println("Invalid length");
            }
        } else {
            Serial.println("Unknown");
        }
    }
};

CharacteristicCallbacks characteristicCallbacks;

void setup() {
    Serial.begin(115200);

    for (int i = 0; i < 500; i++) {
        if (Serial) {
            Serial.println("Serial ready");
            break;
        }
        delay(1);
    }

    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setDeviceName(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    pServer = NimBLEDevice::createServer();
    pServer->advertiseOnDisconnect(true);

    pService = pServer->createService(BLE_PIXELFUN_SERVICE_UUID);

    pProgramCharacteristic = pService->createCharacteristic(
            BLE_PIXELFUN_PROGRAM_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR
    );
    pProgramCharacteristic->setCallbacks(&characteristicCallbacks);
    pProgramCharacteristic->setValue((uint8_t *) program, strlen(program));

    pBrightnessCharacteristic = pService->createCharacteristic(
            BLE_PIXELFUN_BRIGHTNESS_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR
    );
    pBrightnessCharacteristic->setCallbacks(&characteristicCallbacks);
    pBrightnessCharacteristic->setValue(&brightness, 1);

    pFrameRateCharacteristic = pService->createCharacteristic(
            BLE_PIXELFUN_FRAMERATE_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR
    );
    pFrameRateCharacteristic->setCallbacks(&characteristicCallbacks);
    pFrameRateCharacteristic->setValue(&frameRate, 1);

    pColor1Characteristic = pService->createCharacteristic(
            BLE_PIXELFUN_COLOR1_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR
    );
    pColor1Characteristic->setCallbacks(&characteristicCallbacks);
    pColor1Characteristic->setValue(color1, 3);

    pColor2Characteristic = pService->createCharacteristic(
            BLE_PIXELFUN_COLOR2_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR
    );
    pColor2Characteristic->setCallbacks(&characteristicCallbacks);
    pColor2Characteristic->setValue(color2, 3);

    pService->start();

    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(BLE_DEVICE_NAME);
    pAdvertising->addServiceUUID(BLE_PIXELFUN_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMaxPreferred(0x12);

    if (NimBLEDevice::startAdvertising()) {
        Serial.println("Started advertising");
    } else {
        Serial.println("Failed to start advertising");
    }

    if (pixelFun.parse(program)) {
        Serial.println("parse succeeded");
    } else {
        Serial.println("parse failed");
    }

    pixelFun.printAST();

    strip.begin();
    strip.setBrightness(25);
    strip.show();
}

float current_time = 0.0f;

void loop() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int idx = y * WIDTH + x;
            int led_idx = idx;
            if (y % 2 == 0) {
                led_idx = y * WIDTH + (WIDTH - 1 - x);
            }
            float value = pixelFun.eval(current_time, float(idx), float(x), float(y));
            uint8_t r, g, b;
            std::tie(r, g, b) = pixelFun.interpolateColors(color1, color2, value);
            strip.setPixelColor(led_idx, Adafruit_NeoPixel::Color(r, g, b));
        }
    }
    strip.show();
    current_time += 1.0f / float(frameRate);
    delayMicroseconds(1000000 / frameRate);
}
