//
//  PrgDisassembly.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 5/3/22.
//

import Foundation

enum PrgLine: Identifiable {
    case instruction(UInt16, Disassembled)
    case elision(UInt16)
    case failure(UInt16, AldoError)

    var id: UInt16 {
        switch self {
        case .instruction(let offset, _),
                .elision(let offset),
                .failure(let offset, _):
            return offset
        }
    }
}

struct Disassembled {
    let bytes: [UInt8]

    var mnemonic: String { "JMP" }
    var operand: String { "$(8134)" }
    var name: String { "Jump" }
    var addressMode: String { "Absolute Indirect" }
    var description: String { "Unconditional jump to an address" }

    func listing(offset: UInt16) -> String {
        let addr = 0xc000 + offset
        let byteStr = bytes.reduce("") { (result, byte) in
            result.appendingFormat("%02X ", byte)
        }
        return "\(String(format: "%04X", addr)): \(byteStr.padding(toLength: 10, withPad: " ", startingAt: 0))\(mnemonic) \(operand)"
    }
}
