//
//  ScreenView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/9/22.
//

import SwiftUI

struct ScreenView: View {
    private static let nesResolution = (w: 256, h: 240)
    private static let nesScale = 2

    var body: some View {
        ZStack {
            Color(white: 0.15)
                .cornerRadius(5)
                .frame(width: .init(Self.nesResolution.w * Self.nesScale),
                       height: .init(Self.nesResolution.h * Self.nesScale))
            Color.cyan
                .frame(width: .init(Self.nesResolution.w),
                       height: .init(Self.nesResolution.h))
            Text("Emu Screen")
        }
    }
}

struct ScreenView_Previews: PreviewProvider {
    static var previews: some View {
        ScreenView()
    }
}
