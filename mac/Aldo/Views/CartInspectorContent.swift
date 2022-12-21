//
//  CartInspectorContent.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/18/22.
//

import SwiftUI

typealias InspectorComposition = (content: NSViewController,
                                  accessory: NSTitlebarAccessoryViewController)

func createCartInspectorComposition() -> InspectorComposition {
    let v = CartInspectorView()
    let c = NSTitlebarAccessoryViewController()
    c.view = NSHostingView(rootView: v.createToolbarView())
    c.view.frame.size = c.view.fittingSize
    c.layoutAttribute = .trailing
    return (NSHostingController(rootView: v), c)
}

fileprivate struct CartInspectorView: View {
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

    func createToolbarView() -> ToolbarView { .init() }

    private func pickFile() {
        let panel = NSOpenPanel()
        panel.message = "Choose a ROM file"
        if panel.runModal() == .OK {
            cartLoadFailed = !cart.load(from: panel.url)
        }
    }
}

fileprivate struct ToolbarView: View {
    var body: some View {
        HStack {
            Text("THIS SI LONGER TETX BLAH BLASDLFJKASDFLKJ LSADKJ FA;LKSDJ FAL;SFJD ALSDJF A;LDJ FASLDJK FAS;DLFJ ASDL;FKJ ASDL;FJ ASD;LFJKA SDFL;JKASD").font(.title3.bold()).truncationMode(.middle)
                .frame(maxWidth: 700)
                .fixedSize()
            Button {
                aldoLog.log("BUTTON PUSH")
            } label: {
                Label("DO STUFF",
                      systemImage: "arrow.up.doc")
                .imageScale(.large)
            }
            .help("DO STUFF")
        }
        .padding(.trailing)
    }
}
