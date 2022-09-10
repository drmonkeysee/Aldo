//
//  CartView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/31/22.
//

import SwiftUI

struct CartView: View {
    var body: some View {
        VStack(spacing: 5) {
            CartPrgView()
            Divider()
            CartChrView()
                .frame(height: 325)
        }
    }
}

struct CartView_Previews: PreviewProvider {
    static var previews: some View {
        CartView()
    }
}
