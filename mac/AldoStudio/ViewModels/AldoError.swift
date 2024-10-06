//
//  AldoError.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 4/10/22.
//

import Foundation

enum AldoError: Error {
    static var ioErrno: Self { .ioError(.init(cString: strerror(errno))) }
    private static let errCodeFormat = "%s (%d)"

    static func wrapDisError(code: Int32) -> Self {
        .disErr(code, code == ALDO_DIS_ERR_ERNO
                        ? .init(cString: strerror(errno))
                        : nil)
    }

    case unknown
    case systemError(String)
    case ioError(String)
    case cartErr(Int32)
    case disErr(Int32, String?)

    var message: String {
        switch self {
        case .unknown:
            return "Unknown error"
        case let .systemError(str), let .ioError(str):
            return str
        case let .cartErr(code):
            return .init(format: Self.errCodeFormat, aldo_cart_errstr(code),
                         code)
        case let .disErr(code, sysErr):
            var msg = String(format: Self.errCodeFormat, aldo_dis_errstr(code),
                             code)
            if let sysErr { msg.append(": \(sysErr)") }
            return msg
        }
    }
}
