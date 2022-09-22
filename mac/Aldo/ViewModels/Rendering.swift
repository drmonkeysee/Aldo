//
//  Rendering.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 9/17/22.
//

import SpriteKit

final class EmulatorScene: SKScene {
    private static let waitFrames = 120

    let clock = FrameClock()
    private let testAnimation = AnimationTest()
    private let traits = EmulatorTraits()
    private let ram = RamViewer()
    private let otherBlock = SKSpriteNode(color: .blue,
                                          size: .init(width: 256, height: 240))
    private var waitCount = 0

    override func didMove(to view: SKView) {
        size = view.bounds.size
        scaleMode = .resizeFill
        backgroundColor = .black
        addChild(testAnimation.createRenderNode())
        addChild(traits.createRenderNode())
        addChild(ram.createRenderNode())
    }

    override func didChangeSize(_ oldSize: CGSize) {
        testAnimation.rootPosition = .init(x: size.width / 2,
                                           y: size.height / 2)
        traits.rootPosition = .init(x: (150 / 2) + 5,
                                    y: size.height - (200 / 2) - 5)
        ram.rootPosition = .init(x: (300 / 2) + 160,
                                 y: size.height - (500 / 2) - 5)
    }

    override func update(_ currentTime: TimeInterval) {
        clock.frameStart(currentTime)
        testAnimation.update(clock)
        traits.update(clock)
        ram.update(clock)
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

fileprivate protocol RenderElement {
    var rootPosition: CGPoint { get set }

    func createRenderNode() -> SKNode
    func update(_ clock: FrameClock)
}

fileprivate let labelFont = NSFont.monospacedSystemFont(ofSize: 10,
                                                        weight: .bold)
fileprivate func makeLabelNode() -> SKLabelNode {
    let label = SKLabelNode(fontNamed: labelFont.fontName)
    label.fontSize = 10
    label.horizontalAlignmentMode = .left
    return label
}

fileprivate final class AnimationTest: RenderElement {
    private let bounds = SKSpriteNode(color: .cyan,
                                      size: .init(width: 250, height: 200))
    private let bouncer = SKSpriteNode(color: .yellow,
                                       size: .init(width: 50, height: 50))
    private var velocity = CGVector(dx: 1, dy: 1)

    var rootPosition: CGPoint {
        get { bounds.position }
        set(value) { bounds.position = value }
    }

    func createRenderNode() -> SKNode {
        bounds.addChild(bouncer)
        return bounds
    }

    func update(_ clock: FrameClock) {
        bouncer.position.x += velocity.dx
        let halfBounds = CGSize(width: bounds.size.width / 2,
                                height: bounds.size.height / 2)
        let halfBounce = CGSize(width: bouncer.size.width / 2,
                                height: bouncer.size.height / 2)
        if bouncer.position.x - halfBounce.width < -halfBounds.width
            || bouncer.position.x + halfBounce.width >= halfBounds.width {
            velocity.dx = -velocity.dx
        }
        bouncer.position.y += velocity.dy
        if bouncer.position.y - halfBounce.height < -halfBounds.height
            || bouncer.position.y + halfBounce.height >= halfBounds.height {
            velocity.dy = -velocity.dy
        }
    }
}

fileprivate final class EmulatorTraits: RenderElement {
    private let box = SKSpriteNode(color: .darkGray,
                                   size: .init(width: 150, height: 200))
    private let fps = makeLabelNode()
    private let frames = makeLabelNode()
    private let runtime = makeLabelNode()

    var rootPosition: CGPoint {
        get { box.position }
        set(value) { box.position = value }
    }

    func createRenderNode() -> SKNode {
        let leftMargin = -(box.size.width / 2) + 5
        var topOffset = (box.size.height / 2) - 10
        fps.position = .init(x: leftMargin, y: topOffset)
        topOffset -= 15
        frames.position = .init(x: leftMargin, y: topOffset)
        topOffset -= 15
        runtime.position = .init(x: leftMargin, y: topOffset)
        box.addChild(fps)
        box.addChild(frames)
        box.addChild(runtime)
        return box
    }

    func update(_ clock: FrameClock) {
        fps.text = "FPS: 60"
        frames.text = "Frames: \(clock.info.frames)"
        runtime.text = "Runtime: \(String(format: "%.3f", clock.info.runtime))"
    }
}

fileprivate final class RamViewer: RenderElement {
    private let box = SKSpriteNode(color: .darkGray,
                                   size: .init(width: 300, height: 500))
    private let byteLabels = (0..<32).map { _ in makeLabelNode() }

    var rootPosition: CGPoint {
        get { box.position }
        set(value) { box.position = value }
    }

    func createRenderNode() -> SKNode {
        let leftMargin = -(box.size.width / 2) + 5
        var topOffset = (box.size.height / 2) - 20
        for y in 0..<32 {
            let lbl = byteLabels[y]
            lbl.position = .init(x: leftMargin, y: topOffset)
            box.addChild(lbl)
            topOffset -= 15
        }
        return box
    }

    func update(_ clock: FrameClock) {
        for lbl in byteLabels {
            let bytes = (0..<16).map { _ in String(format: "%02X", UInt8.random(in: 0...255)) }
            lbl.text = bytes.joined(separator: " ")
        }
    }
}
