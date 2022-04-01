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
        VStack {
            Text(cart.name ?? "No file selected")
                .font(.title2)
                .truncationMode(.middle)
                .help(cart.name ?? "")
            TabView {
                CartPrgView(cart: cart)
                    .tabItem {
                        Text("PRG ROM")
                    }
                CartChrView()
                    .tabItem {
                        Text("CHR ROM")
                    }
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
