//
//  AldoError.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/10/22.
//

import Foundation

enum AldoError: Error {
    case unknown
    case ioError(String)
    case cartErr(Int32)
    case disErr(Int32)

    var message: String {
        switch self {
        case .unknown:
            return "Unknown error"
        case let .ioError(str):
            return str
        case let .cartErr(code):
            return "\(String(cString: cart_errstr(code))) (\(code))"
        case let .disErr(code):
            var msg = "\(String(cString: dis_errstr(code))) (\(code))"
            if code == DIS_ERR_ERNO {
                msg.append(": \(String(cString: strerror(errno)))")
            }
            return msg
        }
    }
}
