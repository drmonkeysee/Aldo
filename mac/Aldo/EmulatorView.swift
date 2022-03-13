//
//  EmulatorView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/9/22.
//

import SwiftUI

struct EmulatorView: View {
    static let nesResolution = (w: 256, h: 240)
    static let nesScale = 2

    @EnvironmentObject var appState: AppControl

    var body: some View {
        HStack {
            ZStack {
                Color(white: 0.15)
                    .cornerRadius(5)
                    .frame(width: Double(EmulatorView.nesResolution.w
                                         * EmulatorView.nesScale),
                           height: Double(EmulatorView.nesResolution.h
                                          * EmulatorView.nesScale))
                Color.cyan
                    .frame(width: Double(EmulatorView.nesResolution.w),
                           height: Double(EmulatorView.nesResolution.h))
                Text("Emu Screen")
            }
            TabView {
                Text("Other Stuff")
                    .border(.blue)
                    .tabItem {
                        Text("Visualization")
                    }
                Text("Debugger stuff")
                    .border(.gray)
                    .tabItem {
                        Text("Debug")
                    }
                Text("Trace Log")
                    .tabItem {
                        Text("Trace")
                    }
                CartInfoView(cart: appState.cart)
                    .border(.red)
                    .tabItem {
                        Text("Cart Info")
                    }
            }
        }
    }
}

struct EmulatorView_Previews: PreviewProvider {
    static var previews: some View {
        EmulatorView()
    }
}
