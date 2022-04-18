//
//  CopyInfoToClipboardView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/28/22.
//

import SwiftUI

struct CopyInfoToClipboardView: View {
    private static let fadeDuration = 2.0

    @State private var clipOpacity = 1.0
    @State private var checkOpacity = 0.0
    @State private var copyError = false

    let cart: Cart

    var body: some View {
        Button(action: copyToClipboard) {
            ZStack {
                Image(systemName: "arrow.up.doc.on.clipboard")
                    .opacity(clipOpacity)
                Image(systemName: "checkmark.diamond")
                    .opacity(checkOpacity)
                    .foregroundStyle(.green)
            }
        }
        .buttonStyle(.plain)
        .help("Copy to Clipboard")
        .alert("Clipboard Copy Failure", isPresented: $copyError,
               presenting: cart.currentError) { _ in
            // NOTE: default action
        } message: { err in
            Text(err.message)
        }
    }

    private func copyToClipboard() {
        guard let text = cart.getInfoText() else {
            copyError = true
            return
        }
        NSPasteboard.general.clearContents()
        NSPasteboard.general.setString(text, forType: .string)
        checkOpacity = 1.0
        clipOpacity = 0.0
        let halfDuration = CopyInfoToClipboardView.fadeDuration / 2.0
        withAnimation(.easeOut(duration: halfDuration).delay(halfDuration)) {
            checkOpacity = 0.0
        }
        DispatchQueue.main.asyncAfter(
            deadline: .now() + CopyInfoToClipboardView.fadeDuration) {
            clipOpacity = 1.0
        }
    }
}

struct CopyToClipboardView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CopyInfoToClipboardView(cart: cart)
    }
}
