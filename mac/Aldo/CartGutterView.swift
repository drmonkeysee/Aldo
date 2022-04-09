//
//  CartGutterView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/8/22.
//

import SwiftUI

struct CartGutterView: View {
    var cart: Cart

    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                VStack {
                    Text("JMP").font(.title)
                    Text("(6C)")
                }
                Spacer()
                VStack {
                    Text("$(8134)").font(.title)
                    Text("(34) (81)")
                }
            }
            Divider()
            HStack {
                Text("Jump")
                Spacer()
                Text("Absolute Indirect")
            }
            .font(.footnote)
            Divider()
            HStack {
                Text("Flags")
                Spacer()
                Image(systemName: "n.circle")
                Image(systemName: "v.circle")
                Image(systemName: "b.circle")
                Image(systemName: "d.circle")
                Image(systemName: "i.circle")
                Image(systemName: "z.circle")
                Image(systemName: "c.circle")
            }
            .imageScale(.large)
            Divider()
            CartInfoView(cart: cart)
                .frame(minWidth: 270)
        }
    }
}

struct CartGutterView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartGutterView(cart: cart)
    }
}
