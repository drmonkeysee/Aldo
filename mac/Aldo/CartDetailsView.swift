//
//  CartDetailsView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/5/22.
//

import SwiftUI

struct CartDetailsView: View {
    static let testRom = "8000: 58        CLI\n8001: 4C 0B 80  JMP $800B\n8004: E6 90     INC $90\n8006: 40        RTI\n8007: 18        CLC\n8008: 69 05     ADC #$05\n800A: 60        RTS\n800B: A9 0A     LDA #$0A\n800D: 20 07 80  JSR $8007\n8010: 48        PHA\n8011: 68        PLA\n8012: 08        PHP\n8013: A9 00     LDA #$00\n8015: 8D FD 01  STA $01FD\n8018: 28        PLP\n8019: 08        PHP\n801A: 85 FF     STA $FF\n801C: A9 80     LDA #$80\n801E: 85 00     STA $00\n8020: A2 05     LDX #$05\n8022: A9 88     LDA #$88\n8024: 6A        ROR\n8025: CA        DEX\n8026: D0 FC     BNE -4\n8028: C6 24     DEC $24\n802A: E6 26     INC $26\n802C: A9 0A     LDA #$0A\n802E: 85 20     STA $20\n8030: A5 25     LDA $25\n8032: A2 05     LDX #$05\n8034: 95 30     STA $30,X\n8036: A0 03     LDY #$03\n8038: B9 01 03  LDA $0301,Y\n803B: B9 FF 03  LDA $03FF,Y\n803E: B1 16     LDA ($16),Y\n8040: A0 D6     LDY #$D6\n8042: B1 16     LDA ($16),Y\n8044: 99 02 03  STA $0302,Y\n8047: 99 FF 02  STA $02FF,Y\n804A: E6 80     INC $80\n804C: 4C 4A 80  JMP $804A"

    @Binding var fileUrl: URL?

    var body: some View {
        VStack {
            Text(fileUrl?.lastPathComponent ?? "No file selected")
                .help(fileUrl?.lastPathComponent ?? "")
                .font(.title)
                .truncationMode(.middle)
                .frame(maxWidth: .infinity)
            HStack(alignment: .top) {
                VStack {
                    GroupBox {
                        VStack {
                            HStack(alignment: .top) {
                                VStack {
                                    Text("JMP").font(.title)
                                    Text("(6C)")
                                }
                                Spacer()
                                VStack {
                                    Text("$(8134)").font(.title)
                                    Text("(34) (81)")
                                }
                            }
                            Divider()
                            HStack {
                                Text("Jump")
                                Spacer()
                                Text("Absolute Indirect")
                            }
                            .font(.footnote)
                            Divider()
                            HStack {
                                Text("Flags")
                                Spacer()
                                ZStack {
                                    Circle().frame(width: 20)
                                    Text("N").font(.footnote)
                                        .foregroundColor(.black)
                                }
                                ZStack {
                                    Circle().frame(width: 20)
                                    Text("V").font(.footnote)
                                        .foregroundColor(.black)
                                }
                                ZStack {
                                    Circle().frame(width: 20)
                                    Text("B").font(.footnote)
                                        .foregroundColor(.black)
                                }
                                ZStack {
                                    Circle().frame(width: 20)
                                    Text("D").font(.footnote)
                                        .foregroundColor(.black)
                                }
                                ZStack {
                                    Circle().frame(width: 20)
                                    Text("I").font(.footnote)
                                        .foregroundColor(.black)
                                }
                                ZStack {
                                    Circle().frame(width: 20)
                                    Text("Z").font(.footnote)
                                        .foregroundColor(.black)
                                }
                                ZStack {
                                    Circle().frame(width: 20)
                                    Text("C").font(.footnote)
                                        .foregroundColor(.black)
                                }
                            }
                            .scaledToFit()
                        }
                        .padding(EdgeInsets(top: 0, leading: 5, bottom: 5,
                                            trailing: 5))
                        .frame(minWidth: 240, maxWidth: .infinity)
                    }
                    GroupBox {
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
                            VStack (alignment: .leading) {
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
                        }
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 5)
                    }
                }
                GroupBox {
                    ScrollView {
                        Text(CartDetailsView.testRom)
                            .padding(5)
                            .frame(minWidth: 250, maxWidth: .infinity,
                                   alignment: .leading)
                            .multilineTextAlignment(.leading)
                            .textSelection(.enabled)
                    }
                }
                GroupBox {
                    ZStack {
                        RoundedRectangle(cornerRadius: 5)
                            .foregroundColor(.cyan)
                            .frame(width: 256, height: 128)
                        Text("No CHR Data")
                    }
                }
            }
        }
    }
}

struct CartDetails_Previews: PreviewProvider {
    static var previews: some View {
        CartDetailsView(fileUrl: .constant(
            URL(string: "/fake/folder/test-file.rom")))
    }
}