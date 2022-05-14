//
//  Disassembly.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 5/3/22.
//

import Foundation

final class ProgramBanks: ObservableObject {
    let cart: Cart
    @Published var selectedBank = 0

    var count: Int { cart.info.prgBanks }

    var currentListing: ProgramListing {
        ProgramListing(cart, bank: selectedBank)
    }

    init(_ cart: Cart) { self.cart = cart }
}

final class ProgramListing: ObservableObject {
    let cart: Cart
    let bank: Int
    @Published var selectedLine: Int?
    @Published private(set) var status = BankLoadStatus<[PrgLine]>.pending

    init(_ cart: Cart, bank: Int) {
        self.cart = cart
        self.bank = bank
    }

    func load() {
        if let listing = cart.prgListingCache[bank] {
            status = .loaded(listing)
            return
        }
        DispatchQueue.global(qos: .background).async {
            sleep(1)
            let prgListing = (0..<100).map { i in
                PrgLine.disassembled(i, Instruction(bytes: [0x6c, 0x34, 0x81]))
            }
            DispatchQueue.main.async {
                self.cart.prgListingCache[self.bank] = prgListing
                self.status = .loaded(prgListing)
            }
        }
    }
}

enum PrgLine {
    case disassembled(UInt16, Instruction)
    case elision(UInt16)
    case failure(UInt16, AldoError)
}

struct Instruction {
    let bytes: [UInt8]
    //let decoding: decoded

    var mnemonic: String { "JMP" }
    var operand: String { "$(8134)" }
    var name: String { "Jump" }
    var addressMode: String { "Absolute Indirect" }
    var description: String { "Unconditional jump to an address" }
    var flags: UInt8 { 0x34 }

    func line(offset: UInt16) -> String {
        let addr = 0xc000 + offset
        let byteStr = bytes.reduce("") { (result, byte) in
            result.appendingFormat("%02X ", byte)
        }
        return "\(String(format: "%04X", addr)): \(byteStr.padding(toLength: 10, withPad: " ", startingAt: 0))\(mnemonic) \(operand)"
    }
}
