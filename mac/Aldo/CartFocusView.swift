//
//  CartFocusView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/8/22.
//

import SwiftUI

struct CartFocusView: View {
    var body: some View {
        VStack(alignment: .leading) {
            GroupBox {
                InstructionDetailsView()
            } label: {
                Label("Selected Instruction", systemImage: "pencil")
                    .font(.headline)
            }
            Divider()
            GroupBox {
                CartInfoView()
                    .frame(minWidth: 270, maxWidth: .infinity)
            } label: {
                Label("Cart Format", systemImage: "scribble")
                    .font(.headline)
            }
        }
    }
}

fileprivate struct InstructionDetailsView: View {
    @EnvironmentObject var listing: ProgramListing

    var body: some View {
        let inst = currentInstruction
        HStack {
            VStack {
                Text(inst?.mnemonic ?? "--").font(.title)
                Text(displayByte(inst?.byte(at: 0)))
            }
            Spacer()
            VStack {
                Text(inst?.operand ?? "--").font(.title)
                Text(operandBytes(inst))
            }
        }
        Divider()
        HStack {
            Text("--")
            Spacer()
            Text(inst?.addressMode ?? "--")
        }
        .font(.footnote)
        Divider()
        HStack {
            Text("Flags")
            Spacer()
            Image(systemName: "n.circle")
            Image(systemName: "v.circle")
            Image(systemName: "b.circle")
            Image(systemName: "d.circle")
            Image(systemName: "i.circle")
            Image(systemName: "z.circle")
            Image(systemName: "c.circle")
        }
        .imageScale(.large)
    }

    private var currentInstruction: Instruction? {
        if let line = listing.currentLine,
           case .disassembled(_, let inst) = line {
            return inst
        }
        return nil
    }

    private func operandBytes(_ inst: Instruction?) -> String {
        "\(displayByte(inst?.byte(at: 1))) \(displayByte(inst?.byte(at: 2)))"
    }

    private func displayByte(_ byte: UInt8?) -> String {
        if let b = byte { return .init(format: "(%02X)", b) }
        return ""
    }
}

struct CartFocusView_Previews: PreviewProvider {
    static var previews: some View {
        CartFocusView()
    }
}
