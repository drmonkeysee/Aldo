//
//  CartView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/31/22.
//

import SwiftUI

struct CartView: View {
    private static let romPadding = EdgeInsets(top: 5, leading: 5, bottom: 5,
                                               trailing: .zero)

    var cart: Cart

    var body: some View {
        VStack(spacing: 0) {
            HStack(alignment: .top, spacing: 0) {
                CartPrgView()
                    .padding(CartView.romPadding)
                Divider()
                CartFocusView(cart: cart)
                    .padding(5)
                    .frame(maxWidth: .infinity)
                Divider()
                CartChrView()
                    .padding(CartView.romPadding)
            }
        }
    }
}

struct CartDetailsView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartView(cart: cart)
    }
}
