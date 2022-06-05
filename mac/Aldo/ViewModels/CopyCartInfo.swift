//
//  CopyCartInfo.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/18/22.
//

import AppKit

final class CopyCartInfo: ObservableObject {
    let textStream: () async -> CStreamResult
    @Published var copyIconOpacity = 1.0
    @Published var successIconOpacity = 0.0
    @Published var failed = false
    private(set) var currentError: AldoError?

    init(fromStream: @escaping () async -> CStreamResult) {
        textStream = fromStream
    }

    @MainActor
    func execute() async {
        currentError = nil
        let result = await textStream()
        switch result {
        case let .success(data):
            if let text = String(data: data, encoding: .utf8) {
                NSPasteboard.general.clearContents()
                NSPasteboard.general.setString(text, forType: .string)
                copyIconOpacity = 0.0
                successIconOpacity = 1.0
            } else {
                setError(.unknown)
            }
        case let .error(err):
            setError(err)
        }
    }

    private func setError(_ err: AldoError) {
        currentError = err
        failed = true
    }
}
