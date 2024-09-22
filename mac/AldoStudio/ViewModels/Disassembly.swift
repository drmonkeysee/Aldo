//
//  Disassembly.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 5/3/22.
//

import Foundation

final class ProgramBlocks: ObservableObject {
    let cart: Cart
    @Published var selectedBlock = 0
    //private let store: PrgStore

    var count: Int { prgBlocks(cart) }
    var currentListing: ProgramListing { .init(/*store, */index: selectedBlock) }

    init(_ cart: Cart) {
        self.cart = cart
        //store = .init(cart)
    }
}

final class ProgramListing: ObservableObject {
    let index: Int
    @Published var selectedLine: Int?
    @Published private(set) var status = BlockLoadStatus<[PrgLine]>.pending
    //private let store: PrgStore

    var currentLine: PrgLine? {
        /*if let selectedLine, let block = store.cache[index] {
            return block[selectedLine]
        }*/
        return nil
    }

    fileprivate init(/*_ store: PrgStore, */index: Int) {
        //self.store = store
        self.index = index
    }

    /*@MainActor
    func load() async { status = await store.fetch(at: index) }*/
}

enum PrgLine {
    case disassembled(UInt16, Instruction)
    case elision
    case failure(UInt16, AldoError)

    var display: String {
        switch self {
        case let .disassembled(addr, inst):
            return "\(addrStr(addr)): \(inst.display)"
        case .elision:
            return "⋯"
        case let .failure(addr, err):
            return "\(addrStr(addr)): \(err.message)"
        }
    }

    private func addrStr(_ addr: UInt16) -> String {
        .init(format: "%04X", addr)
    }
}

struct Instruction {
    private static let errStr = "ERR"

    static func parse(_ instData: dis_instruction) -> Self {
        withUnsafePointer(to: instData) { p in
                .init(addressMode: .init(cString: dis_inst_addrmode(p)),
                      bytes: getBytes(p),
                      mnemonic: .init(cString: dis_inst_mnemonic(p)),
                      description: .init(cString: dis_inst_description(p)),
                      operand: getOperand(p),
                      unofficial: p.pointee.d.unofficial,
                      cycles: Cycles(count: p.pointee.d.cycles.count,
                                     pageBoundary: p.pointee.d
                                                    .cycles.page_boundary,
                                     branchTaken: p.pointee.d
                                                    .cycles.branch_taken),
                      flags: .init(dis_inst_flags(p)),
                      cells: .init(accumulator: p.pointee.d.datacells.a,
                                   xIndex: p.pointee.d.datacells.x,
                                   yIndex: p.pointee.d.datacells.y,
                                   processorStatus: p.pointee.d.datacells.p,
                                   stackPointer: p.pointee.d.datacells.s,
                                   programCounter: p.pointee.d.datacells.pc,
                                   memory: p.pointee.d.datacells.m))
        }
    }

    private static func getBytes(_ p: CInstPtr) -> [UInt8] {
        (0..<p.pointee.bv.size).map { p.pointee.bv.mem[$0] }
    }

    private static func getOperand(_ p: CInstPtr)-> String {
        return withUnsafeTemporaryAllocation(
            of: CChar.self, capacity: DIS_OPERAND_SIZE) { buffer in
                guard let bufferp = buffer.baseAddress else {
                    aldoLog.debug("Invalid buffer pointer")
                    return Self.errStr
                }
                let err = dis_inst_operand(p, bufferp)
                if err < 0 {
                    let msg = AldoError.wrapDisError(code: err).message
                    aldoLog.debug("Operand Parse Error: \(msg)")
                    return Self.errStr
                }
                return String(cString: bufferp)
            }
    }

    let addressMode: String
    let bytes: [UInt8]
    let mnemonic: String
    let description: String
    let operand: String
    let unofficial: Bool
    let cycles: Cycles
    let flags: CpuFlags
    let cells: DataCells

    var display: String {
        let byteStr = bytes
                        .map { String(format: "%02X", $0) }
                        .joined(separator: " ")
                        .padding(toLength: 9, withPad: " ", startingAt: 0)
        return "\(byteStr) \(mnemonic) \(operand)"
    }

    var cycleCount: String {
        let cy = cycles.count < 0 ? "∞" : "\(cycles.count)"
        let br = cycles.branchTaken ? "\n(+1 if branch taken)" : ""
        let pg = cycles.pageBoundary ? "\n(+1 if page-boundary crossed)" : ""
        return "\(cy) cycles\(br)\(pg)"
    }

    func byte(at: Int) -> UInt8? {
        bytes.indices.contains(at) ? bytes[at] : nil
    }
}

struct Cycles {
    let count: Int8
    let pageBoundary: Bool
    let branchTaken: Bool
}

struct CpuFlags {
    let carry: Bool
    let zero: Bool
    let interrupt: Bool
    let decimal: Bool
    let softbreak: Bool
    let overflow: Bool
    let negative: Bool

    init(_ status: UInt8) {
        carry = status & 0x1 != 0
        zero = status & 0x2 != 0
        interrupt = status & 0x4 != 0
        decimal = status & 0x8 != 0
        softbreak = status & 0x10 != 0
        overflow = status & 0x40 != 0
        negative = status & 0x80 != 0
    }
}

struct DataCells {
    let accumulator: Bool
    let xIndex: Bool
    let yIndex: Bool
    let processorStatus: Bool
    let stackPointer: Bool
    let programCounter: Bool
    let memory: Bool
}

fileprivate func prgBlocks(_ cart: Cart) -> Int { cart.info.format.prgBlocks }

fileprivate actor PrgStore {
    let cart: Cart
    let cache: BlockCache<[PrgLine]>

    init(_ cart: Cart) {
        self.cart = cart
        cache = cart.prgCache
        cache.ensure(slots: prgBlocks(cart))
    }

    func fetch(at: Int) -> BlockLoadStatus<[PrgLine]> {
        if let listing = cache[at] { return .loaded(listing) }

        guard let prgblock = PrgLines(cart.getPrgBlock(at)) else {
            return .failed
        }
        let prgListing = Array(prgblock)
        cache[at] = prgListing
        return .loaded(prgListing)
    }
}

fileprivate struct PrgLines: Sequence, IteratorProtocol {
    let bv: blockview
    private var addr: UInt16
    private var cursor = 0
    private var done = false
    private var prevInstruction = dis_instruction()
    private var skip = false

    init?(_ prgblock: blockview?) {
        guard let prgblock, prgblock.size > 0 else { return nil }
        bv = prgblock
        // NOTE: by convention, count backwards from CPU vector locations
        addr = .init(MEMBLOCK_64KB - bv.size)
    }

    mutating func next() -> PrgLine? {
        if done { return nil }
        var line: PrgLine?
        repeat { line = nextLine() } while line == nil && !done
        return line
    }

    private mutating func nextLine() -> PrgLine? {
        var inst = dis_instruction()
        let err = withUnsafePointer(to: bv) {
            dis_parse_inst($0, cursor, &inst)
        }

        if err < 0 {
            done = true
            return .failure(addr, .wrapDisError(code: err))
        } else if err == 0 {
            done = true
            // NOTE: always print the last line even if it would
            // normally be skipped.
            if skip {
                // NOTE: back up address to the last line
                addr &-= .init(prevInstruction.bv.size)
                return .disassembled(addr, .parse(prevInstruction))
            }
        } else {
            defer {
                cursor += inst.bv.size
                addr &+= .init(inst.bv.size)
            }
            if dis_inst_equal(&inst, &prevInstruction) {
                if !skip {
                    skip = true
                    return .elision
                }
            } else {
                prevInstruction = inst
                skip = false
                return .disassembled(addr, .parse(inst))
            }
        }
        return nil
    }
}
