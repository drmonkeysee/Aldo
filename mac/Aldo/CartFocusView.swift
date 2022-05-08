//
//  CartFocusView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/8/22.
//

import SwiftUI

struct CartFocusView: View {
    let cart: Cart
    let instruction: Instruction?

    init(cart: Cart, instruction: Instruction? = nil) {
        self.cart = cart
        self.instruction = instruction
    }

    var body: some View {
        VStack(alignment: .leading) {
            GroupBox {
                InstructionDetailsView(instruction: instruction)
            } label: {
                Label("Selected Instruction", systemImage: "pencil")
                    .font(.headline)
            }
            Divider()
            GroupBox {
                CartInfoView(cart: cart)
                    .frame(minWidth: 270, maxWidth: .infinity)
            } label: {
                Label("Cart Format", systemImage: "scribble")
                    .font(.headline)
            }
        }
    }
}

struct InstructionDetailsView: View {
    let instruction: Instruction?

    var body: some View {
        HStack {
            VStack {
                Text(instruction?.mnemonic ?? "--").font(.title)
                Text("")
            }
            Spacer()
            VStack {
                Text("--").font(.title)
                Text("")
            }
        }
        Divider()
        HStack {
            Text("--")
            Spacer()
            Text("--")
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
    }
}

struct CartFocusView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartFocusView(cart: cart, instruction: Instruction(bytes: [0x0]))
    }
}
