//
//  Aliases.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/18/22.
//

import Foundation

typealias CBuffer = UnsafeMutablePointer<CChar>
typealias CString = UnsafePointer<CChar>
typealias PlatformHandle = UnsafeMutablePointer<gui_platform>
typealias ContextHandle = UnsafeMutablePointer<UnsafeMutableRawPointer?>
typealias PlatformRenderFunc =
    @convention(c) (UnsafeMutableRawPointer?) -> Float
