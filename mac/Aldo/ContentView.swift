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
        VStack(alignment: .leading) {
            Group {
                Text(fileUrl?.lastPathComponent ?? "No file selected")
                    .truncationMode(.middle)
                Text("Format: iNES")
            }
            Divider()
            Text("Mapper: 000 (<Board Names>)")
            Divider()
            Group {
                Text("PRG ROM: 2 x 16KB")
                Text("WRAM: no")
                Text("CHR ROM: 1 x 8KB")
                Text("CHR RAM: no")
                Text("NT-Mirroring: Vertical")
                Text("Mapper-Ctrl: no")
            }
            Divider()
            Group {
                Text("Trainer: no")
                Text("Bus Conflicts: no")
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
