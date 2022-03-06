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
    @State private var navSelection: Int? = 0

    var body: some View {
        NavigationView {
            List {
                NavigationLink("Aldo",
                               destination: EmulatorView()
                                .navigationTitle("Aldo"),
                               tag: 0, selection: $navSelection)
                NavigationLink("Breadboard",
                               destination: BreadboardView()
                                .navigationTitle("Breadboard"),
                               tag: 1, selection: $navSelection)
                NavigationLink("Asm",
                               destination: AssemblerView()
                                .navigationTitle("Assembler"),
                               tag: 2, selection: $navSelection)
                NavigationLink("Details",
                               destination: CartDetailsView(fileUrl: $fileUrl)
                                .padding(EdgeInsets(top: 0, leading: 5,
                                                    bottom: 5, trailing: 5))
                                .navigationTitle("Cart Details"),
                               tag: 3, selection: $navSelection)

            }
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

struct EmulatorView: View {
    var body: some View {
        Text("Emulator view")
    }
}

struct BreadboardView: View {
    var body: some View {
        Text("Breadboard view")
    }
}

struct AssemblerView: View {
    var body: some View {
        Text("Assembler view")
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
