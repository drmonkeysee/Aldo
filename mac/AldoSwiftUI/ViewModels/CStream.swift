//
//  CStream.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 4/20/22.
//

import Foundation

typealias CBuffer = UnsafeMutablePointer<CChar>
typealias CInstPtr = UnsafePointer<dis_instruction>
typealias CStream = UnsafeMutablePointer<FILE>
typealias CStreamOp = (CStream) throws -> Void

enum CStreamResult {
    case success(Data)
    case error(AldoError)
}

func readCStream(binary: Bool = false,
                 operation: CStreamOp) async -> CStreamResult {
    let p = Pipe()
    errno = 0
    // NOTE: when reading multiple async cstreams Swift and C don't seem to
    // agree on when a file descriptor is available and FDs will be recycled
    // for new Pipes before fclose frees them, causing a read-after-close
    // error; handing C a duplicate FD solves this problem as long as both
    // descriptors are closed in the right order (see cleanup below).
    guard let stream = fdopen(dup(p.fileHandleForWriting.fileDescriptor),
                              binary ? "wb" : "w") else {
        return .error(.ioErrno)
    }

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

    var cleanupError: CStreamResult?
    do {
        defer { cleanupError = cleanup(stream, p.fileHandleForWriting) }
        try operation(stream)
    } catch let error as AldoError {
        return .error(error)
    } catch {
        return .error(.systemError(error.localizedDescription))
    }
    if let err = cleanupError { return err }

    var streamData = Data()
    for await d in asyncStream { streamData.append(d) }
    return .success(streamData)
}

fileprivate func cleanup(_ stream: CStream,
                         _ file: FileHandle) -> CStreamResult? {
    // fclose the cstream first to flush the buffer and clean up the
    // FILE structure, then close the FileHandle so its readability
    // handler gets EOF and completes the buffered read.
    errno = 0
    if fclose(stream) == 0 {
        do {
            try file.close()
        } catch {
            return .error(.systemError(error.localizedDescription))
        }
    }
    else {
        return .error(.ioErrno)
    }
    return nil
}
