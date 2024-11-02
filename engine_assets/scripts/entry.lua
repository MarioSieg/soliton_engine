-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

if engine_cfg['system']['enable_editor'] then
    print('-- Editor is enabled --')
    require 'engine_assets/scripts/editor/editor'
else
    require 'core/standalone_entry'
end

