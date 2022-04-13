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
        handle = tryHandleOp { try CartHandle(filePath) }
        guard handle != nil else { return false }
        file = filePath
        info = handle?.getCartInfo() ?? .none
        return true
    }

    func getInfoText() -> String? {
        return tryHandleOp(handle?.getInfoText)
    }

    func getChrBank(bank: Int) -> NSImage? {
        // TODO: cache images?
        guard handle != nil else { return nil }

        let chrImage = tryHandleOp { try self.handle?.getChrBank(bank) }
        #if DEBUG
        if chrImage?.isValid != true {
            let msg = currentError?.message ?? (chrImage == nil
                                                ? "Image init failed"
                                                : "Image data invalid")
            print("CHR decode failure: \(msg)")
        }
        #endif
        return chrImage
    }

    private func tryHandleOp<T>(_ op: (() throws -> T?)?) -> T? {
        currentError = nil
        if let operation = op {
            do {
                return try operation()
            } catch let err as AldoError {
                currentError = err
            } catch {
                currentError = AldoError.systemError(
                    error.localizedDescription)
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
    private typealias CFile = UnsafeMutablePointer<FILE>

    private var cartRef: OpaquePointer?

    init(_ fromFile: URL) throws {
        errno = 0
        let cFile = fromFile.withUnsafeFileSystemRepresentation { name in
            fopen(name, "rb")
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
        if let o = output, let result = String(data: o, encoding: .utf8) {
            return result
        }
        throw AldoError.unknown
    }

    func getChrBank(_ bank: Int) throws -> NSImage? {
        try verifyRef()

        var bankview = cart_chrbank(cartRef, bank)
        let streamData = try captureCStream(binary: true) { stream in
            let err = dis_cart_chrbank(&bankview, 1, stream)
            if err < 0 { throw AldoError.wrapDisError(code: err) }
        }
        if let sd = streamData {
            return NSImage(data: sd)
        }
        throw AldoError.unknown
    }

    private func verifyRef() throws {
        if cartRef == nil { throw AldoError.ioError("No cart set") }
    }

    private func captureCStream(binary: Bool = false,
                                op: (CFile) throws -> Void) throws -> Data? {
        let p = Pipe()
        guard let stream = fdopen(p.fileHandleForWriting.fileDescriptor,
                                  binary ? "wb" : "w") else {
            throw AldoError.ioError("Cannot open cart data stream")
        }
        do {
            defer { fclose(stream) }
            try op(stream)
        }
        return try p.fileHandleForReading.readToEnd()
    }
}
