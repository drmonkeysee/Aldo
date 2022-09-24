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
                InstructionDefinitionView()
            } label: {
                Label("Selected Instruction", systemImage: "pencil")
                    .font(.headline)
            }
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

fileprivate struct InstructionDefinitionView: View {
    static private let blank = "--"

    @EnvironmentObject var listing: ProgramListing

    var body: some View {
        let inst = currentInstruction
        HStack {
            VStack {
                Text(inst?.mnemonic ?? Self.blank)
                    .font(.title)
                Text(displayByte(inst?.byte(at: 0)))
            }
            Spacer()
            VStack {
                Text(inst?.operand ?? Self.blank)
                    .font(.title)
                Text(operandBytes(inst))
            }
        }
        Divider()
        HStack {
            Text(inst?.description ?? Self.blank)
            Spacer()
            Text(inst?.addressMode ?? Self.blank)
        }
        .font(.footnote)
        Divider()
        Text(inst?.cycleCount ?? Self.blank)
            .font(.footnote)
            .frame(maxWidth: .infinity, alignment: .leading)
        Divider()
        HStack {
            Text("Flags")
            Spacer()
            FlagsView(flags: inst?.flags)
        }
        .imageScale(.large)
        HStack {
            Text("Data")
            Spacer()
            DataView(cells: inst?.cells)
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

fileprivate struct FlagsView: View {
    let flags: CpuFlags?

    var body: some View {
        Image(systemName: icon(val: flags?.negative ?? false, label: "n"))
            .help("Negative")
        Image(systemName: icon(val: flags?.overflow ?? false, label: "v"))
            .help("Overflow")
        Image(systemName: icon(val: flags?.softbreak ?? false, label: "b"))
            .help("Soft-Break")
        Image(systemName: icon(val: flags?.decimal ?? false, label: "d"))
            .help("Decimal Mode")
        Image(systemName: icon(val: flags?.interrupt ?? false, label: "i"))
            .help("Interrupts Disabled")
        Image(systemName: icon(val: flags?.zero ?? false, label: "z"))
            .help("Zero")
        Image(systemName: icon(val: flags?.carry ?? false, label: "c"))
            .help("Carry")
    }

    private func icon(val: Bool, label: String) -> String {
        "\(label).circle\(val ? ".fill" : "")"
    }
}

fileprivate struct DataView: View {
    let cells: DataCells?

    var body: some View {
        Group {
            Image(systemName: icon(val: cells?.memory ?? false, label: "m"))
                .help("Main Memory")
            Image(systemName: icon(val: cells?.programCounter ?? false,
                                   label: "i"))
                .help("Instruction Register (Program Counter)")
            Image(systemName: icon(val: cells?.stackPointer ?? false,
                                   label: "s"))
                .help("Stack Pointer")
            Image(systemName: icon(val: cells?.processorStatus ?? false,
                                   label: "p"))
                .help("Processor Status")
            Image(systemName: icon(val: cells?.yIndex ?? false, label: "y"))
                .help("Y-Index")
            Image(systemName: icon(val: cells?.xIndex ?? false, label: "x"))
                .help("X-Index")
            Image(systemName: icon(val: cells?.accumulator ?? false,
                                   label: "a"))
                .help("Accumulator")
        }
        .padding(.trailing, 1)
    }

    private func icon(val: Bool, label: String) -> String {
        "\(label).square\(val ? ".fill" : "")"
    }
}

struct CartFocusView_Previews: PreviewProvider {
    static var previews: some View {
        CartFocusView()
    }
}
