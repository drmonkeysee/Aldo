//
//  ChrSheet.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/22/22.
//

import Cocoa

final class ChrBanks {
    let cart: Cart
    private let store: ChrStore

    var count: Int { chrBanks(cart) }

    init(_ cart: Cart) {
        self.cart = cart
        store = .init(cart, bankCount: chrBanks(cart))
    }

    func sheet(at bank: Int) -> ChrSheet { .init(store, bank: bank) }
}

final class ChrSheet: ObservableObject {
    static let scale = 2

    let bank: Int
    @Published private(set) var status = BankLoadStatus<NSImage>.pending
    private let store: ChrStore

    fileprivate init(_ store: ChrStore, bank: Int) {
        self.store = store
        self.bank = bank
    }

    @MainActor
    func load() async {
        status = await store.load(bank: bank, scale: Self.scale)
    }
}

// NOTE: standalone func to share between initializer and computed property
fileprivate func chrBanks(_ cart: Cart) -> Int { cart.info.chrBanks }

fileprivate actor ChrStore {
    let cart: Cart
    private let cache: BankCache<NSImage>

    init(_ cart: Cart, bankCount: Int) {
        self.cart = cart
        cache = .init(slots: bankCount)
    }

    func load(bank: Int, scale: Int) async -> BankLoadStatus<NSImage> {
        if let img = cache[bank] { return .loaded(img) }

        let result = await cart.readChrBank(bank: bank, scale: scale)
        switch result {
        case let .success(data):
            let chrSheet = NSImage(data: data)
            if let img = chrSheet, img.isValid {
                cache[bank] = img
                return .loaded(img)
            }
            logFailure("CHR Decode Failure", chrSheet == nil
                                                ? "Image init failed"
                                                : "Image data invalid")
            return .failed
        case let .error(err):
            logFailure("CHR Read Failure", err.message)
            return .failed
        }
    }

    private func logFailure(_ items: String...) {
        #if DEBUG
        print(items)
        #endif
    }
}
