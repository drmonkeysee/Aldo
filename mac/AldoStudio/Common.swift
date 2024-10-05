//
//  Common.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 12/22/22.
//

import Foundation
import os

typealias CInstPtr = UnsafePointer<aldo_dis_instruction>
typealias CStream = UnsafeMutablePointer<FILE>
typealias CStreamOp = (CStream) throws -> Void

let aldoLog = Logger()

func bundleAppName() -> String? {
    Bundle.main.object(forInfoDictionaryKey: "CFBundleDisplayName") as? String
}
