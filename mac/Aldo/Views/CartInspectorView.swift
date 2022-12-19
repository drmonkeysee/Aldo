//
//  CartInspectorView.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/18/22.
//

import SwiftUI

func createCartInspectorView() -> NSViewController {
    NSHostingController(rootView: CartInspectorView())
}

struct CartInspectorView: View {
    private static let fileLabel = "Open ROM File"

    @State private var cartLoadFailed = false
    @StateObject private var cart = Cart()

    var body: some View {
        CartView()
            .environmentObject(cart)
            .navigationTitle(cart.name ?? appName)
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

    private var appName: String {
        Bundle.main.object(forInfoDictionaryKey: "CFBundleName") as? String
            ?? "CFBundleFailure"
    }

    private func pickFile() {
        let panel = NSOpenPanel()
        panel.message = "Choose a ROM file"
        if panel.runModal() == .OK {
            cartLoadFailed = !cart.load(from: panel.url)
        }
    }
}

struct CartInspectorView_Previews: PreviewProvider {
    static var previews: some View {
        CartInspectorView()
    }
}
