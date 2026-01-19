//
//  main.swift
//  Aldo-Dev
//
//  Created by Brandon Stansbury on 1/8/21.
//

import Foundation

// dev target used for IDE editor support

print("Aldo Dev started...")
let result = cli_run(CommandLine.argc, CommandLine.unsafeArgv)
exit(result)
