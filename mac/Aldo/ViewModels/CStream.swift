//
//  CStream.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/20/22.
//

import Foundation

typealias CStream = UnsafeMutablePointer<FILE>
typealias CStreamOp = (CStream) throws -> Void

enum CStreamResult {
    case success(Data)
    case error(AldoError)
}

func readCStream(binary: Bool = false, operation: (CStream) throws -> Void,
                 onComplete: @escaping (CStreamResult) -> Void) {
    let p = Pipe()
    errno = 0
    guard let stream = fdopen(p.fileHandleForWriting.fileDescriptor,
                              binary ? "wb" : "w") else {
        onComplete(.error(.ioErrno))
        return
    }

    var streamData = Data()
    var opFailed = false
    let g = DispatchGroup()
    g.enter()
    p.fileHandleForReading.readabilityHandler = { h in
        let d = h.availableData
        if d.isEmpty {
            p.fileHandleForReading.readabilityHandler = nil
            g.leave()
            return
        }
        streamData.append(d)
    }
    g.notify(queue: .main) {
        if opFailed { return }
        onComplete(.success(streamData))
    }

    do {
        defer { fclose(stream) }
        try operation(stream)
    } catch let error as AldoError {
        opFailed = true
        onComplete(.error(error))
    } catch {
        opFailed = true
        onComplete(.error(.systemError(error.localizedDescription)))
    }
}

func readCStreamAsync(binary: Bool = false,
                      operation: @escaping CStreamOp) async -> CStreamResult {
    let p = Pipe()
    errno = 0
    guard let stream = fdopen(p.fileHandleForWriting.fileDescriptor,
                              binary ? "wb" : "w") else {
        return .error(.ioErrno)
    }

    let op = OpRunner(operation)
    let asyncStream = AsyncStream<Data> { c in
        p.fileHandleForReading.readabilityHandler = { h in
            let d = h.availableData
            if d.isEmpty {
                p.fileHandleForReading.readabilityHandler = nil
                c.finish()
            }
            c.yield(d)
        }
    }

    do {
        try await op.run(stream)
    } catch let error as AldoError {
        return .error(error)
    } catch {
        return .error(.systemError(error.localizedDescription))
    }

    var streamData = Data()
    for await d in asyncStream {
        streamData.append(d)
    }
    return .success(streamData)
}

fileprivate actor OpRunner {
    private let op: CStreamOp

    init(_ op: @escaping CStreamOp) { self.op = op }

    func run(_ stream: CStream) throws {
        defer { fclose(stream) }
        try op(stream)
    }
}
