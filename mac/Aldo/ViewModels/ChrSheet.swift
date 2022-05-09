//
//  ChrSheet.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/22/22.
//

import Cocoa

final class ChrSheet: ObservableObject {
    let cart: Cart
    let bank: Int
    let scale: Int
    @Published private(set) var status: BankLoadStatus<NSImage> = .pending

    init(_ cart: Cart, bank: Int, scale: Int) {
        self.cart = cart
        self.bank = bank
        self.scale = scale
    }

    func load() {
        if let img = cart.chrSheetCache[bank] {
            self.status = .loaded(img)
            return
        }
        cart.readChrBank(bank: bank, scale: scale) { result in
            switch result {
            case let .success(data):
                let chrSheet = NSImage(data: data)
                if let img = chrSheet, img.isValid {
                    self.cart.chrSheetCache[self.bank] = img
                    self.status = .loaded(img)
                } else {
                    self.logFailure("CHR Decode Failure",
                                    chrSheet == nil
                                        ? "Image init failed"
                                        : "Image data invalid")
                    self.status = .failed
                }
            case let .error(err):
                self.logFailure("CHR Read Failure", err.message)
                self.status = .failed
            }
        }
    }

    private func logFailure(_ items: String...) {
        #if DEBUG
        print(items)
        #endif
    }
}
