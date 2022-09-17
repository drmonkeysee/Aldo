//
//  ScreenView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/9/22.
//

import SpriteKit
import SwiftUI

struct ScreenView: View {
    private static let nesResolution = (w: 256, h: 240)
    private static let nesScale = 2

    @State private var scene = EmulatorScene()

    var body: some View {
        ZStack {
            Color(white: 0.15)
                .cornerRadius(5)
                .frame(width: .init(Self.nesResolution.w * Self.nesScale),
                       height: .init(Self.nesResolution.h * Self.nesScale))
            SpriteView(scene: scene,
                       options: [.ignoresSiblingOrder,
                                 .shouldCullNonVisibleNodes],
                       debugOptions: [.showsFPS,
                                      .showsNodeCount,
                                      .showsDrawCount,
                                      .showsQuadCount])
                .frame(width: .init(Self.nesResolution.w),
                       height: .init(Self.nesResolution.h))
            Text("Emu Screen")
        }
    }
}

fileprivate final class EmulatorScene: SKScene {
    private static let blockHalfDim = 25.0
    private static let blockDim = blockHalfDim * 2
    private static let waitFrames = 120

    private let block = SKSpriteNode(color: .yellow,
                                     size: .init(
                                        width: EmulatorScene.blockDim,
                                        height: EmulatorScene.blockDim))
    private var velocity: CGPoint = .init(x: 1, y: 1)
    private var frameCount = 0

    override func didMove(to view: SKView) {
        self.size = view.bounds.size
        self.backgroundColor = .cyan
        self.block.position = .init(x: self.size.width / 2,
                                    y: self.size.height / 2)
        self.addChild(self.block)
    }

    override func update(_ currentTime: TimeInterval) {
        if self.frameCount < Self.waitFrames {
            self.frameCount += 1
            return
        }
        self.block.position.x += self.velocity.x
        if self.block.position.x - Self.blockHalfDim < 0
            || self.block.position.x + Self.blockHalfDim >= self.size.width {
            self.velocity.x = -self.velocity.x
        }
        self.block.position.y += self.velocity.y
        if self.block.position.y - Self.blockHalfDim < 0
            || self.block.position.y + Self.blockHalfDim >= self.size.height {
            self.velocity.y = -self.velocity.y
        }
    }
}

struct ScreenView_Previews: PreviewProvider {
    static var previews: some View {
        ScreenView()
    }
}
