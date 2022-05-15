//
//  ChrSheet.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/22/22.
//

import Cocoa

final class ChrBanks {
    let cart: Cart
    private let cache: BankCache<NSImage>

    var count: Int { chrBanks(cart) }

    init(_ cart: Cart) {
        self.cart = cart
        cache = .init(slots: chrBanks(cart))
    }

    func sheet(at bank: Int) -> ChrSheet {
        .init(cart, bank: bank, cache: cache)
    }
}

final class ChrSheet: ObservableObject {
    static let scale = 2

    let cart: Cart
    let bank: Int
    @Published private(set) var status = BankLoadStatus<NSImage>.pending
    private let cache: BankCache<NSImage>

    fileprivate init(_ cart: Cart, bank: Int, cache: BankCache<NSImage>) {
        self.cart = cart
        self.bank = bank
        self.cache = cache
    }

    @MainActor
    func load() async {
        if let img = await cache[bank] {
            status = .loaded(img)
            return
        }

        let result = await cart.readChrBank(bank: bank, scale: Self.scale)
        switch result {
        case let .success(data):
            let chrSheet = NSImage(data: data)
            if let img = chrSheet, img.isValid {
                await cache.setItem(img, at: bank)
                status = .loaded(img)
            } else {
                logFailure("CHR Decode Failure", chrSheet == nil
                                                    ? "Image init failed"
                                                    : "Image data invalid")
                status = .failed
            }
        case let .error(err):
            logFailure("CHR Read Failure", err.message)
            status = .failed
        }
    }

    private func logFailure(_ items: String...) {
        #if DEBUG
        print(items)
        #endif
    }
}

// NOTE: standalone func to share between initializer and computed property
fileprivate func chrBanks(_ cart: Cart) -> Int { cart.info.chrBanks }
