//
//  CartInfoView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/6/22.
//

import SwiftUI

struct CartInfoView: View {
    @EnvironmentObject var cart: Cart

    var body: some View {
        VStack {
            ScrollView {
                Text(cart.infoText ?? "No rom info")
                    .padding()
            }
            .background {
                Color.cyan
            }
            HStack {
                VStack(alignment: .trailing) {
                    Group {
                        Text("File:")
                        Text("Format:")
                    }
                    Group {
                        Text("Mapper:")
                    }
                    Group {
                        Text("PRG ROM:")
                        Text("WRAM:")
                        Text("CHR ROM:")
                        Text("CHR RAM:")
                        Text("NT-Mirroring:")
                        Text("Mapper-Ctrl:")
                    }
                    Group {
                        Text("Trainer:")
                        Text("Bus Conflicts:")
                    }
                }
                VStack(alignment: .leading) {
                    Group {
                        Text(cart.name ?? "No rom")
                            .truncationMode(.middle)
                        Text(cart.info.name)
                    }
                    Group {
                        Text("000 (<Board Names>)")
                    }
                    Group {
                        Text("2 x 16KB")
                        Text("no")
                        Text("1 x 8KB")
                        Text("no")
                        Text("Vertical")
                        Text("no")
                    }
                    Group {
                        Text("no")
                        Text("no")
                    }
                }
                .padding(.vertical, 5)
            }
        }
    }
}

struct CartFormatView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartInfoView()
            .environmentObject(cart)
    }
}
