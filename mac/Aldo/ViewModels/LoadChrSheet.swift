//
//  LoadChrSheet.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/22/22.
//

import Cocoa

enum ChrSheetStatus {
    case pending
    case loaded(NSImage)
    case failed
}

final class LoadChrSheet: ObservableObject {
    @Published private(set) var status = ChrSheetStatus.pending

    func execute(cart: Cart, bank: Int) {
        cart.readChrBank(bank: bank) { result in
            switch result {
            case .success(let data):
                let chrSheet = NSImage(data: data)
                if let img = chrSheet, img.isValid {
                    self.status = .loaded(img)
                } else {
                    self.logFailure("CHR Decode Failure",
                                    chrSheet == nil
                                        ? "Image init failed"
                                        : "Image data invalid")
                    self.status = .failed
                }
            case .error(let err):
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
