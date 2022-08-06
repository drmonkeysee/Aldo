//
//  EmulatorView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/9/22.
//

import SwiftUI

struct EmulatorView: View {
    private static let nesResolution = (w: 256, h: 240)
    private static let nesScale = 2

    var body: some View {
        HStack {
            ZStack {
                Color(white: 0.15)
                    .cornerRadius(5)
                    .frame(width: .init(Self.nesResolution.w * Self.nesScale),
                           height: .init(Self.nesResolution.h * Self.nesScale))
                Color.cyan
                    .frame(width: .init(Self.nesResolution.w),
                           height: .init(Self.nesResolution.h))
                Text("Emu Screen")
            }
            TabView {
                EmuDetailsView()
                    .tabItem {
                        Text("Details")
                    }
                    .border(.blue)
                Text("Debugger stuff")
                    .border(.gray)
                    .tabItem {
                        Text("Debug")
                    }
                Text("Trace Log")
                    .tabItem {
                        Text("Trace")
                    }
            }
        }
    }
}

struct EmuDetailsView: View {
    var body: some View {
        VStack(alignment: .leading) {
            HStack(alignment: .top) {
                GroupBox {
                    VStack {
                        VStack {
                            Text("A: 00")
                            Text("X: 00")
                            Text("Y: 00")
                        }
                        Divider()
                        VStack {
                            Text("PC: 0000")
                            Text("S: 00")
                            Text("P: 00")
                        }
                        Divider()
                        Text("FLAGS HERE")
                        Divider()
                        Text("SIGNALS HERE")
                    }
                    .font(.system(.body, design: .monospaced))
                } label: {
                    Label("CPU", systemImage: "cpu")
                        .font(.headline)
                }
                GroupBox {
                    VStack {
                        Group {
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                        }
                        Group {
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                            Text("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
                        }
                    }
                    .font(.system(.body, design: .monospaced))
                    .fixedSize(horizontal: true, vertical: false)
                } label: {
                    Label("RAM", systemImage: "memorychip")
                        .font(.headline)
                }
            }
            GroupBox {
                Text("PRG LINES HERE")
                Divider()
                Text("VECTORS HERE")
            } label: {
                Label("PRG", systemImage: "doc.plaintext")
                    .font(.headline)
            }
        }
    }
}

struct EmulatorView_Previews: PreviewProvider {
    static var previews: some View {
        EmulatorView()
    }
}
