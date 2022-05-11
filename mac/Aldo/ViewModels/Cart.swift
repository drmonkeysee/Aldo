//
//  Cart.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/12/22.
//

import Cocoa

final class Cart: ObservableObject {
    let prgListingCache = BankCache<[PrgLine]>()
    let chrSheetCache = BankCache<NSImage>()
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
        resetCaches()
        return true
    }

    func readInfoText(onComplete: @escaping (CStreamResult) -> Void) {
        guard let h = handle else {
            onComplete(.error(.ioError("No cart set")))
            return
        }
        readCStream(operation: { stream in
            cart_write_info(h.unwrapped, name, stream, true)
        }, onComplete: onComplete)
    }

    func readChrBank(bank: Int, scale: Int,
                     onComplete: @escaping (CStreamResult) -> Void) {
        guard let h = handle else {
            onComplete(.error(.ioError("No cart set")))
            return
        }
        readCStream(binary: true, operation: { stream in
            var bankview = cart_chrbank(h.unwrapped, bank)
            let err = dis_cart_chrbank(&bankview, Int32(scale), stream)
            if err < 0 { throw AldoError.wrapDisError(code: err) }
        }, onComplete: onComplete)
    }

    private func loadCart(_ filePath: URL) -> CartHandle? {
        currentError = nil
        do {
            return try CartHandle(filePath)
        } catch let error as AldoError {
            currentError = error
        } catch {
            currentError = .systemError(error.localizedDescription)
        }
        return nil
    }

    private func resetCaches() {
        prgListingCache.reset(capacity: info.prgBanks)
        chrSheetCache.reset(capacity: info.chrBanks)
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
    private var items = [T?]()

    subscript(index: Int) -> T? {
        get {
            guard validIndex(index) else { return nil }
            return items[index]
        }
        set(newValue) {
            if validIndex(index), let val = newValue { items[index] = val }
        }
    }

    fileprivate func reset(capacity: Int) {
        if capacity > 0 {
            items = [T?](repeating: nil, count: capacity)
        } else {
            items.removeAll()
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
