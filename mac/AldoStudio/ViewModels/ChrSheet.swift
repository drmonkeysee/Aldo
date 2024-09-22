//
//  ChrSheet.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 4/22/22.
//

import Cocoa

typealias ChrStore = BlockCacheReal<NSImage>
typealias ChrItem = BlockLoadStatus<NSImage>

final class ChrBlocks {
    static let scale = 2

    static func empty() -> Self { .init(.init(capacity: 0)) }

    @MainActor
    static func loadBlocks(from: Cart) async -> Self {
        let store = ChrStore(capacity: from.info.format.chrBlocks)
        for at in 0..<store.capacity {
            store[at] = await loadBlock(from: from, at: at, scale: scale)
        }
        return .init(store)
    }

    private let store: ChrStore

    var count: Int { store.capacity }

    func sheet(at: Int) -> ChrItem { store[at] }

    fileprivate init(_ store: ChrStore) { self.store = store }
}

@MainActor
fileprivate func loadBlock(from: Cart, at: Int, scale: Int) async -> ChrItem {
    let result = await from.readChrBlock(at: at, scale: scale)
    switch result {
    case let .success(data):
        let chrSheet = NSImage(data: data)
        if let chrSheet, chrSheet.isValid { return .loaded(chrSheet) }
        let reason = chrSheet == nil
                        ? "Image init failed"
                        : "Image data invalid"
        aldoLog.debug("CHR Decode Failure: \(reason)")
        return .failed
    case let .error(err):
        aldoLog.debug("CHR Read Failure: \(err.message)")
        return .failed
    }
}
