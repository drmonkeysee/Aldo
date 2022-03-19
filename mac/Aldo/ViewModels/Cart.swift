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
        guard let filePath = from else { return false }
        loadError = nil
        do {
            handle = try CartHandle(fromFile: filePath)
            file = filePath
            info = .raw
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
    case raw

    var name: String { "\(self)".capitalized }
}

enum CartError: Error {
    case unknown
    case ioError(String)
    case errCode(Int32)

    var message: String {
        switch self {
        case .unknown:
            return "Unknown error"
        case .ioError(let str):
            return str
        case .errCode(let code):
            return "\(String(cString: cart_errstr(code))) (\(code))"
        }
    }
}

fileprivate final class CartHandle {
    private var handle: OpaquePointer? = nil

    init(fromFile: URL) throws {
        errno = 0
        guard let cFile = fromFile.withUnsafeFileSystemRepresentation({
            fopen($0, "rb")
        }) else {
            throw CartError.ioError(String(cString: strerror(errno)))
        }

        defer { fclose(cFile) }
        let err = cart_create(&handle, cFile)
        guard err == 0 else {
            throw CartError.errCode(err)
        }
    }

    deinit {
        guard self.handle != nil else { return }
        cart_free(self.handle)
        self.handle = nil
    }
}
