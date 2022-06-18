//
//  Commands.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 6/14/22.
//

import AppKit

final class ClipboardCopy: TimedCommand {
    let textStream: () async -> CStreamResult

    init(fromStream: @escaping () async -> CStreamResult) {
        textStream = fromStream
    }

    @MainActor
    func copy() async {
        inProgress = true
        currentError = nil
        let result = await textStream()
        switch result {
        case let .success(data):
            if let text = String(data: data, encoding: .utf8) {
                NSPasteboard.general.clearContents()
                NSPasteboard.general.setString(text, forType: .string)
                actionIconOpacity = 0.0
                successIconOpacity = 1.0
                finishTransition()
            } else {
                setError(.unknown)
            }
        case let .error(err):
            setError(err)
        }
    }
}

final class ChrExport: TimedCommand {
    let cart: Cart
    let scales = Array(Int(MinChrScale)...Int(MaxChrScale))
    @Published var scale = ChrSheet.scale
    @Published var done = false
    private(set) var selectedFolder: URL?

    init(_ cart: Cart) {
        self.cart = cart
    }

    @MainActor
    func export(to: URL?) async {
        guard let folder = to else { return }

        inProgress = true
        done = false
        currentError = nil
        selectedFolder = folder
        let result = await cart.exportChrRom(scale: scale, folder: folder)
        switch result {
        case let .success(data):
            if let text = String(data: data, encoding: .utf8) {
                aldoLog.debug("CHR Export:\n\(text)")
                actionIconOpacity = 0.0
                successIconOpacity = 1.0
                done = true
                finishTransition()
            } else {
                setError(.unknown)
            }
        case let .error(err):
            setError(err)
        }
    }
}

class TimedCommand: ObservableObject {
    static let transitionDuration = 2.0

    @Published var actionIconOpacity = 1.0
    @Published var successIconOpacity = 0.0
    @Published var inProgress = false
    @Published var failed = false
    fileprivate(set) var currentError: AldoError?

    fileprivate func setError(_ err: AldoError) {
        currentError = err
        failed = true
        inProgress = false
    }

    fileprivate func finishTransition() {
        DispatchQueue.main.asyncAfter(deadline: .now()
                                      + Self.transitionDuration) {
            self.actionIconOpacity = 1.0
            self.inProgress = false
        }
    }
}
