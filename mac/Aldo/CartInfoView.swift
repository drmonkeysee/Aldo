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
                HStack(alignment: .top) {
                    VStack(alignment: .trailing) {
                        CommonInfoView(section: .labels)
                            .textSelection(.enabled)
                        FormatView(section: .labels, info: cart.info)
                            .textSelection(.enabled)
                    }
                    VStack {
                        CommonInfoView(section: .separators)
                        FormatView(section: .separators, info: cart.info)
                    }
                    VStack(alignment: .leading) {
                        CommonInfoView(section: .values)
                            .textSelection(.enabled)
                        FormatView(section: .values, info: cart.info)
                            .textSelection(.enabled)
                    }
                    switch cart.info {
                    case .raw, .iNes:
                        CopyToClipboardView(fromStream: cart.readInfoText)
                    default:
                        EmptyView()
                    }
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
    @EnvironmentObject var cart: Cart

    var body: some View {
        switch section {
        case .labels:
            FormatLabelsView(labels: Self.labels)
        case .separators:
            FormatSeparatorsView(count: Self.labels.count)
        case .values:
            Text(cart.fileName ?? "No rom")
                .lineLimit(1)
                .truncationMode(.middle)
                .help(cart.fileName ?? "")
            Text(cart.info.name)
        }
    }
}

fileprivate struct FormatView: View {
    let section: InfoSection
    let info: CartInfo

    var body: some View {
        switch section {
        case .labels:
            FormatLabelsView(labels: labels)
        case .separators:
            FormatSeparatorsView(count: labels?.count ?? 0)
        case .values:
            FormatValuesView(info: info)
        }
    }

    private var labels: [String]? {
        switch info {
        case .raw:
            return RawView.labels
        case .iNes:
            return iNesView.labels
        default:
            return nil
        }
    }
}

fileprivate struct FormatLabelsView: View {
    let labels: [String]?

    var body: some View {
        if let viewLabels = labels {
            ForEach(viewLabels, id: \.self) {
                Text($0)
            }
        }
    }
}

fileprivate struct FormatSeparatorsView: View {
    let count: Int

    var body: some View {
        if count > 0 {
            ForEach(0..<count, id: \.self) { _ in
                Text(":")
            }
        }
    }
}

fileprivate struct FormatValuesView: View {
    let info: CartInfo

    var body: some View {
        switch info {
        case .raw:
            RawView()
        case .iNes(_, let hdr, let mirrorName):
            iNesView(header: hdr, mirrorName: mirrorName)
        default:
            EmptyView()
        }
    }
}

fileprivate struct RawView: View {
    static let labels = ["PRG ROM"]

    var body: some View {
        Text("32KB")
    }
}

fileprivate struct iNesView: View {
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
        VStack(alignment: .leading) {
            Text(String(format: "%03u%@", header.mapper_id,
                        header.mapper_implemented ? "" : " (Not Implemented)"))
            Text("<Board Names>")
            Text("\(header.prg_blocks) \(Self.fullSize)")
            Text(header.wram ? wramStr : "no")
            if header.chr_blocks > 0 {
                Text("\(header.chr_blocks) \(Self.halfSize)")
                Text("no")
            } else {
                Text("no")
                Text("1 \(Self.halfSize)")
            }
            Text(mirrorName)
            Text(boolToStr(header.mapper_controlled))
            Text(boolToStr(header.trainer))
            Text(boolToStr(header.bus_conflicts))
        }
    }

    private var wramStr: String {
        let blockCount = header.wram_blocks > 0 ? header.wram_blocks : 1
        return "\(blockCount) \(Self.halfSize)"
    }
}

fileprivate func boolToStr(_ val: Bool) -> String { val ? "yes" : "no" }

struct CartInfoView_Previews: PreviewProvider {
    static var previews: some View {
        CartInfoView()
    }
}
