//
//  Commands.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 6/14/22.
//

import AppKit

final class ClipboardCopy: TimedFeedbackCommand {
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
                finishTransition()
            } else {
                setError(.unknown)
            }
        case let .error(err):
            setError(err)
        }
    }
}

final class ChrExport: TimedFeedbackCommand {
    let cart: Cart
    let scales = Array(Int(MinChrScale)...Int(MaxChrScale))
    @Published var scale = ChrSheet.scale
    @Published var folderAvailable = false
    private(set) var selectedFolder: URL?

    init(_ cart: Cart) { self.cart = cart }

    @MainActor
    func export(to: URL?) async {
        guard let folder = to else { return }

        inProgress = true
        folderAvailable = false
        currentError = nil
        selectedFolder = folder
        let result = await cart.exportChrRom(scale: scale, folder: folder)
        switch result {
        case let .success(data):
            if let text = String(data: data, encoding: .utf8) {
                aldoLog.debug("CHR Export:\n\(text)")
                folderAvailable = true
                finishTransition()
            } else {
                setError(.unknown)
            }
        case let .error(err):
            setError(err)
        }
    }
}

class TimedFeedbackCommand: ObservableObject {
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
        actionIconOpacity = 0.0
        successIconOpacity = 1.0
        DispatchQueue.main.asyncAfter(deadline: .now()
                                      + Self.transitionDuration) {
            self.actionIconOpacity = 1.0
            self.inProgress = false
        }
    }
}
