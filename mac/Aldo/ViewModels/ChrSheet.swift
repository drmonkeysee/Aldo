//
//  ChrSheet.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/22/22.
//

import Cocoa

final class ChrBlocks {
    let cart: Cart
    private let store: ChrStore

    var count: Int { chrBlocks(cart) }

    init(_ cart: Cart) {
        self.cart = cart
        store = .init(cart)
    }

    func sheet(at: Int) -> ChrSheet { .init(store, index: at) }
}

final class ChrSheet: ObservableObject {
    static let scale = 2

    let index: Int
    @Published private(set) var status = BlockLoadStatus<NSImage>.pending
    private let store: ChrStore

    fileprivate init(_ store: ChrStore, index: Int) {
        self.store = store
        self.index = index
    }

    @MainActor
    func load() async {
        status = await store.fetch(at: index, scale: Self.scale)
    }
}

final class ChrExport: ObservableObject {
    static let transitionDuration = 2.0

    let cart: Cart
    let scales = Array(Int(MinChrScale)...Int(MaxChrScale))
    @Published var scale = ChrSheet.scale
    @Published var actionIconOpacity = 1.0
    @Published var successIconOpacity = 0.0
    @Published var inProgress = false
    @Published var done = false
    @Published var failed = false
    private(set) var selectedFolder: URL?
    private(set) var currentError: AldoError?

    init(_ cart: Cart) {
        self.cart = cart
    }

    @MainActor
    func exportChrRom(to: URL?) async {
        guard let folder = to else { return }

        inProgress = true
        done = false
        currentError = nil
        selectedFolder = folder
        let result = await cart.exportChrRom(scale: scale, folder: folder)
        switch result {
        case let .success(data):
            if let text = String(data: data, encoding: .utf8) {
                aldoLog.debug("CHR Export:\n\(text)")
                actionIconOpacity = 0.0
                successIconOpacity = 1.0
                done = true
                finishTransition()
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
        inProgress = false
    }

    private func finishTransition() {
        DispatchQueue.main.asyncAfter(deadline: .now()
                                      + Self.transitionDuration) {
            self.actionIconOpacity = 1.0
            self.inProgress = false
        }
    }
}

fileprivate func chrBlocks(_ cart: Cart) -> Int { cart.info.chrBlocks }

fileprivate actor ChrStore {
    let cart: Cart
    let cache: BlockCache<NSImage>

    init(_ cart: Cart) {
        self.cart = cart
        cache = cart.chrCache
        cache.ensure(slots: chrBlocks(cart))
    }

    func fetch(at: Int, scale: Int) async -> BlockLoadStatus<NSImage> {
        if let img = cache[at] { return .loaded(img) }

        let result = await cart.readChrBlock(at: at, scale: scale)
        switch result {
        case let .success(data):
            let chrSheet = NSImage(data: data)
            if let img = chrSheet, img.isValid {
                cache[at] = img
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
