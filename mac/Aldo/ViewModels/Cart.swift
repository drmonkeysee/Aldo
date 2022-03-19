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
    @Published private(set) var loadError: CartError?
    private var handle: CartHandle?

    var name: String? { file?.lastPathComponent }

    func load(from: URL?) {
        guard let filePath = from else { return }
        file = filePath
        do {
            handle = try CartHandle(fromFile: file!)
        } catch let err as CartError {
            loadError = err
        } catch {
            loadError = CartError.unknown
        }
        info = .raw
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

    func message() -> String {
        switch self {
        case .unknown:
            return "Unknown error"
        case .ioError(let str):
            return str
        case .errCode(let code):
            return String(cString: cart_errstr(code))
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
        cart_free(self.handle)
        self.handle = nil
    }
}
