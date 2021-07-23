#!/usr/bin/env lua

-- Lua CJSON tests
--
-- Mark Pulford <mark@kyne.com.au>
--
-- Note: The output of this script is easier to read with "less -S"
print("package.path")
print(package.path)

local json = require "cjson"
local util = require "cjson.util"


function test_decode_cycle(filename)
    print("************** Before CJSON DECODE ******************",os.time())
    print(os.date("%m/%d/%Y %I:%M:%S %p"))
    local obj1 = json.decode(util.file_load(filename))
    print(obj1.UserName)
    print(obj1.Password)
    print("************** After CJSON DECODE ******************",os.time())
    print(os.date("%m/%d/%Y %I:%M:%S %p"))
    --local obj2 = json.decode(json.encode(obj1))
    --return util.compare_values(obj1, obj2)
end

test_decode_cycle("test240K.json")

-- vi:ai et sw=4 ts=4:
