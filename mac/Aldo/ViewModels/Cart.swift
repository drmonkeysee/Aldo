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
    private(set) var currentError: AldoError?
    private var handle: CartHandle?

    var name: String? { file?.lastPathComponent }

    func load(from: URL?) -> Bool {
        currentError = nil
        guard let filePath = from else {
            currentError = .ioError("No file selected")
            return false
        }
        handle = loadCart(filePath)
        guard let h = handle else { return false }
        file = filePath
        info = h.cartInfo
        return true
    }

    func readInfoText() async -> CStreamResult {
        guard let h = handle else { return .error(.ioError("No cart set")) }

        return await readCStream { stream in
            cart_write_info(h.unwrapped, self.name, stream, true)
        }
    }

    func readChrBank(bank: Int, scale: Int) async -> CStreamResult {
        guard let h = handle else { return .error(.ioError("No cart set")) }

        return await readCStream(binary: true) { stream in
            var bankview = cart_chrbank(h.unwrapped, bank)
            let err = dis_cart_chrbank(&bankview, Int32(scale), stream)
            if err < 0 { throw AldoError.wrapDisError(code: err) }
        }
    }

    private func loadCart(_ filePath: URL) -> CartHandle? {
        currentError = nil
        do {
            return try .init(filePath)
        } catch let error as AldoError {
            currentError = error
        } catch {
            currentError = .systemError(error.localizedDescription)
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

    var prgBanks: Int {
        switch self {
        case .iNes(_, let header, _):
            return Int(header.prg_chunks)
        case .raw:
            return 1
        default:
            return 0
        }
    }

    var chrBanks: Int {
        switch self {
        case .iNes(_, let header, _):
            return Int(header.chr_chunks)
        default:
            return 0
        }
    }
}

enum BankLoadStatus<T> {
    case pending
    case loaded(T)
    case failed
}

final class BankCache<T> {
    private var items: [T?]

    init(capacity: Int) { items = .init(repeating: nil, count: capacity) }

    subscript(index: Int) -> T? {
        get {
            guard validIndex(index) else { return nil }
            return items[index]
        }
        set(newValue) {
            if validIndex(index), let val = newValue { items[index] = val }
        }
    }

    private func validIndex(_ index: Int) -> Bool {
        items.startIndex <= index && index < items.endIndex
    }
}

fileprivate final class CartHandle {
    private var cartRef: OpaquePointer?

    // NOTE: init won't let a live instance ever have a nil reference
    var unwrapped: OpaquePointer { cartRef! }

    var cartInfo: CartInfo {
        var info = cartinfo()
        cart_getinfo(cartRef, &info)
        switch info.format {
        case CRTF_RAW:
            return .raw(.init(cString: cart_formatname(info.format)))
        case CRTF_INES:
            return .iNes(.init(cString: cart_formatname(info.format)),
                         info.ines_hdr,
                         .init(cString: cart_mirrorname(info.ines_hdr.mirror)))
        default:
            return .unknown
        }
    }

    init(_ fromFile: URL) throws {
        errno = 0
        let cFile = fromFile.withUnsafeFileSystemRepresentation { name in
            fopen(name, "rb")
        }
        guard cFile != nil else { throw AldoError.ioErrno }
        defer { fclose(cFile) }

        let err = cart_create(&cartRef, cFile)
        if err < 0 { throw AldoError.cartErr(err) }
    }

    deinit {
        guard cartRef != nil else { return }
        cart_free(cartRef)
        cartRef = nil
    }
}
