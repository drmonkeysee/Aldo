//
//  AppControl.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/12/22.
//

import Foundation

final class AppControl: ObservableObject {
    @Published private(set) var cartFile: URL?
    @Published private(set) var cart: Cart?

    var cartName: String? { cartFile?.lastPathComponent }

    func loadCart(from: URL?) {
        guard let url = from else {
            return
        }
        cartFile = url
        cart = Cart(loadFromFile: url)
    }
}
