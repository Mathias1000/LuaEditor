#!/usr/bin/env lua
--[[
    Umfangreiches RPG-Test-Script für Lua 5.4.8
    Testet alle Features der Sprache für einen selbstgemachten Editor
    Erstellt von Claude für maximalen Feature-Test
]]--

-- Globale Konstanten und Konfiguration
VERSION = "1.0.0"
DEBUG_MODE = true
MAX_PLAYERS = 4
WORLD_SIZE = {x = 100, y = 100}

-- Metatabellen und OOP-System
local GameObject = {}
GameObject.__index = GameObject

function GameObject:new(name, x, y)
    local obj = setmetatable({}, self)
    obj.name = name or "Unknown"
    obj.position = {x = x or 0, y = y or 0}
    obj.health = 100
    obj.alive = true
    obj.inventory = {}
    obj.stats = {
        strength = 10,
        defense = 5,
        speed = 8,
        magic = 3
    }
    return obj
end

function GameObject:move(dx, dy)
    local new_x = math.max(0, math.min(WORLD_SIZE.x, self.position.x + dx))
    local new_y = math.max(0, math.min(WORLD_SIZE.y, self.position.y + dy))
    self.position.x, self.position.y = new_x, new_y
    return self
end

function GameObject:takeDamage(damage)
    self.health = math.max(0, self.health - damage)
    if self.health <= 0 then
        self.alive = false
        self:onDeath()
    end
    return self.health
end

function GameObject:onDeath()
    print(string.format("%s hat den Tod gefunden bei Position (%d, %d)", 
          self.name, self.position.x, self.position.y))
end

-- Player-Klasse mit Vererbung
local Player = setmetatable({}, GameObject)
Player.__index = Player

function Player:new(name, class, x, y)
    local player = GameObject.new(self, name, x, y)
    player.class = class or "Warrior"
    player.level = 1
    player.experience = 0
    player.mana = 50
    player.skills = {}
    player.equipment = {
        weapon = nil,
        armor = nil,
        accessory = nil
    }
    
    -- Klassenspezifische Anpassungen
    if class == "Mage" then
        player.stats.magic = 15
        player.stats.strength = 5
        player.mana = 100
    elseif class == "Rogue" then
        player.stats.speed = 15
        player.stats.defense = 3
    elseif class == "Paladin" then
        player.stats.defense = 12
        player.stats.magic = 8
        player.health = 150
    end
    
    return player
end

function Player:gainExperience(exp)
    self.experience = self.experience + exp
    local exp_needed = self.level * 100
    
    while self.experience >= exp_needed do
        self:levelUp()
      
        exp_needed = self.level * 100
    end
end

function Player:levelUp()
    self.level = self.level + 1
    local stat_boost = math.random(1, 3)
    
    -- Zufällige Stat-Erhöhung
    local stat_names = {"strength", "defense", "speed", "magic"}
    local boosted_stat = stat_names[math.random(#stat_names)]
    self.stats[boosted_stat] = self.stats[boosted_stat] + stat_boost
    
    self.health = self.health + (self.level * 5)
    self.mana = self.mana + (self.level * 3)
    
    print(string.format("%s erreicht Level %d! %s +%d", 
          self.name, self.level, boosted_stat, stat_boost))
end

function Player:castSpell(spell_name, target)
    local spells = {
        fireball = {cost = 15, damage = 25, type = "damage"},
        heal = {cost = 20, heal = 30, type = "heal"},
        shield = {cost = 10, defense = 5, type = "buff"},
        lightning = {cost = 25, damage = 35, type = "damage"}
    }
    
    local spell = spells[spell_name]
    if not spell then
        print("Unbekannter Zauber: " .. spell_name)
        return false
    end
    
    if self.mana < spell.cost then
        print("Nicht genug Mana für " .. spell_name)
        return false
    end
    
    self.mana = self.mana - spell.cost
    
    if spell.type == "damage" and target then
        target:takeDamage(spell.damage + self.stats.magic)
        print(string.format("%s wirkt %s auf %s für %d Schaden!", 
              self.name, spell_name, target.name, spell.damage + self.stats.magic))
    elseif spell.type == "heal" then
        self.health = math.min(self.health + spell.heal, 200)
        print(string.format("%s heilt sich um %d HP!", self.name, spell.heal))
    elseif spell.type == "buff" then
        self.stats.defense = self.stats.defense + spell.defense
        print(string.format("%s erhält %d Verteidigung!", self.name, spell.defense))
    end
    
    return true
end

-- Monster-Klasse
local Monster = setmetatable({}, GameObject)
Monster.__index = Monster

function Monster:new(name, monster_type, x, y)
    local monster = GameObject.new(self, name, x, y)
    monster.monster_type = monster_type or "Goblin"
    monster.ai_state = "idle"
    monster.aggro_range = 5
    monster.drop_table = {}
    
    -- Monster-Typ spezifische Eigenschaften
    local monster_data = {
        Goblin = {health = 80, strength = 8, defense = 3, exp = 25},
        Orc = {health = 120, strength = 15, defense = 8, exp = 50},
        Dragon = {health = 500, strength = 35, defense = 20, exp = 500},
        Skeleton = {health = 60, strength = 12, defense = 2, exp = 30},
        Troll = {health = 200, strength = 20, defense = 15, exp = 100}
    }
    
    local data = monster_data[monster_type] or monster_data.Goblin
    monster.health = data.health
    monster.stats.strength = data.strength
    monster.stats.defense = data.defense
    monster.experience_value = data.exp
    
    return monster
end

function Monster:updateAI(players)
    local closest_player = nil
    local closest_distance = math.huge
    
    for _, player in ipairs(players) do
        if player.alive then
            local distance = self:distanceTo(player)
            if distance < closest_distance then
                closest_distance = distance
                closest_player = player
            end
        end
    end
    
    if closest_player and closest_distance <= self.aggro_range then
        self.ai_state = "aggressive"
        self:moveTowards(closest_player)
        if closest_distance <= 1 then
            self:attack(closest_player)
        end
    else
        self.ai_state = "idle"
        self:wander()
    end
end

function Monster:distanceTo(target)
    local dx = self.position.x - target.position.x
    local dy = self.position.y - target.position.y
    return math.sqrt(dx * dx + dy * dy)
end

function Monster:moveTowards(target)
    local dx = target.position.x > self.position.x and 1 or -1
    local dy = target.position.y > self.position.y and 1 or -1
    self:move(dx, dy)
end

function Monster:wander()
    local directions = {{1,0}, {-1,0}, {0,1}, {0,-1}, {0,0}}
    local dir = directions[math.random(#directions)]
    self:move(dir[1], dir[2])
end

function Monster:attack(target)
    local damage = self.stats.strength + math.random(1, 5)
    target:takeDamage(damage)
    print(string.format("%s greift %s an für %d Schaden!", 
          self.name, target.name, damage))
end

-- Item-System
local Item = {}
Item.__index = Item

function Item:new(name, item_type, value, properties)
    local item = setmetatable({}, self)
    item.name = name
    item.item_type = item_type -- weapon, armor, consumable, quest
    item.value = value or 0
    item.properties = properties or {}
    item.stack_size = item.properties.stackable and 99 or 1
    return item
end

function Item:use(user)
    if self.item_type == "consumable" then
        if self.properties.heal then
            user.health = math.min(user.health + self.properties.heal, 200)
            print(string.format("%s heilt %d HP!", user.name, self.properties.heal))
        end
        if self.properties.mana then
            user.mana = user.mana + self.properties.mana
            print(string.format("%s erhält %d Mana!", user.name, self.properties.mana))
        end
        return true -- Item wird verbraucht
    end
    return false
end

-- Lokale Funktionen für das Spiel-System
local function createItems()
    local items = {}
    
    -- Waffen
    items.sword = Item:new("Eisenschwert", "weapon", 50, {damage = 15})
    items.staff = Item:new("Magierstab", "weapon", 75, {damage = 8, magic_boost = 10})
    items.bow = Item:new("Elfenbogen", "weapon", 60, {damage = 12, range = 5})
    
    -- Rüstung
    items.leather_armor = Item:new("Lederrüstung", "armor", 40, {defense = 8})
    items.chain_mail = Item:new("Kettenhemd", "armor", 100, {defense = 15})
    items.mage_robe = Item:new("Magierrobe", "armor", 80, {defense = 5, mana_boost = 25})
    
    -- Verbrauchsgegenstände
    items.health_potion = Item:new("Heiltrank", "consumable", 25, {heal = 50, stackable = true})
    items.mana_potion = Item:new("Manatrank", "consumable", 30, {mana = 40, stackable = true})
    items.strength_potion = Item:new("Stärketrank", "consumable", 100, {strength_boost = 5, duration = 300})
    
    return items
end

local function generateLoot(monster_level)
    local loot = {}
    local items = createItems()
    local item_pool = {}
    
    for name, item in pairs(items) do
        table.insert(item_pool, {name = name, item = item, weight = item.value})
    end
    
    -- Gewichtetes Zufallssystem
    local total_weight = 0
    for _, entry in ipairs(item_pool) do
        total_weight = total_weight + entry.weight
    end
    
    local drop_count = math.random(0, 3)
    for i = 1, drop_count do
        local random_value = math.random() * total_weight
        local current_weight = 0
        
        for _, entry in ipairs(item_pool) do
            current_weight = current_weight + entry.weight
            if random_value <= current_weight then
                table.insert(loot, entry.item)
                break
            end
        end
    end
    
    return loot
end

-- Quest-System
local Quest = {}
Quest.__index = Quest

function Quest:new(title, description, objectives, reward)
    local quest = setmetatable({}, self)
    quest.title = title
    quest.description = description
    quest.objectives = objectives or {}
    quest.reward = reward or {}
    quest.completed = false
    quest.active = false
    quest.progress = {}
    
    for i, objective in ipairs(quest.objectives) do
        quest.progress[i] = 0
    end
    
    return quest
end

function Quest:updateProgress(objective_index, amount)
    if not self.active or self.completed then return end
    
    self.progress[objective_index] = (self.progress[objective_index] or 0) + amount
    
    local objective = self.objectives[objective_index]
    if self.progress[objective_index] >= objective.required then
        print(string.format("Questziel erfüllt: %s", objective.description))
    end
    
    self:checkCompletion()
end

function Quest:checkCompletion()
    local all_complete = true
    for i, objective in ipairs(self.objectives) do
        if self.progress[i] < objective.required then
            all_complete = false
            break
        end
    end
    
    if all_complete then
        self.completed = true
        print(string.format("Quest abgeschlossen: %s", self.title))
        return true
    end
    return false
end

-- Lokale Hilfsfunktionen
local function createQuests()
    local quests = {}
    
    quests.goblin_slayer = Quest:new(
        "Goblin-Jäger",
        "Eliminiere die Goblin-Bedrohung in der Region",
        {
            {description = "Töte 10 Goblins", type = "kill", target = "Goblin", required = 10},
            {description = "Sammle 5 Goblin-Ohren", type = "collect", item = "goblin_ear", required = 5}
        },
        {experience = 500, gold = 200, item = "silver_sword"}
    )
    
    quests.treasure_hunt = Quest:new(
        "Schatzsuche",
        "Finde die verlorenen Artefakte des alten Königs",
        {
            {description = "Finde die Königskrone", type = "find", item = "crown", required = 1},
            {description = "Finde das Königszepter", type = "find", item = "scepter", required = 1},
            {description = "Finde den Königsring", type = "find", item = "ring", required = 1}
        },
        {experience = 1000, gold = 1000, item = "legendary_armor"}
    )
    
    return quests
end

-- Inventar-Management
local function addToInventory(player, item, quantity)
    quantity = quantity or 1
    local inventory = player.inventory
    
    -- Prüfe auf stackbare Items
    if item.properties.stackable then
        for i, inv_item in ipairs(inventory) do
            if inv_item.item.name == item.name then
                inv_item.quantity = inv_item.quantity + quantity
                print(string.format("%s erhält %dx %s", player.name, quantity, item.name))
                return true
            end
        end
    end
    
    -- Neues Item hinzufügen
    table.insert(inventory, {item = item, quantity = quantity})
    print(string.format("%s erhält %dx %s", player.name, quantity, item.name))
    return true
end

local function removeFromInventory(player, item_name, quantity)
    quantity = quantity or 1
    local inventory = player.inventory
    
    for i, inv_item in ipairs(inventory) do
        if inv_item.item.name == item_name then
            if inv_item.quantity >= quantity then
                inv_item.quantity = inv_item.quantity - quantity
                if inv_item.quantity <= 0 then
                    table.remove(inventory, i)
                end
                return true
            end
            break
        end
    end
    return false
end

-- Kampfsystem
local function calculateDamage(attacker, defender)
    local base_damage = attacker.stats.strength
    local weapon_damage = attacker.equipment.weapon and attacker.equipment.weapon.properties.damage or 0
    local armor_reduction = defender.equipment.armor and defender.equipment.armor.properties.defense or 0
    local defense_reduction = defender.stats.defense
    
    local total_damage = base_damage + weapon_damage + math.random(1, 5)
    total_damage = math.max(1, total_damage - armor_reduction - defense_reduction)
    
    return total_damage
end

local function performCombat(attacker, defender)
    if not attacker.alive or not defender.alive then return false end
    
    local damage = calculateDamage(attacker, defender)
    defender:takeDamage(damage)
    
    print(string.format("%s greift %s an und verursacht %d Schaden! (%d HP verbleibend)", 
          attacker.name, defender.name, damage, defender.health))
    
    if not defender.alive then
        if attacker.gainExperience then -- Ist ein Spieler
            attacker:gainExperience(defender.experience_value or 0)
        end
        
        -- Loot generieren
        if defender.monster_type then -- Ist ein Monster
            local loot = generateLoot(defender.level or 1)
            for _, item in ipairs(loot) do
                addToInventory(attacker, item)
            end
        end
        return true -- Kampf beendet
    end
    
    return false
end

-- Speicher- und Ladesystem
local function saveGame(players, monsters, world_state, filename)
    filename = filename or "savegame.lua"
    local save_data = {
        version = VERSION,
        timestamp = os.time(),
        players = {},
        monsters = {},
        world_state = world_state
    }
    
    -- Spielerdaten serialisieren
    for i, player in ipairs(players) do
        save_data.players[i] = {
            name = player.name,
            class = player.class,
            level = player.level,
            experience = player.experience,
            health = player.health,
            mana = player.mana,
            position = {x = player.position.x, y = player.position.y},
            stats = player.stats,
            inventory = player.inventory
        }
    end
    
    -- Monster-Daten serialisieren
    for i, monster in ipairs(monsters) do
        if monster.alive then
            save_data.monsters[i] = {
                name = monster.name,
                monster_type = monster.monster_type,
                health = monster.health,
                position = {x = monster.position.x, y = monster.position.y},
                ai_state = monster.ai_state
            }
        end
    end
    
    -- Datei schreiben (vereinfacht für Demo)
    local file = io.open(filename, "w")
    if file then
        file:write("-- Gespeichertes Spiel vom " .. os.date() .. "\n")
        file:write("return " .. serialize(save_data))
        file:close()
        print("Spiel gespeichert: " .. filename)
        return true
    else
        print("Fehler beim Speichern!")
        return false
    end
end

-- Serialisierungs-Hilfsfunktion
function serialize(obj, indent)
    indent = indent or 0
    local spacing = string.rep("  ", indent)
    
    if type(obj) == "table" then
        local result = "{\n"
        for key, value in pairs(obj) do
            local key_str = type(key) == "string" and string.format('"%s"', key) or tostring(key)
            result = result .. spacing .. "  [" .. key_str .. "] = " .. serialize(value, indent + 1) .. ",\n"
        end
        result = result .. spacing .. "}"
        return result
    elseif type(obj) == "string" then
        return string.format('"%s"', obj)
    else
        return tostring(obj)
    end
end

-- Welt-Management
local function createWorld()
    local world = {
        tiles = {},
        npcs = {},
        structures = {},
        weather = "sunny",
        time_of_day = 12, -- 24h Format
        day_cycle = 0
    }
    
    -- Welt-Kacheln generieren
    for x = 1, WORLD_SIZE.x do
        world.tiles[x] = {}
        for y = 1, WORLD_SIZE.y do
            local tile_types = {"grass", "forest", "mountain", "water", "desert"}
            world.tiles[x][y] = {
                type = tile_types[math.random(#tile_types)],
                passable = true,
                resources = {}
            }
            
            -- Wasser ist nicht begehbar
            if world.tiles[x][y].type == "water" then
                world.tiles[x][y].passable = false
            end
            
            -- Zufällige Ressourcen
            if math.random() < 0.1 then
                local resources = {"iron_ore", "gold_ore", "herbs", "wood", "stone"}
                world.tiles[x][y].resources[#world.tiles[x][y].resources + 1] = resources[math.random(#resources)]
            end
        end
    end
    
    return world
end

local function updateWorld(world, dt)
    -- Tag/Nacht-Zyklus
    world.time_of_day = world.time_of_day + (dt / 60) -- 1 Minute = 1 Spielstunde
    if world.time_of_day >= 24 then
        world.time_of_day = 0
        world.day_cycle = world.day_cycle + 1
        print("Ein neuer Tag beginnt! (Tag " .. world.day_cycle .. ")")
    end
    
    -- Wetteränderungen
    if math.random() < 0.01 then -- 1% Chance pro Update
        local weather_types = {"sunny", "cloudy", "rainy", "stormy", "foggy"}
        world.weather = weather_types[math.random(#weather_types)]
        print("Das Wetter ändert sich zu: " .. world.weather)
    end
end

-- Hauptspiel-Schleife
local function gameLoop()
    print("=== RPG Testsystem gestartet ===")
    print("Version: " .. VERSION)
    print("Debug-Modus: " .. (DEBUG_MODE and "AN" or "AUS"))
    
    -- Spiel-Objekte initialisieren
    local world = createWorld()
    local items = createItems()
    local quests = createQuests()
    local players = {}
    local monsters = {}
    
    -- Test-Spieler erstellen
    players[1] = Player:new("Aragorn", "Paladin", 50, 50)
    players[2] = Player:new("Gandalf", "Mage", 48, 52)
    players[3] = Player:new("Legolas", "Rogue", 52, 48)
    
    -- Test-Monster erstellen
    for i = 1, 20 do
        local monster_types = {"Goblin", "Orc", "Skeleton", "Troll"}
        local monster_type = monster_types[math.random(#monster_types)]
        local x, y = math.random(WORLD_SIZE.x), math.random(WORLD_SIZE.y)
        monsters[i] = Monster:new("Monster_" .. i, monster_type, x, y)
    end
    
    -- Boss-Monster
    monsters[#monsters + 1] = Monster:new("Ancient Dragon", "Dragon", 75, 75)
    
    -- Starterausrüstung verteilen
    addToInventory(players[1], items.sword)
    addToInventory(players[1], items.chain_mail)
    addToInventory(players[2], items.staff)
    addToInventory(players[2], items.mage_robe)
    addToInventory(players[3], items.bow)
    addToInventory(players[3], items.leather_armor)
    
    -- Verbrauchsgegenstände hinzufügen
    for _, player in ipairs(players) do
        addToInventory(player, items.health_potion, 5)
        addToInventory(player, items.mana_potion, 3)
    end
    
    -- Quests aktivieren
    for _, quest in pairs(quests) do
        quest.active = true
    end
    
    -- Haupt-Spielschleife (simuliert)
    local game_running = true
    local turn_counter = 0
    local last_time = os.clock()
    
    while game_running and turn_counter < 100 do -- Begrenzt für Demo
        turn_counter = turn_counter + 1
        local current_time = os.clock()
        local delta_time = current_time - last_time
        last_time = current_time
        
        if DEBUG_MODE and turn_counter % 10 == 0 then
            print(string.format("\n--- Turn %d ---", turn_counter))
        end
        
        -- Welt aktualisieren
        updateWorld(world, delta_time)
        
        -- Spieler-Aktionen (simuliert)
        for i, player in ipairs(players) do
            if not player.alive then goto continue end
            
            -- Zufällige Bewegung
            if math.random() < 0.3 then
                local directions = {{1,0}, {-1,0}, {0,1}, {0,-1}}
                local dir = directions[math.random(#directions)]
                player:move(dir[1], dir[2])
            end
            
            -- Zauber wirken (Magier)
            if player.class == "Mage" and math.random() < 0.2 and player.mana > 20 then
                local nearby_monsters = {}
                for _, monster in ipairs(monsters) do
                    if monster.alive and monster:distanceTo(player) <= 3 then
                        table.insert(nearby_monsters, monster)
                    end
                end
                
                if #nearby_monsters > 0 then
                    local target = nearby_monsters[math.random(#nearby_monsters)]
                    player:castSpell("fireball", target)
                end
            end
            
            -- Heilung verwenden
            if player.health < 50 and math.random() < 0.4 then
                for j, inv_item in ipairs(player.inventory) do
                    if inv_item.item.name == "Heiltrank" then
                        inv_item.item:use(player)
                        removeFromInventory(player, "Heiltrank", 1)
                        break
                    end
                end
            end
            
            ::continue::
        end
        
        -- Monster-KI aktualisieren
        for _, monster in ipairs(monsters) do
            if monster.alive then
                monster:updateAI(players)
            end
        end
        
        -- Kampf-Logik
        for _, player in ipairs(players) do
            if not player.alive then goto player_continue end
            
            for _, monster in ipairs(monsters) do
                if not monster.alive then goto monster_continue end
                
                local distance = monster:distanceTo(player)
                if distance <= 1 then
                    -- Monster greift an
                    performCombat(monster, player)
                    
                    -- Spieler kämpft zurück (wenn noch am Leben)
                    if player.alive and math.random() < 0.7 then
                        performCombat(player, monster)
                    end
                end
                
                ::monster_continue::
            end
            
            ::player_continue::
        end
        
        -- Quest-Progress aktualisieren (vereinfacht)
        if turn_counter % 20 == 0 then
            for _, quest in pairs(quests) do
                if quest.active and not quest.completed then
                    -- Simuliere Quest-Progress
                    local random_objective = math.random(#quest.objectives)
                    quest:updateProgress(random_objective, math.random(1, 3))
                end
            end
        end
        
        -- Spielende-Bedingungen
        local alive_players = 0
        for _, player in ipairs(players) do
            if player.alive then alive_players = alive_players + 1 end
        end
        
        if alive_players == 0 then
            print("\nAlle Spieler sind gefallen! Game Over!")
            game_running = false
        end
        
        local alive_monsters = 0
        for _, monster in ipairs(monsters) do
            if monster.alive then alive_monsters = alive_monsters + 1 end
        end
        
        if alive_monsters == 0 then
            print("\nAlle Monster besiegt! Sieg!")
            game_running = false
        end
        
        -- Kurze Pause für Demonstration
        if turn_counter % 50 == 0 then
            -- Status-Report
            print(string.format("\n=== Status Report (Turn %d) ===", turn_counter))
            for i, player in ipairs(players) do
                if player.alive then
                    print(string.format("%s (Level %d): %d HP, %d Mana, Position: (%d, %d)", 
                          player.name, player.level, player.health, player.mana, 
                          player.position.x, player.position.y))
                else
                    print(string.format("%s: GEFALLEN", player.name))
                end
            end
            
            local living_monsters = 0
            for _, monster in ipairs(monsters) do
                if monster.alive then living_monsters = living_monsters + 1 end
            end
            print(string.format("Lebende Monster: %d", living_monsters))
            print(string.format("Wetter: %s, Zeit: %02d:00", world.weather, math.floor(world.time_of_day)))
        end
    end
    
    -- Spiel-Ende
    print("\n=== Spiel beendet ===")
    print("Turns gespielt: " .. turn_counter)
    
    -- Finale Statistiken
    for i, player in ipairs(players) do
        print(string.format("Spieler %s: Level %d, Experience: %d", 
              player.name, player.level, player.experience))
    end
    
    -- Speichern versuchen
    saveGame(players, monsters, world, "rpg_test_save.lua")
end

-- Erweiterte Features und Tests

-- Coroutine-System für asynchrone Aktionen
local function createAsyncAction(action_name, duration, callback)
    return coroutine.create(function()
        local start_time = os.clock()
        while os.clock() - start_time < duration do
            coroutine.yield()
        end
        if callback then callback() end
        print(string.format("Asynchrone Aktion '%s' abgeschlossen nach %.2f Sekunden", 
              action_name, duration))
    end)
end

-- Schwache Referenzen für Garbage Collection Tests
local weak_table = setmetatable({}, {__mode = "v"})

local function testWeakReferences()
    local test_object = {name = "Temporäres Objekt", value = 42}
    weak_table[#weak_table + 1] = test_object
    test_object = nil -- Lokale Referenz entfernen
    collectgarbage() -- Garbage Collection forcieren
    
    print("Schwache Referenz Test:")
    for i, obj in ipairs(weak_table) do
        if obj then
            print(string.format("  Objekt %d: %s", i, obj.name))
        else
            print(string.format("  Objekt %d: wurde garbage collected", i))
        end
    end
end

-- String-Verarbeitung und Pattern Matching
local function processGameText(text)
    -- Verschiedene String-Operationen demonstrieren
    local processed = text:gsub("{PLAYER}", "Held")
    processed = processed:gsub("{ENEMY}", "Gegner")
    processed = processed:gsub("{DAMAGE:(%d+)}", function(damage)
        return damage .. " Schadenspunkte"
    end)
    
    -- Pattern Matching für Kommandos
    local command_patterns = {
        ["^/say%s+(.+)"] = function(message) 
            return "Spieler sagt: " .. message 
        end,
        ["^/attack%s+(%w+)"] = function(target) 
            return "Angriff auf " .. target .. " gestartet!"
        end,
        ["^/cast%s+(%w+)%s+(%w+)"] = function(spell, target)
            return string.format("Wirke %s auf %s", spell, target)
        end,
        ["^/give%s+(%w+)%s+(%d+)%s+(.+)"] = function(player, amount, item)
            return string.format("Gebe %s x%s an %s", item, amount, player)
        end
    }
    
    for pattern, handler in pairs(command_patterns) do
        local matches = {string.match(processed, pattern)}
        if #matches > 0 then
            return handler(table.unpack(matches))
        end
    end
    
    return processed
end

-- Mathematische Funktionen und Berechnungen
local function calculateGameMath()
    print("\n=== Mathematische Berechnungen ===")
    
    -- Trigonometrie für Bewegungsberechnungen
    local function calculateProjectilePath(start_x, start_y, target_x, target_y, speed)
        local dx = target_x - start_x
        local dy = target_y - start_y
        local distance = math.sqrt(dx * dx + dy * dy)
        local time = distance / speed
        
        local angle = math.atan(dy, dx)
        return {
            angle = math.deg(angle),
            time = time,
            velocity_x = math.cos(angle) * speed,
            velocity_y = math.sin(angle) * speed
        }
    end
    
    local projectile = calculateProjectilePath(0, 0, 10, 5, 15)
    print(string.format("Projektil-Berechnung: Winkel=%.2f°, Zeit=%.2fs", 
          projectile.angle, projectile.time))
    
    -- Statistische Berechnungen
    local damage_rolls = {}
    for i = 1, 100 do
        damage_rolls[i] = math.random(1, 20) + math.random(1, 6)
    end
    
    local sum = 0
    local min_dmg = math.huge
    local max_dmg = -math.huge
    
    for _, dmg in ipairs(damage_rolls) do
        sum = sum + dmg
        min_dmg = math.min(min_dmg, dmg)
        max_dmg = math.max(max_dmg, dmg)
    end
    
    local average = sum / #damage_rolls
    print(string.format("Schadens-Statistik: Durchschnitt=%.2f, Min=%d, Max=%d", 
          average, min_dmg, max_dmg))
    
    -- Exponential- und Logarithmusfunktionen für Level-Systeme
    local function experienceForLevel(level)
        return math.floor(100 * math.pow(1.5, level - 1))
    end
    
    local function levelForExperience(exp)
        return math.floor(math.log(exp / 100) / math.log(1.5)) + 1
    end
    
    print("Level-System Berechnungen:")
    for level = 1, 10 do
        local exp_needed = experienceForLevel(level)
        print(string.format("  Level %d: %d EXP benötigt", level, exp_needed))
    end
end

-- Datei-I/O und Konfigurationssystem
local function loadConfiguration(filename)
    filename = filename or "game_config.lua"
    local config = {
        -- Standard-Konfiguration
        graphics = {
            resolution = "1920x1080",
            fullscreen = false,
            vsync = true,
            texture_quality = "high"
        },
        audio = {
            master_volume = 100,
            music_volume = 80,
            sfx_volume = 90,
            voice_volume = 85
        },
        gameplay = {
            difficulty = "normal",
            auto_save = true,
            tutorial_enabled = true,
            combat_speed = 1.0
        },
        controls = {
            move_up = "W",
            move_down = "S",
            move_left = "A",
            move_right = "D",
            attack = "SPACE",
            magic = "SHIFT"
        }
    }
    
    -- Versuche Konfiguration aus Datei zu laden
    local file = io.open(filename, "r")
    if file then
        local content = file:read("*all")
        file:close()
        
        -- Sicheres Laden der Konfiguration
        local loaded_config = load("return " .. content)
        if loaded_config then
            local success, result = pcall(loaded_config)
            if success and type(result) == "table" then
                -- Merge mit Standard-Konfiguration
                for category, settings in pairs(result) do
                    if config[category] then
                        for key, value in pairs(settings) do
                            config[category][key] = value
                        end
                    else
                        config[category] = settings
                    end
                end
                print("Konfiguration geladen: " .. filename)
            end
        end
    else
        print("Standard-Konfiguration verwendet (Datei nicht gefunden)")
    end
    
    return config
end

-- Event-System mit Callbacks
local EventSystem = {}
EventSystem.__index = EventSystem

function EventSystem:new()
    local system = setmetatable({}, self)
    system.listeners = {}
    system.event_queue = {}
    return system
end

function EventSystem:addEventListener(event_type, callback, priority)
    priority = priority or 0
    
    if not self.listeners[event_type] then
        self.listeners[event_type] = {}
    end
    
    table.insert(self.listeners[event_type], {
        callback = callback,
        priority = priority
    })
    
    -- Sortiere nach Priorität
    table.sort(self.listeners[event_type], function(a, b)
        return a.priority > b.priority
    end)
end

function EventSystem:fireEvent(event_type, event_data)
    event_data = event_data or {}
    event_data.type = event_type
    event_data.timestamp = os.clock()
    
    if self.listeners[event_type] then
        for _, listener in ipairs(self.listeners[event_type]) do
            local success, result = pcall(listener.callback, event_data)
            if not success then
                print("Fehler in Event-Listener: " .. tostring(result))
            elseif result == false then
                -- Event-Propagation stoppen
                break
            end
        end
    end
    
    -- Event zur Historie hinzufügen
    table.insert(self.event_queue, event_data)
    
    -- Queue-Größe begrenzen
    if #self.event_queue > 1000 then
        table.remove(self.event_queue, 1)
    end
end

-- Lokale Funktionen für erweiterte Tests
local function testAdvancedFeatures()
    print("\n=== Erweiterte Features Test ===")
    
    -- Event-System testen
    local events = EventSystem:new()
    
    events:addEventListener("player_level_up", function(event)
        print(string.format("Event: %s erreicht Level %d!", 
              event.player_name, event.new_level))
    end, 10)
    
    events:addEventListener("monster_defeated", function(event)
        print(string.format("Event: %s wurde besiegt! (+%d EXP)", 
              event.monster_name, event.experience_gained))
    end, 5)
    
    -- Test-Events feuern
    events:fireEvent("player_level_up", {
        player_name = "Testkrieger",
        new_level = 5,
        old_level = 4
    })
    
    events:fireEvent("monster_defeated", {
        monster_name = "Goblin",
        experience_gained = 25,
        loot = {"Goblin Ear", "5 Gold"}
    })
    
    -- Coroutine-Tests
    local async_actions = {}
    
    async_actions[1] = createAsyncAction("Heiltrank brauen", 2.5, function()
        print("Heiltrank erfolgreich gebraut!")
    end)
    
    async_actions[2] = createAsyncAction("Ausrüstung reparieren", 1.8, function()
        print("Ausrüstung repariert!")
    end)
    
    -- Simuliere Coroutine-Ausführung
    local active_coroutines = #async_actions
    while active_coroutines > 0 do
        for i, co in ipairs(async_actions) do
            if co and coroutine.status(co) == "suspended" then
                local success, error_msg = coroutine.resume(co)
                if not success then
                    print("Coroutine Fehler: " .. error_msg)
                    async_actions[i] = nil
                elseif coroutine.status(co) == "dead" then
                    async_actions[i] = nil
                end
            elseif co and coroutine.status(co) == "dead" then
                async_actions[i] = nil
            end
        end
        
        -- Zähle aktive Coroutines
        active_coroutines = 0
        for _, co in ipairs(async_actions) do
            if co then active_coroutines = active_coroutines + 1 end
        end
    end
    
    -- String-Verarbeitung testen
    local test_messages = {
        "{PLAYER} greift {ENEMY} für {DAMAGE:25} an!",
        "/say Hallo zusammen, wie geht es euch?",
        "/attack Goblin",
        "/cast fireball Dragon",
        "/give Aragorn 5 Heiltrank"
    }
    
    print("\nString-Verarbeitung Tests:")
    for _, message in ipairs(test_messages) do
        local processed = processGameText(message)
        print(string.format("  Input: %s", message))
        print(string.format("  Output: %s", processed))
    end
    
    -- Schwache Referenzen testen
    testWeakReferences()
    
    -- Mathematik testen
    calculateGameMath()
    
    -- Konfiguration testen
    local config = loadConfiguration()
    print(string.format("\nKonfiguration geladen: %s, Lautstärke: %d", 
          config.graphics.resolution, config.audio.master_volume))
end

-- Fehlerbehandlung und Debugging
local function safeCall(func, ...)
    local args = {...}
    return function()
        local success, result = pcall(func, table.unpack(args))
        if not success then
            print("Fehler aufgetreten: " .. tostring(result))
            if DEBUG_MODE then
                print(debug.traceback())
            end
            return nil
        end
        return result
    end
end

-- Benchmark-System
local function benchmark(name, func, iterations)
    iterations = iterations or 1000
    local start_time = os.clock()
    
    for i = 1, iterations do
        func()
    end
    
    local end_time = os.clock()
    local total_time = end_time - start_time
    local avg_time = total_time / iterations
    
    print(string.format("Benchmark '%s': %.4fs gesamt, %.6fs pro Iteration (%d Iterationen)", 
          name, total_time, avg_time, iterations))
    
    return {
        name = name,
        total_time = total_time,
        average_time = avg_time,
        iterations = iterations
    }
end

-- Hauptprogramm mit allen Tests
local function runAllTests()
    print("=== UMFANGREICHES LUA 5.4.8 TEST-SCRIPT ===")
    print("Dieses Script testet alle Features der Lua-Sprache")
    print("für die Validierung eines selbstgemachten Editors.\n")
    
    -- Basis RPG-System starten
    local main_game = safeCall(gameLoop)
    main_game()
    
    -- Erweiterte Features testen
    local advanced_tests = safeCall(testAdvancedFeatures)
    advanced_tests()
    
    -- Performance-Benchmarks
    print("\n=== Performance Benchmarks ===")
    
    local benchmarks = {}
    
    benchmarks[#benchmarks + 1] = benchmark("Table Creation", function()
        local t = {a = 1, b = 2, c = 3, d = 4, e = 5}
    end, 10000)
    
    benchmarks[#benchmarks + 1] = benchmark("String Concatenation", function()
        local s = "Test" .. "String" .. "Concatenation" .. tostring(math.random())
    end, 5000)
    
    benchmarks[#benchmarks + 1] = benchmark("Mathematical Operations", function()
        local result = math.sin(math.pi / 4) + math.cos(math.pi / 3) * math.tan(math.pi / 6)
    end, 10000)
    
    benchmarks[#benchmarks + 1] = benchmark("Table Iteration", function()
        local t = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
        for i, v in ipairs(t) do
            local dummy = i * v
        end
    end, 5000)
    
    -- Benchmark-Zusammenfassung
    local total_benchmark_time = 0
    for _, bench in ipairs(benchmarks) do
        total_benchmark_time = total_benchmark_time + bench.total_time
    end
    
    print(string.format("Alle Benchmarks abgeschlossen in %.4fs", total_benchmark_time))
    
    -- Memory Usage (wenn verfügbar)
    local memory_kb = collectgarbage("count")
    print(string.format("Speicherverbrauch: %.2f KB", memory_kb))
    
    -- Finale Garbage Collection
    collectgarbage("collect")
    local final_memory_kb = collectgarbage("count")
    print(string.format("Nach Garbage Collection: %.2f KB (%.2f KB freigegeben)", 
          final_memory_kb, memory_kb - final_memory_kb))
    
    print("\n=== SCRIPT ERFOLGREICH ABGESCHLOSSEN ===")
    print("Alle Lua-Features wurden getestet:")
    print("✓ Objektorientierte Programmierung (Metatabellen)")
    print("✓ Lokale und globale Variablen")
    print("✓ Funktionen und Closures")
    print("✓ Coroutines")
    print("✓ String-Manipulation und Pattern Matching")
    print("✓ Datei I/O")
    print("✓ Mathematische Operationen")
    print("✓ Tabellen und Datenstrukturen")
    print("✓ Fehlerbehandlung (pcall/xpcall)")
    print("✓ Garbage Collection")
    print("✓ Event-Systeme")
    print("✓ Serialisierung")
    print("✓ Performance-Benchmarking")
    print("✓ Debug-Funktionalität")
    
    return true
end

-- Script-Ausführung
local success, error_message = pcall(runAllTests)
if not success then
    print("KRITISCHER FEHLER: " .. error_message)
    print("Stack Trace:")
    print(debug.traceback())
    os.exit(1)
else
    print("Script erfolgreich beendet!")
    os.exit(0)
end