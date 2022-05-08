//
//  CartPrgView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/5/22.
//

import SwiftUI

struct CartPrgView: View {
    let cart: Cart
    let selection: DisassemblySelection
    @State private var bank = 0

    var body: some View {
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
            ProgramListingView(selection: selection,
                               listing: ProgramListing(cart: cart,
                                                       bank: bank))
        }
        .frame(width: 250)
    }
}

struct ProgramListingView: View {
    @ObservedObject var selection: DisassemblySelection
    @ObservedObject var listing: ProgramListing

    var body: some View {
        switch listing.status {
        case .pending:
            let _ = listing.load()
            // TODO: show something
            EmptyView()
        case .loaded(let prg):
            List(0..<prg.count, id: \.self,
                 selection: $selection.line) { i in
                let line = prg[i]
                switch line {
                case let .disassembled(offset, inst):
                    Text(inst.listing(offset: offset))
                        .font(.system(.body, design: .monospaced))
                default:
                    Text("No inst")
                }
            }
            .cornerRadius(5)
        case .failed:
            // TODO: show something
            EmptyView()
        }
    }
}

struct CartPrgView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartPrgView(cart: cart, selection: DisassemblySelection())
    }
}
