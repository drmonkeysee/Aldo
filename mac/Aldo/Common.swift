//
//  Common.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/18/22.
//

import Foundation
import os

typealias CBuffer = UnsafeMutablePointer<CChar>
typealias CString = UnsafePointer<CChar>

typealias PlatformCtx = UnsafeMutableRawPointer
typealias PlatformCtxHandle = UnsafeMutablePointer<PlatformCtx?>
typealias PlatformHandle = UnsafeMutablePointer<gui_platform>
typealias PlatformRenderFunc =
    @convention(c) (UnsafeMutableRawPointer?) -> Float

let aldoLog = Logger()
