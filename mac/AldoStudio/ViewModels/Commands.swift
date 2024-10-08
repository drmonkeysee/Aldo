//
//  Commands.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 6/14/22.
//

import AppKit

final class ClipboardCopy: TimedFeedbackCommand {
    private let textStream: () async -> CStreamResult

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
                showSuccess()
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
    let scales = Array(Int(Aldo_MinChrScale)...Int(Aldo_MaxChrScale))
    @Published var scale = ChrBlocks.scale
    @Published var folderAvailable = false
    private(set) var selectedFolder: URL?

    init(_ cart: Cart) { self.cart = cart }

    @MainActor
    func export(to: URL?) async {
        guard let to else { return }

        inProgress = true
        folderAvailable = false
        currentError = nil
        selectedFolder = to
        let result = await cart.exportChrRom(scale: scale, folder: to)
        switch result {
        case let .success(data):
            if let text = String(data: data, encoding: .utf8) {
                aldoLog.debug("CHR Export:\n\(text)")
                folderAvailable = true
                showSuccess()
            } else {
                setError(.unknown)
            }
        case let .error(err):
            setError(err)
        }
    }
}

// NOTE: @Observable doesn't seem to support inheritance properly; the
// observable properties from this base class aren't visible for @Bindable in
// the child classes.
class TimedFeedbackCommand: ObservableObject {
    static let transitionDuration = 2.0

    @Published var actionIconOpacity = 1.0
    @Published var successIconOpacity = 0.0
    @Published var inProgress = false
    @Published var failed = false
    fileprivate(set) var currentError: AldoError?

    func complete() {
        self.actionIconOpacity = 1.0
        self.inProgress = false
    }

    fileprivate func setError(_ err: AldoError) {
        currentError = err
        failed = true
        inProgress = false
    }

    fileprivate func showSuccess() {
        actionIconOpacity = 0.0
        successIconOpacity = 1.0
    }
}
