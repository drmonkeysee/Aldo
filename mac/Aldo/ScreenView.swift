//
//  ScreenView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/9/22.
//

import SpriteKit
import SwiftUI

struct ScreenView: View {
    private static let nesResolution = (w: 256.0, h: 240.0)
    private static let nesScale = 2

    let scene: EmulatorScene

    var body: some View {
        ZStack {
            SpriteView(scene: scene,
                       preferredFramesPerSecond: FrameClock.targetFps,
                       debugOptions: [.showsFPS,
                                      .showsNodeCount,
                                      .showsDrawCount,
                                      .showsQuadCount])
            Text("Emu Screen")
        }
    }
}

struct ScreenView_Previews: PreviewProvider {
    static var previews: some View {
        ScreenView(scene: EmulatorScene())
    }
}
