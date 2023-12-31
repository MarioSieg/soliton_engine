-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

TERMINAL_COMMANDS = {}

TERMINAL_COMMANDS['panic'] = {
    description = 'Panic!',
    arguments = {},
    execute = function(args)
        app.panic(args[2] or 'Panic!')
    end
}
