//
//  CartChrView.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 3/6/22.
//

import SwiftUI

struct CartChrView: View {
    @Environment(Cart.self) var cart: Cart

    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                if case .iNes = cart.info.format, cart.info.format.chrBlocks > 0 {
                    chrLabel()
                    ChrExportView(cart)
                } else {
                    chrLabel()
                        .padding(.top, 2)
                }
            }
            switch cart.info.format {
            case .iNes where cart.info.format.chrBlocks > 0:
                ChrBlocksView(blocks: cart.chr)
            case .iNes:
                NoChrView(reason: "Cart uses CHR RAM")
            default:
                NoChrView(reason: "No CHR ROM Available")
            }
        }
        .padding([.leading, .top, .bottom], 5)
    }

    private func chrLabel() -> some View {
        Label("CHR ROM", systemImage: "photo")
            .font(.headline)
    }
}

fileprivate struct Constraints {
    static let cornerRadius = 5.0
    static let groupboxPadding = 10.0
    static let outerWidth = sheetSize.w + groupboxPadding
    static let sheetPadding = 5.0
    static let sheetSize = (w: 256.0 * .init(ChrBlocks.scale),
                            h: 128.0 * .init(ChrBlocks.scale))
}

fileprivate struct ChrBlocksView: View {
    let blocks: ChrBlocks

    var body: some View {
        ScrollView(.vertical) {
            LazyVStack {
                ForEach(0..<blocks.count, id: \.self) {
                    ChrSheetView(ordinal: $0, sheet: blocks.sheet(at: $0))
                }
            }
            .padding(EdgeInsets(
                top: Constraints.sheetPadding,
                leading: 1,
                bottom: Constraints.sheetPadding,
                trailing: Constraints.sheetPadding))
        }
        .fixedSize(horizontal: true, vertical: false)
        .frame(width: Constraints.outerWidth)
        .padding(.trailing, Constraints.sheetPadding)
    }
}

fileprivate struct ChrSheetView: View {
    let ordinal: Int
    var sheet: ChrSheet

    var body: some View {
        VStack {
            Text("Block \(ordinal)")
                .font(.headline)
            switch sheet.status {
            case .none:
                NoChrView(reason: "No CHR Data Loaded")
            case let .loaded(img):
                Image(nsImage: img)
                    .interpolation(.none)
                    .cornerRadius(Constraints.cornerRadius)
                    .overlay(
                        RoundedRectangle(
                            cornerRadius: Constraints.cornerRadius)
                        .stroke(.white, lineWidth: 1))
            case .failed:
                ZStack {
                    Color.cyan
                        .cornerRadius(Constraints.cornerRadius)
                        .frame(width: Constraints.sheetSize.w,
                               height: Constraints.sheetSize.h)
                    Text("CHR Block Not Available")
                }
            }
        }
    }
}

fileprivate struct NoChrView: View {
    let reason: String

    var body: some View {
        GroupBox {
            Text(reason)
                .frame(width: Constraints.sheetSize.w
                                - Constraints.groupboxPadding,
                       height: Constraints.sheetSize.h
                                - Constraints.groupboxPadding)
        }
        .padding(.trailing, Constraints.groupboxPadding)
    }
}
