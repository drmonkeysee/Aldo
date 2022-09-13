//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

struct ContentView: View {
    private static let fileLabel = "Open ROM File"
    private static let inset = 5.0

    @State private var navSelection: ContentLinks? = .emulator
    @State private var cartLoadFailed = false
    @StateObject private var cart = Cart()

    var body: some View {
        HStack(alignment: .top, spacing: Self.inset) {
            VStack {
                ScreenView()
                HStack(alignment: .top) {
                    TraitsView()
                    ControlsView()
                }
            }
            .padding([.leading, .top], Self.inset)
            Divider()
            SystemView()
                .padding([.trailing, .bottom], Self.inset)
                .environmentObject(cart)
        }
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

    private func navLinkViews() -> some View {
        List(ContentLinks.allCases) { link in
            NavigationLink(link.navLabel, tag: link,
                           selection: $navSelection) {
                ContentDetail(link: link, cart: cart)
                    .navigationTitle(cart.name ?? link.rawValue)
                    .toolbar {
                        ToolbarItem(placement: .navigation) {
                            Button(action: toggleSidebar) {
                                Label("Toggle Sidebar",
                                      systemImage: "sidebar.left")
                            }
                        }
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
        }
        .listStyle(.sidebar)
    }

    private func toggleSidebar() {
        // NOTE: seems kinda janky but oh well
        // https://stackoverflow.com/a/68331797/2476646
        NSApp.sendAction(#selector(NSSplitViewController.toggleSidebar(_:)),
                         to: nil, from: nil)
    }

    private func pickFile() {
        let panel = NSOpenPanel()
        panel.message = "Choose a ROM file"
        if panel.runModal() == .OK {
            cartLoadFailed = !cart.load(from: panel.url)
        }
    }
}

fileprivate struct TraitsView: View {
    var body: some View {
        GroupBox {
            Group {
                Text("FPS: 60 (50.54)")
                Text("ΔT: 20.291 (+14.876)")
                Text("Frames: 286")
                Text("Runtime: 5.659")
                Text("Cycles: 0")
                Text("Master Clock: INF Hz")
                Text("CPU/PPU Clock: INF/INF Hz")
                Text("Cycles per Second: 4")
                Text("Cycles per Frame: N/A")
                Text("BCD Supported: No")
            }
            .frame(maxWidth: .infinity)
        } label: {
            Label("Traits", systemImage: "server.rack")
        }
    }
}

fileprivate struct ControlsView: View {
    var body: some View {
        GroupBox {
            Group {
                Text("HALT")
                Text("Mode:  Cycle  Step  Run")
                Text("Signal:  IRQ  NMI  RES")
                Text("Halt/Run: <Space>")
                Text("Mode: m/M")
                Text("Signal: i, n, s")
                Text("Speed ±1 (±10): -/= (_/+)")
                Text("Ram F/B: r/R")
                Text("Quit: q")
            }
            .frame(maxWidth: .infinity)
        } label: {
            Label("Controls", systemImage: "gamecontroller")
        }
    }
}

fileprivate enum ContentLinks: String, CaseIterable, Identifiable {
    case emulator = "Aldo"
    case cart = "Cart Details"

    var id: Self { self }
    var navLabel: String { "\(self)".capitalized }
}

fileprivate struct ContentDetail: View {
    let link: ContentLinks
    let cart: Cart

    var body: some View {
        switch link {
        case .emulator:
            EmulatorView()
        case .cart:
            CartView()
                .environmentObject(cart)
        }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
