TERMINAL_COMMANDS = {}

TERMINAL_COMMANDS['panic'] = {
    description = 'Panic!',
    arguments = {},
    execute = function(args)
        App.panic(args[2] or 'Panic!')
    end
}
