//
//  Platform.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/17/22.
//

import AppKit
import os

typealias CBuffer = UnsafeMutablePointer<CChar>
typealias CString = UnsafePointer<CChar>
typealias HPlatform = UnsafeMutablePointer<gui_platform>
typealias HContext = UnsafeMutablePointer<UnsafeMutableRawPointer?>
typealias RenderFunc = @convention(c) (UnsafeMutableRawPointer?) -> Float

let aldoLog = Logger()

final class Platform: NSObject {
    @objc static func setup(_ platform: HPlatform,
                            withScaleFunc: @escaping RenderFunc) -> Bool {
        let lifetime = TestLifetime()
        platform.pointee = .init(appname: appName,
                                 is_hidpi: isHiDPI,
                                 render_scale_factor: withScaleFunc,
                                 open_file: openFile,
                                 display_error: displayError,
                                 free_buffer: freeBuffer,
                                 cleanup: cleanup,
                                 ctx: Unmanaged.passRetained(lifetime)
                                        .toOpaque())
        return true
    }
}

class TestLifetime: NSObject {
    deinit {
        NSLog("DELETE TEST LIFETIME")
    }
}

//
// Platform Implementation
//

fileprivate func appName() -> CBuffer? {
    guard let displayName = bundleName() as? String,
          let cstring = displayName.cString(using: .utf8) else {
        return nil
    }
    let buffer = CBuffer.allocate(capacity: cstring.count)
    strcpy(buffer, cstring)
    return buffer
}

fileprivate func isHiDPI() -> Bool {
    guard let hidpi = bundleHighRes() as? Bool else { return false }
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

fileprivate func displayError(title: CString?, message: CString?) -> Bool {
    let modal = NSAlert()
    if let titlep = title {
        modal.messageText = .init(cString: titlep)
    } else {
        modal.messageText = "INVALID TITLE"
    }
    if let messagep = message {
        modal.informativeText = .init(cString: messagep)
    } else {
        modal.informativeText = "INVALID MESSAGE"
    }
    modal.alertStyle = .warning
    return modal.runModal() == .OK
}

fileprivate func freeBuffer(buffer: CBuffer?) {
    buffer?.deallocate()
}

fileprivate func cleanup(ctx: HContext?) {
    guard let p = ctx?.pointee else { return }
    let _: TestLifetime = Unmanaged.fromOpaque(p).takeRetainedValue()
    ctx?.pointee = nil
}

//
// Helpers
//

fileprivate func bundleName() -> Any? {
    Bundle.main.object(forInfoDictionaryKey: "CFBundleDisplayName")
}

fileprivate func bundleHighRes() -> Any? {
    Bundle.main.object(forInfoDictionaryKey: "NSHighResolutionCapable")
}
