//
//  CartInfoView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/6/22.
//

import SwiftUI

struct CartInfoView: View {
    var body: some View {
        HStack {
            VStack(alignment: .trailing) {
                Group {
                    Text("Format:")
                }
                Group {
                    Text("Mapper:")
                }
                Group {
                    Text("PRG ROM:")
                    Text("WRAM:")
                    Text("CHR ROM:")
                    Text("CHR RAM:")
                    Text("NT-Mirroring:")
                    Text("Mapper-Ctrl:")
                }
                Group {
                    Text("Trainer:")
                    Text("Bus Conflicts:")
                }
            }
            VStack(alignment: .leading) {
                Group {
                    Text("iNES")
                }
                Group {
                    Text("000 (<Board Names>)")
                }
                Group {
                    Text("2 x 16KB")
                    Text("no")
                    Text("1 x 8KB")
                    Text("no")
                    Text("Vertical")
                    Text("no")
                }
                Group {
                    Text("no")
                    Text("no")
                }
            }
            .padding(.vertical, 5)
        }
    }
}

struct CartFormatView_Previews: PreviewProvider {
    static var previews: some View {
        CartInfoView()
    }
}
