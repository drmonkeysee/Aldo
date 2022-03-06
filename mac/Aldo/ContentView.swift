//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

struct ContentView: View {
    static let fileLabel = "Open ROM File"

    @State private var fileUrl: URL?

    var body: some View {
        NavigationView {
            NavigationLink("Details",
                           destination: CartDetails(fileUrl: $fileUrl)
                            .padding(EdgeInsets(top: 0, leading: 5, bottom: 5,
                                                trailing: 5))
                            .navigationTitle("Cart Details"))
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
