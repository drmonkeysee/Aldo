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

    var message: String {
        switch self {
        case .unknown:
            return "Unknown error"
        case let .ioError(str):
            return str
        case let .cartErr(code):
            return "\(String(cString: cart_errstr(code))) (\(code))"
        }
    }
}
