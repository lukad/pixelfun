import CoreBluetooth
import Foundation
import SwiftUI

enum ConnectionStatus {
    case connected(PixelFun)
    case disconnected
    case scanning
    case connecting
    case error

    var color: Color {
        switch self {
        case .connected:
            .green
        case .disconnected:
            .black.opacity(0.7)
        case .scanning:
            .pink
        case .connecting:
            .pink
        case .error:
            .red
        }
    }
}

typealias RGB = (UInt8, UInt8, UInt8)

class PixelFun: ObservableObject {
    @Published var program: String
    @Published var brightness: UInt8
    @Published var fps: UInt8
    @Published var color1: RGB
    @Published var color2: RGB

    private var peripheral: CBPeripheral
    private var programCharacteristic: CBCharacteristic
    private var brightnessCharacteristic: CBCharacteristic
    private var framerateCharacteristic: CBCharacteristic
    private var color1Characteristic: CBCharacteristic
    private var color2Characteristic: CBCharacteristic

    init(program: String = "", brightness: UInt8 = 25, fps: UInt8 = 60, color1: RGB = (0, 0, 0), color2: RGB = (255, 255, 255), peripheral: CBPeripheral, programCharacteristic: CBCharacteristic, brightnessCharacteristic: CBCharacteristic, framerateCharacteristic: CBCharacteristic, color1Characteristic: CBCharacteristic, color2Characteristic: CBCharacteristic) {
        self.program = program
        self.brightness = brightness
        self.color1 = color1
        self.color2 = color2
        self.fps = fps

        self.peripheral = peripheral
        self.programCharacteristic = programCharacteristic
        self.brightnessCharacteristic = brightnessCharacteristic
        self.framerateCharacteristic = framerateCharacteristic
        self.color1Characteristic = color1Characteristic
        self.color2Characteristic = color2Characteristic
    }

    func writeProgram(_ source: String) {
        let data = source.data(using: .ascii, allowLossyConversion: true)!
        peripheral.writeValue(data, for: programCharacteristic, type: .withoutResponse)
    }

    func writeBrightness(_ value: UInt8) {
        peripheral.writeValue(Data([value]), for: brightnessCharacteristic, type: .withoutResponse)
    }

    func writeFramerate(_ value: UInt8) {
        peripheral.writeValue(Data([value]), for: framerateCharacteristic, type: .withoutResponse)
    }

    func writeColor1(_ value: RGB) {
        peripheral.writeValue(Data([value.0, value.1, value.2]), for: color1Characteristic, type: .withoutResponse)
    }

    func writeColor2(_ value: RGB) {
        peripheral.writeValue(Data([value.0, value.1, value.2]), for: color2Characteristic, type: .withoutResponse)
    }
}

class BluetoothService: NSObject, ObservableObject {
    private var cm: CBCentralManager!
    private var pixelFunPeripheral: CBPeripheral?
    private var programCharacteristic: CBCharacteristic?
    private var brightnessCharacteristic: CBCharacteristic?
    private var framerateCharacteristic: CBCharacteristic?
    private var color1Characteristic: CBCharacteristic?
    private var color2Characteristic: CBCharacteristic?

    @Published private(set) var peripheralState: ConnectionStatus = .disconnected

    private(set) var program: String?
    private(set) var brightness: UInt8?
    private(set) var fps: UInt8?
    private(set) var color1: RGB?
    private(set) var color2: RGB?

    override init() {
        super.init()
        self.cm = CBCentralManager(delegate: self, queue: nil)
    }

    func scanForPeripherals() {
        peripheralState = .scanning
        cm.scanForPeripherals(withServices: [CBUUIDs.Service])
    }
}

extension BluetoothService: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            print("CoreBluetooth powered on")
            scanForPeripherals()
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String: Any], rssi RSSI: NSNumber) {
        print("Discovered \(peripheral.name ?? "no name")")
        pixelFunPeripheral = peripheral
        peripheralState = .connecting
        cm.connect(peripheral)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        print("Connecting to \(peripheral.name ?? "no name")")
        peripheral.delegate = self
        peripheral.discoverServices([CBUUIDs.Service])
        cm.stopScan()
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        print("Disconnected from \(peripheral.name ?? "no name")")
        peripheralState = .disconnected
        brightnessCharacteristic = nil
        programCharacteristic = nil
        color1Characteristic = nil
        color2Characteristic = nil
        framerateCharacteristic = nil
        pixelFunPeripheral = nil
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        print("Failed to connect: \(error?.localizedDescription ?? "no error")")
        peripheralState = .error
        brightnessCharacteristic = nil
        programCharacteristic = nil
        color1Characteristic = nil
        color2Characteristic = nil
        framerateCharacteristic = nil
        pixelFunPeripheral = nil
    }
}

extension BluetoothService: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        for service in peripheral.services ?? [] {
            if service.uuid == CBUUIDs.Service {
                print("Discovered pixel fun service")
                let characteristics = [
                    CBUUIDs.ProgramCharacteristic,
                    CBUUIDs.BrightnessCharacteristic,
                    CBUUIDs.FramerateCharacteristic,
                    CBUUIDs.Color1Characteristic,
                    CBUUIDs.Color2Characteristic
                ]
                peripheral.discoverCharacteristics(characteristics, for: service)
            }
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        for characteristic in service.characteristics ?? [] {
            switch characteristic.uuid {
            case CBUUIDs.ProgramCharacteristic:
                programCharacteristic = characteristic
            case CBUUIDs.BrightnessCharacteristic:
                brightnessCharacteristic = characteristic
            case CBUUIDs.FramerateCharacteristic:
                framerateCharacteristic = characteristic
            case CBUUIDs.Color1Characteristic:
                color1Characteristic = characteristic
            case CBUUIDs.Color2Characteristic:
                color2Characteristic = characteristic
            default:
                ()
            }
            print("Discovered characteristic \(characteristic.uuid.uuidString)")
            peripheral.setNotifyValue(true, for: characteristic)
            peripheral.readValue(for: characteristic)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard let data = characteristic.value else {
            print("No data received for \(characteristic.uuid.uuidString)")
            return
        }

        print("Received data for \(characteristic.uuid.uuidString): \(String(decoding: data, as: UTF8.self))")

        if case .connecting = peripheralState {
            switch characteristic.uuid {
            case CBUUIDs.ProgramCharacteristic:
                program = String(decoding: data, as: UTF8.self)
            case CBUUIDs.BrightnessCharacteristic:
                brightness = data.first ?? 0
            case CBUUIDs.FramerateCharacteristic:
                fps = data.first ?? 1
            case CBUUIDs.Color1Characteristic:
                color1 = (data[0], data[1], data[2])
            case CBUUIDs.Color2Characteristic:
                color2 = (data[0], data[1], data[2])
            default:
                ()
            }

            if program != nil && brightness != nil && fps != nil && color1 != nil && color2 != nil {
                print("GOT all initial data")
                let device = PixelFun(
                    program: program!,
                    brightness: brightness!,
                    fps: fps!,
                    color1: color1!,
                    color2: color2!,
                    peripheral: peripheral,
                    programCharacteristic: programCharacteristic!,
                    brightnessCharacteristic: brightnessCharacteristic!,
                    framerateCharacteristic: framerateCharacteristic!,
                    color1Characteristic: color1Characteristic!,
                    color2Characteristic: color2Characteristic!
                )
                print(device.color1)
                print(device.color2)
                peripheralState = .connected(device)
            }
        } else if case .connected(let device) = peripheralState {
            if characteristic.uuid == CBUUIDs.ProgramCharacteristic {
                device.program = String(decoding: data, as: UTF8.self)
                print(device.program)
            } else if characteristic.uuid == CBUUIDs.BrightnessCharacteristic {
                device.brightness = data.first ?? 0
                print("\(device.brightness)")
            } else if characteristic.uuid == CBUUIDs.FramerateCharacteristic {
                device.fps = data.first ?? 1
                print("\(device.fps)")
            } else if characteristic.uuid == CBUUIDs.Color1Characteristic {
                print((data[0], data[1], data[2]))
                device.color1 = (data[0], data[1], data[2])
            } else if characteristic.uuid == CBUUIDs.Color2Characteristic {
                print((data[0], data[1], data[2]))
                device.color2 = (data[0], data[1], data[2])
            }
        }
    }
}
