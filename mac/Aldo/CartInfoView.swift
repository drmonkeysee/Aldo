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
    private static let labels = [
        "Mapper",
        "PRG ROM",
        "WRAM",
        "CHR ROM",
        "CHR RAM",
        "NT-Mirroring",
        "Mapper-Ctrl",
        "Trainer",
        "Bus Conflicts",
    ]

    var section: InfoSection
    var info: CartInfo

    var body: some View {
        switch section {
        case .labels:
            ForEach(iNesFormatView.labels, id: \.self) { label in
                Text(label)
            }
        case .separators:
            ForEach(0..<iNesFormatView.labels.count, id: \.self) { _ in
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
