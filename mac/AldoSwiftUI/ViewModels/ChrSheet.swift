//
//  ChrSheet.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/22/22.
//

import Cocoa

final class ChrBlocks {
    let cart: Cart
    private let store: ChrStore

    var count: Int { chrBlocks(cart) }

    init(_ cart: Cart) {
        self.cart = cart
        store = .init(cart)
    }

    func sheet(at: Int) -> ChrSheet { .init(store, index: at) }
}

final class ChrSheet: ObservableObject {
    static let scale = 2

    let index: Int
    @Published private(set) var status = BlockLoadStatus<NSImage>.pending
    private let store: ChrStore

    fileprivate init(_ store: ChrStore, index: Int) {
        self.store = store
        self.index = index
    }

    @MainActor
    func load() async {
        status = await store.fetch(at: index, scale: Self.scale)
    }
}

fileprivate func chrBlocks(_ cart: Cart) -> Int { cart.info.chrBlocks }

fileprivate actor ChrStore {
    let cart: Cart
    let cache: BlockCache<NSImage>

    init(_ cart: Cart) {
        self.cart = cart
        cache = cart.chrCache
        cache.ensure(slots: chrBlocks(cart))
    }

    func fetch(at: Int, scale: Int) async -> BlockLoadStatus<NSImage> {
        if let img = cache[at] { return .loaded(img) }

        let result = await cart.readChrBlock(at: at, scale: scale)
        switch result {
        case let .success(data):
            let chrSheet = NSImage(data: data)
            if let img = chrSheet, img.isValid {
                cache[at] = img
                return .loaded(img)
            }
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
}
