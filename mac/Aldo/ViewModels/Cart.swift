//
//  Cart.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/12/22.
//

import Foundation

final class Cart {
    private(set) var fileUrl: URL
    private(set) var info: CartInfo

    var fileName: String { fileUrl.lastPathComponent }

    init(loadFromFile: URL) {
        fileUrl = loadFromFile
        info = .raw
        print("Cart ctor")
    }

    deinit {
        print("Cart dtor")
    }
}

enum CartInfo {
    case raw

    var name: String {
        switch self {
        case .raw:
            return "Raw"
        }
    }
}
