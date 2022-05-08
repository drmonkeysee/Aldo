//
//  CartView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/31/22.
//

import SwiftUI

struct CartView: View {
    let cart: Cart

    var body: some View {
        VStack(spacing: 0) {
            HStack(alignment: .top, spacing: 0) {
                CartPrgView(cart: cart)
                Divider()
                CartChrView(cart: cart)
            }
        }
    }
}

struct CartView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartView(cart: cart)
    }
}
