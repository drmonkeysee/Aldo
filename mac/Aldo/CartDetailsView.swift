//
//  CartDetailsView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/31/22.
//

import SwiftUI

struct CartDetailsView: View {
    @ObservedObject var cart: Cart

    var body: some View {
        VStack(spacing: 0) {
            Text(cart.name ?? "No file selected")
                .font(.title2)
                .truncationMode(.middle)
                .help(cart.name ?? "")
                .padding(.vertical, 5)
            Divider()
            HStack(alignment: .top, spacing: 0) {
                CartPrgView()
                    .padding(EdgeInsets(top: 5, leading: 5, bottom: 5,
                                        trailing: .zero))
                Divider()
                CartGutterView(cart: cart)
                    .padding(EdgeInsets(top: .zero, leading: 5, bottom: 5,
                                        trailing: 5))
                    .frame(maxWidth: .infinity)
                Divider()
                CartChrView()
                    .padding(.bottom, 5)
            }
        }
    }
}

struct CartDetailsView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartDetailsView(cart: cart)
    }
}
