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
    @Published var failed = false
    private(set) var currentError: AldoError?

    func execute(cart: Cart) {
        currentError = nil
        cart.readInfoText { result in
            switch result {
            case .success(let data):
                if let text = String(data: data, encoding: .utf8) {
                    NSPasteboard.general.clearContents()
                    NSPasteboard.general.setString(text, forType: .string)
                    self.copyIconOpacity = 0.0
                    self.successIconOpacity = 1.0
                } else {
                    self.setError(.unknown)
                }
            case .error(let err):
                self.setError(err)
            }
        }
    }

    private func setError(_ err: AldoError) {
        currentError = err
        failed = true
    }
}
