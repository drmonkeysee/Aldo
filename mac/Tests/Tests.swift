//
//  Tests.swift
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

import Testing

struct Tests {
    @Test
    func testRunner() {
        let failed = swift_runner()

        #expect(failed == 0)
    }
}
