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
        ProgramView(banks: ProgramBanks(cart))
    }
}

fileprivate struct ProgramView: View {
    @ObservedObject var banks: ProgramBanks

    var body: some View {
        HStack(alignment: .top, spacing: 0) {
            VStack(alignment: .leading) {
                if banks.count > 0 {
                    BankSelectionView(banks: banks)
                    ProgramListingView()
                } else {
                    prgLabel()
                        .padding(.top, 2)
                    NoPrgView(reason: "No PRG ROM Available")
                }
            }
            .frame(width: 255)
            .padding(5)
            Divider()
            CartFocusView()
                .frame(maxHeight: .infinity, alignment: .top)
                .padding(5)
        }
        .environmentObject(banks.currentListing)
    }
}

fileprivate struct BankSelectionView: View {
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
            CopyToClipboardView(
                command: ClipboardCopy(fromStream: banks.cart.readAllPrgBanks))
        }
    }
}

fileprivate struct ProgramListingView: View {
    @EnvironmentObject var listing: ProgramListing

    var body: some View {
        switch listing.status {
        case .pending:
            PendingPrgView(listing: listing)
        case let .loaded(prg):
            LoadedPrgView(prgLines: prg, selectedLine: $listing.selectedLine)
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

fileprivate struct LoadedPrgView: View {
    let prgLines: [PrgLine]
    let selectedLine: Binding<Int?>

    var body: some View {
        List(0..<prgLines.count, id: \.self, selection: selectedLine) { i in
            let line = prgLines[i]
            Text(line.display)
                .font(.system(.body, design: .monospaced))
                .frame(maxWidth: .infinity, alignment: lineAlignment(line))
                .padding(.horizontal, 2)
                .background(PrgLineBackground(line: line))
        }
        .cornerRadius(5)
    }

    private func lineAlignment(_ line: PrgLine) -> Alignment {
        if case .elision = line { return .center }
        return .leading
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

fileprivate struct PrgLineBackground: View {
    let line: PrgLine

    var body: some View {
        if let c = bgColor {
            RoundedRectangle(cornerRadius: 5)
                .fill(c)
                .opacity(0.3)
        }
    }

    private var bgColor: Color? {
        switch line {
        case .disassembled(_, let inst) where inst.unofficial:
            return .cyan
        case .failure:
            return .red
        default:
            return nil
        }
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
