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
            .frame(maxHeight: 250)
            .background {
                Color.cyan
            }
            HStack {
                VStack(alignment: .trailing) {
                    Group {
                        Text("File:")
                        Text("Format:")
                    }
                }
                VStack(alignment: .leading) {
                    Group {
                        Text(cart.name ?? "No rom")
                            .truncationMode(.middle)
                        Text(cart.info.name)
                    }
                }
                .padding(.vertical, 5)
            }
            DetailsView(info: cart.info)
        }
    }
}

fileprivate struct DetailsView: View {
    var info: CartInfo

    var body: some View {
        switch info {
        case .raw:
            RawFormatView()
        case .iNes:
            iNesFormatView()
        default:
            Text("No Data")
        }
    }
}

fileprivate struct RawFormatView: View {
    var body: some View {
        Text("PRG ROM:")
    }
}

fileprivate struct iNesFormatView: View {
    var body: some View {
        Text("Mapper:")
        Text("PRG ROM:")
        Text("WRAM:")
        Text("CHR ROM:")
        Text("CHR RAM:")
        Text("NT-Mirroring:")
        Text("Mapper-Ctrl:")
        Text("Trainer:")
        Text("Bus Conflicts:")
    }
}

struct CartFormatView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartInfoView()
            .environmentObject(cart)
    }
}
