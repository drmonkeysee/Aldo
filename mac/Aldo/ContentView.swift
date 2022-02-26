//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

struct ContentView: View {
    @State private var fileUrl: URL?

    var body: some View {
        VStack {
            Text("Hello from Aldo!")
                .font(.title)
                .foregroundColor(.cyan)
                .padding()
            Text(fileUrl?.lastPathComponent ?? "No file selected")
                .padding(.horizontal)
                .truncationMode(.middle)
            Button("Choose ROM File") {
                self.fileUrl = chooseFile()
            }
            .padding()
        }
    }

    private func chooseFile() -> URL? {
        let panel = NSOpenPanel()
        panel.message = "Choose a ROM file"
        return panel.runModal() == NSApplication.ModalResponse.OK
                ? panel.url
                : nil
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
