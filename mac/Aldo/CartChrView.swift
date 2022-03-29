//
//  CartChrView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/6/22.
//

import SwiftUI

struct CartChrView: View {
    private static let sheetSize = (w: 256, h: 128)

    var body: some View {
        VStack {
            ScrollView(.horizontal) {
                HStack {
                    ForEach(1..<5) { i in
                        VStack {
                            Text("Bank \(i)").font(.caption)
                            Color.cyan
                                .cornerRadius(5)
                                .frame(width: Double(CartChrView.sheetSize.w)
                                                / 2.0,
                                       height: Double(CartChrView.sheetSize.h)
                                                / 2.0)
                        }
                    }
                }
                .border(.blue)
            }
            .border(.red)
            ZStack {
                Color.cyan
                    .cornerRadius(5)
                    .frame(width: Double(CartChrView.sheetSize.w) * 2.0,
                           height: Double(CartChrView.sheetSize.h) * 2.0)
                Text("No CHR Data")
            }
            ZStack {
                Color.cyan
                    .cornerRadius(5)
                Text("Palette")
            }
        }
        .padding()
    }
}

struct CartChrView_Previews: PreviewProvider {
    static var previews: some View {
        CartChrView()
    }
}
