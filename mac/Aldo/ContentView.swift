//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

struct ContentView: View {
    static private let fileLabel = "Open ROM File"

    @State private var fileUrl: URL?

    var body: some View {
        GroupBox {
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
        } label: {
            Text(fileUrl?.lastPathComponent ?? "No file selected")
                .help(fileUrl?.lastPathComponent ?? "")
                .font(.title)
                .truncationMode(.middle)
                .padding(.bottom)
                .frame(maxWidth: .infinity)
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
