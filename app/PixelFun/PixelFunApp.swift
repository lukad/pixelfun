import SwiftUI

@main
struct PixelFunApp: App {
    @AppStorage("keep-awake") var keepAwake = true

    var body: some Scene {
        WindowGroup {
            ContentView().environmentObject(BluetoothService())
                .onAppear {
                    UIApplication.shared.isIdleTimerDisabled = keepAwake
                }
                .onChange(of: keepAwake) {
                    UIApplication.shared.isIdleTimerDisabled = keepAwake
                }
        }
    }
}
