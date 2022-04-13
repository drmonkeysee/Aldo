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

    var body: some View {
        VStack(alignment: .leading) {
            Label("CHR ROM", systemImage: "photo")
                .font(.headline)
            ScrollView(.horizontal) {
                LazyHStack {
                    ForEach(1..<5) { i in
                        VStack {
                            Text("Bank \(i)").font(.caption)
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
            .frame(width: CartChrView.sheetSize.w
                   + (CartChrView.sheetPadding * 2))
            .padding(.trailing, CartChrView.sheetPadding)
            ZStack {
                Color.cyan
                    .cornerRadius(5)
                Text("Palette")
            }
            .padding(.leading, CartChrView.sheetPadding)
        }
    }
}

struct CartChrView_Previews: PreviewProvider {
    static var previews: some View {
        CartChrView()
    }
}
