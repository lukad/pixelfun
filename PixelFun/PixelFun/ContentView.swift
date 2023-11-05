import SwiftUI

struct ObservedHolder<T: ObservableObject, Content: View>: View {
    @ObservedObject var value: T
    var content: (ObservedObject<T>.Wrapper) -> Content

    var body: some View {
        content(_value.projectedValue)
    }
}

func clamp(_ v: CGFloat) -> CGFloat {
    if v > 1.0 {
        return 1.0
    }
    if v < 0.0 {
        return 0.0
    }
    return v
}

extension Color {
    var uiColor: UIColor { .init(self) }

    var rgb: RGB {
        var (r, g, b, a): (CGFloat, CGFloat, CGFloat, CGFloat) = (0, 0, 0, 0)
        uiColor.getRed(&r, green: &g, blue: &b, alpha: &a)
        return (UInt8(clamp(r) * 255), UInt8(clamp(g) * 255), UInt8(clamp(b) * 255))
    }
}

struct ContentView: View {
    @EnvironmentObject var bt: BluetoothService

    @State private var showSettings = false

    var body: some View {
        NavigationStack {
            VStack(alignment: .leading) {
                switch bt.peripheralState {
                case .disconnected:
                    Text("Disconnected")
                        .padding()
                case .error:
                    Text("Error")
                        .padding()
                case .scanning:
                    Text("Scanning")
                        .padding()
                case .connecting:
                    Text("Connecting")
                        .padding()
                case .connected(let device):
                    var brightnessValue: Float {
                        Float(device.brightness)
                    }
                    var fpsValue: Float {
                        Float(device.fps)
                    }
                    var color1Value: Color {
                        let x = Color(
                            red: Double(device.color1.0) / 255.0,
                            green: Double(device.color1.1) / 255.0,
                            blue: Double(device.color1.2) / 255.0
                        )
                        print("Converted \(device.color1) to \(x)")
                        return x
                    }
                    var color2Value: Color {
                        Color(
                            red: Double(device.color2.0) / 255.0,
                            green: Double(device.color2.1) / 255.0,
                            blue: Double(device.color2.2) / 255.0
                        )
                    }
                    ObservedHolder(value: device) { object in
                        DeviceView(
                            program: object.program,
                            fps: Binding(
                                get: { fpsValue },
                                set: { device.fps = UInt8($0) }
                            ),
                            brightness: Binding(
                                get: { brightnessValue },
                                set: { device.brightness = UInt8($0) }
                            ),
                            color1: Binding(
                                get: { color1Value },
                                set: { device.color1 = $0.rgb }
                            ),
                            color2: Binding(
                                get: { color2Value },
                                set: { device.color2 = $0.rgb }
                            )
                        )
                    }
                    .onReceive(device.$program, perform: {
                        device.writeProgram($0)
                    })
                    .onReceive(device.$brightness, perform: {
                        device.writeBrightness($0)
                    })
                    .onReceive(device.$fps, perform: {
                        device.writeFramerate($0)
                    })
                    .onReceive(device.$color1, perform: {
                        device.writeColor1($0)
                    })
                    .onReceive(device.$color2, perform: {
                        device.writeColor2($0)
                    })
                }
            }
            .navigationTitle("PixelFun")
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button {
                        showSettings = true
                    } label: {
                        Image(systemName: "gear")
                    }
                }
            }
            .sheet(isPresented: $showSettings, content: {
                SettingsView()
            })
        }
    }
}

#Preview {
    ContentView()
        .environmentObject(BluetoothService())
}
