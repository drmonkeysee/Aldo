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
        store = .init(cart)
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
    let operand: String

    init(_ instData: dis_instruction) {
        mnemonic = withUnsafePointer(to: instData) { p in
            .init(cString: dis_inst_mnemonic(p))
        }
        operand = withUnsafePointer(to: instData) { p in
            let buffer = UnsafeMutablePointer<CChar>.allocate(
                capacity: DIS_OPERAND_SIZE)
            defer { buffer.deallocate() }
            let err = dis_inst_operand(p, buffer)
            if err < 0 {
                aldoLog("Operand Parse Error",
                        AldoError.wrapDisError(code: err).message)
                return "ERR"
            }
            return .init(cString: buffer)
        }
    }

    var name: String { "Jump" }
    var addressMode: String { "Absolute Indirect" }
    var description: String { "Unconditional jump to an address" }
    var flags: UInt8 { 0x34 }

    func line(addr: UInt16) -> String {
        let byteStr = "XX XX XX"
        return "\(String(format: "%04X", addr)): \(byteStr.padding(toLength: 10, withPad: " ", startingAt: 0))\(mnemonic) \(operand)"
    }
}

fileprivate func prgBanks(_ cart: Cart) -> Int { cart.info.prgBanks }

fileprivate actor PrgStore {
    let cart: Cart
    let cache: BankCache<[PrgLine]>

    init(_ cart: Cart) {
        self.cart = cart
        cache = cart.prgCache
        cache.ensure(slots: prgBanks(cart))
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
    private var addr: UInt16
    private var cursor = 0
    private var parseErr = false

    init?(_ prgbank: bankview?) {
        guard let pb = prgbank else { return nil }
        bv = pb
        // NOTE: by convention, count backwards from CPU vector locations
        addr = UInt16(MEMBLOCK_64KB - bv.size)
    }

    mutating func next() -> PrgLine? {
        if parseErr { return nil }

        var inst = dis_instruction()
        let err = withUnsafePointer(to: bv) { p in
            dis_parse_inst(p, cursor, &inst)
        }

        defer {
            if err > 0 {
                cursor += inst.bv.size
                addr &+= UInt16(inst.bv.size)
            }
        }
        if err == 0 { return nil }
        if err < 0 {
            parseErr = true
            return .failure(addr, .wrapDisError(code: err))
        }
        return .disassembled(addr, .init(inst))
    }
}
