//
//  Tests.swift
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

import XCTest

final class Tests: XCTestCase {
    func testRunner() throws {
        let failed = swift_runner()

        XCTAssertEqual(0, failed)
    }
}
