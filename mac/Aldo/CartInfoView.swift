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
    private static let labels = [
        "File",
        "Format",
    ]

    let section: InfoSection
    @ObservedObject var cart: Cart

    var body: some View {
        switch section {
        case .labels:
            DetailLabelsView(labels: CommonInfoView.labels)
        case .separators:
            DetailSeparatorsView(count: CommonInfoView.labels.count)
        case .values:
            Text(cart.name ?? "No rom")
                .truncationMode(.middle)
                .help(cart.name ?? "")
            Text(cart.info.name)
        }
    }
}

fileprivate struct DetailsView: View {
    let section: InfoSection
    let info: CartInfo

    var body: some View {
        switch section {
        case .labels:
            DetailLabelsView(labels: labels)
        case .separators:
            DetailSeparatorsView(count: labels?.count ?? 0)
        case .values:
            DetailValuesView(info: info)
        }
    }

    private var labels: [String]? {
        switch info {
        case .raw:
            return RawDetailsView.labels
        case .iNes:
            return iNesDetailsView.labels
        default:
            return nil
        }
    }
}

fileprivate struct DetailLabelsView: View {
    let labels: [String]?

    var body: some View {
        if let viewLabels = labels {
            ForEach(viewLabels, id: \.self) { label in
                Text(label)
            }
        } else {
            EmptyView()
        }
    }
}

fileprivate struct DetailSeparatorsView: View {
    let count: Int

    var body: some View {
        if count > 0 {
            ForEach(0..<count, id: \.self) { _ in
                Text(":")
            }
        } else {
            EmptyView()
        }
    }
}

fileprivate struct DetailValuesView: View {
    let info: CartInfo

    var body: some View {
        switch info {
        case .raw:
            RawDetailsView()
        case .iNes:
            iNesDetailsView(info: info)
        default:
            EmptyView()
        }
    }
}

fileprivate struct RawDetailsView: View {
    static let labels = ["PRG ROM"]

    var body: some View {
        Text("32KB")
    }
}

fileprivate struct iNesDetailsView: View {
    static let labels = [
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

    let info: CartInfo

    var body: some View {
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

struct CartFormatView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartInfoView()
            .environmentObject(cart)
    }
}
