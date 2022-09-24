//
//  CpuView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 9/12/22.
//

import SwiftUI

struct CpuView: View {
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

struct CpuView_Previews: PreviewProvider {
    static var previews: some View {
        CpuView()
    }
}
