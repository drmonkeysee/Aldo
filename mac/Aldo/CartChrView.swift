//
//  CartChrView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/6/22.
//

import SwiftUI

struct CartChrView: View {
    private static let sheetSize = (w: 256.0 * 2, h: 128.0 * 2)
    private static let sheetPadding = 5.0
    private static let outerWidth = sheetSize.w + (sheetPadding * 2)

    @ObservedObject var cart: Cart

    var body: some View {
        VStack(alignment: .leading) {
            Label("CHR ROM", systemImage: "photo")
                .font(.headline)
            if case .iNes(_, let header, _) = cart.info {
                ScrollView(.horizontal) {
                    LazyHStack {
                        ForEach(0..<Int(header.chr_chunks), id: \.self) { i in
                            VStack {
                                Text("Bank \(i)")
                                    .font(.caption)
                                Color.cyan
                                    .cornerRadius(5)
                                    .frame(width: CartChrView.sheetSize.w,
                                           height: CartChrView.sheetSize.h)
                            }
                        }
                    }
                    .padding(CartChrView.sheetPadding)
                }
                .fixedSize(horizontal: false, vertical: true)
                .frame(width: CartChrView.outerWidth)
                .padding(.trailing, CartChrView.sheetPadding)
                ZStack {
                    Color.cyan
                        .cornerRadius(5)
                    Text("Palette")
                }
                .padding(.leading, CartChrView.sheetPadding)
            } else {
                GroupBox {
                    Text("No CHR ROM Available")
                        .padding()
                }
                .frame(width: CartChrView.outerWidth)
                .padding(.trailing, CartChrView.sheetPadding)
            }
        }
    }
}

struct CartChrView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartChrView(cart: cart)
    }
}
