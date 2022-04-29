//
//  CStream.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/20/22.
//

import Foundation

typealias CStream = UnsafeMutablePointer<FILE>

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
