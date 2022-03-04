//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

struct ContentView: View {
    static private let fileLabel = "Open ROM File"
    static private let testRom = "PRG ROM\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah\nblah"

    @State private var fileUrl: URL?

    var body: some View {
        VStack {
            Text(fileUrl?.lastPathComponent ?? "No file selected")
                .help(fileUrl?.lastPathComponent ?? "")
                .font(.title)
                .truncationMode(.middle)
                .padding(.bottom)
                .frame(maxWidth: .infinity)
            HStack(alignment: .top) {
                GroupBox(label: Label("Format", systemImage: "info.circle")
                            .font(.headline)) {
                    HStack {
                        VStack(alignment: .trailing) {
                            Group {
                                Text("Format:")
                            }
                            Group {
                                Text("Mapper:")
                            }
                            Group {
                                Text("PRG ROM:")
                                Text("WRAM:")
                                Text("CHR ROM:")
                                Text("CHR RAM:")
                                Text("NT-Mirroring:")
                                Text("Mapper-Ctrl:")
                            }
                            Group {
                                Text("Trainer:")
                                Text("Bus Conflicts:")
                            }
                        }
                        VStack (alignment: .leading) {
                            Group {
                                Text("iNES")
                            }
                            Group {
                                Text("000 (<Board Names>)")
                            }
                            Group {
                                Text("2 x 16KB")
                                Text("no")
                                Text("1 x 8KB")
                                Text("no")
                                Text("Vertical")
                                Text("no")
                            }
                            Group {
                                Text("no")
                                Text("no")
                            }
                        }
                    }
                    .frame(minWidth: 230, maxWidth: .infinity)
                    .padding(.vertical, 5)
                }
                GroupBox(label: Label("PRG ROM", systemImage: "cpu")
                            .font(.headline)) {
                    ScrollView {
                        Text(ContentView.testRom)
                            .padding(5)
                            .frame(minWidth: 250, maxWidth: .infinity,
                                   alignment: .leading)
                            .multilineTextAlignment(.leading)
                    }
                }
                GroupBox(label: Label("CHR ROM", systemImage: "photo.circle")
                            .font(.headline)) {
                    ZStack {
                        RoundedRectangle(cornerRadius: 5)
                            .foregroundColor(.cyan)
                            .frame(width: 256, height: 128)
                        Text("No CHR Data")
                    }
                }
            }
        }
        .padding()
        .toolbar {
            ToolbarItem {
                Button(action: pickFile) {
                    Label(ContentView.fileLabel, systemImage: "plus")
                }
                .help(ContentView.fileLabel)
            }
        }
        .navigationTitle("Rom Details")
    }

    private func pickFile() {
        let panel = NSOpenPanel()
        panel.message = "Choose a ROM file"
        fileUrl = panel.runModal() == NSApplication.ModalResponse.OK
                    ? panel.url
                    : nil
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
