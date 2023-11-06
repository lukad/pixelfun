//
//  DeviceView.swift
//  PixelFun
//
//  Created by Luka Dornhecker on 05.11.23.
//

import SwiftUI

struct DeviceView: View {
    @Binding var program: String
    @Binding var fps: Float
    @Binding var brightness: Float
    @Binding var color1: Color
    @Binding var color2: Color

    var brightnessValue: Float {
        Float(brightness)
    }

    var body: some View {
        Form {
            Section(header: Text("Animation")) {
                HStack {
                    Text("\(Int(fps)) FPS")
                    Slider(value: $fps, in: 1 ... 120, step: 1)
                }
                TextField("Program", text: $program, axis: .vertical)
                    .autocorrectionDisabled()
                    .textInputAutocapitalization(.never)
                    .textContentType(.none)
                    .keyboardType(.asciiCapable)
            }
            Section(header: Text("Colors")) {
                HStack {
                    Text("Brightness")
                    Slider(value: $brightness, in: 0 ... 255, step: 1)
                }
                ColorPicker("Foreground", selection: $color1, supportsOpacity: false)
                ColorPicker("Background", selection: $color2, supportsOpacity: false)
            }
        }
    }
}

#Preview {
    DeviceView(
        program: .constant("sin(t)"),
        fps: .constant(60.0),
        brightness: .constant(25.0),
        color1: .constant(Color.red),
        color2: .constant(Color.blue)
    )
}
