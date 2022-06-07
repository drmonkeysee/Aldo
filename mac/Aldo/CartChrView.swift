//
//  CartChrView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/6/22.
//

import SwiftUI

struct CartChrView: View {
    @EnvironmentObject var cart: Cart

    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Label("CHR ROM", systemImage: "photo")
                    .font(.headline)
                if case .iNes = cart.info, cart.info.chrBanks > 0 {
                    ExportView(command: ChrExport(cart))
                }
            }
            switch cart.info {
            case .iNes where cart.info.chrBanks > 0:
                ChrBanksView(banks: ChrBanks(cart))
                PaletteView()
            case .iNes:
                NoChrView(reason: "Cart uses CHR RAM")
            default:
                NoChrView(reason: "No CHR ROM Available")
            }
        }
        .padding(EdgeInsets(top: 5, leading: 5, bottom: 5, trailing: .zero))
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

fileprivate struct ChrBanksView: View {
    let banks: ChrBanks

    var body: some View {
        ScrollView(.horizontal) {
            LazyHStack {
                ForEach(0..<banks.count, id: \.self) { i in
                    ChrSheetView(sheet: banks.sheet(at: i))
                }
            }
            .padding(Constraints.sheetPadding)
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
            Text("Bank \(sheet.bank)")
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
                    Text("CHR Bank Not Available")
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
            Text("Loading CHR Bank...")
        }
        .task { await sheet.load() }
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
        .padding(.leading, Constraints.sheetPadding)
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

fileprivate struct ExportView: View {
    @ObservedObject var command: ChrExport

    var body: some View {
        HStack {
            Button(action: pickFolder) {
                Label("Export", systemImage: "arrow.up.doc")
            }
            Picker(selection: $command.scale) {
                ForEach(command.scales, id: \.self) { i in
                    Text("\(i)x")
                }
            } label: {
                Text("Scale")
            }
            .frame(width: 120)
            Button(action: openTargetFolder) {
                Image(systemName: "folder")
            }
            .disabled(!command.done)
            .help("Open export folder")
        }
        .alert("CHR Export Failure", isPresented: $command.failed,
               presenting: command.currentError) { _ in
            // NOTE: default action
        } message: { err in
            Text(err.message)
        }
    }

    private func pickFolder() {
        let panel = NSOpenPanel()
        panel.message = "Choose export folder"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        if panel.runModal() == .OK {
            Task(priority: .userInitiated) {
                await command.exportChrBanks(to: panel.url)
            }
        }
    }

    private func openTargetFolder() {
        guard let folder = command.selectedFolder,
              folder.hasDirectoryPath else {
            aldoLog.debug("Missing/invalid export target folder")
            return
        }
        NSWorkspace.shared.selectFile(nil,
                                      inFileViewerRootedAtPath: folder.path)
    }
}

struct CartChrView_Previews: PreviewProvider {
    static var previews: some View {
        CartChrView()
    }
}
