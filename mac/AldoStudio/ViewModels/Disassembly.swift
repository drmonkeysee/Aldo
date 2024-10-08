//
//  Disassembly.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 5/3/22.
//

import Foundation
import Observation

fileprivate typealias ProgramStore = BlockCache<[PrgLine]>
fileprivate typealias ProgramBlock = BlockLoadStatus<[PrgLine]>

@Observable
final class ProgramBlocks {
    static func empty() -> Self { .init(.init(capacity: 0)) }

    @MainActor
    static func loadBlocks(from: Cart) async -> Self {
        let store = ProgramStore(capacity: from.info.format.prgBlocks)
        for at in 0..<store.capacity {
            store[at] = loadBlock(from: from, at: at)
        }
        return .init(store)
    }

    var selectedBlock = 0
    var count: Int { store.capacity }
    var currentListing: ProgramListing { .init(listing: store[selectedBlock]) }
    private let store: ProgramStore

    fileprivate init(_ store: ProgramStore) { self.store = store }
}

@Observable
final class ProgramListing {
    var selectedLine: Int?
    private(set) var status = ProgramBlock.none

    var currentLine: PrgLine? {
        if let selectedLine, case let .loaded(block) = status {
            return block[selectedLine]
        }
        return nil
    }

    fileprivate init(listing: ProgramBlock) { status = listing }
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
    static func parse(_ instData: aldo_dis_instruction) -> Self {
        withUnsafePointer(to: instData) { p in
                .init(addressMode: .init(cString: aldo_dis_inst_addrmode(p)),
                      bytes: getBytes(p),
                      mnemonic: .init(cString: aldo_dis_inst_mnemonic(p)),
                      description: .init(
                        cString: aldo_dis_inst_description(p)),
                      operand: getOperand(p),
                      unofficial: p.pointee.d.unofficial,
                      cycles: Cycles(count: p.pointee.d.cycles.count,
                                     pageBoundary: p.pointee.d
                                                    .cycles.page_boundary,
                                     branchTaken: p.pointee.d
                                                    .cycles.branch_taken),
                      flags: .init(aldo_dis_inst_flags(p)),
                      cells: .init(accumulator: p.pointee.d.datacells.a,
                                   xIndex: p.pointee.d.datacells.x,
                                   yIndex: p.pointee.d.datacells.y,
                                   processorStatus: p.pointee.d.datacells.p,
                                   stackPointer: p.pointee.d.datacells.s,
                                   programCounter: p.pointee.d.datacells.pc,
                                   memory: p.pointee.d.datacells.m))
        }
    }

    private static let errStr = "ERR"

    private static func getBytes(_ p: CInstPtr) -> [UInt8] {
        (0..<p.pointee.bv.size).map { p.pointee.bv.mem[$0] }
    }

    private static func getOperand(_ p: CInstPtr)-> String {
        return withUnsafeTemporaryAllocation(
            of: CChar.self, capacity: ALDO_DIS_OPERAND_SIZE) { buffer in
                guard let bufferp = buffer.baseAddress else {
                    aldoLog.debug("Invalid buffer pointer")
                    return Self.errStr
                }
                let err = aldo_dis_inst_operand(p, bufferp)
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

fileprivate struct PrgLines: Sequence, IteratorProtocol {
    let bv: aldo_blockview
    private var addr: UInt16
    private var cursor = 0
    private var done = false
    private var prevInstruction = aldo_dis_instruction()
    private var skip = false

    init?(_ prgblock: aldo_blockview?) {
        guard let prgblock, prgblock.size > 0 else { return nil }
        bv = prgblock
        // NOTE: by convention, count backwards from CPU vector locations
        addr = .init(ALDO_MEMBLOCK_64KB - bv.size)
    }

    mutating func next() -> PrgLine? {
        if done { return nil }
        var line: PrgLine?
        repeat { line = nextLine() } while line == nil && !done
        return line
    }

    private mutating func nextLine() -> PrgLine? {
        var inst = aldo_dis_instruction()
        let err = withUnsafePointer(to: bv) {
            aldo_dis_parse_inst($0, cursor, &inst)
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
            if aldo_dis_inst_equal(&inst, &prevInstruction) {
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

fileprivate func loadBlock(from: Cart, at: Int) -> ProgramBlock {
    guard let prgblock = PrgLines(from.getPrgBlock(at)) else {
        return .failed
    }
    let prgListing = Array(prgblock)
    return .loaded(prgListing)
}
