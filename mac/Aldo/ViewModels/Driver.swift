//
//  Driver.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 9/17/22.
//

import SpriteKit

final class EmulatorScene: SKScene {
    private static let blockHalfDim = 25.0
    private static let blockDim = blockHalfDim * 2
    private static let waitFrames = 120

    let clock = FrameClock()
    private let block = SKSpriteNode(color: .yellow,
                                     size: .init(
                                        width: EmulatorScene.blockDim,
                                        height: EmulatorScene.blockDim))
    private var velocity: CGPoint = .init(x: 1, y: 1)
    private var waitCount = 0

    override func didMove(to view: SKView) {
        size = view.bounds.size
        backgroundColor = .cyan
        block.position = .init(x: size.width / 2,
                                    y: size.height / 2)
        addChild(block)
    }

    override func update(_ currentTime: TimeInterval) {
        clock.frameStart(currentTime)
        if waitCount < Self.waitFrames {
            waitCount += 1
            return
        }
        block.position.x += velocity.x
        if block.position.x - Self.blockHalfDim < 0
            || block.position.x + Self.blockHalfDim >= size.width {
            velocity.x = -velocity.x
        }
        block.position.y += velocity.y
        if block.position.y - Self.blockHalfDim < 0
            || block.position.y + Self.blockHalfDim >= size.height {
            velocity.y = -velocity.y
        }
    }

    override func didFinishUpdate() {
        clock.frameEnd()
    }
}

final class FrameClock: ObservableObject {
    static let targetFps = 60
    static let publishIntervalMs = 250.0

    @Published private(set) var info = cycleclock()
    @Published private(set) var frameTime = 0.0
    @Published private(set) var frameLeft = 0.0
    private var current = 0.0
    private var previous = 0.0
    private var start: Double?

    func frameStart(_ currentTime: TimeInterval) {
        if start == nil {
            start = currentTime
        }
        current = currentTime
        frameTime = current - previous
        info.runtime = current - start!
    }

    func frameEnd() {
        previous = current
        info.frames += 1
    }
}
