//
//  CartChrView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/6/22.
//

import SwiftUI

struct CartChrView: View {
    @ObservedObject var cart: Cart

    var body: some View {
        VStack(alignment: .leading) {
            Label("CHR ROM", systemImage: "photo")
                .font(.headline)
            switch cart.info {
            case .iNes(_, let header, _) where header.chr_chunks > 0:
                ChrBanksView(cart: cart, bankCount: Int(header.chr_chunks))
                PaletteView()
            case .iNes:
                NoChrView(reason: "Cart uses CHR RAM")
            default:
                NoChrView(reason: "No CHR ROM Available")
            }
        }
    }
}

fileprivate struct Constraints {
    static let sheetSize = (w: 256.0 * 2, h: 128.0 * 2)
    static let sheetPadding = 5.0
    static let outerWidth = sheetSize.w + (sheetPadding * 2)
}

fileprivate struct ChrBanksView: View {
    @ObservedObject var cart: Cart
    let bankCount: Int

    var body: some View {
        ScrollView(.horizontal) {
            LazyHStack {
                ForEach(0..<bankCount, id: \.self) { i in
                    ChrSheetView(cart: cart, bank: i)
                }
            }
            .padding(Constraints.sheetPadding)
        }
        .fixedSize(horizontal: false, vertical: true)
        .frame(width: Constraints.outerWidth)
        .padding(.trailing, Constraints.sheetPadding)
    }
}

fileprivate struct ChrSheetView: View {
    @ObservedObject var cart: Cart
    let bank: Int

    @StateObject private var command = LoadChrSheet()

    var body: some View {
        VStack {
            Text("Bank \(bank)")
                .font(.caption)
            switch command.status {
            case .pending:
                PendingChrView(command, cart, bank)
            case .loaded(let img):
                Image(nsImage: img)
                    .interpolation(.none)
            case .failed:
                ZStack {
                    Color.cyan
                        .cornerRadius(5)
                        .frame(width: Constraints.sheetSize.w,
                               height: Constraints.sheetSize.h)
                    Text("CHR Bank Not Available")
                }
            }
        }
    }
}

fileprivate struct PendingChrView: View {
    init(_ cmd: LoadChrSheet, _ cart: Cart, _ bank: Int) {
        cmd.execute(cart: cart, bank: bank)
    }

    var body: some View {
        ZStack {
            Color.cyan
                .cornerRadius(5)
                .frame(width: Constraints.sheetSize.w,
                       height: Constraints.sheetSize.h)
            Text("Loading CHR Bank...")
        }
    }
}

fileprivate struct NoChrView: View {
    let reason: String

    var body: some View {
        GroupBox {
            Text(reason)
                .frame(width: Constraints.sheetSize.w,
                       height: Constraints.sheetSize.h)
        }
        .frame(width: Constraints.outerWidth)
        .padding(.leading, Constraints.sheetPadding)
        .padding(.trailing, Constraints.sheetPadding * 2)
    }
}

fileprivate struct PaletteView: View {
    var body: some View {
        ZStack {
            Color.cyan
                .cornerRadius(5)
            Text("Palette")
        }
        .padding(.leading, Constraints.sheetPadding)
    }
}

struct CartChrView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartChrView(cart: cart)
    }
}
