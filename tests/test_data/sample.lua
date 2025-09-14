-- Sample Lua file for testing
-- This file contains various Lua constructs for parser testing

-- Variables
local myNumber = 42
local myString = "Hello, World!"
local myBoolean = true
globalVar = "I'm global"

-- Functions
function greet(name)
    if name then
        print("Hello, " .. name .. "!")
    else
        print("Hello, stranger!")
    end
end

local function calculate(a, b, operation)
    if operation == "add" then
        return a + b
    elseif operation == "subtract" then
        return a - b
    elseif operation == "multiply" then
        return a * b
    elseif operation == "divide" then
        if b ~= 0 then
            return a / b
        else
            error("Division by zero!")
        end
    else
        error("Unknown operation: " .. tostring(operation))
    end
end

-- Tables
local person = {
    name = "John Doe",
    age = 30,
    city = "New York"
}

local numbers = {1, 2, 3, 4, 5}

-- Table with methods
local calculator = {}

function calculator:add(a, b)
    return a + b
end

function calculator:multiply(a, b)
    return a * b
end

-- Loops
for i = 1, 10 do
    print("Number: " .. i)
end

for key, value in pairs(person) do
    print(key .. ": " .. tostring(value))
end

-- String operations
local text = "Lua is awesome"
local upperText = string.upper(text)
local length = string.len(text)
local substring = string.sub(text, 1, 3)

-- Math operations
local result = math.sqrt(16)
local randomNum = math.random(1, 100)
local maxValue = math.max(10, 20, 30)

-- Conditional statements
if myNumber > 40 then
    print("Number is greater than 40")
elseif myNumber == 40 then
    print("Number is exactly 40")
else
    print("Number is less than 40")
end

-- While loop
local counter = 0
while counter < 5 do
    print("Counter: " .. counter)
    counter = counter + 1
end

-- Repeat-until loop
local x = 0
repeat
    x = x + 1
    print("X is now: " .. x)
until x >= 3

-- Error handling
local success, result = pcall(function()
    return calculate(10, 2, "divide")
end)

if success then
    print("Result: " .. result)
else
    print("Error: " .. result)
end
