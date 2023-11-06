import CoreBluetooth
import Foundation

struct CBUUIDs {
    static let Service = CBUUID(string: "565AA538-1311-41B8-BE4D-7018A7CF18AF")
    static let ProgramCharacteristic = CBUUID(string: "ABC02BC7-123F-4DEC-98FF-3B7750A401DE")
    static let BrightnessCharacteristic = CBUUID(string: "02307AFC-72B4-48DE-9FDF-EE26BA1A71C7")
    static let FramerateCharacteristic = CBUUID(string: "C8B74D1F-B691-4825-A60C-7D78D77A322E")
    static let Color1Characteristic = CBUUID(string: "EF598BF8-6CEC-4054-8926-990C5D46B1DA")
    static let Color2Characteristic = CBUUID(string: "4B95E86E-5207-4230-B838-ED361BDFC859")
}
