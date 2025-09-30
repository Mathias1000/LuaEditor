-- Comprehensive Test Class for Lua Editor Features
-- Tests: Autocompletion, Syntax Highlighting, Navigation, Auto-Indentation
-- Type various combinations to test the editor capabilities

-- Global variables for completion testing
local game_world = {}
local player_manager = {}
local monster_spawner = {}

-- Math and string library usage (test standard library completion)
-- Try typing: math. or string. or table. or os.
local pi_value = math.pi
local random_number = math.random(1, 100)
local formatted_string = string.format("Random: %d", random_number)
local table_size = table.insert

-- Class definition with multiple methods and properties
GameEngine = {}
GameEngine.__index = GameEngine

function GameEngine:new(name)
    local engine = setmetatable({}, self)
    engine.name = name or "DefaultEngine"
    engine.version = "1.0.0"
    engine.running = false
    engine.fps_target = 60
    engine.delta_time = 0.016
    engine.total_time = 0
    engine.frame_count = 0
    engine.systems = {}
    engine.entities = {}
    engine.resources = {}
    return engine
end

-- Test self. completion - type "self." inside these methods
function GameEngine:initialize()
    self.running = true
    self.total_time = 0
    self.frame_count = 0
    print("Engine initialized: " .. self.name)
end

function GameEngine:update(dt)
    if not self.running then
        return
    end
    
    self.delta_time = dt
    self.total_time = self.total_time + dt
    self.frame_count = self.frame_count + 1
    
    -- Update all systems
    for _, system in ipairs(self.systems) do
        if system.enabled then
            system:update(self.delta_time)
        end
    end
end

-- Test self: completion - type "self:" inside these methods  
function GameEngine:start()
    self:initialize()
    print("Starting game loop...")
    
    while self.running do
        local start_time = os.clock()
        
        self:update(self.delta_time)
        self:render()
        
        local frame_time = os.clock() - start_time
        if frame_time < 1.0 / self.fps_target then
            -- Sleep simulation
        end
        
        -- Exit condition for demo
        if self.frame_count > 1000 then
            self:shutdown()
        end
    end
end

function GameEngine:render()
    -- Render all entities
    for _, entity in ipairs(self.entities) do
        if entity.visible then
            entity:render()
        end
    end
end

function GameEngine:shutdown()
    self.running = false
    print("Engine shutdown complete")
end

-- Component System with member completion testing
Transform = {}
Transform.__index = Transform

function Transform:new(x, y, z)
    local transform = setmetatable({}, self)
    transform.position = {x = x or 0, y = y or 0, z = z or 0}
    transform.rotation = {x = 0, y = 0, z = 0}
    transform.scale = {x = 1, y = 1, z = 1}
    transform.parent = nil
    transform.children = {}
    return transform
end

-- Test member access - type "transform.position." or "transform.rotation."
function Transform:translate(dx, dy, dz)
    self.position.x = self.position.x + (dx or 0)
    self.position.y = self.position.y + (dy or 0) 
    self.position.z = self.position.z + (dz or 0)
end

function Transform:rotate(rx, ry, rz)
    self.rotation.x = self.rotation.x + (rx or 0)
    self.rotation.y = self.rotation.y + (ry or 0)
    self.rotation.z = self.rotation.z + (rz or 0)
end

function Transform:setScale(sx, sy, sz)
    self.scale.x = sx or 1
    self.scale.y = sy or self.scale.x
    self.scale.z = sz or self.scale.x
end

-- Player class with extensive properties for completion testing
Player = {}
Player.__index = Player

function Player:new(name, class_type)
    local player = setmetatable({}, self)
    player.name = name or "Anonymous"
    player.class = class_type or "Warrior"
    player.level = 1
    player.experience = 0
    player.health = 100
    player.max_health = 100
    player.mana = 50
    player.max_mana = 50
    player.stamina = 100
    player.max_stamina = 100
    player.strength = 10
    player.agility = 10
    player.intelligence = 10
    player.defense = 5
    player.magic_resistance = 5
    player.critical_chance = 0.05
    player.inventory = {}
    player.equipped = {}
    player.skills = {}
    player.achievements = {}
    player.transform = Transform:new(0, 0, 0)
    return player
end

-- Test extensive member completion - type "player." inside these methods
function Player:gainExperience(amount)
    self.experience = self.experience + amount
    local exp_needed = self.level * 100
    
    while self.experience >= exp_needed do
        self:levelUp()
        exp_needed = self.level * 100
    end
end

function Player:levelUp()
    self.level = self.level + 1
    self.max_health = self.max_health + 10
    self.max_mana = self.max_mana + 5
    self.health = self.max_health
    self.mana = self.max_mana
    print(self.name .. " reached level " .. self.level .. "!")
end

function Player:takeDamage(amount, damage_type)
    local final_damage = amount
    
    if damage_type == "physical" then
        final_damage = math.max(1, amount - self.defense)
    elseif damage_type == "magical" then
        final_damage = math.max(1, amount - self.magic_resistance)
    end
    
    self.health = math.max(0, self.health - final_damage)
    
    if self.health <= 0 then
        self:onDeath()
    end
end

function Player:heal(amount)
    self.health = math.min(self.max_health, self.health + amount)
end

function Player:castSpell(spell_name, target)
    if self.mana < 10 then
        print("Not enough mana!")
        return false
    end
    
    self.mana = self.mana - 10
    print(self.name .. " casts " .. spell_name)
    
    if target then
        target:takeDamage(20, "magical")
    end
    
    return true
end

function Player:onDeath()
    print(self.name .. " has fallen!")
end

-- Monster class for testing object completion
Monster = {}
Monster.__index = Monster

function Monster:new(monster_type, level)
    local monster = setmetatable({}, self)
    monster.type = monster_type or "Goblin"
    monster.level = level or 1
    monster.health = 50 + (level * 10)
    monster.max_health = monster.health
    monster.damage = 5 + (level * 2)
    monster.experience_value = level * 15
    monster.aggro_range = 5
    monster.ai_state = "idle"
    monster.target = nil
    monster.transform = Transform:new()
    return monster
end

-- Test monster. completion
function Monster:update(dt)
    if self.health <= 0 then
        return
    end
    
    -- Simple AI state machine
    if self.ai_state == "idle" then
        self:lookForTarget()
    elseif self.ai_state == "chasing" then
        self:chaseTarget(dt)
    elseif self.ai_state == "attacking" then
        self:attackTarget()
    end
end

function Monster:lookForTarget()
    -- Find nearest player logic would go here
    self.ai_state = "chasing"
end

function Monster:chaseTarget(dt)
    if self.target then
        -- Movement logic would go here
        local distance = self:distanceToTarget()
        if distance < 1.5 then
            self.ai_state = "attacking"
        end
    else
        self.ai_state = "idle"
    end
end

function Monster:attackTarget()
    if self.target then
        self.target:takeDamage(self.damage, "physical")
    end
    self.ai_state = "chasing"
end

function Monster:distanceToTarget()
    if not self.target then
        return math.huge
    end
    -- Simple distance calculation
    return math.random(1, 10) -- Placeholder
end

-- Module pattern for testing require completion
local utils = {}

function utils.clamp(value, min_val, max_val)
    return math.max(min_val, math.min(max_val, value))
end

function utils.lerp(a, b, t)
    return a + (b - a) * utils.clamp(t, 0, 1)
end

function utils.distance(x1, y1, x2, y2)
    local dx = x2 - x1
    local dy = y2 - y1
    return math.sqrt(dx * dx + dy * dy)
end

-- Test auto-indentation and end insertion
function testAutoIndentation()
    if true then
        print("Testing if-then-end")
        for i = 1, 10 do
            print("Testing for-do-end: " .. i)
        end
    end
    
    while false do
        print("Testing while-do-end")
    end
    
    repeat
        print("Testing repeat-until")
    until true
end

-- Test F12 navigation - put cursor on any function name and press F12
function functionA()
    functionB()
end

function functionB() 
    functionC()
end

function functionC()
    print("Navigation test complete")
end

-- Test import system (if you create a separate .lua file)
-- local imported_module = require("test_module")

-- Testing string patterns and completion
function testStringOperations()
    local test_string = "Hello, World!"
    
    -- Type "test_string:" or "string." to test completion
    local length = string.len(test_string)
    local upper = string.upper(test_string)
    local lower = string.lower(test_string)
    local sub = string.sub(test_string, 1, 5)
    local found = string.find(test_string, "World")
    local replaced = string.gsub(test_string, "World", "Lua")
    
    return {length, upper, lower, sub, found, replaced}
end

-- Main execution function
function main()
    print("=== Lua Editor Feature Test ===")
    
    -- Create instances to test completion
    local engine = GameEngine:new("TestEngine")
    local player = Player:new("TestPlayer", "Mage")
    local monster = Monster:new("TestGoblin", 5)
    
    -- Test method calls - type "engine:" or "player:" to see methods
    engine:initialize()
    player:gainExperience(150)
    monster:lookForTarget()
    
    -- Test member access - type "player." to see all properties
    player.health = 80
    player.mana = 30
    
    -- Test standard libraries
    local math_result = math.sin(math.pi / 2)
    local table_ops = {}
    table.insert(table_ops, "test")
    
    print("All features tested successfully!")
    return true
end

-- Call main function
main()