//
//  Cart.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/12/22.
//

import Cocoa

final class Cart: ObservableObject {
    let prgCache = BankCache<[PrgLine]>()
    let chrCache = BankCache<NSImage>()
    @Published private(set) var file: URL?
    @Published private(set) var info = CartInfo.none
    private(set) var currentError: AldoError?
    private var handle: CartHandle?

    var fileName: String? { file?.lastPathComponent }
    var name: String? { file?.deletingPathExtension().lastPathComponent }
    private var noCart: CStreamResult { .error(.ioError("No cart set")) }

    func load(from: URL?) -> Bool {
        currentError = nil
        guard let filePath = from else {
            currentError = .ioError("No file selected")
            return false
        }
        guard let h = loadCart(filePath) else { return false }
        handle = h
        file = filePath
        info = h.cartInfo
        resetCaches()
        return true
    }

    func readInfoText() async -> CStreamResult {
        guard let h = handle else { return noCart }

        return await readCStream { stream in
            cart_write_info(h.unwrapped, self.fileName, stream, true)
        }
    }

    func getPrgBank(_ bank: Int) -> bankview? {
        guard let h = handle else { return nil }
        return cart_prgblock(h.unwrapped, bank)
    }

    func readAllPrgBanks() async -> CStreamResult {
        guard let n = fileName, let h = handle else { return noCart }

        return await readCStream { stream in
            try n.withCString { cartFile in
                var appstate = control()
                appstate.cartfile = cartFile
                appstate.unified_disoutput = true
                let err = dis_cart_prg(h.unwrapped, &appstate, stream)
                if err < 0 { throw AldoError.wrapDisError(code: err) }
            }
        }
    }

    func readChrBank(bank: Int, scale: Int) async -> CStreamResult {
        guard let h = handle else { return noCart }

        return await readCStream(binary: true) { stream in
            let bankview = cart_chrblock(h.unwrapped, bank)
            let err = withUnsafePointer(to: bankview) { p in
                dis_cart_chrbank(p, .init(scale), stream)
            }
            if err < 0 { throw AldoError.wrapDisError(code: err) }
        }
    }

    func exportChrBanks(scale: Int, folder: URL) async -> CStreamResult {
        guard let n = name, let h = handle else { return noCart }

        return await readCStream { stream in
            let prefix = "\(folder.appendingPathComponent(n).path)-bank"
            try prefix.withCString { chrprefix in
                var appstate = control()
                appstate.unified_disoutput = true
                appstate.chrdecode_prefix = chrprefix
                appstate.chrscale = Int32(scale)
                errno = 0
                let err = dis_cart_chr(h.unwrapped, &appstate, stream)
                if err < 0 {
                    if err == DIS_ERR_ERNO {
                        throw AldoError.ioErrno
                    }
                    throw AldoError.wrapDisError(code: err)
                }
            }
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

    private func resetCaches() {
        prgCache.reset()
        chrCache.reset()
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
            return .init(header.prg_chunks)
        case .raw:
            return 1
        default:
            return 0
        }
    }

    var chrBanks: Int {
        switch self {
        case .iNes(_, let header, _):
            return .init(header.chr_chunks)
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
        get { items.indices.contains(index) ? items[index] : nil }
        set(newValue) {
            if items.indices.contains(index), let val = newValue {
                items[index] = val
            }
        }
    }

    func ensure(slots: Int) {
        guard items.isEmpty else { return }
        items = .init(repeating: nil, count: slots)
    }

    func reset() { items = [] }
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
