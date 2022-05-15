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
    private let cache: BankCache<[PrgLine]>

    var count: Int { prgBanks(cart) }

    var currentListing: ProgramListing {
        .init(cart, bank: selectedBank, cache: cache)
    }

    init(_ cart: Cart) {
        self.cart = cart
        cache = .init(slots: prgBanks(cart))
    }
}

final class ProgramListing: ObservableObject {
    let cart: Cart
    let bank: Int
    @Published var selectedLine: Int?
    @Published private(set) var status = BankLoadStatus<[PrgLine]>.pending
    private let cache: BankCache<[PrgLine]>

    fileprivate init(_ cart: Cart, bank: Int, cache: BankCache<[PrgLine]>) {
        self.cart = cart
        self.bank = bank
        self.cache = cache
    }

    @MainActor
    func load() async {
        if let listing = await cache[bank] {
            status = .loaded(listing)
            return
        }

        let loader = PrgLoader()
        let prgListing = await loader.load(bank: bank)
        await cache.setItem(prgListing, at: bank)
        status = .loaded(prgListing)
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

// NOTE: standalone func to share between initializer and computed property
fileprivate func prgBanks(_ cart: Cart) -> Int { cart.info.prgBanks }

fileprivate actor PrgLoader {
    func load(bank: Int) -> [PrgLine] {
        sleep(1)
        return (0..<100).map { i in
            PrgLine.disassembled(i, .init(bytes: [0x6c, 0x34, 0x81]))
        }
    }
}
