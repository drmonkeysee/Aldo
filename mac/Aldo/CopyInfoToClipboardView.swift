//
//  CopyInfoToClipboardView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/28/22.
//

import SwiftUI

struct CopyInfoToClipboardView: View {
    private static let fadeDuration = 2.0

    let cart: Cart
    @StateObject private var command = CopyCartInfo()

    var body: some View {
        Button {
            command.execute(cart: cart)
        } label: {
            ZStack {
                Image(systemName: "arrow.up.doc.on.clipboard")
                    .opacity(command.copyIconOpacity)
                Image(systemName: "checkmark.diamond")
                    .opacity(command.successIconOpacity)
                    .foregroundStyle(.green)
                    .onReceive(command.$successIconOpacity,
                               perform: animateSuccess)
            }
        }
        .buttonStyle(.plain)
        .help("Copy to Clipboard")
        .alert("Clipboard Copy Failure", isPresented: $command.failed,
               presenting: command.currentError) { _ in
            // NOTE: default action
        } message: { err in
            Text(err.message)
        }
    }

    private func animateSuccess(val: Double) {
        guard val == 1.0 else { return }

        DispatchQueue.main.async {
            let halfDuration = Self.fadeDuration / 2.0
            withAnimation(
                .easeOut(duration: halfDuration).delay(halfDuration)) {
                    command.successIconOpacity = 0.0
            }
        }
        DispatchQueue.main.asyncAfter(deadline: .now() + Self.fadeDuration) {
            command.copyIconOpacity = 1.0
        }
    }
}

struct CopyToClipboardView_Previews: PreviewProvider {
    private static let cart = Cart()

    static var previews: some View {
        CopyInfoToClipboardView(cart: cart)
    }
}
