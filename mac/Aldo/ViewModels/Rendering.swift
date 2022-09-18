//
//  Rendering.swift
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
    private let screen = SKSpriteNode(color: .cyan,
                                      size: .init(width: 256, height: 240))
    private let block = SKSpriteNode(color: .yellow,
                                     size: .init(
                                        width: EmulatorScene.blockDim,
                                        height: EmulatorScene.blockDim))
    private let otherBlock = SKSpriteNode(color: .blue,
                                          size: .init(width: 256, height: 240))
    private var velocity = CGVector(dx: 1, dy: 1)
    private var waitCount = 0

    override func didMove(to view: SKView) {
        size = view.bounds.size
        scaleMode = .resizeFill
        backgroundColor = .black
        addChild(screen)
        addChild(otherBlock)
        screen.addChild(block)
    }

    override func didChangeSize(_ oldSize: CGSize) {
        screen.position = .init(x: size.width / 2, y: size.height / 2)
    }

    override func update(_ currentTime: TimeInterval) {
        block.position.x += velocity.dx
        let screenHalfWidth = screen.size.width / 2
        let screenHalfHeight = screen.size.height / 2
        if block.position.x - Self.blockHalfDim < -screenHalfWidth
            || block.position.x + Self.blockHalfDim >= screenHalfWidth {
            velocity.dx = -velocity.dx
        }
        block.position.y += velocity.dy
        if block.position.y - Self.blockHalfDim < -screenHalfHeight
            || block.position.y + Self.blockHalfDim >= screenHalfHeight {
            velocity.dy = -velocity.dy
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
