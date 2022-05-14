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

func readCStream(binary: Bool = false,
                 operation: @escaping CStreamOp) async -> CStreamResult {
    let p = Pipe()
    errno = 0
    guard let stream = fdopen(p.fileHandleForWriting.fileDescriptor,
                              binary ? "wb" : "w") else {
        return .error(.ioErrno)
    }

    let op = OpRunner(operation)
    let asyncStream = AsyncStream<Data> { continuation in
        p.fileHandleForReading.readabilityHandler = { handle in
            let d = handle.availableData
            if d.isEmpty {
                handle.readabilityHandler = nil
                continuation.finish()
            }
            continuation.yield(d)
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
