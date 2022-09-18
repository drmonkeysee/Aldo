//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

final class ContentModel: ObservableObject {
    let cart = Cart()
    let driver = EmulatorScene()

    var frameClock: FrameClock { driver.clock }
}

struct ContentView: View {
    private static let fileLabel = "Open ROM File"
    private static let inset = 5.0

    @State private var cartLoadFailed = false
    @StateObject private var model = ContentModel()

    var body: some View {
        HStack(alignment: .top, spacing: Self.inset) {
            VStack {
                ScreenView(scene: model.driver)
                HStack(alignment: .top) {
                    TraitsView()
                        .environmentObject(model.frameClock)
                    ControlsView()
                }
            }
            .padding([.leading, .top], Self.inset)
            Divider()
            TabView {
                CartView()
                    .tabItem {
                        Text("Cart")
                    }
                CpuView()
                    .tabItem {
                        Text("CPU")
                    }
            }
            .padding([.trailing, .bottom], Self.inset)
            .environmentObject(model.cart)
        }
        .navigationTitle(model.cart.name ?? appName)
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
               presenting: model.cart.currentError) { _ in
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
            cartLoadFailed = !model.cart.load(from: panel.url)
        }
    }
}

fileprivate struct TraitsView: View {
    @EnvironmentObject var clock: FrameClock

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

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
