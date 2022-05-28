//
//  CartFocusView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/8/22.
//

import SwiftUI

struct CartFocusView: View {
    let instruction: Instruction?

    init(instruction: Instruction? = nil) { self.instruction = instruction }

    var body: some View {
        VStack(alignment: .leading) {
            GroupBox {
                InstructionDetailsView(instruction: instruction)
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
    let instruction: Instruction?

    var body: some View {
        HStack {
            VStack {
                Text(instruction?.mnemonic ?? "--").font(.title)
                Text(displayByte(instruction?.byte(at: 0)))
            }
            Spacer()
            VStack {
                Text(instruction?.operand ?? "--").font(.title)
                Text("""
                     \(displayByte(instruction?.byte(at: 1))) \
                     \(displayByte(instruction?.byte(at: 2)))
                     """)
            }
        }
        Divider()
        HStack {
            Text("--")
            Spacer()
            Text(instruction?.addressMode ?? "--")
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

    private func displayByte(_ byte: UInt8?) -> String {
        if let b = byte { return .init(format: "(%02X)", b) }
        return ""
    }
}

struct CartFocusView_Previews: PreviewProvider {
    static var previews: some View {
        CartFocusView(instruction: .parse(dis_instruction()))
    }
}
