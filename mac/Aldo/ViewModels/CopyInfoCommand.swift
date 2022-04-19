//
//  CopyInfoCommand.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/18/22.
//

import AppKit

final class CopyInfoCommand: ObservableObject {
    @Published var copyIconOpacity = 1.0
    @Published var successIconOpacity = 0.0
    @Published var copyError = false

    func execute(cart: Cart) {
        guard let text = cart.getInfoText() else {
            copyError = true
            return
        }
        NSPasteboard.general.clearContents()
        NSPasteboard.general.setString(text, forType: .string)
        successIconOpacity = 1.0
        copyIconOpacity = 0.0
    }
}
