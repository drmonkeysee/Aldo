//
//  Cart.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/12/22.
//

import Foundation

final class Cart: ObservableObject {
    @Published private(set) var file: URL?
    @Published private(set) var info: CartInfo = .none

    var name: String? { file?.lastPathComponent }

    func load(from: URL?) {
        file = from
        info = .raw
    }
}

enum CartInfo {
    case none
    case raw

    var name: String { "\(self)".capitalized }
}
