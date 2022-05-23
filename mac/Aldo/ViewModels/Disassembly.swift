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
    private let store: PrgStore

    var count: Int { prgBanks(cart) }
    var currentListing: ProgramListing { .init(store, bank: selectedBank) }

    init(_ cart: Cart) {
        self.cart = cart
        store = .init(cart, bankCount: prgBanks(cart))
    }
}

final class ProgramListing: ObservableObject {
    let bank: Int
    @Published var selectedLine: Int?
    @Published private(set) var status = BankLoadStatus<[PrgLine]>.pending
    private let store: PrgStore

    fileprivate init(_ store: PrgStore, bank: Int) {
        self.store = store
        self.bank = bank
    }

    @MainActor
    func load() async { status = await store.fetch(bank: bank) }
}

enum PrgLine {
    case disassembled(UInt16, Instruction)
    case elision(UInt16)
    case failure(UInt16, AldoError)
}

struct Instruction {
    let mnemonic: String

    init(_ instData: dis_instruction) {
        mnemonic = withUnsafePointer(to: instData) { p in
            .init(cString: dis_inst_mnemonic(p))
        }
    }

    var operand: String { "$(8134)" }
    var name: String { "Jump" }
    var addressMode: String { "Absolute Indirect" }
    var description: String { "Unconditional jump to an address" }
    var flags: UInt8 { 0x34 }

    func line(addr: UInt16) -> String {
        let byteStr = "XX XX XX"
        return "\(String(format: "%04X", addr)): \(byteStr.padding(toLength: 10, withPad: " ", startingAt: 0))\(mnemonic) \(operand)"
    }
}

// NOTE: standalone func to share between initializer and computed property
fileprivate func prgBanks(_ cart: Cart) -> Int { cart.info.prgBanks }

fileprivate actor PrgStore {
    let cart: Cart
    private let cache: BankCache<[PrgLine]>

    init(_ cart: Cart, bankCount: Int) {
        self.cart = cart
        cache = .init(slots: bankCount)
    }

    func fetch(bank: Int) -> BankLoadStatus<[PrgLine]> {
        if let listing = cache[bank] { return .loaded(listing) }

        guard let prgbank = PrgLines(cart.getPrgBank(bank)) else {
            return .failed
        }
        let prgListing = Array(prgbank)
        cache[bank] = prgListing
        return .loaded(prgListing)
    }
}

fileprivate struct PrgLines: Sequence, IteratorProtocol {
    let bv: bankview
    private var cursor = 0
    private var parseErr = false

    init?(_ prgbank: bankview?) {
        guard let pb = prgbank else { return nil }
        bv = pb
    }

    mutating func next() -> PrgLine? {
        if parseErr { return nil }

        // NOTE: by convention, count backwards from CPU vector locations
        let addr = UInt16(MEMBLOCK_64KB - bv.size + cursor);
        var inst = dis_instruction()
        let err = withUnsafePointer(to: bv) { p in
            dis_parse_inst(p, cursor, &inst)
        }

        defer { cursor += inst.bv.size }
        if err == 0 { return nil }
        if err < 0 {
            parseErr = true
            return .failure(addr, .wrapDisError(code: err))
        }
        return .disassembled(addr, .init(inst))
    }
}
