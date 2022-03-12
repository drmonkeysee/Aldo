//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

enum ContentLinks: String, CaseIterable, Identifiable {
    case emulator = "Aldo"
    case breadboard = "Breadboard"
    case assembler = "Assembler"
    case program = "Cart PRG Data"
    case character = "Cart CHR Data"

    var id: Self { self }
    var navLabel: String { "\(self)".capitalized }
}

struct ContentView: View {
    static let fileLabel = "Open ROM File"

    @State private var fileUrl: URL?
    @State private var navSelection: ContentLinks? = .emulator

    var body: some View {
        NavigationView(content: navLinkViews)
        .toolbar {
            ToolbarItem(placement: .navigation) {
                Button(action: toggleSidebar) {
                    Label("Toggle Sidebar", systemImage: "sidebar.left")
                }
            }
            ToolbarItem {
                Button(action: pickFile) {
                    Label(ContentView.fileLabel, systemImage: "plus")
                }
                .help(ContentView.fileLabel)
            }
        }
    }

    private func navLinkViews() -> some View {
        List(ContentLinks.allCases) { link in
            NavigationLink(link.navLabel, tag: link,
                           selection: $navSelection) {
                ContentDetail(link: link, fileUrl: $fileUrl)
                    .navigationTitle(link.rawValue)
            }
        }
        .listStyle(.sidebar)
    }

    private func toggleSidebar() {
        // NOTE: seems kinda janky but oh well
        // https://stackoverflow.com/a/68331797/2476646
        NSApp.sendAction(#selector(NSSplitViewController.toggleSidebar(_:)),
                         to: nil, from: nil)
    }

    private func pickFile() {
        let panel = NSOpenPanel()
        panel.message = "Choose a ROM file"
        fileUrl = panel.runModal() == NSApplication.ModalResponse.OK
                    ? panel.url
                    : nil
    }
}

fileprivate struct ContentDetail: View {
    let link: ContentLinks
    @Binding var fileUrl: URL?

    var body: some View {
        let navPadding = EdgeInsets(top: 0, leading: 5, bottom: 5, trailing: 5)
        switch link {
        case .emulator:
            EmulatorView()
                .padding(navPadding)
        case .breadboard:
            BreadboardView()
        case .assembler:
            AssemblerView()
        case .program:
            CartPrgView(fileUrl: $fileUrl)
                .padding(navPadding)
        case .character:
            CartChrView()
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
