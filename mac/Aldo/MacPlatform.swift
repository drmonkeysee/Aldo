//
//  MacPlatform.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/17/22.
//

import AppKit
import os

typealias CBuffer = UnsafeMutablePointer<CChar>
typealias CString = UnsafePointer<CChar>

typealias PlatformHandle = UnsafeMutablePointer<gui_platform>
typealias PlatformRenderFunc =
    @convention(c) (UnsafeMutableRawPointer?) -> Float

let aldoLog = Logger()

final class MacPlatform: NSObject {
    @objc static func setup(_ platform: PlatformHandle,
                            withScaleFunc: @escaping PlatformRenderFunc)
                            -> Bool {
        platform.pointee = .init(appname: appName,
                                 is_hidpi: isHiDPI,
                                 render_scale_factor: withScaleFunc,
                                 open_file: openFile,
                                 launch_studio: launchStudio,
                                 display_error: displayError,
                                 free_buffer: freeBuffer)
        return true
    }
}

//
// Platform Implementation
//

fileprivate func appName() -> CBuffer? {
    guard let displayName = bundleName(),
          let cstring = displayName.cString(using: .utf8) else {
        return nil
    }
    let buffer = CBuffer.allocate(capacity: cstring.count)
    strcpy(buffer, cstring)
    return buffer
}

fileprivate func isHiDPI() -> Bool {
    guard let hidpi = bundleHighRes() else { return false }
    return hidpi
}

fileprivate func openFile() -> CBuffer? {
    let panel = NSOpenPanel()
    panel.message = "Choose a ROM file"
    guard panel.runModal() == .OK, let path = panel.url else { return nil }
    return path.withUnsafeFileSystemRepresentation {
        guard let filepath = $0 else {
            aldoLog.error("Unable to represent file selection as system path")
            return nil
        }
        let buffer = CBuffer.allocate(capacity: strlen(filepath) + 1)
        strcpy(buffer, filepath)
        return buffer
    }
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
        if let err {
            aldoLog.error(
                "Aldo Studio Launch Error: \(err.localizedDescription)")
            Task { @MainActor in
                let _ = aldoAlert(title: openStudioErrorTitle,
                                  message: err.localizedDescription)
            }
        }
    }
}

fileprivate func displayError(title: CString?, message: CString?) -> Bool {
    var titleStr: String? = nil
    var msgStr: String? = nil
    if let title {
        titleStr = .init(cString: title)
    }
    if let message {
        msgStr = .init(cString: message)
    }
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

fileprivate func bundleHighRes() -> Bool? {
    Bundle.main.object(forInfoDictionaryKey: "NSHighResolutionCapable")
        as? Bool
}
