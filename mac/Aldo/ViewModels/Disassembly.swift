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

    var currentLine: PrgLine? {
        if let line = selectedLine, let bank = store.cache[bank] {
            return bank[line]
        }
        return nil
    }

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
    static func parse(_ instData: dis_instruction) -> Self {
        withUnsafePointer(to: instData) { p in
                .init(addressMode: .init(cString: dis_inst_addrmode(p)),
                      bytes: getBytes(p),
                      mnemonic: .init(cString: dis_inst_mnemonic(p)),
                      operand: getOperand(p),
                      unofficial: p.pointee.d.unofficial)
        }
    }

    private static func getBytes(_ p: CInstPtr) -> [UInt8] {
        (0..<p.pointee.bv.size).map { i in p.pointee.bv.mem[i] }
    }

    private static func getOperand(_ p: CInstPtr)-> String {
        let buffer = CBuffer.allocate(capacity: DIS_OPERAND_SIZE)
        defer { buffer.deallocate() }
        let err = dis_inst_operand(p, buffer)
        if err < 0 {
            let msg = AldoError.wrapDisError(code: err).message
            aldoLog.debug("Operand Parse Error: \(msg)")
            return "ERR"
        }
        return .init(cString: buffer)
    }

    let addressMode: String
    let bytes: [UInt8]
    let mnemonic: String
    let operand: String
    let unofficial: Bool

    var name: String { "Jump" }
    var description: String { "Unconditional jump to an address" }
    var flags: UInt8 { 0x34 }

    func displayLine(addr: UInt16) -> String {
        let addrStr = String(format: "%04X", addr)
        let byteStr = bytes
                        .map { b in String(format: "%02X", b) }
                        .joined(separator: " ")
                        .padding(toLength: 9, withPad: " ", startingAt: 0)
        return "\(addrStr): \(byteStr) \(mnemonic) \(operand)"
    }

    func byte(at: Int) -> UInt8? {
        guard 0 <= at && at < bytes.count else { return nil }
        return bytes[at]
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
        return .disassembled(addr, .parse(inst))
    }
}
