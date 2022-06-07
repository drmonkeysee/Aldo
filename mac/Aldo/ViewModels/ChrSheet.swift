//
//  ChrSheet.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/22/22.
//

import Cocoa

final class ChrBanks {
    let cart: Cart
    private let store: ChrStore

    var count: Int { chrBanks(cart) }

    init(_ cart: Cart) {
        self.cart = cart
        store = .init(cart)
    }

    func sheet(at bank: Int) -> ChrSheet { .init(store, bank: bank) }
}

final class ChrSheet: ObservableObject {
    static let scale = 2

    let bank: Int
    @Published private(set) var status = BankLoadStatus<NSImage>.pending
    private let store: ChrStore

    fileprivate init(_ store: ChrStore, bank: Int) {
        self.store = store
        self.bank = bank
    }

    @MainActor
    func load() async {
        status = await store.fetch(bank: bank, scale: Self.scale)
    }
}

final class ChrExport: ObservableObject {
    let cart: Cart
    let scales = Array(Int(MinChrScale)...Int(MaxChrScale))
    @Published var scale = ChrSheet.scale
    @Published var done = false
    @Published var failed = false
    private(set) var selectedFolder: URL?
    private(set) var currentError: AldoError?

    init(_ cart: Cart) {
        self.cart = cart
    }

    @MainActor
    func exportChrBanks(to: URL?) async {
        guard let folder = to else { return }

        selectedFolder = folder
        done = false
        currentError = nil
        let result = await cart.exportChrBanks(scale: scale, folder: folder)
        switch result {
        case let .success(data):
            if let text = String(data: data, encoding: .utf8) {
                aldoLog.debug("CHR Export:\n\(text)")
                done = true
            } else {
                setError(.unknown)
            }
        case let .error(err):
            setError(err)
        }
    }

    private func setError(_ err: AldoError) {
        currentError = err
        failed = true
    }
}

fileprivate func chrBanks(_ cart: Cart) -> Int { cart.info.chrBanks }

fileprivate actor ChrStore {
    let cart: Cart
    let cache: BankCache<NSImage>

    init(_ cart: Cart) {
        self.cart = cart
        cache = cart.chrCache
        cache.ensure(slots: chrBanks(cart))
    }

    func fetch(bank: Int, scale: Int) async -> BankLoadStatus<NSImage> {
        if let img = cache[bank] { return .loaded(img) }

        let result = await cart.readChrBank(bank: bank, scale: scale)
        switch result {
        case let .success(data):
            let chrSheet = NSImage(data: data)
            if let img = chrSheet, img.isValid {
                cache[bank] = img
                return .loaded(img)
            }
            let reason = chrSheet == nil
                            ? "Image init failed"
                            : "Image data invalid"
            aldoLog.debug("CHR Decode Failure: \(reason)")
            return .failed
        case let .error(err):
            aldoLog.debug("CHR Read Failure: \(err.message)")
            return .failed
        }
    }
}
