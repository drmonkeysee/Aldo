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
    private let otherBlock = SKSpriteNode(color: .blue,
                                          size: .init(width: 256, height: 240))
    private var waitCount = 0

    override func didMove(to view: SKView) {
        size = view.bounds.size
        scaleMode = .resizeFill
        backgroundColor = .black
        addChild(testAnimation.createRenderNode())
        addChild(traits.createRenderNode())
    }

    override func didChangeSize(_ oldSize: CGSize) {
        testAnimation.rootNode.position = .init(x: size.width / 2,
                                                y: size.height / 2)
        traits.rootNode.position = .init(x: (100 / 2) + 5,
                                         y: size.height - (200 / 2) - 5)
    }

    override func update(_ currentTime: TimeInterval) {
        clock.frameStart(currentTime)
        testAnimation.update(clock)
        traits.update(clock)
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
    var rootNode: SKNode { get }

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

    var rootNode: SKNode { get { bounds }}

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

fileprivate struct EmulatorTraits: RenderElement {
    private let box = SKSpriteNode(color: .darkGray,
                                   size: .init(width: 100, height: 200))
    private let fps = makeLabelNode()
    private let frames = makeLabelNode()

    var rootNode: SKNode { get { box } }

    func createRenderNode() -> SKNode {
        let leftMargin = -(box.size.width / 2) + 5
        var topOffset = (box.size.height / 2) - 10
        fps.position = .init(x: leftMargin, y: topOffset)
        topOffset -= 15
        frames.position = .init(x: leftMargin, y: topOffset)
        box.addChild(fps)
        box.addChild(frames)
        return box
    }

    func update(_ clock: FrameClock) {
        fps.text = "FPS: 60"
        frames.text = "Frames: \(clock.info.frames)"
    }
}
