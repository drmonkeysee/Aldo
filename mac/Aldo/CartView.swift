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
                let prgSelection = DisassemblySelection()
                CartPrgView(cart: cart, selection: prgSelection)
                    .padding(5)
                Divider()
                CartFocusView(cart: cart, prgSelection: prgSelection)
                    .padding(5)
                    .frame(maxWidth: .infinity)
                Divider()
                CartChrView(cart: cart)
                    .padding(EdgeInsets(top: 5, leading: 5, bottom: 5,
                                        trailing: .zero))
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
