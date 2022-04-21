//
//  AldoError.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/10/22.
//

import Foundation

enum AldoError: Error {
    private static let errCodeFormat = "%s (%d)"

    static var ioErrno: Self {
        .ioError(String(cString: strerror(errno)))
    }

    static func wrapDisError(code: Int32) -> Self {
        .disErr(code, code == DIS_ERR_ERNO
                        ? String(cString: strerror(errno))
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
            return String(format: AldoError.errCodeFormat, cart_errstr(code),
                          code)
        case let .disErr(code, sysErr):
            var msg = String(format: AldoError.errCodeFormat, dis_errstr(code),
                             code)
            if let se = sysErr {
                msg.append(": \(se)")
            }
            return msg
        }
    }
}
