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
    @Published private(set) var error: CartErrCode?
    private var handle: CartHandle?

    var name: String? { file?.lastPathComponent }

    func load(from: URL?) {
        guard let filePath = from else { return }
        file = filePath
        info = .raw
    }
}

enum CartInfo {
    case none
    case raw

    var name: String { "\(self)".capitalized }
}

final class CartHandle {
    private let handle: UnsafeRawPointer?

    init?(fromFile: URL) {
        // open file
        // if fails set error
        // open cart
        // if fails set error
        // close file
        nil
    }
}
