-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'
local inspect = require 'ext.inspect'

local UI = require 'editor.imgui'
local ICONS = require 'editor.icons'
local Time = require('Time')
local Scene = require('Scene')
local EFLAGS = ENTITY_FLAGS

local AssetTree = {
    name = ICONS.FOLDER_TREE..' Asset Tree',
    isVisible = ffi.new('bool[1]', true),
    scanDir = 'assets',
    dirTree = {}
}

function AssetTree:buildDirTreeRecursive(path, parent)
    path = path or self.scanDir
    parent = parent or self.dirTree
    for entry in lfs.dir(path) do
        if entry ~= '.' and entry ~= '..' then
            local fullPath = path..'/'..entry
            local attr = lfs.attributes(fullPath)
            if attr and (attr.mode == 'directory' or attr.mode == 'file') then
                local node = {
                    name = entry,
                    is_file = attr.mode == 'file',
                    children = {}
                }
                table.insert(parent, node)
                if not node.is_file then
                    self:buildDirTreeRecursive(fullPath, node.children)
                end
            end
        end
    end
end

AssetTree:buildDirTreeRecursive() -- Build directory tree once on start

function AssetTree:render()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    local DIR_COLOR = 0xffaaaaaa
    local FILE_COLOR = 0xffcccccc
    UI.PushStyleColor(ffi.C.ImGuiCol_Text, DIR_COLOR)
    if UI.Begin(self.name, self.isVisible) then
        for i=1, #self.dirTree do
            local node = self.dirTree[i]
            if UI.TreeNode(node.name) then
                for j=1, #node.children do
                    local child = node.children[j]
                    if child.is_file then
                        UI.PushStyleColor(ffi.C.ImGuiCol_Text, FILE_COLOR)
                        UI.Indent()
                        UI.TextUnformatted(child.name)
                        UI.Unindent()
                        UI.PopStyleColor()
                    else
                        if UI.TreeNode(child.name) then
                            UI.TreePop()
                        end
                    end
                end
                UI.TreePop()
            end
        end
    end
    UI.PopStyleColor()
    UI.End()
end

return AssetTree

