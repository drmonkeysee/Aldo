//
//  CartInspectorView.swift
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 12/18/22.
//

import SwiftUI

func createCartInspectorView() -> NSViewController {
    NSHostingController(rootView: CartInspectorView()
                                    .frame(width: 800, height: 600))
}

struct CartInspectorView: View {
    var body: some View {
        Text("CART INSPECTOR")
    }
}

struct CartInspectorView_Previews: PreviewProvider {
    static var previews: some View {
        CartInspectorView()
    }
}
