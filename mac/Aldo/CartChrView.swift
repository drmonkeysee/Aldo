//
//  CartChrView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/6/22.
//

import SwiftUI

struct CartChrView: View {
    var body: some View {
        GroupBox {
            ZStack {
                Color.cyan
                    .cornerRadius(5)
                    .frame(width: 256, height: 128)
                Text("No CHR Data")
            }
        }
    }
}

struct CartChrView_Previews: PreviewProvider {
    static var previews: some View {
        CartChrView()
    }
}
