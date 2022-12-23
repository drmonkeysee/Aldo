//
//  Common.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 12/22/22.
//

import Foundation
import os

typealias CBuffer = UnsafeMutablePointer<CChar>
typealias CInstPtr = UnsafePointer<dis_instruction>
typealias CStream = UnsafeMutablePointer<FILE>
typealias CStreamOp = (CStream) throws -> Void

let aldoLog = Logger()
