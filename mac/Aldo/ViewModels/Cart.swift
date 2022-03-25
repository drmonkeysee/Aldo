//
//  Cart.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/12/22.
//

import Foundation

final class Cart: ObservableObject {
    @Published private(set) var file: URL?
    @Published private(set) var info = CartInfo.none

    private(set) var loadError: CartError?
    private var handle: CartHandle?

    var name: String? { file?.lastPathComponent }

    func load(from: URL?) -> Bool {
        loadError = nil
        guard let filePath = from else {
            loadError = CartError.ioError("No file selected")
            return false
        }
        do {
            handle = try CartHandle(fromFile: filePath)
            file = filePath
            info = handle?.getCartInfo() ?? .none
            return true
        } catch let err as CartError {
            loadError = err
            return false
        } catch {
            loadError = CartError.unknown
            return false
        }
    }
}

enum CartInfo {
    case none
    case unknown
    case raw(String)
    case iNes(String, ines_header)

    var name: String {
        switch self {
        case let .raw(str), let .iNes(str, _):
            return str
        default:
            return "\(self)".capitalized
        }
    }
}

enum CartError: Error {
    case unknown
    case ioError(String)
    case errCode(Int32)

    var message: String {
        switch self {
        case .unknown:
            return "Unknown error"
        case let .ioError(str):
            return str
        case let .errCode(code):
            return "\(String(cString: cart_errstr(code))) (\(code))"
        }
    }
}

fileprivate final class CartHandle {
    private var cartRef: OpaquePointer? = nil

    init(fromFile: URL) throws {
        errno = 0
        guard let cFile = fromFile.withUnsafeFileSystemRepresentation({
            fopen($0, "rb")
        }) else {
            throw CartError.ioError(String(cString: strerror(errno)))
        }
        defer { fclose(cFile) }

        let err = cart_create(&cartRef, cFile)
        guard err == 0 else { throw CartError.errCode(err) }
    }

    deinit {
        guard cartRef != nil else { return }
        cart_free(cartRef)
        cartRef = nil
    }

    func getCartInfo() -> CartInfo {
        guard cartRef != nil else { return .none }

        var info = cartinfo()
        cart_getinfo(cartRef, &info)
        switch info.format {
        case CRTF_RAW:
            return .raw(String(cString: cart_formatname(info.format)))
        case CRTF_INES:
            return .iNes(String(cString: cart_formatname(info.format)),
                         info.ines_hdr)
        default:
            return .unknown
        }
    }
}
