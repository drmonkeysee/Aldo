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
        VStack {
            Text("Hello from Aldo!")
                .font(.title)
                .foregroundColor(.cyan)
                .padding()
            Text(fileUrl?.lastPathComponent ?? "No file selected")
                .frame(minWidth: 300)
                .padding()
                .truncationMode(.middle)
        }
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
