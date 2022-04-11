//
//  Cart.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/12/22.
//

import Cocoa
import Foundation

final class Cart: ObservableObject {
    @Published private(set) var file: URL?
    @Published private(set) var info = CartInfo.none
    private(set) var currentError: AldoError?
    private var handle: CartHandle?

    var name: String? { file?.lastPathComponent }

    func load(from: URL?) -> Bool {
        currentError = nil
        guard let filePath = from else {
            currentError = AldoError.ioError("No file selected")
            return false
        }
        do {
            handle = try CartHandle(filePath)
        } catch let err as AldoError {
            currentError = err
            return false
        } catch {
            currentError = AldoError.unknown
            return false
        }
        file = filePath
        info = handle?.getCartInfo() ?? .none
        return true
    }

    func getInfoText() -> String? {
        return getHandleData(handle?.getInfoText)
    }

    func getChrBank(bank: Int) -> NSImage? {
        return getHandleData { try self.handle?.getChrBank(bank) }
    }

    private func getHandleData<T>(_ op: (() throws -> T?)?) -> T? {
        if let operation = op {
            do {
                return try operation()
            } catch let err as AldoError {
                currentError = err
            } catch {
                currentError = AldoError.unknown
            }
        }
        return nil
    }
}

enum CartInfo {
    case none
    case unknown
    case raw(String)
    case iNes(String, ines_header, String)

    var name: String {
        switch self {
        case let .raw(str), .iNes(let str, _, _):
            return str
        default:
            return "\(self)".capitalized
        }
    }
}

fileprivate final class CartHandle {
    private var cartRef: OpaquePointer? = nil

    init(_ fromFile: URL) throws {
        errno = 0
        let cFile = fromFile.withUnsafeFileSystemRepresentation {
            fopen($0, "rb")
        }
        guard cFile != nil else {
            throw AldoError.ioError(String(cString: strerror(errno)))
        }
        defer { fclose(cFile) }

        let err = cart_create(&cartRef, cFile)
        if err < 0 { throw AldoError.cartErr(err) }
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
                         info.ines_hdr,
                         String(
                            cString: cart_mirrorname(info.ines_hdr.mirror)))
        default:
            return .unknown
        }
    }

    func getInfoText() throws -> String {
        try verifyRef()

        let output = try captureCStream { stream in
            cart_write_info(cartRef, stream, true)
        }
        guard let result = String(data: output, encoding: .utf8) else {
            throw AldoError.unknown
        }
        return result
    }

    func getChrBank(_ bank: Int) throws -> NSImage? {
        try verifyRef()
        var bankview = cart_chrbank(cartRef, bank)
        let streamData = try captureCStream { stream in
            let err = dis_cart_chrbank(&bankview, 2, stream)
            if err < 0 { throw AldoError.wrapDisError(code: err) }
        }
        return NSImage(data: streamData)
    }

    private func verifyRef() throws {
        if cartRef == nil { throw AldoError.ioError("No cart set") }
    }

    private func
    captureCStream(_ op: (UnsafeMutablePointer<FILE>) throws -> Void)
    throws -> Data {
        let p = Pipe()
        guard let stream = fdopen(p.fileHandleForWriting.fileDescriptor, "w")
        else {
            throw AldoError.ioError("Cannot open cart data stream")
        }

        do {
            defer { fclose(stream) }
            try op(stream)
        }

        return p.fileHandleForReading.availableData
    }
}
