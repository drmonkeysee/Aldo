//
//  ContentView.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 12/22/22.
//

import SwiftUI

struct ContentView: View {
    private static let fileLabel = "Open ROM File"

    @State private var cartLoadFailed = false
    @StateObject private var cart = Cart()

    var body: some View {
        CartInspectorView()
            .environmentObject(cart)
            .navigationTitle(appTitle)
            .toolbar {
                ToolbarItem {
                    Button(action: pickFile) {
                        Label(Self.fileLabel,
                              systemImage: "arrow.up.doc")
                    }
                    .help(Self.fileLabel)
                }
            }
            .alert("Rom Load Failure", isPresented: $cartLoadFailed,
                   presenting: cart.currentError) { _ in
                // NOTE: default action
            } message: {
                Text($0.message)
            }
    }

    private var appTitle: String {
        var title = bundleAppName() ?? "CFBundleFailure"
        if let name = cart.name { title.append(": \(name)") }
        return title
    }

    private func pickFile() {
        let panel = NSOpenPanel()
        panel.message = "Choose a ROM file"
        if panel.runModal() == .OK {
            cartLoadFailed = !cart.load(from: panel.url)
        }
    }
}
