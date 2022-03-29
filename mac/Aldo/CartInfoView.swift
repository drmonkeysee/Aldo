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
            HStack(alignment: .top) {
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
                if cart.file != nil {
                    CopyToClipboardView(cart: cart)
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
    private static let labels = ["File", "Format"]

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
        }
    }
}

fileprivate struct DetailValuesView: View {
    let info: CartInfo

    var body: some View {
        switch info {
        case .raw:
            RawDetailsView()
        case .iNes(_, let hdr, let mirrorName):
            iNesDetailsView(header: hdr, mirrorName: mirrorName)
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
        "Boards",
        "PRG ROM",
        "WRAM",
        "CHR ROM",
        "CHR RAM",
        "NT-Mirroring",
        "Mapper-Ctrl",
        "Trainer",
        "Bus Conflicts",
    ]
    private static let fullSize = "x 16KB"
    private static let halfSize = "x 8KB"

    let header: ines_header
    let mirrorName: String

    var body: some View {
        Text(String(format: "%03u%@", header.mapper_id,
                    header.mapper_implemented ? "" : " (Not Implemented)"))
        Text("<Board Names>")
        Text("\(header.prg_chunks) \(iNesDetailsView.fullSize)")
        Text(header.wram ? wramStr : "no")
        if header.chr_chunks > 0 {
            Text("\(header.chr_chunks) \(iNesDetailsView.halfSize)")
            Text("no")
        } else {
            Text("no")
            Text("1 \(iNesDetailsView.halfSize)")
        }
        Text(mirrorName)
        Text(boolToStr(header.mapper_controlled))
        Text(boolToStr(header.trainer))
        Text(boolToStr(header.bus_conflicts))
    }

    private var wramStr: String {
        let chunkCount = header.wram_chunks > 0 ? header.wram_chunks : 1
        return "\(chunkCount) \(iNesDetailsView.halfSize)"
    }
}

fileprivate func boolToStr(_ val: Bool) -> String { val ? "yes" : "no" }

struct CartFormatView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CartInfoView()
            .environmentObject(cart)
    }
}
