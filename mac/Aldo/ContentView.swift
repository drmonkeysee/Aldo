//
//  ContentView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 2/24/22.
//

import SwiftUI

struct ContentView: View {
    private static let fileLabel = "Open ROM File"

    @State private var navSelection: ContentLinks? = .emulator
    @State private var cartLoadFailed: Bool = false
    @StateObject private var cart = Cart()

    var body: some View {
        NavigationView(content: navLinkViews)
    }

    private func navLinkViews() -> some View {
        List(ContentLinks.allCases) { link in
            NavigationLink(link.navLabel, tag: link,
                           selection: $navSelection) {
                ContentDetail(link: link, cart: cart)
                    .navigationTitle(link.rawValue)
                    .toolbar {
                        ToolbarItem(placement: .navigation) {
                            Button(action: toggleSidebar) {
                                Label("Toggle Sidebar",
                                      systemImage: "sidebar.left")
                            }
                        }
                        ToolbarItem {
                            Button(action: pickFile) {
                                Label(ContentView.fileLabel,
                                      systemImage: "plus")
                            }
                            .help(ContentView.fileLabel)
                        }
                    }
                    .alert("Rom Load Failure", isPresented: $cartLoadFailed,
                           presenting: cart.currentError) { _ in
                        // NOTE: default action
                    } message: { err in
                        Text(err.message)
                    }
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
        if panel.runModal() == NSApplication.ModalResponse.OK {
            cartLoadFailed = !cart.load(from: panel.url)
        }
    }
}

fileprivate enum ContentLinks: String, CaseIterable, Identifiable {
    case emulator = "Aldo"
    case breadboard = "Breadboard"
    case assembler = "Assembler"
    case cart = "Cart Details"

    var id: Self { self }
    var navLabel: String { "\(self)".capitalized }
}

fileprivate struct ContentDetail: View {
    let link: ContentLinks
    @ObservedObject var cart: Cart

    var body: some View {
        switch link {
        case .emulator:
            EmulatorView()
                .environmentObject(cart)
                .padding(5)
        case .breadboard:
            BreadboardView()
        case .assembler:
            AssemblerView()
        case .cart:
            CartDetailsView(cart: cart)
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
