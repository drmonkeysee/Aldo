//
//  EmulatorView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/9/22.
//

import SwiftUI

struct EmulatorView: View {
    static let nesResolution = (w: 256, h: 240)
    static let nesScale = 1

    var body: some View {
        HStack {
            ZStack {
                Color.cyan
                    .frame(width: Double(EmulatorView.nesResolution.w
                                         * EmulatorView.nesScale),
                           height: Double(EmulatorView.nesResolution.h
                                          * EmulatorView.nesScale))
                Text("Emu Screen")
            }
            Text("Extra stuff")
                .frame(minWidth: 200, maxWidth: .infinity)
        }
    }
}

struct EmulatorView_Previews: PreviewProvider {
    static var previews: some View {
        EmulatorView()
    }
}
