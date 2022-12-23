//
//  CartInspectorView.swift
//  Aldo-Studio
//
//  Created by Brandon Stansbury on 3/31/22.
//

import SwiftUI

struct CartInspectorView: View {
    var body: some View {
        HStack(alignment: .top, spacing: 0) {
            CartPrgView()
            Divider()
            CartChrView()
        }
    }
}
