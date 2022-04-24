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
        print("Load CHR Bank")
    }
}
