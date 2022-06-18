//
//  CommandViews.swift
//  Aldo-App
//
//  Created by Brandon Stansbury on 6/18/22.
//

import SwiftUI

struct CopyToClipboardView: TimedCommandView {
    @ObservedObject var command: ClipboardCopy

    init(fromStream: @escaping () async -> CStreamResult) {
        command = .init(fromStream: fromStream)
    }

    var body: some View {
        ZStack {
            Button {
                Task(priority: .userInitiated) { await command.copy() }
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
}

struct ChrExportView: TimedCommandView {
    @ObservedObject var command: ChrExport

    init(_ cart: Cart) {
        command = .init(cart)
    }

    var body: some View {
        HStack {
            Button(action: pickFolder) {
                ZStack {
                    Label("Export", systemImage: "square.and.arrow.up.circle")
                        .opacity(command.actionIconOpacity)
                    Label("Done!", systemImage: "checkmark.circle")
                        .opacity(command.successIconOpacity)
                        .foregroundColor(.green)
                        .onReceive(command.$successIconOpacity,
                                   perform: animateSuccess)
                }
            }
            .disabled(command.inProgress)
            Picker("Scale", selection: $command.scale) {
                ForEach(command.scales, id: \.self) { i in
                    Text("\(i)x")
                }
            }
            .frame(width: 120)
            Button(action: openTargetFolder) {
                Image(systemName: "folder")
            }
            .disabled(!command.done)
            .help("Open export folder")
        }
        .alert("CHR Export Failure", isPresented: $command.failed,
               presenting: command.currentError) { _ in
            // NOTE: default action
        } message: { err in
            Text(err.message)
        }
    }

    private func pickFolder() {
        let panel = NSOpenPanel()
        panel.message = "Choose export folder"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        if panel.runModal() == .OK {
            Task(priority: .userInitiated) {
                await command.export(to: panel.url)
            }
        }
    }

    private func openTargetFolder() {
        guard let folder = command.selectedFolder,
              folder.hasDirectoryPath else {
            aldoLog.debug("Missing/invalid export target folder")
            return
        }
        NSWorkspace.shared.selectFile(nil,
                                      inFileViewerRootedAtPath: folder.path)
    }
}

protocol TimedCommandView: View {
    associatedtype Command: TimedCommand

    var command: Command { get }
}

extension TimedCommandView {
    fileprivate func animateSuccess(val: Double) {
        guard val == 1.0 else { return }

        DispatchQueue.main.async {
            let halfDuration = Command.transitionDuration / 2.0
            withAnimation(
                .easeOut(duration: halfDuration).delay(halfDuration)) {
                    command.successIconOpacity = 0.0
            }
        }
    }
}

struct CopyToClipboardView_Previews: PreviewProvider {
    static var previews: some View {
        CopyToClipboardView(fromStream: {.success(.init())})
    }
}

struct ChrExportView_Previews: PreviewProvider {
    static var previews: some View {
        ChrExportView(Cart())
    }
}
