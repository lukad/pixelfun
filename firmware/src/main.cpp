#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <NimBLEDevice.h>

#include <NimBLEHIDDevice.h>

#include "PixelFun.h"

#define DATA_PIN A11
#define PIXEL_COUNT 64

Adafruit_NeoPixel strip(PIXEL_COUNT, DATA_PIN, NEO_GRB + NEO_KHZ800);

PixelFun<1024> pixelFun;

#define BLE_DEVICE_NAME "PixelFun"
#define BLE_PIXELFUN_SERVICE_UUID "565AA538-1311-41B8-BE4D-7018A7CF18AF"
#define BLE_PIXELFUN_PROGRAM_CHARACTERISTIC_UUID "ABC02BC7-123F-4DEC-98FF-3B7750A401DE"
#define BLE_PIXELFUN_BRIGHTNESS_CHARACTERISTIC_UUID "02307AFC-72B4-48DE-9FDF-EE26BA1A71C7"

NimBLEServer *pServer;
NimBLEService *pService;
NimBLECharacteristic *pProgramCharacteristic;
NimBLECharacteristic *pBrightnessCharacteristic;
NimBLEAdvertising *pAdvertising;


char program[1024] = "sin(2*t-hypot(x-3.5,y-3.5))";
uint8_t brightness = 25;

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
            } else {
                Serial.println("parse failed");
            }
        } else if (characteristic == pBrightnessCharacteristic) {
            Serial.println("Brightness");
            brightness = characteristic->getValue().data()[0];
            Serial.println(brightness);
            strip.setBrightness(brightness);
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
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

    pServer = NimBLEDevice::createServer();
    pServer->advertiseOnDisconnect(true);


    pService = pServer->createService(BLE_PIXELFUN_SERVICE_UUID);

    pProgramCharacteristic = pService->createCharacteristic(
            BLE_PIXELFUN_PROGRAM_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_AUTHEN | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_AUTHEN | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::NOTIFY
    );
    pProgramCharacteristic->setCallbacks(&characteristicCallbacks);
    pProgramCharacteristic->setValue((uint8_t *) program, strlen(program));

    pBrightnessCharacteristic = pService->createCharacteristic(
            BLE_PIXELFUN_BRIGHTNESS_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_AUTHEN | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_AUTHEN | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::NOTIFY
    );
    pBrightnessCharacteristic->setCallbacks(&characteristicCallbacks);
    pBrightnessCharacteristic->setValue(&brightness, 1);

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

const uint32_t fps = 60;
const uint32_t dt_ms = 1000 / fps;
const float dt_s = 1.0 / fps;

float current_time = 0.0f;

uint32_t interpolate_colors(int r1, int g1, int b1, int r2, int g2, int b2, float t) {
    if (t > 0.0) {
        return Adafruit_NeoPixel::Color((uint8_t) ((float) r1 * t), (uint8_t) ((float) g1 * t),
                                        (uint8_t) ((float) b1 * t));
    } else if (t < 1.0) {
        return Adafruit_NeoPixel::Color((uint8_t) ((float) r2 * -t), (uint8_t) ((float) g2 * -t),
                                        (uint8_t) ((float) b2 * -t));
    } else {
        return Adafruit_NeoPixel::Color(0, 0, 0);
    }
}

void loop() {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int idx;
            if (y % 2 == 0) {
                idx = y * 8 + (7 - x);
            } else {
                idx = y * 8 + x;
            }
            float value = pixelFun.eval(current_time, float(idx), float(x), float(y));
            strip.setPixelColor(idx, interpolate_colors(251, 72, 196, 63, 255, 33, value));
        }
    }
    strip.show();
    current_time += dt_s;
    delay(dt_ms);
}
