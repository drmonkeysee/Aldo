//
//  Cart.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 3/12/22.
//

import Cocoa
import Observation

@Observable
final class Cart {
    let chrCache = BlockCache<NSImage>()
    private(set) var file: URL?
    private(set) var info = CartInfo.empty()
    private(set) var prg = ProgramBlocks.empty()
    private(set) var chr = ChrBlocks.empty()
    @ObservationIgnored private(set) var currentError: AldoError?
    private var handle: CartHandle?

    // TODO: make whole class MainActor
    @MainActor
    func load(from: URL?) async -> Bool {
        currentError = nil
        guard let from else {
            currentError = .ioError("No file selected")
            return false
        }
        guard let h = loadCart(from) else { return false }
        handle = h
        file = from
        info = await parseInfo(from, handle: h)
        prg = await ProgramBlocks.loadBlocks(from: self)
        chr = await ChrBlocks.loadBlocks(from: self)
        resetCaches()
        return true
    }

    func getPrgBlock(_ at: Int) -> blockview? {
        guard let handle else { return nil }
        return cart_prgblock(handle.unwrapped, at)
    }

    @MainActor
    func readPrgRom() async -> CStreamResult {
        guard let fileName = info.fileName, let handle else { return .noCart }

        return await readCStream { stream in
            try fileName.withCString { cartFile in
                let err = dis_cart_prg(handle.unwrapped, info.cartName, false, true,
                                       stream)
                if err < 0 { throw AldoError.wrapDisError(code: err) }
            }
        }
    }

    @MainActor
    func readChrBlock(at: Int, scale: Int) async -> CStreamResult {
        guard let handle else { return .noCart }

        return await readCStream(binary: true) { stream in
            let bv = cart_chrblock(handle.unwrapped, at)
            let err = withUnsafePointer(to: bv) {
                dis_cart_chrblock($0, .init(scale), stream)
            }
            if err < 0 { throw AldoError.wrapDisError(code: err) }
        }
    }

    @MainActor
    func exportChrRom(scale: Int, folder: URL) async -> CStreamResult {
        guard let name = info.cartName, let handle else { return .noCart }

        return await readCStream { stream in
            let prefix = "\(folder.appendingPathComponent(name).path)-chr"
            try prefix.withCString { chrprefix in
                errno = 0
                let err = dis_cart_chr(handle.unwrapped, Int32(scale),
                                       chrprefix, stream)
                if err < 0 {
                    if err == DIS_ERR_ERNO { throw AldoError.ioErrno }
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

    @MainActor
    private func parseInfo(_ filePath: URL,
                           handle: CartHandle) async -> CartInfo {
        let cartName = filePath.deletingPathExtension().lastPathComponent
        return .init(fileName: filePath.lastPathComponent,
                     cartName: cartName, format: handle.cartFormat,
                     formatText: await readInfoText(cartName: cartName))
    }

    @MainActor
    private func readInfoText(cartName: String) async -> CStreamResult {
        guard let handle else { return .noCart }

        return await readCStream {
            cart_write_info(handle.unwrapped, cartName, true, $0)
        }
    }

    private func resetCaches() {
        chrCache.reset()
    }
}

struct CartInfo {
    static func empty() -> CartInfo {
        .init(fileName: nil, cartName: nil, format: .none, formatText: .noCart)
    }

    let fileName: String?
    let cartName: String?
    let format: CartFormat
    let formatText: CStreamResult
}

enum CartFormat {
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

    var prgBlocks: Int {
        switch self {
        case .iNes(_, let header, _):
            return .init(header.prg_blocks)
        case .raw:
            return 1
        default:
            return 0
        }
    }

    var chrBlocks: Int {
        switch self {
        case .iNes(_, let header, _):
            return .init(header.chr_blocks)
        default:
            return 0
        }
    }
}

enum BlockLoadStatus<T> {
    case pending
    case loaded(T)
    case failed
}

final class BlockCacheReal<T> {
    var capacity: Int { items.count }
    private var items: [BlockLoadStatus<T>]

    init(capacity: Int) { items = .init(repeating: .pending, count: capacity) }

    subscript(index: Int) -> BlockLoadStatus<T> {
        get { items.indices.contains(index) ? items[index] : .pending }
        set(newValue) {
            if items.indices.contains(index) {
                items[index] = newValue
            }
        }
    }

    func reset() { items = [] }
}

final class BlockCache<T> {
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

    var cartFormat: CartFormat {
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
        let cFile = fromFile.withUnsafeFileSystemRepresentation {
            fopen($0, "rb")
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
