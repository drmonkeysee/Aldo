//
//  CartChrView.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 3/6/22.
//

import SwiftUI

struct CartChrView: View {
    @EnvironmentObject var cart: Cart

    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                if case .iNes = cart.info, cart.info.chrBlocks > 0 {
                    chrLabel()
                    ChrExportView(cart)
                } else {
                    chrLabel()
                        .padding(.top, 2)
                }
            }
            switch cart.info {
            case .iNes where cart.info.chrBlocks > 0:
                ChrBlocksView(blocks: ChrBlocks(cart))
                PaletteView()
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
    static let sheetSize = (w: 256.0 * .init(ChrSheet.scale),
                            h: 128.0 * .init(ChrSheet.scale))
}

fileprivate struct ChrBlocksView: View {
    let blocks: ChrBlocks

    var body: some View {
        ScrollView(.horizontal) {
            LazyHStack {
                ForEach(0..<blocks.count, id: \.self) {
                    ChrSheetView(sheet: blocks.sheet(at: $0))
                }
            }
            .padding(EdgeInsets(
                top: Constraints.sheetPadding,
                leading: 1,
                bottom: Constraints.sheetPadding,
                trailing: Constraints.sheetPadding))
        }
        .fixedSize(horizontal: false, vertical: true)
        .frame(width: Constraints.outerWidth)
        .padding(.trailing, Constraints.sheetPadding)
    }
}

fileprivate struct ChrSheetView: View {
    @ObservedObject var sheet: ChrSheet

    var body: some View {
        VStack {
            Text("Block \(sheet.index)")
                .font(.caption)
            switch sheet.status {
            case .pending:
                PendingChrView(sheet: sheet)
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

fileprivate struct PendingChrView: View {
    let sheet: ChrSheet

    var body: some View {
        ZStack {
            Color.cyan
                .cornerRadius(Constraints.cornerRadius)
                .frame(width: Constraints.sheetSize.w,
                       height: Constraints.sheetSize.h)
            Text("Loading CHR Block...")
        }
        //.task { await sheet.load() }
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

fileprivate struct PaletteView: View {
    var body: some View {
        Text("Palette")
            .frame(width: Constraints.sheetSize.w / 2,
                   height: Constraints.sheetSize.h / 2)
            .background(.cyan)
            .cornerRadius(Constraints.cornerRadius)
            .padding(.trailing, Constraints.sheetPadding)
    }
}
