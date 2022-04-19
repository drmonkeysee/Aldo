//
//  CopyInfoToClipboardView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/28/22.
//

import SwiftUI

struct CopyInfoToClipboardView: View {
    private static let fadeDuration = 2.0

    @StateObject private var copyInfoCmd = CopyInfoCommand()

    let cart: Cart

    var body: some View {
        Button {
            copyInfoCmd.execute(cart: cart)
        } label: {
            ZStack {
                Image(systemName: "arrow.up.doc.on.clipboard")
                    .opacity(copyInfoCmd.copyIconOpacity)
                Image(systemName: "checkmark.diamond")
                    .opacity(copyInfoCmd.successIconOpacity)
                    .foregroundStyle(.green)
                    .onReceive(copyInfoCmd.$successIconOpacity,
                               perform: animateSuccess)
            }
        }
        .buttonStyle(.plain)
        .help("Copy to Clipboard")
        .alert("Clipboard Copy Failure", isPresented: $copyInfoCmd.copyError,
               presenting: cart.currentError) { _ in
            // NOTE: default action
        } message: { err in
            Text(err.message)
        }
    }

    private func animateSuccess(val: Double) {
        guard val == 1.0 else { return }

        let halfDuration = CopyInfoToClipboardView.fadeDuration / 2.0
        withAnimation(.easeOut(duration: halfDuration).delay(halfDuration)) {
            copyInfoCmd.successIconOpacity = 0.0
        }
        DispatchQueue.main.asyncAfter(
            deadline: .now() + CopyInfoToClipboardView.fadeDuration) {
                copyInfoCmd.copyIconOpacity = 1.0
        }
    }
}

struct CopyToClipboardView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CopyInfoToClipboardView(cart: cart)
    }
}
