//
//  Platform.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/17/22.
//

import Foundation

typealias CBuffer = UnsafeMutablePointer<CChar>
typealias CString = UnsafePointer<CChar>
typealias HPlatform = UnsafeMutablePointer<gui_platform>
typealias HContext = UnsafeMutablePointer<UnsafeMutableRawPointer?>
typealias RenderFunc = @convention(c) (UnsafeMutableRawPointer?) -> Float

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
    return false
}

fileprivate func renderScaleFactor(window: OpaquePointer?) -> Float {
    return 1.0
}

fileprivate func openFile() -> CBuffer? {
    return nil
}

fileprivate func displayError(title: CString?, message: CString?) -> Bool {
    return false
}

fileprivate func freeBuffer(buffer: CBuffer?) {
    buffer?.deallocate()
}

fileprivate func cleanup(ctx: HContext?) {
    let p = ctx?.pointee
    let _: TestLifetime = Unmanaged.fromOpaque(p.unsafelyUnwrapped)
                            .takeRetainedValue()
    ctx?.pointee = nil
}

fileprivate func bundleName() -> Any? {
    Bundle.main.object(forInfoDictionaryKey: "CFBundleDisplayName")
}
