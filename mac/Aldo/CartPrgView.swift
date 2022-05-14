//
//  CartPrgView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/5/22.
//

import SwiftUI

struct CartPrgView: View {
    @EnvironmentObject var cart: Cart

    var body: some View {
        NavigationView {
            VStack(alignment: .leading) {
                if cart.info.prgBanks > 0 {
                    ProgramView(banks: ProgramBanks(cart))
                } else {
                    prgLabel()
                        .padding(.top, 2)
                    NoPrgView(reason: "No PRG ROM Available")
                }
            }
            .frame(width: 250)
            .padding(5)
            PrgDetailView()
        }
    }
}

fileprivate struct ProgramView: View {
    @ObservedObject var banks: ProgramBanks

    var body: some View {
        HStack {
            Picker(selection: $banks.selectedBank) {
                ForEach(0..<banks.count, id: \.self) { i in
                    Text("Bank \(i)")
                }
            } label: {
                prgLabel()
            }
            // TODO: make this work with prg bank
            CopyInfoToClipboardView(cart: banks.cart)
        }
        ProgramListingView(listing: banks.currentListing)
    }
}

fileprivate struct ProgramListingView: View {
    @ObservedObject var listing: ProgramListing

    var body: some View {
        switch listing.status {
        case .pending:
            PendingPrgView(listing: listing)
        case let .loaded(prg):
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
    let listing: ProgramListing

    var body: some View {
        NoPrgView(reason: "Loading PRG Bank...")
            .task { await listing.load() }
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

    init(_ instruction: Instruction? = nil) { self.instruction = instruction }

    var body: some View {
        CartFocusView(instruction: instruction)
            .frame(maxHeight: .infinity, alignment: .top)
            .padding(5)
    }
}

fileprivate func prgLabel() -> some View {
    Label("PRG ROM", systemImage: "doc.plaintext")
        .font(.headline)
}

struct CartPrgView_Previews: PreviewProvider {
    static var previews: some View {
        CartPrgView()
    }
}
