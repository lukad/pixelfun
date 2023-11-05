import SwiftUI

struct SettingsView: View {
    @AppStorage("keep-awake") var keepAwake = true
    @Environment(\.dismiss) var dismiss

    var body: some View {
        NavigationStack {
            List {
                Section(header: Text("Configuration")) {
                    Toggle("Keep display awake", isOn: $keepAwake)
                }
                Section(header: Text("Links")) {
                    Link("Mastodon", destination: URL(string: "https://chaos.social/@lukad")!)
                }
                Section(header: Text("Credits")) {
                    Link("tixy.land", destination: URL(string: "https://tixy.land")!)
                }
            }
            .navigationTitle("Settings")
            .toolbar {
                ToolbarItem {
                    Button {
                        dismiss()
                    } label: {
                        Text("Done").bold()
                    }
                }
            }
        }
    }
}

#Preview {
    SettingsView()
}
