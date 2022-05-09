//
//  CartPrgView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/5/22.
//

import SwiftUI

struct CartPrgView: View {
    @EnvironmentObject var cart: Cart
    @State private var bank = 0

    var body: some View {
        NavigationView {
            VStack(alignment: .leading) {
                HStack {
                    Picker(selection: $bank) {
                        ForEach(0..<4) { i in
                            Text("Bank \(i)")
                        }
                    } label: {
                        Label("PRG ROM", systemImage: "doc.plaintext")
                            .font(.headline)
                    }
                    // TODO: make this work with prg bank
                    CopyInfoToClipboardView(cart: cart)
                }
                ProgramListingView(listing: ProgramListing(cart: cart,
                                                           bank: bank))
            }
            .frame(width: 250)
            .padding(5)
            PrgDetailView()
        }
        .onReceive(cart.$file) { _ in
            bank = 0
        }
    }
}

fileprivate struct ProgramListingView: View {
    @ObservedObject var listing: ProgramListing

    var body: some View {
        switch listing.status {
        case .pending:
            PendingPrgView(listing)
        case .loaded(let prg):
            List(0..<prg.count, id: \.self) { i in
                let line = prg[i]
                switch line {
                case let .disassembled(offset, inst):
                    NavigationLink(tag: i, selection: $listing.selectedLine) {
                        PrgDetailView(inst)
                    } label: {
                        Text(inst.line(offset: offset))
                            .font(.system(.body, design: .monospaced))
                    }
                default:
                    // TODO: handle all PrgLine types
                    Text("No inst")
                }
            }
            .cornerRadius(5)
        case .failed:
            NoPrgView(reason: "PRG Bank Not Available")
        }
    }
}

fileprivate struct PendingPrgView: View {
    init(_ listing: ProgramListing) {
        listing.load()
    }

    var body: some View {
        NoPrgView(reason: "Loading PRG Bank...")
    }
}

fileprivate struct NoPrgView: View {
    let reason: String

    var body: some View {
        GroupBox {
            Text(reason)
                .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
    }
}

fileprivate struct PrgDetailView: View {
    let instruction: Instruction?

    init(_ instruction: Instruction? = nil) {
        self.instruction = instruction
    }

    var body: some View {
        CartFocusView(instruction: instruction)
            .frame(maxHeight: .infinity, alignment: .top)
            .padding(5)
    }
}

struct CartPrgView_Previews: PreviewProvider {
    static var previews: some View {
        CartPrgView()
    }
}
