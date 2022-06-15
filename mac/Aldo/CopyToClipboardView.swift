//
//  CopyToClipboardView.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 3/28/22.
//

import SwiftUI

struct CopyToClipboardView: View {
    @ObservedObject var command: ClipboardCopy

    var body: some View {
        ZStack {
            Button {
                Task(priority: .userInitiated) { await command.execute() }
            } label: {
                Image(systemName: "arrow.up.doc.on.clipboard")
                    .opacity(command.actionIconOpacity)
            }
            .buttonStyle(.plain)
            .help("Copy to Clipboard")
            .disabled(command.inProgress)
            Image(systemName: "checkmark.diamond")
                .opacity(command.successIconOpacity)
                .font(.body.bold())
                .foregroundColor(.green)
                .onReceive(command.$successIconOpacity,
                           perform: animateSuccess)
        }
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
            let halfDuration = ClipboardCopy.transitionDuration / 2.0
            withAnimation(
                .easeOut(duration: halfDuration).delay(halfDuration)) {
                    command.successIconOpacity = 0.0
            }
        }
    }
}

struct CopyToClipboardView_Previews: PreviewProvider {
    static var previews: some View {
        CopyToClipboardView(command: ClipboardCopy(fromStream: {
            .success(.init())
        }))
    }
}
