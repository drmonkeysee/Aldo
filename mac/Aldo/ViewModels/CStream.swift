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
    g.notify(queue: DispatchQueue.main) { onComplete(.success(streamData)) }

    do {
        defer { fclose(stream) }
        try operation(stream)
    } catch let error as AldoError {
        onComplete(.error(error))
    } catch {
        onComplete(.error(.systemError(error.localizedDescription)))
    }
}
