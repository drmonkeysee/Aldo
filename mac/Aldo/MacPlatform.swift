//
//  MacPlatform.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/17/22.
//

import AppKit
import os
import UniformTypeIdentifiers

typealias CBuffer = UnsafeMutablePointer<CChar>
typealias CString = UnsafePointer<CChar>
typealias CStringArray = UnsafePointer<CString?>

typealias PlatformHandle = UnsafeMutablePointer<gui_platform>
typealias PlatformRenderFunc =
    @convention(c) (UnsafeMutableRawPointer?) -> Float

let aldoLog = Logger()

final class MacPlatform: NSObject {
    @objc static func setup(_ platform: PlatformHandle,
                            withScaleFunc: @escaping PlatformRenderFunc)
                            -> Bool {
        platform.pointee = .init(appname: appName,
                                 orgname: orgName,
                                 is_hidpi: isHiDPI,
                                 render_scale_factor: withScaleFunc,
                                 open_file: openFile,
                                 save_file: saveFile,
                                 launch_studio: launchStudio,
                                 display_error: displayError,
                                 free_buffer: freeBuffer)
        return true
    }
}

fileprivate final class OpenFileFilter: NSObject, NSOpenSavePanelDelegate {
    private let filter: Set<String>

    init(_ rawArray: CStringArray?) {
        if let rawArray {
            filter = .init(FilterSequence(current: rawArray))
        } else {
            filter = .init()
        }
    }

    func panel(_ sender: Any, shouldEnable url: URL) -> Bool {
        if filter.isEmpty { return true }
        return filter.contains(url.pathExtension.lowercased())
    }
}

fileprivate struct FilterSequence: Sequence, IteratorProtocol {
    var current: CStringArray

    func makeIterator() -> some IteratorProtocol { self }

    mutating func next() -> String? {
        guard let cstr = current.pointee else { return nil }
        defer { current += 1 }
        return .init(cString: cstr).lowercased()
    }
}

//
// Platform Implementation
//

fileprivate func appName() -> CBuffer? {
    guard let displayName = bundleName() else { return nil }
    return strToCBuffer(displayName)
}

fileprivate func orgName() -> CBuffer? {
    guard let bundleID = bundleIdentifier(),
          let appIdx = bundleID.lastIndex(of: ".") else { return nil }
    let org = bundleID.prefix(upTo: appIdx)
    return strToCBuffer(org)
}

fileprivate func isHiDPI() -> Bool {
    guard let hidpi = bundleHighRes() else { return false }
    return hidpi
}

fileprivate func openFile(title: CString?, filter: CStringArray?) -> CBuffer? {
    let panel = NSOpenPanel()
    if let title {
        panel.message = .init(cString: title)
    }
    let fileFilter = OpenFileFilter(filter)
    panel.delegate = fileFilter
    guard panel.runModal() == .OK, let path = panel.url else { return nil }
    return urlToCBuffer(path)
}

fileprivate func saveFile(title: CString?,
                          suggestedName: CString?) -> CBuffer? {
    let panel = NSSavePanel()
    if let title {
        panel.title = .init(cString: title)
    }
    if let suggestedName {
        panel.nameFieldStringValue = .init(cString: suggestedName)
        if let url = URL(string: panel.nameFieldStringValue),
           let ext = UTType(filenameExtension: url.pathExtension,
                            conformingTo: .text) {
            panel.allowedContentTypes = [ext]
            panel.allowsOtherFileTypes = true
            panel.isExtensionHidden = false
        }
    }
    guard panel.runModal() == .OK, let path = panel.url else { return nil }
    return urlToCBuffer(path)
}

fileprivate let openStudioErrorTitle = "Open Studio Error"

fileprivate func launchStudio() {
    guard let studioUrl = Bundle.main.url(
            forAuxiliaryExecutable: "AldoStudio.app") else {
        let msg = "Unable to find Aldo Studio app bundle"
        aldoLog.error("Studio URL Error: \(msg)")
        let _ = aldoAlert(title: openStudioErrorTitle, message: msg)
        return
    }
    aldoLog.debug("Studio bundle located at \(studioUrl)")
    let config = NSWorkspace.OpenConfiguration()
    config.promptsUserIfNeeded = false
    NSWorkspace.shared.openApplication(at: studioUrl,
                                       configuration: config) { _, err in
        guard let err else { return }
        aldoLog.error("Aldo Studio Launch Error: \(err.localizedDescription)")
        Task { @MainActor in
            let _ = aldoAlert(title: openStudioErrorTitle,
                              message: err.localizedDescription)
        }
    }
}

fileprivate func displayError(title: CString?, message: CString?) -> Bool {
    var titleStr: String? = nil
    var msgStr: String? = nil
    if let title { titleStr = .init(cString: title) }
    if let message { msgStr = .init(cString: message) }
    return aldoAlert(title: titleStr ?? "INVALID TITLE",
                     message: msgStr ?? "INVALID MESSAGE")
}

fileprivate func freeBuffer(_ buffer: CBuffer?) { buffer?.deallocate() }

//
// Helpers
//

fileprivate func aldoAlert(title: String, message: String) -> Bool {
    let modal = NSAlert()
    modal.messageText = title
    modal.informativeText = message
    modal.alertStyle = .warning
    return modal.runModal() == .OK
}

fileprivate func bundleName() -> String? {
    Bundle.main.object(forInfoDictionaryKey: "CFBundleDisplayName") as? String
}

fileprivate func bundleIdentifier() -> String? {
    Bundle.main.object(forInfoDictionaryKey: "CFBundleIdentifier") as? String
}

fileprivate func bundleHighRes() -> Bool? {
    Bundle.main.object(forInfoDictionaryKey: "NSHighResolutionCapable")
        as? Bool
}

fileprivate func urlToCBuffer(_ url: URL) -> CBuffer? {
    url.withUnsafeFileSystemRepresentation {
        guard let filepath = $0 else {
            aldoLog.error("Unable to represent file selection as system path")
            return nil
        }
        let buffer = CBuffer.allocate(capacity: strlen(filepath) + 1)
        strcpy(buffer, filepath)
        return buffer
    }
}

fileprivate func strToCBuffer(_ str: any StringProtocol) -> CBuffer? {
    guard let cstring = str.cString(using: .utf8) else { return nil }
    let buffer = CBuffer.allocate(capacity: cstring.count)
    strcpy(buffer, cstring)
    return buffer
}
