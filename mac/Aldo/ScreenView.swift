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

fileprivate class EmulatorScene: SKScene {
    override func didMove(to view: SKView) {
        self.size = view.bounds.size
        self.backgroundColor = .cyan
    }

    override func update(_ currentTime: TimeInterval) {
        self.removeAllChildren()
        let block = SKSpriteNode(color: .yellow,
                                 size: .init(width: 100, height: 100))
        self.addChild(block)
    }
}

struct ScreenView_Previews: PreviewProvider {
    static var previews: some View {
        ScreenView()
    }
}
