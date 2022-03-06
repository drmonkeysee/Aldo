//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

enum NavLinks: CaseIterable, Identifiable {
    case emulator
    case breadboard
    case assembler
    case details

    var id: Self {
        self
    }

    var navLabel: String {
        switch self {
        case .details:
            return "Cart Details"
        default:
            return "\(self)".capitalized
        }
    }
}

struct ContentView: View {
    static let fileLabel = "Open ROM File"

    @State private var fileUrl: URL?
    @State private var navSelection: NavLinks? = .emulator

    var body: some View {
        NavigationView {
            List {
                renderNavLinks()
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

    private func renderNavLinks() -> some View {
        ForEach(NavLinks.allCases) { link in
            switch link {
            case .emulator:
                NavigationLink(link.navLabel, tag: link,
                               selection: $navSelection) {
                    EmulatorView()
                        .navigationTitle(link.navLabel)
                }
            case .breadboard:
                NavigationLink(link.navLabel, tag: link,
                               selection: $navSelection) {
                    BreadboardView()
                        .navigationTitle(link.navLabel)
                }
            case .assembler:
                NavigationLink(link.navLabel, tag: link,
                               selection: $navSelection) {
                    AssemblerView()
                        .navigationTitle(link.navLabel)
                }
            case .details:
                NavigationLink(link.navLabel, tag: link,
                               selection: $navSelection) {
                    CartDetailsView(fileUrl: $fileUrl)
                        .padding(EdgeInsets(top: 0, leading: 5, bottom: 5,
                                            trailing: 5))
                        .navigationTitle(link.navLabel)
                }
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
