//
//  main.swift
//  Aldo-Cli
//
//  Created by Brandon Stansbury on 1/8/21.
//

import Foundation

// NOTE: cli target used for IDE editor support

print("Aldo CLI Started...")

let result = cli_run(CommandLine.argc, CommandLine.unsafeArgv)

exit(result)
