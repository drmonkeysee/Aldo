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
        .disErr(code, sysMsg(code, if: ALDO_DIS_ERR_ERNO))
    }

    static func wrapCartError(code: Int32) -> Self {
        .cartErr(code, sysMsg(code, if: ALDO_CART_ERR_ERNO))
    }

    private static func sysMsg(_ code: Int32, if errNo: Int) -> String? {
        code == errNo ? .init(cString: strerror(errno)) : nil
    }

    case unknown
    case systemError(String)
    case ioError(String)
    case cartErr(Int32, String?)
    case disErr(Int32, String?)

    var message: String {
        switch self {
        case .unknown:
            return "Unknown error"
        case let .systemError(str), let .ioError(str):
            return str
        case let .cartErr(code, sysErr):
            return errMessage(code: code, sysErr: sysErr) {
                .init(cString: aldo_cart_errstr($0))
            }
        case let .disErr(code, sysErr):
            return errMessage(code: code, sysErr: sysErr) {
                .init(cString: aldo_dis_errstr($0))
            }
        }
    }

    private func errMessage(code: Int32, sysErr: String?,
                            errRes: (Int32) -> String) -> String {
        var msg = String(format: Self.errCodeFormat, errRes(code), code)
        if let sysErr { msg.append(": \(sysErr)") }
        return msg
    }
}
