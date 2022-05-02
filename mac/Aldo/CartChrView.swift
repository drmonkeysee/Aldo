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
    static let cornerRadius = 5.0
    static let groupboxPadding = 10.0
    static let outerWidth = sheetSize.w + (sheetPadding * 2)
    static let sheetPadding = 5.0
    static let sheetSize = (w: 256.0 * 2, h: 128.0 * 2)
}

fileprivate struct ChrBanksView: View {
    @ObservedObject var cart: Cart
    let bankCount: Int

    var body: some View {
        ScrollView(.horizontal) {
            LazyHStack {
                ForEach(0..<bankCount, id: \.self) { i in
                    ChrSheetView(sheet: ChrSheet(cart, bank: i))
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
    @ObservedObject var sheet: ChrSheet

    var body: some View {
        VStack {
            Text("Bank \(sheet.bank)")
                .font(.caption)
            switch sheet.status {
            case .pending:
                PendingChrView(sheet)
            case .loaded(let img):
                Image(nsImage: img)
                    .interpolation(.none)
                    .cornerRadius(Constraints.cornerRadius)
                    .overlay(
                        RoundedRectangle(
                            cornerRadius: Constraints.cornerRadius)
                        .stroke(.white, lineWidth: 1))
            case .failed:
                ZStack {
                    Color.cyan
                        .cornerRadius(Constraints.cornerRadius)
                        .frame(width: Constraints.sheetSize.w,
                               height: Constraints.sheetSize.h)
                    Text("CHR Bank Not Available")
                }
            }
        }
    }
}

fileprivate struct PendingChrView: View {
    init(_ sheet: ChrSheet) { sheet.load() }

    var body: some View {
        ZStack {
            Color.cyan
                .cornerRadius(Constraints.cornerRadius)
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
                .frame(width: Constraints.sheetSize.w
                                - Constraints.groupboxPadding,
                       height: Constraints.sheetSize.h
                                - Constraints.groupboxPadding)
        }
        .padding(.leading, Constraints.sheetPadding)
        .padding(.trailing, Constraints.sheetPadding * 2)
    }
}

fileprivate struct PaletteView: View {
    var body: some View {
        ZStack {
            Color.cyan
                .cornerRadius(Constraints.cornerRadius)
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
