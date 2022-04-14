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
            if case .iNes(_, let header, _) = cart.info {
                ChrBanksView(cart: cart, bankCount: Int(header.chr_chunks))
            } else {
                NoChrView()
            }
            PaletteView()
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
                    VStack {
                        Text("Bank \(i)")
                            .font(.caption)
                        if let img = cart.getChrBank(bank: i) {
                            Image(nsImage: img)
                                .interpolation(.none)
                        } else {
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
            .padding(Constraints.sheetPadding)
        }
        .fixedSize(horizontal: false, vertical: true)
        .frame(width: Constraints.outerWidth)
        .padding(.trailing, Constraints.sheetPadding)
    }
}

fileprivate struct NoChrView: View {
    var body: some View {
        GroupBox {
            Text("No CHR ROM Available")
                .padding()
        }
        .frame(width: Constraints.outerWidth)
        .padding(.trailing, Constraints.sheetPadding)
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
