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

fileprivate func launchStudio() {
    guard let studioUrl = Bundle.main.url(
            forAuxiliaryExecutable: "AldoStudio.app") else {
        aldoLog.error("Unable to find Aldo Studio app bundle")
        return
    }
    aldoLog.debug("Studio bundle located at \(studioUrl)")
    NSWorkspace.shared.openApplication(at: studioUrl,
                                       configuration: .init()) { _, err in
        if let err {
            aldoLog.error(
                "Error opening Aldo Studio: \(err.localizedDescription)")
        }
    }
}

fileprivate func displayError(title: CString?, message: CString?) -> Bool {
    let modal = NSAlert()
    modal.messageText = title == nil ? "INVALID TITLE" : .init(cString: title!)
    modal.informativeText = message == nil
                            ? "INVALID MESSAGE"
                            : .init(cString: message!)
    modal.alertStyle = .warning
    return modal.runModal() == .OK
}

fileprivate func freeBuffer(_ buffer: CBuffer?) { buffer?.deallocate() }

//
// Helpers
//

fileprivate func bundleName() -> String? {
    Bundle.main.object(forInfoDictionaryKey: "CFBundleDisplayName") as? String
}

fileprivate func bundleHighRes() -> Bool? {
    Bundle.main.object(forInfoDictionaryKey: "NSHighResolutionCapable")
        as? Bool
}
