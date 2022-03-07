//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

enum NavLinks: String, CaseIterable, Identifiable {
    case emulator = "Aldo"
    case breadboard = "Breadboard"
    case assembler = "Assembler"
    case details = "Cart Details"

    var id: Self {
        self
    }

    var navLabel: String {
        "\(self)".capitalized
    }
}

struct ContentView: View {
    static let fileLabel = "Open ROM File"

    @State private var fileUrl: URL?
    @State private var navSelection: NavLinks? = .emulator

    var body: some View {
        NavigationView(content: navLinkViews)
        .toolbar {
            ToolbarItem {
                Button(action: pickFile) {
                    Label(ContentView.fileLabel, systemImage: "plus")
                }
                .help(ContentView.fileLabel)
            }
        }
    }

    private func navLinkViews() -> some View {
        List(NavLinks.allCases) { link in
            NavigationLink(link.navLabel, tag: link,
                           selection: $navSelection) {
                let navPadding = EdgeInsets(top: 0, leading: 5, bottom: 5,
                                            trailing: 5)
                switch link {
                case .emulator:
                    EmulatorView()
                        .padding(navPadding)
                        .navigationTitle(link.rawValue)
                case .breadboard:
                    BreadboardView()
                        .navigationTitle(link.rawValue)
                case .assembler:
                    AssemblerView()
                        .navigationTitle(link.rawValue)
                case .details:
                    CartDetailsView(fileUrl: $fileUrl)
                        .padding(navPadding)
                        .navigationTitle(link.rawValue)
                }
            }
        }
        .listStyle(.sidebar)
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
    static let nesResolution = (256, 240)
    static let nesScale = 1

    var body: some View {
        HStack {
            ZStack {
                Color.cyan
                    .frame(width: Double(EmulatorView.nesResolution.0
                                    * EmulatorView.nesScale),
                           height: Double(EmulatorView.nesResolution.1
                                    * EmulatorView.nesScale))
                Text("Emu Screen")
            }
            Text("Extra stuff")
                .frame(minWidth: 200, maxWidth: .infinity)
        }
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
