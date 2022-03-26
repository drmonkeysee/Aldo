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
            HStack {
                VStack(alignment: .trailing) {
                    CommonInfoView(section: .labels, cart: cart)
                    DetailsView(section: .labels, info: cart.info)
                }
                VStack {
                    CommonInfoView(section: .separators, cart: cart)
                    DetailsView(section: .separators, info: cart.info)
                }
                VStack(alignment: .leading) {
                    CommonInfoView(section: .values, cart: cart)
                    DetailsView(section: .values, info: cart.info)
                }
            }
        }
    }
}

fileprivate enum InfoSection {
    case labels
    case separators
    case values
}

fileprivate struct CommonInfoView: View {
    var section: InfoSection
    @ObservedObject var cart: Cart

    var body: some View {
        switch section {
        case .labels:
            Text("File")
            Text("Format")
        case .separators:
            Text(":")
            Text(":")
        case .values:
            Text(cart.name ?? "No rom")
                .truncationMode(.middle)
                .help(cart.name ?? "")
            Text(cart.info.name)
        }
    }
}

fileprivate struct DetailsView: View {
    var section: InfoSection
    var info: CartInfo

    var body: some View {
        switch info {
        case .raw:
            RawFormatView(section: section)
        case .iNes:
            iNesFormatView(section: section, info: info)
        default:
            EmptyView()
        }
    }
}

fileprivate struct RawFormatView: View {
    var section: InfoSection

    var body: some View {
        switch section {
        case .labels:
            Text("PRG ROM")
        case .separators:
            Text(":")
        case .values:
            Text("32KB")
        }
    }
}

fileprivate struct iNesFormatView: View {
    var section: InfoSection
    var info: CartInfo

    var body: some View {
        switch section {
        case .labels:
            Text("Mapper")
            Text("PRG ROM")
            Text("WRAM")
            Text("CHR ROM")
            Text("CHR RAM")
            Text("NT-Mirroring")
            Text("Mapper-Ctrl")
            Text("Trainer")
            Text("Bus Conflicts")
        case .separators:
            ForEach(0..<9) { _ in
                Text(":")
            }
        case .values:
            Text("Val")
            Text("Val")
            Text("Val")
            Text("Val")
            Text("Val")
            Text("Val")
            Text("Val")
            Text("Val")
            Text("Val")
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
