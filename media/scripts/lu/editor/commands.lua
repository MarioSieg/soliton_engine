local m = {}

m['panic'] = {
    description = 'Panic!',
    arguments = {},
    execute = function(args)
        app.panic(args[2] or 'Panic!')
    end
}

return m
