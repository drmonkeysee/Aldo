//
//  Common.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/18/22.
//

import Foundation
import os

typealias CBuffer = UnsafeMutablePointer<CChar>
typealias CInstPtr = UnsafePointer<dis_instruction>
typealias ContextHandle = UnsafeMutablePointer<UnsafeMutableRawPointer?>
typealias CStream = UnsafeMutablePointer<FILE>
typealias CStreamOp = (CStream) throws -> Void
typealias CString = UnsafePointer<CChar>
typealias PlatformHandle = UnsafeMutablePointer<gui_platform>
typealias PlatformRenderFunc =
    @convention(c) (UnsafeMutableRawPointer?) -> Float

let aldoLog = Logger()
