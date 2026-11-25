if not game:IsLoaded() then
    pcall(function() game.Loaded:Wait() end)
end

if _G.__TheForge_Cleanup then
    pcall(_G.__TheForge_Cleanup)
end

local ScriptRunning = true
local Connections = {}
local OrigHooks = {}

-- Isolates game ModuleScript requires in a disposable child thread so the main UI thread never loses Plugin capability
local function runIsolated(fn)
    local done = false
    task.spawn(function()
        pcall(fn)
        done = true
    end)
    local t0 = os.clock()
    while not done and (os.clock() - t0) < 4.0 do
        task.wait()
    end
end

-- Runs game controller/UI methods on a disposable identity-2 child thread so Showcase/Equipments
-- never inherits RobloxScript identity (8) and fails require(Shared.Data.Runes)
local function runAsGameThread(fn, timeout)
    local done = false
    task.spawn(function()
        local setId = (typeof(setthreadidentity) == "function" and setthreadidentity)
            or (typeof(setidentity) == "function" and setidentity)
            or (syn and typeof(syn.set_thread_identity) == "function" and syn.set_thread_identity)
        if setId then
            pcall(setId, 2)
        end
        pcall(fn)
        done = true
    end)
    local t0 = os.clock()
    while not done and (os.clock() - t0) < (timeout or 4.0) do
        task.wait()
    end
end

local ModuleCache = {}
local function safeRequire(mod)
    if not mod then return nil end
    if ModuleCache[mod] ~= nil then return ModuleCache[mod] end
    local res = nil
    local ok = false
    runAsGameThread(function()
        ok, res = pcall(require, mod)
    end, 4.0)
    if ok and res ~= nil then
        ModuleCache[mod] = res
        return res
    end
    local okDirect, directRes = pcall(require, mod)
    if okDirect and directRes ~= nil then
        ModuleCache[mod] = directRes
        return directRes
    end
    return nil
end


local ControllerCache = {}
local function SafeGetController(name, knitPkg)
    if type(name) ~= "string" or name == "" then return nil end
    if ControllerCache[name] then return ControllerCache[name] end

    local ReplicatedStorage = game:GetService("ReplicatedStorage")
    local k = knitPkg
    if not k then
        local shared = ReplicatedStorage:FindFirstChild("Shared")
        local pkgs = shared and shared:FindFirstChild("Packages")
        local knitMod = pkgs and pkgs:FindFirstChild("Knit")
        if knitMod then k = safeRequire(knitMod) end
    end

    if k and type(k.GetController) == "function" then
        local ok, ctrl = pcall(k.GetController, name)
        if ok and type(ctrl) == "table" then
            ControllerCache[name] = ctrl
            return ctrl
        end
    end

    -- Try game environment's require() in case exploit require() loaded an unstarted KnitClient instance
    if typeof(getrenv) == "function" then
        local renv = getrenv()
        local gReq = renv and renv.require
        if type(gReq) == "function" then
            local shared = ReplicatedStorage:FindFirstChild("Shared")
            local pkgs = shared and shared:FindFirstChild("Packages")
            local knitMod = pkgs and pkgs:FindFirstChild("Knit")
            if knitMod then
                local okK, renvKnit = pcall(gReq, knitMod)
                if okK and type(renvKnit) == "table" and type(renvKnit.GetController) == "function" then
                    local okC, ctrl = pcall(renvKnit.GetController, name)
                    if okC and type(ctrl) == "table" then
                        ControllerCache[name] = ctrl
                        return ctrl
                    end
                end
            end
        end
    end

    -- If Knit is still starting up on join, wait briefly for OnStart()
    if k and type(k.OnStart) == "function" then
        local ready = false
        task.spawn(function()
            local okS, prom = pcall(k.OnStart)
            if okS and type(prom) == "table" and type(prom.await) == "function" then
                pcall(function() prom:await() end)
            end
            ready = true
        end)
        local t0 = os.clock()
        while not ready and (os.clock() - t0) < 1.5 do
            task.wait(0.05)
        end
        if type(k.GetController) == "function" then
            local okC, ctrl = pcall(k.GetController, name)
            if okC and type(ctrl) == "table" then
                ControllerCache[name] = ctrl
                return ctrl
            end
        end
    end

    -- Scan live GC for the already-started game controller table
    if typeof(getgc) == "function" then
        for _, obj in ipairs(getgc(true)) do
            if type(obj) == "table" and rawget(obj, "Name") == name then
                if type(rawget(obj, "KnitStart")) == "function"
                    or type(rawget(obj, "KnitInit")) == "function"
                    or rawget(obj, "Modules") ~= nil
                    or rawget(obj, "Replica") ~= nil
                    or rawget(obj, "replica") ~= nil
                    or rawget(obj, "ForgeActive") ~= nil then
                    ControllerCache[name] = obj
                    return obj
                end
            end
        end
    end

    local ctrls = ReplicatedStorage:FindFirstChild("Controllers")
    local mod = ctrls and ctrls:FindFirstChild(name)
    if mod then
        local ctrl = safeRequire(mod)
        if type(ctrl) == "table" then
            ControllerCache[name] = ctrl
            return ctrl
        end
    end
    return nil
end


local function Notify(title, desc, dur)
    local uiLib = (typeof(getgenv) == "function" and getgenv().Library) or _G.Library or (shared and shared.Library)
    if uiLib and type(uiLib.Notify) == "function" then
        pcall(function()
            uiLib:Notify({
                Title = tostring(title or "The Forge"),
                Desc = tostring(desc or ""),
                Duration = dur or 3
            })
        end)
    end
end

local function trackConn(conn)
    table.insert(Connections, conn)
    return conn
end

local function safeHook(tbl, name, key, hookFn)
    if tbl and typeof(tbl[name]) == "function" and typeof(hookfunction) == "function" then
        local old = hookfunction(tbl[name], hookFn)
        OrigHooks[key or name] = { target = tbl, method = name, orig = old or tbl[name] }
        return old
    end
    return nil
end


-- Clean previous window frames (Shadow, DropdownSelect) without destroying root ScreenGui or ReopenButton
local searchParents = {}
local uiRoot = (typeof(gethui) == "function" and select(2, pcall(gethui))) or nil
if uiRoot then
    table.insert(searchParents, uiRoot)
end

local Players = game:GetService("Players")
local LocalPlayer = Players.LocalPlayer
if not LocalPlayer then
    pcall(function() LocalPlayer = Players.PlayerAdded:Wait() end)
    LocalPlayer = LocalPlayer or Players.LocalPlayer
end

local PlayerGui = (LocalPlayer and LocalPlayer:WaitForChild("PlayerGui", 8)) or (LocalPlayer and LocalPlayer:FindFirstChildOfClass("PlayerGui"))
if PlayerGui and not table.find(searchParents, PlayerGui) then
    table.insert(searchParents, PlayerGui)
end

do
    local foundAether = false
    local function cleanInParent(parent)
        if not parent then return end
        local okChildren, children = pcall(function() return parent:GetChildren() end)
        if not okChildren or type(children) ~= "table" then return end
        for _, ch in ipairs(children) do
            if ch.Name == "TheForge_UI" then
                pcall(function() ch:Destroy() end)
            elseif ch.Name == "Aether" or ch:GetAttribute("AetherUI") then
                foundAether = true
                for _, inner in ipairs(ch:GetChildren()) do
                    -- Never destroy ReopenButton or Notification
                    if inner.Name == "Shadow" or inner.Name == "DropdownSelect" then
                        pcall(function() inner:Destroy() end)
                    end
                end
            end
        end
    end
    for _, parent in ipairs(searchParents) do
        cleanInParent(parent)
    end
    -- Only fall back to CoreGui (fully pcall-guarded) if Aether wasn't found in gethui/PlayerGui
    if not foundAether then
        pcall(function()
            cleanInParent(game:GetService("CoreGui"))
        end)
    end
end

-- ===================================================================
-- 2. CORE SERVICES & DEPENDENCIES
-- ===================================================================
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local RunService = game:GetService("RunService")
local UserInputService = game:GetService("UserInputService")
local TweenService = game:GetService("TweenService")
local HttpService = game:GetService("HttpService")

-- Environment check: Game globals reside in getrenv()._G
local GameGlobals = (typeof(getrenv) == "function" and getrenv()._G) or _G or {}

-- ===================================================================
-- 3. STATE & CONFIGURATION NAMESPACES
-- ===================================================================
local State = {
    Character = nil,
    Humanoid = nil,
    HRP = nil,
    PlayerData = nil,
    PlayerStatus = nil,
    CurrentRock = nil,
    CurrentMob = nil,
    TargetRock = nil,
    LastMinedRock = nil,
    ResumingAfterDeath = false,
    TargetMob = nil,
    LastFarmedMob = nil,
    ResumingMobAfterDeath = false,
    SellingMobLoot = false,
    IsSellingRunes = false,
    PendingRuneSell = false,
    LastRuneSellAttempt = 0,
    LastMobLootSellAttempt = 0,
    LastMineTeleport = 0,
    LastMobTeleport = 0,
    LastPromptTrigger = {},
    LastCraftCheck = 0,
    LastCraftRequest = 0,
    LastMeltPump = 0,
    LastEndScreenAccept = 0,
    LastStashSell = 0,
    LastClaimDaily = 0,
    PourMouseDown = false,
    NoclipActive = false,
    OriginalWalkSpeed = 16,
    EspObjects = {},
    PendingSellArmorGuids = {},
    LastEquipCapacity = 0,
    IsInRaid = false,
    GolemPhase = nil,
    ManualDrainOnce = false
}

local Config = {
    -- Mining
    AutoMine = false,
    StayAtSavedPos = false, -- Stay anchored at SavedMinePos and mine in-reach rocks from that spot
    SavedMinePos = {}, -- Persisted spot { x, y, z, lx, ly, lz } for StayAtSavedPos
    MineTarget = "All",
    MineTargets = { "All" }, -- Multi-select rock targets
    MineZones = { "Any" }, -- Multi-select mining zones ("Any" = all unlocked zones)
    MinePositionMode = "Sunken Under-Ledge (Horizontal Up)", -- "Sunken Under-Ledge (Horizontal Up)", "Rock Shield (Opposite Mobs)", "Adjacent Ground"
    SunkenDepth = 2.40, -- Submerged depth in studs beneath rock base (1.00 to 3.00)
    SunkenDistance = 0.85, -- Horizontal offset in studs outside rock perimeter (0.85 = optimal pickaxe reach)
    SunkenPitch = 88, -- Pitch angle in degrees (88-90 = completely horizontal 180° flat)
    StuckRockThreshold = 4.0, -- Seconds of zero damage while in stance before skipping to another rock
    StuckRockBlacklistTime = 45, -- Seconds to blacklist a stubborn rock before retrying
    AvoidPlayerOccupiedRocks = true, -- Switches to another rock if another player actually owns/claimed it
    PrioritizeStrongestRock = true, -- When multiple rock types are selected, mines strongest to weakest first
    MinTweenY = -20, -- Minimum safe Y elevation during cross-map / inter-room tweening
    GlideSpeed = 55, -- Fast, anti-tp-safe tween speed in studs/sec
    MineDelay = 0.15,
    MineDistance = 5000, -- Entire map scan radius by default
    AutoEquipPickaxe = true,
    PrioritizeCritical = true,
    AutoDefense = true, -- Spams Weapon alongside Pickaxe while mining so mobs take damage without stopping mining
    FilterRocksByOre = false, -- Crack rocks until Ore(Model) spawns, then skip any rock lacking WantedRockOres until it regenerates
    WantedRockOres = { "Darkryte", "Demonite" }, -- Multi-select wanted ores inside rocks
    FilterRocksByOreBackupEnabled = false, -- When all wanted/pending rocks are broken, fully break backup rocks to force respawns
    BackupMineTargets = { "Auto (Rocks Lacking Wanted Ores)" }, -- Multi-select backup/filler rocks to fully clear
    
    -- Minigames / Forge & Auto-Forge Profit Engine
    AutoHammer = true,
    HammerGrade = "Perfect", -- "Perfect", "Great", "Good"
    AutoMelt = true,
    AutoPour = true,
    AutoMoldBreak = true,
    AutoClaimForged = true,
    AutoForgeArmor = false, -- Combined: waits until bag is full, crafts all usable ores -> Armor, and auto-sells ONLY the newly crafted armors inside the function
    AutoFastForge = true, -- Automatically uses Fast Forge gamepass (ID 1600910863) when owned to skip Melt & Pour minigames
    AutoForgeTargetOres = 31, -- 31 ores = Heavy Armor mathematical peak ($37.89 * Mult per ore)
    AutoForgeTriggerPct = 100, -- Stash % at which AutoForgeArmor starts draining (100 = when full)
    ForgeOreBlacklist = {}, -- Multi-select blacklist: selected ores are NEVER used in forging!
    AutoEquipBestArmor = false, -- Periodically & post-craft equips highest-HP Helmet, Chestplate, Leggings & best weapon
    AutoCraftBestWeaponFirst = false, -- If true, checks Base DPS vs current weapon and crafts 1 better weapon before Armor if possible
    ProtectCustomWeapon = false, -- If true, never crafts over or replaces the selected main/custom weapon
    ProtectedWeaponChoice = "Auto (Equipped Weapon)", -- Specific weapon GUID label or Auto (Equipped Weapon)
    ProtectCustomArmor = false, -- If true, never replaces or sells selected custom/main Armor pieces
    ProtectedArmorChoices = { "Auto (Equipped Armor)" }, -- Multi-select Armor GUID labels or Auto (Equipped Armor)
    WeaponClassTarget = "Any Class", -- "Any Class", "Keep Current Class", or specific weapon class
    WeaponCompareMode = "Base DPS", -- "Base DPS" or "Base Damage"
    WeaponHuntOverride = false, -- When ON, 1-1-1-1 Target Sword Hunt overrules every Weapon forge
    WeaponHuntRunning = false, -- Continuous 1-1-1-1 hunt loop until target sword is forged or ores run out
    WeaponHuntTarget = "Any Straight Sword",
    WeaponHuntOre1 = "Auto: Best From Inventory",
    WeaponHuntOre2 = "Auto: Best From Inventory",
    WeaponHuntOre3 = "Auto: Best From Inventory",
    WeaponHuntOre4 = "Auto: Best From Inventory",
    WeaponHuntSellDuds = true,
    WeaponHuntRefundDuds = false,
    WeaponHuntReserveOres = true,
    CalcItemType = "Armor",
    CalcCraftCategory = "Armor",
    CalcArmorPiece = "All 3 Armor Pieces",
    CalcSlot1Ore = "Iron",
    CalcSlot1Count = 15,
    CalcSlot2Ore = "Tin",
    CalcSlot2Count = 16,
    CalcSlot3Ore = "None",
    CalcSlot3Count = 0,
    CalcSlot4Ore = "None",
    CalcSlot4Count = 0,
    
    -- Crafting & Economy
    AutoClaimSmelt = true,
    SmeltCheckInterval = 3.0,
    AutoCraft = false,
    CraftTarget = "None",
    CraftTargets = {}, -- Multi-select priority list of Ore Craft / Medallion recipes
    CraftBatchAmount = 1, -- Quantity per open craft slot
    AutoCraftInterruptFarm = false, -- OFF = strict Auto Mine / Combat > Auto Craft > Auto Forge; ON = craft whenever a slot opens
    AutoSellStash = false,
    SellStashInterval = 60.0,
    SellOreWhitelist = {}, -- Multi-select whitelist: ONLY selected ores can be sold!
    AutoClaimDaily = true,
    
    -- Combat / Mobs & Auto-Parry
    AutoMobFarm = false,
    ResumeLastMobAfterDeath = true, -- Return straight to the same mob you died fighting after respawning
    AutoSellMobLootWhenFull = false, -- Auto-sell unblacklisted loot to Greedy Cey when bag hits capacity
    MobLootSellBlacklist = {}, -- Multi-select blacklist of items (fragments, bones, essences, runes, ores) to NEVER sell
    MobTarget = "Closest",
    MobFarmTargets = { "All" }, -- Multi-select enemy species filter (filtered per current island)
    PrioritizeStrongestMob = true, -- When multiple mob types are selected, kills strongest to weakest first
    SplitEventVariantMobs = (game.PlaceId == 74414241680540), -- Per-world: splits Limbo/Heaven/Hell mutated enemies from normal species (default ON in Crimson Sakura)
    MobFarmPositionMode = "Above Enemy", -- "Above Enemy", "Behind Enemy", or "Under Map"
    MobFarmVerticalOffset = 5.5, -- Height in studs above enemy center in Above Enemy mode
    MobFarmUnderOffset = 6.0, -- Depth in studs below enemy center in Under Map mode
    MobFarmDistance = 4.0, -- Horizontal standoff distance in studs behind enemy
    MobFarmMaxDistance = 5000, -- Max map scan radius for Auto Farm Mobs
    AutoAttackMob = true,
    AutoBlock = false,
    AutoParry = true, -- Automatically parries incoming mob & boss attacks via ParryIndicator + animations
    AutoParryRange = 18.0, -- Max distance in studs to trigger Auto-Parry
    ParryHoldDuration = 0.35, -- Seconds to hold block during parry window before releasing
    AutoParryEquipWeapon = false, -- If true, forces weapon equip when an enemy attacks even outside combat
    AutoDodgeUnparryable = true, -- Sidesteps red unparryable attacks & Bomber charges
    EvadeBossSkills = true, -- Distances away during Asura / Boss skill moves
    EvadeBossSkills360Only = true, -- When true, only distances for 360° / counter moves (Buddha's Killer, Prophecy Vision, Phase 2 Morph, Barrage); frontal slashes are dodged via Behind Enemy
    BossSkillSafeDistance = 50.0, -- Horizontal safe retreat distance in studs while a boss skill is active

    -- Abyss & Raid Automation
    AutoRaid = false,
    RaidMode = "Abyss", -- "Abyss", "Tower Of Hell", or current island world boss
    AutoAsuraToTower = false, -- Farms Asura until AsuraTargetTickets is met, then runs Tower Of Hell
    AsuraTargetTickets = 5, -- Raid Tickets threshold before switching from Asura to Tower Of Hell
    RaidTier = 1, -- Difficulty tier (1-15) for Tower of Hell / tiered raids
    RaidAutoCreateStart = true, -- Automatically create party & start run when idle in lobby
    AutoPickAbyssCards = true,
    AbyssCardStrategy = "Balanced (Meta DPS+Survive)", -- "Balanced (Meta DPS+Survive)", "Max Damage (Glass Cannon OK)", "Safe Survival (No Risky Cursed)"
    AbyssCollectHealOrbs = true,
    AbyssHealOrbHP = 75, -- Collect HealOrb drops when HP% drops below threshold
    AbyssAutoAdvanceGate = true, -- Enters opened ArrivalCage gates & opens Boss Chests
    AutoRaidRevive = true,
    AutoRaidRetry = true,
    AbyssStopFloor = 0, -- 0 = endless, 1-100 = AbyssService:Leave() at floor
    RaidLowHpFallback = true, -- Retreats onto Arrival Roof / safe perch when low HP to regen + drink Health Potions
    RaidFallbackEnterHP = 35, -- Retreat & pop Health Potions when HP% drops to or below this
    RaidFallbackResumeHP = 85, -- Return to combat once HP% recovers to this threshold
    RaidAutoBuyHealthPots = true, -- Buys Health Potions in Lobby/Island before entering run if below target stock
    RaidHealthPotStock = 5, -- Target Health Potion count to keep before starting a Raid / Abyss run

    -- Character & Utilities
    AntiAFK = true,
    CustomWalkSpeed = false,
    WalkSpeedValue = 28,
    Noclip = false,
    NoclipCamera = false,
    Fullbright = false,
    RemoveFog = false,
    FpsBoost = false,
    AutoClaimIndex = false,
    AutoClaimLevelRewards = false,

    -- Potions (Auto Buy & Auto Use)
    AutoBuyPotions = false,
    AutoBuyPotionTypes = { "Miner Potion I", "Luck Potion I" },
    AutoBuyPotionStock = 3,
    AutoBuyAfterBuffsExpire = true,
    AutoBuyPotionIntervalMin = 5,
    AutoUsePotions = false,
    AutoStackPotions = true,
    AutoUsePotionTypes = { "Miner Potion I", "Luck Potion I" },
    AutoUseHealthPct = 55,

    -- Discord Webhook (Money, Level & Rare Ore Alerts)
    WebhookEnabled = false,
    WebhookUrl = "",
    WebhookUserId = "",
    WebhookNotifyRareOres = true,
    WebhookNotifyForgeCycle = true, -- Sends Before (Bag Full) & After (Forge Complete) status embeds
    WebhookNotifyRaidRewards = true, -- Sends Tower of Hell / Abyss run rewards & summary embed
    WebhookRaidPing = false, -- Pings Discord User ID on Tower of Hell / Abyss reward embeds
    WebhookRaidSendEmpty = true, -- Still sends run summary (Gold/Wave/Duration) even if 0 items dropped
    AutoExecOnTeleport = false, -- Queues loader/script via queue_on_teleport across server hops & Island/Raid teleports
    AutoExecLoader = "", -- Optional loader URL or loadstring(...)() snippet (auto-reads __AetherAutoExec global if blank)
    WebhookPingMinRarity = "Mythic", -- Minimum rarity tier to ping ("Off (Custom List Only)", "Rare".."Divine")
    WebhookWatchOres = { "Darkryte", "Demonite", "Arcane Crystal", "Heavenite", "Gargantuan" },
    WebhookStatusIntervalMin = 10, -- Periodic Gold/Level summary interval in minutes (0 = rare ore alerts / manual only)

    -- Runes Optimizer & Auto Sell Runes
    AutoPutRunes = false,
    AutoSellRunes = false,
    RuneSellKeepList = {}, -- Multi-select whitelist of runes to KEEP (everything unchecked is sold when runes/bag max out)
    RuneEquipFilter = "Pickaxes & Weapons",
    RuneTargetEquipment = "",
    RuneTraitFocus = "All (Highest % Overall)",
    RuneSelectedManual = "Auto: Best Compatible",
    
    -- Visuals / ESP & Persistence
    RockEsp = false,
    MobEsp = false,
    AutoSaveConfig = true,
    Theme = "Default"
}

-- ===================================================================
-- 3B. PERSISTENT AUTO-SAVE CONFIG MANAGER (Aether/Configs/TheForge.json)
-- ===================================================================
local ConfigManager = {
    Defaults = {},
    LastSnapshot = nil,
    LoadedOk = true,
    Worlds = {},
    PerWorldKeys = {
        MineTargets = true,
        MineTarget = true,
        MineZones = true,
        MobFarmTargets = true,
        SplitEventVariantMobs = true,
        TargetMob = true,
        WantedRockOres = true,
        BackupMineTargets = true,
        SavedMinePos = true,
        StayAtSavedPos = true,
    },
}

for k, v in pairs(Config) do
    ConfigManager.Defaults[k] = (type(v) == "table") and table.clone(v) or v
end

function ConfigManager.GetPath()
    return "Aether/Configs/TheForge.json"
end

function ConfigManager.EnsureFolder()
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if not hasFS then return false end
    pcall(function()
        if type(isfolder) == "function" and type(makefolder) == "function" then
            if not isfolder("Aether") then makefolder("Aether") end
            if not isfolder("Aether/Configs") then makefolder("Aether/Configs") end
        end
    end)
    return true
end

local function sanitizeForJson(val, depth)
    depth = (depth or 0) + 1
    if depth > 6 then return nil end
    local tv = type(val)
    if tv == "boolean" or tv == "string" then
        return val
    elseif tv == "number" then
        if val ~= val or val == math.huge or val == -math.huge then
            return nil
        end
        return val
    elseif tv == "table" then
        local strKeys, numKeys = 0, 0
        for k, v in pairs(val) do
            if type(k) == "number" and k >= 1 and math.floor(k) == k then
                numKeys = numKeys + 1
            elseif type(k) == "string" then
                strKeys = strKeys + 1
            end
        end
        local out = {}
        if strKeys > 0 then
            -- Pure JSON object (skip numeric keys if mixed so HttpService:JSONEncode never errors)
            for k, v in pairs(val) do
                if type(k) == "string" then
                    local sv = sanitizeForJson(v, depth)
                    if sv ~= nil then out[k] = sv end
                end
            end
        elseif numKeys > 0 then
            -- Dense 1-based JSON array (avoid sparse array gaps so JSONEncode treats as array)
            local arr = {}
            for k, v in pairs(val) do
                if type(k) == "number" and k >= 1 and math.floor(k) == k then
                    table.insert(arr, { k = k, v = v })
                end
            end
            table.sort(arr, function(a, b) return a.k < b.k end)
            for _, entry in ipairs(arr) do
                local sv = sanitizeForJson(entry.v, depth)
                if sv ~= nil then table.insert(out, sv) end
            end
        end
        return out
    end
    return nil
end


function ConfigManager.SerializeConfig()
    local settings = {}
    for k, v in pairs(Config) do
        if not ConfigManager.PerWorldKeys[k] then
            local sv = sanitizeForJson(v, 0)
            if sv ~= nil then
                settings[k] = sv
            end
        end
    end
    return settings
end

function ConfigManager.SerializeWorldConfig()
    local worldCfg = {}
    for k in pairs(ConfigManager.PerWorldKeys) do
        local sv = sanitizeForJson(Config[k], 0)
        if sv ~= nil then
            worldCfg[k] = sv
        end
    end
    return worldCfg
end

function ConfigManager.BuildSnapshot()
    return HttpService:JSONEncode({
        global = ConfigManager.SerializeConfig(),
        world = ConfigManager.SerializeWorldConfig(),
    })
end

function ConfigManager.Save()
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if not hasFS then return false end
    local ok = pcall(function()
        ConfigManager.EnsureFolder()
        local filePath = ConfigManager.GetPath()
        -- Preserve other worlds' saved data on disk so switching worlds never erases prior world configs
        if type(ConfigManager.Worlds) ~= "table" then ConfigManager.Worlds = {} end
        pcall(function()
            if type(isfile) == "function" and isfile(filePath) then
                local raw = readfile(filePath)
                local decOk, dec = pcall(function() return HttpService:JSONDecode(raw) end)
                if decOk and type(dec) == "table" and type(dec.Worlds) == "table" then
                    for pidKey, wTbl in pairs(dec.Worlds) do
                        if pidKey ~= tostring(game.PlaceId) and type(wTbl) == "table" and not ConfigManager.Worlds[pidKey] then
                            ConfigManager.Worlds[pidKey] = wTbl
                        end
                    end
                end
            end
        end)

        local settings = ConfigManager.SerializeConfig()
        local worldCfg = ConfigManager.SerializeWorldConfig()
        ConfigManager.Worlds[tostring(game.PlaceId)] = worldCfg

        local dataToSave = {
            GameId = game.GameId,
            PlaceId = game.PlaceId,
            GameName = "TheForge",
            Settings = settings,
            Worlds = ConfigManager.Worlds,
        }
        local encoded = HttpService:JSONEncode(dataToSave)
        local writeOk = false
        for _ = 1, 3 do
            if pcall(writefile, filePath, encoded) then
                writeOk = true
                break
            end
            task.wait(0.05)
        end
        if writeOk then
            pcall(writefile, filePath .. ".bak", encoded)
            ConfigManager.LastSnapshot = ConfigManager.BuildSnapshot()
        end
    end)
    return ok
end

function ConfigManager.ScheduleSave(delaySec)
    local token = (ConfigManager.SaveToken or 0) + 1
    ConfigManager.SaveToken = token
    ConfigManager.SavePending = true
    ConfigManager.SaveDueAt = os.clock() + (delaySec or 0.08)
    task.delay(delaySec or 0.08, function()
        if not ScriptRunning or ConfigManager.SaveToken ~= token then return end
        if Config.AutoSaveConfig ~= false and ConfigManager.LoadedOk ~= false then
            pcall(function()
                local cur = ConfigManager.BuildSnapshot()
                ConfigManager.SavePending = false
                ConfigManager.SaveDueAt = nil
                if cur ~= ConfigManager.LastSnapshot then
                    ConfigManager.Save()
                end
            end)
        end
    end)
end

function ConfigManager.Load()
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if not hasFS then return false end
    local filePath = ConfigManager.GetPath()
    local bakPath = filePath .. ".bak"
    local fileExists = false
    local bakExists = false
    for attempt = 1, 4 do
        pcall(function()
            if type(isfile) == "function" then
                fileExists = isfile(filePath)
                bakExists = isfile(bakPath)
            end
        end)
        if fileExists or bakExists then break end
        if attempt < 4 then task.wait(0.1) end
    end
    if not fileExists and not bakExists then
        ConfigManager.LoadedOk = true
        ConfigManager.Save()
        return false
    end

    local function readDecoded(path)
        local raw = nil
        for _ = 1, 4 do
            local ok = pcall(function() raw = readfile(path) end)
            if ok and raw and raw ~= "" then break end
            task.wait(0.08)
        end
        if not raw or raw == "" then return nil end
        local okDec, dec = pcall(function() return HttpService:JSONDecode(raw) end)
        if okDec and type(dec) == "table" then return dec end
        return nil
    end

    local function pickValidPayload(path)
        local dec = readDecoded(path)
        if type(dec) ~= "table" then return nil end
        -- Reject UI.lua Settings Manager payloads (which have Elements without Settings/Worlds)
        if dec.Elements ~= nil and dec.Settings == nil and dec.Worlds == nil then
            return nil
        end
        return dec
    end

    local decoded = (fileExists and pickValidPayload(filePath)) or (bakExists and pickValidPayload(bakPath)) or nil
    if not decoded then
        ConfigManager.LoadedOk = false
        return false
    end

    local settings = decoded.Settings or decoded
    if type(settings) ~= "table" then
        ConfigManager.LoadedOk = false
        return false
    end
    ConfigManager.LoadedOk = true

    ConfigManager.Worlds = (type(decoded.Worlds) == "table") and decoded.Worlds or {}
    local worldKey = tostring(game.PlaceId)
    local hasWorldEntry = type(ConfigManager.Worlds[worldKey]) == "table"

    local function normalizeLoadedTable(tbl)
        local cloned = {}
        local isArray = true
        local maxIdx = 0
        for k, val in pairs(tbl) do
            if type(k) ~= "number" or k < 1 or math.floor(k) ~= k then
                isArray = false
                break
            elseif k > maxIdx then
                maxIdx = k
            end
        end
        if isArray then
            for _, val in ipairs(tbl) do
                table.insert(cloned, val)
            end
            return cloned
        end
        -- Handle dictionary multi-select format { ["Ore"] = true }
        local dictBoolOnly = true
        for k, val in pairs(tbl) do
            if type(k) ~= "string" or type(val) ~= "boolean" then
                dictBoolOnly = false
                break
            end
        end
        if dictBoolOnly and tbl.placeId == nil and tbl.x == nil then
            for k, val in pairs(tbl) do
                if val == true then table.insert(cloned, k) end
            end
            return cloned
        end
        return table.clone(tbl)
    end

    for k, v in pairs(settings) do
        if ConfigManager.Defaults[k] ~= nil then
            -- If Worlds[placeId] already exists, don't let legacy/other-world fields in Settings overwrite per-world defaults
            if not (hasWorldEntry and ConfigManager.PerWorldKeys[k]) then
                if type(ConfigManager.Defaults[k]) == "table" and type(v) == "table" then
                    Config[k] = normalizeLoadedTable(v)
                elseif type(v) == type(ConfigManager.Defaults[k]) then
                    Config[k] = v
                end
            end
        end
    end

    if hasWorldEntry then
        for k, v in pairs(ConfigManager.Worlds[worldKey]) do
            if ConfigManager.PerWorldKeys[k] and ConfigManager.Defaults[k] ~= nil then
                if type(ConfigManager.Defaults[k]) == "table" and type(v) == "table" then
                    Config[k] = normalizeLoadedTable(v)
                elseif type(v) == type(ConfigManager.Defaults[k]) then
                    Config[k] = v
                end
            end
        end
    end

    -- Migrate legacy per-world SellOreWhitelist into global Config.SellOreWhitelist ONLY if Settings had no SellOreWhitelist key yet
    if settings.SellOreWhitelist == nil then
        local srcWorld = hasWorldEntry and ConfigManager.Worlds[worldKey]
        if type(srcWorld) == "table" and type(srcWorld.SellOreWhitelist) == "table" and next(srcWorld.SellOreWhitelist) ~= nil then
            Config.SellOreWhitelist = normalizeLoadedTable(srcWorld.SellOreWhitelist)
        else
            for _, wTbl in pairs(ConfigManager.Worlds) do
                if type(wTbl) == "table" and type(wTbl.SellOreWhitelist) == "table" and next(wTbl.SellOreWhitelist) ~= nil then
                    Config.SellOreWhitelist = normalizeLoadedTable(wTbl.SellOreWhitelist)
                    break
                end
            end
        end
    end

    -- Never inherit another world's SavedMinePos / StayAtSavedPos
    if type(Config.SavedMinePos) ~= "table" or Config.SavedMinePos.placeId ~= game.PlaceId then
        Config.SavedMinePos = {}
        Config.StayAtSavedPos = false
    end

    pcall(function()
        ConfigManager.LastSnapshot = ConfigManager.BuildSnapshot()
    end)
    return true
end

function ConfigManager.Reset()
    for k, v in pairs(ConfigManager.Defaults) do
        Config[k] = (type(v) == "table") and table.clone(v) or v
    end
    ConfigManager.Save()
end

ConfigManager.Load()
if not ConfigManager.LastSnapshot then
    pcall(function()
        ConfigManager.LastSnapshot = ConfigManager.BuildSnapshot()
    end)
end

-- Flush Config before any Raid / Island teleport so recent dropdown/toggle changes never get lost
if LocalPlayer and LocalPlayer.OnTeleport then
    trackConn(LocalPlayer.OnTeleport:Connect(function()
        if Config.AutoSaveConfig ~= false and ConfigManager.LoadedOk ~= false then
            pcall(ConfigManager.Save)
        end
    end))
end

-- Fast Background Auto-Save Watcher: saves within ~0.1s of any callback or 0.35s snapshot diff
task.spawn(function()
    while ScriptRunning do
        task.wait(0.25)
        if Config.AutoSaveConfig ~= false and ConfigManager.LoadedOk ~= false then
            pcall(function()
                local due = ConfigManager.SavePending and os.clock() >= (ConfigManager.SaveDueAt or 0)
                local cur = ConfigManager.BuildSnapshot()
                if due or cur ~= ConfigManager.LastSnapshot then
                    ConfigManager.SavePending = false
                    ConfigManager.SaveDueAt = nil
                    ConfigManager.Save()
                end
            end)
        end
    end
end)

-- Dynamically resolves ONLY the rock species that exist in the current World / Island
local RockTypes = { "All" }
runIsolated(function()
    local seen = { ["All"] = true }
    local worldList = {}
    local okCave, CaveData = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Cave) end)
    local okRock, RockData = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Rock) end)
    local rocksFolder = workspace:WaitForChild("Rocks", 4) or workspace:FindFirstChild("Rocks")
    if rocksFolder then
        local function collectRockNames(container)
            for _, ch in ipairs(container:GetChildren()) do
                if ch:IsA("Model") and (ch:GetAttribute("Health") ~= nil or ch:FindFirstChild("Hitbox") or ch:GetAttribute("RequiredDamage") ~= nil) then
                    if not seen[ch.Name] then
                        seen[ch.Name] = true
                        table.insert(worldList, ch.Name)
                    end
                elseif ch:IsA("Folder") or ch:IsA("BasePart") or ch:IsA("Model") then
                    collectRockNames(ch)
                end
            end
        end
        for _, zone in ipairs(rocksFolder:GetChildren()) do
            if okCave and type(CaveData) == "table" and CaveData[zone.Name] and type(CaveData[zone.Name].Rocks) == "table" then
                for _, entry in ipairs(CaveData[zone.Name].Rocks) do
                    if entry.Name and not seen[entry.Name] then
                        seen[entry.Name] = true
                        table.insert(worldList, entry.Name)
                    end
                end
            end
            collectRockNames(zone)
        end
    end
    if #worldList == 0 then
        worldList = { "Pebble", "Rock", "Boulder", "Lucky Block" }
    else
        table.sort(worldList, function(a, b)
            if a == "Lucky Block" then return false end
            if b == "Lucky Block" then return true end
            local da = (okRock and type(RockData) == "table" and RockData[a] and RockData[a].RequiredDamage) or 9999
            local db = (okRock and type(RockData) == "table" and RockData[b] and RockData[b].RequiredDamage) or 9999
            if da ~= db then return da < db end
            return a < b
        end)
    end
    for _, name in ipairs(worldList) do
        table.insert(RockTypes, name)
    end
end)

-- Dynamically resolves mineable zone folders under workspace.Rocks for the current World
local MineZones = { "Any" }
local MineZoneLabelToFolder = {}
runIsolated(function()
    local rocksFolder = workspace:FindFirstChild("Rocks")
    if not rocksFolder then return end
    local names = {}
    local seen = { ["Any"] = true }
    for _, zone in ipairs(rocksFolder:GetChildren()) do
        if zone.Name ~= "BossShields" and not zone.Name:find("Closed", 1, true) then
            local niceName = zone.Name:gsub("^Island%d+", "")
            if niceName == "" then niceName = zone.Name end
            MineZoneLabelToFolder[niceName] = zone.Name
            MineZoneLabelToFolder[zone.Name] = zone.Name
            if not seen[niceName] then
                seen[niceName] = true
                table.insert(names, niceName)
            end
        end
    end
    table.sort(names)
    for _, n in ipairs(names) do
        table.insert(MineZones, n)
    end
end)

-- Dynamically builds MobTypes ("All" + enemies belonging exclusively to the current Island / PlaceId)
local MobTypes = { "All" }
function ConfigManager.RebuildMobTypes()
    table.clear(MobTypes)
    table.insert(MobTypes, "All")
    local splitVariants = (Config.SplitEventVariantMobs == true)
    local variantPrefixes = { "Limbo", "Heaven", "Hell" }
    runIsolated(function()
        local seen = { ["All"] = true }
        local currentIslandName = nil
        pcall(function()
            local Islands = safeRequire(ReplicatedStorage.Shared.Data.Islands)
            for _, isl in ipairs(Islands.Data or {}) do
                if isl.PlaceId == game.PlaceId then
                    currentIslandName = isl.Name
                    break
                end
            end
        end)

        -- Maps each Island (or PlaceId) to its Enemies.Data[...].Island region names
        local IslandEnemyRegions = {
            ["Stonewake's Cross"] = { ["Stonewake's Cross"] = true, ["Iron Valley"] = true },
            ["Forgotten Kingdom"] = { ["Forgotten Kingdom"] = true, ["Goblin Cave"] = true },
            ["Frostspire Expanse"] = { ["Frostspire Expanse"] = true, ["The Peak"] = true, ["Raven Cave"] = true, ["The Abyss"] = true },
            ["Crimson Sakura Isles"] = { ["Crimson Sakura Isles"] = true, ["Bamboo Cave"] = true, ["Holy Tree"] = true, ["Jade Spirit Cave"] = true, ["Aurelia Hall"] = true },
        }
        local PlaceIdFallbackIsland = {
            [76558904092080] = "Stonewake's Cross",
            [129009554587176] = "Forgotten Kingdom",
            [131884594917121] = "Frostspire Expanse",
            [74414241680540] = "Crimson Sakura Isles",
        }
        local resolvedIsland = currentIslandName or PlaceIdFallbackIsland[game.PlaceId]
        local allowedRegions = resolvedIsland and IslandEnemyRegions[resolvedIsland] or nil

        local function getEventPrefix(m)
            local mut = m:GetAttribute("Mutation")
            if mut == "Limbo" or mut == "Heaven" or mut == "Hell" then
                return mut
            end
            local hum = m:FindFirstChildOfClass("Humanoid")
            local dn = hum and hum.DisplayName
            if type(dn) == "string" then
                for _, pref in ipairs(variantPrefixes) do
                    if dn:sub(1, #pref + 1) == (pref .. " ") then
                        return pref
                    end
                end
            end
            return nil
        end

        local function hasAnyPrefix(name)
            for _, pref in ipairs(variantPrefixes) do
                if name:sub(1, #pref + 1) == (pref .. " ") then
                    return true
                end
            end
            return false
        end

        local localList = {}
        local fallbackList = {}
        local localSet = {}

        -- 1. Live spawned hostile NPC species currently present in this map
        local living = workspace:FindFirstChild("Living")
        if living then
            for _, m in ipairs(living:GetChildren()) do
                if m:IsA("Model") and m:GetAttribute("IsNpc") == true and m:GetAttribute("DpsDummy") ~= true then
                    local sp = tostring(m.Name):gsub("%d+$", "")
                    if sp ~= "" and not sp:find("Dummy", 1, true) then
                        localSet[sp] = true
                        if splitVariants then
                            local pref = getEventPrefix(m)
                            if pref then
                                localSet[pref .. " " .. sp] = true
                            end
                        end
                    end
                end
            end
        end

        -- 2. Filter Shared.Data.Enemies strictly to current island's zones (+ any live-spawned species)
        local okE, EnemiesData = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Enemies) end)
        if okE and type(EnemiesData) == "table" then
            for key, entry in pairs(EnemiesData) do
                if type(entry) == "table" then
                    local name = tostring(entry.Name or key)
                    if name ~= "" and not name:find("Dummy", 1, true) and not seen[name] then
                        seen[name] = true
                        local belongsHere = localSet[name]
                            or (allowedRegions and entry.Island and allowedRegions[entry.Island] == true)
                            or (resolvedIsland and entry.Island == resolvedIsland)
                        if belongsHere then
                            table.insert(localList, name)
                        else
                            table.insert(fallbackList, name)
                        end
                    end
                end
            end
        end

        for sp in pairs(localSet) do
            if not seen[sp] and not hasAnyPrefix(sp) then
                seen[sp] = true
                table.insert(localList, sp)
            end
        end

        local variantList = {}
        local variantAdded = {}
        local function addVariantsFor(baseName)
            if not splitVariants or type(baseName) ~= "string" or baseName == "" or hasAnyPrefix(baseName) then return end
            for _, pref in ipairs(variantPrefixes) do
                local vName = pref .. " " .. baseName
                if not variantAdded[vName] then
                    variantAdded[vName] = true
                    table.insert(variantList, vName)
                end
            end
        end

        table.sort(localList)
        if #localList > 0 then
            for _, n in ipairs(localList) do
                table.insert(MobTypes, n)
                addVariantsFor(n)
            end
        else
            table.sort(fallbackList)
            for _, n in ipairs(fallbackList) do
                table.insert(MobTypes, n)
                addVariantsFor(n)
            end
        end

        if splitVariants then
            for sp in pairs(localSet) do
                if hasAnyPrefix(sp) and not variantAdded[sp] then
                    variantAdded[sp] = true
                    table.insert(variantList, sp)
                end
            end
            table.sort(variantList)
            for _, vName in ipairs(variantList) do
                table.insert(MobTypes, vName)
            end
        end
    end)

    ConfigManager.ValidMobSet = {}
    for _, n in ipairs(MobTypes) do ConfigManager.ValidMobSet[n] = true end
end
ConfigManager.RebuildMobTypes()

-- Lookup sets + current-world config sanitizer so switching between Worlds 1-4 never leaks foreign rocks/zones/mobs/saved spots
ConfigManager.ValidRockSet = {}
for _, n in ipairs(RockTypes) do ConfigManager.ValidRockSet[n] = true end
ConfigManager.ValidZoneSet = {}
for _, n in ipairs(MineZones) do ConfigManager.ValidZoneSet[n] = true end

function ConfigManager.SanitizeMultiList(raw, validSet, fallbackItem)
    local out = {}
    local seen = {}
    if type(raw) == "table" then
        for k, v in pairs(raw) do
            local name = (type(k) == "number") and v or (v and k or nil)
            if type(name) == "string" and name ~= "" and validSet[name] and not seen[name] then
                seen[name] = true
                table.insert(out, name)
            end
        end
    elseif type(raw) == "string" and validSet[raw] then
        table.insert(out, raw)
    end
    if #out == 0 then
        table.insert(out, fallbackItem)
    elseif #out > 1 then
        for idx, item in ipairs(out) do
            if item == fallbackItem then
                table.remove(out, idx)
                break
            end
        end
    end
    return out
end

function ConfigManager.SanitizeForCurrentWorld()
    local hasRocksCatalog = false
    for _, n in ipairs(RockTypes) do
        if n ~= "All" and n ~= "Pebble" and n ~= "Rock" and n ~= "Boulder" and n ~= "Lucky Block" then
            hasRocksCatalog = true
            break
        end
    end
    if hasRocksCatalog or (#RockTypes > 1 and workspace:FindFirstChild("Rocks") ~= nil) then
        Config.MineTargets = ConfigManager.SanitizeMultiList(Config.MineTargets, ConfigManager.ValidRockSet, "All")
        if type(Config.MineTarget) ~= "string" or not ConfigManager.ValidRockSet[Config.MineTarget] then
            Config.MineTarget = "All"
        end
        local backupValid = { ["Auto (Rocks Lacking Wanted Ores)"] = true }
        if type(ConfigManager.ValidRockSet) == "table" then
            for rk, rv in pairs(ConfigManager.ValidRockSet) do
                if rv and rk ~= "All" then
                    backupValid[rk] = true
                end
            end
        end
        Config.BackupMineTargets = ConfigManager.SanitizeMultiList(Config.BackupMineTargets, backupValid, "Auto (Rocks Lacking Wanted Ores)")
    end
    if #MineZones > 1 then
        Config.MineZones = ConfigManager.SanitizeMultiList(Config.MineZones, ConfigManager.ValidZoneSet, "Any")
    end
    if #MobTypes > 1 then
        Config.MobFarmTargets = ConfigManager.SanitizeMultiList(Config.MobFarmTargets, ConfigManager.ValidMobSet, "All")
        if type(Config.TargetMob) ~= "string" or not ConfigManager.ValidMobSet[Config.TargetMob] then
            Config.TargetMob = "All"
        end
    end
    if type(Config.SavedMinePos) ~= "table" or Config.SavedMinePos.placeId ~= game.PlaceId then
        Config.SavedMinePos = {}
        Config.StayAtSavedPos = false
    end
    local worldKey = tostring(game.PlaceId)
    if type(ConfigManager.Worlds) ~= "table" or type(ConfigManager.Worlds[worldKey]) ~= "table" then
        if Config.AutoSaveConfig ~= false and ConfigManager.LoadedOk ~= false then
            pcall(ConfigManager.Save)
        end
    end
    pcall(function()
        ConfigManager.LastSnapshot = ConfigManager.BuildSnapshot()
    end)
end
ConfigManager.SanitizeForCurrentWorld()

-- Dynamically builds GreedyCeyLootTypes (sellable shards/fragments, bones, essences, runes & ores)
local GreedyCeyLootTypes = {}
function ConfigManager.RebuildGreedyCeyLootTypes()
    table.clear(GreedyCeyLootTypes)
    runIsolated(function()
        local added = {}
        local priorityList = {}
        local otherList = {}

        local okE, EnemiesData = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Enemies) end)
        local enemyDropSet = {}
        if okE and type(EnemiesData) == "table" then
            for _, entry in pairs(EnemiesData) do
                if type(entry) == "table" and type(entry.LootData) == "table" then
                    for _, arr in pairs(entry.LootData) do
                        if type(arr) == "table" then
                            for _, item in ipairs(arr) do
                                if type(item) == "table" and item.Name then
                                    enemyDropSet[item.Name] = true
                                end
                            end
                        end
                    end
                end
            end
        end

        local okEx, Extras = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Extras) end)
        local itemTypes = (typeof(getrenv) == "function" and getrenv()._G and getrenv()._G.ItemTypes) or _G.ItemTypes
        if type(itemTypes) == "table" then
            for itemName, info in pairs(itemTypes) do
                if type(info) == "table" and not added[itemName] then
                    local kind = info[1]
                    local isSellable = (kind == "Ore" or kind == "Material" or kind == "Rune"
                        or (kind == "MiscQuest" and okEx and Extras and type(Extras.GetSellPrice) == "function" and Extras:GetSellPrice(itemName) ~= nil))
                    if isSellable then
                        added[itemName] = true
                        local isPriority = enemyDropSet[itemName]
                            or kind == "Material"
                            or kind == "Rune"
                            or itemName:find("Bone", 1, true)
                            or itemName:find("Shard", 1, true)
                            or itemName:find("Fragment", 1, true)
                            or itemName:find("Essence", 1, true)
                        if isPriority then
                            table.insert(priorityList, itemName)
                        else
                            table.insert(otherList, itemName)
                        end
                    end
                end
            end
        end

        local okM, Mats = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Materials) end)
        if okM and type(Mats) == "table" and type(Mats.Items) == "table" then
            for _, entry in pairs(Mats.Items) do
                if type(entry) == "table" and entry.Name and not added[entry.Name] then
                    added[entry.Name] = true
                    table.insert(priorityList, entry.Name)
                end
            end
        end

        table.sort(priorityList)
        table.sort(otherList)
        for _, n in ipairs(priorityList) do table.insert(GreedyCeyLootTypes, n) end
        for _, n in ipairs(otherList) do table.insert(GreedyCeyLootTypes, n) end
    end)
    return #GreedyCeyLootTypes > 0
end
ConfigManager.RebuildGreedyCeyLootTypes()

-- ===================================================================
-- 4. KNIT DIRECT REMOTES RESOLUTION
-- ===================================================================
local Remotes = {}
local function ResolveRemotes()
    local sFolder = ReplicatedStorage:WaitForChild("Shared", 6)
    if not sFolder then return end
    local pFolder = sFolder:WaitForChild("Packages", 6)
    if not pFolder then return end
    local kFolder = pFolder:WaitForChild("Knit", 6)
    if not kFolder then return end
    local services = kFolder:WaitForChild("Services", 6)
    if not services then return end

    local function getRemote(serviceName, rType, remoteName)
        local s = services:FindFirstChild(serviceName)
        if s and s:FindFirstChild(rType) then
            return s[rType]:FindFirstChild(remoteName)
        end
        return nil
    end

    Remotes.ToolActivated = getRemote("ToolService", "RF", "ToolActivated")
    Remotes.StartBlock = getRemote("ToolService", "RF", "StartBlock")
    Remotes.StopBlock = getRemote("ToolService", "RF", "StopBlock")
    Remotes.EquipItem = getRemote("CharacterService", "RF", "EquipItem")
    Remotes.EquipMisc = getRemote("CharacterService", "RF", "EquipMisc")
    Remotes.ResetCharacter = getRemote("CharacterService", "RF", "Reset")
    Remotes.Run = getRemote("CharacterService", "RF", "Run")
    Remotes.StopRun = getRemote("CharacterService", "RF", "StopRun")
    Remotes.ChangeSequence = getRemote("ForgeService", "RF", "ChangeSequence")
    Remotes.StartForge = getRemote("ForgeService", "RF", "StartForge")
    Remotes.EndForge = getRemote("ForgeService", "RF", "EndForge")
    Remotes.SellAllStash = getRemote("InventoryService", "RF", "SellAllStash")
    Remotes.Sell = getRemote("InventoryService", "RF", "Sell")
    Remotes.SellAnywhere = getRemote("InventoryService", "RF", "SellAnywhere")
    Remotes.RecycleBasket = getRemote("InventoryService", "RF", "RecycleBasket")
    Remotes.FavoriteItem = getRemote("InventoryService", "RF", "FavoriteItem")
    Remotes.Purchase = getRemote("ProximityService", "RF", "Purchase")
    Remotes.ForceDialogue = getRemote("ProximityService", "RF", "ForceDialogue")
    Remotes.ProximityForge = getRemote("ProximityService", "RF", "Forge")
    Remotes.RunCommand = getRemote("DialogueService", "RF", "RunCommand")
    Remotes.StartCrafting = getRemote("OreCraftService", "RF", "StartCrafting")
    Remotes.CheckCraftCompletions = getRemote("OreCraftService", "RF", "CheckCraftCompletions")
    Remotes.CraftRequest = getRemote("OreCraftService", "RF", "CraftRequest")
    Remotes.OreCraftEvent = getRemote("OreCraftService", "RE", "Event")
    Remotes.RequestTeleport = getRemote("WaypointService", "RF", "RequestTeleport")
    Remotes.RequestUnlock = getRemote("WaypointService", "RF", "RequestUnlock")
    Remotes.TeleportToIsland = getRemote("PortalService", "RF", "TeleportToIsland")
    Remotes.ClaimDaily = getRemote("DailyLoginService", "RF", "Claim")
    Remotes.ClaimMailAll = getRemote("MailService", "RF", "ClaimAll")
    Remotes.CodexClaimAll = getRemote("CodexService", "RF", "ClaimAll")
    Remotes.LevelRewardsClaimAll = getRemote("LevelRewardsService", "RF", "ClaimAll")
    Remotes.RuneGetPriceInfo = getRemote("RuneService", "RF", "GetPriceInfo")
    Remotes.RunePurchaseAttach = getRemote("RuneService", "RF", "PurchaseAttach")

    -- Abyss, Party & Raid Remotes
    Remotes.AbyssEnter = getRemote("AbyssService", "RF", "Enter")
    Remotes.AbyssLeave = getRemote("AbyssService", "RF", "Leave")
    Remotes.AbyssGetInfo = getRemote("AbyssService", "RF", "GetInfo")
    Remotes.AbyssPickCard = getRemote("AbyssService", "RF", "PickCard")
    Remotes.AbyssSkipCards = getRemote("AbyssService", "RF", "SkipCards")
    Remotes.AbyssRerollCards = getRemote("AbyssService", "RF", "RerollCards")
    Remotes.AbyssCardOffer = getRemote("AbyssService", "RE", "CardOffer")
    Remotes.AbyssCardPicked = getRemote("AbyssService", "RE", "CardPicked")
    Remotes.AbyssProgress = getRemote("AbyssService", "RE", "Progress")
    Remotes.AbyssDrops = getRemote("AbyssService", "RE", "Drops")
    Remotes.BossArenaLoading = getRemote("BossSpawnService", "RE", "ArenaLoading")
    Remotes.AbyssBossChestOpen = getRemote("AbyssChestService", "RF", "Open")
    Remotes.PartyCreate = getRemote("PartyService", "RF", "CreateParty")
    Remotes.PartyActivate = getRemote("PartyService", "RF", "Activate")
    Remotes.PartySetTier = getRemote("PartyService", "RF", "SetTier")
    Remotes.PartyLeave = getRemote("PartyService", "RF", "Leave")
    Remotes.RaidRetry = getRemote("RaidService", "RF", "Retry")
    Remotes.RaidGoToIsland = getRemote("RaidService", "RF", "GoToIsland")
    Remotes.RaidRevive = getRemote("RaidService", "RF", "Revive")
    Remotes.RaidReviveOffer = getRemote("RaidService", "RE", "ReviveOffer")
    Remotes.RaidEnded = getRemote("RaidService", "RE", "RaidEnded")
    Remotes.RaidRestarted = getRemote("RaidService", "RE", "RaidRestarted")
    Remotes.RaidCurrentWave = getRemote("RaidService", "RE", "CurrentWave")
    Remotes.RaidEnemiesLeft = getRemote("RaidService", "RE", "EnemiesLeft")
end
ResolveRemotes()

-- ===================================================================
-- 5. REPLICA & PLAYER STATE TRACKING
-- ===================================================================
local function UpdateCharacter(char)
    State.Character = char or (LocalPlayer and LocalPlayer.Character)
    if State.Character then
        if NavEngine then
            NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
            NavEngine.IsGliding = false
            NavEngine.CachedRock = nil
        end
        State.IsBuyingPotions = false
        State.IsBuyingHealthPotions = false
        State.SellingMobLoot = false
        State.Humanoid = State.Character:WaitForChild("Humanoid", 4)
        State.HRP = State.Character:WaitForChild("HumanoidRootPart", 4)
        if CombatEngine then
            CombatEngine.ActiveStanceCF = nil
            CombatEngine.ActiveStanceMob = nil
            CombatEngine.HoverLockActive = false
            CombatEngine.LastEngagedMob = nil
            CombatEngine.DodgeUntil = 0
            CombatEngine.IsParrying = false
            CombatEngine.ParryHoldUntil = 0
        end
        if RaidEngine then
            RaidEngine.RecoveringHP = false
            RaidEngine.SafeRegenCF = nil
            RaidEngine.StrandedSince = nil
            if not RaidEngine.IsInActiveRun() and not RaidEngine.GetActiveBossArena() then
                RaidEngine.ArenaLoadingUntil = 0
            end
        end
        if State.Humanoid then
            State.OriginalWalkSpeed = State.Humanoid.WalkSpeed
        end
        if Config.AutoMine and not Config.StayAtSavedPos and not State.IsAutoForging and not State.DrainingFullBag and not State.IsCrafting and not State.IsBuyingPotions
            and State.LastMinedRock and MiningEngine
            and MiningEngine.IsRockAlive(State.LastMinedRock)
            and MiningEngine.MatchesTargetRock(State.LastMinedRock.Name) then
            -- Death recovery: return to the exact rock being mined before death
            State.TargetRock = State.LastMinedRock
            State.CurrentRock = State.LastMinedRock
            State.ResumingAfterDeath = true
            NavEngine.CachedRock = nil
            MiningEngine.TargetLastHp = State.LastMinedRock:GetAttribute("Health")
            MiningEngine.TargetLastDamageTime = os.clock()
            MiningEngine.SwingsOnTarget = 0
            MiningEngine.NoDamageSwings = 0
        else
            State.TargetRock = nil
            State.CurrentRock = nil
            State.LastMinedRock = nil
            State.ResumingAfterDeath = false
            if MiningEngine then
                MiningEngine.TargetLastHp = nil
                MiningEngine.TargetLastDamageTime = nil
                MiningEngine.SwingsOnTarget = 0
                MiningEngine.NoDamageSwings = 0
            end
        end

        if Config.AutoMobFarm and Config.ResumeLastMobAfterDeath and not State.IsAutoForging and not State.DrainingFullBag and not State.IsCrafting and not State.IsBuyingPotions
            and State.LastFarmedMob and State.LastFarmedMob.Parent and State.LastFarmedMob:IsDescendantOf(workspace)
            and CombatEngine and CombatEngine.IsHostileMob(State.LastFarmedMob)
            and CombatEngine.MatchesTargetMob(State.LastFarmedMob) then
            local mHum = State.LastFarmedMob:FindFirstChildOfClass("Humanoid")
            if mHum and mHum.Health > 0 then
                State.TargetMob = State.LastFarmedMob
                State.CurrentMob = State.LastFarmedMob
                State.ResumingMobAfterDeath = true
            else
                State.LastFarmedMob = nil
                State.TargetMob = nil
                State.CurrentMob = nil
                State.ResumingMobAfterDeath = false
            end
        else
            State.TargetMob = nil
            State.CurrentMob = nil
            State.ResumingMobAfterDeath = false
            if not Config.AutoMobFarm then
                State.LastFarmedMob = nil
            end
        end
    end
end
UpdateCharacter(LocalPlayer.Character)
trackConn(LocalPlayer.CharacterAdded:Connect(UpdateCharacter))

local function ResolvePlayerData()
    pcall(function()
        local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
        local PC = SafeGetController("PlayerController", KnitPkg)
        local replica = PC and (PC.Replica or PC.replica)
        if not (type(replica) == "table" and type(replica.Data) == "table") and KnitPkg then
            for _, cname in ipairs({ "ForgeController", "OreCraftController", "ToolController", "TutorialController", "WaypointController" }) do
                local ok, ctrl = pcall(KnitPkg.GetController, cname)
                if ok and type(ctrl) == "table" then
                    local cand = ctrl.replica or ctrl.Replica
                    if type(cand) == "table" and type(cand.Data) == "table" then
                        replica = cand
                        break
                    end
                end
            end
        end
        if not (type(replica) == "table" and type(replica.Data) == "table") and typeof(getgc) == "function" then
            for _, obj in ipairs(getgc(true)) do
                if type(obj) == "table" then
                    local data = rawget(obj, "Data")
                    if type(data) == "table" and type(rawget(data, "Inventory")) == "table" and type(rawget(data, "Favorites")) == "table" then
                        local tok = rawget(obj, "Token")
                        if tok == "PlayerData" or rawget(data, "Gold") ~= nil or rawget(data, "Potions") ~= nil then
                            replica = obj
                            break
                        end
                    end
                end
            end
        end
        if type(replica) == "table" and type(replica.Data) == "table" then
            State.PlayerData = replica
        end
        if PC and PC.Status ~= nil then
            State.PlayerStatus = PC.Status
        end
    end)
    return State.PlayerData
end
runIsolated(ResolvePlayerData)

-- ===================================================================
-- 6. ANTI-CHEAT SAFE POSITIONING & BROADPHASE
-- ===================================================================
local NavEngine = {}
local MiningEngine = {}
local TeleportEngine = { WaypointStatusLabel = nil }

-- Computes a position directly on the floor terrain facing the target object
function NavEngine.GetSafeGroundCFrame(targetInstance, preferredDistance)
    preferredDistance = preferredDistance or 4.0
    local primary = nil
    if targetInstance:IsA("Model") then
        primary = targetInstance.PrimaryPart or targetInstance:FindFirstChildWhichIsA("BasePart")
    elseif targetInstance:IsA("BasePart") then
        primary = targetInstance
    end
    if not primary then return nil end

    local targetPos = primary.Position

    -- Find nearest hostile mob to position on the opposite (shielded) side of the rock
    local nearestMobPos = nil
    local shortestMobDist = math.huge
    local living = workspace:FindFirstChild("Living")
    if living then
        for _, m in ipairs(living:GetChildren()) do
            if m:IsA("Model") and m:GetAttribute("IsNpc") == true and m ~= State.Character then
                local mHrp = m:FindFirstChild("HumanoidRootPart")
                local mHum = m:FindFirstChild("Humanoid")
                if mHrp and mHum and mHum.Health > 0 then
                    local d = (mHrp.Position - targetPos).Magnitude
                    if d < 45 and d < shortestMobDist then
                        shortestMobDist = d
                        nearestMobPos = mHrp.Position
                    end
                end
            end
        end
    end

    local directions = {}
    if nearestMobPos then
        -- Opposite direction vector: puts the solid rock directly between player and zombie
        local awayDir = (targetPos - nearestMobPos).Unit
        awayDir = Vector3.new(awayDir.X, 0, awayDir.Z).Unit
        table.insert(directions, awayDir)
        -- Tangent directions around rock
        table.insert(directions, Vector3.new(-awayDir.Z, 0, awayDir.X))
        table.insert(directions, Vector3.new(awayDir.Z, 0, -awayDir.X))
    end

    -- Standard cardinal directions fallback
    table.insert(directions, primary.CFrame.LookVector)
    table.insert(directions, -primary.CFrame.LookVector)
    table.insert(directions, primary.CFrame.RightVector)
    table.insert(directions, -primary.CFrame.RightVector)

    local rayParams = RaycastParams.new()
    rayParams.FilterType = Enum.RaycastFilterType.Exclude
    rayParams.FilterDescendantsInstances = { workspace.Rocks, workspace:FindFirstChild("Debris") or workspace, State.Character or LocalPlayer.Character }
    rayParams.IgnoreWater = true

    for _, dir in ipairs(directions) do
        local origin = targetPos + (dir.Unit * preferredDistance) + Vector3.new(0, 5, 0)
        local hit = workspace:Raycast(origin, Vector3.new(0, -35, 0), rayParams)
        if hit and hit.Position then
            local groundPos = hit.Position + Vector3.new(0, 3.0, 0)
            return CFrame.lookAt(groundPos, Vector3.new(targetPos.X, groundPos.Y, targetPos.Z))
        end
    end

    -- Fallback straight down with offset
    local fallbackHit = workspace:Raycast(targetPos + Vector3.new(0, 6, 0), Vector3.new(0, -40, 0), rayParams)
    if fallbackHit then
        local gPos = fallbackHit.Position + Vector3.new(0, 3.0, 0)
        return CFrame.lookAt(gPos, targetPos)
    end

    return CFrame.new(targetPos + Vector3.new(0, 2.5, preferredDistance))
end

function NavEngine.IsSunkenMode(mode)
    mode = mode or Config.MinePositionMode
    return mode == "Underground" or mode == "Underground (Safe)" or mode == "Sunken Under-Ledge (Horizontal Up)"
end

-- Computes target CFrame for mining based on selected mining position
function NavEngine.GetMiningCFrame(rock, mode)
    mode = mode or Config.MinePositionMode or "Underground"
    local primary = rock and (rock:FindFirstChild("Hitbox") or rock.PrimaryPart or rock:FindFirstChildWhichIsA("BasePart"))
    if not primary then return nil end
    local rockPos = primary.Position

    if NavEngine.IsSunkenMode(mode) then
        local depth = Config.SunkenDepth or 2.40
        local baseDist = Config.SunkenDistance or 0.85
        local pitchDeg = Config.SunkenPitch or 88

        -- Return cached stance for this rock if parameters haven't changed (prevents mid-glide target jitter)
        if NavEngine.CachedRock == rock
            and NavEngine.CachedDepth == depth
            and NavEngine.CachedDist == baseDist
            and NavEngine.CachedPitch == pitchDeg
            and NavEngine.CachedCFrame then
            return NavEngine.CachedCFrame
        end

        local halfX = primary.Size.X * 0.5
        local halfZ = primary.Size.Z * 0.5
        local rockHalfHeight = primary.Size.Y * 0.5
        local rockBottomY = rockPos.Y - rockHalfHeight
        local trueSubmerge = math.clamp(depth, 1.40, 2.75)

        local rayParams = RaycastParams.new()
        rayParams.FilterType = Enum.RaycastFilterType.Exclude
        rayParams.FilterDescendantsInstances = { workspace.Rocks, workspace.Living, State.Character or LocalPlayer.Character }
        rayParams.IgnoreWater = true

        -- Raycast from near the rock base (critical for tall crystals/icebergs so ledges/roofs above rockPos don't skew floorY)
        local rayStartY = math.min(rockPos.Y + 4, rockBottomY + 12)
        local centerHit = workspace:Raycast(Vector3.new(rockPos.X, rayStartY, rockPos.Z), Vector3.new(0, -45, 0), rayParams)
        local baseFloorY = (centerHit and centerHit.Position.Y) or rockBottomY

        local bestPos = nil
        local bestScore = -math.huge

        -- Evaluate 8 angles (0, 45, ..., 315 deg) with exact OBB face distance + local floor raycast
        for deg = 0, 315, 45 do
            local rad = math.rad(deg)
            local localDir = Vector3.new(math.cos(rad), 0, math.sin(rad))
            local worldDir = primary.CFrame:VectorToWorldSpace(localDir)
            local flatVec = Vector3.new(worldDir.X, 0, worldDir.Z)
            if flatVec.Magnitude > 0.01 then
                local flatDir = flatVec.Unit
                local objDir = primary.CFrame:VectorToObjectSpace(flatDir)
                local dx = math.abs(objDir.X) > 0.001 and (halfX / math.abs(objDir.X)) or 999
                local dz = math.abs(objDir.Z) > 0.001 and (halfZ / math.abs(objDir.Z)) or 999
                local faceRadius = math.min(dx, dz)
                local horizOffset = faceRadius + math.max(baseDist * 0.55, 0.45)

                local candX = rockPos.X + flatDir.X * horizOffset
                local candZ = rockPos.Z + flatDir.Z * horizOffset
                local floorHit = workspace:Raycast(Vector3.new(candX, rayStartY, candZ), Vector3.new(0, -45, 0), rayParams)
                local rawFloorY = (floorHit and floorHit.Position.Y) or baseFloorY
                -- Guard against steep slopes / cliff walls lifting localFloorY onto the side/top of tall rocks
                local isElevatedLedge = (rawFloorY > baseFloorY + 3.5)
                local localFloorY = isElevatedLedge and math.min(rawFloorY, baseFloorY + 2.0) or rawFloorY
                local minY = localFloorY - 2.75
                local maxY = math.max(minY, math.min(localFloorY - 1.10, rockBottomY - 0.85))
                local candY = math.clamp(localFloorY - trueSubmerge, minY, maxY)
                local candPos = Vector3.new(candX, candY, candZ)

                -- Check unobstructed line of sight to rock base/center (MineBlocked protection)
                local losTarget = Vector3.new(rockPos.X, math.min(rockPos.Y, rockBottomY + 3.0), rockPos.Z)
                local r1 = workspace:Raycast(candPos, losTarget - candPos, rayParams)
                local r2 = workspace:Raycast(losTarget, candPos - losTarget, rayParams)
                local losClear = (not r1 or r1.Instance:IsDescendantOf(rock)) and (not r2 or r2.Instance:IsDescendantOf(rock))

                local score = (losClear and 1000 or 0) + (isElevatedLedge and -250 or 200)
                if State.HRP then
                    local dPlayer = (Vector3.new(State.HRP.Position.X, candY, State.HRP.Position.Z) - candPos).Magnitude
                    score = score - dPlayer
                end

                if score > bestScore then
                    bestScore = score
                    bestPos = candPos
                end
            end
        end

        local sunkenPos = bestPos or Vector3.new(rockPos.X + halfX + 0.6, rockBottomY - trueSubmerge, rockPos.Z)
        local baseLook = CFrame.lookAt(sunkenPos, Vector3.new(rockPos.X, sunkenPos.Y, rockPos.Z))
        local finalCF = baseLook * CFrame.Angles(math.rad(pitchDeg), 0, 0)

        NavEngine.CachedRock = rock
        NavEngine.CachedDepth = depth
        NavEngine.CachedDist = baseDist
        NavEngine.CachedPitch = pitchDeg
        NavEngine.CachedCFrame = finalCF
        return finalCF
    elseif mode == "Rock Shield (Opposite Mobs)" or mode == "Adjacent Ground" then
        -- Positions on safe ground terrain facing rock; when mobs are near, positions behind the rock
        return NavEngine.GetSafeGroundCFrame(rock, 3.8)
    elseif mode == "Inside Rock (Pebbles Only)" then
        local rayParams = RaycastParams.new()
        rayParams.FilterType = Enum.RaycastFilterType.Exclude
        rayParams.FilterDescendantsInstances = { workspace.Rocks, State.Character or LocalPlayer.Character }
        rayParams.IgnoreWater = true

        local hit = workspace:Raycast(rockPos + Vector3.new(0, 8, 0), Vector3.new(0, -45, 0), rayParams)
        local groundY = hit and hit.Position.Y or (rockPos.Y - 2.5)

        local insidePos = Vector3.new(rockPos.X, groundY + 3.0, rockPos.Z)
        local lookDir = insidePos + (primary.CFrame.LookVector or Vector3.new(0, 0, 1))
        return CFrame.lookAt(insidePos, lookDir)
    else
        return NavEngine.GetSafeGroundCFrame(rock, 3.8)
    end
end

-- Resolves the real playable interior bounds of the active BossArena (excluding decorative trees/cliffs outside temple walls)
function NavEngine.GetBossPlayableBounds(arena, arenaCF, arenaSz)
    if not arena then return nil end
    local bSpawn = arena:FindFirstChild("BossSpawnLocation")
    local pSpawn = arena:FindFirstChild("PlayerSpawnLocation")
    if bSpawn and bSpawn:IsA("BasePart") and pSpawn and pSpawn:IsA("BasePart") then
        local bp, pp = bSpawn.Position, pSpawn.Position
        local minX = math.min(bp.X, pp.X) - 48
        local maxX = math.max(bp.X, pp.X) + 48
        local minZ = math.min(pp.Z, bp.Z) - 4
        local maxZ = math.max(pp.Z, bp.Z) + 6
        local barrier = arena:FindFirstChild("Barrier", true)
        if barrier and barrier:IsA("BasePart") and barrier.Position.Z > pp.Z then
            maxZ = math.max(minZ + 4, math.min(maxZ, barrier.Position.Z - 3.5))
        end
        local baseY = math.min(bp.Y, pp.Y)
        return { minX = minX, maxX = maxX, minZ = minZ, maxZ = maxZ, minY = baseY - 2.0, maxY = baseY + 38.0 }
    end
    if arenaCF and arenaSz then
        local hs = arenaSz * 0.5
        return {
            minX = arenaCF.Position.X - math.max(6, hs.X - 12),
            maxX = arenaCF.Position.X + math.max(6, hs.X - 12),
            minZ = arenaCF.Position.Z - math.max(6, hs.Z - 12),
            maxZ = arenaCF.Position.Z + math.max(6, hs.Z - 12),
            minY = arenaCF.Position.Y - hs.Y * 0.45,
            maxY = arenaCF.Position.Y + hs.Y * 0.45,
        }
    end
    return nil
end


-- Clamps a world position inside active BossArena boundaries so we never follow phasing bosses off the map
function NavEngine.ClampPosToBossArena(pos, isUnderMap)
    if not RaidEngine or type(RaidEngine.GetActiveBossArena) ~= "function" then return pos end
    local arena, arenaCF, arenaSz = RaidEngine.GetActiveBossArena()
    local b = arena and NavEngine.GetBossPlayableBounds(arena, arenaCF, arenaSz)
    if not b then return pos end
    local minY = isUnderMap and (b.minY - 20) or b.minY
    return Vector3.new(
        math.clamp(pos.X, b.minX, b.maxX),
        math.clamp(pos.Y, minY, b.maxY),
        math.clamp(pos.Z, b.minZ, b.maxZ)
    )
end

-- Returns safe standing HRP height above solid floor based on live HipHeight + HRP half-height
function NavEngine.GetCharacterStandOffset()
    local hip = (State.Humanoid and tonumber(State.Humanoid.HipHeight)) or 2.0
    local halfH = (State.HRP and State.HRP.Size.Y * 0.5) or 1.0
    return math.max(3.35, hip + halfH + 0.35)
end

-- Raycasts downward/along dir skipping non-collidable (CanCollide = false) decorative/trigger parts
function NavEngine.RaycastSolidFloor(origin, direction, extraExclude)
    local excludes = {
        workspace:FindFirstChild("Living"),
        workspace:FindFirstChild("Rocks"),
        workspace:FindFirstChild("Debris"),
        State.Character or LocalPlayer.Character,
    }
    if type(extraExclude) == "table" then
        for _, inst in ipairs(extraExclude) do
            table.insert(excludes, inst)
        end
    end
    local rp = RaycastParams.new()
    rp.FilterType = Enum.RaycastFilterType.Exclude
    rp.FilterDescendantsInstances = excludes
    rp.IgnoreWater = true
    for _ = 1, 10 do
        local hit = workspace:Raycast(origin, direction, rp)
        if not hit then return nil end
        local inst = hit.Instance
        if inst and (inst:IsA("Terrain") or (inst:IsA("BasePart") and inst.CanCollide)) then
            return hit
        end
        table.insert(excludes, inst)
        rp.FilterDescendantsInstances = excludes
    end
    return nil
end


-- Prevents the player from ever falling into the void or walking out of bounds in BossArenas
function NavEngine.PreventBossArenaVoidFall()
    if not State.HRP or not RaidEngine or type(RaidEngine.GetActiveBossArena) ~= "function" then return false end
    local arena, arenaCF, arenaSz = RaidEngine.GetActiveBossArena()
    local b = arena and NavEngine.GetBossPlayableBounds(arena, arenaCF, arenaSz)
    if not b then return false end
    local pos = State.HRP.Position
    local isIntentionalUnder = (Config.MobFarmPositionMode == "Under Map")
        and (Config.AutoMobFarm or Config.AutoRaid)
        and CombatEngine.ActiveStanceCF ~= nil
    local fellLow = not isIntentionalUnder and (pos.Y < (b.minY - 5.0))
    local outXZ = (not CombatEngine.ActiveStanceCF)
        and (pos.X < b.minX - 6 or pos.X > b.maxX + 6 or pos.Z < b.minZ - 6 or pos.Z > b.maxZ + 6)
    if fellLow or outXZ then
        CombatEngine.ActiveStanceCF = nil
        NavEngine.RestoreCharacterPhysics()
        local standOff = NavEngine.GetCharacterStandOffset()
        local midX, midZ = (b.minX + b.maxX) * 0.5, (b.minZ + b.maxZ) * 0.5
        local loX, hiX = math.min(b.minX + 6, midX), math.max(b.maxX - 6, midX)
        local loZ, hiZ = math.min(b.minZ + 6, midZ), math.max(b.maxZ - 6, midZ)
        local safeX = math.clamp(pos.X, loX, hiX)
        local safeZ = math.clamp(pos.Z, loZ, hiZ)
        local safeY = b.minY + standOff + 0.5
        local pSpawn = arena:FindFirstChild("PlayerSpawnLocation")
        if pSpawn and pSpawn:IsA("BasePart") then
            safeY = pSpawn.Position.Y + standOff + 0.5
        end
        local hit = NavEngine.RaycastSolidFloor(Vector3.new(safeX, safeY + 25, safeZ), Vector3.new(0, -50, 0))
        if hit and hit.Position and hit.Position.Y >= (b.minY - 2.0) then
            safeY = hit.Position.Y + standOff + 0.45
        elseif pSpawn and pSpawn:IsA("BasePart") then
            safeX, safeY, safeZ = pSpawn.Position.X, pSpawn.Position.Y + standOff + 0.5, pSpawn.Position.Z
        end
        if State.HRP.Anchored then State.HRP.Anchored = false end
        State.HRP.CFrame = CFrame.new(safeX, safeY, safeZ)
        State.HRP.AssemblyLinearVelocity = Vector3.zero
        State.HRP.AssemblyAngularVelocity = Vector3.zero
        return true
    end
    return false
end

-- Computes target CFrame for Auto Farm Mobs ("Above Enemy", "Behind Enemy", or "Under Map")
function NavEngine.GetMobFarmCFrame(mob, mode)
    mode = mode or Config.MobFarmPositionMode or "Above Enemy"
    local isBehind = (mode == "Behind Enemy")
    local isUnderMap = (mode == "Under Map" or mode == "Underground" or mode == "Underneath Enemy")
    local mHRP = mob and (mob:FindFirstChild("HumanoidRootPart") or mob:FindFirstChildWhichIsA("BasePart"))
    if not mHRP then return nil end
    local mobPos = mHRP.Position

    local rayParams = RaycastParams.new()
    rayParams.FilterType = Enum.RaycastFilterType.Exclude
    rayParams.FilterDescendantsInstances = {
        workspace:FindFirstChild("Living"),
        workspace:FindFirstChild("Rocks"),
        workspace:FindFirstChild("Debris"),
        State.Character or LocalPlayer.Character,
    }
    rayParams.IgnoreWater = true

    local rawPos, rawLook
    if isBehind then
        local isKitingTarget = CombatEngine and type(CombatEngine.IsKitingMob) == "function" and CombatEngine.IsKitingMob(mob)
        local velXZ = Vector3.new(mHRP.AssemblyLinearVelocity.X, 0, mHRP.AssemblyLinearVelocity.Z)
        local movingFast = velXZ.Magnitude > 3.5
        -- Lead kiting/fleeing mobs (e.g. Abyssal Watcher) slightly ahead of their movement vector so we intercept instead of tail-chasing
        local focusPos = (isKitingTarget and movingFast)
            and (mobPos + velXZ.Unit * math.clamp(velXZ.Magnitude * 0.22, 1.5, 3.8))
            or mobPos

        local backFlat = Vector3.new(-mHRP.CFrame.LookVector.X, 0, -mHRP.CFrame.LookVector.Z)
        if isKitingTarget and movingFast then
            backFlat = Vector3.new(-velXZ.Z, 0, velXZ.X).Unit -- Flank perpendicular to kite vector
        elseif backFlat.Magnitude < 0.05 then
            backFlat = Vector3.new(0, 0, 1)
        end
        backFlat = backFlat.Unit

        local losParams = RaycastParams.new()
        losParams.FilterType = Enum.RaycastFilterType.Exclude
        losParams.FilterDescendantsInstances = {
            workspace:FindFirstChild("Living"),
            workspace:FindFirstChild("Debris"),
            State.Character or LocalPlayer.Character,
        }
        losParams.IgnoreWater = true

        -- Stay >= 2.4 studs outside mob center (when unobstructed) to prevent CombatInside flags
        local desiredDist = math.max(math.clamp(Config.MobFarmDistance or 4.0, 2.4, 10.0), 2.4)
        if isKitingTarget then
            desiredDist = math.min(desiredDist, 2.8)
        end
        local bestPos = nil

        for _, deg in ipairs({ 0, 50, -50, 95, -95, 140, -140 }) do
            local rad = math.rad(deg)
            local cosA, sinA = math.cos(rad), math.sin(rad)
            local dir = Vector3.new(backFlat.X * cosA - backFlat.Z * sinA, 0, backFlat.X * sinA + backFlat.Z * cosA).Unit

            local actualDist = desiredDist
            local wallHit = workspace:Raycast(Vector3.new(focusPos.X, focusPos.Y + 1.0, focusPos.Z), dir * (desiredDist + 1.4), losParams)
            if wallHit and wallHit.Distance then
                actualDist = math.max(1.8, wallHit.Distance - 1.4)
            end

            local candX = focusPos.X + dir.X * actualDist
            local candZ = focusPos.Z + dir.Z * actualDist
            local floorHit = NavEngine.RaycastSolidFloor(Vector3.new(candX, focusPos.Y + 5.0, candZ), Vector3.new(0, -25, 0))
            local validFloor = floorHit and floorHit.Position and floorHit.Position.Y >= (focusPos.Y - 12.0)
            local standY = validFloor and math.clamp(floorHit.Position.Y + NavEngine.GetCharacterStandOffset(), focusPos.Y - 4.0, focusPos.Y + 5.5) or focusPos.Y
            local candPos = Vector3.new(candX, standY, candZ)
            local arenaClamped = NavEngine.ClampPosToBossArena(candPos, false)
            local insidePlayable = (Vector3.new(arenaClamped.X - candPos.X, 0, arenaClamped.Z - candPos.Z).Magnitude < 1.5)

            local losHit = workspace:Raycast(candPos, Vector3.new(mobPos.X, standY, mobPos.Z) - candPos, losParams)
            if validFloor and insidePlayable and not losHit then
                bestPos = candPos
                break
            elseif (validFloor or not bestPos) and insidePlayable then
                bestPos = candPos
            end
        end

        rawPos = bestPos or Vector3.new(focusPos.X + backFlat.X * desiredDist, focusPos.Y, focusPos.Z + backFlat.Z * desiredDist)
        rawLook = Vector3.new(mobPos.X, rawPos.Y, mobPos.Z)
    elseif isUnderMap then
        local underDepth = math.clamp(Config.MobFarmUnderOffset or 6.0, 2.5, 14.0)
        local stanceY = mobPos.Y - underDepth
        local floorHit = NavEngine.RaycastSolidFloor(Vector3.new(mobPos.X, mobPos.Y + 1.0, mobPos.Z), Vector3.new(0, -16, 0))
        if floorHit and floorHit.Position then
            stanceY = math.min(stanceY, floorHit.Position.Y - 2.2)
            local maxReach = math.max(underDepth + 2.5, 8.5)
            if (mobPos.Y - stanceY) > maxReach then
                stanceY = mobPos.Y - maxReach
            end
        end
        rawPos = Vector3.new(mobPos.X, stanceY, mobPos.Z)
        rawLook = Vector3.new(mobPos.X + 0.05, mobPos.Y, mobPos.Z)
    else
        -- Above Enemy: find true top of mob model
        local mobTopY = mobPos.Y + 2.2
        local head = mob:FindFirstChild("Head")
        if head and head:IsA("BasePart") then
            mobTopY = math.max(mobTopY, head.Position.Y + (head.Size.Y * 0.5))
        else
            for _, p in ipairs(mob:GetDescendants()) do
                if p:IsA("BasePart") and p.Name ~= "HumanoidRootPart" then
                    mobTopY = math.max(mobTopY, p.Position.Y + (p.Size.Y * 0.5))
                end
            end
        end

        local offset = math.clamp(Config.MobFarmVerticalOffset or 5.5, 2.5, 12.0)
        local candY = math.max(mobPos.Y + offset, mobTopY + 1.4)

        -- Respect cave ceilings so player never clips inside overhead rock roofs
        local ceilParams = RaycastParams.new()
        ceilParams.FilterType = Enum.RaycastFilterType.Exclude
        ceilParams.FilterDescendantsInstances = {
            workspace:FindFirstChild("Living"),
            workspace:FindFirstChild("Debris"),
            State.Character or LocalPlayer.Character,
        }
        ceilParams.IgnoreWater = true
        local ceilHit = workspace:Raycast(Vector3.new(mobPos.X, mobPos.Y + 1.5, mobPos.Z), Vector3.new(0, (candY - mobPos.Y) + 2.5, 0), ceilParams)
        if ceilHit and ceilHit.Position then
            candY = math.min(candY, ceilHit.Position.Y - 2.3)
        end

        rawPos = Vector3.new(mobPos.X, candY, mobPos.Z)
        rawLook = Vector3.new(mobPos.X + 0.05, mobPos.Y, mobPos.Z)
    end

    local voidMinY = (workspace.FallenPartsDestroyHeight or -500) + 35
    if rawPos.Y < voidMinY then
        rawPos = Vector3.new(rawPos.X, voidMinY, rawPos.Z)
    end
    local safePos = NavEngine.ClampPosToBossArena(rawPos, isUnderMap)
    if safePos ~= rawPos and isBehind then
        rawLook = Vector3.new(mobPos.X, safePos.Y, mobPos.Z)
    end
    return CFrame.lookAt(safePos, rawLook)
end

-- Lifts the character back onto solid ground if currently positioned beneath a floor (e.g. exiting Under Map stance)
function NavEngine.SurfaceIfUnderground()
    if not State.HRP then return end
    local pos = State.HRP.Position
    local belowHit = NavEngine.RaycastSolidFloor(pos + Vector3.new(0, 0.5, 0), Vector3.new(0, -18, 0))
    if belowHit then return end
    local topHit = NavEngine.RaycastSolidFloor(Vector3.new(pos.X, pos.Y + 26, pos.Z), Vector3.new(0, -28, 0))
    if topHit and topHit.Position and topHit.Position.Y > pos.Y - 1.0 then
        NavEngine.RestoreCharacterPhysics()
        if State.HRP.Anchored then State.HRP.Anchored = false end
        State.HRP.CFrame = CFrame.new(pos.X, topHit.Position.Y + NavEngine.GetCharacterStandOffset() + 0.4, pos.Z)
        State.HRP.AssemblyLinearVelocity = Vector3.zero
        State.HRP.AssemblyAngularVelocity = Vector3.zero
    end
end

-- Computes safe retreat CFrame away from Boss/Asura skill attacks while staying inside active Boss Arena bounds
function NavEngine.GetBossSkillRetreatCFrame(mob, safeDist, mode)
    local mHRP = mob and (mob:FindFirstChild("HumanoidRootPart") or mob:FindFirstChildWhichIsA("BasePart"))
    if not mHRP then return nil end
    mode = mode or Config.MobFarmPositionMode or "Above Enemy"
    safeDist = math.clamp(tonumber(safeDist) or Config.BossSkillSafeDistance or 50.0, 22, 95)

    local mobPos = mHRP.Position
    local backVec = Vector3.new(-mHRP.CFrame.LookVector.X, 0, -mHRP.CFrame.LookVector.Z)
    if State.HRP then
        local fromBoss = Vector3.new(State.HRP.Position.X - mobPos.X, 0, State.HRP.Position.Z - mobPos.Z)
        if fromBoss.Magnitude > 8 then
            backVec = fromBoss.Unit
        end
    end
    if backVec.Magnitude < 0.05 then backVec = Vector3.new(0, 0, 1) end
    backVec = backVec.Unit

    local arena, arenaCF, arenaSz = nil, nil, nil
    if RaidEngine and type(RaidEngine.GetActiveBossArena) == "function" then
        arena, arenaCF, arenaSz = RaidEngine.GetActiveBossArena()
    end
    local bounds = arena and NavEngine.GetBossPlayableBounds(arena, arenaCF, arenaSz) or nil

    local rp = RaycastParams.new()
    rp.FilterType = Enum.RaycastFilterType.Exclude
    rp.FilterDescendantsInstances = {
        workspace:FindFirstChild("Living"),
        workspace:FindFirstChild("Rocks"),
        workspace:FindFirstChild("Debris"),
        State.Character or LocalPlayer.Character,
    }
    rp.IgnoreWater = true

    local bestXZ = nil
    local angles = { 0, 40, -40, 80, -80, 125, -125, 170 }
    for _, deg in ipairs(angles) do
        local rad = math.rad(deg)
        local c, s = math.cos(rad), math.sin(rad)
        local dir = Vector3.new(backVec.X * c - backVec.Z * s, 0, backVec.X * s + backVec.Z * c).Unit
        local cx, cz = mobPos.X + dir.X * safeDist, mobPos.Z + dir.Z * safeDist
        local insideArena = true
        if bounds then
            if cx < bounds.minX or cx > bounds.maxX or cz < bounds.minZ or cz > bounds.maxZ then
                insideArena = false
            end
        end
        if insideArena then
            bestXZ = Vector3.new(cx, mobPos.Y, cz)
            break
        elseif not bestXZ then
            bestXZ = Vector3.new(cx, mobPos.Y, cz)
        end
    end

    if bounds and bestXZ then
        bestXZ = Vector3.new(
            math.clamp(bestXZ.X, bounds.minX, bounds.maxX),
            mobPos.Y,
            math.clamp(bestXZ.Z, bounds.minZ, bounds.maxZ)
        )
    end
    bestXZ = bestXZ or Vector3.new(mobPos.X + backVec.X * safeDist, mobPos.Y, mobPos.Z + backVec.Z * safeDist)

    local floorHit = NavEngine.RaycastSolidFloor(Vector3.new(bestXZ.X, mobPos.Y + 12, bestXZ.Z), Vector3.new(0, -35, 0))
    local stanceY = mobPos.Y
    if mode == "Under Map" or mode == "Underground" or mode == "Underneath Enemy" then
        local underDepth = math.clamp(Config.MobFarmUnderOffset or 6.0, 2.5, 14.0)
        stanceY = mobPos.Y - underDepth
        if floorHit and floorHit.Position then
            stanceY = math.min(stanceY, floorHit.Position.Y - 2.2)
            local maxReach = math.max(underDepth + 2.5, 8.5)
            if (mobPos.Y - stanceY) > maxReach then
                stanceY = mobPos.Y - maxReach
            end
        end
    elseif mode == "Behind Enemy" then
        stanceY = floorHit and (floorHit.Position.Y + NavEngine.GetCharacterStandOffset()) or mobPos.Y
    else
        stanceY = mobPos.Y + math.clamp(Config.MobFarmVerticalOffset or 5.5, 2.5, 12.0)
        if floorHit and floorHit.Position then
            stanceY = math.max(stanceY, floorHit.Position.Y + 4.5)
        end
        if arenaCF and arenaSz then
            stanceY = math.min(stanceY, arenaCF.Position.Y + (arenaSz.Y * 0.45))
        end
    end

    local finalPos = Vector3.new(bestXZ.X, stanceY, bestXZ.Z)
    return CFrame.lookAt(finalPos, Vector3.new(mobPos.X + 0.05, finalPos.Y, mobPos.Z))
end

-- Disables local collision on all rocks in workspace.Rocks so boulders/rocks never wedge the player
function NavEngine.DisableAllRocksCollision()
    local rocksFolder = workspace:FindFirstChild("Rocks")
    if not rocksFolder then return end
    local function disableRecursive(container)
        for _, ch in ipairs(container:GetChildren()) do
            if ch:IsA("Model") and (ch:GetAttribute("Health") ~= nil or ch:FindFirstChild("Hitbox") or ch:GetAttribute("RequiredDamage") ~= nil) then
                for _, p in ipairs(ch:GetDescendants()) do
                    if p:IsA("BasePart") and p.CanCollide then
                        p.CanCollide = false
                    end
                end
            elseif ch:IsA("Folder") or ch:IsA("BasePart") or ch:IsA("Model") then
                disableRecursive(ch)
            end
        end
    end
    disableRecursive(rocksFolder)
end

function NavEngine.IsSecretMazeWorld()
    local rocks = workspace:FindFirstChild("Rocks")
    if rocks and (rocks:FindFirstChild("Island3MazeStart") or rocks:FindFirstChild("Island3MazeEnd")) then
        return true
    end
    local reg = workspace:FindFirstChild("Debris") and workspace.Debris:FindFirstChild("Regions")
    return reg ~= nil and reg:FindFirstChild("Island3Maze") ~= nil
end

function NavEngine.IsOnSecretMazeSide(pos)
    return pos ~= nil and pos.X > -500 and pos.X < 2000 and pos.Z < -1200
end

function NavEngine.IsInSecretMazeInterior(pos)
    return NavEngine.IsOnSecretMazeSide(pos) and pos.Y < -55
end

function NavEngine.IsOnMainFrostspireSide(pos)
    return pos ~= nil and pos.X > -3000 and pos.Z > -1000
end

local CORE_COLLIDE_PARTS = {
    HumanoidRootPart = true,
    Torso = true,
    UpperTorso = true,
    LowerTorso = true,
    Head = true,
}

function NavEngine.RestoreCharacterPhysics()
    NavEngine.IsGliding = false
    if CombatEngine then
        CombatEngine.HoverLockActive = false
    end
    if State.HRP then
        local hv = State.HRP:FindFirstChild("AetherHoverLV")
        if hv then pcall(function() hv:Destroy() end) end
        local ha = State.HRP:FindFirstChild("AetherHoverAtt")
        if ha then pcall(function() ha:Destroy() end) end
    end
    if State.Humanoid then
        pcall(function()
            State.Humanoid.PlatformStand = false
            State.Humanoid.AutoRotate = true
            local st = State.Humanoid:GetState()
            if st == Enum.HumanoidStateType.PlatformStanding or st == Enum.HumanoidStateType.Physics then
                State.Humanoid:ChangeState(Enum.HumanoidStateType.GettingUp)
            end
        end)
    end
    if not Config.Noclip and State.Character then
        local saved = NavEngine.SavedCollide
        for _, part in ipairs(State.Character:GetDescendants()) do
            if part:IsA("BasePart") then
                local wantCollide = CORE_COLLIDE_PARTS[part.Name] and true or ((saved and saved[part] ~= nil) and saved[part] or nil)
                if wantCollide ~= nil then
                    pcall(function() part.CanCollide = wantCollide end)
                end
            end
        end
    end
end

-- Recover immediately on script load in case a previous run left the character in PlatformStand / Noclip
pcall(NavEngine.RestoreCharacterPhysics)

-- Internal single-leg glide (respects shared ActiveGlideId mutex, Debris exclusion, and Secret Maze ceiling clamp)
function NavEngine.DirectGlideTo(targetCFrame, speed, forceComplete, sharedGlideId, minYOverride, maxYOverride, allowPreempt)
    if not State.HRP or not targetCFrame then return false end
    local myGlideId = sharedGlideId
    if not myGlideId then
        NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
        myGlideId = NavEngine.ActiveGlideId
    end
    speed = speed or Config.GlideSpeed or 40
    local startPos = State.HRP.Position
    local targetPos = targetCFrame.Position
    local dist = (targetPos - startPos).Magnitude

    if State.HRP.Anchored then
        State.HRP.Anchored = false
    end

    if dist <= 2.8 then
        if NavEngine.ActiveGlideId == myGlideId then
            NavEngine.RestoreCharacterPhysics()
        end
        if State.HRP then
            State.HRP.CFrame = targetCFrame
            State.HRP.AssemblyLinearVelocity = Vector3.zero
            State.HRP.AssemblyAngularVelocity = Vector3.zero
        end
        return true
    end

    -- Snapshot original character part collisions before disabling them (only if not already actively gliding/noclipping)
    if State.Character and not NavEngine.IsGliding and not Config.Noclip then
        local saved = {}
        local hadSolidCore = false
        for _, part in ipairs(State.Character:GetDescendants()) do
            if part:IsA("BasePart") then
                saved[part] = part.CanCollide
                if CORE_COLLIDE_PARTS[part.Name] and part.CanCollide then
                    hadSolidCore = true
                end
            end
        end
        if not hadSolidCore then
            for _, part in ipairs(State.Character:GetDescendants()) do
                if part:IsA("BasePart") and CORE_COLLIDE_PARTS[part.Name] then
                    saved[part] = true
                end
            end
        end
        NavEngine.SavedCollide = saved
    end

    NavEngine.IsGliding = true
    -- Put Humanoid into PlatformStand during glide so Terrain/mountain ground-snap never fights descent
    if State.Humanoid then
        pcall(function() State.Humanoid.PlatformStand = true end)
    end
    local function finishGlide(result)
        -- Only the currently active glide may clear IsGliding/PlatformStand (prevents prevPlatformStand race on preempted glides)
        if NavEngine.ActiveGlideId == myGlideId then
            NavEngine.RestoreCharacterPhysics()
        end
        return result
    end

    -- Disable local rock collision so boulders/rocks never block movement
    pcall(NavEngine.DisableAllRocksCollision)

    local rayParams = RaycastParams.new()
    rayParams.FilterType = Enum.RaycastFilterType.Exclude
    local filterList = { workspace.Rocks, workspace.Living, State.Character or LocalPlayer.Character }
    local debrisFolder = workspace:FindFirstChild("Debris")
    if debrisFolder then
        table.insert(filterList, debrisFolder)
    end
    local shopsFolder = workspace:FindFirstChild("Shops")
    if shopsFolder then
        table.insert(filterList, shopsFolder)
    end
    rayParams.FilterDescendantsInstances = filterList
    rayParams.IgnoreWater = true

    local minSafeY = Config.MinTweenY or -20
    local isLongCrossMap = (dist > 45)
    local inMazeInterior = NavEngine.IsSecretMazeWorld()
        and NavEngine.IsInSecretMazeInterior(startPos)
        and NavEngine.IsInSecretMazeInterior(targetPos)

    local transitMinY
    local transitMaxY = maxYOverride
    if minYOverride ~= nil then
        transitMinY = minYOverride
    elseif inMazeInterior then
        -- Never force player upward out of the Secret Maze into WaterDamage (-53..+9) or TeleportPart3 (-26..-15)
        transitMinY = math.max(-145, math.min(startPos.Y, targetPos.Y) - 2.0)
        transitMaxY = transitMaxY or -95
    elseif isLongCrossMap then
        transitMinY = math.max(minSafeY, math.min(startPos.Y, targetPos.Y) + 3.0)
    else
        transitMinY = math.min(minSafeY, targetPos.Y)
    end

    local curPos = startPos
    local lastProgressCheck = os.clock()
    local lastProgressPos = startPos
    local stallCount = 0
    local startTime = os.clock()
    local maxTime = (dist / math.max(10, speed)) * 1.8 + 15.0

    local startChar = State.Character
    local startRock = State.TargetRock
    local startMob = (not startRock) and State.TargetMob or nil
    local lastClosestScan = os.clock()
    while ScriptRunning and State.HRP and (os.clock() - startTime) < maxTime do
        -- Ensure only the latest GlideTo controls HRP.CFrame and abort immediately if player died or respawned
        if NavEngine.ActiveGlideId ~= myGlideId or State.Character ~= startChar or not State.Humanoid or State.Humanoid.Health <= 0 then
            return finishGlide(false)
        end
        -- Abort immediately if the owning feature was disabled mid-glide (even for forceComplete legs)
        if State.IsAutoForging and not Config.AutoForgeArmor and not State.ManualDrainOnce and not Config.WeaponHuntRunning then
            return finishGlide(false)
        end
        if State.IsCrafting and not Config.AutoCraft then
            return finishGlide(false)
        end
        if State.IsBuyingPotions and not Config.AutoBuyPotions then
            return finishGlide(false)
        end
        if State.IsBuyingHealthPotions and RaidEngine and (RaidEngine.IsInActiveRun() or RaidEngine.GetActiveBossArena() ~= nil or os.clock() < (RaidEngine.ArenaLoadingUntil or 0)) then
            return finishGlide(false)
        end
        -- Abort if AutoMine/AutoMobFarm was turned off, target changed, or a higher-priority routine (potions/forge) took over
        if not forceComplete then
            if State.IsBuyingPotions or State.CheckingPotionsOnStart or State.IsUsingPotion or State.UsingPotionCycle or State.IsAutoForging or State.DrainingFullBag or State.IsCrafting then
                return finishGlide(false)
            end
            if startRock and not Config.AutoMine then
                return finishGlide(false)
            end
            if startMob and not Config.AutoMobFarm and not Config.AutoRaid then
                return finishGlide(false)
            end
            if not Config.AutoMine and not Config.AutoMobFarm and not Config.AutoRaid then
                break
            end
            if startRock and State.TargetRock ~= startRock then
                break
            end
            if startMob and State.TargetMob ~= startMob then
                break
            end
            -- Closer-Rock Preemption (only when safe & allowed for this leg)
            if allowPreempt ~= false and startRock and not State.ResumingAfterDeath and Config.AutoMine and MiningEngine and MiningEngine.FindBestRock then
                local nowScan = os.clock()
                local curRem = (targetPos - State.HRP.Position).Magnitude
                if curRem > 22 and (nowScan - lastClosestScan) >= 0.40 then
                    lastClosestScan = nowScan
                    local closerRock, closerDist = MiningEngine.FindBestRock()
                    local allowTier = (Config.PrioritizeStrongestRock == false)
                        or (MiningEngine.GetRockStrengthScore(closerRock) >= MiningEngine.GetRockStrengthScore(startRock))
                    if closerRock and closerRock ~= startRock and allowTier and closerDist and (closerDist + 8) < curRem then
                        State.TargetRock = closerRock
                        State.CurrentRock = closerRock
                        State.LastMinedRock = closerRock
                        NavEngine.CachedRock = nil
                        MiningEngine.TargetLastHp = closerRock:GetAttribute("Health")
                        MiningEngine.TargetLastDamageTime = nowScan
                        return finishGlide(false)
                    end
                end
            end
        end

        local dt = RunService.Heartbeat:Wait()
        if not State.HRP then return finishGlide(false) end

        -- Keep character limbs non-collidable during glide so we never get wedged between walls/rocks
        if State.Character then
            for _, part in ipairs(State.Character:GetDescendants()) do
                if part:IsA("BasePart") and part.CanCollide then
                    part.CanCollide = false
                end
            end
        end

        -- Sync curPos if server rolled us back or pushed us
        local actualPos = State.HRP.Position
        if (actualPos - curPos).Magnitude > 14 then
            curPos = actualPos
        end

        local toTarget = targetPos - curPos
        local remDist = toTarget.Magnitude
        if remDist <= 2.4 then
            finishGlide(true)
            if State.HRP then
                State.HRP.CFrame = targetCFrame
                State.HRP.AssemblyLinearVelocity = Vector3.zero
                State.HRP.AssemblyAngularVelocity = Vector3.zero
            end
            State.ResumingAfterDeath = false
            return true
        end

        -- Stall / wedge detection every 0.70s
        local now = os.clock()
        if now - lastProgressCheck >= 0.70 then
            local moved = (actualPos - lastProgressPos).Magnitude
            lastProgressCheck = now
            lastProgressPos = actualPos
            if moved < 1.2 then
                stallCount = stallCount + 1
                if targetPos.Y < (actualPos.Y - 6.0) then
                    local flatDir = Vector3.new(targetPos.X - actualPos.X, 0, targetPos.Z - actualPos.Z)
                    local push = (flatDir.Magnitude > 0.5) and (flatDir.Unit * 5.0) or Vector3.new(5.0, 0, 0)
                    curPos = actualPos + push + Vector3.new(0, -3.0, 0)
                else
                    local liftPos = actualPos + Vector3.new(0, 4.5, 0)
                    if transitMaxY and liftPos.Y > transitMaxY then
                        liftPos = Vector3.new(liftPos.X, transitMaxY, liftPos.Z)
                    end
                    curPos = liftPos
                end
                if stallCount >= 2 and not forceComplete then
                    return finishGlide(false)
                end
            else
                stallCount = 0
            end
        end

        local stepDist = math.min(remDist, speed * dt)
        local nextPos = curPos + toTarget.Unit * stepDist
        local horizRem = Vector3.new(targetPos.X - nextPos.X, 0, targetPos.Z - nextPos.Z).Magnitude

        -- Enforce Minimum/Maximum Safe Y-Level & Ground Floor Clearance while in horizontal transit
        if horizRem > 4.5 then
            if nextPos.Y < transitMinY then
                nextPos = Vector3.new(nextPos.X, transitMinY, nextPos.Z)
            end
            if transitMaxY and nextPos.Y > transitMaxY then
                nextPos = Vector3.new(nextPos.X, transitMaxY, nextPos.Z)
            end
            -- Raycast solid local floor so cross-map tween rides safely above ground, but allow smooth descent off high mountains
            local floorHit = NavEngine.RaycastSolidFloor(Vector3.new(nextPos.X, nextPos.Y + 15, nextPos.Z), Vector3.new(0, -35, 0))
            if floorHit and nextPos.Y < (floorHit.Position.Y + 3.4) and (floorHit.Position.Y - nextPos.Y) < 14 then
                local descendingFromMountain = (targetPos.Y < nextPos.Y - 12.0) and (floorHit.Position.Y > targetPos.Y + 12.0) and (horizRem < 40)
                if not descendingFromMountain then
                    local safeFloorY = floorHit.Position.Y + NavEngine.GetCharacterStandOffset()
                    if transitMaxY and safeFloorY > transitMaxY then
                        safeFloorY = transitMaxY
                    end
                    nextPos = Vector3.new(nextPos.X, safeFloorY, nextPos.Z)
                end
            end
        end

        curPos = nextPos
        local lookAtPos = Vector3.new(targetPos.X, curPos.Y, targetPos.Z)
        if (lookAtPos - curPos).Magnitude > 0.1 then
            State.HRP.CFrame = CFrame.lookAt(curPos, lookAtPos)
        else
            State.HRP.CFrame = CFrame.new(curPos)
        end
        State.HRP.AssemblyLinearVelocity = Vector3.zero
        State.HRP.AssemblyAngularVelocity = Vector3.zero
    end

    return finishGlide(false)
end

-- Smooth glide navigation with Secret Maze Entrance Routing (avoids WaterDamage & TeleportPart3/1),
-- Waypoint shortcuts, Minimum Safe Y-Level protection, Anti-Wedging Noclip, and Stall Escape
function NavEngine.GlideTo(targetCFrame, speed, forceComplete)
    if not State.HRP or not targetCFrame then return false end
    NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
    local myGlideId = NavEngine.ActiveGlideId
    speed = speed or Config.GlideSpeed or 40

    local startPos = State.HRP.Position
    local targetPos = targetCFrame.Position

    -- Secret Maze <-> Main Island smart multi-hop routing (prevents flying underwater or hitting TeleportPart3)
    if NavEngine.IsSecretMazeWorld() and (startPos - targetPos).Magnitude > 60 then
        local startMain = NavEngine.IsOnMainFrostspireSide(startPos)
        local targetMaze = NavEngine.IsOnSecretMazeSide(targetPos)
        local startMaze = NavEngine.IsOnSecretMazeSide(startPos)
        local targetMain = NavEngine.IsOnMainFrostspireSide(targetPos)

        local startChar = State.Character
        local function routeCancelled()
            if NavEngine.ActiveGlideId ~= myGlideId or not State.HRP or State.Character ~= startChar or not State.Humanoid or State.Humanoid.Health <= 0 or not ScriptRunning then
                return true
            end
            if not forceComplete then
                if State.IsBuyingPotions or State.CheckingPotionsOnStart or State.IsUsingPotion or State.UsingPotionCycle or State.IsAutoForging or State.DrainingFullBag or State.IsCrafting then
                    return true
                end
                if not Config.AutoMine and not Config.AutoMobFarm and not Config.AutoRaid then
                    return true
                end
            end
            return false
        end

        if startMain and targetMaze then
            -- 1. If Ferry waypoint is unlocked and far away on Main Island, warp right to Ferry pier first (~1s)
            if TeleportEngine and type(TeleportEngine.IsWaypointUnlocked) == "function"
                and (startPos - Vector3.new(242, 30, -303)).Magnitude > 140
                and TeleportEngine.IsWaypointUnlocked("Ferry") then
                TeleportEngine.TeleportToWaypoint("Ferry")
                if routeCancelled() then return false end
                startPos = State.HRP and State.HRP.Position or startPos
            end

            -- 2. Fly high (+36Y, strictly above WaterDamage +9.34 and West of TeleportPart3) to Secret Maze Entrance Dock -> Shaft -> Corridor
            if NavEngine.IsOnMainFrostspireSide(startPos) then
                local legs = {}
                if startPos.Z > -420 then
                    table.insert(legs, { cf = CFrame.new(212, 38, -385), minY = 28 })
                end
                table.insert(legs, { cf = CFrame.new(210, 26, -1680), minY = 26 })
                table.insert(legs, { cf = CFrame.new(200, 16, -1765), minY = 12 })
                table.insert(legs, { cf = CFrame.new(192, 14, -1815), minY = 10 })
                if targetPos.Y < -55 then
                    table.insert(legs, { cf = CFrame.new(192, -120, -1815), minY = -130, maxY = 22 })
                    table.insert(legs, { cf = CFrame.new(451, -128, -1845), minY = -142, maxY = -95 })
                end
                for _, leg in ipairs(legs) do
                    if routeCancelled() then return false end
                    local okLeg = NavEngine.DirectGlideTo(leg.cf, speed, forceComplete, myGlideId, leg.minY, leg.maxY, false)
                    if not okLeg then return false end
                end
            elseif NavEngine.IsOnSecretMazeSide(startPos) and startPos.Y >= -55 and targetPos.Y < -55 then
                -- Already on Maze island dock/surface; route down Entrance Shaft first
                local descLegs = {
                    { cf = CFrame.new(200, 16, -1765), minY = 12 },
                    { cf = CFrame.new(192, 14, -1815), minY = 10 },
                    { cf = CFrame.new(192, -120, -1815), minY = -130, maxY = 22 },
                    { cf = CFrame.new(451, -128, -1845), minY = -142, maxY = -95 },
                }
                for _, leg in ipairs(descLegs) do
                    if routeCancelled() then return false end
                    local okLeg = NavEngine.DirectGlideTo(leg.cf, speed, forceComplete, myGlideId, leg.minY, leg.maxY, false)
                    if not okLeg then return false end
                end
            end

            if routeCancelled() then return false end
            return NavEngine.DirectGlideTo(targetCFrame, speed, forceComplete, myGlideId, nil, (targetPos.Y < -55) and -95 or nil, true)
        elseif startMaze and targetMain then
            -- 1. Try instant Main Island Waypoint warp (Village is free & unlocked -> ~1.5s return instead of 35s!)
            if TeleportEngine and type(TeleportEngine.IsWaypointUnlocked) == "function" then
                local bestWp, bestWpDist = nil, math.huge
                local wpFolder = workspace:FindFirstChild("Waypoints")
                if wpFolder then
                    for _, wpName in ipairs({ "Village", "Ferry", "Spider Cave", "The Peak" }) do
                        local inst = wpFolder:FindFirstChild(wpName)
                        if inst and TeleportEngine.IsWaypointUnlocked(wpName) then
                            local wpPos = inst:IsA("Model") and inst:GetPivot().Position or (inst:IsA("BasePart") and inst.Position)
                            if wpPos then
                                local d = (wpPos - targetPos).Magnitude
                                if d < bestWpDist then
                                    bestWpDist = d
                                    bestWp = wpName
                                end
                            end
                        end
                    end
                end
                if bestWp then
                    TeleportEngine.TeleportToWaypoint(bestWp)
                    if routeCancelled() then return false end
                    if State.HRP and NavEngine.IsOnMainFrostspireSide(State.HRP.Position) then
                        return NavEngine.DirectGlideTo(targetCFrame, speed, forceComplete, myGlideId, nil, nil, true)
                    end
                end
            end

            -- 2. Fallback reverse exit through Secret Maze Shaft -> Dock -> High over Ocean (+38Y)
            local exitLegs = {}
            if State.HRP and State.HRP.Position.Y < -55 then
                table.insert(exitLegs, { cf = CFrame.new(451, -128, -1845), minY = -142, maxY = -95 })
                table.insert(exitLegs, { cf = CFrame.new(192, -120, -1815), minY = -142, maxY = -95 })
                table.insert(exitLegs, { cf = CFrame.new(192, 14, -1815), minY = -130, maxY = 25 })
                table.insert(exitLegs, { cf = CFrame.new(200, 18, -1765), minY = 12 })
            end
            table.insert(exitLegs, { cf = CFrame.new(210, 28, -1680), minY = 24 })
            table.insert(exitLegs, { cf = CFrame.new(212, 38, -385), minY = 28 })
            for _, leg in ipairs(exitLegs) do
                if routeCancelled() then return false end
                local okLeg = NavEngine.DirectGlideTo(leg.cf, speed, forceComplete, myGlideId, leg.minY, leg.maxY, false)
                if not okLeg then return false end
            end
            if routeCancelled() then return false end
            return NavEngine.DirectGlideTo(targetCFrame, speed, forceComplete, myGlideId, nil, nil, true)
        elseif startMaze and startPos.Y >= -55 and targetPos.Z < -1500 and targetPos.Y < -55 then
            -- On Maze island surface heading into Secret Maze interior -> use Shaft Entrance
            local descLegs = {
                { cf = CFrame.new(200, 16, -1765), minY = 12 },
                { cf = CFrame.new(192, 14, -1815), minY = 10 },
                { cf = CFrame.new(192, -120, -1815), minY = -130, maxY = 22 },
                { cf = CFrame.new(451, -128, -1845), minY = -142, maxY = -95 },
            }
            for _, leg in ipairs(descLegs) do
                if routeCancelled() then return false end
                local okLeg = NavEngine.DirectGlideTo(leg.cf, speed, forceComplete, myGlideId, leg.minY, leg.maxY, false)
                if not okLeg then return false end
            end
            if routeCancelled() then return false end
            return NavEngine.DirectGlideTo(targetCFrame, speed, forceComplete, myGlideId, nil, -95, true)
        end
    end

    return NavEngine.DirectGlideTo(targetCFrame, speed, forceComplete, myGlideId, nil, nil, true)
end

-- Teleports character with physics broadphase impulse
function NavEngine.SafeTeleport(targetCFrame)
    if not State.HRP or not targetCFrame then return false end
    State.HRP.CFrame = targetCFrame
    -- Micro-velocity impulse to force spatial broadphase contact recalculation
    State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
    task.wait(0.06)
    return true
end

-- Unified navigation: Smooth Glide across all transport routes
function NavEngine.NavigateTo(targetCFrame, forceComplete)
    if not State.HRP or not targetCFrame then return false end
    return NavEngine.GlideTo(targetCFrame, Config.GlideSpeed or 40, forceComplete)
end

-- Verifies the character is safely grounded
function NavEngine.IsCharacterGrounded()
    if not State.Humanoid or not State.HRP then return false end
    local state = State.Humanoid:GetState()
    if state == Enum.HumanoidStateType.Freefall or state == Enum.HumanoidStateType.Flying then
        return false
    end
    return State.Humanoid.FloorMaterial ~= Enum.Material.Air
end

-- ===================================================================
-- 7. COMBAT & WEAPON MANAGEMENT (Auto-Parry, Auto-Defense & Mob Farming)
-- ===================================================================
local CombatEngine = {
    AttackAnimIds = {},
    AnimParryLead = {},
    LastParryByMob = {},
    LastParryIndByMob = {},
    LastParryTime = 0,
    LastServerParrySuccess = 0,
    ParryHoldUntil = 0,
    ParryEpoch = 0,
    DodgeUntil = 0,
    IsParrying = false,
    Blacklist = {},
    BossSkillAnims = {},
    BossSkillEffects = {},
    ActiveBossSkills = {},
    ActiveSkillName = nil,
}
State.CombatEngine = CombatEngine

function CombatEngine.GetPingSeconds()
    local pingMs = 55
    pcall(function()
        local item = game:GetService("Stats").Network.ServerStatsItem["Data Ping"]
        if item then
            pingMs = item:GetValue() or 55
        end
    end)
    return math.clamp(pingMs / 1000, 0.02, 0.35)
end

function CombatEngine.ResetCharacterNow(reason)
    pcall(function()
        if State.HRP then
            State.HRP.Anchored = false
        end
        if State.Humanoid then
            State.Humanoid.AutoRotate = true
        end
    end)
    if type(Notify) == "function" and reason then
        Notify("Recovery", reason .. " — resetting character...", 3.5)
    end
    local resetOk = false
    if Remotes.ResetCharacter then
        resetOk = pcall(function()
            Remotes.ResetCharacter:InvokeServer()
        end)
    end
    if not resetOk and State.Humanoid and State.Humanoid.Health > 0 then
        pcall(function()
            State.Humanoid.Health = 0
        end)
    end
end

function CombatEngine.IsBlockingOrParrying()
    if CombatEngine.IsParrying or os.clock() < (CombatEngine.ParryHoldUntil or 0) then
        return true
    end
    local st = State.Character and State.Character:FindFirstChild("Status")
    if st and (st:FindFirstChild("Blocking") or st:FindFirstChild("Parrying")) then
        return true
    end
    return false
end

-- Detects ranged/kiting mobs (e.g. Abyssal Watcher) that run away when approached
function CombatEngine.IsKitingMob(mob)
    if not mob then return false end
    local et = mob:GetAttribute("EnemyType") or mob:GetAttribute("EnemyName") or mob:GetAttribute("Name") or mob.Name
    local lower = tostring(et):lower()
    return lower:find("abyssal watcher", 1, true) ~= nil or lower:find("watcher", 1, true) ~= nil
end

-- Detects if a boss/enemy currently has an active immunity shield (e.g. Galaxite Behemoth BehemothImmune, Skeleton King ShieldActive)
function CombatEngine.IsBossShielded(mob)
    if not mob or not mob.Parent then return false end
    if mob:GetAttribute("ShieldActive") == true or mob:GetAttribute("Immune") == true or mob:GetAttribute("Invulnerable") == true then
        return true
    end
    if (tonumber(mob:GetAttribute("ShieldLeft")) or 0) > 0 then
        return true
    end
    if mob:FindFirstChild("BehemothImmune", true) ~= nil then
        return true
    end
    local st = mob:FindFirstChild("Status")
    if st and (st:FindFirstChild("Immune") or st:FindFirstChild("Invulnerable") or st:FindFirstChild("Shield") or st:FindFirstChild("BossShield")) then
        return true
    end
    return false
end

function CombatEngine.GetMobVariantPrefix(mob)
    if not mob then return nil end
    local mut = mob:GetAttribute("Mutation")
    if mut == "Limbo" or mut == "Heaven" or mut == "Hell" then
        return mut
    end
    local hum = mob:FindFirstChildOfClass("Humanoid")
    local dn = hum and hum.DisplayName
    if type(dn) == "string" then
        for _, pref in ipairs({ "Limbo", "Heaven", "Hell" }) do
            if dn:sub(1, #pref + 1) == (pref .. " ") then
                return pref
            end
        end
    end
    return nil
end


function CombatEngine.IsLimboMob(mob)
    return CombatEngine.GetMobVariantPrefix(mob) == "Limbo"
end

function CombatEngine.GetMobSpeciesName(mob)
    if not mob then return "" end
    local attr = mob:GetAttribute("EnemyType") or mob:GetAttribute("EnemyName") or mob:GetAttribute("Name")
    local baseName = (type(attr) == "string" and attr ~= "") and attr or tostring(mob.Name):gsub("%d+$", "")
    if Config.SplitEventVariantMobs then
        local pref = CombatEngine.GetMobVariantPrefix(mob)
        if pref and baseName ~= "" and baseName:sub(1, #pref + 1) ~= (pref .. " ") then
            return pref .. " " .. baseName
        end
    end
    return baseName
end

function CombatEngine.MatchesTargetMob(mob)
    local sel = Config.MobFarmTargets
    local validMobs = ConfigManager and ConfigManager.ValidMobSet
    if type(sel) == "table" then
        local sp = CombatEngine.GetMobSpeciesName(mob)
        local hasValidSpecific = false
        for k, v in pairs(sel) do
            local name = (type(k) == "number") and tostring(v) or (v and tostring(k))
            if name and name ~= "" and name ~= "All" and (not validMobs or validMobs[name]) then
                hasValidSpecific = true
                if name == sp then return true end
            elseif name == "All" then
                return true
            end
        end
        return not hasValidSpecific
    elseif type(sel) == "string" and sel ~= "" and sel ~= "All" then
        if validMobs and not validMobs[sel] then return true end
        return CombatEngine.GetMobSpeciesName(mob) == sel
    end
    return true
end

function CombatEngine.IsHostileMob(mob)
    if not mob or not mob.Parent or not mob:IsA("Model") or mob == State.Character then
        return false
    end
    if mob:GetAttribute("IsNpc") ~= true then
        return false
    end
    if mob:GetAttribute("DpsDummy") == true or tostring(mob.Name):find("Dummy", 1, true) then
        return false
    end
    local hum = mob:FindFirstChildOfClass("Humanoid")
    local hrp = mob:FindFirstChild("HumanoidRootPart")
    return (hum ~= nil and hum.Health > 0 and hrp ~= nil)
end

function CombatEngine.GetNearbyMobs(originPos, maxRadius)
    maxRadius = maxRadius or 24.0
    local living = workspace:FindFirstChild("Living")
    local list = {}
    local closestMob = nil
    local closestDist = math.huge
    local hasBomber = false
    if not living or not originPos then
        return 0, nil, math.huge, false, list
    end
    for _, m in ipairs(living:GetChildren()) do
        if CombatEngine.IsHostileMob(m) then
            local mHrp = m:FindFirstChild("HumanoidRootPart")
            if mHrp then
                local d = (mHrp.Position - originPos).Magnitude
                if d <= maxRadius then
                    table.insert(list, { model = m, pos = mHrp.Position, dist = d })
                    if tostring(m.Name):find("Bomber", 1, true) and d <= 16.0 then
                        hasBomber = true
                    end
                    if d < closestDist then
                        closestDist = d
                        closestMob = m
                    end
                end
            end
        end
    end
    return #list, closestMob, closestDist, hasBomber, list
end

-- Populate CombatEngine.AttackAnimIds from ReplicatedStorage.Assets.Animations
pcall(function()
    local animFolder = ReplicatedStorage:FindFirstChild("Assets") and ReplicatedStorage.Assets:FindFirstChild("Animations")
    local skipFolders = {
        GreatSword = true, StraightSword = true, Dagger = true, GreatAxe = true,
        Katana = true, Rapier = true, Mace = true, Polearm = true,
        CandyCane = true, ["Candy Cane"] = true, Movement = true,
        PickaxeHit1 = true, PickaxeHit2 = true, WaveEmote = true, ["Golem Companion"] = true
    }
    -- Populate Known Boss & Asura Skill Animations & Effects (excluded from parryable M1 table)
    CombatEngine.BossSkillAnims["rbxassetid://93412490559691"] = { skill = "Quadruple Slash", is360 = false, holdAttacks = false }
    CombatEngine.BossSkillAnims["rbxassetid://137303804583681"] = { skill = "Worldsplitter", is360 = false, holdAttacks = false }
    CombatEngine.BossSkillAnims["rbxassetid://121419468374386"] = { skill = "Decimate", is360 = false, holdAttacks = false }
    CombatEngine.BossSkillAnims["rbxassetid://139659389252394"] = { skill = "Buddha's Killer", is360 = true, holdAttacks = true }
    CombatEngine.BossSkillAnims["rbxassetid://105907339771163"] = { skill = "Prophecy Vision", is360 = true, holdAttacks = true }
    CombatEngine.BossSkillAnims["rbxassetid://124430298288598"] = { skill = "Phase 2 Morph", is360 = true, holdAttacks = true }

    CombatEngine.BossSkillEffects = {
        AsuraQuadruple = { skill = "Quadruple Slash", dur = 2.8, is360 = false, holdAttacks = false },
        AsuraWorldsplitter = { skill = "Worldsplitter", dur = 2.8, is360 = false, holdAttacks = false },
        AsuraDecimate = { skill = "Decimate", dur = 5.0, is360 = false, holdAttacks = false },
        AsuraBuddha = { skill = "Buddha's Killer", dur = 4.0, is360 = true, holdAttacks = true },
        AsuraProphecyVision = { skill = "Prophecy Vision", dur = 2.6, is360 = true, holdAttacks = true },
        AsuraMorph = { skill = "Phase 2 Morph", dur = 4.2, is360 = true, holdAttacks = true },
        SpiritGolemCrystalBarrage = { skill = "Crystal Barrage", dur = 3.0, is360 = true, holdAttacks = false },
    }

    if animFolder then
        for _, ch in ipairs(animFolder:GetChildren()) do
            if not skipFolders[ch.Name] then
                for _, a in ipairs(ch:GetDescendants()) do
                    if a:IsA("Animation") and a.AnimationId ~= "" then
                        local nl = a.Name:lower()
                        local isSkillAnim = nl:find("barrage") or nl:find("quadruple") or nl:find("worldsplitter")
                            or nl:find("decimate") or nl:find("buddha") or nl:find("prophecy")
                        if isSkillAnim then
                            CombatEngine.BossSkillAnims[a.AnimationId] = CombatEngine.BossSkillAnims[a.AnimationId] or {
                                skill = a.Name,
                                is360 = (nl:find("barrage") ~= nil or nl:find("buddha") ~= nil or nl:find("prophecy") ~= nil),
                                holdAttacks = (nl:find("prophecy") ~= nil or nl:find("buddha") ~= nil)
                            }
                        elseif not (nl:find("idle") or nl:find("walk") or nl:find("run") or nl:find("died")
                            or nl:find("stun") or nl:find("parried") or nl:find("takehit") or nl:find("dodge")
                            or nl:find("block") or nl:find("parry") or nl:find("phase") or nl:find("camera")) then
                            if not CombatEngine.BossSkillAnims[a.AnimationId] then
                                CombatEngine.AttackAnimIds[a.AnimationId] = true
                            end
                        end
                    end
                end
            end
        end
    end
end)

-- Detects if a target boss/mob is currently executing a known multi-hit/AoE skill (Animation + EffectService signal)
function CombatEngine.GetActiveBossSkill(mob)
    if not mob or not mob.Parent then return nil end
    local only360 = (Config.EvadeBossSkills360Only ~= false)
    local now = os.clock()
    local tracked = CombatEngine.ActiveBossSkills[mob]
    if tracked and now < (tracked.untilTime or 0) then
        if not only360 or tracked.is360 then
            return { active = true, skill = tracked.skill, is360 = tracked.is360 == true, holdAttacks = tracked.holdAttacks == true }
        end
    elseif tracked and now >= (tracked.untilTime or 0) then
        CombatEngine.ActiveBossSkills[mob] = nil
    end

    for _, d in ipairs(mob:GetDescendants()) do
        if d:IsA("Animator") then
            for _, tr in ipairs(d:GetPlayingAnimationTracks()) do
                local aId = tr.Animation and tr.Animation.AnimationId
                local meta = aId and CombatEngine.BossSkillAnims[aId]
                if meta and (not only360 or meta.is360) and tr.IsPlaying and (tr.Length <= 0 or tr.TimePosition < (tr.Length - 0.12)) then
                    return { active = true, skill = meta.skill, is360 = meta.is360 == true, holdAttacks = meta.holdAttacks == true }
                end
            end
        end
    end
    return nil
end


-- Tier 2 EffectService Listener: captures Boss skill broadcasts (AsuraQuadruple, AsuraDecimate, AsuraWorldsplitter, AsuraBuddha, etc.)
do
    local okEff, effEvent = pcall(function()
        return ReplicatedStorage.Shared.Packages.Knit.Services.EffectService.RE:FindFirstChild("Event")
    end)
    if okEff and effEvent and (effEvent:IsA("RemoteEvent") or effEvent:IsA("UnreliableRemoteEvent")) then
        trackConn(effEvent.OnClientEvent:Connect(function(modInst, methodName, arg1)
            if not ScriptRunning or typeof(modInst) ~= "Instance" then return end
            local effName = (modInst.Name == "Handler" and modInst.Parent) and modInst.Parent.Name or modInst.Name
            local meta = CombatEngine.BossSkillEffects and CombatEngine.BossSkillEffects[effName]
            if not meta then return end
            local attacker = nil
            if typeof(arg1) == "Instance" then
                attacker = arg1:IsA("Model") and arg1 or arg1:FindFirstAncestorOfClass("Model")
            end
            if attacker and attacker ~= State.Character then
                local existing = CombatEngine.ActiveBossSkills[attacker]
                local newUntil = os.clock() + (meta.dur or 3.0)
                if not existing or newUntil > (existing.untilTime or 0) then
                    CombatEngine.ActiveBossSkills[attacker] = {
                        skill = meta.skill,
                        untilTime = newUntil,
                        is360 = meta.is360 == true,
                        holdAttacks = meta.holdAttacks == true,
                    }
                end
            end
        end))
    end
end

function CombatEngine.EnsureWeaponEquipped()
    if not State.Character then return false end
    local equippedTool = State.Character:FindFirstChildWhichIsA("Tool")
    if equippedTool and (equippedTool.Name == "Weapon" or equippedTool:GetAttribute("IsWeapon") or (equippedTool.Name ~= "Pickaxe" and equippedTool.Name ~= "PortalTool" and equippedTool.Name ~= "Inventory")) then
        return true
    end

    -- 1. Check Backpack for "Weapon" or any weapon tool
    if LocalPlayer.Backpack then
        local bpTool = LocalPlayer.Backpack:FindFirstChild("Weapon")
        if not bpTool then
            for _, t in ipairs(LocalPlayer.Backpack:GetChildren()) do
                if t:IsA("Tool") and t.Name ~= "Pickaxe" and t.Name ~= "PortalTool" and t.Name ~= "Inventory" then
                    bpTool = t
                    break
                end
            end
        end
        if bpTool and State.Humanoid and not State.IsUsingPotion then
            State.Humanoid:EquipTool(bpTool)
            task.wait(0.1)
            if State.Character:FindFirstChildWhichIsA("Tool") then return true end
        end
    end

    -- 2. Click Hotbar Slot 2 (Standard Weapon slot in The Forge)
    local pgui = LocalPlayer:FindFirstChild("PlayerGui")
    local hotbar = pgui and pgui:FindFirstChild("BackpackGui") and pgui.BackpackGui:FindFirstChild("Backpack") and pgui.BackpackGui.Backpack:FindFirstChild("Hotbar")
    local slot2 = hotbar and hotbar:FindFirstChild("2")
    if slot2 and typeof(firesignal) == "function" then
        firesignal(slot2.MouseButton1Click)
        task.wait(0.1)
        local eq = State.Character:FindFirstChildWhichIsA("Tool")
        if eq and eq.Name ~= "Pickaxe" then return true end
    end

    -- 3. Remote Equip via CharacterService / Replica
    if Remotes.EquipItem and State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Equipped then
        local weaponData = State.PlayerData.Data.Equipped.Weapon
        local weaponGuid = (type(weaponData) == "table" and (weaponData.GUID or weaponData.Guid)) or (type(weaponData) == "string" and weaponData)
        if weaponGuid then
            pcall(function() Remotes.EquipItem:InvokeServer(weaponGuid) end)
            task.wait(0.15)
            return true
        end
    end
    return false
end

-- Executes a timed Parry (StartBlock -> hold parry window -> StopBlock) or Unparryable Evade
function CombatEngine.TriggerParry(attacker, isUnparryable)
    if not ScriptRunning or not Config.AutoParry or State.IsAutoForging then return false end
    if not State.HRP or not State.Humanoid or State.Humanoid.Health <= 0 then return false end

    local aHrp = attacker and attacker:FindFirstChild("HumanoidRootPart")
    local dist = aHrp and (aHrp.Position - State.HRP.Position).Magnitude or 0
    if aHrp and dist > (Config.AutoParryRange or 18.0) then
        return false
    end

    -- Handle Unparryable (Red flash / Bomber imminent blast): evade out of hitbox whether grounded or anchored in AutoMobFarm
    if isUnparryable and Config.AutoDodgeUnparryable then
        if aHrp then
            local away = Vector3.new(State.HRP.Position.X - aHrp.Position.X, 0, State.HRP.Position.Z - aHrp.Position.Z)
            local stepDir = (away.Magnitude > 0.1) and away.Unit or -State.HRP.CFrame.LookVector
            local sidestep = Vector3.new(-stepDir.Z, 0, stepDir.X)
            CombatEngine.DodgeUntil = os.clock() + 0.65
            if not State.HRP.Anchored and not CombatEngine.HoverLockActive then
                local dodgePos = State.HRP.Position + (stepDir * 6.5) + (sidestep * 4.0)
                NavEngine.SafeTeleport(CFrame.lookAt(dodgePos, Vector3.new(aHrp.Position.X, dodgePos.Y, aHrp.Position.Z)))
            elseif Config.AutoMobFarm or Config.AutoRaid then
                local yLift = (Config.MobFarmPositionMode == "Under Map") and 0 or 3.0
                local dodgePos = State.HRP.Position + (stepDir * 7.5) + Vector3.new(0, yLift, 0)
                local voidMinY = (workspace.FallenPartsDestroyHeight or -500) + 35
                if dodgePos.Y < voidMinY then
                    dodgePos = Vector3.new(dodgePos.X, voidMinY, dodgePos.Z)
                end
                local dcf = CFrame.lookAt(dodgePos, Vector3.new(aHrp.Position.X, dodgePos.Y, aHrp.Position.Z))
                if CombatEngine.HoverLockActive then
                    CombatEngine.SetHoverStance(true)
                    CombatEngine.ActiveStanceCF = dcf
                    State.HRP.CFrame = dcf
                    State.HRP.AssemblyLinearVelocity = Vector3.zero
                    State.HRP.AssemblyAngularVelocity = Vector3.zero
                else
                    NavEngine.SafeTeleport(dcf)
                end
            end
        end
        return true
    end

    local equippedTool = State.Character and State.Character:FindFirstChildWhichIsA("Tool")
    local hasWeapon = equippedTool and (equippedTool.Name == "Weapon" or equippedTool:GetAttribute("IsWeapon"))
    if not hasWeapon then
        if Config.AutoParryEquipWeapon or Config.AutoMobFarm or State.InAutoDefense then
            CombatEngine.EnsureWeaponEquipped()
            equippedTool = State.Character and State.Character:FindFirstChildWhichIsA("Tool")
            hasWeapon = equippedTool and (equippedTool.Name == "Weapon" or equippedTool:GetAttribute("IsWeapon"))
            if not hasWeapon then return false end
        else
            return false
        end
    end

    local now = os.clock()
    local pingSec = CombatEngine.GetPingSeconds()
    local onParryCooldown = (now - (CombatEngine.LastServerParrySuccess or 0)) < 0.92
        or (now - (CombatEngine.LastParryTime or 0)) < 0.92
    local baseHold = math.max(math.clamp(Config.ParryHoldDuration or 0.40, 0.28, 0.85), 0.42 + pingSec * 0.6)
    local holdDur = onParryCooldown and math.max(baseHold, 0.62) or baseHold
    if attacker then
        CombatEngine.LastParryByMob[attacker] = now
    end

    -- If already blocking/parrying (e.g., second mob swings consecutively), extend active block instead of dropping guard!
    if CombatEngine.IsParrying then
        CombatEngine.ParryHoldUntil = math.max(CombatEngine.ParryHoldUntil or 0, now + holdDur)
        return true
    end

    CombatEngine.ParryEpoch = (CombatEngine.ParryEpoch or 0) + 1
    local myEpoch = CombatEngine.ParryEpoch
    CombatEngine.IsParrying = true
    CombatEngine.LastParryTime = now
    CombatEngine.ParryHoldUntil = now + holdDur

    task.spawn(function()
        pcall(function()
            if aHrp and State.HRP then
                local lookTarget = ((Config.AutoMobFarm or Config.AutoRaid) and (State.HRP.Anchored or CombatEngine.HoverLockActive))
                    and aHrp.Position
                    or Vector3.new(aHrp.Position.X, State.HRP.Position.Y, aHrp.Position.Z)
                State.HRP.CFrame = CFrame.lookAt(State.HRP.Position, lookTarget)
            end
            local TC = nil
            pcall(function()
                TC = SafeGetController("ToolController")
            end)
            -- Reset stale client isBlocking flag so TC:StartBlock() can never silently ignore a needed parry
            if TC then
                TC.isBlocking = false
            end
            if Remotes.StartBlock then
                pcall(function() Remotes.StartBlock:InvokeServer() end)
            elseif TC and type(TC.StartBlock) == "function" then
                pcall(function() TC:StartBlock() end)
            end

            -- Anchor hold deadline from when StartBlock confirmed so network RTT doesn't shorten the parry window
            CombatEngine.ParryHoldUntil = math.max(CombatEngine.ParryHoldUntil or 0, os.clock() + holdDur)

            while ScriptRunning and os.clock() < (CombatEngine.ParryHoldUntil or 0) do
                if not State.Humanoid or State.Humanoid.Health <= 0 then break end
                task.wait(0.03)
            end

            if Remotes.StopBlock then
                pcall(function() Remotes.StopBlock:InvokeServer() end)
                if TC then TC.isBlocking = false end
            elseif TC and type(TC.StopBlock) == "function" then
                TC.isBlocking = true
                pcall(function() TC:StopBlock() end)
            end
        end)
        if CombatEngine.ParryEpoch == myEpoch then
            CombatEngine.IsParrying = false
            CombatEngine.ParryHoldUntil = 0
        end
    end)
    return true
end

-- Tier 1 Auto-Parry Hook: Intercepts authoritative server ParryIndicator, Parry success & BomberCharge effects
task.spawn(function()
    pcall(function()
    local effects = ReplicatedStorage:FindFirstChild("Assets") and ReplicatedStorage.Assets:FindFirstChild("Effects")
    local parryInd = effects and effects:FindFirstChild("ParryIndicator") and effects.ParryIndicator:FindFirstChild("Handler")
    if parryInd and parryInd:IsA("ModuleScript") then
        local mod = require(parryInd)
        if type(mod) == "table" and type(mod.effect) == "function" then
            safeHook(mod, "effect", "ParryIndicator", function(attacker, isParryable, ...)
                if ScriptRunning and Config.AutoParry and attacker and attacker ~= State.Character then
                    CombatEngine.LastParryIndByMob[attacker] = os.clock()
                    -- Calibrate exact animation lead time for this enemy's attack animation so future swings pre-fire early by Ping
                    if isParryable ~= false then
                        pcall(function()
                            local hum = attacker:FindFirstChildOfClass("Humanoid")
                            local animator = hum and hum:FindFirstChildOfClass("Animator")
                            if animator then
                                local pingSec = CombatEngine.GetPingSeconds()
                                for _, tr in ipairs(animator:GetPlayingAnimationTracks()) do
                                    local aId = tr.Animation and tr.Animation.AnimationId
                                    if aId and CombatEngine.AttackAnimIds[aId] and tr.TimePosition > 0.05 then
                                        CombatEngine.AnimParryLead[aId] = math.max(0.03, tr.TimePosition - pingSec - 0.03)
                                        break
                                    end
                                end
                            end
                        end)
                    end
                    task.spawn(function()
                        CombatEngine.TriggerParry(attacker, isParryable == false)
                    end)
                end
                if typeof(attacker) ~= "Instance" or not attacker.Parent then return end
                local hk = OrigHooks["ParryIndicator"]
                if hk and type(hk.orig) == "function" then
                    local okCall, res = pcall(hk.orig, attacker, isParryable, ...)
                    if okCall then return res end
                end
            end)
        end
    end

    local parryDoneInd = effects and effects:FindFirstChild("Parry") and effects.Parry:FindFirstChild("Handler")
    if parryDoneInd and parryDoneInd:IsA("ModuleScript") then
        local pMod = require(parryDoneInd)
        if type(pMod) == "table" and type(pMod.effect) == "function" then
            safeHook(pMod, "effect", "ParrySuccess", function(targetChar, ...)
                -- Record parry cooldown timestamp; only release early if no other nearby mob is mid-swing
                if ScriptRunning and targetChar == State.Character then
                    local now = os.clock()
                    CombatEngine.LastServerParrySuccess = now
                    if CombatEngine.IsParrying and State.HRP then
                        local otherSwinging = false
                        local _, _, _, _, nearList = CombatEngine.GetNearbyMobs(State.HRP.Position, Config.AutoParryRange or 18.0)
                        for _, entry in ipairs(nearList) do
                            local mSt = entry.model and entry.model:FindFirstChild("Status")
                            if mSt and mSt:FindFirstChild("Attacking") and (now - (CombatEngine.LastParryByMob[entry.model] or 0)) > 0.35 then
                                otherSwinging = true
                                break
                            end
                        end
                        if not otherSwinging then
                            CombatEngine.ParryHoldUntil = math.min(CombatEngine.ParryHoldUntil or now, now + 0.12)
                        end
                    end
                end
                if typeof(targetChar) ~= "Instance" or not targetChar.Parent then return end
                local hk = OrigHooks["ParrySuccess"]
                if hk and type(hk.orig) == "function" then
                    local okCall, res = pcall(hk.orig, targetChar, ...)
                    if okCall then return res end
                end
            end)
        end
    end

    local bomberInd = effects and effects:FindFirstChild("BomberCharge") and effects.BomberCharge:FindFirstChild("Handler")
    if bomberInd and bomberInd:IsA("ModuleScript") then
        local bMod = require(bomberInd)
        if type(bMod) == "table" and type(bMod.effect) == "function" then
            safeHook(bMod, "effect", "BomberCharge", function(bomberMob, stage, ...)
                -- BomberCharge stage 5 (~0.8s before blast) is imminent detonation; avoid wasting moves on stage 1
                if ScriptRunning and Config.AutoParry and Config.AutoDodgeUnparryable and bomberMob and tonumber(stage) and stage >= 4 then
                    task.spawn(function()
                        CombatEngine.TriggerParry(bomberMob, true)
                    end)
                end
                if typeof(bomberMob) ~= "Instance" or not bomberMob.Parent then return end
                local hk = OrigHooks["BomberCharge"]
                if hk and type(hk.orig) == "function" then
                    local okCall, res = pcall(hk.orig, bomberMob, stage, ...)
                    if okCall then return res end
                end
            end)
        end
    end
    end)
end)

-- Tier 2 Ping-Compensated Predictive & Fallback Scanner:
-- Fires early using calibrated AnimParryLead[aId] (cancelling network RTT!), or as fallback if ParryIndicator was missed.
task.spawn(function()
    while ScriptRunning do
        if Config.AutoParry and not State.IsAutoForging and State.HRP and State.Humanoid and State.Humanoid.Health > 0 then
            pcall(function()
                local maxR = Config.AutoParryRange or 18.0
                local _, _, _, _, nearList = CombatEngine.GetNearbyMobs(State.HRP.Position, maxR)
                local now = os.clock()
                for _, entry in ipairs(nearList) do
                    local m = entry.model
                    local hum = m:FindFirstChildOfClass("Humanoid")
                    local animator = hum and hum:FindFirstChildOfClass("Animator")
                    if animator then
                        local mobCooldown = now - (CombatEngine.LastParryByMob[m] or 0)
                        local indAge = now - (CombatEngine.LastParryIndByMob[m] or 0)
                        for _, tr in ipairs(animator:GetPlayingAnimationTracks()) do
                            local aId = tr.Animation and tr.Animation.AnimationId
                            if aId and CombatEngine.AttackAnimIds[aId] then
                                local lead = CombatEngine.AnimParryLead[aId]
                                if lead then
                                    if mobCooldown >= 0.72 and tr.TimePosition >= lead and tr.TimePosition <= (lead + 0.24) then
                                        CombatEngine.TriggerParry(m, false)
                                        break
                                    end
                                elseif indAge > 1.05 and mobCooldown >= 0.85 and tr.TimePosition >= 0.30 and tr.TimePosition <= 0.60 then
                                    CombatEngine.TriggerParry(m, false)
                                    break
                                end
                            end
                        end
                    end
                end
            end)
        end
        task.wait(0.03)
    end
end)

-- One-time cleanup: restore any previously expanded mob HumanoidRootParts from older runs
pcall(function()
    local living = workspace:FindFirstChild("Living")
    if living then
        for _, m in ipairs(living:GetChildren()) do
            if CombatEngine.IsHostileMob(m) then
                local hrp = m:FindFirstChild("HumanoidRootPart")
                if hrp and hrp:GetAttribute("Aether_HbSize") then
                    hrp.Size = Vector3.new(2.2, 2.2, 1.1)
                    hrp.CanCollide = false
                    hrp.Transparency = 1.0
                    hrp:SetAttribute("Aether_HbSize", nil)
                    hrp:SetAttribute("Aether_HbVis", nil)
                end
            end
        end
    end
end)

-- ===================================================================
-- 8. MINING AUTOMATION ENGINE (Target Latch & Complete Rock Destruction)
-- ===================================================================
MiningEngine.Blacklist = {}              -- [rock] = expirationTimestamp
MiningEngine.TargetLastHp = nil
MiningEngine.TargetLastDamageTime = nil
MiningEngine.SwingsOnTarget = 0
MiningEngine.NoDamageSwings = 0
MiningEngine.StatusLabel = nil

-- Resolves the equipped pickaxe's name and its native Mine Power from Shared.Data.Equipments
function MiningEngine.GetPlayerPickaxePower()
    if not State.PlayerData then ResolvePlayerData() end
    local pickName = nil
    if State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Equipped then
        local pick = State.PlayerData.Data.Equipped.Pickaxe
        if type(pick) == "table" and pick.Name then
            pickName = pick.Name
        elseif type(pick) == "string" then
            pickName = pick
        end
    end

    if not pickName and State.Character then
        local tool = State.Character:FindFirstChildOfClass("Tool")
        if tool and tool:GetAttribute("ItemJSON") then
            local ok, data = pcall(HttpService.JSONDecode, HttpService, tool:GetAttribute("ItemJSON"))
            if ok and data and data.Name then
                pickName = data.Name
            end
        end
    end

    if not pickName and State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory and State.PlayerData.Data.Inventory.Equipments then
        for _, eq in ipairs(State.PlayerData.Data.Inventory.Equipments) do
            if type(eq) == "table" and eq.Name and tostring(eq.Name):find("Pickaxe") then
                pickName = eq.Name
                break
            end
        end
    end

    local Equipments = safeRequire(ReplicatedStorage.Shared.Data.Equipments)
    if pickName and Equipments.Items and Equipments.Items.Pickaxe and Equipments.Items.Pickaxe[pickName] then
        return Equipments.Items.Pickaxe[pickName]["Mine Power"] or 4, pickName
    end

    return 4, pickName or "Stone Pickaxe"
end

-- Strictly checks if a rock is physically present and alive (health > 0)
function MiningEngine.IsRockAlive(rock)
    if not rock or not rock.Parent then return false end
    if not rock:IsDescendantOf(workspace) then return false end

    local hp = rock:GetAttribute("Health")
    if hp ~= nil and hp <= 0 then
        return false -- Completely destroyed!
    end

    local primary = rock.PrimaryPart or rock:FindFirstChildWhichIsA("BasePart")
    if not primary then
        return false
    end

    return true
end

-- Checks if a rock's name matches the user's multi-select MineTargets (or single MineTarget)
-- NOTE: If the user has selected ANY specific rock type (other than "All"), "All" is automatically
-- ignored so selecting a specific rock in the multi-dropdown immediately targets ONLY that rock!
function MiningEngine.HasSpecificTargetSelected()
    local targets = Config.MineTargets
    local validSet = ConfigManager and ConfigManager.ValidRockSet
    if type(targets) == "table" then
        for k, v in pairs(targets) do
            local name = (type(k) == "number") and tostring(v) or (v and tostring(k))
            if name and name ~= "All" and name ~= "" and (not validSet or validSet[name]) then
                return true
            end
        end
    end
    return false
end

function MiningEngine.MatchesTargetRock(rockName)
    local targets = Config.MineTargets
    local validSet = ConfigManager and ConfigManager.ValidRockSet
    if type(targets) == "table" then
        local hasSpecific = MiningEngine.HasSpecificTargetSelected()
        if not hasSpecific then
            return true -- Only "All" (or zero valid rocks from this world) -> match all rocks
        end
        for k, v in pairs(targets) do
            local name = (type(k) == "number") and tostring(v) or (v and tostring(k))
            if name ~= "All" and name == rockName and (not validSet or validSet[name]) then
                return true
            end
        end
        return false
    end
    return (Config.MineTarget == "All") or (rockName == Config.MineTarget)
end

function MiningEngine.GetRockZoneFolder(rock)
    if not rock or not rock.Parent then return nil end
    local rocksFolder = workspace:FindFirstChild("Rocks")
    if not rocksFolder then return nil end
    local cur = rock.Parent
    while cur and cur.Parent and cur.Parent ~= rocksFolder do
        cur = cur.Parent
    end
    if cur and cur.Parent == rocksFolder then
        return cur.Name
    end
    return nil
end


function MiningEngine.MatchesTargetZone(rock)
    local zones = Config.MineZones
    if type(zones) ~= "table" then return true end
    local validZones = ConfigManager and ConfigManager.ValidZoneSet
    local hasSpecific = false
    local wantedFolders = {}
    for k, v in pairs(zones) do
        local label = (type(k) == "number") and tostring(v) or (v and tostring(k))
        if label and label ~= "" and label ~= "Any" and (not validZones or validZones[label] or MineZoneLabelToFolder[label]) then
            hasSpecific = true
            local mapped = MineZoneLabelToFolder[label] or label
            wantedFolders[mapped] = true
            wantedFolders[label] = true
        end
    end
    if not hasSpecific then
        return true -- Only "Any" (or zero valid zones from this world) -> mine across all unlocked zones
    end
    local folderName = MiningEngine.GetRockZoneFolder(rock)
    return (folderName ~= nil and wantedFolders[folderName] == true)
end

-- Locked/gated zone rules mapped from ReplicatedStorage.Shared.Data.Regions & Proximities
MiningEngine.GatedZoneRules = {
    { pattern = "Island2GoblinCave", flag = "UnlockedGoblinCaveDoor" },
    { pattern = "DemoniteCave", flag = "UnlockedDemoniteCaveDoor" },
    { pattern = "FallenAngel", flag = "UnlockedFallenAngelCaveDoor" },
    { pattern = "RedCave", flag = "UnlockedRedCaveDoor" },
    { pattern = "GalaxyCave", flag = "UnlockedGalaxyCaveDoor" },
}
MiningEngine.GatedRegionRules = {
    ["Goblin Cave"] = "UnlockedGoblinCaveDoor",
    ["DemoniteCave"] = "UnlockedDemoniteCaveDoor",
    ["FallenAngel"] = "UnlockedFallenAngelCaveDoor",
    ["Island3RedCave"] = "UnlockedRedCaveDoor",
    ["Island3RedCave2"] = "UnlockedRedCaveDoor",
    ["Galaxy Cave"] = "UnlockedGalaxyCaveDoor",
    ["GalaxyCave"] = "UnlockedGalaxyCaveDoor",
}
MiningEngine._cachedRegionParts = nil
MiningEngine._cachedRegionCount = -1

function MiningEngine.GetMiscDialogues()
    if not State.PlayerData then ResolvePlayerData() end
    local data = State.PlayerData and State.PlayerData.Data
    return (type(data) == "table" and type(data.MiscDialogues) == "table") and data.MiscDialogues or {}
end

function MiningEngine.GetGatedRegionParts()
    local debris = workspace:FindFirstChild("Debris")
    local regFolder = debris and debris:FindFirstChild("Regions")
    if not regFolder then
        MiningEngine._cachedRegionParts = {}
        MiningEngine._cachedRegionCount = 0
        return MiningEngine._cachedRegionParts
    end
    local children = regFolder:GetChildren()
    if MiningEngine._cachedRegionParts and MiningEngine._cachedRegionCount == #children then
        return MiningEngine._cachedRegionParts
    end
    local list = {}
    for _, rp in ipairs(children) do
        local flag = MiningEngine.GatedRegionRules[rp.Name]
        if flag and rp:IsA("BasePart") then
            table.insert(list, { part = rp, flag = flag })
        end
    end
    MiningEngine._cachedRegionParts = list
    MiningEngine._cachedRegionCount = #children
    return list
end

MiningEngine.SkippedRocks = {}
MiningEngine.PeekStatusLabel = nil
MiningEngine.SkippedRocksCount = 0

function MiningEngine.ClearSkippedRocks()
    table.clear(MiningEngine.SkippedRocks)
end

function MiningEngine.GetActiveSkippedCount()
    local count = 0
    for r in pairs(MiningEngine.SkippedRocks) do
        if r and r.Parent and r:IsDescendantOf(workspace) then
            count = count + 1
        else
            MiningEngine.SkippedRocks[r] = nil
        end
    end
    return count
end

function MiningEngine.HasRockRegenerated(rock, revealedOres)
    local info = MiningEngine.SkippedRocks[rock]
    if not info then return true end
    if not rock or not rock.Parent or not rock:IsDescendantOf(workspace) then
        MiningEngine.SkippedRocks[rock] = nil
        return true
    end
    local hp = tonumber(rock:GetAttribute("Health")) or 0
    local maxHp = tonumber(rock:GetAttribute("MaxHealth")) or 1
    revealedOres = revealedOres or MiningEngine.GetRockRevealedOres(rock)
    -- Server regeneration resets Health back to MaxHealth (100%) and clears spawned Ore(Model) children
    if maxHp > 0 and hp >= maxHp * 0.99 and #revealedOres == 0 and (os.clock() - (info.skippedAt or 0)) >= 1.5 then
        MiningEngine.SkippedRocks[rock] = nil
        return true
    end
    return false
end

function MiningEngine.GetWantedOreSet()
    local set = {}
    local count = 0
    local wanted = Config.WantedRockOres
    if type(wanted) == "table" then
        for k, v in pairs(wanted) do
            local name = (type(k) == "number") and tostring(v) or (v and tostring(k))
            if name and name ~= "" and name ~= "None" and not set[name] then
                set[name] = true
                count = count + 1
            end
        end
    elseif type(wanted) == "string" and wanted ~= "" and wanted ~= "None" then
        set[wanted] = true
        count = 1
    end
    return set, count
end

function MiningEngine.GetRockRevealedOres(rock)
    local ores = {}
    if not rock or not rock.Parent then return ores end
    for _, ch in ipairs(rock:GetChildren()) do
        if ch.Name == "Ore" or ch:GetAttribute("Ore") ~= nil then
            local oreName = ch:GetAttribute("Ore")
            if type(oreName) == "string" and oreName ~= "" then
                table.insert(ores, oreName)
            end
        end
    end
    return ores
end

function MiningEngine.RockHasWantedOre(rock, revealedOres)
    revealedOres = revealedOres or MiningEngine.GetRockRevealedOres(rock)
    local wantedSet, wantedCount = MiningEngine.GetWantedOreSet()
    if wantedCount == 0 then
        return true, revealedOres, revealedOres
    end
    local matches = {}
    for _, oreName in ipairs(revealedOres) do
        if wantedSet[oreName] then
            table.insert(matches, oreName)
        end
    end
    return #matches > 0, matches, revealedOres
end

function MiningEngine.SetPeekStatus(title, desc)
    if MiningEngine.PeekStatusLabel then
        pcall(function()
            MiningEngine.PeekStatusLabel:SetTitle(title)
            if desc ~= nil then
                MiningEngine.PeekStatusLabel:SetDesc(desc)
            end
        end)
    end
end

function MiningEngine.GetRockPossibleOresMap()
    if MiningEngine._RockPossibleOresMap then
        return MiningEngine._RockPossibleOresMap
    end
    local map = {}
    pcall(function()
        local shared = ReplicatedStorage:FindFirstChild("Shared")
        local data = shared and shared:FindFirstChild("Data")
        local rockMod = data and data:FindFirstChild("Rock")
        if rockMod and rockMod:IsA("ModuleScript") then
            local ok, tbl = pcall(require, rockMod)
            if ok and type(tbl) == "table" then
                for rockName, info in pairs(tbl) do
                    if type(info) == "table" then
                        local list = info.Ores or info.Drops or info.OreDrops
                        if type(list) == "table" then
                            local set = {}
                            for _, oreName in ipairs(list) do
                                if type(oreName) == "string" and oreName ~= "" then
                                    set[oreName] = true
                                end
                            end
                            map[rockName] = set
                        end
                    end
                end
            end
        end
    end)
    MiningEngine._RockPossibleOresMap = map
    return map
end

function MiningEngine.RockCanDropAnyWantedOre(rockName)
    local wantedSet, wantedCount = MiningEngine.GetWantedOreSet()
    if wantedCount == 0 then
        return true
    end
    local possible = MiningEngine.GetRockPossibleOresMap()[rockName]
    if not possible then
        return true -- Unknown rock data: treat conservatively as potentially wanted
    end
    for oreName in pairs(wantedSet) do
        if possible[oreName] then
            return true
        end
    end
    return false
end

function MiningEngine.MatchesBackupRock(rockName)
    if not rockName or rockName == "Lucky Block" then return false end
    local targets = Config.BackupMineTargets
    local autoMode = false
    local hasExplicit = false
    local explicitMatch = false
    if type(targets) == "table" then
        for k, v in pairs(targets) do
            local name = (type(k) == "number") and tostring(v) or (v and tostring(k))
            if name == "Auto (Rocks Lacking Wanted Ores)" then
                autoMode = true
            elseif name and name ~= "" and name ~= "All" then
                hasExplicit = true
                if name == rockName then
                    explicitMatch = true
                end
            end
        end
    elseif type(targets) == "string" then
        if targets == "Auto (Rocks Lacking Wanted Ores)" or targets == "" then
            autoMode = true
        elseif targets == rockName then
            explicitMatch = true
        end
    else
        autoMode = true
    end
    if not autoMode and not hasExplicit then
        autoMode = true
    end
    if explicitMatch then
        return true
    end
    if autoMode then
        return not MiningEngine.RockCanDropAnyWantedOre(rockName)
    end
    return false
end

-- Returns "wanted", "pending", or "skipped" when Config.FilterRocksByOre is active
function MiningEngine.EvaluateRockOreFilter(rock)
    if not Config.FilterRocksByOre then
        return "wanted"
    end
    if MiningEngine.IsBackupTarget and rock == State.TargetRock then
        MiningEngine.SetPeekStatus(
            "Backup Clear: " .. tostring(rock.Name),
            "Fully breaking filler rock to force a new rock respawn"
        )
        return "wanted"
    end
    local _, wantedCount = MiningEngine.GetWantedOreSet()
    if wantedCount == 0 then
        return "wanted"
    end
    local revealedOres = MiningEngine.GetRockRevealedOres(rock)
    if #revealedOres == 0 then
        return "pending"
    end
    local hasWanted, matches = MiningEngine.RockHasWantedOre(rock, revealedOres)
    if hasWanted then
        MiningEngine.SetPeekStatus(
            "Match: " .. table.concat(matches, ", "),
            "Breaking " .. tostring(rock.Name) .. " (Revealed: " .. table.concat(revealedOres, ", ") .. ")"
        )
        return "wanted", matches
    end

    -- None of the revealed ores match WantedRockOres -> skip until server regenerates it!
    if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
    if State.Humanoid then State.Humanoid.AutoRotate = true end
    MiningEngine.SkippedRocks[rock] = {
        hpAtSkip = tonumber(rock:GetAttribute("Health")) or 0,
        skippedAt = os.clock(),
    }
    MiningEngine.SkippedRocksCount = (MiningEngine.SkippedRocksCount or 0) + 1
    MiningEngine.SetRockLocalCollision(rock, true)
    NavEngine.CachedRock = nil
    State.TargetRock = nil
    State.CurrentRock = nil
    State.LastMinedRock = nil
    State.ResumingAfterDeath = false
    MiningEngine.TargetLastHp = nil
    MiningEngine.TargetLastDamageTime = nil
    MiningEngine.SwingsOnTarget = 0
    MiningEngine.NoDamageSwings = 0

    MiningEngine.SetPeekStatus(
        string.format("Skipped #%d: %s", MiningEngine.SkippedRocksCount, tostring(rock.Name)),
        "Inside: " .. table.concat(revealedOres, ", ") .. " (waiting for regen)"
    )

    local nextRock = MiningEngine.FindBestRock()
    if nextRock then
        State.TargetRock = nextRock
        State.CurrentRock = nextRock
        State.LastMinedRock = nextRock
        MiningEngine.TargetLastHp = nextRock:GetAttribute("Health")
        MiningEngine.TargetLastDamageTime = os.clock()
    end
    return "skipped"
end

function MiningEngine.IsPositionZoneUnlocked(pos, misc)
    if not pos then return true end
    misc = misc or MiningEngine.GetMiscDialogues()
    local gatedRegions = MiningEngine.GetGatedRegionParts()
    for _, entry in ipairs(gatedRegions) do
        if not misc[entry.flag] then
            local rp = entry.part
            if rp and rp.Parent then
                local rel = rp.CFrame:PointToObjectSpace(pos)
                local half = rp.Size * 0.5
                if math.abs(rel.X) <= half.X and math.abs(rel.Z) <= half.Z and rel.Y <= half.Y + 4 then
                    return false
                end
            end
        end
    end
    return true
end

-- Prevents targeting rocks inside locked caverns (e.g. Chapter 2 Goblin/Crystal Cave) that teleport players out
function MiningEngine.IsRockZoneUnlocked(rock, misc)
    if not rock or not rock.Parent then return false end
    misc = misc or MiningEngine.GetMiscDialogues()

    -- 1. Check zone folder ancestry under workspace.Rocks
    local rocksFolder = workspace:FindFirstChild("Rocks")
    local cur = rock.Parent
    while cur and cur ~= rocksFolder and cur ~= workspace do
        local zName = cur.Name
        for _, rule in ipairs(MiningEngine.GatedZoneRules) do
            if zName:find(rule.pattern, 1, true) and not misc[rule.flag] then
                return false
            end
        end
        cur = cur.Parent
    end

    -- 2. Check XZ containment beneath gated Workspace.Debris.Regions ceiling slabs
    local primary = rock:FindFirstChild("Hitbox") or rock.PrimaryPart or rock:FindFirstChildWhichIsA("BasePart")
    if primary and not MiningEngine.IsPositionZoneUnlocked(primary.Position, misc) then
        return false
    end

    return true
end

-- Validates whether a rock can be targeted for a new mining cycle
function MiningEngine.IsRockMineable(rock, misc, ignoreOrePeek)
    if not MiningEngine.IsRockAlive(rock) then return false end
    if rock:GetAttribute("IsInitiated") == false then return false end

    -- Skip rocks outside selected Target Mining Zones (unless set to "Any")
    if not MiningEngine.MatchesTargetZone(rock) then
        return false
    end

    -- Skip rocks inside locked region caverns (e.g., Island2GoblinCave / Crystal Cave before quest unlock)
    if not MiningEngine.IsRockZoneUnlocked(rock, misc) then
        return false
    end

    -- Blacklist check (glitched/stuck/unreachable rocks)
    if MiningEngine.Blacklist[rock] and os.clock() < MiningEngine.Blacklist[rock] then
        return false
    end

    -- Ore Peek Filter check (wait until skipped rock regenerates, or skip already-cracked junk rocks)
    if Config.FilterRocksByOre and not ignoreOrePeek then
        local _, wantedCount = MiningEngine.GetWantedOreSet()
        if wantedCount > 0 then
            local revealed = MiningEngine.GetRockRevealedOres(rock)
            if not MiningEngine.HasRockRegenerated(rock, revealed) then
                return false
            end
            if #revealed > 0 and not MiningEngine.RockHasWantedOre(rock, revealed) then
                return false
            end
        end
    end

    local hasSpecific = MiningEngine.HasSpecificTargetSelected()
    if not hasSpecific or ignoreOrePeek then
        -- Filter by RequiredDamage and skip 10k HP Lucky Blocks when mining "All" or backup-clearing
        local playerPower = MiningEngine.GetPlayerPickaxePower()
        local reqDamage = rock:GetAttribute("RequiredDamage") or 0
        if reqDamage > playerPower then
            return false -- Equipped pickaxe cannot damage this rock tier!
        end
        if rock.Name == "Lucky Block" then
            return false
        end
    end

    return true
end

function MiningEngine.GetOtherPlayersData()
    local list = {}
    for _, plr in ipairs(Players:GetPlayers()) do
        if plr ~= LocalPlayer and plr.Character then
            local hrp = plr.Character:FindFirstChild("HumanoidRootPart")
            local hum = plr.Character:FindFirstChildOfClass("Humanoid")
            if hrp and hum and hum.Health > 0 then
                local hasPick = plr.Character:FindFirstChild("Pickaxe") ~= nil
                table.insert(list, { pos = hrp.Position, hasPick = hasPick })
            end
        end
    end
    return list
end

function MiningEngine.GetRockStrengthScore(rock)
    if not rock then return 0 end
    if not MiningEngine.RockStrengthCache then
        local cache = {}
        local okR, RockData = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Rock) end)
        if okR and type(RockData) == "table" then
            for k, entry in pairs(RockData) do
                if type(entry) == "table" then
                    local reqDmg = tonumber(entry.RequiredDamage or entry.MinDamage) or 0
                    local hp = tonumber(entry.Health) or 0
                    cache[tostring(k)] = (reqDmg * 1000000) + hp
                end
            end
        end
        MiningEngine.RockStrengthCache = cache
    end

    local cached = MiningEngine.RockStrengthCache[tostring(rock.Name)]
    if cached and cached > 0 then
        return cached
    end
    local req = tonumber(rock:GetAttribute("RequiredDamage")) or 0
    local hp = tonumber(rock:GetAttribute("MaxHealth") or rock:GetAttribute("Health")) or 0
    return (req * 1000000) + hp
end

function MiningEngine.IsRockClaimedByOtherPlayer(rock)
    if not rock or not rock.Parent then return false end
    local lastHit = rock:GetAttribute("LastHitPlayer")
    local myName = LocalPlayer and LocalPlayer.Name
    if type(lastHit) ~= "string" or lastHit == "" then
        return false
    end
    if myName and lastHit == myName then
        return false
    end
    local lastTime = tonumber(rock:GetAttribute("LastHitTime")) or 0
    if lastTime == 0 then
        return false
    end
    local dtClock = math.abs(os.clock() - lastTime)
    local dtServer = math.abs(workspace:GetServerTimeNow() - lastTime)
    return (dtClock <= 8) or (dtServer <= 8)
end

function MiningEngine.IsRockOccupiedByPlayer(rock, otherPlayers, activeOnly)
    if not rock or not rock.Parent then return false end
    local lastHit = rock:GetAttribute("LastHitPlayer")
    local myName = LocalPlayer and LocalPlayer.Name
    -- If WE already claimed this rock, never abandon it even if another player stands nearby or swings at it
    if type(lastHit) == "string" and myName and lastHit == myName then
        return false
    end
    if MiningEngine.IsRockClaimedByOtherPlayer(rock) then
        return true
    end
    -- When already latched onto an unclaimed rock, only yield if another player actually claims LastHitPlayer first
    if activeOnly then
        return false
    end
    otherPlayers = otherPlayers or MiningEngine.GetOtherPlayersData()
    if #otherPlayers == 0 then return false end
    local primary = rock:FindFirstChild("Hitbox") or rock.PrimaryPart or rock:FindFirstChildWhichIsA("BasePart")
    if not primary then return false end
    local rPos = primary.Position
    local rockRadius = math.max(primary.Size.X, primary.Size.Z) * 0.5
    local pickRadius = math.max(13.0, rockRadius + 8.5)
    local closeRadius = math.max(9.5, rockRadius + 5.5)
    for _, p in ipairs(otherPlayers) do
        local d = (p.pos - rPos).Magnitude
        if (p.hasPick and d <= pickRadius) or d <= closeRadius then
            return true
        end
    end
    return false
end

function MiningEngine.FindBestRock()
    local rocksFolder = workspace:FindFirstChild("Rocks")
    if not rocksFolder or not State.HRP then
        MiningEngine.IsBackupTarget = false
        return nil, math.huge
    end

    local hrpPos = State.HRP.Position
    local savedCF = (Config.StayAtSavedPos and MiningEngine.GetSavedSpotCFrame()) or nil
    local scanOrigin = savedCF and savedCF.Position or hrpPos
    local misc = MiningEngine.GetMiscDialogues()
    local useStrength = (Config.PrioritizeStrongestRock ~= false)
    local maxScanDist = Config.StayAtSavedPos and 14.5 or (Config.MineDistance or 5000)
    local otherPlayers = Config.AvoidPlayerOccupiedRocks and MiningEngine.GetOtherPlayersData() or {}

    local function runPass(matchFn, ignoreOrePeek)
        local bestRock = nil
        local shortestDist = math.huge
        local bestTierScore = -1
        local bestScore = math.huge
        local fallbackRock = nil
        local fallbackDist = math.huge
        local fallbackTierScore = -1
        local fallbackScore = math.huge

        local function scanContainer(container)
            for _, ch in ipairs(container:GetChildren()) do
                if ch:IsA("Model") and (ch:GetAttribute("Health") ~= nil or ch:FindFirstChild("Hitbox") or ch:GetAttribute("RequiredDamage") ~= nil) then
                    if matchFn(ch.Name) and MiningEngine.IsRockMineable(ch, misc, ignoreOrePeek) then
                        local primary = ch:FindFirstChild("Hitbox") or ch.PrimaryPart or ch:FindFirstChildWhichIsA("BasePart")
                        if primary then
                            local rPos = primary.Position
                            local dist = (rPos - scanOrigin).Magnitude
                            if dist <= maxScanDist then
                                -- Penalize huge vertical elevation differences (>28 studs) so rocks on the current floor beat cross-layer rocks
                                local dy = math.abs(rPos.Y - hrpPos.Y)
                                local score = dist + ((dy > 28) and (dy * 1.5) or 0)
                                local tierScore = useStrength and MiningEngine.GetRockStrengthScore(ch) or 0

                                -- Priority boost: claim already-cracked rocks showing a Wanted Ore immediately
                                if Config.FilterRocksByOre and not ignoreOrePeek then
                                    local revealed = MiningEngine.GetRockRevealedOres(ch)
                                    if #revealed > 0 and MiningEngine.RockHasWantedOre(ch, revealed) then
                                        score = score - 5000
                                    end
                                end

                                local occupied = Config.AvoidPlayerOccupiedRocks and MiningEngine.IsRockOccupiedByPlayer(ch, otherPlayers, false)
                                if occupied then
                                    if tierScore > fallbackTierScore or (tierScore == fallbackTierScore and score < fallbackScore) then
                                        fallbackTierScore = tierScore
                                        fallbackScore = score
                                        fallbackDist = dist
                                        fallbackRock = ch
                                    end
                                elseif tierScore > bestTierScore or (tierScore == bestTierScore and score < bestScore) then
                                    bestTierScore = tierScore
                                    bestScore = score
                                    shortestDist = dist
                                    bestRock = ch
                                end
                            end
                        end
                    end
                elseif ch:IsA("Folder") or ch:IsA("BasePart") or ch:IsA("Model") then
                    scanContainer(ch)
                end
            end
        end

        scanContainer(rocksFolder)
        if bestRock then
            return bestRock, shortestDist
        end
        return fallbackRock, fallbackDist
    end

    local primaryRock, primaryDist = runPass(MiningEngine.MatchesTargetRock, false)
    if primaryRock then
        MiningEngine.IsBackupTarget = false
        return primaryRock, primaryDist
    end

    if Config.FilterRocksByOre and Config.FilterRocksByOreBackupEnabled then
        local backupRock, backupDist = runPass(MiningEngine.MatchesBackupRock, true)
        if backupRock then
            MiningEngine.IsBackupTarget = true
            return backupRock, backupDist
        end
    end

    MiningEngine.IsBackupTarget = false
    return nil, math.huge
end

function MiningEngine.SetRockLocalCollision(rock, canCollide)
    if not rock then return end
    pcall(function()
        for _, d in ipairs(rock:GetDescendants()) do
            if d:IsA("BasePart") then
                d.CanCollide = canCollide
            end
        end
    end)
end

function MiningEngine.EnsurePickaxeEquipped()
    if not State.Character or State.IsUsingPotion then return false end
    local equippedTool = State.Character:FindFirstChildWhichIsA("Tool")
    if equippedTool and (equippedTool.Name == "Pickaxe" or equippedTool:GetAttribute("IsPickaxe")) then
        return true
    end

    -- 1. Check Backpack
    if LocalPlayer.Backpack then
        local bpTool = LocalPlayer.Backpack:FindFirstChild("Pickaxe") or LocalPlayer.Backpack:FindFirstChildWhichIsA("Tool")
        if bpTool and State.Humanoid and not State.IsUsingPotion then
            State.Humanoid:EquipTool(bpTool)
            task.wait(0.1)
            if State.Character:FindFirstChild("Pickaxe") then return true end
        end
    end

    -- 2. Click Hotbar Slot 1 (Standard Pickaxe slot in The Forge)
    local pgui = LocalPlayer:FindFirstChild("PlayerGui")
    local hotbar = pgui and pgui:FindFirstChild("BackpackGui") and pgui.BackpackGui:FindFirstChild("Backpack") and pgui.BackpackGui.Backpack:FindFirstChild("Hotbar")
    local slot1 = hotbar and hotbar:FindFirstChild("1")
    if slot1 and typeof(firesignal) == "function" then
        firesignal(slot1.MouseButton1Click)
        task.wait(0.1)
        if State.Character:FindFirstChild("Pickaxe") then return true end
    end

    -- 3. Remote Equip via CharacterService
    if Remotes.EquipItem and State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Equipped then
        local pickaxeData = State.PlayerData.Data.Equipped.Pickaxe
        local pickaxeGuid = (type(pickaxeData) == "table" and pickaxeData.GUID) or (type(pickaxeData) == "string" and pickaxeData)
        if pickaxeGuid then
            pcall(function() Remotes.EquipItem:InvokeServer(pickaxeGuid) end)
            task.wait(0.15)
            return true
        end
    end
    return false
end

function MiningEngine.ExecuteMineHit()
    if not Remotes.ToolActivated then return end
    pcall(function()
        Remotes.ToolActivated:InvokeServer("Pickaxe", false)
    end)
    -- Simultaneously spam Weapon alongside Pickaxe so nearby mobs take damage without cancelling active Auto-Parry
    if Config.AutoDefense and not CombatEngine.IsBlockingOrParrying() then
        task.spawn(function()
            pcall(function()
                Remotes.ToolActivated:InvokeServer("Weapon", false)
            end)
        end)
    end
end

function MiningEngine.GetSavedSpotCFrame()
    local sp = Config.SavedMinePos
    if type(sp) == "table" and sp.placeId == game.PlaceId and type(sp.x) == "number" and type(sp.y) == "number" and type(sp.z) == "number" then
        local pos = Vector3.new(sp.x, sp.y, sp.z)
        if type(sp.lx) == "number" and type(sp.ly) == "number" and type(sp.lz) == "number" then
            local lv = Vector3.new(sp.lx, sp.ly, sp.lz)
            if lv.Magnitude > 0.01 then
                return CFrame.lookAt(pos, pos + lv)
            end
        end
        return CFrame.new(pos)
    end
    return nil
end


function MiningEngine.UpdateSavedSpotLabel()
    if not MiningEngine.SavedSpotLabel then return end
    pcall(function()
        local cf = MiningEngine.GetSavedSpotCFrame()
        if cf then
            local p = cf.Position
            MiningEngine.SavedSpotLabel:SetTitle(string.format("Saved Spot: (%d, %d, %d)", math.round(p.X), math.round(p.Y), math.round(p.Z)))
            MiningEngine.SavedSpotLabel:SetDesc("Anchored here during Auto Mine (returns after Auto Forge / death)")
        else
            MiningEngine.SavedSpotLabel:SetTitle("Saved Spot: Not Set")
            MiningEngine.SavedSpotLabel:SetDesc("Stand where you want to mine and press 'Save Current Position'")
        end
    end)
end

function MiningEngine.SaveCurrentSpot()
    if not State.HRP then return false end
    local cf = State.HRP.CFrame
    local p = cf.Position
    local lv = cf.LookVector
    Config.SavedMinePos = {
        placeId = game.PlaceId,
        x = math.round(p.X * 100) / 100,
        y = math.round(p.Y * 100) / 100,
        z = math.round(p.Z * 100) / 100,
        lx = math.round(lv.X * 1000) / 1000,
        ly = math.round(lv.Y * 1000) / 1000,
        lz = math.round(lv.Z * 1000) / 1000,
    }
    if Config.AutoSaveConfig ~= false then
        pcall(ConfigManager.Save)
    end
    MiningEngine.UpdateSavedSpotLabel()
    local uiLib = (typeof(getgenv) == "function" and getgenv().Library) or _G.Library
    if uiLib and type(uiLib.Notify) == "function" then
        pcall(function()
            uiLib:Notify({
                Title = "Saved Mining Spot",
                Desc = string.format("Saved position (%d, %d, %d)", math.round(p.X), math.round(p.Y), math.round(p.Z)),
                Duration = 3
            })
        end)
    end
    return true
end

function MiningEngine.UpdateStatus()
    if not MiningEngine.StatusLabel then return end
    pcall(function()
        local pwr, pName = MiningEngine.GetPlayerPickaxePower()
        if Config.AutoMine then
            if State.TargetRock and MiningEngine.IsRockAlive(State.TargetRock) then
                local hp = State.TargetRock:GetAttribute("Health") or 0
                local maxHp = State.TargetRock:GetAttribute("MaxHealth") or hp
                MiningEngine.StatusLabel:SetTitle("Mining: " .. State.TargetRock.Name .. " [" .. tostring(hp) .. "/" .. tostring(maxHp) .. "]")
                MiningEngine.StatusLabel:SetDesc("Pickaxe: " .. tostring(pName) .. " (Power: " .. tostring(pwr) .. ")")
            else
                local maxD = Config.MineDistance or 75
                MiningEngine.StatusLabel:SetTitle("No rocks within " .. tostring(maxD) .. " studs")
                MiningEngine.StatusLabel:SetDesc("Walk into cave | Pickaxe: " .. tostring(pName))
            end
        else
            MiningEngine.StatusLabel:SetTitle("Status: Idle")
            MiningEngine.StatusLabel:SetDesc("Pickaxe: " .. tostring(pName) .. " (Power: " .. tostring(pwr) .. ")")
        end
    end)
end

function MiningEngine.SafeUnanchor()
    if State.HRP and State.HRP.Anchored then
        if NavEngine and NavEngine.IsSunkenMode(Config.MinePositionMode) then
            State.HRP.CFrame = CFrame.new(State.HRP.Position + Vector3.new(0, 3.8, 0))
        end
        State.HRP.Anchored = false
    end
    if State.Humanoid and not State.Humanoid.AutoRotate then
        State.Humanoid.AutoRotate = true
    end
end

-- Mining Task Loop (Anti-TP Protection & Target Latch Until Complete Destruction)
task.spawn(function()
    while ScriptRunning do
        pcall(MiningEngine.UpdateStatus)
        if Config.AutoMine and not State.IsAutoForging and not State.DrainingFullBag and not State.IsCrafting and not State.IsBuyingPotions and not State.CheckingPotionsOnStart and not State.IsUsingPotion and not State.UsingPotionCycle and State.HRP and State.Humanoid and State.Humanoid.Health > 0 then
            -- STAY AT SAVED SPOT MODE (Anchored stationary farm, returns automatically after Auto-Forge / Death)
            if Config.StayAtSavedPos then
                local savedCF = MiningEngine.GetSavedSpotCFrame()
                if not savedCF then
                    MiningEngine.SaveCurrentSpot()
                    savedCF = MiningEngine.GetSavedSpotCFrame()
                end
                if savedCF and State.HRP then
                    local distFromSpot = (State.HRP.Position - savedCF.Position).Magnitude
                    if distFromSpot > 2.5 then
                        if State.HRP.Anchored then State.HRP.Anchored = false end
                        if State.Humanoid then State.Humanoid.AutoRotate = true end
                        NavEngine.GlideTo(savedCF, Config.GlideSpeed or 55, true)
                    end

                    -- Once at the saved spot, lock Anchored = true so mobs/players/physics can never push you off
                    if State.HRP and (State.HRP.Position - savedCF.Position).Magnitude <= 2.8 then
                        State.ResumingAfterDeath = false
                        if State.Humanoid and State.Humanoid.AutoRotate then
                            State.Humanoid.AutoRotate = false
                        end
                        if not State.HRP.Anchored then
                            State.HRP.Anchored = true
                        end
                        State.HRP.AssemblyLinearVelocity = Vector3.zero
                        State.HRP.AssemblyAngularVelocity = Vector3.zero

                        local rock = State.TargetRock
                        local rockMatches = rock and (MiningEngine.IsBackupTarget and MiningEngine.MatchesBackupRock(rock.Name) or MiningEngine.MatchesTargetRock(rock.Name))
                        if not MiningEngine.IsRockAlive(rock) or not rockMatches or not MiningEngine.MatchesTargetZone(rock) then
                            State.TargetRock = nil
                            State.CurrentRock = nil
                            MiningEngine.IsBackupTarget = false
                            MiningEngine.TargetLastHp = nil
                            MiningEngine.TargetLastDamageTime = nil
                            rock = MiningEngine.FindBestRock()
                            if rock and MiningEngine.IsRockMineable(rock, nil, MiningEngine.IsBackupTarget) then
                                State.TargetRock = rock
                                State.CurrentRock = rock
                                State.LastMinedRock = rock
                                MiningEngine.TargetLastHp = rock:GetAttribute("Health")
                                MiningEngine.TargetLastDamageTime = os.clock()
                            end
                        end

                        if rock and MiningEngine.IsRockAlive(rock) then
                            local primary = rock:FindFirstChild("Hitbox") or rock.PrimaryPart or rock:FindFirstChildWhichIsA("BasePart")
                            if primary then
                                local rPos = primary.Position
                                local aimPoint = (math.abs(rPos.Y - savedCF.Position.Y) > 2.5)
                                    and rPos
                                    or Vector3.new(rPos.X, savedCF.Position.Y, rPos.Z)
                                State.HRP.CFrame = CFrame.lookAt(savedCF.Position, aimPoint)

                                if Config.AutoEquipPickaxe then
                                    MiningEngine.EnsurePickaxeEquipped()
                                end
                                if MiningEngine.EvaluateRockOreFilter(rock) == "skipped" then
                                    task.wait(0.08)
                                    continue
                                end
                                MiningEngine.ExecuteMineHit()

                                local curHp = rock:GetAttribute("Health")
                                if curHp and MiningEngine.TargetLastHp and curHp < MiningEngine.TargetLastHp then
                                    MiningEngine.TargetLastHp = curHp
                                    MiningEngine.TargetLastDamageTime = os.clock()
                                else
                                    if curHp and not MiningEngine.TargetLastHp then
                                        MiningEngine.TargetLastHp = curHp
                                    end
                                    if not MiningEngine.TargetLastDamageTime then
                                        MiningEngine.TargetLastDamageTime = os.clock()
                                    end
                                    if (os.clock() - MiningEngine.TargetLastDamageTime) >= math.max(Config.StuckRockThreshold or 4.0, 2.8) then
                                        MiningEngine.Blacklist[rock] = os.clock() + 8.0
                                        State.TargetRock = nil
                                        State.CurrentRock = nil
                                    end
                                end
                            end
                        else
                            -- Keep anchored at saved spot orientation waiting for rocks in range to respawn
                            State.HRP.CFrame = savedCF
                        end
                    end
                end
                task.wait(Config.MineDelay)
                continue
            end

            -- 1. LATCH INTEGRITY: Check if the currently latched rock is still alive AND matches current selection
            local rock = State.TargetRock
            local rockMatches = rock and (MiningEngine.IsBackupTarget and MiningEngine.MatchesBackupRock(rock.Name) or MiningEngine.MatchesTargetRock(rock.Name))
            if not MiningEngine.IsRockAlive(rock) or not rockMatches or not MiningEngine.MatchesTargetZone(rock) then
                -- Rock is destroyed, despawned, or unselected! Unanchor safely above floor before clearing latched target
                MiningEngine.SafeUnanchor()
                if rock then MiningEngine.SetRockLocalCollision(rock, true) end
                State.TargetRock = nil
                State.CurrentRock = nil
                State.LastMinedRock = nil
                State.ResumingAfterDeath = false
                MiningEngine.IsBackupTarget = false
                NavEngine.CachedRock = nil
                MiningEngine.TargetLastHp = nil
                MiningEngine.TargetLastDamageTime = nil
                MiningEngine.SwingsOnTarget = 0
                MiningEngine.NoDamageSwings = 0

                -- Search for the closest selected mineable rock
                rock = MiningEngine.FindBestRock()
                if rock and MiningEngine.IsRockMineable(rock, nil, MiningEngine.IsBackupTarget) then
                    State.TargetRock = rock
                    State.CurrentRock = rock
                    State.LastMinedRock = rock
                    MiningEngine.TargetLastHp = rock:GetAttribute("Health")
                    MiningEngine.TargetLastDamageTime = os.clock()
                    MiningEngine.SwingsOnTarget = 0
                    MiningEngine.NoDamageSwings = 0
                end
            end

            -- 2. If we have an alive rock, latch firmly and mine it until destruction
            if rock and MiningEngine.IsRockAlive(rock) then
                State.CurrentRock = rock
                State.LastMinedRock = rock

                -- Player-Contested Rock Switch: only switch if another player actually claimed this rock AND an unclaimed alternative exists
                if Config.AvoidPlayerOccupiedRocks and MiningEngine.IsRockOccupiedByPlayer(rock, nil, true) then
                    local otherRock = MiningEngine.FindBestRock()
                    if otherRock and otherRock ~= rock and not MiningEngine.IsRockOccupiedByPlayer(otherRock, nil, true) then
                        MiningEngine.SafeUnanchor()
                        MiningEngine.SetRockLocalCollision(rock, true)
                        MiningEngine.Blacklist[rock] = os.clock() + 10.0
                        NavEngine.CachedRock = nil
                        State.TargetRock = otherRock
                        State.CurrentRock = otherRock
                        State.LastMinedRock = otherRock
                        MiningEngine.TargetLastHp = otherRock:GetAttribute("Health")
                        MiningEngine.TargetLastDamageTime = os.clock()
                        task.wait(0.08)
                        continue
                    end
                end

                if Config.AutoEquipPickaxe then
                    MiningEngine.EnsurePickaxeEquipped()
                end

                -- Early mid-approach ore filter check: abort glide immediately if another player already revealed junk ores
                if MiningEngine.EvaluateRockOreFilter(rock) == "skipped" then
                    MiningEngine.SafeUnanchor()
                    task.wait(0.05)
                    continue
                end

                local primary = rock:FindFirstChild("Hitbox") or rock.PrimaryPart or rock:FindFirstChildWhichIsA("BasePart")
                if primary and State.HRP then
                    local rockPos = primary.Position
                    local charPos = State.HRP.Position
                    local isSunken = NavEngine.IsSunkenMode(Config.MinePositionMode)

                    -- Disable local collision on target rock when mining underground so rock geometry never pushes us on top
                    if isSunken or (Config.MinePositionMode == "Inside Rock (Pebbles Only)" and rock.Name == "Pebble") then
                        MiningEngine.SetRockLocalCollision(rock, false)
                    end

                    -- 1. Compute target stance CFrame (cached per rock so it never shifts mid-approach)
                    local targetCFrame = NavEngine.GetMiningCFrame(rock, Config.MinePositionMode)
                    if targetCFrame then
                        local horizDist = Vector3.new(charPos.X - targetCFrame.Position.X, 0, charPos.Z - targetCFrame.Position.Z).Magnitude
                        local distToRock = (charPos - rockPos).Magnitude

                        -- 2. Clean Above-Ground Smooth Glide Approach: glide if horizontal XZ distance > 2.5 studs
                        if horizDist > 2.5 then
                            if State.HRP.Anchored then
                                if isSunken then
                                    State.HRP.CFrame = CFrame.new(State.HRP.Position + Vector3.new(0, 3.8, 0))
                                end
                                State.HRP.Anchored = false
                            end
                            if State.Humanoid then State.Humanoid.AutoRotate = true end
                            -- Reset damage watchdog while still traveling so travel time is never counted as stuck time
                            MiningEngine.TargetLastDamageTime = os.clock()
                            local approachCF = isSunken
                                and CFrame.new(targetCFrame.Position.X, targetCFrame.Position.Y + 3.2, targetCFrame.Position.Z)
                                or targetCFrame
                            local navReached = NavEngine.NavigateTo(approachCF)
                            -- Update position after navigation completes
                            if State.HRP then
                                charPos = State.HRP.Position
                                horizDist = Vector3.new(charPos.X - targetCFrame.Position.X, 0, charPos.Z - targetCFrame.Position.Z).Magnitude
                                distToRock = (charPos - rockPos).Magnitude
                            end
                            -- If preempted mid-glide to a closer rock, immediately switch without blacklisting
                            if State.TargetRock and State.TargetRock ~= rock then
                                NavEngine.CachedRock = nil
                                rock = State.TargetRock
                            elseif State.IsBuyingPotions or State.CheckingPotionsOnStart or State.IsAutoForging or State.DrainingFullBag or State.IsCrafting then
                                -- Higher-priority routine (potions/craft/forge) interrupted transit; yield without blacklisting the rock
                                NavEngine.CachedRock = nil
                                State.TargetRock = nil
                                State.CurrentRock = nil
                            elseif not State.Humanoid or State.Humanoid.Health <= 0 then
                                -- Died mid-transit: preserve LastMinedRock for respawn recovery without blacklisting
                                NavEngine.CachedRock = nil
                                State.TargetRock = nil
                                State.CurrentRock = nil
                            elseif not navReached or horizDist > 3.5 then
                                -- Short navigation stall retry (8s) so close rocks aren't locked out for 45s forcing cross-map trips
                                MiningEngine.Blacklist[rock] = os.clock() + math.min(8, Config.StuckRockBlacklistTime or 45)
                                NavEngine.CachedRock = nil
                                State.TargetRock = nil
                                State.CurrentRock = nil
                                State.LastMinedRock = nil
                                State.ResumingAfterDeath = false
                                local nextRock = MiningEngine.FindBestRock()
                                if nextRock then
                                    State.TargetRock = nextRock
                                    State.CurrentRock = nextRock
                                    State.LastMinedRock = nextRock
                                    MiningEngine.TargetLastHp = nextRock:GetAttribute("Health")
                                    MiningEngine.TargetLastDamageTime = os.clock()
                                end
                            else
                                -- Reset watchdog upon arrival so the rock gets a full window of real swings
                                State.ResumingAfterDeath = false
                                MiningEngine.TargetLastDamageTime = os.clock()
                            end
                        end

                        -- 3. Stance Locking: Once horizontally within 2.5 studs, anchor FIRST then snap into sunken horizontal CFrame
                        if horizDist <= 2.5 and State.HRP then
                            State.ResumingAfterDeath = false
                            if isSunken then
                                if State.Humanoid and State.Humanoid.AutoRotate then
                                    State.Humanoid.AutoRotate = false
                                end
                                if not State.HRP.Anchored then
                                    State.HRP.Anchored = true
                                    State.HRP.AssemblyLinearVelocity = Vector3.zero
                                    State.HRP.AssemblyAngularVelocity = Vector3.zero
                                end
                                -- Enforce horizontal sunken CFrame while anchored so floor collision can never pop us up
                                State.HRP.CFrame = targetCFrame
                            else
                                if State.Humanoid and not State.Humanoid.AutoRotate then
                                    State.Humanoid.AutoRotate = true
                                end
                                local lookDir = Vector3.new(rockPos.X, State.HRP.Position.Y, rockPos.Z)
                                State.HRP.CFrame = CFrame.lookAt(State.HRP.Position, lookDir)
                            end
                        end

                        -- 4. Mining Execution: swing pickaxe when locked in stance (use horizontal reach so tall crystals/icebergs always count)
                        local maxReach = math.max(primary.Size.X, primary.Size.Z) * 0.5 + 8.0
                        local horizToRock = State.HRP and Vector3.new(State.HRP.Position.X - rockPos.X, 0, State.HRP.Position.Z - rockPos.Z).Magnitude or distToRock
                        local canMine = (horizToRock <= maxReach) and (horizDist <= 2.5)

                        if canMine then
                            -- Ore Peek check BEFORE swinging so we never accidentally finish a rock holding junk ores
                            if MiningEngine.EvaluateRockOreFilter(rock) == "skipped" then
                                task.wait(0.08)
                                continue
                            end

                            -- Execute swing hit
                            MiningEngine.ExecuteMineHit()
                            MiningEngine.SwingsOnTarget = (MiningEngine.SwingsOnTarget or 0) + 1

                            -- Damage Tracking (Wall-clock time in stance, accounting for 0.50s-0.85s server swing windup)
                            local curHp = rock:GetAttribute("Health")
                            if curHp and MiningEngine.TargetLastHp and curHp < MiningEngine.TargetLastHp then
                                -- Hit connected! Reset zero-damage timer and record new HP
                                MiningEngine.TargetLastHp = curHp
                                MiningEngine.TargetLastDamageTime = os.clock()
                                MiningEngine.NoDamageSwings = 0
                            else
                                if curHp and not MiningEngine.TargetLastHp then
                                    MiningEngine.TargetLastHp = curHp
                                end
                                if not MiningEngine.TargetLastDamageTime then
                                    MiningEngine.TargetLastDamageTime = os.clock()
                                end

                                -- TRUE WALL-CLOCK FAST SKIP PROTOCOL:
                                -- A single pickaxe swing takes ~0.85s on the server. Wait at least 3.2s (or user setting)
                                -- of continuous zero-damage time IN STANCE before blacklisting and skipping to another rock.
                                local skipSeconds = math.max(Config.StuckRockThreshold or 4.0, 2.8)
                                if (os.clock() - MiningEngine.TargetLastDamageTime) >= skipSeconds then
                                    MiningEngine.SafeUnanchor()
                                    local blDuration = Config.StuckRockBlacklistTime or 45
                                    MiningEngine.Blacklist[rock] = os.clock() + blDuration
                                    MiningEngine.SetRockLocalCollision(rock, true)
                                    NavEngine.CachedRock = nil
                                    State.TargetRock = nil
                                    State.CurrentRock = nil
                                    State.LastMinedRock = nil
                                    State.ResumingAfterDeath = false
                                    MiningEngine.TargetLastHp = nil
                                    MiningEngine.TargetLastDamageTime = nil
                                    MiningEngine.SwingsOnTarget = 0
                                    MiningEngine.NoDamageSwings = 0

                                    -- Immediately latch next best rock
                                    local nextRock = MiningEngine.FindBestRock()
                                    if nextRock then
                                        State.TargetRock = nextRock
                                        State.CurrentRock = nextRock
                                        State.LastMinedRock = nextRock
                                        MiningEngine.TargetLastHp = nextRock:GetAttribute("Health")
                                        MiningEngine.TargetLastDamageTime = os.clock()
                                    end
                                end
                            end

                            -- Check if this hit destroyed the rock
                            if not MiningEngine.IsRockAlive(rock) then
                                MiningEngine.SafeUnanchor()
                                MiningEngine.SetRockLocalCollision(rock, true)
                                NavEngine.CachedRock = nil
                                State.TargetRock = nil
                                State.CurrentRock = nil
                                State.LastMinedRock = nil
                                State.ResumingAfterDeath = false
                                MiningEngine.TargetLastHp = nil
                                MiningEngine.TargetLastDamageTime = nil
                                MiningEngine.SwingsOnTarget = 0
                                MiningEngine.NoDamageSwings = 0
                                task.wait(0.18) -- Brief delay for ore drop replication before next rock
                            end
                        end
                    end
                end
            else
                -- No alive target rock currently available in range
                if Config.FilterRocksByOre then
                    local waitingCount = MiningEngine.GetActiveSkippedCount()
                    if waitingCount > 0 then
                        MiningEngine.SetPeekStatus(
                            "Waiting for Rock Regen...",
                            string.format("%d skipped rock(s) regenerating to 100%% HP", waitingCount)
                        )
                    end
                end
                if not Config.AutoMobFarm and not State.IsInRaid and not Config.AutoRaid then
                    MiningEngine.SafeUnanchor()
                end
                State.TargetRock = nil
                State.CurrentRock = nil
                State.LastMinedRock = nil
                State.ResumingAfterDeath = false
            end
        else
            if not Config.AutoMobFarm and not State.IsInRaid and not Config.AutoRaid then
                if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
                if State.Humanoid and not State.Humanoid.AutoRotate then State.Humanoid.AutoRotate = true end
            end
            -- Preserve LastMinedRock and TargetRock while dead/respawning so UpdateCharacter can return to the same rock
            if not Config.AutoMine or State.IsAutoForging or State.DrainingFullBag or State.IsCrafting or State.IsBuyingPotions then
                State.TargetRock = nil
                State.CurrentRock = nil
                State.LastMinedRock = nil
                State.ResumingAfterDeath = false
            end
        end
        task.wait(Config.MineDelay)
    end
end)

-- ===================================================================
-- 8. FORGE & MINIGAME AUTOMATION (Perfect QTE Auto-Solve & Balancer)
-- ===================================================================
local ForgeEngine = {
    CraftRecipes = {
        "None", "Eye Ore", "Demonite", "Magmaite", "Mythril", "Darkryte",
        "Lightite", "Rainbow Crystal", "Arcane Crystal", "Heat Steel",
        "Voidstar", "Etherealite", "Suryafal", "Heavenite", "Gargantuan",
        "Aetherit", "Tiger's Eye", "Earthite"
    },
    OreDataByName = {},
    AllOreNames = { "None" },
    ForgeableOreNames = {},
    SellableOreNames = {},
    CalcSummaryLabel = nil,
    CalcChancesLabel = nil,
    CalcAltCompareLabel = nil,
    AutoForgeStatusLabel = nil,
    ArmorSellStatusLabel = nil,
    StashSellStatusLabel = nil,
    SlotDropdowns = {},
    SlotSliders = {},
    HuntSlotDropdowns = {},
    SessionEpoch = 0,
}
State.ForgeEngine = ForgeEngine

-- Populate ForgeEngine.OreDataByName, AllOreNames (calc), and SellableOreNames (selling)
function ForgeEngine.EnsureOreCatalog()
    if ForgeEngine.OreCatalogComplete and #ForgeEngine.SellableOreNames >= 140 then
        return true
    end
    local prevCount = #ForgeEngine.SellableOreNames
    runIsolated(function()
        local oreMod = ReplicatedStorage:FindFirstChild("Shared")
            and ReplicatedStorage.Shared:FindFirstChild("Data")
            and ReplicatedStorage.Shared.Data:FindFirstChild("Ore")
        if not oreMod then return end

        local function registerOreDef(o)
            if type(o) == "table" and type(o.Name) == "string" and o.Name ~= "" then
                ForgeEngine.OreDataByName[o.Name] = o
            end
        end

        local OreList = safeRequire(oreMod)
        if type(OreList) == "table" then
            for _, o in pairs(OreList) do
                registerOreDef(o)
            end
        end
        -- Also scan child ModuleScripts directly in case Shared.Data.Ore was required before all 158 children replicated
        for _, desc in ipairs(oreMod:GetDescendants()) do
            if desc:IsA("ModuleScript") and not ForgeEngine.OreDataByName[desc.Name] then
                local o = safeRequire(desc)
                registerOreDef(o)
                if type(OreList) == "table" and type(o) == "table" then
                    table.insert(OreList, o)
                end
            end
        end

        local sortable = {}
        local sellSortable = {}
        local sellSeen = {}
        for _, o in pairs(ForgeEngine.OreDataByName) do
            if type(o) == "table" and o.Name then
                sellSeen[o.Name] = true
                table.insert(sellSortable, o)
                if (o.Multiplier or 0) > 0 then
                    table.insert(sortable, o)
                end
            end
        end
        if #sellSortable == 0 then return end

        table.sort(sortable, function(a, b)
            local pa = a.PriceMultiplier or a.Multiplier or 0
            local pb = b.PriceMultiplier or b.Multiplier or 0
            if pa ~= pb then return pa > pb end
            return a.Name < b.Name
        end)
        table.clear(ForgeEngine.AllOreNames)
        table.insert(ForgeEngine.AllOreNames, "None")
        table.clear(ForgeEngine.ForgeableOreNames)
        for _, o in ipairs(sortable) do
            table.insert(ForgeEngine.AllOreNames, o.Name)
            table.insert(ForgeEngine.ForgeableOreNames, o.Name)
        end

        -- Sort sellable ores ascending by PriceMultiplier so common/filler ores appear right at the top
        table.sort(sellSortable, function(a, b)
            local pa = a.PriceMultiplier or a.Multiplier or 0
            local pb = b.PriceMultiplier or b.Multiplier or 0
            if pa ~= pb then return pa < pb end
            return a.Name < b.Name
        end)
        table.clear(ForgeEngine.SellableOreNames)
        for _, o in ipairs(sellSortable) do
            table.insert(ForgeEngine.SellableOreNames, o.Name)
        end
        -- Ensure any saved SellOreWhitelist items remain present in the dropdown list even during early streaming
        if type(Config.SellOreWhitelist) == "table" then
            for k, v in pairs(Config.SellOreWhitelist) do
                local savedName = (type(k) == "number" and type(v) == "string" and v) or (type(k) == "string" and v == true and k) or nil
                if savedName and savedName ~= "" and savedName ~= "None" and not sellSeen[savedName] then
                    sellSeen[savedName] = true
                    table.insert(ForgeEngine.SellableOreNames, savedName)
                end
            end
        end
        ForgeEngine.OreCatalogComplete = (#sellSortable >= 140)
    end)
    return next(ForgeEngine.OreDataByName) ~= nil, (#ForgeEngine.SellableOreNames > prevCount)
end
ForgeEngine.EnsureOreCatalog()

-- 8A-1. Forge Calculator Math Engine (Exact replica of ItemRarity + Equipments + theforge.gg/calculator)
function ForgeEngine.CalculateRecipe(oresMap, itemType, quality)
    itemType = itemType or "Armor"
    quality = quality or 100
    local totalOres = 0
    local statSum = 0
    local priceSum = 0
    local neutralStat = 0
    local neutralPrice = 0

    for oreName, count in pairs(oresMap or {}) do
        if type(count) == "number" and count > 0 and oreName ~= "None" then
            local o = ForgeEngine.OreDataByName[oreName]
            if o then
                local m = o.Multiplier or 0
                local pm = o.PriceMultiplier or m
                if not o.NeutralMultiplier then
                    statSum = statSum + m * count
                    priceSum = priceSum + pm * count
                    totalOres = totalOres + count
                else
                    neutralStat = neutralStat + m * count
                    neutralPrice = neutralPrice + pm * count
                end
            end
        end
    end

    if totalOres < 3 then
        return {
            valid = false,
            itemType = itemType,
            totalOres = totalOres,
            statMult = 0,
            priceMult = 0,
            expectedPrice = 0,
            goldPerOre = 0,
            minPrice = 0,
            maxPrice = 0,
            expectedStat = 0,
            minStat = 0,
            maxStat = 0,
            probabilities = {},
            traits = {}
        }
    end

    local statOreAvg = (totalOres > 0) and (statSum / totalOres) or neutralStat
    local priceOreAvg = (totalOres > 0) and (priceSum / totalOres) or neutralPrice
    local statMult = statOreAvg * (1 + quality / 100)
    local priceMult = priceOreAvg * (0.5 + quality / 200)

    local okIR, ItemRarity = pcall(function() return safeRequire(ReplicatedStorage.Shared.ItemRarity) end)
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)

    local probs = {}
    local expectedBasePrice = 0
    local minBasePrice = math.huge
    local maxBasePrice = 0
    local expectedBaseStat = 0
    local minBaseStat = math.huge
    local maxBaseStat = 0

    if okIR and ItemRarity and ItemRarity[itemType] then
        local clonedPool = {}
        for _, entry in ipairs(ItemRarity[itemType]) do
            table.insert(clonedPool, table.clone(entry))
        end
        local rawProbs = ItemRarity:CalculateProbabilities(totalOres, clonedPool)
        for _, p in ipairs(rawProbs) do
            if p.chance > 0.05 then
                local bt = okEQ and Equipments.BaseTypes and Equipments.BaseTypes[p.name]
                local bPrice = (bt and bt.Price) or 100
                local bStat = 0
                if itemType == "Weapon" then
                    bStat = (bt and bt.Damage) or 0
                else
                    local vName = bt and bt.Types and bt.Types[1]
                    local aItem = vName and okEQ and Equipments.Items and Equipments.Items.Armor and Equipments.Items.Armor[vName]
                    bStat = (aItem and aItem.Health) or 0
                end

                expectedBasePrice = expectedBasePrice + bPrice * (p.chance / 100)
                if bPrice < minBasePrice then minBasePrice = bPrice end
                if bPrice > maxBasePrice then maxBasePrice = bPrice end

                if bStat > 0 then
                    expectedBaseStat = expectedBaseStat + bStat * (p.chance / 100)
                    if bStat < minBaseStat then minBaseStat = bStat end
                    if bStat > maxBaseStat then maxBaseStat = bStat end
                end

                table.insert(probs, {
                    name = (bt and bt.Name) or p.name,
                    rawName = p.name,
                    chance = p.chance,
                    estPrice = math.floor(bPrice * priceMult),
                    estStat = bStat * statMult
                })
            end
        end
    end
    if minBasePrice == math.huge then minBasePrice = 0 end
    if minBaseStat == math.huge then minBaseStat = 0 end

    local activeTraits = {}
    for oreName, count in pairs(oresMap or {}) do
        if type(count) == "number" and count > 0 and (count / totalOres) >= 0.10 then
            local o = ForgeEngine.OreDataByName[oreName]
            if o and o.Traits and #o.Traits > 0 then
                local pct = math.floor((count / totalOres) * 100)
                table.insert(activeTraits, string.format("%s (%d%%)", oreName, pct))
            end
        end
    end

    local expectedPrice = expectedBasePrice * priceMult
    return {
        valid = true,
        itemType = itemType,
        totalOres = totalOres,
        statMult = statMult,
        priceMult = priceMult,
        expectedPrice = expectedPrice,
        goldPerOre = expectedPrice / totalOres,
        minPrice = minBasePrice * priceMult,
        maxPrice = maxBasePrice * priceMult,
        expectedStat = expectedBaseStat * statMult,
        minStat = minBaseStat * statMult,
        maxStat = maxBaseStat * statMult,
        probabilities = probs,
        traits = activeTraits
    }
end

function ForgeEngine.GetCalculatorOresMap()
    local map = {}
    local slots = {
        { ore = Config.CalcSlot1Ore, count = Config.CalcSlot1Count },
        { ore = Config.CalcSlot2Ore, count = Config.CalcSlot2Count },
        { ore = Config.CalcSlot3Ore, count = Config.CalcSlot3Count },
        { ore = Config.CalcSlot4Ore, count = Config.CalcSlot4Count }
    }
    for _, s in ipairs(slots) do
        if s.ore and s.ore ~= "None" and (tonumber(s.count) or 0) > 0 then
            map[s.ore] = (map[s.ore] or 0) + math.floor(tonumber(s.count))
        end
    end
    return map
end

function ForgeEngine.CalculateGolem(oresMap, quality)
    quality = quality or 100
    local totalOres = 0
    for oreName, count in pairs(oresMap or {}) do
        if type(count) == "number" and count > 0 and oreName ~= "None" then
            totalOres = totalOres + math.floor(count)
        end
    end
    local okG, Golems = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Golems) end)
    local minOres = (okG and Golems and Golems.MinOres) or 10
    if not okG or not Golems or totalOres < minOres then
        return { valid = false, totalOres = totalOres, minOres = minOres }
    end

    local fractions = {}
    for oreName, count in pairs(oresMap or {}) do
        if type(count) == "number" and count > 0 and oreName ~= "None" then
            fractions[oreName] = count / totalOres
        end
    end

    local rarityName, mainOre, rarityData = "Common", nil, { Name = "Common", MineDamage = 0.1, TraitScale = 0.06 }
    local price, recyclePrice, strength, rawTraits = 0, 0, 0.06, {}
    runAsGameThread(function()
        local okR, rName, mOre = pcall(function() return Golems:GetResult(oresMap) end)
        if okR and rName then
            rarityName = rName
            mainOre = mOre
        end
        local okRD, rd = pcall(function() return Golems:GetRarityData(rarityName or "Common") end)
        if okRD and type(rd) == "table" then
            rarityData = rd
        end
        local golemInput = {
            Ores = fractions,
            OresSpent = oresMap,
            Rarity = rarityData.Name or rarityName or "Common",
            Quality = quality,
        }
        local okP, pVal = pcall(function()
            if type(Golems.GetPrice) == "function" then
                return Golems:GetPrice(golemInput)
            elseif type(Golems.GetBasePrice) == "function" then
                return Golems:GetBasePrice(totalOres)
            end
            return 0
        end)
        if okP and pVal then price = tonumber(pVal) or 0 end

        local okRP, rpVal = pcall(function() return Golems:GetRecyclePrice(golemInput) end)
        if okRP and rpVal then recyclePrice = tonumber(rpVal) or 0 end

        local okS, sVal = pcall(function() return Golems:GetStrength(golemInput) end)
        if okS and sVal then
            strength = tonumber(sVal) or rarityData.TraitScale or 0.06
        else
            strength = rarityData.TraitScale or 0.06
        end

        local okT, tVal = pcall(function() return Golems:GetTraits(golemInput) end)
        if okT and type(tVal) == "table" then rawTraits = tVal end
    end, 1.5)
    local traitParts = {}
    for k, v in pairs(rawTraits) do
        table.insert(traitParts, string.format("%s: %.1f", tostring(k), tonumber(v) or 0))
    end
    table.sort(traitParts)

    return {
        valid = true,
        totalOres = totalOres,
        minOres = minOres,
        rarity = rarityData.Name or rarityName or "Common",
        mainOre = mainOre or "None",
        mineHelpPct = math.round((rarityData.MineDamage or 0.1) * 100),
        traitScalePct = math.round((rarityData.TraitScale or 0.06) * 100),
        strengthPct = strength * 100,
        price = math.floor(price),
        recyclePrice = math.floor(recyclePrice),
        traits = traitParts,
    }
end

function ForgeEngine.GetOptimalGolemBatch(targetCount)
    targetCount = math.max(10, targetCount or 10)
    local owned = ForgeEngine.GetPlayerOreInventory()
    if #owned == 0 then return nil, 0 end

    -- Golems take rarity from MostUsedOre and traits from >=10% share ores, so stack highest-rarity ores first
    local rarityRank = { Common = 1, Uncommon = 2, Rare = 3, Epic = 4, Legendary = 5, Mythic = 6, Exotic = 7, Relic = 8, Divine = 9, Unobtainable = 10 }
    local sorted = {}
    for _, item in ipairs(owned) do
        local info = ForgeEngine.OreDataByName[item.Name]
        table.insert(sorted, {
            Name = item.Name,
            Count = item.Count,
            RarityRank = (info and rarityRank[info.Rarity]) or 1,
            Mult = item.Mult or 0,
        })
    end
    table.sort(sorted, function(a, b)
        if a.Count >= 10 ~= (b.Count >= 10) then return a.Count >= 10 end
        if a.RarityRank ~= b.RarityRank then return a.RarityRank > b.RarityRank end
        if a.Mult ~= b.Mult then return a.Mult > b.Mult end
        return a.Count > b.Count
    end)

    local batch = {}
    local rem = targetCount
    local total = 0
    for i = 1, math.min(4, #sorted) do
        if rem <= 0 then break end
        local take = math.min(sorted[i].Count, rem)
        if take > 0 then
            batch[sorted[i].Name] = take
            rem = rem - take
            total = total + take
        end
    end
    if total < 10 then return nil, total end
    return batch, total
end

function ForgeEngine.UpdateCalculatorUI()
    if not ForgeEngine.CalcSummaryLabel or not ForgeEngine.CalcChancesLabel then return end
    pcall(function()
        local map = ForgeEngine.GetCalculatorOresMap()
        local mode = Config.CalcItemType or "Armor"
        local okGolem, golemRes = pcall(ForgeEngine.CalculateGolem, map, 100)
        if not okGolem or type(golemRes) ~= "table" then
            golemRes = { valid = false, totalOres = 0, minOres = 10 }
        end

        if mode == "Golem" then
            local armorRes = ForgeEngine.CalculateRecipe(map, "Armor", 100)
            local weaponRes = ForgeEngine.CalculateRecipe(map, "Weapon", 100)
            if not golemRes.valid then
                ForgeEngine.CalcSummaryLabel:SetTitle(string.format("[Golem] Need >= %d Ores (Current: %d)", golemRes.minOres, golemRes.totalOres))
                ForgeEngine.CalcSummaryLabel:SetDesc("Golems require at least 10 ores across Slots 1-4; main ore determines Rarity")
                ForgeEngine.CalcChancesLabel:SetTitle("Golem Traits: --")
                ForgeEngine.CalcChancesLabel:SetDesc("Any ore >=10% of the mix transfers scaled traits (TraitScale * Quality)")
            else
                ForgeEngine.CalcSummaryLabel:SetTitle(string.format(
                    "[Golem] %s (%s) | +%d%% Mining | %d%% Traits",
                    tostring(golemRes.mainOre),
                    tostring(golemRes.rarity),
                    golemRes.mineHelpPct,
                    golemRes.traitScalePct
                ))
                ForgeEngine.CalcSummaryLabel:SetDesc(string.format(
                    "100%% Quality Value: $%d | Recycle Value: $%d | %d Ores | Effective Strength: %.1f%%",
                    golemRes.price,
                    golemRes.recyclePrice,
                    golemRes.totalOres,
                    golemRes.strengthPct
                ))
                local traitText = (#golemRes.traits > 0) and table.concat(golemRes.traits, " | ") or "No Ore Traits (Need >=10% of an ore with traits)"
                ForgeEngine.CalcChancesLabel:SetTitle("Golem Active Traits (" .. tostring(#golemRes.traits) .. ")")
                ForgeEngine.CalcChancesLabel:SetDesc(traitText)
            end
            if ForgeEngine.CalcAltCompareLabel then
                if armorRes.valid and weaponRes.valid then
                    ForgeEngine.CalcAltCompareLabel:SetTitle(string.format(
                        "Armor vs Weapon: Armor $%d (%.1f-%.1f HP) | Weapon $%d (%.1f-%.1f Dmg)",
                        math.floor(armorRes.expectedPrice), armorRes.minStat, armorRes.maxStat,
                        math.floor(weaponRes.expectedPrice), weaponRes.minStat, weaponRes.maxStat
                    ))
                    ForgeEngine.CalcAltCompareLabel:SetDesc("Same ore slots evaluated across Armor, Weapon, and Golem")
                else
                    ForgeEngine.CalcAltCompareLabel:SetTitle("Armor / Weapon Comparison: Need >= 3 Ores")
                    ForgeEngine.CalcAltCompareLabel:SetDesc("Same ore slots evaluated across Armor, Weapon, and Golem")
                end
            end
            return
        end

        local altMode = (mode == "Weapon") and "Armor" or "Weapon"
        local res = ForgeEngine.CalculateRecipe(map, mode, 100)
        local altRes = ForgeEngine.CalculateRecipe(map, altMode, 100)

        if not res.valid then
            ForgeEngine.CalcSummaryLabel:SetTitle(string.format("[%s] Need >= 3 Ores (Current: %d)", mode, res.totalOres))
            ForgeEngine.CalcSummaryLabel:SetDesc("Add at least 3 ores across Slots 1-4 to calculate stats & profit")
            ForgeEngine.CalcChancesLabel:SetTitle("Class Chances: --")
            ForgeEngine.CalcChancesLabel:SetDesc("Switch between Armor, Weapon & Golem above or load an optimal recipe")
            if ForgeEngine.CalcAltCompareLabel then
                ForgeEngine.CalcAltCompareLabel:SetTitle(altMode .. " & Golem Preview: Need >= 3 Ores")
                ForgeEngine.CalcAltCompareLabel:SetDesc("Armor, Weapon & Golem stats are calculated live")
            end
            return
        end

        local statLabel = (mode == "Weapon") and "Dmg" or "HP"
        local statPrefix = (mode == "Weapon") and "" or "+"
        local multLabel = (mode == "Weapon") and "Dmg Mult" or "Def Mult"

        ForgeEngine.CalcSummaryLabel:SetTitle(string.format(
            "[%s] Est. Sell: $%s ($%.1f/ore) | %s: %s%.1f - %s%.1f",
            mode,
            tostring(math.floor(res.expectedPrice)),
            res.goldPerOre,
            statLabel,
            statPrefix, res.minStat,
            statPrefix, res.maxStat
        ))
        ForgeEngine.CalcSummaryLabel:SetDesc(string.format(
            "100%% MW %s: %.2fx | Price Mult: %.2fx | %d Ores | Sell Range: $%d - $%d",
            multLabel,
            res.statMult,
            res.priceMult,
            res.totalOres,
            math.floor(res.minPrice),
            math.floor(res.maxPrice)
        ))

        local topParts = {}
        for i = 1, math.min(3, #res.probabilities) do
            local p = res.probabilities[i]
            table.insert(topParts, string.format("%s %.1f%% (%s%.1f %s, $%d)", p.name, p.chance, statPrefix, p.estStat, statLabel, p.estPrice))
        end
        local extraParts = {}
        for i = 4, math.min(6, #res.probabilities) do
            local p = res.probabilities[i]
            table.insert(extraParts, string.format("%s %.1f%% (%s%.1f %s)", p.name, p.chance, statPrefix, p.estStat, statLabel))
        end
        local traitStr = (#res.traits > 0) and ("Traits: " .. table.concat(res.traits, ", ")) or "Traits: None (Need >=10%)"
        local descLine = (#extraParts > 0) and (table.concat(extraParts, " | ") .. "  •  " .. traitStr) or traitStr

        ForgeEngine.CalcChancesLabel:SetTitle(table.concat(topParts, " | "))
        ForgeEngine.CalcChancesLabel:SetDesc(descLine)

        if ForgeEngine.CalcAltCompareLabel and altRes and altRes.valid then
            local altStatLbl = (altMode == "Weapon") and "Dmg" or "HP"
            local altPrefix = (altMode == "Weapon") and "" or "+"
            local altTop = {}
            for i = 1, math.min(3, #altRes.probabilities) do
                local p = altRes.probabilities[i]
                table.insert(altTop, string.format("%s %.1f%% (%s%.1f %s, $%d)", p.name, p.chance, altPrefix, p.estStat, altStatLbl, p.estPrice))
            end
            local golemInline = golemRes.valid
                and string.format(" | Golem: %s (%s, +%d%% Mine, $%d)", golemRes.mainOre, golemRes.rarity, golemRes.mineHelpPct, golemRes.price)
                or string.format(" | Golem: %d/%d ores", golemRes.totalOres, golemRes.minOres)
            ForgeEngine.CalcAltCompareLabel:SetTitle(string.format(
                "%s Comparison (%d Ores): Est. $%s ($%.1f/ore) | %s: %s%.1f - %s%.1f",
                altMode,
                altRes.totalOres,
                tostring(math.floor(altRes.expectedPrice)),
                altRes.goldPerOre,
                altStatLbl,
                altPrefix, altRes.minStat,
                altPrefix, altRes.maxStat
            ))
            ForgeEngine.CalcAltCompareLabel:SetDesc(table.concat(altTop, " | ") .. golemInline)
        end
    end)
end

-- Returns set of ore names blacklisted from ALL forging actions
function ForgeEngine.GetBlacklistedForgeOresMap()
    local map = {}
    local bl = Config.ForgeOreBlacklist
    if type(bl) == "table" then
        for k, v in pairs(bl) do
            local name = (type(k) == "number" and type(v) == "string" and v) or (type(k) == "string" and v == true and k) or nil
            if name and name ~= "" and name ~= "None" then
                map[name] = true
            end
        end
    elseif type(bl) == "string" and bl ~= "" and bl ~= "None" then
        map[bl] = true
    end
    return map
end

-- 8A-2. Mathematical Optimal Armor Batch Selector (Maximizes Gold Earned from Live Player Inventory)
function ForgeEngine.GetPlayerOreInventory(ignoreBlacklist, includeFavorited)
    ForgeEngine.EnsureOreCatalog()
    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local inv = data and data.Inventory
    local favs = (data and data.Favorites) or {}
    local list = {}
    if not inv then return list end
    local blacklisted = (not ignoreBlacklist) and ForgeEngine.GetBlacklistedForgeOresMap() or {}
    local itemTypes = (typeof(getrenv) == "function" and getrenv()._G and getrenv()._G.ItemTypes) or _G.ItemTypes
    for k, v in pairs(inv) do
        if type(v) == "number" and math.floor(v) >= 1 and not blacklisted[k] and (includeFavorited or not favs[k]) then
            local o = ForgeEngine.OreDataByName[k]
            local isOre = (o ~= nil) or (type(itemTypes) == "table" and type(itemTypes[k]) == "table" and itemTypes[k][1] == "Ore")
            if isOre then
                table.insert(list, {
                    Name = k,
                    Count = math.floor(v),
                    Mult = (o and o.Multiplier) or 0,
                    PriceMult = (o and (o.PriceMultiplier or o.Multiplier)) or 0
                })
            end
        end
    end
    -- Sort by PriceMultiplier descending so highest-value ores are prioritized
    table.sort(list, function(a, b)
        if a.PriceMult ~= b.PriceMult then return a.PriceMult > b.PriceMult end
        return a.Count > b.Count
    end)
    return list
end

function ForgeEngine.GetReservedHuntOreMap()
    local map = {}
    if (Config.WeaponHuntOverride or Config.WeaponHuntRunning) and Config.WeaponHuntReserveOres ~= false then
        for idx = 1, 4 do
            local oreName = Config["WeaponHuntOre" .. tostring(idx)]
            if type(oreName) == "string" and oreName ~= "" and oreName ~= "None" and oreName ~= "Auto: Best From Inventory" then
                map[oreName] = true
            end
        end
    end
    return map
end

function ForgeEngine.GetOptimalArmorBatch(targetCount)
    targetCount = targetCount or Config.AutoForgeTargetOres or 31
    local rawOwned = ForgeEngine.GetPlayerOreInventory()
    if #rawOwned == 0 then return nil, 0 end

    local reserved = ForgeEngine.GetReservedHuntOreMap()
    local owned = {}
    for _, ore in ipairs(rawOwned) do
        if not reserved[ore.Name] then
            table.insert(owned, ore)
        end
    end
    if #owned == 0 then return nil, 0 end

    local totalUsable = 0
    for _, ore in ipairs(owned) do
        totalUsable = totalUsable + (ore.Count or 0)
    end
    if totalUsable < 3 then return nil, totalUsable end

    -- Evaluate up to top 12 ore stacks across 4 crucible slots to find the combination
    -- that reaches targetCount (or highest reachable breakpoint >= 3) with maximum expected sell value
    local pool = {}
    for i = 1, math.min(12, #owned) do
        table.insert(pool, owned[i])
    end

    -- Also ensure the 4 largest-quantity stacks are in the candidate pool so we can always reach 31+ ores
    local byCount = table.clone(owned)
    table.sort(byCount, function(a, b) return a.Count > b.Count end)
    for i = 1, math.min(4, #byCount) do
        local found = false
        for _, p in ipairs(pool) do
            if p.Name == byCount[i].Name then found = true; break end
        end
        if not found then table.insert(pool, byCount[i]) end
    end

    local bestBatch = nil
    local bestScore = -1
    local bestTotal = 0

    local function evaluateSubset(subset)
        local avail = 0
        for _, s in ipairs(subset) do avail = avail + s.Count end
        if avail < 3 then return end

        local cap = math.min(avail, targetCount)
        -- Sort subset by PriceMult descending so we take highest-multiplier ores first up to cap
        local sortedSub = table.clone(subset)
        table.sort(sortedSub, function(a, b) return a.PriceMult > b.PriceMult end)

        local batch = {}
        local rem = cap
        for _, s in ipairs(sortedSub) do
            if rem <= 0 then break end
            local take = math.min(s.Count, rem)
            if take > 0 then
                batch[s.Name] = take
                rem = rem - take
            end
        end

        local calc = ForgeEngine.CalculateRecipe(batch, "Armor", 100)
        if calc.valid then
            -- Score prioritizes reaching targetCount first, then maximizes Expected Sell Price & Gold/Ore
            local reachBonus = (calc.totalOres >= targetCount) and 1000000 or (calc.totalOres * 10000)
            local score = reachBonus + calc.expectedPrice
            if score > bestScore then
                bestScore = score
                bestBatch = batch
                bestTotal = calc.totalOres
            end
        end
    end

    local m = #pool
    if m <= 4 then
        evaluateSubset(pool)
    else
        for i = 1, m - 3 do
            for j = i + 1, m - 2 do
                for k = j + 1, m - 1 do
                    for l = k + 1, m do
                        evaluateSubset({ pool[i], pool[j], pool[k], pool[l] })
                    end
                end
            end
        end
    end

    -- Greedy fallback: ensure any >= 3 non-blacklisted ores always produce a 1-4 slot batch
    if not bestBatch or bestTotal < 3 then
        local fbBatch = {}
        local fbTotal = 0
        local cap = math.min(totalUsable, targetCount)
        local usedSlots = 0
        -- Try highest-count stacks first if top-4 by PriceMult can't reach 3, otherwise top PriceMult
        local candidates = owned
        local top4MultSum = 0
        for i = 1, math.min(4, #owned) do
            top4MultSum = top4MultSum + (owned[i].Count or 0)
        end
        if top4MultSum < 3 then
            candidates = table.clone(owned)
            table.sort(candidates, function(a, b) return (a.Count or 0) > (b.Count or 0) end)
        end
        for _, ore in ipairs(candidates) do
            if usedSlots >= 4 or fbTotal >= cap then break end
            local take = math.min(ore.Count or 0, cap - fbTotal)
            if take > 0 then
                fbBatch[ore.Name] = take
                fbTotal = fbTotal + take
                usedSlots = usedSlots + 1
            end
        end
        if fbTotal >= 3 then
            return fbBatch, fbTotal
        end
    end

    return bestBatch, bestTotal
end

-- Optimal Weapon Batch Selector (Highest damage-multiplier ores from live inventory, up to targetCount)
function ForgeEngine.GetOptimalWeaponBatch(targetCount)
    targetCount = targetCount or 35
    local owned = ForgeEngine.GetPlayerOreInventory()
    if #owned == 0 then return nil, 0 end

    local byMult = table.clone(owned)
    table.sort(byMult, function(a, b)
        if a.Mult ~= b.Mult then return a.Mult > b.Mult end
        return a.Count > b.Count
    end)

    local batch = {}
    local rem = targetCount
    local total = 0
    for i = 1, math.min(4, #byMult) do
        if rem <= 0 then break end
        local take = math.min(byMult[i].Count, rem)
        if take > 0 then
            batch[byMult[i].Name] = take
            rem = rem - take
            total = total + take
        end
    end
    if total < 3 then return nil, total end
    return batch, total
end

-- Helpers for Custom / Main Weapon protection & Base DPS comparison
ForgeEngine.ProtectedWeaponLabels = { "Auto (Equipped Weapon)" }
ForgeEngine.ProtectedWeaponMap = {}
ForgeEngine.ProtectedWeaponDropdown = nil
ForgeEngine.ProtectedArmorLabels = { "Auto (Equipped Armor)" }
ForgeEngine.ProtectedArmorMap = {}
ForgeEngine.ProtectedArmorDropdown = nil

function ForgeEngine.SafeCalcPrice(Equipments, item)
    if not Equipments or type(item) ~= "table" then return 0 end
    local price = 0
    local ok, val = pcall(function() return Equipments:CalculatePrice(item) end)
    if ok and type(val) == "number" then
        return val
    end
    runAsGameThread(function()
        local ok2, val2 = pcall(function() return Equipments:CalculatePrice(item) end)
        if ok2 and type(val2) == "number" then
            price = val2
        end
    end, 2.0)
    return price
end

function ForgeEngine.GetOwnedWeaponLabels()
    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local eqList = data and data.Inventory and data.Inventory.Equipments
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
    local equippedGuid = data and data.Equipped and data.Equipped.Weapon and data.Equipped.Weapon.GUID or nil

    table.clear(ForgeEngine.ProtectedWeaponLabels)
    table.clear(ForgeEngine.ProtectedWeaponMap)
    table.insert(ForgeEngine.ProtectedWeaponLabels, "Auto (Equipped Weapon)")

    if okEQ and Equipments and type(eqList) == "table" then
        for _, item in ipairs(eqList) do
            if type(item) == "table" and item.GUID and item.Type and Equipments:GetItemType(item.Type) == "Weapon" then
                local prefix = (item.GUID == equippedGuid) and "[Eq] " or ""
                local upg = (tonumber(item.Upgrade) or 0) > 0 and (" +" .. tostring(item.Upgrade)) or ""
                local lbl = string.format("%s%s%s #%s", prefix, tostring(item.Type), upg, tostring(item.GUID):sub(1, 4))
                ForgeEngine.ProtectedWeaponMap[lbl] = item.GUID
                table.insert(ForgeEngine.ProtectedWeaponLabels, lbl)
            end
        end
    end
    return ForgeEngine.ProtectedWeaponLabels
end

function ForgeEngine.IsMainWeaponProtected()
    if not Config.ProtectCustomWeapon then return false, nil end
    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local equipped = data and data.Equipped and data.Equipped.Weapon
    local choice = Config.ProtectedWeaponChoice or "Auto (Equipped Weapon)"
    if choice == "Auto (Equipped Weapon)" then
        return (type(equipped) == "table" and equipped.Type ~= nil), equipped
    end
    if not ForgeEngine.ProtectedWeaponMap[choice] then
        ForgeEngine.GetOwnedWeaponLabels()
    end
    local targetGuid = ForgeEngine.ProtectedWeaponMap[choice]
    local guidHint = tostring(choice):match("#([%w%-]+)$")
    local eqList = data and data.Inventory and data.Inventory.Equipments or {}
    for _, item in ipairs(eqList) do
        if type(item) == "table" and item.GUID then
            if (targetGuid and item.GUID == targetGuid) or (guidHint and tostring(item.GUID):sub(1, #guidHint) == guidHint) then
                return true, item
            end
        end
    end
    return (type(equipped) == "table" and equipped.Type ~= nil), equipped
end

function ForgeEngine.GetOwnedArmorLabels()
    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local eqList = data and data.Inventory and data.Inventory.Equipments
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
    local equippedGuids = {}
    if data and type(data.Equipped) == "table" then
        for _, slot in ipairs({ "Helmet", "Chestplate", "Leggings" }) do
            local worn = data.Equipped[slot]
            if type(worn) == "table" and worn.GUID then
                equippedGuids[worn.GUID] = true
            end
        end
    end

    table.clear(ForgeEngine.ProtectedArmorLabels)
    table.clear(ForgeEngine.ProtectedArmorMap)
    table.insert(ForgeEngine.ProtectedArmorLabels, "Auto (Equipped Armor)")

    if okEQ and Equipments and type(eqList) == "table" then
        for _, item in ipairs(eqList) do
            if type(item) == "table" and item.GUID and item.Type and Equipments:GetItemType(item.Type) == "Armor" then
                local prefix = equippedGuids[item.GUID] and "[Eq] " or ""
                local upg = (tonumber(item.Upgrade) or 0) > 0 and (" +" .. tostring(item.Upgrade)) or ""
                local lbl = string.format("%s%s%s #%s", prefix, tostring(item.Type), upg, tostring(item.GUID):sub(1, 4))
                ForgeEngine.ProtectedArmorMap[lbl] = item.GUID
                table.insert(ForgeEngine.ProtectedArmorLabels, lbl)
            end
        end
    end
    return ForgeEngine.ProtectedArmorLabels
end

function ForgeEngine.GetProtectedArmorSets(Equipments)
    local protGuids = {}
    local protSlots = {}
    if not Config.ProtectCustomArmor then return protGuids, protSlots end
    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local curEq = data and data.Equipped
    local choices = type(Config.ProtectedArmorChoices) == "table" and Config.ProtectedArmorChoices or { "Auto (Equipped Armor)" }
    local protectAuto = (#choices == 0)
    local needsRebuild = false
    for _, lbl in ipairs(choices) do
        if lbl ~= "Auto (Equipped Armor)" and not ForgeEngine.ProtectedArmorMap[lbl] then
            needsRebuild = true
            break
        end
    end
    if needsRebuild then
        ForgeEngine.GetOwnedArmorLabels()
    end
    local hintPrefixes = {}
    for _, lbl in ipairs(choices) do
        if lbl == "Auto (Equipped Armor)" then
            protectAuto = true
        else
            local guid = ForgeEngine.ProtectedArmorMap[lbl]
            if guid then
                protGuids[guid] = true
            else
                local hint = tostring(lbl):match("#([%w%-]+)$")
                if hint then table.insert(hintPrefixes, hint) end
            end
        end
    end
    if protectAuto and type(curEq) == "table" then
        for _, slot in ipairs({ "Helmet", "Chestplate", "Leggings" }) do
            local worn = curEq[slot]
            if type(worn) == "table" and worn.GUID then
                protGuids[worn.GUID] = true
                protSlots[slot] = true
            end
        end
    end
    if type(data and data.Inventory and data.Inventory.Equipments) == "table" then
        for _, item in ipairs(data.Inventory.Equipments) do
            if type(item) == "table" and item.GUID then
                if not protGuids[item.GUID] and #hintPrefixes > 0 then
                    for _, hint in ipairs(hintPrefixes) do
                        if tostring(item.GUID):sub(1, #hint) == hint then
                            protGuids[item.GUID] = true
                            break
                        end
                    end
                end
                if Equipments and protGuids[item.GUID] and item.Type then
                    local iData = Equipments:GetItemData(item.Type)
                    local slot = iData and iData.EquipSlot
                    if slot then protSlots[slot] = true end
                end
            end
        end
    end
    return protGuids, protSlots
end

ForgeEngine.WeaponClassDisplayList = {
    "Any Class",
    "Keep Current Class",
    "Dagger",
    "Straight Sword",
    "Mace",
    "Gauntlet",
    "Sword & Shield",
    "Axe",
    "Katana",
    "Great Sword",
    "Spear",
    "Great Axe",
    "Halberd",
    "Colossal Sword",
}

ForgeEngine.WeaponClassNameToKey = {
    ["Dagger"] = "Dagger",
    ["Straight Sword"] = "StraightSword",
    ["Mace"] = "Mace",
    ["Gauntlet"] = "Gauntlet",
    ["Sword & Shield"] = "SwordShield",
    ["Axe"] = "Axe",
    ["Katana"] = "Katana",
    ["Great Sword"] = "GreatSword",
    ["Spear"] = "Spear",
    ["Great Axe"] = "GreatAxe",
    ["Halberd"] = "Halberd",
    ["Colossal Sword"] = "ColossalSword",
}

function ForgeEngine.GetWeaponClassKey(Equipments, weaponItem)
    if not Equipments or type(weaponItem) ~= "table" or not weaponItem.Type then return nil end
    for k, bt in pairs(Equipments.BaseTypes or {}) do
        if type(bt) == "table" and ((bt.Types and table.find(bt.Types, weaponItem.Type)) or k == weaponItem.Type) then
            return k
        end
    end
    return nil
end


function ForgeEngine.ResolveTargetWeaponClassKey(Equipments, equippedWeapon)
    local sel = Config.WeaponClassTarget or "Any Class"
    if sel == "Any Class" then return nil end
    if sel == "Keep Current Class" then
        return ForgeEngine.GetWeaponClassKey(Equipments, equippedWeapon)
    end
    return ForgeEngine.WeaponClassNameToKey[sel]
end

function ForgeEngine.GetWeaponBaseStats(Equipments, weaponItem)
    if not Equipments or type(weaponItem) ~= "table" or not weaponItem.Type then
        return 0, 0
    end
    local okD, flatDmg = pcall(function() return Equipments:CalculateFlatDamage(weaponItem) end)
    flatDmg = (okD and type(flatDmg) == "number") and flatDmg or 0
    local spd = 0.7
    for k, bt in pairs(Equipments.BaseTypes or {}) do
        if type(bt) == "table" and ((bt.Types and table.find(bt.Types, weaponItem.Type)) or k == weaponItem.Type) then
            spd = (bt.Info and tonumber(bt.Info.AttackSpeed)) or spd
            break
        end
    end
    local wData = Equipments.Items and Equipments.Items.Weapon and Equipments.Items.Weapon[weaponItem.Type]
    local spdMult = (wData and tonumber(wData.AttackSpeedMultiplier)) or 1.0
    local effSpd = math.max(0.2, spd / math.max(0.2, spdMult))
    return flatDmg / effSpd, flatDmg
end

function ForgeEngine.GetWeaponCompareScore(Equipments, weaponItem)
    local dps, flat = ForgeEngine.GetWeaponBaseStats(Equipments, weaponItem)
    local score = (Config.WeaponCompareMode == "Base Damage") and flat or dps
    return score, dps, flat
end

-- Returns highest-scoring owned Weapon matching WeaponClassTarget & WeaponCompareMode
function ForgeEngine.GetBestOwnedWeaponBaseDamage()
    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local eqList = data and data.Inventory and data.Inventory.Equipments
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
    if not okEQ or not Equipments or not eqList then return nil, 0, 0, 0 end

    local equippedWeapon = data.Equipped and data.Equipped.Weapon
    local targetClassKey = ForgeEngine.ResolveTargetWeaponClassKey(Equipments, equippedWeapon)
    local eqClassKey = ForgeEngine.GetWeaponClassKey(Equipments, equippedWeapon)

    local bestItem = nil
    local bestScore, bestDPS, bestFlat = 0, 0, 0
    if type(equippedWeapon) == "table" and equippedWeapon.Type then
        if not targetClassKey or eqClassKey == targetClassKey then
            bestItem = equippedWeapon
            bestScore, bestDPS, bestFlat = ForgeEngine.GetWeaponCompareScore(Equipments, equippedWeapon)
        end
    end

    for _, item in ipairs(eqList) do
        if type(item) == "table" and item.GUID and item.Type then
            if Equipments:GetItemType(item.Type) == "Weapon" then
                local cls = ForgeEngine.GetWeaponClassKey(Equipments, item)
                if not targetClassKey or cls == targetClassKey then
                    local score, dps, flat = ForgeEngine.GetWeaponCompareScore(Equipments, item)
                    if score > bestScore then
                        bestScore = score
                        bestDPS = dps
                        bestFlat = flat
                        bestItem = item
                    end
                end
            end
        end
    end
    return bestItem, bestScore, bestDPS, bestFlat, targetClassKey
end

-- Evaluates weapon ore counts (3..55) using top Damage-Mult ores and picks optimal batch for Target Class & Compare Mode
function ForgeEngine.GetHighestBaseWeaponBatch(targetClassKey)
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
    local compareDmg = (Config.WeaponCompareMode == "Base Damage")
    local bestBatch, bestCalc, bestTotal = nil, nil, 0
    local bestClassChance = -1
    local bestMetric = 0

    for _, count in ipairs({ 3, 4, 5, 6, 8, 9, 12, 16, 20, 24, 30, 35, 45, 55 }) do
        local cand, total = ForgeEngine.GetOptimalWeaponBatch(count)
        if cand and total >= 3 then
            local calc = ForgeEngine.CalculateRecipe(cand, "Weapon", 100)
            if calc and calc.valid then
                local expDPS = 0
                local classChance = 0
                local classEstStat = 0
                for _, p in ipairs(calc.probabilities or {}) do
                    local bt = okEQ and Equipments and Equipments.BaseTypes and Equipments.BaseTypes[p.rawName]
                    local spd = (bt and bt.Info and tonumber(bt.Info.AttackSpeed)) or 0.7
                    expDPS = expDPS + (p.estStat / math.max(0.2, spd)) * (p.chance / 100)
                    if targetClassKey and p.rawName == targetClassKey then
                        classChance = p.chance or 0
                        classEstStat = compareDmg and (p.estStat or 0) or ((p.estStat or 0) / math.max(0.2, spd))
                    end
                end
                calc.expectedDPS = expDPS
                local expMetric = compareDmg and (calc.expectedStat or 0) or expDPS
                calc.compareMetric = targetClassKey and (classEstStat > 0 and classEstStat or expMetric) or expMetric

                if targetClassKey then
                    -- Maximize chance of rolling the chosen class first, then highest Base DPS/DMG
                    if classChance > bestClassChance or (math.abs(classChance - bestClassChance) < 0.5 and calc.compareMetric > bestMetric) then
                        bestClassChance = classChance
                        bestMetric = calc.compareMetric
                        bestBatch = cand
                        bestCalc = calc
                        bestTotal = total
                    end
                elseif expMetric > bestMetric then
                    bestMetric = expMetric
                    bestBatch = cand
                    bestCalc = calc
                    bestTotal = total
                end
            end
        end
    end
    return bestBatch, bestCalc, bestTotal
end

-- Before forging Armor in a cycle: crafts a Weapon ONLY if custom weapon isn't protected and expected score
-- strictly beats current best matching Weapon, then checks actual RNG class & stat post-craft
function ForgeEngine.TryCraftBestWeaponIfUpgrade()
    if Config.WeaponHuntOverride then
        return ForgeEngine.RunWeaponHunt1111({ triggeredByOverride = true })
    end
    local isProtected, protItem = ForgeEngine.IsMainWeaponProtected()
    if isProtected then
        ForgeEngine.SetStatus(
            "Main Weapon Protected (" .. tostring(protItem and protItem.Type or "Equipped") .. ")",
            "Skipped weapon crafting -> Proceeding to Armor"
        )
        return false
    end

    local _, curBestScore, _, _, targetClassKey = ForgeEngine.GetBestOwnedWeaponBaseDamage()
    local candBatch, candCalc, candTotal = ForgeEngine.GetHighestBaseWeaponBatch(targetClassKey)
    if not candBatch or not candCalc or not candCalc.valid or candTotal < 3 then
        return false
    end

    local modeLabel = (Config.WeaponCompareMode == "Base Damage") and "DMG" or "DPS"
    local expMetric = candCalc.compareMetric or candCalc.expectedDPS or candCalc.expectedStat or 0
    -- Skip crafting if expected score isn't strictly higher than current matching weapon's score
    if curBestScore > 0 and expMetric <= (curBestScore * 1.02) then
        ForgeEngine.SetStatus(
            string.format("Weapon Check: Keeping Current (%.1f %s)", curBestScore, modeLabel),
            string.format("Best craftable expected ~%.1f %s -> Starting Armor", expMetric, modeLabel)
        )
        return false
    end

    ForgeEngine.SetStatus(
        string.format("Crafting Better Weapon (~%.1f vs %.1f %s)", expMetric, curBestScore, modeLabel),
        string.format("Forging %d-ore Weapon before Armor...", candTotal)
    )

    State.LastNewlyForgedItems = nil
    local forgedOk = ForgeEngine.ExecuteArmorForgeAndSell(candBatch, "Weapon", true)
    if not forgedOk or type(State.LastNewlyForgedItems) ~= "table" or #State.LastNewlyForgedItems == 0 then
        return false
    end

    ResolvePlayerData()
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
    local favs = (State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Favorites) or {}
    local equippedNew = false
    local dudBasket = {}

    for _, newItem in ipairs(State.LastNewlyForgedItems) do
        if type(newItem) == "table" and newItem.GUID and newItem.Type
            and okEQ and Equipments and Equipments:GetItemType(newItem.Type) == "Weapon" then
            local newClassKey = ForgeEngine.GetWeaponClassKey(Equipments, newItem)
            local classMatches = (not targetClassKey) or (newClassKey == targetClassKey)
            local newScore, newDPS, newFlat = ForgeEngine.GetWeaponCompareScore(Equipments, newItem)
            if classMatches and newScore > curBestScore then
                if Config.AutoEquipBestArmor and Remotes.EquipItem and not equippedNew then
                    pcall(function() Remotes.EquipItem:InvokeServer(newItem) end)
                    equippedNew = true
                    task.wait(0.2)
                end
                curBestScore = newScore
                ForgeEngine.SetStatus(
                    string.format("New Best Weapon! (%.1f DPS | %.1f DMG)", newDPS, newFlat),
                    tostring(newItem.Type) .. (equippedNew and " equipped -> Forging Armor" or " saved -> Forging Armor")
                )
            else
                -- RNG rolled off-class (e.g., 1%) or weaker stats: keep current weapon and sell dud if enabled
                if Config.AutoEquipBestArmor and not favs[newItem.GUID] and not ForgeEngine.IsItemEquipped(newItem.GUID) then
                    dudBasket[newItem.GUID] = true
                end
                local reason = not classMatches and ("Off-Class (" .. tostring(newClassKey) .. ")") or string.format("Worse (%.1f <= %.1f %s)", newScore, curBestScore, modeLabel)
                ForgeEngine.SetStatus(
                    "Weapon RNG Rejected: " .. reason,
                    "Kept current weapon" .. (next(dudBasket) and " & selling dud -> Forging Armor" or " -> Forging Armor")
                )
            end
        end
    end

    if next(dudBasket) then
        ForgeEngine.SellEquipmentBasket(dudBasket)
        task.wait(0.25)
    end
    return true
end

-- Selects highest-tier lockable ore batch for a specific Armor slot ("Helmet", "Leggings", "Chestplate")
-- Uses highest-multiplier ores first from optional remainingPool (or live inventory)
function ForgeEngine.GetOptimalPieceBatch(slotKind, remainingPool)
    local owned = remainingPool
    if not owned then
        owned = ForgeEngine.GetPlayerOreInventory()
    end
    if not owned or #owned == 0 then return nil, 0, nil end

    -- Sort by Mult descending so crafted gear gets best defense/traits
    local byMult = {}
    local totalAvail = 0
    for _, o in ipairs(owned) do
        if o.Count > 0 then
            table.insert(byMult, { Name = o.Name, Count = o.Count, Mult = o.Mult, PriceMult = o.PriceMult })
            totalAvail = totalAvail + o.Count
        end
    end
    if totalAvail < 3 then return nil, 0, nil end
    table.sort(byMult, function(a, b)
        if a.Mult ~= b.Mult then return a.Mult > b.Mult end
        return a.Count > b.Count
    end)

    local tiersBySlot = {
        Helmet = {
            { class = "HeavyHelmet", ores = 35 },
            { class = "MediumHelmet", ores = 20 },
            { class = "LightHelmet", ores = 3 },
        },
        Leggings = {
            { class = "HeavyLeggings", ores = 42 },
            { class = "MediumLeggings", ores = 24 },
            { class = "LightLeggings", ores = 11 },
        },
        Chestplate = {
            { class = "HeavyChestplate", ores = 52 },
            { class = "MediumChestplate", ores = 30 },
            { class = "LightChestplate", ores = 15 },
        },
    }
    local tiers = tiersBySlot[slotKind]
    if not tiers then return nil, 0, nil end

    local chosenClass, targetOres = nil, 0
    for _, t in ipairs(tiers) do
        if totalAvail >= t.ores then
            chosenClass = t.class
            targetOres = t.ores
            break
        end
    end
    if not chosenClass then
        -- Not enough for lowest lockable Chestplate/Leggings; still take up to max available (min 3)
        chosenClass = nil
        targetOres = math.min(totalAvail, tiers[#tiers].ores)
    end

    local batch = {}
    local rem = targetOres
    local takenTotal = 0

    -- Pick up to 4 ore stacks: if top-4 by Mult can't reach targetOres, blend in largest-count stacks so we hit lock_min_ores!
    local top4Sum = 0
    for i = 1, math.min(4, #byMult) do
        top4Sum = top4Sum + byMult[i].Count
    end
    local chosenPool = {}
    if top4Sum >= targetOres or #byMult <= 4 then
        for i = 1, math.min(4, #byMult) do
            table.insert(chosenPool, byMult[i])
        end
    else
        -- Reserve largest stacks first until 4-slot capacity >= targetOres, then fill remaining slots with highest-Mult ores
        local byCount = table.clone(byMult)
        table.sort(byCount, function(a, b)
            if a.Count ~= b.Count then return a.Count > b.Count end
            return a.Mult > b.Mult
        end)
        local usedNames = {}
        local needSlots = 0
        local runningCap = 0
        for _, o in ipairs(byCount) do
            if runningCap >= targetOres or needSlots >= 4 then break end
            table.insert(chosenPool, o)
            usedNames[o.Name] = true
            runningCap = runningCap + o.Count
            needSlots = needSlots + 1
        end
        for _, o in ipairs(byMult) do
            if #chosenPool >= 4 then break end
            if not usedNames[o.Name] then
                table.insert(chosenPool, o)
                usedNames[o.Name] = true
            end
        end
        table.sort(chosenPool, function(a, b) return a.Mult > b.Mult end)
    end

    for i = 1, #chosenPool do
        if rem <= 0 then break end
        local take = math.min(chosenPool[i].Count, rem)
        if take > 0 then
            batch[chosenPool[i].Name] = take
            chosenPool[i].Count = chosenPool[i].Count - take
            rem = rem - take
            takenTotal = takenTotal + take
        end
    end
    if takenTotal < 3 then return nil, 0, nil end

    -- Deduct taken ores back into remainingPool if passed
    if remainingPool then
        for _, poolItem in ipairs(remainingPool) do
            local used = batch[poolItem.Name] or 0
            if used > 0 then
                poolItem.Count = math.max(0, poolItem.Count - used)
            end
        end
    end

    return batch, takenTotal, chosenClass
end

-- Builds sequential queue for all 3 Armor pieces (Chestplate, Leggings, Helmet) from live inventory
function ForgeEngine.BuildAll3ArmorQueue()
    local owned = ForgeEngine.GetPlayerOreInventory()
    local pool = {}
    for _, o in ipairs(owned) do
        table.insert(pool, { Name = o.Name, Count = o.Count, Mult = o.Mult, PriceMult = o.PriceMult })
    end
    local queue = {}
    for _, slotKind in ipairs({ "Chestplate", "Leggings", "Helmet" }) do
        local b, tot, lClass = ForgeEngine.GetOptimalPieceBatch(slotKind, pool)
        if b and tot >= 3 then
            table.insert(queue, {
                ores = b,
                total = tot,
                itemType = "Armor",
                lockedClass = lClass,
                label = lClass or slotKind
            })
        end
    end
    return queue
end

-- 8A-3. Remote Selective Ore Selling & Armor Selling
function ForgeEngine.GetSelectedSellOresList()
    local list = {}
    local map = {}
    local wl = Config.SellOreWhitelist
    if type(wl) == "table" then
        for k, v in pairs(wl) do
            local oreName = nil
            if type(k) == "number" and type(v) == "string" then
                oreName = v
            elseif type(k) == "string" and v == true then
                oreName = k
            end
            if oreName and oreName ~= "None" and not map[oreName] then
                map[oreName] = true
                table.insert(list, oreName)
            end
        end
    elseif type(wl) == "string" and wl ~= "None" then
        map[wl] = true
        table.insert(list, wl)
    end
    return list, map
end

function ForgeEngine.HasSellAnywherePass()
    ResolvePlayerData()
    local gp = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Gamepasses
    return gp and gp.SellAnywhere == true or false
end

function ForgeEngine.HasFastForgePass()
    ResolvePlayerData()
    local gp = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Gamepasses
    if gp and gp.FastForge == true then
        return true
    end
    if ForgeEngine.CachedFastForgePass ~= nil then
        return ForgeEngine.CachedFastForgePass
    end
    local owns = false
    if LocalPlayer and LocalPlayer.UserId then
        local ok, res = pcall(function()
            return game:GetService("MarketplaceService"):UserOwnsGamePassAsync(LocalPlayer.UserId, 1600910863)
        end)
        if ok and res == true then
            owns = true
        end
    end
    ForgeEngine.CachedFastForgePass = owns
    return owns
end

function ForgeEngine.ApplyFastForgePreference(forgeUI)
    local wantFast = (Config.AutoFastForge ~= false) and ForgeEngine.HasFastForgePass()
    if forgeUI then
        if type(forgeUI.ChangeFastForge) == "function" then
            pcall(function() forgeUI:ChangeFastForge(wantFast) end)
        end
        forgeUI.FastForge = wantFast
    end
    return wantFast
end

ForgeEngine.HookedDialogueFns = ForgeEngine.HookedDialogueFns or {}
ForgeEngine.SuppressDialogueUntil = 0

function ForgeEngine.EnsureDialogueHook()
    if typeof(getconnections) ~= "function" or typeof(hookfunction) ~= "function" then
        return false
    end
    local dr = ReplicatedStorage:FindFirstChild("DialogueEvents")
    dr = dr and dr:FindFirstChild("DialogueRemote") or ReplicatedStorage:FindFirstChild("DialogueRemote", true)
    if not dr or not dr:IsA("RemoteEvent") then
        return false
    end
    local anyHooked = false
    pcall(function()
        for idx, conn in ipairs(getconnections(dr.OnClientEvent)) do
            if conn.LuaConnection == true and typeof(conn.Function) == "function" then
                local fn = conn.Function
                if not ForgeEngine.HookedDialogueFns[fn] then
                    local origFn = nil
                    local wrapper = function(tree, speakerInfo, ...)
                        if ScriptRunning and os.clock() < (ForgeEngine.SuppressDialogueUntil or 0) then
                            local tName = (typeof(tree) == "Instance") and tree.Name or ""
                            if tName == "SellConfirmMisc" or tName == "SellConfirm" then
                                return nil
                            end
                        end
                        if origFn then
                            return origFn(tree, speakerInfo, ...)
                        end
                    end
                    origFn = hookfunction(fn, wrapper)
                    ForgeEngine.HookedDialogueFns[fn] = true
                    OrigHooks["DialogueRemoteOnEvent_" .. tostring(idx) .. "_" .. tostring(#OrigHooks + 1)] = {
                        func = fn,
                        orig = origFn,
                    }
                end
                anyHooked = true
            end
        end
    end)
    return anyHooked
end

function ForgeEngine.DismissDialogueCleanly(maxWaitSec)
    local pgui = LocalPlayer and LocalPlayer:FindFirstChild("PlayerGui")
    local dUi = pgui and pgui:FindFirstChild("DialogueUI")

    if dUi and dUi.Enabled then
        -- First skip typing & click "Goodbye." / "No" / "Cancel" so DialogueHandler:Close() runs natively (clears u3 & DisableBackpack)
        local respList = dUi:FindFirstChild("ResponseBillboard", true) and dUi.ResponseBillboard:FindFirstChild("List")
        local waitEnd = os.clock() + (maxWaitSec or 2.8)
        local clickedClose = false
        while dUi.Enabled and os.clock() < waitEnd do
            if respList then
                for _, child in ipairs(respList:GetChildren()) do
                    local btn = child:FindFirstChild("Button")
                    if btn and btn:IsA("GuiButton") then
                        local txt = tostring(btn.Text):lower()
                        if txt:find("no", 1, true) or txt:find("goodbye", 1, true) or txt:find("nevermind", 1, true) or txt:find("bye", 1, true) or txt:find("cancel", 1, true) then
                            if typeof(firesignal) == "function" then
                                pcall(function() firesignal(btn.MouseButton1Click) end)
                            end
                            clickedClose = true
                            task.wait(0.15)
                            break
                        end
                    end
                end
            end
            if not dUi.Enabled or clickedClose then break end
            task.wait(0.06)
        end
        if clickedClose and dUi.Enabled then
            local closeWait = os.clock() + 0.75
            while dUi.Enabled and os.clock() < closeWait do
                task.wait(0.05)
            end
        end
    end

    if dUi and dUi.Enabled then
        pcall(function()
            dUi.Enabled = false
            local rb = dUi:FindFirstChild("ResponseBillboard")
            if rb and rb:IsA("GuiObject") then rb.Visible = false end
            local pf = dUi:FindFirstChild("PromptFrame")
            local tl = pf and pf:FindFirstChild("TextLabel")
            if tl and tl:IsA("TextLabel") then tl.Text = "" end
        end)
        local Lighting = game:GetService("Lighting")
        local dof = Lighting:FindFirstChild("DialogueDOF")
        if dof then pcall(function() dof:Destroy() end) end
        local cam = workspace.CurrentCamera
        if cam and cam.CameraType == Enum.CameraType.Scriptable then
            cam.CameraType = Enum.CameraType.Custom
            cam.FieldOfView = cam:GetAttribute("Default") or 70
        end
    end

    -- Never leave orphaned DisableBackpack tags or hidden HUD overlays when DialogueUI is closed
    if not dUi or not dUi.Enabled then
        runAsGameThread(function()
            local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
            local FC = SafeGetController("ForgeController", KnitPkg)
            local UIC = SafeGetController("UIController", KnitPkg)
            local mods = UIC and UIC.Modules
            local keepLocked = (FC and FC.ForgeActive == true)
                or (mods and mods.RaceUI and mods.RaceUI.IsOpen == true)
                or (mods and mods.PickaxePreview and mods.PickaxePreview.IsOpen == true)
            if not keepLocked then
                for _, ch in ipairs({ State.Character, LocalPlayer and LocalPlayer.Character }) do
                    local st = ch and ch:FindFirstChild("Status")
                    if st then
                        for _, child in ipairs(st:GetChildren()) do
                            if child.Name == "DisableBackpack" then
                                pcall(function() child:Destroy() end)
                            end
                        end
                    end
                end
            end
            if UIC and type(UIC.RefreshOverlays) == "function" then
                pcall(function() UIC:RefreshOverlays() end)
            end
            if UIC and type(UIC.SetTopbarHidden) == "function" then
                pcall(function() UIC:SetTopbarHidden("Dialogue", false) end)
            end
        end, 1.5)
    end
end

function ForgeEngine.SellViaDialogue(npcName, dialogueType, basket)
    local prox = workspace:FindFirstChild("Proximity")
    local npc = prox and prox:FindFirstChild(npcName)
    if not npc or not basket or not next(basket) then return false end

    local pgui = LocalPlayer and LocalPlayer:FindFirstChild("PlayerGui")
    local dUi = pgui and pgui:FindFirstChild("DialogueUI")
    local dialogueHooked = ForgeEngine.EnsureDialogueHook()

    -- Clear any pre-existing DialogueUI cleanly before starting ForceDialogue
    if dUi and dUi.Enabled then
        pcall(ForgeEngine.DismissDialogueCleanly, 1.5)
    end

    if dialogueHooked then
        ForgeEngine.SuppressDialogueUntil = os.clock() + 4.5
    end

    local sold = false
    runAsGameThread(function()
        local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
        local PS = KnitPkg.GetService("ProximityService")
        local DS = KnitPkg.GetService("DialogueService")
        local UIC = SafeGetController("UIController", KnitPkg)
        if UIC and UIC.Modules then
            if dialogueType == "SellConfirmMisc" and UIC.Modules.MiscSell and UIC.Modules.MiscSell.SellInfo then
                UIC.Modules.MiscSell.SellInfo.Basket = basket
            elseif UIC.Modules.WeaponSell and UIC.Modules.WeaponSell.SellInfo then
                UIC.Modules.WeaponSell.SellInfo.Basket = basket
            end
        end
        if PS and DS then
            PS:ForceDialogue(npc, dialogueType):await()
            if not dialogueHooked then
                local waitStart = os.clock()
                while dUi and not dUi.Enabled and (os.clock() - waitStart) < 0.9 do
                    task.wait(0.05)
                end
                task.wait(0.15)
            else
                task.wait(0.08)
            end
            local okCmd, cmdRes = DS:RunCommand("SellConfirm", { Basket = basket }):await()
            sold = (okCmd ~= false) and (cmdRes ~= false)
        end
        if UIC and UIC.Modules then
            if dialogueType == "SellConfirmMisc" and UIC.Modules.MiscSell and UIC.Modules.MiscSell.SellInfo then
                UIC.Modules.MiscSell.SellInfo.Basket = {}
            elseif UIC.Modules.WeaponSell and UIC.Modules.WeaponSell.SellInfo then
                UIC.Modules.WeaponSell.SellInfo.Basket = {}
            end
        end
    end, 5.0)
    if not sold and Remotes.ForceDialogue and Remotes.RunCommand then
        pcall(function()
            Remotes.ForceDialogue:InvokeServer(npc, dialogueType)
            task.wait(dialogueHooked and 0.08 or 0.35)
            local okCmd, cmdRes = pcall(function()
                return Remotes.RunCommand:InvokeServer("SellConfirm", { Basket = basket })
            end)
            sold = okCmd and (cmdRes ~= false)
        end)
    end

    if dialogueHooked then
        ForgeEngine.SuppressDialogueUntil = os.clock() + 0.5
    end
    if dUi and dUi.Enabled then
        pcall(ForgeEngine.DismissDialogueCleanly, 2.8)
    end

    return sold
end

function ForgeEngine.IsMiscBasketRemaining(basket)
    if not basket or not next(basket) then return false end
    ResolvePlayerData()
    local inv = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory
    if type(inv) ~= "table" then return false end
    local guidSet = {}
    local nameQty = {}
    if type(inv.Misc) == "table" then
        for _, entry in pairs(inv.Misc) do
            if type(entry) == "table" then
                if entry.GUID then guidSet[entry.GUID] = true end
                if entry.Name then
                    nameQty[entry.Name] = (nameQty[entry.Name] or 0) + math.max(1, math.floor(tonumber(entry.Quantity) or 1))
                end
            end
        end
    end
    for key in pairs(basket) do
        if type(inv[key]) == "number" and math.floor(inv[key]) >= 1 then
            return true
        end
        if guidSet[key] or (nameQty[key] and nameQty[key] >= 1) then
            return true
        end
    end
    return false
end

function ForgeEngine.SellMiscBasket(basket)
    if not basket or not next(basket) then return false end
    local hasPass = ForgeEngine.HasSellAnywherePass()
    local prox = workspace:FindFirstChild("Proximity")
    local cey = prox and prox:FindFirstChild("Greedy Cey")
    if not hasPass and not cey then return false end

    if hasPass then
        local soldAny = false
        pcall(function()
            local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
            local InvSvc = KnitPkg.GetService("InventoryService")
            if InvSvc then
                local _, res = InvSvc:SellAnywhere(basket):await()
                soldAny = (res == true)
            end
        end)
        if soldAny then
            local pollT = os.clock()
            while (os.clock() - pollT) < 0.8 do
                if not ForgeEngine.IsMiscBasketRemaining(basket) then return true end
                task.wait(0.1)
            end
            if not ForgeEngine.IsMiscBasketRemaining(basket) then return true end
        end
    end

    if not cey then return false end

    -- Try in-place dialogue sell first; verify actual removal via Replica
    ForgeEngine.SellViaDialogue("Greedy Cey", "SellConfirmMisc", basket)
    local poll1 = os.clock()
    while (os.clock() - poll1) < 0.7 do
        if not ForgeEngine.IsMiscBasketRemaining(basket) then return true end
        task.wait(0.1)
    end

    -- Server may reject remote dialogue when far from Greedy Cey; glide cleanly to Greedy Cey and retry once
    if State.HRP then
        local cPos = cey:IsA("Model") and cey:GetPivot().Position or cey.Position
        if (State.HRP.Position - cPos).Magnitude > 12 then
            if State.HRP.Anchored then State.HRP.Anchored = false end
            if State.Humanoid then State.Humanoid.AutoRotate = true end
            NavEngine.GlideTo(CFrame.new(cPos + Vector3.new(0, 1.5, 4.0), cPos), Config.GlideSpeed or 55, true)
            task.wait(0.12)
        end
        ForgeEngine.SellViaDialogue("Greedy Cey", "SellConfirmMisc", basket)
        local poll2 = os.clock()
        while (os.clock() - poll2) < 1.2 do
            if not ForgeEngine.IsMiscBasketRemaining(basket) then return true end
            task.wait(0.1)
        end
    end

    return not ForgeEngine.IsMiscBasketRemaining(basket)
end

function ForgeEngine.SellSelectedOres()
    local whitelistArr = ForgeEngine.GetSelectedSellOresList()
    if #whitelistArr == 0 then
        return 0, 0, false
    end

    ResolvePlayerData()
    local inv = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory
    if not inv then return 0, 0, false end

    local favs = (State.PlayerData.Data and State.PlayerData.Data.Favorites) or {}
    local basket = {}
    local oreTypesCount = 0
    local totalUnits = 0

    for _, oreName in ipairs(whitelistArr) do
        local cnt = inv[oreName]
        if type(cnt) == "number" and cnt > 0 and not favs[oreName] then
            local qty = math.floor(cnt)
            if qty > 0 then
                basket[oreName] = qty
                oreTypesCount = oreTypesCount + 1
                totalUnits = totalUnits + qty
            end
        end
    end

    if totalUnits <= 0 then
        return 0, 0, false
    end

    local soldOk = ForgeEngine.SellMiscBasket(basket)
    return totalUnits, oreTypesCount, soldOk
end

function ForgeEngine.IsLootBlacklisted(itemName)
    if not itemName or itemName == "" then return false end
    local bl = Config.MobLootSellBlacklist
    if type(bl) == "table" then
        if bl[itemName] == true then return true end
        for k, v in pairs(bl) do
            if (type(k) == "number" and v == itemName) or (type(k) == "string" and k == itemName and v == true) then
                return true
            end
        end
    elseif type(bl) == "string" and bl ~= "" then
        return bl == itemName
    end
    return false
end

-- Sells all sellable Stash items (Essences, Shards/Fragments, Bones/Ores, Runes, Sellable Misc) EXCEPT MobLootSellBlacklist & Favorites
function ForgeEngine.SellAllMobLootExceptBlacklist()
    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local inv = data and data.Inventory
    if not inv then return 0, 0, false end

    local favs = data.Favorites or {}
    local itemTypes = (typeof(getrenv) == "function" and getrenv()._G and getrenv()._G.ItemTypes) or _G.ItemTypes or {}
    local okEx, Extras = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Extras) end)

    local basket = {}
    local typeCount = 0
    local totalUnits = 0

    -- 1. Numeric Stash keys (Ore / Bones like Boneite, Dark Boneite, Royal Boneite, Slimite, plus mined Ores)
    for itemName, cnt in pairs(inv) do
        if type(cnt) == "number" and cnt > 0 and not favs[itemName] and not ForgeEngine.IsLootBlacklisted(itemName) then
            local info = itemTypes[itemName]
            local kind = type(info) == "table" and info[1] or nil
            if kind == "Ore" or ForgeEngine.OreDataByName[itemName] then
                local qty = math.floor(cnt)
                if qty > 0 then
                    basket[itemName] = qty
                    typeCount = typeCount + 1
                    totalUnits = totalUnits + qty
                end
            end
        end
    end

    -- 2. Inventory.Misc entries (Material = Essences/Shards/Fragments, Rune = by GUID, MiscQuest if sellable)
    if type(inv.Misc) == "table" then
        for _, entry in pairs(inv.Misc) do
            if type(entry) == "table" then
                if entry.Id and entry.GUID then
                    local info = itemTypes[entry.Id]
                    local kind = type(info) == "table" and info[1] or nil
                    if kind == "Rune" and not Config.AutoSellRunes and not favs[entry.GUID] and not favs[entry.Id]
                        and not ForgeEngine.IsLootBlacklisted(entry.Id)
                        and not ForgeEngine.IsLootBlacklisted(entry.GUID) then
                        local qty = math.max(1, math.floor(entry.Quantity or 1))
                        basket[entry.GUID] = qty
                        typeCount = typeCount + 1
                        totalUnits = totalUnits + qty
                    end
                elseif entry.Name then
                    local itemName = entry.Name
                    local info = itemTypes[itemName]
                    local kind = type(info) == "table" and info[1] or nil
                    local isSellable = (kind == "Material")
                        or (kind == "MiscQuest" and okEx and Extras and type(Extras.GetSellPrice) == "function" and Extras:GetSellPrice(itemName) ~= nil)
                    if isSellable and not favs[itemName] and not ForgeEngine.IsLootBlacklisted(itemName) then
                        local qty = math.max(1, math.floor(entry.Quantity or 1))
                        if qty > 0 then
                            basket[itemName] = (basket[itemName] or 0) + qty
                            typeCount = typeCount + 1
                            totalUnits = totalUnits + qty
                        end
                    end
                end
            end
        end
    end

    if totalUnits <= 0 then
        return 0, 0, false
    end

    local soldOk = ForgeEngine.SellMiscBasket(basket)
    return totalUnits, typeCount, soldOk
end

function ForgeEngine.IsItemEquipped(guid)
    ResolvePlayerData()
    local eq = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Equipped
    if not eq or not guid then return false end
    for _, slotData in pairs(eq) do
        if type(slotData) == "table" and (slotData.GUID == guid or slotData.Name == guid) then
            return true
        elseif type(slotData) == "string" and slotData == guid then
            return true
        end
    end
    return false
end

-- Authoritative Equipment Bag capacity detection (StatusService:GetPlayerEquipmentInfo + Replica fallback)
function ForgeEngine.GetEquipmentCapacityStatus()
    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local eqList = data and data.Inventory and data.Inventory.Equipments or {}
    local guidCount = 0
    for _, item in ipairs(eqList) do
        if type(item) == "table" and item.GUID then
            guidCount = guidCount + 1
        end
    end

    local upg = (data and tonumber(data.EquipmentBagUpgrade)) or 0
    if not State.LastEquipCapacity or State.LastEquipCapacity <= 0 or State.LastEquipUpgLevel ~= upg then
        runAsGameThread(function()
            local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
            local SS = KnitPkg and KnitPkg.GetService("StatusService")
            if SS and type(SS.GetPlayerEquipmentInfo) == "function" then
                local okInfo, info = SS:GetPlayerEquipmentInfo():await()
                if okInfo and type(info) == "table" and type(info.Capacity) == "number" and info.Capacity > 0 then
                    State.LastEquipCapacity = info.Capacity
                    State.LastEquipUpgLevel = upg
                end
            end
        end, 2.0)
    end

    local cap = (State.LastEquipCapacity and State.LastEquipCapacity > 0 and State.LastEquipCapacity) or (5 + upg)
    local used = math.max(guidCount, #eqList)
    local rem = math.max(0, cap - used)
    local isFull = (cap > 0 and used >= cap)
    return used, cap, isFull, rem
end

function ForgeEngine.GetSellableUnequippedArmorBasket(Equipments, extraProtectGuids)
    ResolvePlayerData()
    local inv = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory
    local eqList = inv and inv.Equipments
    if not eqList then return {}, 0 end

    if not Equipments then
        local okEQ, eqMod = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
        Equipments = okEQ and eqMod or nil
    end
    if not Equipments then return {}, 0 end

    local favs = (State.PlayerData.Data and State.PlayerData.Data.Favorites) or {}
    local protArmorGuids = ForgeEngine.GetProtectedArmorSets(Equipments)
    local basket = {}
    local count = 0
    for _, item in ipairs(eqList) do
        if type(item) == "table" and item.GUID and not ForgeEngine.IsItemEquipped(item.GUID)
            and not favs[item.GUID] and not protArmorGuids[item.GUID]
            and not (extraProtectGuids and extraProtectGuids[item.GUID]) then
            local iType = Equipments:GetItemType(item.Type or item.Name)
            if iType == "Armor" then
                basket[item.GUID] = true
                count = count + 1
            end
        end
    end
    return basket, count
end

function ForgeEngine.SellEquipmentBasket(basket)
    if not basket or not next(basket) then return false end

    local function ensureAtMarbles()
        if ForgeEngine.HasSellAnywherePass() then return true end
        local prox = workspace:FindFirstChild("Proximity")
        local marbles = prox and prox:FindFirstChild("Marbles")
        if marbles and State.HRP then
            local mPos = marbles:IsA("Model") and marbles:GetPivot().Position or marbles.Position
            if (State.HRP.Position - mPos).Magnitude > 10 then
                NavEngine.GlideTo(CFrame.new(mPos + Vector3.new(0, 2.8, 0)), Config.GlideSpeed or 55, true)
                task.wait(0.15)
            end
        end
        return true
    end

    local function runSell(targetBasket)
        local sold = false
        if ForgeEngine.HasSellAnywherePass() then
            pcall(function()
                local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
                local InvSvc = KnitPkg.GetService("InventoryService")
                if InvSvc then
                    local _, res = InvSvc:SellAnywhere(targetBasket, true):await()
                    sold = (res == true)
                end
            end)
        end
        if not sold then
            ensureAtMarbles()
            sold = ForgeEngine.SellViaDialogue("Marbles", "SellConfirm", targetBasket)
        end
        return sold
    end

    local function getRemaining(sourceBasket)
        ResolvePlayerData()
        local data = State.PlayerData and State.PlayerData.Data
        local invEq = data and data.Inventory and data.Inventory.Equipments
        local favs = (data and data.Favorites) or {}
        local rem = {}
        if type(invEq) == "table" then
            for _, item in ipairs(invEq) do
                if type(item) == "table" and item.GUID and sourceBasket[item.GUID]
                    and not ForgeEngine.IsItemEquipped(item.GUID) and not favs[item.GUID] then
                    rem[item.GUID] = true
                end
            end
        end
        return rem
    end

    local remaining = getRemaining(basket)
    if not next(remaining) then return true end

    for attempt = 1, 3 do
        if attempt > 1 then
            ensureAtMarbles()
            task.wait(0.25)
        end

        -- On retry with many items, split into <= 10-item chunks to avoid server batch limits
        local guids = {}
        for g in pairs(remaining) do
            table.insert(guids, g)
        end
        if attempt >= 2 and #guids > 10 then
            for i = 1, #guids, 10 do
                local chunk = {}
                for j = i, math.min(i + 9, #guids) do
                    chunk[guids[j]] = true
                end
                runSell(chunk)
                task.wait(0.25)
            end
        else
            runSell(remaining)
        end

        -- Poll Replica up to 1.5s until sold GUIDs disappear from Inventory.Equipments
        local pollStart = os.clock()
        while (os.clock() - pollStart) < 1.5 do
            task.wait(0.15)
            remaining = getRemaining(basket)
            if not next(remaining) then
                break
            end
        end

        -- Clear confirmed sold GUIDs from PendingSellArmorGuids
        if type(State.PendingSellArmorGuids) == "table" then
            for g in pairs(basket) do
                if not remaining[g] then
                    State.PendingSellArmorGuids[g] = nil
                end
            end
        end

        if not next(remaining) then
            return true
        end
    end

    return not next(remaining)
end

function ForgeEngine.SellAllUnequippedArmor()
    if Config.AutoEquipBestArmor then
        State.LastBestArmorCheck = 0
        local eqCount = ForgeEngine.EquipBestArmor()
        if eqCount > 0 then
            task.wait(0.35)
        end
    end
    local basket, count = ForgeEngine.GetSellableUnequippedArmorBasket()
    if count > 0 then
        local soldOk = ForgeEngine.SellEquipmentBasket(basket)
        return count, soldOk
    end
    return 0, true
end

function ForgeEngine.SetStatus(title, desc)
    if not ForgeEngine.AutoForgeStatusLabel then return end
    pcall(function()
        if title then ForgeEngine.AutoForgeStatusLabel:SetTitle(title) end
        if desc then ForgeEngine.AutoForgeStatusLabel:SetDesc(desc) end
    end)
end

function ForgeEngine.WaitForMinigamesClear(timeout)
    timeout = timeout or 4.0
    local start = os.clock()
    while ScriptRunning and (os.clock() - start) < timeout do
        local pgui = LocalPlayer:FindFirstChild("PlayerGui")
        local forgeGui = pgui and pgui:FindFirstChild("Forge")
        local busy = false
        if forgeGui then
            local melt = forgeGui:FindFirstChild("MeltMinigame")
            local pour = forgeGui:FindFirstChild("PourMinigame")
            local hammer = forgeGui:FindFirstChild("HammerMinigame")
            local endScreen = forgeGui:FindFirstChild("EndScreen")
            if (melt and melt.Visible)
                or (pour and pour.Visible)
                or (hammer and hammer.Visible)
                or (endScreen and endScreen.Visible) then
                busy = true
            end
        end
        if not busy then
            return true
        end
        task.wait(0.15)
    end
    return false
end

function ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
    ForgeEngine.SessionEpoch = (ForgeEngine.SessionEpoch or 0) + 1
    local myEpoch = ForgeEngine.SessionEpoch
    pcall(function()
        local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
        if not FC then FC = SafeGetController("ForgeController", KnitPkg) end
        local UIC = SafeGetController("UIController", KnitPkg)
        if not ForgeUI then ForgeUI = UIC and UIC.Modules and UIC.Modules.Forge end

        -- NOTE: ForgeController:EndForge() checks `if self.ForgeActive then ... end` before destroying NoMovement / DisableBackpack / DisableMusic!
        -- Therefore we MUST let ForgeUI:Close() / FC:EndForge() run WHILE FC.ForgeActive is still true.
        if ForgeUI then
            local closeDone = false
            task.spawn(function()
                pcall(function()
                    ForgeUI.addedOres = {}
                    ForgeUI.LockedClass = nil
                    if type(ForgeUI.Close) == "function" then
                        ForgeUI:Close()
                    end
                end)
                closeDone = true
            end)
            local waitStart = os.clock()
            while not closeDone and (os.clock() - waitStart) < 2.0 do
                task.wait(0.05)
            end
            pcall(function()
                ForgeUI.IsOpen = false
                ForgeUI.addedOres = {}
                ForgeUI.LockedClass = nil
            end)
        end

        if FC and FC.ForgeActive then
            pcall(function() FC:EndForge() end)
            local endWait = os.clock()
            while FC.ForgeActive and (os.clock() - endWait) < 1.5 do
                task.wait(0.05)
            end
        end

        if FC then
            FC.ForgeActive = false
            if FC.Sequence ~= "Close" then
                pcall(function() FC:ChangeSequence("Close") end)
            end
        end

        -- Guarantee movement/backpack/music/camera tags & Forge prompt are restored even if ForgeActive was already cleared early
        local function clearForgeLocks()
            local chars = { State.Character, LocalPlayer and LocalPlayer.Character }
            for _, ch in ipairs(chars) do
                local st = ch and ch:FindFirstChild("Status")
                if st then
                    for _, tagName in ipairs({ "NoMovement", "DisableBackpack" }) do
                        for _, child in ipairs(st:GetChildren()) do
                            if child.Name == tagName then
                                pcall(function() child:Destroy() end)
                            end
                        end
                    end
                end
            end
            local SoundService = game:GetService("SoundService")
            local svcStatus = SoundService:FindFirstChild("ServiceStatus")
            if svcStatus then
                for _, child in ipairs(svcStatus:GetChildren()) do
                    if child.Name == "DisableMusic" then
                        pcall(function() child:Destroy() end)
                    end
                end
            end
            if State.HRP and not Config.StayAtSavedPos then
                State.HRP.Anchored = false
            end
            if State.Humanoid then
                State.Humanoid.AutoRotate = true
            end
            local prox = workspace:FindFirstChild("Proximity")
            local forgeObj = prox and prox:FindFirstChild("Forge")
            local prompt = forgeObj and forgeObj:FindFirstChildOfClass("ProximityPrompt")
            if prompt then
                prompt.Enabled = true
            end
            local cam = workspace.CurrentCamera
            if cam and cam.CameraType == Enum.CameraType.Scriptable and (not FC or not FC.ForgeActive) then
                cam.CameraType = Enum.CameraType.Custom
                cam.FieldOfView = cam:GetAttribute("Default") or 70
            end
        end

        clearForgeLocks()

        local pgui = LocalPlayer and LocalPlayer:FindFirstChild("PlayerGui")
        local forgeGui = pgui and pgui:FindFirstChild("Forge")
        if forgeGui then
            forgeGui.Enabled = false
            for _, frameName in ipairs({ "OreSelect", "MeltMinigame", "PourMinigame", "HammerMinigame", "EndScreen" }) do
                local f = forgeGui:FindFirstChild(frameName)
                if f and f:IsA("GuiObject") then
                    f.Visible = false
                end
            end
        end

        if UIC and type(UIC.Close) == "function" and UIC.Current == "Forge" then
            task.spawn(function() pcall(function() UIC:Close("Forge", true) end) end)
        end
        if UIC and type(UIC.RefreshOverlays) == "function" then
            task.spawn(function() pcall(function() UIC:RefreshOverlays() end) end)
        end

        -- Delayed guard in case Showcase's task.delay(0.1, ChangeSequence("OreSelect")) or task.delay(0.4, Open) ran right around close
        for _, delaySec in ipairs({ 0.25, 0.55 }) do
            task.delay(delaySec, function()
                if ForgeEngine.SessionEpoch ~= myEpoch then return end
                pcall(function()
                    if FC and FC.ForgeActive then
                        pcall(function() FC:EndForge() end)
                        FC.ForgeActive = false
                    end
                    if ForgeUI then ForgeUI.IsOpen = false end
                    local pg = LocalPlayer and LocalPlayer:FindFirstChild("PlayerGui")
                    local fg = pg and pg:FindFirstChild("Forge")
                    if fg then fg.Enabled = false end
                    clearForgeLocks()
                    if UIC and type(UIC.RefreshOverlays) == "function" then
                        UIC:RefreshOverlays()
                    end
                end)
            end)
        end
    end)
end

-- Evaluates inventory Armor via Equipments:CalculateHealth and equips the strongest Helmet/Chestplate/Leggings
function ForgeEngine.EquipBestArmor()
    if not Remotes.EquipItem then return 0 end
    if os.clock() - (State.LastBestArmorCheck or 0) < 1.5 then return 0 end
    State.LastBestArmorCheck = os.clock()

    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local invEq = data and data.Inventory and data.Inventory.Equipments
    local curEq = data and data.Equipped
    if not invEq then return 0 end

    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
    if not okEQ or not Equipments then return 0 end

    local bestBySlot = {}
    for _, item in ipairs(invEq) do
        if type(item) == "table" and item.GUID and item.Type then
            if Equipments:GetItemType(item.Type) == "Armor" then
                local iData = Equipments:GetItemData(item.Type)
                local slot = iData and iData.EquipSlot
                if slot == "Helmet" or slot == "Chestplate" or slot == "Leggings" then
                    local okHp, hp = pcall(function() return Equipments:CalculateHealth(item) end)
                    local score = (okHp and type(hp) == "number") and hp or (ForgeEngine.SafeCalcPrice(Equipments, item) or 0)
                    if not bestBySlot[slot] or score > bestBySlot[slot].score then
                        bestBySlot[slot] = { item = item, score = score }
                    end
                end
            end
        end
    end

    local _, protSlots = ForgeEngine.GetProtectedArmorSets(Equipments)
    local equippedCount = 0
    for _, slot in ipairs({ "Helmet", "Chestplate", "Leggings" }) do
        local best = bestBySlot[slot]
        if best and best.item and not protSlots[slot] then
            local cur = curEq and curEq[slot]
            local curGuid = type(cur) == "table" and cur.GUID or nil
            if curGuid ~= best.item.GUID then
                local ok, res = pcall(function()
                    return Remotes.EquipItem:InvokeServer(best.item)
                end)
                if ok and (not res or res.Response == "EQUIPPED") then
                    equippedCount = equippedCount + 1
                end
                task.wait(0.15)
            end
        end
    end

    -- Also evaluate and equip the best matching Weapon (respecting ProtectCustomWeapon, WeaponClassTarget, and WeaponCompareMode)
    if not Config.ProtectCustomWeapon then
        local bestWep = select(1, ForgeEngine.GetBestOwnedWeaponBaseDamage())
        local curWep = curEq and curEq.Weapon
        local curWepGuid = type(curWep) == "table" and curWep.GUID or nil
        if bestWep and bestWep.GUID and curWepGuid ~= bestWep.GUID then
            local ok, res = pcall(function()
                return Remotes.EquipItem:InvokeServer(bestWep)
            end)
            if ok and (not res or res.Response == "EQUIPPED") then
                equippedCount = equippedCount + 1
            end
            task.wait(0.15)
        end
    end

    return equippedCount
end

-- 8A-4. 1-1-1-1 Target Sword / Weapon Hunt Engine & Automated Forge Execution
function ForgeEngine.GetTargetWeaponOptions()
    local list = { "Any Straight Sword" }
    local added = { ["Any Straight Sword"] = true }
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)

    local straightSwords = {}
    if okEQ and Equipments and type(Equipments.BaseTypes) == "table" and type(Equipments.BaseTypes.StraightSword) == "table" then
        for _, wName in ipairs(Equipments.BaseTypes.StraightSword.Types or {}) do
            if type(wName) == "string" and wName ~= "" and not added[wName] then
                added[wName] = true
                table.insert(straightSwords, wName)
            end
        end
    end
    if #straightSwords == 0 then
        straightSwords = {
            "Candy Cane", "Chaos", "Chokuto", "Crystalized Broadsword", "Cutlass",
            "Dragon Blade", "Falchion", "Gladius", "Hachiwari", "Hell Slayer",
            "Hook Blade", "Rapier", "Thornspine"
        }
        for _, wName in ipairs(straightSwords) do added[wName] = true end
    end
    table.sort(straightSwords)
    for _, wName in ipairs(straightSwords) do
        table.insert(list, wName)
    end

    local classOptions = {
        "Any Katana", "Any Great Sword", "Any Colossal Sword", "Any Dagger",
        "Any Spear", "Any Axe", "Any Great Axe", "Any Mace", "Any Gauntlet",
        "Any Halberd", "Any Sword & Shield"
    }
    for _, cName in ipairs(classOptions) do
        if not added[cName] then
            added[cName] = true
            table.insert(list, cName)
        end
    end

    local otherWeapons = {}
    if okEQ and Equipments and type(Equipments.Items) == "table" and type(Equipments.Items.Weapon) == "table" then
        for wName, wData in pairs(Equipments.Items.Weapon) do
            if type(wName) == "string" and type(wData) == "table" and not added[wName] then
                added[wName] = true
                table.insert(otherWeapons, wName)
            end
        end
    end
    table.sort(otherWeapons)
    for _, wName in ipairs(otherWeapons) do
        table.insert(list, wName)
    end
    return list
end

ForgeEngine.WeaponHuntClassMap = {
    ["Any Straight Sword"] = "StraightSword",
    ["Any Katana"] = "Katana",
    ["Any Great Sword"] = "GreatSword",
    ["Any Colossal Sword"] = "ColossalSword",
    ["Any Dagger"] = "Dagger",
    ["Any Spear"] = "Spear",
    ["Any Axe"] = "Axe",
    ["Any Great Axe"] = "GreatAxe",
    ["Any Mace"] = "Mace",
    ["Any Gauntlet"] = "Gauntlet",
    ["Any Halberd"] = "Halberd",
    ["Any Sword & Shield"] = "SwordShield",
}

function ForgeEngine.DoesWeaponMatchHuntTarget(Equipments, item, target)
    if not Equipments or type(item) ~= "table" or not item.Type then return false end
    if Equipments:GetItemType(item.Type) ~= "Weapon" then return false end
    target = tostring(target or "Any Straight Sword")
    local wantClass = ForgeEngine.WeaponHuntClassMap[target]
    if wantClass then
        return ForgeEngine.GetWeaponClassKey(Equipments, item) == wantClass
    end
    return tostring(item.Type) == target
end

function ForgeEngine.GetWeaponHuntBatch()
    ForgeEngine.EnsureOreCatalog()
    local blacklisted = ForgeEngine.GetBlacklistedForgeOresMap()
    local batch = {}
    local seen = {}
    local autoSlots = {}
    local pd = ResolvePlayerData()
    local data = pd and pd.Data
    local inv = (data and type(data.Inventory) == "table" and data.Inventory) or {}
    local favs = (data and type(data.Favorites) == "table" and data.Favorites) or {}

    local function normOreKey(name)
        return type(name) == "string" and name:lower():gsub("[%s%-_]+", "") or ""
    end

    local function resolveCanonicalOre(name)
        if type(name) ~= "string" or name == "" then return name end
        if ForgeEngine.OreDataByName[name] or type(inv[name]) == "number" then
            return name
        end
        local want = normOreKey(name)
        for k, _ in pairs(ForgeEngine.OreDataByName) do
            if normOreKey(k) == want then return k end
        end
        for k, v in pairs(inv) do
            if type(v) == "number" and normOreKey(k) == want then return k end
        end
        return name
    end

    -- Read-only local UI fallback ONLY when PlayerData Replica has an empty/unresolved Inventory (never mutates State.PlayerData)
    local uiFallbackOres = nil
    local function getOreCountLocal(oreName)
        local cnt = math.floor(tonumber(inv[oreName]) or 0)
        if cnt > 0 then return cnt end
        local want = normOreKey(oreName)
        for k, v in pairs(inv) do
            if type(v) == "number" and normOreKey(k) == want then
                cnt = math.max(cnt, math.floor(v))
            end
        end
        if cnt > 0 then return cnt end
        if type(inv.Misc) == "table" and ForgeEngine.OreDataByName[oreName] then
            for _, item in ipairs(inv.Misc) do
                if type(item) == "table" and type(item.Name) == "string" and (item.Name == oreName or normOreKey(item.Name) == want) then
                    cnt = cnt + math.max(0, math.floor(tonumber(item.Quantity) or 1))
                end
            end
            if cnt > 0 then return cnt end
        end
        if next(inv) == nil then
            if not uiFallbackOres then
                uiFallbackOres = {}
                pcall(function()
                    local pg = LocalPlayer and LocalPlayer:FindFirstChildOfClass("PlayerGui")
                    local forgeBg = pg and pg:FindFirstChild("Forge") and pg.Forge:FindFirstChild("OreSelect")
                        and pg.Forge.OreSelect:FindFirstChild("OresFrame") and pg.Forge.OreSelect.OresFrame:FindFirstChild("Frame")
                        and pg.Forge.OreSelect.OresFrame.Frame:FindFirstChild("Background")
                    if forgeBg then
                        for _, child in ipairs(forgeBg:GetChildren()) do
                            if ForgeEngine.OreDataByName[child.Name] then
                                local qtyObj = child:FindFirstChild("Main") and child.Main:FindFirstChild("Quantity")
                                local qtyTxt = qtyObj and tostring(qtyObj.Text or "") or ""
                                local q = tonumber(qtyTxt:match("x?(%d+)")) or 0
                                if q > 0 then uiFallbackOres[child.Name] = q end
                            end
                        end
                    end
                end)
            end
            cnt = math.floor(tonumber(uiFallbackOres[oreName]) or 0)
        end
        return cnt
    end

    local favBlocked = {}
    local favSeen = {}
    local function addFavBlocked(name)
        if name and not favSeen[name] then
            favSeen[name] = true
            table.insert(favBlocked, name)
        end
    end

    local missingSlots = {}
    local selectErr, dupErr, blErr = nil, nil, nil

    for idx = 1, 4 do
        local hd = ForgeEngine.HuntSlotDropdowns and ForgeEngine.HuntSlotDropdowns[idx]
        if hd and type(hd.GetValue) == "function" then
            local okVal, ddVal = pcall(hd.GetValue, hd)
            if okVal and type(ddVal) == "string" and ddVal ~= "" then
                Config["WeaponHuntOre" .. tostring(idx)] = ddVal
            end
        end
        local oreName = Config["WeaponHuntOre" .. tostring(idx)]
        if oreName == "Auto: Best From Inventory" then
            table.insert(autoSlots, idx)
        else
            if type(oreName) ~= "string" or oreName == "" or oreName == "None" then
                selectErr = selectErr or string.format("Select an ore for Slot %d", idx)
            else
                oreName = resolveCanonicalOre(oreName)
                if seen[oreName] then
                    dupErr = dupErr or string.format("Slot %d duplicates '%s' (need 4 distinct ores for 1-1-1-1)", idx, oreName)
                elseif blacklisted[oreName] then
                    blErr = blErr or string.format("'%s' is in Forge Ore Blacklist!", oreName)
                elseif favs[oreName] then
                    addFavBlocked(oreName)
                else
                    local have = getOreCountLocal(oreName)
                    if have < 1 then
                        local extra = ""
                        if (oreName == "Yin" or oreName == "Yang") and getOreCountLocal("Yin-Yang") >= 1 then
                            extra = string.format("; you have %d 'Yin-Yang'", getOreCountLocal("Yin-Yang"))
                        end
                        table.insert(missingSlots, { idx = idx, name = oreName, extra = extra })
                    else
                        seen[oreName] = true
                        batch[oreName] = 1
                    end
                end
            end
        end
    end

    if selectErr then return nil, selectErr end
    if dupErr then return nil, dupErr end
    if blErr then return nil, blErr end

    if #missingSlots > 0 then
        if #missingSlots == 1 then
            local m = missingSlots[1]
            return nil, string.format("Slot %d: Out of '%s' (0 in inventory%s)", m.idx, m.name, m.extra)
        end
        local parts = {}
        for _, m in ipairs(missingSlots) do
            table.insert(parts, string.format("Slot %d '%s' (0%s)", m.idx, m.name, m.extra))
        end
        return nil, "Out of ores: " .. table.concat(parts, ", ")
    end

    if #autoSlots > 0 then
        local owned = ForgeEngine.GetPlayerOreInventory(false, false)
        table.sort(owned, function(a, b)
            if a.Mult ~= b.Mult then return a.Mult > b.Mult end
            if a.PriceMult ~= b.PriceMult then return a.PriceMult > b.PriceMult end
            return a.Count > b.Count
        end)
        for _, _ in ipairs(autoSlots) do
            local picked = nil
            for _, ore in ipairs(owned) do
                if not seen[ore.Name] and not blacklisted[ore.Name] and (ore.Count or 0) >= 1 then
                    picked = ore.Name
                    break
                end
            end
            if not picked then
                local ownedWithFavs = ForgeEngine.GetPlayerOreInventory(false, true)
                for _, ore in ipairs(ownedWithFavs) do
                    if favs[ore.Name] and not seen[ore.Name] and not blacklisted[ore.Name] and (ore.Count or 0) >= 1 then
                        addFavBlocked(ore.Name)
                    end
                end
                if #favBlocked > 0 then
                    if #favBlocked == 1 then
                        return nil, string.format("Ore '%s' is favorited! Unfavorite it or pick another ore.", favBlocked[1])
                    end
                    return nil, string.format("Favorited ores skipped: %s! Unfavorite them or pick other ores.", table.concat(favBlocked, ", "))
                end
                return nil, "Need 4 distinct non-blacklisted ores in inventory for 1-1-1-1"
            end
            seen[picked] = true
            batch[picked] = 1
        end
    end

    if #favBlocked > 0 then
        if #favBlocked == 1 then
            return nil, string.format("Ore '%s' is favorited! Unfavorite it or pick another ore.", favBlocked[1])
        end
        return nil, string.format("Favorited ores skipped: %s! Unfavorite them or pick other ores.", table.concat(favBlocked, ", "))
    end

    return batch, nil
end

function ForgeEngine.SetWeaponHuntStatus(title, desc)
    if ForgeEngine.WeaponHuntStatusLabel then
        pcall(function()
            if title then ForgeEngine.WeaponHuntStatusLabel:SetTitle(title) end
            if desc then ForgeEngine.WeaponHuntStatusLabel:SetDesc(desc) end
        end)
    end
    ForgeEngine.SetStatus(title, desc)
end

function ForgeEngine._EnsureForgeOpen(FC, ForgeUI, PS, forgeModel)
    if type(ForgeEngine.EnsureMinigameHooks) == "function" then
        pcall(ForgeEngine.EnsureMinigameHooks, FC)
    end
    ForgeEngine.SessionEpoch = (ForgeEngine.SessionEpoch or 0) + 1
    local forgePos = forgeModel:IsA("Model") and forgeModel:GetPivot().Position or forgeModel.Position
    if State.HRP and (State.HRP.Position - forgePos).Magnitude > 10 then
        ForgeEngine.SetStatus("Auto-Forge: Tweening to Forge", "Moving to town Forge station...")
        NavEngine.GlideTo(CFrame.new(forgePos + Vector3.new(0, 2.8, 0)), Config.GlideSpeed or 55, true)
    end
    if not FC.ForgeActive then
        runAsGameThread(function()
            if PS and type(PS.Forge) == "function" then
                pcall(function() PS:Forge(forgeModel):await() end)
            elseif Remotes.ProximityForge then
                pcall(function() Remotes.ProximityForge:InvokeServer(forgeModel) end)
            end
        end, 4.5)
        if not FC.ForgeActive then
            runAsGameThread(function()
                pcall(function() FC:StartForge(forgeModel) end)
            end, 2.5)
        end
    end
    local waitStart = os.clock()
    while (not FC.ForgeActive or FC.Sequence ~= "OreSelect") and (os.clock() - waitStart) < 6.0 do
        task.wait(0.15)
    end
    if not FC.ForgeActive then
        ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
        return false
    end
    task.wait(0.9)
    pcall(function()
        local pgui = LocalPlayer:FindFirstChild("PlayerGui")
        local osFrame = pgui and pgui:FindFirstChild("Forge") and pgui.Forge:FindFirstChild("OreSelect")
        local openWait = os.clock()
        while FC.ForgeActive and osFrame and not osFrame.Visible and (os.clock() - openWait) < 2.0 do
            task.wait(0.1)
        end
    end)
    return FC.ForgeActive
end

function ForgeEngine.RecycleItemByGuid(guid, itemObj)
    if not guid or not Remotes.RecycleBasket then
        return false, "no_remote"
    end
    local canAfford = true
    local priceCost = nil
    pcall(function()
        local RecycleUtil = safeRequire(ReplicatedStorage.Shared.Utils.RecycleUtil)
        if RecycleUtil and type(RecycleUtil.GetPrice) == "function" and type(RecycleUtil.GetOres) == "function" and itemObj then
            local ores = RecycleUtil.GetOres(itemObj)
            if ores then
                priceCost = tonumber(RecycleUtil.GetPrice(ores))
                ResolvePlayerData()
                local gold = tonumber(State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Gold) or 0
                if priceCost and priceCost > gold then
                    canAfford = false
                end
            end
        end
    end)
    if not canAfford then
        return false, "insufficient_gold"
    end
    local ok, res = pcall(function()
        return Remotes.RecycleBasket:InvokeServer({ [guid] = true })
    end)
    if ok and res ~= false then
        return true, priceCost
    end
    return false, "rejected"
end

function ForgeEngine._RunSingleForgeRoll(FC, ForgeUI, PS, forgeModel, rollBatch, targetItemType, lockedClass, preGuids, statusPrefix)
    ForgeEngine.WaitForMinigamesClear(4.0)
    local pgui = LocalPlayer:FindFirstChild("PlayerGui")
    local osFrame = pgui and pgui:FindFirstChild("Forge") and pgui.Forge:FindFirstChild("OreSelect")
    local osWait = os.clock()
    while FC.ForgeActive and osFrame and not osFrame.Visible and (os.clock() - osWait) < 2.5 do
        task.wait(0.1)
    end
    if State.PourMouseDown then
        State.PourMouseDown = false
        pcall(function() game:GetService("VirtualInputManager"):SendMouseButtonEvent(400, 300, 0, false, game, 0) end)
    end

    local function inject()
        if ForgeUI then
            runAsGameThread(function()
                pcall(function() ForgeUI:SelectItemType(targetItemType) end)
                ForgeUI.selectedItemType = targetItemType
                ForgeUI.LockedClass = nil
                ForgeUI.addedOres = table.clone(rollBatch)
                ForgeEngine.ApplyFastForgePreference(ForgeUI)
                pcall(function() ForgeUI:ClearMolds() end)
                ForgeUI.LockedClass = lockedClass
                pcall(function() ForgeUI:UpdateAddedOres() end)
                if ForgeUI.UpdateProbabilities then
                    pcall(function() ForgeUI:UpdateProbabilities() end)
                end
            end, 1.5)
        end
        FC.ItemType = targetItemType
        FC.Ores = table.clone(rollBatch)
    end

    local function triggerMelt()
        local meltDone = false
        task.spawn(function()
            local setId = (typeof(setthreadidentity) == "function" and setthreadidentity)
                or (typeof(setidentity) == "function" and setidentity)
                or (syn and typeof(syn.set_thread_identity) == "function" and syn.set_thread_identity)
            if setId then pcall(setId, 2) end
            pcall(function() FC:ChangeSequence("Melt") end)
            meltDone = true
        end)
        local t0 = os.clock()
        while not meltDone and (os.clock() - t0) < 6.0 do
            if FC.Sequence == "Pour" or FC.Sequence == "Hammer" or FC.Sequence == "Water" then break end
            task.wait(0.1)
        end
        if FC.Sequence ~= "OreSelect" and ForgeUI then
            pcall(function() ForgeUI.addedOres = {} end)
        end
        return meltDone
    end

    inject()
    local meltDone = triggerMelt()
    if meltDone and FC.Sequence == "OreSelect" then
        task.wait(0.9)
        if FC.ForgeActive and FC.Sequence == "OreSelect" then
            inject()
            local retryDone = triggerMelt()
            if retryDone and FC.Sequence == "OreSelect" then
                ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
                task.wait(0.45)
                if ForgeEngine._EnsureForgeOpen(FC, ForgeUI, PS, forgeModel) then
                    inject()
                    triggerMelt()
                end
            end
        end
    end

    if not FC.ForgeActive or FC.Sequence == "OreSelect" then
        return nil, "melt_rejected"
    end

    local forgeStart = os.clock()
    local newItem = nil
    local oreSelectIdleSince = nil
    local lastSeq = ""
    while ScriptRunning and (os.clock() - forgeStart) < 90.0 do
        task.wait(0.25)
        local curSeq = tostring(FC.Sequence or "")
        if curSeq ~= lastSeq and curSeq ~= "" and curSeq ~= "OreSelect" then
            lastSeq = curSeq
            if statusPrefix then
                ForgeEngine.SetWeaponHuntStatus(statusPrefix .. " (" .. curSeq .. ")", "Forging 1-1-1-1 Weapon...")
            end
        end
        ResolvePlayerData()
        local curEq = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory and State.PlayerData.Data.Inventory.Equipments
        if curEq then
            for _, item in ipairs(curEq) do
                if type(item) == "table" and item.GUID and not preGuids[item.GUID] then
                    newItem = item
                    break
                end
            end
        end
        if newItem and FC.Sequence == "OreSelect" then break end
        if (os.clock() - forgeStart) > 5.0 and not FC.ForgeActive then break end
        if not newItem and curSeq == "OreSelect" then
            oreSelectIdleSince = oreSelectIdleSince or os.clock()
            if (os.clock() - oreSelectIdleSince) > 5.5 then break end
        else
            oreSelectIdleSince = nil
        end
    end

    if newItem and newItem.GUID then
        preGuids[newItem.GUID] = true
        ForgeEngine.WaitForMinigamesClear(4.0)
        return newItem, nil
    end
    return nil, "timeout"
end

function ForgeEngine.RunWeaponHunt1111(opts)
    if State.IsAutoForging then return false end
    local batch, errReason = ForgeEngine.GetWeaponHuntBatch()
    if not batch then
        ForgeEngine.SetWeaponHuntStatus("1-1-1-1 Hunt: Stopped", errReason or "Invalid hunt config")
        Notify("1-1-1-1 Weapon Hunt", errReason or "Configure 4 non-blacklisted ores first.", 3.5)
        Config.WeaponHuntRunning = false
        return false
    end

    State.IsAutoForging = true
    local targetName = tostring(Config.WeaponHuntTarget or "Any Straight Sword")
    local matchedItem = nil
    local attempt = 0

    local ok, err = pcall(function()
        if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
        if State.Humanoid then State.Humanoid.AutoRotate = true end
        if State.PourMouseDown then
            State.PourMouseDown = false
            pcall(function() game:GetService("VirtualInputManager"):SendMouseButtonEvent(400, 300, 0, false, game, 0) end)
        end

        local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
        local Equipments = safeRequire(ReplicatedStorage.Shared.Data.Equipments)
        local FC = SafeGetController("ForgeController", KnitPkg)
        local UIC = SafeGetController("UIController", KnitPkg)
        local ForgeUI = UIC and UIC.Modules and UIC.Modules.Forge
        local PS = nil
        pcall(function() PS = KnitPkg and KnitPkg.GetService("ProximityService") end)
        local prox = workspace:FindFirstChild("Proximity")
        local forgeModel = prox and prox:FindFirstChild("Forge")
        if not FC or not forgeModel or not Equipments then
            ForgeEngine.SetWeaponHuntStatus("1-1-1-1 Hunt: Error", "Forge station or Equipments unavailable")
            return
        end

        ResolvePlayerData()
        local preGuids = {}
        local initialPreGuids = {}
        local inv = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory
        for _, item in ipairs((inv and inv.Equipments) or {}) do
            if type(item) == "table" and item.GUID then
                preGuids[item.GUID] = true
                initialPreGuids[item.GUID] = true
            end
        end

        local huntDudGuids = {}
        local function flushDuds()
            if Config.WeaponHuntSellDuds == false or not next(huntDudGuids) then return end
            ResolvePlayerData()
            local favs = (State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Favorites) or {}
            local basket = {}
            for g in pairs(huntDudGuids) do
                if not initialPreGuids[g] and not favs[g] and not ForgeEngine.IsItemEquipped(g) then
                    basket[g] = true
                end
            end
            table.clear(huntDudGuids)
            if next(basket) then
                ForgeEngine.SellEquipmentBasket(basket)
                task.wait(0.3)
            end
        end

        if FC.ForgeActive and FC.Sequence ~= "OreSelect" then
            ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
            task.wait(0.6)
        end
        if not ForgeEngine._EnsureForgeOpen(FC, ForgeUI, PS, forgeModel) then
            return
        end

        local maxRolls = (opts and opts.maxRolls) or 500
        while ScriptRunning and attempt < maxRolls do
            if not Config.WeaponHuntRunning and not (opts and opts.triggeredByOverride and Config.WeaponHuntOverride) then
                break
            end

            local rollBatch, rollErr = ForgeEngine.GetWeaponHuntBatch()
            if not rollBatch then
                ForgeEngine.SetWeaponHuntStatus("1-1-1-1 Hunt: Finished", rollErr or "Ores depleted")
                Notify("1-1-1-1 Weapon Hunt", rollErr or "Out of required hunt ores.", 3.5)
                Config.WeaponHuntRunning = false
                break
            end

            local _, _, eqFull, eqRem = ForgeEngine.GetEquipmentCapacityStatus()
            if eqFull or eqRem <= 1 then
                ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
                flushDuds()
                local _, _, stillFull, stillRem = ForgeEngine.GetEquipmentCapacityStatus()
                if (stillFull or stillRem <= 0) and Config.AutoEquipBestArmor then
                    ForgeEngine.SellAllUnequippedArmor()
                    task.wait(0.25)
                    _, _, stillFull, stillRem = ForgeEngine.GetEquipmentCapacityStatus()
                end
                if stillFull or stillRem <= 0 then
                    ForgeEngine.SetWeaponHuntStatus("1-1-1-1 Hunt: Bag Full", "Equipment bag full — free a slot to continue")
                    Notify("1-1-1-1 Weapon Hunt", "Equipment bag full! Clear/unfavorite a slot.", 3.5)
                    Config.WeaponHuntRunning = false
                    break
                end
                if not ForgeEngine._EnsureForgeOpen(FC, ForgeUI, PS, forgeModel) then
                    break
                end
            end

            attempt = attempt + 1
            ForgeEngine.SetWeaponHuntStatus(
                string.format("1-1-1-1 Roll #%d -> %s", attempt, targetName),
                "Forging 1-1-1-1 batch..."
            )

            local newItem, failReason = ForgeEngine._RunSingleForgeRoll(
                FC, ForgeUI, PS, forgeModel, rollBatch, "Weapon", nil, preGuids,
                string.format("1-1-1-1 #%d", attempt)
            )
            if not newItem then
                ForgeEngine.SetWeaponHuntStatus("1-1-1-1 Hunt: Interrupted", "Reason: " .. tostring(failReason))
                break
            end

            local isMatch = ForgeEngine.DoesWeaponMatchHuntTarget(Equipments, newItem, targetName)
            local qlty = math.round(tonumber(newItem.Quality) or 100)
            if isMatch then
                matchedItem = newItem
                Config.WeaponHuntRunning = false
                if Config.AutoEquipBestArmor and Remotes.EquipItem then
                    pcall(function() Remotes.EquipItem:InvokeServer(newItem) end)
                    task.wait(0.2)
                end
                local msg = string.format("Rolled %s (~%d%%) on Attempt #%d!", tostring(newItem.Type), qlty, attempt)
                ForgeEngine.SetWeaponHuntStatus("1-1-1-1 Target Obtained!", msg)
                Notify("1-1-1-1 Target Sword Found!", msg, 5.0)
                break
            else
                local refunded = false
                local refundErr = nil
                if Config.WeaponHuntRefundDuds and newItem.GUID then
                    refunded, refundErr = ForgeEngine.RecycleItemByGuid(newItem.GUID, newItem)
                end
                if not refunded and Config.WeaponHuntSellDuds ~= false and newItem.GUID then
                    huntDudGuids[newItem.GUID] = true
                end
                local stepDesc = refunded
                    and ("Not target (" .. targetName .. ") -> Refunded Ores! Rolling again...")
                    or (refundErr == "insufficient_gold"
                        and ("Not enough Gold to refund -> Rolling again...")
                        or ("Not target (" .. targetName .. ") -> Rolling again..."))
                ForgeEngine.SetWeaponHuntStatus(
                    string.format("Roll #%d: %s (~%d%%)", attempt, tostring(newItem.Type), qlty),
                    stepDesc
                )
                task.wait(0.85)
            end
        end

        ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
        if Remotes.CheckCraftCompletions then
            pcall(function() Remotes.CheckCraftCompletions:InvokeServer() end)
        end
        flushDuds()
    end)

    if not ok then
        warn("[The Forge 1-1-1-1 Hunt]:", err)
        ForgeEngine.SetWeaponHuntStatus("1-1-1-1 Hunt Error", tostring(err))
    end
    State.IsAutoForging = false
    return matchedItem ~= nil, matchedItem
end

-- 8A-5. End-to-End Automated Forge & Sell Execution (Strict Mutex + Clean Multi-Batch & Piece Queue Chaining)
function ForgeEngine.ExecuteArmorForgeAndSell(customBatchOrQueue, itemTypeOverride, skipAutoSell, lockedClassOverride)
    if Config.WeaponHuntOverride and itemTypeOverride == "Weapon" then
        return ForgeEngine.RunWeaponHunt1111({ triggeredByOverride = true })
    end
    if State.IsAutoForging then return false end
    local targetItemType = (itemTypeOverride == "Weapon") and "Weapon" or "Armor"
    local isQueue = type(customBatchOrQueue) == "table" and customBatchOrQueue[1] ~= nil and type(customBatchOrQueue[1]) == "table" and customBatchOrQueue[1].ores ~= nil

    ResolvePlayerData()
    local batch, totalOres = nil, 0
    local queueIdx = 1
    local activeLockedClass = lockedClassOverride

    local blacklisted = ForgeEngine.GetBlacklistedForgeOresMap()
    local function sanitizeBatch(raw)
        if type(raw) ~= "table" then return nil, 0 end
        local clean, tot = {}, 0
        for oreName, cnt in pairs(raw) do
            if not blacklisted[oreName] and type(cnt) == "number" and cnt > 0 then
                clean[oreName] = cnt
                tot = tot + cnt
            end
        end
        return clean, tot
    end

    if isQueue then
        local first = customBatchOrQueue[1]
        batch, totalOres = sanitizeBatch(first.ores)
        targetItemType = (first.itemType == "Weapon") and "Weapon" or "Armor"
        activeLockedClass = first.lockedClass
    elseif customBatchOrQueue then
        batch, totalOres = sanitizeBatch(customBatchOrQueue)
    elseif targetItemType == "Weapon" then
        batch, totalOres = ForgeEngine.GetOptimalWeaponBatch(35)
    else
        batch, totalOres = ForgeEngine.GetOptimalArmorBatch(Config.AutoForgeTargetOres or 31)
        if (State.DrainingFullBag or State.ManualDrainOnce) and (not batch or totalOres < 3) then
            batch, totalOres = ForgeEngine.GetOptimalArmorBatch(3)
        end
    end
    if not batch or totalOres < 3 then
        if ForgeEngine.AutoForgeStatusLabel then
            ForgeEngine.AutoForgeStatusLabel:SetTitle("Auto-Forge: Waiting for Ores")
            ForgeEngine.AutoForgeStatusLabel:SetDesc(string.format("Need >= 3 non-blacklisted ores (usable: %d)", totalOres or 0))
        end
        return false
    end

    State.IsAutoForging = true
    State.AutoForgeDebug = "1_start"
    -- Clear unequipped non-favorite/non-protected Armor proactively before forging if remaining equipment slots are low
    if not skipAutoSell then
        local eqUsed, eqCap, isEqFull, eqRem = ForgeEngine.GetEquipmentCapacityStatus()
        if isEqFull or eqRem <= 3 or (eqCap > 0 and eqUsed >= math.max(1, eqCap - 2)) then
            local cleared = ForgeEngine.SellAllUnequippedArmor()
            if cleared > 0 then
                task.wait(0.25)
            end
            local _, _, stillFull, stillRem = ForgeEngine.GetEquipmentCapacityStatus()
            if stillFull or stillRem <= 0 then
                ForgeEngine.SetStatus(
                    "Auto-Forge Paused: Equipment Bag Full",
                    "Remaining items are Favorited or Protected — free a slot to resume forging!"
                )
                State.DrainingFullBag = false
                State.ManualDrainOnce = false
                State.IsAutoForging = false
                return false
            end
        end
    end
    if ForgeEngine.AutoForgeStatusLabel then
        pcall(function()
            ForgeEngine.AutoForgeStatusLabel:SetTitle(string.format("Auto-Forge: Preparing (%d Ores)", totalOres))
            ForgeEngine.AutoForgeStatusLabel:SetDesc("Initializing Forge station session...")
        end)
    end
    local ok, err = pcall(function()
        -- Release any mining anchor and held minigame mouse state before starting
        if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
        if State.Humanoid then State.Humanoid.AutoRotate = true end
        if State.PourMouseDown then
            State.PourMouseDown = false
            pcall(function() game:GetService("VirtualInputManager"):SendMouseButtonEvent(400, 300, 0, false, game, 0) end)
        end

        -- Snapshot existing equipment GUIDs across the entire multi-batch forge session
        -- initialPreGuids stays untouched so ONLY items forged inside THIS call can ever be sold
        local preGuids = {}
        local initialPreGuids = {}
        local inv = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory
        for _, item in ipairs((inv and inv.Equipments) or {}) do
            if type(item) == "table" and item.GUID then
                preGuids[item.GUID] = true
                initialPreGuids[item.GUID] = true
            end
        end

        local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
        local FC = SafeGetController("ForgeController", KnitPkg)
        local UIC = SafeGetController("UIController", KnitPkg)
        local ForgeUI = UIC and UIC.Modules and UIC.Modules.Forge
        local PS = nil
        pcall(function() PS = KnitPkg.GetService("ProximityService") end)
        local prox = workspace:FindFirstChild("Proximity")
        local forgeModel = prox and prox:FindFirstChild("Forge")
        if not FC or not forgeModel then
            State.AutoForgeDebug = "err_no_fc_or_model"
            return
        end

        local function isForgeAborted()
            if not ScriptRunning then return true end
            if not customBatchOrQueue and not State.ManualDrainOnce and not Config.AutoForgeArmor then
                return true
            end
            return false
        end

        local function openForgeSession()
            if isForgeAborted() then return end
            if type(ForgeEngine.EnsureMinigameHooks) == "function" then
                pcall(ForgeEngine.EnsureMinigameHooks, FC)
            end
            ForgeEngine.SessionEpoch = (ForgeEngine.SessionEpoch or 0) + 1
            State.AutoForgeDebug = "2_open_check_dist"
            local forgePos = forgeModel:IsA("Model") and forgeModel:GetPivot().Position or forgeModel.Position
            if State.HRP and (State.HRP.Position - forgePos).Magnitude > 10 then
                if ForgeEngine.AutoForgeStatusLabel then
                    pcall(function()
                        ForgeEngine.AutoForgeStatusLabel:SetTitle("Auto-Forge: Tweening to Forge")
                        ForgeEngine.AutoForgeStatusLabel:SetDesc("Moving to town Forge station...")
                    end)
                end
                NavEngine.GlideTo(CFrame.new(forgePos + Vector3.new(0, 2.8, 0)), Config.GlideSpeed or 55, true)
            end
            if isForgeAborted() then return end

            -- Server-authoritative Forge opening via ProximityService:Forge (only when not already active)
            State.AutoForgeDebug = "3_ps_forge"
            runAsGameThread(function()
                if PS and type(PS.Forge) == "function" then
                    pcall(function() PS:Forge(forgeModel):await() end)
                elseif Remotes.ProximityForge then
                    pcall(function() Remotes.ProximityForge:InvokeServer(forgeModel) end)
                end
            end, 4.5)
            if not FC.ForgeActive then
                runAsGameThread(function()
                    pcall(function() FC:StartForge(forgeModel) end)
                end, 2.5)
            end
            State.AutoForgeDebug = "4_wait_oreselect"
            local waitStart = os.clock()
            while (not FC.ForgeActive or FC.Sequence ~= "OreSelect") and (os.clock() - waitStart) < 6.0 do
                task.wait(0.15)
            end
        end

        -- 1. Open Forge cleanly once (if stuck mid-sequence, close & wait cleanly first)
        if FC.ForgeActive and FC.Sequence ~= "OreSelect" then
            ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
            task.wait(0.6)
        end
        if isForgeAborted() then
            ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
            return
        end
        if not FC.ForgeActive then
            openForgeSession()
        else
            local forgePos = forgeModel:IsA("Model") and forgeModel:GetPivot().Position or forgeModel.Position
            if State.HRP and (State.HRP.Position - forgePos).Magnitude > 10 then
                NavEngine.GlideTo(CFrame.new(forgePos + Vector3.new(0, 2.8, 0)), Config.GlideSpeed or 55, true)
            end
        end
        if isForgeAborted() then
            ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
            return
        end

        local seqWait = os.clock()
        while FC.ForgeActive and FC.Sequence ~= "OreSelect" and (os.clock() - seqWait) < 6.0 do
            task.wait(0.15)
        end
        if not FC.ForgeActive then
            State.AutoForgeDebug = "err_fc_inactive"
            ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
            return
        end
        -- Wait past ForgeController OreSelect task.delay(0.4) -> ForgeUI:Open() so Open() never wipes our injected ores
        task.wait(0.9)
        pcall(function()
            local pgui = LocalPlayer:FindFirstChild("PlayerGui")
            local osFrame = pgui and pgui:FindFirstChild("Forge") and pgui.Forge:FindFirstChild("OreSelect")
            local openWait = os.clock()
            while FC.ForgeActive and osFrame and not osFrame.Visible and (os.clock() - openWait) < 2.0 do
                task.wait(0.1)
            end
        end)

        local newlyForgedItems = {}
        local currentBatch = batch
        local currentTotal = totalOres
        local batchIndex = 0

        -- Chain batches sequentially inside the open session so minigames NEVER overlap!
        while ScriptRunning and currentBatch and currentTotal >= 3 and batchIndex < 200 do
            if isForgeAborted() then break end
            batchIndex = batchIndex + 1
            ForgeEngine.WaitForMinigamesClear(4.0)

            -- Ensure ForgeController's ChangeSequence("OreSelect") task.delay(0.4) has opened OreSelect before injecting batch
            if batchIndex > 1 then
                local osReadyWait = os.clock()
                local pgui = LocalPlayer:FindFirstChild("PlayerGui")
                local osFrame = pgui and pgui:FindFirstChild("Forge") and pgui.Forge:FindFirstChild("OreSelect")
                while FC.ForgeActive and osFrame and not osFrame.Visible and (os.clock() - osReadyWait) < 2.5 do
                    task.wait(0.1)
                end
                task.wait(0.15)
            end

            -- Always ensure clean mouse release before next Melt -> Pour sequence
            if State.PourMouseDown then
                State.PourMouseDown = false
                pcall(function() game:GetService("VirtualInputManager"):SendMouseButtonEvent(400, 300, 0, false, game, 0) end)
            end

            local function injectArmorBatch(targetBatch)
                State.AutoForgeDebug = "5_inject_batch_" .. tostring(batchIndex)
                if ForgeUI then
                    -- Run ForgeUI module methods in an isolated identity-2 child task
                    runAsGameThread(function()
                        pcall(function() ForgeUI:SelectItemType(targetItemType) end)
                        ForgeUI.selectedItemType = targetItemType
                        ForgeUI.LockedClass = nil
                        ForgeUI.addedOres = table.clone(targetBatch)
                        ForgeEngine.ApplyFastForgePreference(ForgeUI)
                        pcall(function() ForgeUI:ClearMolds() end)
                        ForgeUI.LockedClass = activeLockedClass
                        pcall(function() ForgeUI:UpdateAddedOres() end)
                        if ForgeUI.UpdateProbabilities then
                            pcall(function() ForgeUI:UpdateProbabilities() end)
                        end
                    end, 1.5)
                end
                FC.ItemType = targetItemType
                FC.Ores = table.clone(targetBatch)
            end

            injectArmorBatch(currentBatch)

            State.AutoForgeDebug = "6_calc_and_label"
            local calcOk, calcInfo = pcall(ForgeEngine.CalculateRecipe, currentBatch, targetItemType, 100)
            local estGold = (calcOk and type(calcInfo) == "table" and math.floor(calcInfo.expectedPrice or 0)) or 0
            local pieceTag = activeLockedClass and (targetItemType .. " [" .. tostring(activeLockedClass) .. "]") or targetItemType
            ForgeEngine.SetStatus(
                string.format("Forging %s #%d (%d Ores -> ~$%d)", pieceTag, batchIndex, currentTotal, estGold),
                "Solving Melt -> Pour -> Hammer sequentially..."
            )

            -- Note: Auto-Forge uses its own non-configurable solver path via State.IsAutoForging
            -- without overwriting user Config.AutoMelt / AutoPour / AutoHammer / AutoMoldBreak / AutoClaimForged / AutoClaimSmelt toggles.

            local function triggerMeltClean()
                local meltCallDone = false
                task.spawn(function()
                    local setId = (typeof(setthreadidentity) == "function" and setthreadidentity)
                        or (typeof(setidentity) == "function" and setidentity)
                        or (syn and typeof(syn.set_thread_identity) == "function" and syn.set_thread_identity)
                    if setId then pcall(setId, 2) end
                    local mOk, mErr = pcall(function() FC:ChangeSequence("Melt") end)
                    if not mOk then State.AutoForgeDebug = "melt_err:" .. tostring(mErr) end
                    meltCallDone = true
                end)
                -- Wait for server ForgeService:ChangeSequence("Melt"):await() to return BEFORE checking FC.Sequence
                local checkMelt = os.clock()
                while not meltCallDone and (os.clock() - checkMelt) < 6.0 do
                    if FC.Sequence == "Pour" or FC.Sequence == "Hammer" or FC.Sequence == "Water" then
                        break
                    end
                    task.wait(0.1)
                end
                -- Mirror UIController.Forge button cleanup once Melt is accepted
                if FC.Sequence ~= "OreSelect" and ForgeUI then
                    pcall(function() ForgeUI.addedOres = {} end)
                end
                return meltCallDone
            end

            -- 3. Trigger Melt cleanly; if server rejects back to OreSelect, retry in-session, then fresh-reopen Forge
            State.AutoForgeDebug = "7_melt_call_" .. tostring(batchIndex)
            local meltCallDone = triggerMeltClean()

            if meltCallDone and FC.Sequence == "OreSelect" then
                State.AutoForgeDebug = "8_melt_retry_" .. tostring(batchIndex)
                task.wait(0.9)
                if FC.ForgeActive and FC.Sequence == "OreSelect" then
                    injectArmorBatch(currentBatch)
                    local retryDone = triggerMeltClean()
                    if retryDone and FC.Sequence == "OreSelect" then
                        -- Server rejected Melt in this stale session -> cleanly reset Forge session once
                        State.AutoForgeDebug = "8b_reopen_forge_" .. tostring(batchIndex)
                        ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
                        task.wait(0.45)
                        openForgeSession()
                        if FC.ForgeActive then
                            task.wait(0.85)
                            injectArmorBatch(currentBatch)
                            triggerMeltClean()
                        end
                    end
                end
            end

            if not FC.ForgeActive or FC.Sequence == "OreSelect" then
                ForgeEngine.SetStatus(
                    string.format("Auto-Forge: Batch #%d Melt Rejected", batchIndex),
                    "Restarting Forge loop cleanly..."
                )
                break
            end

            State.AutoForgeDebug = "9_wait_item_" .. tostring(batchIndex)
            local forgeStart = os.clock()
            local newArmorItem = nil
            local lastReportedSeq = ""
            local oreSelectIdleSince = nil
            while (os.clock() - forgeStart) < 90.0 do
                task.wait(0.25)
                local curSeq = tostring(FC.Sequence or "")
                if curSeq ~= lastReportedSeq and curSeq ~= "" and curSeq ~= "OreSelect" then
                    lastReportedSeq = curSeq
                    ForgeEngine.SetStatus(string.format("Forging Armor #%d: %s (%d Ores)", batchIndex, curSeq, currentTotal))
                end

                ResolvePlayerData()
                local curEq = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory and State.PlayerData.Data.Inventory.Equipments
                if curEq then
                    for _, item in ipairs(curEq) do
                        if type(item) == "table" and item.GUID and not preGuids[item.GUID] then
                            newArmorItem = item
                            break
                        end
                    end
                end
                if newArmorItem and FC.Sequence == "OreSelect" then
                    break
                end
                if (os.clock() - forgeStart) > 5.0 and not FC.ForgeActive then
                    break
                end
                if not newArmorItem and curSeq == "OreSelect" then
                    oreSelectIdleSince = oreSelectIdleSince or os.clock()
                    if (os.clock() - oreSelectIdleSince) > 5.5 then
                        break
                    end
                else
                    oreSelectIdleSince = nil
                end
            end

            if newArmorItem and newArmorItem.GUID then
                preGuids[newArmorItem.GUID] = true
                table.insert(newlyForgedItems, newArmorItem)
                ForgeEngine.WaitForMinigamesClear(4.0)
            else
                ForgeEngine.SetStatus(
                    "Auto-Forge: Timed Out (" .. tostring(FC.Sequence) .. ")",
                    "Stopping batch cleanly to recover..."
                )
                break
            end

            -- Continue chaining next queued piece (All 3 Armor Pieces) or continuous AutoForgeArmor / Full-Bag Drain
            if isQueue then
                queueIdx = queueIdx + 1
                local nextEntry = customBatchOrQueue[queueIdx]
                if not nextEntry or not nextEntry.ores or not FC.ForgeActive then
                    break
                end
                currentBatch, currentTotal = sanitizeBatch(nextEntry.ores)
                if not currentBatch or currentTotal < 3 then
                    break
                end
                targetItemType = (nextEntry.itemType == "Weapon") and "Weapon" or "Armor"
                activeLockedClass = nextEntry.lockedClass
                task.wait(1.0)
            else
                if customBatchOrQueue or (not Config.AutoForgeArmor and not State.DrainingFullBag and not State.ManualDrainOnce) or not FC.ForgeActive then
                    break
                end
                if not skipAutoSell then
                    local _, _, eqFullNow, eqRemNow = ForgeEngine.GetEquipmentCapacityStatus()
                    if eqFullNow or eqRemNow <= 1 then
                        ForgeEngine.SetStatus(
                            "Auto-Forge: Equipment Bag Full",
                            "Closing Forge cleanly to sell crafted Armor before continuing..."
                        )
                        break
                    end
                end
                local targetPerBatch = Config.AutoForgeTargetOres or 31
                local nextBatch, nextTotal = ForgeEngine.GetOptimalArmorBatch(targetPerBatch)
                if not nextBatch or nextTotal < 3 then
                    break
                end
                currentBatch = nextBatch
                currentTotal = nextTotal
                task.wait(0.85) -- Additional settle buffer; batchIndex > 1 loop also waits for OreSelect.Visible
            end
        end

        -- 4. Close BOTH Forge UI & Station Session cleanly FIRST so UIController never thinks Forge is still open
        State.AutoForgeDebug = "10_close_before_sell"
        ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
        if Remotes.CheckCraftCompletions then
            pcall(function() Remotes.CheckCraftCompletions:InvokeServer() end)
        end

        -- Poll Replica up to 1.8s so late-replicating items from the final batch are always detected
        local trackedNewlyForged = {}
        for _, item in ipairs(newlyForgedItems) do
            if type(item) == "table" and item.GUID then
                trackedNewlyForged[item.GUID] = true
            end
        end
        local settleStart = os.clock()
        local lastFoundAt = settleStart
        while (os.clock() - settleStart) < 1.8 do
            task.wait(0.2)
            ResolvePlayerData()
            local postEq = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory and State.PlayerData.Data.Inventory.Equipments
            local addedNow = false
            if type(postEq) == "table" then
                for _, item in ipairs(postEq) do
                    if type(item) == "table" and item.GUID and not initialPreGuids[item.GUID] and not trackedNewlyForged[item.GUID] then
                        trackedNewlyForged[item.GUID] = true
                        table.insert(newlyForgedItems, item)
                        addedNow = true
                        lastFoundAt = os.clock()
                    end
                end
            end
            if not addedNow and (#newlyForgedItems >= batchIndex and (os.clock() - settleStart) >= 0.4 or (os.clock() - lastFoundAt) >= 0.65) then
                break
            end
        end

        -- 5. Equip best Armor first (if 'Equip Best Then Sell' is enabled), then sell crafted/pending + unequipped Armor
        State.AutoForgeDebug = "11_sell"
        local Equipments = safeRequire(ReplicatedStorage.Shared.Data.Equipments)
        if not Equipments then
            if #newlyForgedItems > 0 then
                ForgeEngine.SetStatus("Auto-Forge: Equipments Missing", "Could not load Equipments module")
            end
            return
        end

        local eqCount = 0
        local replacedOldGuids = {}
        if #newlyForgedItems > 0 or (not skipAutoSell and targetItemType == "Armor" and (Config.AutoForgeArmor or Config.AutoEquipBestArmor)) then
            if Config.AutoEquipBestArmor then
                local prevEquippedGuids = {}
                local curEq = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Equipped
                if type(curEq) == "table" then
                    for _, slot in ipairs({ "Helmet", "Chestplate", "Leggings" }) do
                        local worn = curEq[slot]
                        if type(worn) == "table" and worn.GUID then
                            prevEquippedGuids[worn.GUID] = true
                        end
                    end
                end
                State.LastBestArmorCheck = 0
                eqCount = ForgeEngine.EquipBestArmor()
                if eqCount > 0 then
                    task.wait(0.35)
                    ResolvePlayerData()
                    for g in pairs(prevEquippedGuids) do
                        if not ForgeEngine.IsItemEquipped(g) then
                            replacedOldGuids[g] = true
                        end
                    end
                end
            end

            local favs = (State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Favorites) or {}
            local protArmorGuids = ForgeEngine.GetProtectedArmorSets(Equipments)
            State.PendingSellArmorGuids = State.PendingSellArmorGuids or {}
            local totalSellVal = 0
            local sellBasket = {}
            local sellCount = 0

            for _, item in ipairs(newlyForgedItems) do
                local itemType = Equipments:GetItemType(item.Type or item.Name)
                totalSellVal = totalSellVal + math.floor(ForgeEngine.SafeCalcPrice(Equipments, item) or 0)
                if item.GUID and itemType == "Armor" and not ForgeEngine.IsItemEquipped(item.GUID)
                    and not favs[item.GUID] and not protArmorGuids[item.GUID] then
                    State.PendingSellArmorGuids[item.GUID] = true
                end
            end
            for oldGuid in pairs(replacedOldGuids) do
                if not favs[oldGuid] and not ForgeEngine.IsItemEquipped(oldGuid) and not protArmorGuids[oldGuid] then
                    State.PendingSellArmorGuids[oldGuid] = true
                end
            end

            if not skipAutoSell and targetItemType == "Armor" then
                -- Build sellBasket from ALL live sellable unequipped Armors (respects Equipped, Favorites, Protect Custom Armor)
                -- plus any still-present PendingSellArmorGuids so multi-session retries never abandon items in initialPreGuids
                local allSellableBasket = ForgeEngine.GetSellableUnequippedArmorBasket(Equipments)
                if Config.AutoForgeArmor or Config.AutoEquipBestArmor then
                    for g in pairs(allSellableBasket) do
                        sellBasket[g] = true
                    end
                else
                    for g in pairs(State.PendingSellArmorGuids) do
                        if allSellableBasket[g] then
                            sellBasket[g] = true
                        end
                    end
                end
                for _ in pairs(sellBasket) do
                    sellCount = sellCount + 1
                end
            end

            if not skipAutoSell and targetItemType == "Armor" and sellCount > 0 then
                local soldOk = ForgeEngine.SellEquipmentBasket(sellBasket)
                local lastItem = newlyForgedItems[#newlyForgedItems]
                local qlty = lastItem and math.round(lastItem.Quality or 100) or 100
                local eqPrefix = (eqCount > 0) and string.format("Equipped %d Best & ", eqCount) or ""
                local statusTitle = soldOk
                    and string.format("%sSold %d Armor(s) [~%d%%] (+$%d)!", eqPrefix, sellCount, qlty, totalSellVal)
                    or string.format("%sSold/Attempted %d Armor(s)", eqPrefix, sellCount)
                local statusDesc = soldOk
                    and "All batches crafted & unequipped/replaced armors sold!"
                    or "Check Marbles sell response if protected/remaining items exist"
                ForgeEngine.SetStatus(statusTitle, statusDesc)
                if ForgeEngine.ArmorSellStatusLabel then
                    pcall(function()
                        ForgeEngine.ArmorSellStatusLabel:SetTitle(statusTitle)
                        ForgeEngine.ArmorSellStatusLabel:SetDesc(statusDesc)
                    end)
                end
            elseif #newlyForgedItems > 0 then
                local lastItem = newlyForgedItems[#newlyForgedItems]
                if eqCount > 0 then
                    ForgeEngine.SetStatus(
                        string.format("Forged & Equipped %d Best Armor(s)!", eqCount),
                        "Latest: " .. tostring(lastItem.Type or targetItemType) .. " saved & best equipped"
                    )
                else
                    ForgeEngine.SetStatus(
                        string.format("Forged %d %s(s) ($%d)", #newlyForgedItems, targetItemType, totalSellVal),
                        "Latest: " .. tostring(lastItem.Type or targetItemType) .. " saved to inventory"
                    )
                end
            end
        end

        State.LastNewlyForgedItems = newlyForgedItems
        State.AutoForgeDebug = "12_done"
    end)
    if not ok then
        State.AutoForgeDebug = "pcall_err:" .. tostring(err)
        warn("[The Forge Auto-Forge]:", err)
        ForgeEngine.SetStatus("Auto-Forge Error", tostring(err))
    end
    State.IsAutoForging = false
    return ok
end

-- Hook NotificationController / UIController Notifications to detect rejected "Stash Full" mining drops immediately
task.spawn(function()
    pcall(function()
    local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
    local UIC = SafeGetController("UIController", KnitPkg)
    local NotifMod = UIC and UIC.Modules and UIC.Modules.Notifications
    if NotifMod and type(NotifMod.Notify) == "function" then
        safeHook(NotifMod, "Notify", "AutoForge_StashFullNotify", function(self, payload, ...)
            if type(payload) == "table" and type(payload.Text) == "string" then
                local plain = payload.Text:gsub("<[^>]->", ""):lower()
                if (plain:find("stash", 1, true) or plain:find("inventory", 1, true) or plain:find("bag", 1, true))
                    and plain:find("full", 1, true) then
                    State.StashFullSignalUntil = os.clock() + 15.0
                end
            end
            return OrigHooks["AutoForge_StashFullNotify"].orig(self, payload, ...)
        end)
    end
    end)
end)

function ForgeEngine.GetStashCapacityStatus()
    local used, cap = 0, 0
    local gotUsed, gotCap = false, false
    pcall(function()
        local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
        local UIC = SafeGetController("UIController", KnitPkg)
        local InvUI = UIC and UIC.Modules and UIC.Modules.Inventory
        if InvUI then
            if type(InvUI.CalculateTotal) == "function" then
                local u = InvUI:CalculateTotal("Stash")
                if type(u) == "number" then
                    used = u
                    gotUsed = true
                end
            end
            if type(InvUI.GetBagCapacity) == "function" then
                local c = InvUI:GetBagCapacity()
                if type(c) == "number" and c > 0 then
                    cap = c
                    gotCap = true
                end
            end
        end
    end)

    -- Direct Replica.Data fallback if InvUI wasn't ready or returned 0
    if not gotUsed or not gotCap then
        ResolvePlayerData()
        local data = State.PlayerData and State.PlayerData.Data
        if data then
            if not gotCap and type(data.Level) == "number" then
                local dbl = data.Gamepasses and data.Gamepasses.DoubleStorage
                cap = (100 + data.Level * 2) * (dbl and 2 or 1)
            end
            if not gotUsed and type(data.Inventory) == "table" then
                local sum = 0
                local itemTypes = (typeof(getrenv) == "function" and getrenv()._G and getrenv()._G.ItemTypes) or _G.ItemTypes
                for k, v in pairs(data.Inventory) do
                    if type(v) == "number" and v > 0 then
                        if (itemTypes and itemTypes[k] and itemTypes[k][1] == "Ore") or ForgeEngine.OreDataByName[k] then
                            sum = sum + v
                        end
                    end
                end
                if type(data.Inventory.Misc) == "table" then
                    for _, entry in pairs(data.Inventory.Misc) do
                        if type(entry) == "table" then
                            local id = entry.Id
                            local name = entry.Name
                            if id and itemTypes and itemTypes[id] and itemTypes[id][1] == "Rune" then
                                sum = sum + 1
                            elseif name and itemTypes and itemTypes[name] and itemTypes[name][1] == "Potion" then
                                sum = sum + (entry.Quantity or 1)
                            end
                        end
                    end
                end
                used = sum
            end
        end
    end

    local usableOres = 0
    pcall(function()
        for _, ore in ipairs(ForgeEngine.GetPlayerOreInventory()) do
            usableOres = usableOres + (ore.Count or 0)
        end
    end)

    local notifiedFull = (State.StashFullSignalUntil or 0) > os.clock()
    local pct = math.clamp(tonumber(Config.AutoForgeTriggerPct) or 100, 25, 100)
    local triggerAt = (cap > 0) and math.max(1, math.floor(cap * (pct / 100))) or 0
    local isFull = (cap > 0 and (used >= triggerAt or notifiedFull))
    return used, cap, isFull, usableOres
end

local WebhookEngine = {
    ForgeBeforeSnapshot = nil,
    NotifiedBagFull = false,
}

-- Background Auto-Forge Profit Loop & Continuous 1-1-1-1 Weapon Hunt Loop
task.spawn(function()
    while ScriptRunning do
        if Config.WeaponHuntRunning and not State.IsAutoForging then
            pcall(ForgeEngine.RunWeaponHunt1111, { triggeredByLoop = true })
            task.wait(0.5)
        end
        local autoEnabled = Config.AutoForgeArmor == true
        local draining = State.DrainingFullBag == true or State.ManualDrainOnce == true
        if autoEnabled or draining then
            if not State.IsAutoForging then
                local used, cap, isFull, usableOres = ForgeEngine.GetStashCapacityStatus()
                if (autoEnabled and isFull) or State.ManualDrainOnce then
                    if usableOres >= 3 then
                        if not State.DrainingFullBag then
                            State.DrainingFullBag = true
                            State.DidWeaponUpgradeCheckThisCycle = false
                            State.DidCraftBeforeForgeThisCycle = false
                            State.StashFullSignalUntil = 0
                            local pdData = (State.PlayerData and State.PlayerData.Data) or {}
                            WebhookEngine.ForgeBeforeSnapshot = {
                                gold = tonumber(pdData.Gold) or 0,
                                level = tonumber(pdData.Level) or (LocalPlayer and tonumber(LocalPlayer:GetAttribute("Level"))) or 1,
                                bagUsed = used,
                                bagCap = cap,
                                usableOres = usableOres,
                                startedAt = os.clock(),
                            }
                            if Config.WebhookEnabled and Config.WebhookNotifyForgeCycle and type(WebhookEngine.SendForgeCycleReport) == "function" then
                                task.spawn(WebhookEngine.SendForgeCycleReport, "before", WebhookEngine.ForgeBeforeSnapshot)
                            end
                        else
                            State.StashFullSignalUntil = 0
                        end
                    else
                        State.DrainingFullBag = false
                        State.ManualDrainOnce = false
                        ForgeEngine.SetStatus(
                            string.format("Bag %d / %d | %d Usable Ores", used, cap, usableOres),
                            "Fewer than 3 non-blacklisted ores available to forge!"
                        )
                    end
                end
                if State.DrainingFullBag then
                    -- Priority: Auto Mine / Auto Combat > Auto Craft > Auto Forge
                    if Config.AutoCraft and State.CraftEngine and not State.DidCraftBeforeForgeThisCycle then
                        State.DidCraftBeforeForgeThisCycle = true
                        pcall(State.CraftEngine.RunAutoCraftCycle, true)
                        task.wait(0.25)
                        local _, _, _, remainingUsable = ForgeEngine.GetStashCapacityStatus()
                        if remainingUsable < 3 then
                            State.DrainingFullBag = false
                            State.ManualDrainOnce = false
                            State.StashFullSignalUntil = 0
                            ForgeEngine.SetStatus(
                                "Auto-Craft Used Remaining Ores",
                                "Prioritized Auto Craft -> resuming Auto Mine / Standby"
                            )
                            task.wait(1.0)
                            continue
                        end
                    end
                    if Config.AutoCraftBestWeaponFirst and not State.DidWeaponUpgradeCheckThisCycle then
                        State.DidWeaponUpgradeCheckThisCycle = true
                        pcall(ForgeEngine.TryCraftBestWeaponIfUpgrade)
                        task.wait(0.35)
                    end
                    local batch, totalOres = ForgeEngine.GetOptimalArmorBatch(Config.AutoForgeTargetOres or 31)
                    if batch and totalOres >= 3 then
                        ForgeEngine.ExecuteArmorForgeAndSell()
                        -- Allow Replica inventory updates to replicate before checking remaining ores
                        task.wait(0.5)
                    else
                        -- Final post-drain equipment sweep: guarantee no unequipped non-favorite/non-protected Armor remains unsold
                        local remBasket, remCount = ForgeEngine.GetSellableUnequippedArmorBasket()
                        if remCount > 0 then
                            ForgeEngine.SellAllUnequippedArmor()
                        end
                        State.DrainingFullBag = false
                        State.ManualDrainOnce = false
                        State.DidCraftBeforeForgeThisCycle = false
                        State.StashFullSignalUntil = 0
                        local snap = WebhookEngine.ForgeBeforeSnapshot
                        WebhookEngine.ForgeBeforeSnapshot = nil
                        if Config.WebhookEnabled and Config.WebhookNotifyForgeCycle and type(WebhookEngine.SendForgeCycleReport) == "function" then
                            task.spawn(WebhookEngine.SendForgeCycleReport, "after", snap)
                        end
                        ForgeEngine.SetStatus(
                            string.format("Bag Drained (%d / %d) -> Resuming Mine", used, cap),
                            "All non-blacklisted ores forged (< 3 left) & armors sold!"
                        )
                    end
                elseif autoEnabled and not isFull then
                    local pct = math.clamp(tonumber(Config.AutoForgeTriggerPct) or 100, 25, 100)
                    ForgeEngine.SetStatus(
                        string.format("Auto-Forge: Bag %d / %d (%d usable)", used, cap, usableOres),
                        string.format("Triggers at %d%% bag fill (or click Forge All Remaining Ores Now)", pct)
                    )
                end
            end
        else
            State.DrainingFullBag = false
            State.ManualDrainOnce = false
            pcall(function()
                local used, cap, isFull, usableOres = ForgeEngine.GetStashCapacityStatus()
                ForgeEngine.SetStatus(
                    string.format("Auto-Forge: OFF | Bag %d / %d%s", used, cap, isFull and " [FULL]" or ""),
                    string.format("Enable Auto-Forge or click Forge All (%d usable ores)", usableOres)
                )
            end)
        end
        task.wait(1.5)
    end
end)

-- 8A & 8A-2. Idempotent Forge Minigame Hooks (HammerMinigame, GolemMinigame, MeltMinigame, PourMinigame)
function ForgeEngine.EnsureMinigameHooks(fcInstance)
    local fcFolder = ReplicatedStorage:FindFirstChild("Controllers")
        and ReplicatedStorage.Controllers:FindFirstChild("ForgeController")
    if not fcFolder then return false end

    -- Ensure FC.Minigames table is populated if StartForge is ever called directly before ForgeService.Event fires
    pcall(function()
        local FC = fcInstance
        if not FC then
            local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
            FC = SafeGetController("ForgeController", KnitPkg)
        end
        if type(FC) == "table" and type(FC.Minigames) ~= "table" then
            local mgMap = {}
            local allLoaded = true
            for _, ch in ipairs(fcFolder:GetChildren()) do
                if ch:IsA("ModuleScript") then
                    local okM, modTbl = pcall(function() return safeRequire(ch) end)
                    if okM and type(modTbl) == "table" then
                        mgMap[ch.Name] = modTbl
                    else
                        allLoaded = false
                    end
                end
            end
            if allLoaded and next(mgMap) ~= nil then
                FC.Minigames = mgMap
            end
        end
    end)

    -- 1. Auto-Hammer: Hook HammerMinigame.CreateNote (Synchronous oldCreateNote + exact server-clock click)
    if not OrigHooks["AutoHammer_NoteHook"] then
        pcall(function()
            local hammerMod = fcFolder:FindFirstChild("HammerMinigame")
            local HammerMG = hammerMod and safeRequire(hammerMod)
            if HammerMG and type(HammerMG.CreateNote) == "function" then
                local oldCreateNote
                oldCreateNote = safeHook(HammerMG, "CreateNote", "AutoHammer_NoteHook", function(self, noteData, anvilPart, markerAttachment)
                    local hammerActive = Config.AutoHammer or State.IsAutoForging
                    local capturedFrame = nil
                    local childConn = nil
                    local pgui = LocalPlayer:FindFirstChild("PlayerGui")
                    local hammerUI = pgui and pgui:FindFirstChild("Forge") and pgui.Forge:FindFirstChild("HammerMinigame")

                    if hammerActive and ScriptRunning and type(noteData) == "table" then
                        -- Mathematically enforce "Perfect" rating on timing calculation
                        if type(noteData.TimingIntervals) == "table" then
                            for _, interval in ipairs(noteData.TimingIntervals) do
                                if type(interval) == "table" and interval.Name == "Perfect" then
                                    interval.Interval = 999999
                                end
                            end
                        end

                        if hammerUI then
                            childConn = hammerUI.ChildAdded:Connect(function(ch)
                                if not capturedFrame and ch:IsA("GuiButton") and ch.Name == "Frame" and not ch:GetAttribute("AetherHooked") then
                                    ch:SetAttribute("AetherHooked", true)
                                    capturedFrame = ch
                                    if childConn then
                                        childConn:Disconnect()
                                        childConn = nil
                                    end
                                end
                            end)
                        end

                        local lifetime = tonumber(noteData.Lifetime)
                        if lifetime and lifetime > 0 then
                            local targetZIndex = noteData.ZIndex
                            local gradeRatio = 25 / 44
                            if not State.IsAutoForging then
                                if Config.HammerGrade == "Great" then
                                    gradeRatio = (25 / 44) + 0.055
                                elseif Config.HammerGrade == "Good" then
                                    gradeRatio = (25 / 44) + 0.078
                                end
                            end
                            -- Pre-lead by 1 frame (0.012s) so CreateNote's `while not u19 do task.wait() end` wakes up right at Lifetime * 25/44
                            local perfectDelay = math.max(0.01, (lifetime * gradeRatio) - 0.012)
                            local fireAtServer = workspace:GetServerTimeNow() + perfectDelay

                            task.delay(math.max(0, perfectDelay - 0.03), function()
                                if childConn then
                                    childConn:Disconnect()
                                    childConn = nil
                                end
                                while ScriptRunning and workspace:GetServerTimeNow() < fireAtServer do
                                    RunService.Heartbeat:Wait()
                                end
                                if not ScriptRunning or not (Config.AutoHammer or State.IsAutoForging) then return end
                                pcall(function()
                                    local ui = hammerUI
                                    if not ui or not ui.Parent then
                                        local pg = LocalPlayer:FindFirstChild("PlayerGui")
                                        ui = pg and pg:FindFirstChild("Forge") and pg.Forge:FindFirstChild("HammerMinigame")
                                    end
                                    if not ui or not ui.Visible then return end

                                    local btn = capturedFrame
                                    if not btn or not btn.Parent then
                                        for _, child in ipairs(ui:GetChildren()) do
                                            if child:IsA("GuiButton") and child.Name == "Frame"
                                                and not child:GetAttribute("AetherClicked")
                                                and (not targetZIndex or child.ZIndex == targetZIndex) then
                                                btn = child
                                                break
                                            end
                                        end
                                    end

                                    if btn and btn.Parent and not btn:GetAttribute("AetherClicked") then
                                        btn:SetAttribute("AetherClicked", true)
                                        if typeof(firesignal) == "function" then
                                            firesignal(btn.MouseButton1Click)
                                            pcall(function() firesignal(btn.Activated) end)
                                        else
                                            local vim = game:GetService("VirtualInputManager")
                                            local cx = btn.AbsolutePosition.X + (btn.AbsoluteSize.X * 0.5)
                                            local cy = btn.AbsolutePosition.Y + (btn.AbsoluteSize.Y * 0.5)
                                            pcall(function()
                                                vim:SendMouseButtonEvent(cx, cy, 0, true, game, 0)
                                                vim:SendMouseButtonEvent(cx, cy, 0, false, game, 0)
                                            end)
                                        end
                                    end
                                end)
                            end)
                        end
                    end
                    local res = oldCreateNote and oldCreateNote(self, noteData, anvilPart, markerAttachment)
                    if childConn then
                        childConn:Disconnect()
                        childConn = nil
                    end
                    return res
                end)
            end
        end)
    end

    -- 2. GolemMinigame Phase Hooks (Mould / Build / Awaken)
    if not OrigHooks["Golem_Mould"] then
        pcall(function()
            local golemMod = fcFolder:FindFirstChild("GolemMinigame")
            local GolemMG = golemMod and safeRequire(golemMod)
            if type(GolemMG) == "table" then
                if type(GolemMG.Mould) == "function" and not OrigHooks["Golem_Mould"] then
                    local oldMould
                    oldMould = safeHook(GolemMG, "Mould", "Golem_Mould", function(self, ...)
                        State.GolemPhase = "Mould"
                        local res = table.pack(pcall(oldMould, self, ...))
                        if State.GolemPhase == "Mould" then State.GolemPhase = nil end
                        if not res[1] then error(res[2]) end
                        return table.unpack(res, 2, res.n)
                    end)
                end
                if type(GolemMG.Build) == "function" and not OrigHooks["Golem_Build"] then
                    local oldBuild
                    oldBuild = safeHook(GolemMG, "Build", "Golem_Build", function(self, ...)
                        State.GolemPhase = "Build"
                        local res = table.pack(pcall(oldBuild, self, ...))
                        if State.GolemPhase == "Build" then State.GolemPhase = nil end
                        if not res[1] then error(res[2]) end
                        return table.unpack(res, 2, res.n)
                    end)
                end
                if type(GolemMG.Awaken) == "function" and not OrigHooks["Golem_Awaken"] then
                    local oldAwaken
                    oldAwaken = safeHook(GolemMG, "Awaken", "Golem_Awaken", function(self, ...)
                        State.GolemPhase = "Awaken"
                        local res = table.pack(pcall(oldAwaken, self, ...))
                        if State.GolemPhase == "Awaken" then State.GolemPhase = nil end
                        if not res[1] then error(res[2]) end
                        return table.unpack(res, 2, res.n)
                    end)
                end
            end
        end)
    end

    -- 3. Zero-VIM Internal State Driver for Real MeltMinigame & PourMinigame (RenderPriority.First)
    -- Runs 100% of the real MeltMinigame:Start and PourMinigame:Start functions (including Janitor task.wait(0.25) & exact return math)
    -- while driving their internal frame checks directly before RenderStepped so quality is 100% Masterwork without touching the mouse!
    if not ForgeEngine.RenderStepDriverBound then
        ForgeEngine.RenderStepDriverBound = true
        local meltStartFn = nil
        local meltBoolUpvalIdx = nil
        local savedLineSize = nil
        local savedLineImgSize = nil
        local savedAreaSize = nil

        local function resolveMeltUpvalue()
            if meltStartFn and meltBoolUpvalIdx then return end
            pcall(function()
                local meltMod = fcFolder:FindFirstChild("MeltMinigame")
                local MeltMG = meltMod and safeRequire(meltMod)
                if type(MeltMG) == "table" and type(MeltMG.Start) == "function" then
                    meltStartFn = (OrigHooks["AutoMelt_Start"] and OrigHooks["AutoMelt_Start"].orig) or MeltMG.Start
                    if typeof(debug.getupvalues) == "function" then
                        for idx, val in pairs(debug.getupvalues(meltStartFn)) do
                            if type(val) == "boolean" then
                                meltBoolUpvalIdx = idx
                                break
                            end
                        end
                    end
                end
            end)
        end
        resolveMeltUpvalue()

        pcall(function()
            RunService:UnbindFromRenderStep("Aether_ForgeMinigameDrive")
        end)
        RunService:BindToRenderStep("Aether_ForgeMinigameDrive", Enum.RenderPriority.First.Value, function()
            if not ScriptRunning then return end
            local pgui = LocalPlayer:FindFirstChild("PlayerGui")
            local forgeGui = pgui and pgui:FindFirstChild("Forge")
            if not forgeGui then return end

            -- A. MeltMinigame Internal Pump Driver (Zero Mouse Movement)
            -- In MeltMinigame.RenderStepped:
            --   local Scale = Top.Position.Y.Scale
            --   local math_clamp_ret = math.clamp(v28, 0, 1 - Bottom.Size.Y.Scale * 2)  -- always >= 0 regardless of Mouse.Y!
            --   if Scale < math_clamp_ret then u24 = u24 + (math_clamp_ret - Scale) end
            -- Setting u1 = true and Top.Position.Y.Scale = -1 at RenderPriority.First makes (math_clamp_ret - Scale) >= 1.0 >= 0.45
            -- on every frame without moving the mouse, and line 258 of MeltMinigame immediately resets Top.Position to u2.Position!
            local meltMG = forgeGui:FindFirstChild("MeltMinigame")
            if meltMG and meltMG.Visible and (Config.AutoMelt or State.IsAutoForging) then
                local heater = meltMG:FindFirstChild("Heater")
                local top = heater and heater:FindFirstChild("Top")
                if heater and top and heater.Position.X.Scale > 0.0 then
                    resolveMeltUpvalue()
                    if meltStartFn and meltBoolUpvalIdx and typeof(debug.setupvalue) == "function" then
                        pcall(debug.setupvalue, meltStartFn, meltBoolUpvalIdx, true)
                    elseif typeof(firesignal) == "function" then
                        pcall(function() firesignal(top.MouseButton1Down) end)
                    end
                    top.Position = UDim2.new(top.Position.X.Scale, top.Position.X.Offset, -1, 0)
                end
            end

            -- B. PourMinigame Internal Overlap Driver (Zero Mouse Clicks, 0.000s Out-of-Zone)
            -- In PourMinigame.RenderStepped:
            --   local Scale = Line.Position.Y.Scale   -- in [0, 1]
            --   local Scale2 = Area.Position.Y.Scale  -- in [0.05, 0.8]
            --   if Scale < Scale2 + Area.Size.Y.Scale and Scale2 < Scale + Line.Size.Y.Scale then u9 = u9 + p20
            -- Since Line has BackgroundTransparency = 1 (visual is child ImageLabel), expanding Line.Size.Y.Scale & Area.Size.Y.Scale
            -- to 1.5 guarantees 100% overlap on every single frame while real PourMinigame:Start runs its native Janitor & return math!
            local pourMG = forgeGui:FindFirstChild("PourMinigame")
            local frame = pourMG and pourMG:FindFirstChild("Frame")
            local area = frame and frame:FindFirstChild("Area")
            local line = frame and frame:FindFirstChild("Line")
            if pourMG and pourMG.Visible and (Config.AutoPour or State.IsAutoForging) and State.GolemPhase ~= "Mould" then
                if area and line then
                    if not savedAreaSize then savedAreaSize = area.Size end
                    if not savedLineSize then
                        savedLineSize = line.Size
                        local img = line:FindFirstChildOfClass("ImageLabel")
                        if img then savedLineImgSize = img.Size end
                    end
                    area.Size = UDim2.new(savedAreaSize.X.Scale, savedAreaSize.X.Offset, 1.5, 0)
                    line.Size = UDim2.new(savedLineSize.X.Scale, savedLineSize.X.Offset, 1.5, 0)
                    local img = line:FindFirstChildOfClass("ImageLabel")
                    if img and savedLineImgSize then
                        img.Size = UDim2.new(savedLineImgSize.X.Scale, savedLineImgSize.X.Offset, 0.0175 / 1.5, 0)
                    end
                end
            else
                if area and savedAreaSize then
                    area.Size = savedAreaSize
                    savedAreaSize = nil
                end
                if line and savedLineSize then
                    line.Size = savedLineSize
                    local img = line:FindFirstChildOfClass("ImageLabel")
                    if img and savedLineImgSize then
                        img.Size = savedLineImgSize
                    end
                    savedLineSize = nil
                    savedLineImgSize = nil
                end
            end
        end)
    end

    return (OrigHooks["AutoHammer_NoteHook"] ~= nil and ForgeEngine.RenderStepDriverBound == true)
end

task.spawn(function()
    pcall(function()
        local pg = LocalPlayer and (LocalPlayer:FindFirstChild("PlayerGui") or LocalPlayer:WaitForChild("PlayerGui", 10))
        if pg and not pg:FindFirstChild("Forge") then
            pcall(function() pg:WaitForChild("Forge", 12) end)
        end
    end)
    for _ = 1, 10 do
        if not ScriptRunning then return end
        if ForgeEngine.EnsureMinigameHooks() then break end
        task.wait(0.5)
    end

    -- Golem Build + Awaken UI child handler
    pcall(function()
        local vim = game:GetService("VirtualInputManager")
        local function clickGolemButton(btn)
            if not btn or not btn.Parent or btn:GetAttribute("AetherClicked") then return end
            btn:SetAttribute("AetherClicked", true)
            if typeof(firesignal) == "function" then
                pcall(function() firesignal(btn.MouseButton1Click) end)
                pcall(function() firesignal(btn.Activated) end)
            else
                local cx = btn.AbsolutePosition.X + (btn.AbsoluteSize.X * 0.5)
                local cy = btn.AbsolutePosition.Y + (btn.AbsoluteSize.Y * 0.5)
                pcall(function()
                    vim:SendMouseButtonEvent(cx, cy, 0, true, game, 0)
                    vim:SendMouseButtonEvent(cx, cy, 0, false, game, 0)
                end)
            end
        end

        local function handleGolemHammerChild(ch)
            if not ScriptRunning or not ch or not ch:IsA("GuiButton") or ch.Name ~= "Frame" then return end
            if ch:GetAttribute("AetherHooked") or ch:GetAttribute("AetherGolemHandled") then return end
            local t0 = os.clock()
            local inner = ch:FindFirstChild("Frame")
            local circle = inner and inner:FindFirstChild("Circle")
            local s0 = circle and circle.Size.X.Scale or 2.5

            task.defer(function()
                if not ScriptRunning or not ch.Parent then return end
                if ch:GetAttribute("AetherHooked") or ch:GetAttribute("AetherGolemHandled") then return end
                if not (Config.AutoHammer or State.IsAutoForging) then return end
                ch:SetAttribute("AetherGolemHandled", true)

                inner = inner or ch:FindFirstChild("Frame")
                circle = circle or (inner and inner:FindFirstChild("Circle"))
                if not circle then return end

                if circle.Visible and State.GolemPhase ~= "Awaken" then
                    -- Golem Build timingNote: Circle shrinks linearly from s0 -> 0 over Lifetime (1.15-1.45s); Perfect = 25/44 elapsed (19/44 remaining scale)
                    if s0 <= 0.05 then s0 = math.max(circle.Size.X.Scale, 2.5) end
                    local gradeRemain = 19 / 44 -- Perfect (0.4318)
                    if Config.HammerGrade == "Great" then
                        gradeRemain = 1 - (25 / 44 + 0.055)
                    elseif Config.HammerGrade == "Good" then
                        gradeRemain = 1 - (25 / 44 + 0.078)
                    end
                    local targetScale = s0 * gradeRemain
                    local targetElapsedRatio = 1 - gradeRemain
                    local fireAt = nil

                    local rsConn
                    rsConn = RunService.RenderStepped:Connect(function()
                        if not ScriptRunning or not ch.Parent or ch:GetAttribute("AetherClicked") or not (Config.AutoHammer or State.IsAutoForging) then
                            if rsConn then rsConn:Disconnect(); rsConn = nil end
                            return
                        end
                        local curScale = circle.Size.X.Scale
                        local now = os.clock()
                        local dt = now - t0
                        if not fireAt and dt > 0.02 and curScale < s0 * 0.97 then
                            local rate = (s0 - curScale) / dt
                            if rate > 0.1 then
                                local estLifetime = s0 / rate
                                fireAt = t0 + (estLifetime * targetElapsedRatio)
                            end
                        end
                        if curScale <= targetScale or (fireAt and now >= fireAt) or dt >= 1.45 then
                            if rsConn then rsConn:Disconnect(); rsConn = nil end
                            clickGolemButton(ch)
                        end
                    end)
                else
                    -- Golem Awaken heart prompt: listens to UserInputService.InputBegan (MouseButton1 / Space), requires 20 mashes (MashGain = 0.05)
                    task.spawn(function()
                        local deadline = os.clock() + 12.0
                        while ScriptRunning and ch.Parent and os.clock() < deadline and (Config.AutoHammer or State.IsAutoForging) do
                            local cx = ch.AbsolutePosition.X + (ch.AbsoluteSize.X * 0.5)
                            local cy = ch.AbsolutePosition.Y + (ch.AbsoluteSize.Y * 0.5)
                            local sent = pcall(function()
                                vim:SendMouseButtonEvent(cx, cy, 0, true, game, 0)
                                vim:SendMouseButtonEvent(cx, cy, 0, false, game, 0)
                            end)
                            if not sent then
                                pcall(function()
                                    vim:SendKeyEvent(true, Enum.KeyCode.Space, false, game)
                                    vim:SendKeyEvent(false, Enum.KeyCode.Space, false, game)
                                end)
                            end
                            task.wait(0.02)
                        end
                    end)
                end
            end)
        end

        local boundHammerUIs = {}
        local function bindHammerUI(hm)
            if not hm or boundHammerUIs[hm] then return end
            boundHammerUIs[hm] = true
            for _, ch in ipairs(hm:GetChildren()) do
                handleGolemHammerChild(ch)
            end
            trackConn(hm.ChildAdded:Connect(handleGolemHammerChild))
        end

        local pg = LocalPlayer:FindFirstChild("PlayerGui")
        local fg = pg and pg:FindFirstChild("Forge")
        local hm = fg and fg:FindFirstChild("HammerMinigame")
        if hm then
            bindHammerUI(hm)
        elseif pg then
            local watchConn
            watchConn = trackConn(pg.DescendantAdded:Connect(function(desc)
                if desc.Name == "HammerMinigame" and desc.Parent and desc.Parent.Name == "Forge" then
                    bindHammerUI(desc)
                    if watchConn then pcall(function() watchConn:Disconnect() end) end
                end
            end))
        end
    end)
end)

-- 8B. Minigame Controller Loop: Auto Mold Breaker & Showcase Accept (Melt/Pour handled via hooks in 8A-2)
task.spawn(function()
    while ScriptRunning do
        pcall(function()
            local pgui = LocalPlayer:FindFirstChild("PlayerGui")
            local forgeGui = pgui and pgui:FindFirstChild("Forge")

            -- 1. Auto Mold Breaker (Clicks the anvil mold 3x to advance to Hammer notes)
            if Config.AutoMoldBreak or State.IsAutoForging then
                local debris = workspace:FindFirstChild("Debris")
                if debris then
                    for _, ch in ipairs(debris:GetChildren()) do
                        if ch.Name == "Mold" and not ch:GetAttribute("IsDestroyed") then
                            local cd = ch:FindFirstChildWhichIsA("ClickDetector", true)
                            if cd and typeof(fireclickdetector) == "function" then
                                fireclickdetector(cd)
                                fireclickdetector(cd)
                                fireclickdetector(cd)
                            end
                        end
                    end
                end
            end

            if forgeGui then
                -- 2. Auto Claim / Accept Forged Gear (Showcase / EndScreen only when Forge is actually showcasing)
                local endScreen = forgeGui:FindFirstChild("EndScreen")
                local stats = endScreen and endScreen:FindFirstChild("Stats")
                local inShowcase = false
                if endScreen and endScreen.Visible then
                    inShowcase = true
                elseif stats and stats.Visible then
                    pcall(function()
                        local FC = SafeGetController("ForgeController")
                        if FC and FC.ForgeActive and FC.Sequence == "Showcase" then
                            inShowcase = true
                        end
                    end)
                end
                if inShowcase and (Config.AutoClaimForged or State.IsAutoForging) then
                    local now = os.clock()
                    if now - (State.LastEndScreenAccept or 0) >= 1.0 then
                        State.LastEndScreenAccept = now
                        local namePopup = endScreen and endScreen:FindFirstChild("NamePopup")
                        local namePopupOpen = namePopup and namePopup.Visible and namePopup.Size.X.Scale > 0.05
                        local targetBtn = nil
                        if namePopupOpen then
                            local btns = (namePopup:FindFirstChild("Frame") or namePopup):FindFirstChild("Buttons")
                            targetBtn = btns and btns:FindFirstChild("Yes")
                        else
                            targetBtn = stats and stats:FindFirstChild("Frame") and stats.Frame:FindFirstChild("AcceptButton")
                        end
                        if targetBtn then
                            runAsGameThread(function()
                                local InputAction = safeRequire(ReplicatedStorage.Shared.InputAction)
                                local actBtn = InputAction.Button(nil, targetBtn)
                                if actBtn and actBtn.Activated then
                                    actBtn.Activated:Fire()
                                end
                            end, 1.0)
                            if typeof(firesignal) == "function" then
                                pcall(function() firesignal(targetBtn.Activated) end)
                                pcall(function() firesignal(targetBtn.MouseButton1Click) end)
                            end
                        end
                    end
                end
            end
        end)
        task.wait(0.02)
    end
end)

-- 8C. Dedicated Non-Blocking RenderStepped Pour Balancer (Golem Mould only; Armor/Weapon Pour handled via hook in 8A-2)
do
    local vim = game:GetService("VirtualInputManager")
    local lastMouldAreaY = nil
    local mouldWaitUntil = 0
    local function setPourHold(down)
        local cam = workspace.CurrentCamera
        local vp = cam and cam.ViewportSize or Vector2.new(1280, 720)
        local clickX = math.floor(vp.X * 0.5)
        local clickY = math.floor(vp.Y * 0.88)
        local ok = pcall(function()
            vim:SendMouseButtonEvent(clickX, clickY, 0, down, game, 0)
        end)
        if not ok then
            if down and typeof(mouse1press) == "function" then
                pcall(mouse1press)
            elseif (not down) and typeof(mouse1release) == "function" then
                pcall(mouse1release)
            end
        end
    end
    trackConn(RunService.RenderStepped:Connect(function()
        if not ScriptRunning then return end
        local pgui = LocalPlayer:FindFirstChild("PlayerGui")
        local forgeGui = pgui and pgui:FindFirstChild("Forge")
        local pourMG = forgeGui and forgeGui:FindFirstChild("PourMinigame")
        if pourMG and pourMG.Visible and (Config.AutoPour or State.IsAutoForging) then
            local frame = pourMG:FindFirstChild("Frame")
            local area = frame and frame:FindFirstChild("Area")
            local line = frame and frame:FindFirstChild("Line")
            -- Ignore the 0.25s entry/exit slide tween so AutoPour never holds/clicks into Hammer/Showcase
            local frameOnScreen = frame and frame.Position.X.Scale > 0.50 and frame.Position.X.Scale < 0.96
            if frame and area and line and frameOnScreen then
                -- PourMinigame.Inside.Visible stays false during normal PourMinigame, so ONLY treat State.GolemPhase == "Mould" as Golem Mould!
                if State.GolemPhase ~= "Mould" then
                    if State.PourMouseDown then
                        State.PourMouseDown = false
                        setPourHold(false)
                    end
                    return
                end

                local areaCenter = area.AbsolutePosition.Y + (area.AbsoluteSize.Y * 0.5)
                -- Golem Mould stops on MouseButton1 release across 3 fixed checkpoints; Line.AnchorPoint.Y == 0 matches 1 - fill
                local lineTop = line.AbsolutePosition.Y
                if lastMouldAreaY and math.abs(areaCenter - lastMouldAreaY) > 4 then
                    mouldWaitUntil = 0
                    lastMouldAreaY = areaCenter
                end
                if State.PourMouseDown then
                    if lineTop <= areaCenter + 0.5 then
                        State.PourMouseDown = false
                        setPourHold(false)
                        lastMouldAreaY = areaCenter
                        mouldWaitUntil = os.clock() + 0.08
                    end
                else
                    if os.clock() >= mouldWaitUntil and lineTop > areaCenter + 1.5 and frame.Position.X.Scale < 0.98 then
                        lastMouldAreaY = areaCenter
                        State.PourMouseDown = true
                        setPourHold(true)
                    end
                end
            end
        else
            lastMouldAreaY = nil
            mouldWaitUntil = 0
            if State.PourMouseDown then
                State.PourMouseDown = false
                setPourHold(false)
            end
        end
    end))
end

-- ===================================================================
-- 9. CRAFTING & ECONOMY AUTOMATION (Priority: Auto Mine/Combat > Auto Craft > Auto Forge)
-- ===================================================================
local CraftEngine = {
    Recipes = {},
    RecipeNames = {},
    CatalogLoaded = false,
    StatusLabel = nil,
    SlotInfoLabel = nil,
}
State.CraftEngine = CraftEngine

function CraftEngine.LoadCatalog(forceReload)
    if CraftEngine.CatalogLoaded and not forceReload and #CraftEngine.RecipeNames > 0 then
        return true
    end
    local loaded = false
    runAsGameThread(function()
        local CraftCatalog = safeRequire(ReplicatedStorage.Shared.CraftCatalog)
        local OreCrafting = safeRequire(ReplicatedStorage.Shared.Data.OreCrafting)
        if not CraftCatalog then return end

        local newRecipes = {}
        local newNames = {}
        local seen = {}
        for _, g in ipairs(CraftCatalog.Groups()) do
            local lockText = nil
            local pd = State.PlayerData and State.PlayerData.Data
            if pd and type(CraftCatalog.Lock) == "function" then
                lockText = CraftCatalog.Lock(g, pd, g.Station)
            end
            for _, item in ipairs(CraftCatalog.Sorted(g)) do
                if item and item.Name and item.Recipe and not seen[item.Name] then
                    seen[item.Name] = true
                    newRecipes[item.Name] = {
                        name = item.Name,
                        station = g.Station or (OreCrafting and OreCrafting.AnywhereStation) or "Anywhere",
                        groupId = g.Id,
                        groupName = g.Name,
                        recipe = item.Recipe,
                        craftTime = tonumber(item.CraftTime) or 5,
                        rarity = item.Rarity or "Common",
                        lockText = lockText,
                    }
                    table.insert(newNames, item.Name)
                end
            end
        end
        if #newNames > 0 then
            CraftEngine.Recipes = newRecipes
            CraftEngine.RecipeNames = newNames
            CraftEngine.ObtainableSlots = (OreCrafting and OreCrafting.ObtainableSlots) or 4
            CraftEngine.CatalogLoaded = true
            loaded = true
        end
    end, 4.0)
    return loaded
end
pcall(CraftEngine.LoadCatalog, false)

function CraftEngine.HasCraftAnywhere()
    ResolvePlayerData()
    local gp = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Gamepasses
    return gp and gp.CraftAnywhere == true or false
end

function CraftEngine.GetSelectedRecipes()
    local list = {}
    local seen = {}
    local sel = Config.CraftTargets
    if type(sel) == "table" then
        for k, v in pairs(sel) do
            local name = (type(k) == "number" and type(v) == "string" and v) or (type(k) == "string" and v == true and k) or nil
            if name and name ~= "None" and not seen[name] then
                seen[name] = true
                table.insert(list, name)
            end
        end
    elseif type(sel) == "string" and sel ~= "" and sel ~= "None" then
        seen[sel] = true
        table.insert(list, sel)
    end
    if #list == 0 and type(Config.CraftTarget) == "string" and Config.CraftTarget ~= "" and Config.CraftTarget ~= "None" then
        table.insert(list, Config.CraftTarget)
    end
    return list
end

function CraftEngine.GetItemCount(itemName, pdData)
    if not pdData then return 0 end
    if itemName == "Gold" then
        return tonumber(pdData.Gold) or 0
    end
    local inv = pdData.Inventory
    if type(inv) ~= "table" then return 0 end
    local total = (type(inv[itemName]) == "number") and math.floor(inv[itemName]) or 0
    if type(inv.Misc) == "table" then
        for _, entry in pairs(inv.Misc) do
            if type(entry) == "table" and (entry.Name == itemName or entry.Id == itemName) then
                total = total + math.max(1, math.floor(tonumber(entry.Quantity) or 1))
            end
        end
    end
    return total
end

function CraftEngine.GetSlotSummary()
    ResolvePlayerData()
    local pdData = State.PlayerData and State.PlayerData.Data
    local slots = pdData and pdData.OreCraftSlots or {}
    local maxSlots = CraftEngine.ObtainableSlots or 4
    local unlocked, openSlots, busySlots, readySlots = 0, 0, 0, 0
    local soonestRem = math.huge
    local parts = {}
    local nowUnix = os.time()
    for i = 1, math.max(#slots, maxSlots) do
        local s = slots[i]
        if s and s.Locked ~= true then
            unlocked = unlocked + 1
            if s.Ore == nil then
                openSlots = openSlots + 1
                table.insert(parts, string.format("S%d: Empty", i))
            else
                busySlots = busySlots + 1
                local rData = CraftEngine.Recipes[s.Ore]
                local perTime = rData and rData.craftTime or 0
                local amt = tonumber(s.Amount) or 1
                local rem = (perTime > 0 and s.StartTime) and math.max(0, math.ceil((perTime * amt) - (nowUnix - s.StartTime))) or 0
                if rem > 0 then
                    if rem < soonestRem then soonestRem = rem end
                    table.insert(parts, string.format("S%d: %s x%d (%ds)", i, tostring(s.Ore), amt, rem))
                else
                    readySlots = readySlots + 1
                    table.insert(parts, string.format("S%d: %s x%d (Ready!)", i, tostring(s.Ore), amt))
                end
            end
        end
    end
    return unlocked, openSlots, busySlots, table.concat(parts, " | "), readySlots, soonestRem
end

function CraftEngine.ScheduleCompletionClaim()
    if not ScriptRunning or not (Config.AutoClaimSmelt or Config.AutoCraft or State.IsAutoForging) then
        return
    end
    local _, _, _, _, readySlots, soonestRem = CraftEngine.GetSlotSummary()
    if readySlots > 0 then
        local now = os.clock()
        if (now - (State.LastCraftCheck or 0)) >= 2.5 and Remotes.CheckCraftCompletions then
            State.LastCraftCheck = now
            task.spawn(function()
                pcall(function() Remotes.CheckCraftCompletions:InvokeServer() end)
                task.wait(0.15)
                pcall(CraftEngine.UpdateStatus)
                pcall(CraftEngine.ScheduleCompletionClaim)
            end)
        end
        return
    end
    if soonestRem < math.huge then
        CraftEngine.ClaimEpoch = (CraftEngine.ClaimEpoch or 0) + 1
        local myEpoch = CraftEngine.ClaimEpoch
        task.delay(soonestRem + 0.25, function()
            if not ScriptRunning or CraftEngine.ClaimEpoch ~= myEpoch then return end
            if not (Config.AutoClaimSmelt or Config.AutoCraft or State.IsAutoForging) then return end
            State.LastCraftCheck = os.clock()
            if Remotes.CheckCraftCompletions then
                pcall(function() Remotes.CheckCraftCompletions:InvokeServer() end)
            end
            task.wait(0.15)
            pcall(CraftEngine.UpdateStatus)
            pcall(CraftEngine.ScheduleCompletionClaim)
        end)
    end
end

function CraftEngine.FindCraftStation(preferredStation)
    local prox = workspace:FindFirstChild("Proximity")
    local stationCandidates = {}
    local galaxyFallback = {}
    if prox then
        if preferredStation and preferredStation ~= "" then
            local exact = prox:FindFirstChild(preferredStation)
            if exact then
                local pos = exact:IsA("BasePart") and exact.Position or exact:GetPivot().Position
                return exact, exact.Name, pos
            end
        end
        for _, ch in ipairs(prox:GetChildren()) do
            local nl = ch.Name:lower()
            if nl:find("craft station", 1, true) then
                table.insert(stationCandidates, ch)
            elseif ch.Name == "Galaxy Guardian" then
                table.insert(galaxyFallback, ch)
            end
        end
    end
    local candidates = (#stationCandidates > 0) and stationCandidates or galaxyFallback
    if #candidates > 0 then
        local chosen = candidates[1]
        if State.HRP then
            local bestD = math.huge
            for _, c in ipairs(candidates) do
                local p = c:IsA("BasePart") and c.Position or c:GetPivot().Position
                local d = (p - State.HRP.Position).Magnitude
                if d < bestD then
                    bestD = d
                    chosen = c
                end
            end
        end
        local pos = chosen:IsA("BasePart") and chosen.Position or chosen:GetPivot().Position
        return chosen, chosen.Name, pos
    end
    return nil, nil, nil
end

function CraftEngine.GetCraftQueue()
    CraftEngine.LoadCatalog(false)
    ResolvePlayerData()
    local pdData = State.PlayerData and State.PlayerData.Data
    if not pdData then return {} end

    local _, openSlots = CraftEngine.GetSlotSummary()
    if openSlots <= 0 then return {} end

    local selected = CraftEngine.GetSelectedRecipes()
    if #selected == 0 then return {} end

    local _, localStationName = CraftEngine.FindCraftStation(nil)
    local hasAnywhere = CraftEngine.HasCraftAnywhere()
    if not hasAnywhere and not localStationName then
        return {}
    end

    local vGold = tonumber(pdData.Gold) or 0
    local vItems = {}
    local function getVirtualCount(mat)
        if mat == "Gold" then return vGold end
        if vItems[mat] == nil then
            vItems[mat] = CraftEngine.GetItemCount(mat, pdData)
        end
        return vItems[mat]
    end

    local CraftCatalog = nil
    runAsGameThread(function()
        CraftCatalog = safeRequire(ReplicatedStorage.Shared.CraftCatalog)
    end, 1.5)

    local wantPerSlot = math.clamp(math.floor(tonumber(Config.CraftBatchAmount) or 1), 1, 100)
    local queue = {}

    -- Fill up to openSlots (round-robin through selected recipes so 2+ unlocked slots stay productive)
    local madeProgress = true
    while #queue < openSlots and madeProgress do
        madeProgress = false
        for _, rName in ipairs(selected) do
            if #queue >= openSlots then break end
            local rData = CraftEngine.Recipes[rName]
            if rData and type(rData.recipe) == "table" then
                local isLocked = false
                if CraftCatalog and type(CraftCatalog.Lock) == "function" and type(CraftCatalog.GroupOf) == "function" then
                    local stCheck = hasAnywhere and rData.station or localStationName
                    local grp = CraftCatalog.GroupOf(rName)
                    if grp and CraftCatalog.Lock(grp, pdData, stCheck) ~= nil then
                        isLocked = true
                    end
                end
                if not isLocked then
                    local maxAfford = math.huge
                    for mat, reqQty in pairs(rData.recipe) do
                        if reqQty > 0 then
                            local avail = getVirtualCount(mat)
                            maxAfford = math.min(maxAfford, math.floor(avail / reqQty))
                        end
                    end
                    local batchAmt = (maxAfford < math.huge) and math.min(wantPerSlot, maxAfford) or 0
                    if batchAmt >= 1 then
                        for mat, reqQty in pairs(rData.recipe) do
                            if reqQty > 0 then
                                if mat == "Gold" then
                                    vGold = vGold - (reqQty * batchAmt)
                                else
                                    vItems[mat] = getVirtualCount(mat) - (reqQty * batchAmt)
                                end
                            end
                        end
                        table.insert(queue, { name = rName, amount = batchAmt, station = rData.station })
                        madeProgress = true
                    end
                end
            end
        end
    end

    return queue
end

function CraftEngine.UpdateStatus(customTitle, customDesc)
    local unlocked, openSlots, busySlots, slotText = CraftEngine.GetSlotSummary()
    local selected = CraftEngine.GetSelectedRecipes()
    if CraftEngine.SlotInfoLabel then
        pcall(function()
            CraftEngine.SlotInfoLabel:SetTitle(string.format("Craft Slots: %d/%d Busy (%d Open)", busySlots, unlocked, openSlots))
            CraftEngine.SlotInfoLabel:SetDesc(slotText ~= "" and slotText or "No unlocked Craft slots detected")
        end)
    end
    if CraftEngine.StatusLabel then
        pcall(function()
            local title = customTitle or (Config.AutoCraft
                and (#selected == 0 and "Auto-Craft: Pick Recipes Below" or string.format("Auto-Craft: ON (%d Recipe%s)", #selected, #selected == 1 and "" or "s"))
                or "Auto-Craft: OFF")
            local desc = customDesc or "Priority: Auto Mine / Combat > Auto Craft > Auto Forge"
            CraftEngine.StatusLabel:SetTitle(title)
            CraftEngine.StatusLabel:SetDesc(desc)
        end)
    end
end

function CraftEngine.RunAutoCraftCycle(forceCheck)
    if not ScriptRunning or State.IsCrafting or State.IsAutoForging then return 0 end
    if not forceCheck and RaidEngine and RaidEngine.IsInActiveRun() then return 0 end
    if not State.HRP or not State.Humanoid or State.Humanoid.Health <= 0 then return 0 end

    if Remotes.CheckCraftCompletions then
        pcall(function() Remotes.CheckCraftCompletions:InvokeServer() end)
        task.wait(0.12)
    end

    local queue = CraftEngine.GetCraftQueue()
    if #queue == 0 then
        CraftEngine.UpdateStatus()
        return 0
    end

    State.IsCrafting = true
    CombatEngine.ClearStanceLock()
    local craftedCount = 0
    local preTripCF = (not Config.StayAtSavedPos and (Config.AutoMine or Config.AutoMobFarm) and State.HRP) and State.HRP.CFrame or nil
    local travelledToStation = false

    pcall(function()
        local hasAnywhere = CraftEngine.HasCraftAnywhere()
        local stationName = "Anywhere"
        if not hasAnywhere then
            local stnObj, stnName, stnPos = CraftEngine.FindCraftStation(queue[1].station)
            if not stnObj or not stnPos then
                CraftEngine.UpdateStatus("Auto-Craft: No Craft Station", "No Craft Station found in this world (use CraftAnywhere)")
                return
            end
            stationName = stnName
            if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
            if State.Humanoid then State.Humanoid.AutoRotate = true end
            CraftEngine.UpdateStatus("Auto-Craft: Moving to " .. tostring(stnName), "Priority: Auto Mine/Combat > Auto Craft > Auto Forge")
            if (State.HRP.Position - stnPos).Magnitude > 18 then
                travelledToStation = true
                NavEngine.NavigateTo(CFrame.new(stnPos + Vector3.new(0, 2.0, 4.0), stnPos), true)
            end
            if not ScriptRunning or (not Config.AutoCraft and not forceCheck) then
                return
            end
            if not State.HRP or (State.HRP.Position - stnPos).Magnitude > 23 then
                return
            end
            -- Broadphase nudge so server MaxStationDistance validation always registers
            State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
            task.wait(0.08)
        end

        if Remotes.StartCrafting then
            pcall(function() Remotes.StartCrafting:InvokeServer(stationName) end)
            task.wait(0.08)
        end
        for _, job in ipairs(queue) do
            if not ScriptRunning or (not Config.AutoCraft and not forceCheck) then break end
            if Remotes.CraftRequest then
                local okReq = pcall(function() return Remotes.CraftRequest:InvokeServer(job.name, job.amount) end)
                if okReq then
                    craftedCount = craftedCount + 1
                    CraftEngine.UpdateStatus("Auto-Crafted: " .. tostring(job.name) .. " x" .. tostring(job.amount), "Queued into open Craft slot")
                end
                task.wait(0.15)
            end
        end

        -- Clean up any temporary NoMovement tag if Craft UI wasn't opened manually
        local st = State.Character and State.Character:FindFirstChild("Status")
        local UIC = nil
        runAsGameThread(function()
            local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
            UIC = SafeGetController("UIController", KnitPkg)
        end, 1.0)
        local oreCraftOpen = UIC and UIC.Modules and UIC.Modules.OreCraft and UIC.Modules.OreCraft.IsOpen
        if st and not oreCraftOpen then
            for _, ch in ipairs(st:GetChildren()) do
                if ch.Name == "NoMovement" then
                    pcall(function() ch:Destroy() end)
                end
            end
        end
        if Remotes.CheckCraftCompletions then
            pcall(function() Remotes.CheckCraftCompletions:InvokeServer() end)
        end
        if travelledToStation and not State.DrainingFullBag and (Config.AutoMine or Config.AutoMobFarm) then
            if Config.AutoMine and Config.StayAtSavedPos and MiningEngine then
                local savedCF = MiningEngine.GetSavedSpotCFrame()
                if savedCF and State.HRP and (State.HRP.Position - savedCF.Position).Magnitude > 4 then
                    NavEngine.GlideTo(savedCF, Config.GlideSpeed or 55, true)
                end
            elseif preTripCF and State.HRP and (State.HRP.Position - preTripCF.Position).Magnitude > 14 then
                NavEngine.GlideTo(preTripCF, Config.GlideSpeed or 55, true)
            end
        end
    end)

    State.IsCrafting = false
    CraftEngine.UpdateStatus()
    pcall(CraftEngine.ScheduleCompletionClaim)
    return craftedCount
end

-- Event-driven Craft completion scheduling (zero wasted polling; claims the exact second a slot timer hits 0)
do
    if Remotes.OreCraftEvent and Remotes.OreCraftEvent:IsA("RemoteEvent") then
        trackConn(Remotes.OreCraftEvent.OnClientEvent:Connect(function()
            if not ScriptRunning then return end
            task.delay(0.15, function()
                pcall(CraftEngine.UpdateStatus)
                pcall(CraftEngine.ScheduleCompletionClaim)
            end)
        end))
    end
    runAsGameThread(function()
        local pd = State.PlayerData or ResolvePlayerData()
        if pd and type(pd.OnWrite) == "function" then
            for _, writeKey in ipairs({ "AddOreCraft", "UnlockOreCraft", "SkipOreCraftTime", "CancelOreCraft" }) do
                pcall(function()
                    pd:OnWrite(writeKey, function()
                        if not ScriptRunning then return end
                        task.delay(0.12, function()
                            pcall(CraftEngine.UpdateStatus)
                            pcall(CraftEngine.ScheduleCompletionClaim)
                        end)
                    end)
                end)
            end
        end
    end, 2.0)
    pcall(CraftEngine.ScheduleCompletionClaim)
end

task.spawn(function()
    while ScriptRunning do
        local now = os.clock()

        -- Local UI timer refresh + automatic claim ONLY when a slot's StartTime + CraftTime has actually elapsed
        local _, _, _, _, readySlots = CraftEngine.GetSlotSummary()
        pcall(CraftEngine.UpdateStatus)
        if (Config.AutoClaimSmelt or Config.AutoCraft or State.IsAutoForging) and readySlots > 0 and (now - (State.LastCraftCheck or 0) >= 2.5) then
            State.LastCraftCheck = now
            if Remotes.CheckCraftCompletions then
                pcall(function() Remotes.CheckCraftCompletions:InvokeServer() end)
            end
            task.delay(0.15, function()
                pcall(CraftEngine.UpdateStatus)
                pcall(CraftEngine.ScheduleCompletionClaim)
            end)
        end

        -- Background Auto Craft scheduling: Priority is Auto Mine / Auto Combat > Auto Craft > Auto Forge
        if Config.AutoCraft and not State.IsAutoForging and not State.DrainingFullBag and not State.IsCrafting and not State.IsBuyingPotions and (now - (State.LastCraftRequest or 0) >= 4.0) then
            local inRaidRun = RaidEngine and RaidEngine.IsInActiveRun()
            if not inRaidRun then
                local farmActive = (Config.AutoMine == true) or (Config.AutoMobFarm == true) or (Config.AutoRaid == true)
                local hasAnywhere = CraftEngine.HasCraftAnywhere()
                local isFullNoForge = (not Config.AutoForgeArmor) and select(3, ForgeEngine.GetStashCapacityStatus())
                local allowCraftNow = hasAnywhere or (not farmActive) or (Config.AutoCraftInterruptFarm == true) or isFullNoForge
                if allowCraftNow then
                    State.LastCraftRequest = now
                    local q = CraftEngine.GetCraftQueue()
                    if #q > 0 then
                        CraftEngine.RunAutoCraftCycle(false)
                    end
                end
            end
        end

        -- Auto-Sell Selected Whitelisted Ores (NEVER sells non-picked ores!)
        local inRaidForStash = (RaidEngine and RaidEngine.IsInActiveRun()) or State.IsInRaid
        if Config.AutoSellStash and (not inRaidForStash or ForgeEngine.HasSellAnywherePass()) and (now - State.LastStashSell >= Config.SellStashInterval) then
            State.LastStashSell = now
            local okSell, soldUnits, soldTypes = pcall(ForgeEngine.SellSelectedOres)
            if okSell and type(soldUnits) == "number" and soldUnits > 0 and ForgeEngine.StashSellStatusLabel then
                ForgeEngine.StashSellStatusLabel:SetTitle(string.format("Auto-Sold %d Ore(s) (%d Types)", soldUnits, soldTypes))
                ForgeEngine.StashSellStatusLabel:SetDesc("Non-selected ores safely preserved")
            end
        end

        -- Auto-Claim Daily & Mail
        if Config.AutoClaimDaily and (now - State.LastClaimDaily >= 120.0) then
            State.LastClaimDaily = now
            if Remotes.ClaimDaily then pcall(function() Remotes.ClaimDaily:InvokeServer() end) end
            if Remotes.ClaimMailAll then pcall(function() Remotes.ClaimMailAll:InvokeServer() end) end
        end

        -- Auto-Equip Best Armor (Helmet, Chestplate, Leggings by CalculateHealth)
        if Config.AutoEquipBestArmor and not State.IsAutoForging then
            pcall(ForgeEngine.EquipBestArmor)
        end

        task.wait(1.0)
    end
end)

-- ===================================================================
-- 10. COMBAT & MOB FARM ENGINE
-- ===================================================================
function CombatEngine.GetMobStrengthScore(mob)
    if not mob then return 0 end
    if not CombatEngine.MobStrengthCache then
        local cache = {}
        local okE, EnemiesData = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Enemies) end)
        if okE and type(EnemiesData) == "table" then
            for k, entry in pairs(EnemiesData) do
                if type(entry) == "table" then
                    local baseHp = tonumber(entry.BaseHealth) or 0
                    local scale = tonumber(entry.HealthScalingPerLevel) or 0
                    local maxLvl = tonumber(entry.MaxLevel) or 1
                    local dmg = tonumber(entry.Damage) or tonumber(entry.MaxDamage) or tonumber(entry.MinDamage) or 0
                    local layout = tonumber(entry.LayoutOrder) or 0
                    local score = baseHp + (scale * maxLvl)
                    if score <= 0 then
                        score = dmg * 10 + layout
                    end
                    local keyName = tostring(k)
                    local dispName = tostring(entry.Name or k)
                    cache[keyName] = score
                    cache[dispName] = score
                end
            end
        end
        CombatEngine.MobStrengthCache = cache
    end

    local species = tostring(mob.Name):gsub("%d+$", ""):gsub("^%s+", ""):gsub("%s+$", "")
    local cached = CombatEngine.MobStrengthCache[species]
    if cached and cached > 0 then
        return cached
    end
    local hum = mob:FindFirstChildOfClass("Humanoid")
    return (hum and tonumber(hum.MaxHealth)) or 0
end

function CombatEngine.FindClosestMob()
    local living = workspace:FindFirstChild("Living")
    if not living or not State.HRP then return nil end

    local hrpPos = State.HRP.Position
    local maxScan = Config.MobFarmMaxDistance or 5000
    local now = os.clock()
    local misc = MiningEngine and MiningEngine.GetMiscDialogues and MiningEngine.GetMiscDialogues() or nil
    local useStrength = (Config.PrioritizeStrongestMob ~= false)
    local bestNormalMob, bestNormalScore, bestNormalDist = nil, -1, math.huge
    local bestShieldedMob, bestShieldedScore, bestShieldedDist = nil, -1, math.huge

    for _, mob in ipairs(living:GetChildren()) do
        if CombatEngine.IsHostileMob(mob) and CombatEngine.MatchesTargetMob(mob) then
            local blUntil = CombatEngine.Blacklist[mob]
            if not blUntil or now >= blUntil then
                local mHRP = mob:FindFirstChild("HumanoidRootPart")
                if mHRP then
                    local dist = (mHRP.Position - hrpPos).Magnitude
                    if dist <= maxScan and (not MiningEngine or MiningEngine.IsPositionZoneUnlocked(mHRP.Position, misc)) then
                        local score = useStrength and CombatEngine.GetMobStrengthScore(mob) or 0
                        if CombatEngine.IsBossShielded(mob) then
                            if score > bestShieldedScore or (score == bestShieldedScore and dist < bestShieldedDist) then
                                bestShieldedScore = score
                                bestShieldedDist = dist
                                bestShieldedMob = mob
                            end
                        elseif score > bestNormalScore or (score == bestNormalScore and dist < bestNormalDist) then
                            bestNormalScore = score
                            bestNormalDist = dist
                            bestNormalMob = mob
                        end
                    end
                end
            end
        end
    end
    return bestNormalMob or bestShieldedMob
end

-- Shared weapon swing helper used by EngageTargetMob & continuous autoclicker (swings unconditionally regardless of distance)
function CombatEngine.TrySwingWeapon(mHRP)
    if not State.HRP then return false end
    local heldTool = State.Character and State.Character:FindFirstChildWhichIsA("Tool")
    local hasWeapon = heldTool and heldTool.Name ~= "Pickaxe" and heldTool.Name ~= "PortalTool"
    local nowAtk = os.clock()
    local inFlight = CombatEngine.InFlightAttacks or 0
    if hasWeapon and (nowAtk - (CombatEngine.LastMobAttack or 0)) >= 0.06 and inFlight < 3 and not CombatEngine.IsBlockingOrParrying() and Remotes.ToolActivated then
        CombatEngine.LastMobAttack = nowAtk
        CombatEngine.InFlightAttacks = inFlight + 1
        task.spawn(function()
            pcall(function()
                Remotes.ToolActivated:InvokeServer("Weapon", false)
            end)
            CombatEngine.InFlightAttacks = math.max(0, (CombatEngine.InFlightAttacks or 1) - 1)
        end)
    end
    return hasWeapon
end

-- Continuous weapon autoclicker loop for Auto Mob Farm & Auto Raid (keeps swinging even while actively traveling to enemies)
task.spawn(function()
    while ScriptRunning do
        local raidActive = Config.AutoRaid and (State.IsInRaid or (game.PlaceId == 84980181568016) or workspace:GetAttribute("AbyssRun") == true or workspace:FindFirstChild("RaidLevels") ~= nil)
        local recoveringHp = _G.__TheForge_RaidEngine and _G.__TheForge_RaidEngine.RecoveringHP
        if (Config.AutoMobFarm or raidActive)
            and (Config.AutoAttackMob ~= false)
            and not recoveringHp
            and not State.IsAutoForging
            and not State.DrainingFullBag
            and not State.IsCrafting
            and not State.IsBuyingPotions
            and not State.SellingMobLoot
            and State.HRP and State.Humanoid and State.Humanoid.Health > 0 then
            local hasTarget = (State.CurrentMob ~= nil) or (State.TargetMob ~= nil) or raidActive
            if hasTarget then
                CombatEngine.EnsureWeaponEquipped()
                CombatEngine.TrySwingWeapon()
            end
        end
        task.wait(0.08)
    end
end)

-- Unanchored physics-replicated hover stance (LinearVelocity + PlatformStand, NEVER HRP.Anchored) so server tracks CFrame live, mobs detect & hitbox hits register reliably
function CombatEngine.SetHoverStance(enabled)
    local hrp = State.HRP
    local hum = State.Humanoid
    if not enabled then
        CombatEngine.HoverLockActive = false
        if hrp then
            local hv = hrp:FindFirstChild("AetherHoverLV")
            if hv then pcall(function() hv:Destroy() end) end
            local ha = hrp:FindFirstChild("AetherHoverAtt")
            if ha then pcall(function() ha:Destroy() end) end
        end
        if not NavEngine.IsGliding then
            NavEngine.RestoreCharacterPhysics()
        end
        return
    end

    if not hrp or not hum then return end
    if hrp.Anchored then
        hrp.Anchored = false
    end
    if hum.AutoRotate then
        hum.AutoRotate = false
    end
    if not hum.PlatformStand then
        pcall(function() hum.PlatformStand = true end)
    end

    if not CombatEngine.HoverLockActive and State.Character and not NavEngine.IsGliding and not Config.Noclip then
        if not NavEngine.SavedCollide then
            local saved = {}
            local hadSolidCore = false
            for _, part in ipairs(State.Character:GetDescendants()) do
                if part:IsA("BasePart") then
                    saved[part] = part.CanCollide
                    if CORE_COLLIDE_PARTS[part.Name] and part.CanCollide then
                        hadSolidCore = true
                    end
                end
            end
            if not hadSolidCore then
                for _, part in ipairs(State.Character:GetDescendants()) do
                    if part:IsA("BasePart") and CORE_COLLIDE_PARTS[part.Name] then
                        saved[part] = true
                    end
                end
            end
            NavEngine.SavedCollide = saved
        end
        for _, part in ipairs(State.Character:GetDescendants()) do
            if part:IsA("BasePart") and part.CanCollide then
                pcall(function() part.CanCollide = false end)
            end
        end
    end

    local att = hrp:FindFirstChild("AetherHoverAtt")
    if not att or not att:IsA("Attachment") then
        att = Instance.new("Attachment")
        att.Name = "AetherHoverAtt"
        att.Parent = hrp
    end
    local lv = hrp:FindFirstChild("AetherHoverLV")
    if not lv or not lv:IsA("LinearVelocity") then
        lv = Instance.new("LinearVelocity")
        lv.Name = "AetherHoverLV"
        lv.Attachment0 = att
        lv.MaxForce = math.huge
        lv.VectorVelocity = Vector3.zero
        lv.RelativeTo = Enum.ActuatorRelativeTo.World
        lv.Parent = hrp
    end
    CombatEngine.HoverLockActive = true
end

function CombatEngine.ClearStanceLock()
    CombatEngine.ActiveStanceCF = nil
    CombatEngine.ActiveStanceMob = nil
    CombatEngine.SetHoverStance(false)
end

-- Continuous per-frame unanchored hover lock & live target tracking so CharacterController never bounces HRP, kiting mobs cannot outrun swings, and client->server CFrame replication stays 100% live
do
    local function applyStanceLock(isHeartbeat)
        if not ScriptRunning then return end
        local cf = CombatEngine.ActiveStanceCF
        if not cf or not State.HRP then return end
        if Config.MobFarmPositionMode == "Behind Enemy" or (not Config.AutoMobFarm and not Config.AutoRaid) then
            CombatEngine.ClearStanceLock()
            return
        end
        if State.IsBuyingPotions or State.IsAutoForging or State.DrainingFullBag or State.IsCrafting or State.SellingMobLoot then
            CombatEngine.ClearStanceLock()
            return
        end
        if os.clock() < (CombatEngine.DodgeUntil or 0) then
            return
        end
        -- Never rubberband a server teleport (e.g. Wave 2 / Floor transition) back to stale Wave 1 coordinates
        if (State.HRP.Position - cf.Position).Magnitude > 65 then
            CombatEngine.ClearStanceLock()
            return
        end
        CombatEngine.SetHoverStance(true)
        if isHeartbeat and not CombatEngine.ActiveSkillName then
            local mob = CombatEngine.ActiveStanceMob
            local mHum = mob and mob.Parent and mob:FindFirstChildOfClass("Humanoid")
            local mHRP = mob and mob.Parent and (mob:FindFirstChild("HumanoidRootPart") or mob:FindFirstChildWhichIsA("BasePart"))
            if not mob or not mob.Parent or not mHum or mHum.Health <= 0 or not mHRP then
                CombatEngine.ClearStanceLock()
                return
            end
            local mPos = mHRP.Position
            local dx, dz = mPos.X - cf.Position.X, mPos.Z - cf.Position.Z
            local horizSq = dx * dx + dz * dz
            local isUnder = (Config.MobFarmPositionMode == "Under Map" or Config.MobFarmPositionMode == "Underground" or Config.MobFarmPositionMode == "Underneath Enemy")
            local expectedY = mPos.Y + (CombatEngine.ActiveStanceYOffset or (isUnder and -6.0 or (Config.MobFarmVerticalOffset or 5.5)))
            if horizSq < 784 and (horizSq > 0.04 or math.abs(expectedY - cf.Position.Y) > 0.25) then
                local liveCF = NavEngine.GetMobFarmCFrame(mob, Config.MobFarmPositionMode)
                if liveCF then
                    cf = liveCF
                    CombatEngine.ActiveStanceYOffset = liveCF.Position.Y - mPos.Y
                    CombatEngine.ActiveStanceCF = cf
                end
            end
        end
        State.HRP.CFrame = cf
        State.HRP.AssemblyLinearVelocity = Vector3.zero
        State.HRP.AssemblyAngularVelocity = Vector3.zero
    end
    trackConn(RunService.RenderStepped:Connect(function() applyStanceLock(false) end))
    trackConn(RunService.Heartbeat:Connect(function() applyStanceLock(true) end))
end

-- Shared melee engagement routine used by both AutoMobFarm and RaidEngine
function CombatEngine.EngageTargetMob(mob, opts)
    if not mob or not mob.Parent or not State.HRP or not State.Humanoid then
        return false, 0.14
    end
    local loopDelay = 0.14
    State.CurrentMob = mob
    State.LastFarmedMob = mob
    CombatEngine.EnsureWeaponEquipped()

    local posMode = Config.MobFarmPositionMode or "Above Enemy"
    local isBehind = (posMode == "Behind Enemy")
    local usesAnchorStance = not isBehind

    -- Clear lingering block flags whenever switching targets; keep Above-Enemy horizontal hover intact to prevent upright/horizontal bouncing
    if mob ~= CombatEngine.LastEngagedMob then
        CombatEngine.LastEngagedMob = mob
        if isBehind then
            CombatEngine.ClearStanceLock()
            if State.HRP and State.HRP.Anchored then
                State.HRP.Anchored = false
                State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
            end
        end
        if State.Humanoid then
            State.Humanoid.AutoRotate = isBehind
        end
        pcall(function()
            local TC = SafeGetController("ToolController")
            if TC then TC.isBlocking = false end
        end)
    end

    -- Clear any orphaned Blocking status after ParryHoldUntil expired so ToolActivated is never suppressed
    if not CombatEngine.IsParrying and os.clock() >= (CombatEngine.ParryHoldUntil or 0) then
        local charSt = State.Character and State.Character:FindFirstChild("Status")
        if charSt and charSt:FindFirstChild("Blocking") and Remotes.StopBlock then
            pcall(function() Remotes.StopBlock:InvokeServer() end)
        end
    end

    local hum = mob:FindFirstChildOfClass("Humanoid")
    local mHRP = mob:FindFirstChild("HumanoidRootPart") or mob:FindFirstChildWhichIsA("BasePart")
    if not (mHRP and hum and State.HRP) then
        return false, loopDelay
    end

    if RaidEngine and type(RaidEngine.GetActiveBossArena) == "function" then
        local a, aCF, aSz = RaidEngine.GetActiveBossArena()
        local b = a and NavEngine.GetBossPlayableBounds(a, aCF, aSz)
        if b and mHRP.Position.Y < (b.minY - 15.0) then
            -- Boss clipped out and is falling into the void: stay safely inside temple floor until void insta-kills him
            CombatEngine.ClearStanceLock()
            NavEngine.PreventBossArenaVoidFall()
            if State.HRP.Anchored then State.HRP.Anchored = false end
            return true, 0.12
        end
    end

    -- Evade Asura / Boss multi-hit skill moves (Barrage/Quadruple Slash, Decimate, Worldsplitter, Buddha, ProphecyVision)
    local bossSkill = (Config.EvadeBossSkills ~= false) and CombatEngine.GetActiveBossSkill(mob) or nil
    if bossSkill then
        CombatEngine.ActiveSkillName = bossSkill.skill
        local safeDist = math.clamp(tonumber(Config.BossSkillSafeDistance) or 50.0, 22, 95)
        local curPos = State.HRP.Position
        local mPos = mHRP.Position
        local horizDistFromBoss = Vector3.new(curPos.X - mPos.X, 0, curPos.Z - mPos.Z).Magnitude
        local retreatCF = NavEngine.GetBossSkillRetreatCFrame(mob, safeDist, posMode)

        if retreatCF then
            if usesAnchorStance then
                CombatEngine.SetHoverStance(true)
                if horizDistFromBoss < (safeDist - 4.0) then
                    local evadeSpeed = math.max(Config.GlideSpeed or 55, 95)
                    local stepStart = os.clock()
                    while ScriptRunning and State.HRP and mob.Parent and hum.Health > 0 and (os.clock() - stepStart) < 0.28 do
                        local dt = RunService.Heartbeat:Wait()
                        if not State.HRP or not mob.Parent or hum.Health <= 0 then break end
                        if not Config.AutoMobFarm and not Config.AutoRaid then break end
                        local pNow = State.HRP.Position
                        local delta = retreatCF.Position - pNow
                        local dMag = delta.Magnitude
                        CombatEngine.SetHoverStance(true)
                        State.HRP.AssemblyLinearVelocity = Vector3.zero
                        State.HRP.AssemblyAngularVelocity = Vector3.zero
                        if dMag <= 1.5 then
                            CombatEngine.ActiveStanceCF = retreatCF
                            State.HRP.CFrame = retreatCF
                            break
                        end
                        local nextPos = pNow + delta.Unit * math.min(dMag, evadeSpeed * dt)
                        local nextCF = CFrame.new(nextPos) * retreatCF.Rotation
                        CombatEngine.ActiveStanceCF = nextCF
                        State.HRP.CFrame = nextCF
                    end
                else
                    CombatEngine.SetHoverStance(true)
                    CombatEngine.ActiveStanceCF = CFrame.new(curPos) * retreatCF.Rotation
                    State.HRP.CFrame = CombatEngine.ActiveStanceCF
                    State.HRP.AssemblyLinearVelocity = Vector3.zero
                    State.HRP.AssemblyAngularVelocity = Vector3.zero
                end
            else
                CombatEngine.ClearStanceLock()
                if State.HRP.Anchored then State.HRP.Anchored = false end
                if State.Humanoid then State.Humanoid.AutoRotate = true end
                if horizDistFromBoss < (safeDist - 4.0) then
                    if (curPos - retreatCF.Position).Magnitude > 14 then
                        NavEngine.NavigateTo(retreatCF, true)
                    else
                        State.HRP.CFrame = retreatCF
                        State.HRP.AssemblyLinearVelocity = Vector3.zero
                    end
                end
            end
        end
        return true, 0.06
    else
        CombatEngine.ActiveSkillName = nil
    end

    local targetCFrame = NavEngine.GetMobFarmCFrame(mob, posMode)
    if not targetCFrame then
        return false, loopDelay
    end

    local charPos = State.HRP.Position
    local mPosNow = mHRP.Position
    local isKitingTarget = CombatEngine.IsKitingMob(mob)
    local mobVelXZ = Vector3.new(mHRP.AssemblyLinearVelocity.X, 0, mHRP.AssemblyLinearVelocity.Z)
    local toPlayerXZ = Vector3.new(charPos.X - mPosNow.X, 0, charPos.Z - mPosNow.Z)
    local mobDistHoriz = toPlayerXZ.Magnitude
    local heightDiff = math.abs(charPos.Y - mPosNow.Y)
    local desiredDist = math.max(math.clamp(Config.MobFarmDistance or 4.0, 2.4, 10.0), 2.4)
    local mLookXZ = Vector3.new(mHRP.CFrame.LookVector.X, 0, mHRP.CFrame.LookVector.Z)
    local inFrontCone = (mobDistHoriz > 0.2 and mLookXZ.Magnitude > 0.05) and (toPlayerXZ.Unit:Dot(mLookXZ.Unit) > 0.45) or false
    local behindHold = isBehind
        and mobDistHoriz >= 2.0
        and mobDistHoriz <= (isKitingTarget and 4.5 or (desiredDist + 3.0))
        and heightDiff <= 5.5
        and (not isKitingTarget or mobVelXZ.Magnitude <= 3.5)
        and (not inFrontCone or (os.clock() - (CombatEngine.LastBehindReposition or 0)) < 0.75)

    local horizDist = Vector3.new(charPos.X - targetCFrame.Position.X, 0, charPos.Z - targetCFrame.Position.Z).Magnitude
    local vertDist = math.abs(charPos.Y - targetCFrame.Position.Y)
    local fullDist = (charPos - targetCFrame.Position).Magnitude

    if usesAnchorStance then
        if horizDist > 180 or fullDist > 180 then
            -- Very far target across world map: drop hover and glide cleanly
            CombatEngine.ClearStanceLock()
            if State.HRP.Anchored then State.HRP.Anchored = false end
            if State.Humanoid then State.Humanoid.AutoRotate = true end
            local navOk = NavEngine.NavigateTo(targetCFrame)
            if not navOk then
                if State.Humanoid and State.Humanoid.Health > 0 and (not opts or opts.blacklistOnFail ~= false) then
                    CombatEngine.Blacklist[mob] = os.clock() + 6.0
                    State.TargetMob = nil
                    State.CurrentMob = nil
                    State.LastFarmedMob = nil
                end
                return false, loopDelay
            end
        elseif horizDist > 2.2 or vertDist > 3.0 then
            -- Unanchored physics hover across the whole arena (client->server CFrame replication stays 100% live!)
            local slideSpeed = math.max(Config.GlideSpeed or 55, isKitingTarget and 85 or 55)
            local maxSlideTime = math.clamp((fullDist / slideSpeed) + 0.8, 1.2, 6.0)
            local slideStart = os.clock()
            CombatEngine.SetHoverStance(true)
            while ScriptRunning and State.HRP and mob.Parent and hum.Health > 0 and (os.clock() - slideStart) < maxSlideTime do
                if State.IsBuyingPotions or State.IsAutoForging or State.DrainingFullBag or State.IsCrafting or State.SellingMobLoot then break end
                if not Config.AutoMobFarm and not Config.AutoRaid then break end
                if os.clock() < (CombatEngine.DodgeUntil or 0) then break end
                local dt = RunService.Heartbeat:Wait()
                if not State.HRP or not mob.Parent or hum.Health <= 0 then break end
                local liveTarget = NavEngine.GetMobFarmCFrame(mob, posMode)
                if not liveTarget then break end
                local curPos = State.HRP.Position
                local delta = liveTarget.Position - curPos
                local dMag = delta.Magnitude
                CombatEngine.SetHoverStance(true)
                State.HRP.AssemblyLinearVelocity = Vector3.zero
                State.HRP.AssemblyAngularVelocity = Vector3.zero
                CombatEngine.ActiveStanceMob = mob
                CombatEngine.ActiveStanceYOffset = liveTarget.Position.Y - mHRP.Position.Y
                if dMag <= 1.4 then
                    CombatEngine.ActiveStanceCF = liveTarget
                    State.HRP.CFrame = liveTarget
                    CombatEngine.TrySwingWeapon(mHRP)
                    break
                end
                local stepPos = curPos + delta.Unit * math.min(dMag, slideSpeed * dt)
                local nextCF = CFrame.new(stepPos) * liveTarget.Rotation
                CombatEngine.ActiveStanceCF = nextCF
                State.HRP.CFrame = nextCF
                if dMag <= 16.0 then
                    CombatEngine.TrySwingWeapon(mHRP)
                end
            end
        end
    elseif not behindHold and (mobDistHoriz > (isKitingTarget and 26.0 or 14.0) or heightDiff > 6.0) then
        CombatEngine.ClearStanceLock()
        CombatEngine.LastBehindReposition = os.clock()
        if State.HRP.Anchored then State.HRP.Anchored = false end
        if State.Humanoid then State.Humanoid.AutoRotate = true end

        local navOk = NavEngine.NavigateTo(targetCFrame)
        if not navOk then
            if State.Humanoid and State.Humanoid.Health > 0 and (not opts or opts.blacklistOnFail ~= false) then
                CombatEngine.Blacklist[mob] = os.clock() + 6.0
                State.TargetMob = nil
                State.CurrentMob = nil
                State.LastFarmedMob = nil
            end
            return false, loopDelay
        end
    end

    -- Refresh CFrame after transit and hold stance cleanly
    local isDodging = os.clock() < (CombatEngine.DodgeUntil or 0)
    if behindHold then
        -- Hold steady in melee range behind/flanking enemy instead of circling every time enemy turns
        CombatEngine.ActiveStanceCF = nil
        CombatEngine.ActiveStanceMob = nil
        if State.HRP and not isDodging then
            if State.HRP.Anchored then State.HRP.Anchored = false end
            if not Config.Noclip and not NavEngine.IsGliding then
                NavEngine.RestoreCharacterPhysics()
            end
            if State.Humanoid then State.Humanoid.AutoRotate = false end
            if not CombatEngine.IsBlockingOrParrying() then
                local pNow = State.HRP.Position
                local lookTarget = Vector3.new(mHRP.Position.X, pNow.Y, mHRP.Position.Z)
                if (lookTarget - pNow).Magnitude > 0.1 then
                    State.HRP.CFrame = CFrame.lookAt(pNow, lookTarget)
                end
            end
            if CombatEngine.TrySwingWeapon(mHRP) then
                loopDelay = isKitingTarget and 0.03 or 0.04
            else
                loopDelay = isKitingTarget and 0.03 or 0.05
            end
        end
        return true, loopDelay
    end

    targetCFrame = NavEngine.GetMobFarmCFrame(mob, posMode)
    if targetCFrame and State.HRP and not isDodging then
        charPos = State.HRP.Position
        horizDist = Vector3.new(charPos.X - targetCFrame.Position.X, 0, charPos.Z - targetCFrame.Position.Z).Magnitude
        vertDist = math.abs(charPos.Y - targetCFrame.Position.Y)
        if usesAnchorStance and horizDist <= 6.0 and vertDist <= 8.0 then
            CombatEngine.SetHoverStance(true)
            CombatEngine.ActiveStanceMob = mob
            CombatEngine.ActiveStanceYOffset = targetCFrame.Position.Y - mHRP.Position.Y
            CombatEngine.ActiveStanceCF = targetCFrame
            State.HRP.CFrame = targetCFrame
            State.HRP.AssemblyLinearVelocity = Vector3.zero
            State.HRP.AssemblyAngularVelocity = Vector3.zero
        elseif isBehind and horizDist <= (isKitingTarget and 26.0 or 14.0) then
            -- Behind Enemy short reposition: stay unanchored on solid floor and intercept kiting mobs immediately
            CombatEngine.ClearStanceLock()
            CombatEngine.LastBehindReposition = os.clock()
            if State.HRP.Anchored then State.HRP.Anchored = false end
            if not Config.Noclip and not NavEngine.IsGliding then
                NavEngine.RestoreCharacterPhysics()
            end
            if State.Humanoid then State.Humanoid.AutoRotate = false end
            local safePos = targetCFrame.Position
            if State.HRP.Position.Y > safePos.Y and (State.HRP.Position.Y - safePos.Y) <= 1.6 then
                safePos = Vector3.new(safePos.X, State.HRP.Position.Y, safePos.Z)
            end
            local rot = CombatEngine.IsBlockingOrParrying() and State.HRP.CFrame.Rotation or targetCFrame.Rotation
            State.HRP.CFrame = CFrame.new(safePos) * rot
            local curVelY = State.HRP.AssemblyLinearVelocity.Y
            State.HRP.AssemblyLinearVelocity = Vector3.new(0, math.max(curVelY, 0), 0)
            State.HRP.AssemblyAngularVelocity = Vector3.zero
            if isKitingTarget and State.Humanoid then
                pcall(function() State.Humanoid:MoveTo(mHRP.Position) end)
            end
            loopDelay = isKitingTarget and 0.03 or 0.05
        elseif isBehind then
            CombatEngine.ActiveStanceCF = nil
            CombatEngine.ActiveStanceMob = nil
            if State.HRP.Anchored then State.HRP.Anchored = false end
            if State.Humanoid then State.Humanoid.AutoRotate = true end
        end

        if CombatEngine.TrySwingWeapon(mHRP) then
            loopDelay = isKitingTarget and 0.03 or 0.04
        end
    elseif isDodging then
        CombatEngine.ActiveStanceCF = nil
        CombatEngine.ActiveStanceMob = nil
    end

    return true, loopDelay
end

task.spawn(function()
    while ScriptRunning do
        local loopDelay = 0.14
        local raidActive = Config.AutoRaid and (State.IsInRaid or (game.PlaceId == 84980181568016) or workspace:GetAttribute("AbyssRun") == true or workspace:FindFirstChild("RaidLevels") ~= nil)
        if Config.AutoMobFarm and not raidActive and not State.IsAutoForging and not State.DrainingFullBag and not State.IsCrafting and not State.IsBuyingPotions and State.HRP and State.Humanoid and State.Humanoid.Health > 0 then
            -- Auto-Sell Mob Loot to Greedy Cey -> Auto-Sell Unchecked Runes when bag hits capacity
            if (Config.AutoSellMobLootWhenFull or Config.AutoSellRunes) and (os.clock() - (State.LastMobLootSellAttempt or 0)) >= 6.0 then
                local used, cap, isFull = ForgeEngine.GetStashCapacityStatus()
                if isFull then
                    State.LastMobLootSellAttempt = os.clock()
                    if Config.AutoSellMobLootWhenFull then
                        State.SellingMobLoot = true
                        CombatEngine.ClearStanceLock()
                        if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
                        if State.Humanoid then State.Humanoid.AutoRotate = true end
                        local soldUnits, soldTypes, soldOk = ForgeEngine.SellAllMobLootExceptBlacklist()
                        State.StashFullSignalUntil = 0
                        State.SellingMobLoot = false
                        if soldUnits > 0 and soldOk then
                            Notify("Greedy Cey Auto-Sell", string.format("Bag full (%d/%d) -> Sold %d item(s) (%d types)!", used, cap, soldUnits, soldTypes), 3.5)
                        elseif soldUnits > 0 then
                            Notify("Greedy Cey Auto-Sell", "Bag full, attempted sell (check if Greedy Cey is reachable)", 3.0)
                        elseif not Config.AutoSellRunes then
                            Notify("Greedy Cey Auto-Sell", "Bag full, but all items are blacklisted or favorited!", 3.0)
                        end
                    end
                    if Config.AutoSellRunes and type(State.SellUnkeptRunes) == "function" then
                        State.SellUnkeptRunes(true)
                    end
                    task.wait(0.35)
                end
            end

            local mob = State.TargetMob
            local isMobAlive = mob and mob.Parent and mob:IsDescendantOf(workspace)
            local hum = isMobAlive and mob:FindFirstChildOfClass("Humanoid")
            if isMobAlive and hum and hum.Health > 0 and CombatEngine.IsBossShielded(mob) then
                local addMob = CombatEngine.FindClosestMob()
                if addMob and addMob ~= mob and not CombatEngine.IsBossShielded(addMob) then
                    mob = addMob
                    State.TargetMob = addMob
                    State.LastFarmedMob = addMob
                end
            elseif isMobAlive and hum and hum.Health > 0 and Config.PrioritizeStrongestMob ~= false then
                local hpPctNow = (hum.MaxHealth > 0) and (hum.Health / hum.MaxHealth) or 1
                if hpPctNow > 0.25 then
                    local betterMob = CombatEngine.FindClosestMob()
                    if betterMob and betterMob ~= mob and not CombatEngine.IsBossShielded(betterMob)
                        and CombatEngine.GetMobStrengthScore(betterMob) > CombatEngine.GetMobStrengthScore(mob) then
                        mob = betterMob
                        State.TargetMob = betterMob
                        State.LastFarmedMob = betterMob
                    end
                end
            elseif not isMobAlive or not hum or hum.Health <= 0 or not CombatEngine.MatchesTargetMob(mob) then
                if not Config.AutoMine then
                    if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
                    if State.Humanoid and not State.Humanoid.AutoRotate then State.Humanoid.AutoRotate = true end
                end
                State.TargetMob = nil
                State.LastFarmedMob = nil
                State.ResumingMobAfterDeath = false
                mob = CombatEngine.FindClosestMob()
                if mob then
                    State.TargetMob = mob
                    State.LastFarmedMob = mob
                end
            end

            if mob and mob.Parent then
                local _, stepDelay = CombatEngine.EngageTargetMob(mob, { blacklistOnFail = true })
                loopDelay = stepDelay or 0.14
            else
                CombatEngine.LastEngagedMob = nil
                if not Config.AutoMine then
                    CombatEngine.ClearStanceLock()
                    if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
                    if State.Humanoid and not State.Humanoid.AutoRotate then State.Humanoid.AutoRotate = true end
                end
            end
        elseif not raidActive then
            CombatEngine.LastEngagedMob = nil
            if not Config.AutoMine and not State.IsAutoForging then
                CombatEngine.ClearStanceLock()
                if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
                if State.Humanoid and not State.Humanoid.AutoRotate then State.Humanoid.AutoRotate = true end
            end
            -- Preserve LastFarmedMob if dead while AutoMobFarm + ResumeLastMobAfterDeath are ON so respawn returns straight to it!
            local isDeadResuming = Config.AutoMobFarm and Config.ResumeLastMobAfterDeath and State.Humanoid and State.Humanoid.Health <= 0
            if not isDeadResuming then
                State.TargetMob = nil
                State.CurrentMob = nil
                if not Config.AutoMobFarm then
                    State.LastFarmedMob = nil
                    State.ResumingMobAfterDeath = false
                end
            end
        end
        task.wait(loopDelay)
    end
end)

-- ===================================================================
-- 10B. AUTO RAID & AUTO ABYSS ENGINE (Abyss Cards, Party, Orbs & Gates)
-- ===================================================================
local RaidEngine = {
    OwnedCards = {},
    CrossedGates = {},
    OpenedChests = {},
    BlacklistedMobs = {},
    CurrentFloor = 0,
    CurrentWave = 0,
    EnemiesLeft = nil,
    IsEnded = false,
    StatusText = "Idle",
    LastPartyAction = 0,
    LastEnterAttempt = 0,
    LastLeaveAttempt = 0,
    LastCardPick = 0,
    LastReviveAttempt = 0,
    RaidPlaceSet = { [84980181568016] = true },
    -- All 38 Abyss Card Weights (Balanced prioritizes highest Attack/DMG + Defense/DR/HP stats)
    CardMeta = {
        -- Common (1-5)
        [1]  = { name = "Sharpened",       rarity = "Common",  bal = 72,  dps = 76,  safe = 52,  syn = { 23 } },
        [2]  = { name = "Hardy",           rarity = "Common",  bal = 68,  dps = 26,  safe = 74,  syn = { 24 } },
        [3]  = { name = "Swift",           rarity = "Common",  bal = 32,  dps = 28,  safe = 38,  syn = { 25 } },
        [4]  = { name = "Keen Eye",        rarity = "Common",  bal = 58,  dps = 68,  safe = 44,  syn = { 26 } },
        [5]  = { name = "Guarded",         rarity = "Common",  bal = 70,  dps = 24,  safe = 78,  syn = { 27 } },
        -- Rare (6-11)
        [6]  = { name = "Brutal",          rarity = "Rare",    bal = 92,  dps = 98,  safe = 70,  syn = { 23, 30 } },
        [7]  = { name = "Frenzy",          rarity = "Rare",    bal = 86,  dps = 92,  safe = 66,  syn = { 28, 30 } },
        [8]  = { name = "Vital Core",      rarity = "Rare",    bal = 84,  dps = 38,  safe = 94,  syn = { 24 } },
        [9]  = { name = "Mending",         rarity = "Rare",    bal = 62,  dps = 30,  safe = 78,  syn = { 28 } },
        [10] = { name = "Glide",           rarity = "Rare",    bal = 38,  dps = 34,  safe = 46,  syn = { 25 } },
        [11] = { name = "Overcharge",      rarity = "Rare",    bal = 90,  dps = 94,  safe = 72 },
        -- Epic (12-16)
        [12] = { name = "Executioner",     rarity = "Epic",    bal = 98,  dps = 108, safe = 74,  syn = { 26 } },
        [13] = { name = "Titan",           rarity = "Epic",    bal = 112, dps = 96,  safe = 112, syn = { 29 } },
        [14] = { name = "Bulwark",         rarity = "Epic",    bal = 108, dps = 52,  safe = 116, syn = { 27, 29 } },
        [15] = { name = "Vampiric",        rarity = "Epic",    bal = 104, dps = 78,  safe = 108 },
        [16] = { name = "Second Life",     rarity = "Epic",    bal = 96,  dps = 68,  safe = 110 },
        -- Cursed (17-22)
        [17] = { name = "Glass Cannon",    rarity = "Cursed",  bal = 28,  dps = 118, safe = -130 },
        [18] = { name = "Greed",           rarity = "Cursed",  bal = 14,  dps = 20,  safe = -90 },
        [19] = { name = "Bloodlust",       rarity = "Cursed",  bal = 24,  dps = 110, safe = -125 },
        [20] = { name = "Berserker",       rarity = "Cursed",  bal = 26,  dps = 112, safe = -125 },
        [21] = { name = "Fragile Fortune", rarity = "Cursed",  bal = 22,  dps = 102, safe = -125 },
        [22] = { name = "Tunnel Vision",   rarity = "Cursed",  bal = 36,  dps = 86,  safe = -65 },
        -- Fusion (23-30)
        [23] = { name = "Butcher",         rarity = "Fusion",  bal = 142, dps = 152, safe = 110, reqs = { 1, 6 } },
        [24] = { name = "Ironheart",       rarity = "Fusion",  bal = 132, dps = 80,  safe = 144, reqs = { 2, 8 } },
        [25] = { name = "Phantom Step",    rarity = "Fusion",  bal = 86,  dps = 84,  safe = 92,  reqs = { 3, 10 } },
        [26] = { name = "Deadeye",         rarity = "Fusion",  bal = 124, dps = 140, safe = 96,  reqs = { 4, 12 } },
        [27] = { name = "Bastion",         rarity = "Fusion",  bal = 138, dps = 82,  safe = 150, reqs = { 5, 14 } },
        [28] = { name = "Second Wind",     rarity = "Fusion",  bal = 118, dps = 106, safe = 122, reqs = { 7, 9 } },
        [29] = { name = "Colossus",        rarity = "Fusion",  bal = 145, dps = 112, safe = 156, reqs = { 13, 14 } },
        [30] = { name = "Rampage",         rarity = "Fusion",  bal = 140, dps = 154, safe = 108, reqs = { 6, 7 } },
        -- Abyssal (31-38)
        [31] = { name = "Abyssal Frenzy",  rarity = "Abyssal", bal = 134, dps = 142, safe = 106 },
        [32] = { name = "Last Stand",      rarity = "Abyssal", bal = 132, dps = 128, safe = 134 },
        [33] = { name = "Undying",         rarity = "Abyssal", bal = 114, dps = 90,  safe = 136 },
        [34] = { name = "First Strike",    rarity = "Abyssal", bal = 122, dps = 134, safe = 98 },
        [35] = { name = "Final Verdict",   rarity = "Abyssal", bal = 124, dps = 138, safe = 100 },
        [36] = { name = "Detonation",      rarity = "Abyssal", bal = 118, dps = 130, safe = 96 },
        [37] = { name = "Deep Pact",       rarity = "Abyssal", bal = 136, dps = 150, safe = 82 },
        [38] = { name = "Golden Tide",     rarity = "Abyssal", bal = 60,  dps = 50,  safe = 64 },
    },
    FusionReqs = {
        [23] = { 1, 6 },
        [24] = { 2, 8 },
        [25] = { 3, 10 },
        [26] = { 4, 12 },
        [27] = { 5, 14 },
        [28] = { 7, 9 },
        [29] = { 13, 14 },
        [30] = { 6, 7 },
    },
    LiveCardsById = {},
    CardNumById = {},
}

function RaidEngine.Notify(title, desc, dur)
    local uiLib = Library or (typeof(getgenv) == "function" and getgenv().Library) or _G.Library
    if uiLib and type(uiLib.Notify) == "function" then
        pcall(function()
            uiLib:Notify({
                Title = tostring(title or "Auto Raid"),
                Desc = tostring(desc or ""),
                Duration = dur or 3
            })
        end)
    end
end

for idx, entry in pairs(RaidEngine.CardMeta) do
    if type(idx) == "number" and type(entry) == "table" and entry.name then
        local noSpace = tostring(entry.name):gsub("%s+", "")
        RaidEngine.CardMeta[noSpace] = entry
        RaidEngine.CardNumById[noSpace] = idx
    end
end
RaidEngine.CardMeta["Vital"] = RaidEngine.CardMeta[8]
RaidEngine.CardNumById["Vital"] = 8
RaidEngine.CardMeta["Keen"] = RaidEngine.CardMeta[4]
RaidEngine.CardNumById["Keen"] = 4

runIsolated(function()
    local function addPlaces(tbl)
        if type(tbl) ~= "table" then return end
        for _, v in pairs(tbl) do
            if type(v) == "table" and type(v.PlaceIDs) == "table" then
                for _, pid in pairs(v.PlaceIDs) do
                    if type(pid) == "number" then RaidEngine.RaidPlaceSet[pid] = true end
                end
            elseif type(v) == "number" and v > 1000000 then
                RaidEngine.RaidPlaceSet[v] = true
            end
        end
    end
    pcall(function()
        local AD = safeRequire(ReplicatedStorage.Shared.Data.Abyss)
        if type(AD) == "table" then
            addPlaces(AD)
            if type(AD.Cards) == "table" then
                RaidEngine.LiveCards = AD.Cards
                for idx, c in pairs(AD.Cards) do
                    if type(c) == "table" and c.Id then
                        RaidEngine.LiveCardsById[c.Id] = c
                        if type(idx) == "number" and RaidEngine.CardMeta[idx] then
                            RaidEngine.CardMeta[c.Id] = RaidEngine.CardMeta[idx]
                            RaidEngine.CardNumById[c.Id] = idx
                        end
                    end
                end
            end
        end
    end)
    pcall(function()
        local RD = safeRequire(ReplicatedStorage.Shared.Data.Raids)
        addPlaces(RD)
    end)
end)

function RaidEngine.GetAvailablePartyModes()
    local modes = {}
    local seen = {}
    local function push(name)
        if type(name) == "string" and name ~= "" and not seen[name] then
            seen[name] = true
            table.insert(modes, name)
        end
    end
    push("Abyss")
    push("Tower Of Hell")
    local prox = workspace:FindFirstChild("Proximity")
    if prox then
        for _, d in ipairs(prox:GetDescendants()) do
            local pt = d:GetAttribute("PartyType")
            if type(pt) == "string" then push(pt) end
        end
    end
    pcall(function()
        local RD = safeRequire(ReplicatedStorage.Shared.Data.Raids)
        if type(RD) == "table" then
            for k, v in pairs(RD) do
                if type(v) == "table" and (v.Tiers or v.PlaceIDs) then
                    push(tostring(k))
                end
            end
        end
    end)
    return modes
end

function RaidEngine.IsInAbyssLobby()
    if workspace:GetAttribute("AbyssRun") == true then return false end
    local lobby = workspace:FindFirstChild("AbyssLobby")
    local pos = State.HRP and State.HRP.Position
    if lobby and pos then
        local reg = lobby:FindFirstChild("RegionPart", true)
        if reg and reg:IsA("BasePart") then
            local rel = reg.CFrame:PointToObjectSpace(pos)
            local hs = reg.Size * 0.5
            if math.abs(rel.X) <= hs.X + 40 and math.abs(rel.Y) <= hs.Y + 120 and math.abs(rel.Z) <= hs.Z + 40 then
                return true
            end
        end
    end
    if pos and math.abs(pos.X + 9000) <= 450 and math.abs(pos.Z + 9000) <= 450 and pos.Y >= 850 and pos.Y <= 1450 then
        return true
    end
    return false
end

function RaidEngine.GetActiveBossArena()
    if not State.HRP then return nil end
    local assets = workspace:FindFirstChild("Assets")
    local arenas = assets and assets:FindFirstChild("BossArenas")
    if not arenas then return nil end
    local pos = State.HRP.Position
    for _, arena in ipairs(arenas:GetChildren()) do
        if arena:IsA("Model") or arena:IsA("Folder") then
            local okBox, cf, sz = pcall(function() return arena:GetBoundingBox() end)
            if okBox and cf and sz then
                local rel = cf:PointToObjectSpace(pos)
                local hs = sz * 0.5
                if math.abs(rel.X) <= hs.X + 160 and math.abs(rel.Y) <= hs.Y + 180 and math.abs(rel.Z) <= hs.Z + 160 then
                    RaidEngine.LastBossArenaAt = os.clock()
                    RaidEngine.LastBossArenaPos = cf.Position
                    RaidEngine.StrandedSince = nil
                    return arena, cf, sz
                end
            end
        end
    end
    return nil
end
function RaidEngine.IsNearActiveBossMob(maxDist)
    if not State.HRP then return false end
    maxDist = maxDist or 450
    local living = workspace:FindFirstChild("Living")
    if not living then return false end
    local myPos = State.HRP.Position
    for _, mob in ipairs(living:GetChildren()) do
        if mob:IsA("Model") and mob ~= State.Character then
            local hum = mob:FindFirstChildOfClass("Humanoid")
            local mHrp = mob:FindFirstChild("HumanoidRootPart") or mob.PrimaryPart
            if hum and hum.Health > 0 and mHrp and (mHrp.Position - myPos).Magnitude <= maxDist then
                local lname = string.lower(mob.Name)
                if mob:GetAttribute("IsBoss") == true or lname:find("asura", 1, true) or lname:find("boss", 1, true) or lname:find("incarnate", 1, true) or lname:find("behemoth", 1, true) then
                    return true
                end
            end
        end
    end
    return false
end


function RaidEngine.GetRaidTicketCount()
    ResolvePlayerData()
    local misc = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory and State.PlayerData.Data.Inventory.Misc
    local total = 0
    if type(misc) == "table" then
        for _, entry in ipairs(misc) do
            if type(entry) == "table" and (entry.Id == "RaidTicket" or entry.Name == "Raid Ticket" or entry.Name == "RaidTicket") then
                total = total + (tonumber(entry.Quantity) or 1)
            end
        end
    end
    return total
end

function RaidEngine.NormalizeMode(mode)
    if type(mode) ~= "string" then return "Abyss" end
    local trimmed = mode:gsub("%s*%(Raid Tickets%)$", "")
    if trimmed == "" then return "Abyss" end
    return trimmed
end

function RaidEngine.FormatModeLabel(mode)
    local norm = RaidEngine.NormalizeMode(mode)
    if norm == "Asura's Incarnate" then
        return "Asura's Incarnate (Raid Tickets)"
    end
    return norm
end

function RaidEngine.ResolveEffectiveMode()
    if not Config.AutoAsuraToTower then
        return RaidEngine.NormalizeMode(Config.RaidMode or "Abyss")
    end
    if RaidEngine.RaidPlaceSet[game.PlaceId] or workspace:FindFirstChild("RaidLevels") ~= nil then
        return "Tower Of Hell"
    end
    local targetTickets = math.max(1, math.floor(tonumber(Config.AsuraTargetTickets) or 5))
    local tickets = RaidEngine.GetRaidTicketCount()
    if tickets >= targetTickets then
        return "Tower Of Hell", tickets, targetTickets
    end
    return "Asura's Incarnate", tickets, targetTickets
end

function RaidEngine.GetBossWindowInfo(mode)
    mode = RaidEngine.NormalizeMode(mode)
    if type(mode) ~= "string" or mode == "" then return nil end
    local info = nil
    pcall(function()
        local Bosses = safeRequire(ReplicatedStorage.Shared.Data.Bosses)
        local cfg = Bosses and Bosses[mode]
        if type(cfg) == "table" then
            local TimeUtil = safeRequire(ReplicatedStorage.Shared.Utils.TimeUtil)
            local open, rem, eventClosed = true, 0, false
            if TimeUtil and cfg.SpawnInterval then
                local timeUntilNext = select(1, TimeUtil.GetTimeUntilNextInterval(cfg.SpawnInterval))
                local elapsed = TimeUtil.GetUTC() - timeUntilNext
                local dur = cfg.ActiveDuration or cfg.SpawnInterval
                open = elapsed <= dur
                rem = open and math.max(0, dur - elapsed) or math.max(0, timeUntilNext - TimeUtil.GetUTC())
            end
            info = { isBoss = true, open = open and not eventClosed, remainingSec = rem, cfg = cfg }
        end
    end)
    return info
end

function RaidEngine.IsInActiveRun()
    if workspace:GetAttribute("AbyssRun") == true then return true end
    if workspace:FindFirstChild("RaidLevels") ~= nil then return true end
    if RaidEngine.RaidPlaceSet[game.PlaceId] then return true end
    if (RaidEngine.CurrentWave or 0) > 0 and not RaidEngine.IsEnded then return true end
    if RaidEngine.GetActiveBossArena() ~= nil then return true end
    return false
end

function RaidEngine.GetPartyState()
    local info = { inParty = false, isOwner = false, partyType = nil, tier = nil }
    pcall(function()
        runAsGameThread(function()
            local Knit = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
            local PC = SafeGetController("PartyController", Knit)
            local cur = PC and PC.CurrentParty
            if cur then
                info.inParty = true
                info.partyType = cur.Type
                info.tier = cur.Tier
                local data = cur.Replica and cur.Replica.Data
                if data then
                    info.isOwner = (data.Owner == LocalPlayer) or (data.Owner == LocalPlayer.UserId)
                    info.partyType = info.partyType or data.Type
                    info.tier = info.tier or data.Tier
                end
            end
        end, 1.5)
    end)
    return info
end

function RaidEngine.GetAbyssInfo()
    if not Remotes.AbyssGetInfo then return nil end
    local ok, res = pcall(function() return Remotes.AbyssGetInfo:InvokeServer() end)
    if ok and type(res) == "table" then return res end
    return nil
end


function RaidEngine.ScoreCard(cardId, strategy)
    strategy = strategy or Config.AbyssCardStrategy or "Balanced (Meta DPS+Survive)"
    local normNum = tonumber(cardId) or RaidEngine.CardNumById[cardId]
    local meta = RaidEngine.CardMeta[cardId] or (normNum and RaidEngine.CardMeta[normNum])
    local live = (RaidEngine.LiveCardsById and RaidEngine.LiveCardsById[cardId]) or (RaidEngine.LiveCards and RaidEngine.LiveCards[cardId])
    local rarity = (meta and meta.rarity) or (live and live.Rarity) or "Common"
    local base = 30
    if meta then
        if strategy == "Max Damage (Glass Cannon OK)" then
            base = meta.dps
        elseif strategy == "Safe Survival (No Risky Cursed)" then
            base = meta.safe
        else
            base = meta.bal
        end
    else
        local rarityBase = { Abyssal = 115, Fusion = 110, Epic = 82, Rare = 64, Common = 44, Cursed = (strategy == "Safe Survival (No Risky Cursed)" and -100 or 45) }
        base = rarityBase[rarity] or 40
    end

    -- Fusion prerequisite synergy bonus: prioritize missing half of an unbuilt Fusion!
    local bonus = 0
    if meta and meta.syn then
        for _, fusionId in ipairs(meta.syn) do
            if not RaidEngine.OwnedCards[fusionId] then
                local reqs = RaidEngine.FusionReqs[fusionId]
                if reqs then
                    local hasOther = false
                    for _, reqId in ipairs(reqs) do
                        if reqId ~= (normNum or cardId) and RaidEngine.OwnedCards[reqId] then
                            hasOther = true
                            break
                        end
                    end
                    if hasOther then bonus = bonus + 34 else bonus = bonus + 8 end
                end
            end
        end
    end

    local ownedCount = RaidEngine.OwnedCards[cardId] or (normNum and RaidEngine.OwnedCards[normNum]) or 0
    if ownedCount > 0 and rarity ~= "Fusion" then
        bonus = bonus + 4
    end
    return base + bonus, (meta and meta.name) or (live and live.Name) or ("Card " .. tostring(cardId)), rarity
end

function RaidEngine.HandleCardOffer(offer)
    if not ScriptRunning or type(offer) ~= "table" or type(offer.Cards) ~= "table" or #offer.Cards == 0 then
        return false
    end
    local strategy = Config.AbyssCardStrategy or "Balanced (Meta DPS+Survive)"
    local bestId, bestScore, bestName, bestRarity = nil, -math.huge, "", ""
    for _, cid in ipairs(offer.Cards) do
        local idNum = tonumber(cid) or cid
        local sc, nm, rar = RaidEngine.ScoreCard(idNum, strategy)
        if sc > bestScore then
            bestScore = sc
            bestId = idNum
            bestName = nm
            bestRarity = rar
        end
    end

    if strategy == "Safe Survival (No Risky Cursed)" and bestScore < 0 and offer.CanSkip and Remotes.AbyssSkipCards then
        local ok = pcall(function() return Remotes.AbyssSkipCards:InvokeServer() end)
        if ok then
            RaidEngine.Notify("Abyss Card Picker", "Skipped all risky Cursed cards (Safe Survival)", 3.0)
            return true
        end
    end

    if bestId ~= nil and Remotes.AbyssPickCard then
        local ok = pcall(function() return Remotes.AbyssPickCard:InvokeServer(bestId) end)
        if ok then
            local normNum = tonumber(bestId) or RaidEngine.CardNumById[bestId]
            RaidEngine.OwnedCards[bestId] = (RaidEngine.OwnedCards[bestId] or 0) + 1
            if normNum and normNum ~= bestId then
                RaidEngine.OwnedCards[normNum] = (RaidEngine.OwnedCards[normNum] or 0) + 1
            end
            RaidEngine.LastCardPick = os.clock()
            RaidEngine.Notify("Abyss Card Picked", string.format("%s [%s] (score %d)", tostring(bestName), tostring(bestRarity), math.floor(bestScore)), 3.0)
            return true
        end
    end
    return false
end

function RaidEngine.FindClosestRaidEnemy()
    if not State.HRP then return nil end
    local hrpPos = State.HRP.Position
    local now = os.clock()
    -- Prioritize unshielded adds/minions (e.g. Crystal Centipede, Abyssal Watcher) whenever a Boss is immune/shielded
    local bestNormalMob, bestNormalDist = nil, 450 -- Stay inside current floor arena; adjacent floor slots are 3,000 studs across void
    local bestShieldedMob, bestShieldedDist = nil, 450
    local function scanContainer(root)
        if not root then return end
        for _, m in ipairs(root:GetChildren()) do
            if CombatEngine.IsHostileMob(m) then
                local bl = RaidEngine.BlacklistedMobs[m]
                if not bl or now >= bl then
                    local mHRP = m:FindFirstChild("HumanoidRootPart") or m:FindFirstChildWhichIsA("BasePart")
                    if mHRP then
                        local voidLimit = (workspace.FallenPartsDestroyHeight or -500) + 60
                        local d = (mHRP.Position - hrpPos).Magnitude
                        if d <= 450 and mHRP.Position.Y > voidLimit and math.abs(mHRP.Position.Y - hrpPos.Y) <= 160 then
                            if CombatEngine.IsBossShielded(m) then
                                if d < bestShieldedDist then
                                    bestShieldedDist = d
                                    bestShieldedMob = m
                                end
                            elseif d < bestNormalDist then
                                bestNormalDist = d
                                bestNormalMob = m
                            end
                        end
                    end
                end
            end
        end
    end
    scanContainer(workspace:FindFirstChild("Living"))
    local rl = workspace:FindFirstChild("RaidLevels")
    if rl then
        for _, room in ipairs(rl:GetChildren()) do
            if room:IsA("Folder") or room:IsA("Model") then
                scanContainer(room)
                local mobsF = room:FindFirstChild("Enemies") or room:FindFirstChild("Mobs")
                if mobsF then scanContainer(mobsF) end
            end
        end
    end
    if bestNormalMob then
        return bestNormalMob, bestNormalDist
    end
    return bestShieldedMob, bestShieldedDist
end

-- Breaks BossShieldPumpkin / Cursed Altar objects with Pickaxe if a boss's ShieldActive requires mining altars
function RaidEngine.BreakBossShieldPumpkin()
    if not State.HRP or not State.Humanoid or State.Humanoid.Health <= 0 then return false end
    local CS = game:GetService("CollectionService")
    local hrpPos = State.HRP.Position
    local bestObj, bestPart, bestDist = nil, nil, 320
    for _, obj in ipairs(CS:GetTagged("BossShieldPumpkin")) do
        if obj and obj.Parent and obj:IsDescendantOf(workspace) then
            local pt = obj:IsA("Model") and (obj.PrimaryPart or obj:FindFirstChild("Hitbox") or obj:FindFirstChildWhichIsA("BasePart")) or (obj:IsA("BasePart") and obj or nil)
            if pt then
                local d = (pt.Position - hrpPos).Magnitude
                if d < bestDist then
                    bestDist = d
                    bestObj = obj
                    bestPart = pt
                end
            end
        end
    end
    if not bestObj or not bestPart then return false end

    local held = State.Character and State.Character:FindFirstChildWhichIsA("Tool")
    if not held or held.Name ~= "Pickaxe" then
        local bpPick = LocalPlayer.Backpack and LocalPlayer.Backpack:FindFirstChild("Pickaxe")
        if bpPick and State.Humanoid then
            State.Humanoid:EquipTool(bpPick)
            task.wait(0.08)
        end
    end
    CombatEngine.ActiveStanceCF = nil
    if State.HRP.Anchored then State.HRP.Anchored = false end
    RaidEngine.StatusText = "Breaking Boss Shield Altar (" .. tostring(bestObj.Name) .. ")"
    local standPos = bestPart.Position + Vector3.new(0, NavEngine.GetCharacterStandOffset(), 3.2)
    local floorHit = NavEngine.RaycastSolidFloor(Vector3.new(bestPart.Position.X, bestPart.Position.Y + 6, bestPart.Position.Z + 3.2), Vector3.new(0, -25, 0))
    if floorHit then
        standPos = Vector3.new(floorHit.Position.X, floorHit.Position.Y + NavEngine.GetCharacterStandOffset(), floorHit.Position.Z)
    end
    if (State.HRP.Position - bestPart.Position).Magnitude > 9.0 then
        NavEngine.NavigateTo(CFrame.lookAt(standPos, bestPart.Position), false)
    end
    if Remotes.ToolActivated and (State.HRP.Position - bestPart.Position).Magnitude <= 12.0 then
        pcall(function() Remotes.ToolActivated:InvokeServer("Pickaxe", false) end)
    end
    return true
end

function RaidEngine.GetCorruptedHpInfo()
    if not State.Character or not State.Humanoid then
        return false, 0, 100
    end
    local maxHp = math.max(1, State.Humanoid.MaxHealth)
    local corruptAmt = 0
    for _, attrName in ipairs({ "BehemothCorruption", "Corruption", "FrozenHealth", "AntiHeal", "CurseCorruption" }) do
        local v = tonumber(State.Character:GetAttribute(attrName))
        if v and v > corruptAmt then
            corruptAmt = v
        end
    end
    local isCorrupt = corruptAmt > 1
    if not isCorrupt and PlayerGui then
        for _, hudName in ipairs({ "StatusGui", "StatusHud" }) do
            local hud = PlayerGui:FindFirstChild(hudName)
            local cor = hud and hud:FindFirstChild("Corrupt", true)
            if cor and cor:IsA("GuiObject") and cor.Visible and (cor.AbsoluteSize.X > 2 and cor.AbsoluteSize.Y > 2) then
                isCorrupt = true
                break
            end
        end
    end
    local effMax = math.max(1, maxHp - corruptAmt)
    return isCorrupt, corruptAmt, effMax
end

function RaidEngine.FindBestHealOrb(maxDist)
    if not State.HRP then return nil end
    maxDist = math.min(maxDist or 280, 320)
    local hrpPos = State.HRP.Position
    local now = os.clock()
    local bestPart, bestDist = nil, maxDist
    local skipped = RaidEngine.SkippedOrbs
    local function scanForOrbs(root)
        if not root then return end
        for _, ch in ipairs(root:GetChildren()) do
            if ch.Name == "HealOrb" or ch:GetAttribute("HealOrb") then
                local skUntil = skipped and skipped[ch]
                if not skUntil or now >= skUntil then
                    local p = ch:IsA("BasePart") and ch or (ch:FindFirstChild("Core") or ch:FindFirstChild("Shell") or ch:FindFirstChildWhichIsA("BasePart"))
                    if p and p.Parent and math.abs(p.Position.Y - hrpPos.Y) <= 190 then
                        local d = (p.Position - hrpPos).Magnitude
                        if d < bestDist then
                            bestDist = d
                            bestPart = p
                        end
                    end
                end
            end
        end
    end
    scanForOrbs(workspace)
    scanForOrbs(workspace:FindFirstChild("Debris"))
    scanForOrbs(workspace:FindFirstChild("AbyssDropsFX"))
    local rl = workspace:FindFirstChild("RaidLevels")
    if rl then
        for _, room in ipairs(rl:GetChildren()) do
            if room:IsA("Model") or room:IsA("Folder") then
                scanForOrbs(room)
            end
        end
    end
    return bestPart, bestDist
end

function RaidEngine.CollectHealOrb(orbPart)
    if not orbPart or not orbPart.Parent or not State.HRP or not State.Humanoid then return false end
    local orbRoot = (orbPart.Parent and orbPart.Parent.Name == "HealOrb") and orbPart.Parent or orbPart
    CombatEngine.ClearStanceLock()
    CombatEngine.LastEngagedMob = nil
    if State.HRP.Anchored then State.HRP.Anchored = false end
    State.Humanoid.AutoRotate = true
    local ok = NavEngine.NavigateTo(CFrame.new(orbPart.Position + Vector3.new(0, 1.0, 0)), false)
    if ok and State.HRP and orbPart.Parent then
        -- Broadphase nudge + short dwell so server Radius=5 orb pickup registers reliably
        State.HRP.CFrame = CFrame.new(orbPart.Position + Vector3.new(0, 0.8, 0))
        State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
        local t0 = os.clock()
        while orbRoot.Parent and (os.clock() - t0) < 0.35 do
            task.wait(0.05)
        end
    end
    if orbRoot.Parent then
        RaidEngine.SkippedOrbs = RaidEngine.SkippedOrbs or {}
        RaidEngine.SkippedOrbs[orbRoot] = os.clock() + 3.5
    end
    return not orbRoot.Parent or ok
end

function RaidEngine.HandleBossChestsAndGates()
    if not State.HRP or not State.Humanoid or not Config.AbyssAutoAdvanceGate then return false end
    local hrpPos = State.HRP.Position
    local now = os.clock()

    -- Check Boss Chests inside the current floor arena without scanning the entire workspace tree
    local containers = { workspace:FindFirstChild("Debris"), workspace:FindFirstChild("RaidLevels") }
    local topCandidates = {}
    for _, ch in ipairs(workspace:GetChildren()) do
        if (ch:IsA("Model") or ch:IsA("BasePart")) then
            local lname = string.lower(ch.Name)
            if lname:find("bosschest", 1, true) or lname:find("raidchest", 1, true) or lname == "chest" then
                table.insert(topCandidates, ch)
            end
        end
    end
    local function tryOpenChest(inst)
        if not inst or not (inst:IsA("Model") or inst:IsA("BasePart")) or RaidEngine.OpenedChests[inst] then return false end
        local lname = string.lower(inst.Name)
        if lname:find("bosschest", 1, true) or lname:find("raidchest", 1, true) or lname == "chest" then
            local pos = inst:IsA("BasePart") and inst.Position or (inst.PrimaryPart and inst.PrimaryPart.Position or inst:GetPivot().Position)
            if (pos - hrpPos).Magnitude <= 160 and math.abs(pos.Y - hrpPos.Y) <= 55 then
                local prompt = inst:FindFirstChildWhichIsA("ProximityPrompt", true)
                if prompt and prompt.Enabled then
                    if State.HRP.Anchored then State.HRP.Anchored = false end
                    if NavEngine.NavigateTo(CFrame.new(pos + Vector3.new(0, 2.5, 0)), false) then
                        local lastTrig = State.LastPromptTrigger[inst] or 0
                        if (now - lastTrig) >= 2.0 then
                            State.LastPromptTrigger[inst] = now
                            local trig = false
                            if typeof(fireproximityprompt) == "function" then
                                pcall(function() fireproximityprompt(prompt, 0); trig = true end)
                            end
                            if not trig then
                                pcall(function() prompt:InputHoldBegin(); task.wait(prompt.HoldDuration or 0.25); prompt:InputHoldEnd() end)
                            end
                            if Remotes.AbyssBossChestOpen then
                                pcall(function() Remotes.AbyssBossChestOpen:InvokeServer() end)
                            end
                            RaidEngine.OpenedChests[inst] = true
                            return true
                        end
                    end
                end
            end
        end
        return false
    end
    for _, topInst in ipairs(topCandidates) do
        if tryOpenChest(topInst) then return true end
    end
    for _, root in ipairs(containers) do
        if root then
            for _, inst in ipairs(root:GetDescendants()) do
                if tryOpenChest(inst) then return true end
            end
        end
    end

    return false
end

function RaidEngine.GetSafeRegenSpot()
    if not State.HRP then return nil, false end
    local hrpPos = State.HRP.Position

    -- Anchor height calculation to the arena floor beneath us so jumping/hovering doesn't compound altitude
    local floorRp = RaycastParams.new()
    floorRp.FilterType = Enum.RaycastFilterType.Exclude
    floorRp.FilterDescendantsInstances = {
        workspace:FindFirstChild("Living"),
        workspace:FindFirstChild("Debris"),
        State.Character or LocalPlayer.Character,
    }
    local floorHit = workspace:Raycast(hrpPos + Vector3.new(0, 2, 0), Vector3.new(0, -350, 0), floorRp)
    local baseFloorY = (floorHit and floorHit.Position and floorHit.Position.Y) or hrpPos.Y
    local targetTopY = math.max(hrpPos.Y + 80, baseFloorY + 125)
    local maxOuterCapY = nil

    -- Compute horizontal retreat coordinates away from the nearest enemy while staying inside current room bounds
    local safeX, safeZ = hrpPos.X, hrpPos.Z
    local enemyMob = RaidEngine.FindClosestRaidEnemy()
    local eHrp = enemyMob and (enemyMob:FindFirstChild("HumanoidRootPart") or enemyMob:FindFirstChildWhichIsA("BasePart"))
    if eHrp then
        local away = Vector3.new(hrpPos.X - eHrp.Position.X, 0, hrpPos.Z - eHrp.Position.Z)
        local dir = (away.Magnitude > 0.5) and away.Unit or Vector3.new(1, 0, 0)
        safeX = eHrp.Position.X + dir.X * 135
        safeZ = eHrp.Position.Z + dir.Z * 135
    end

    -- Scan current RaidLevels room walls/shell so player rises well above arena walls/inner ceiling while staying inside outer seal shell
    local rl = workspace:FindFirstChild("RaidLevels")
    if rl then
        local bestRoom, bestDist, bestCF, bestSz = nil, 650, nil, nil
        for _, rm in ipairs(rl:GetChildren()) do
            if rm:IsA("Model") or rm:IsA("Folder") then
                local okBox, cf, sz = pcall(function() return rm:GetBoundingBox() end)
                if okBox and cf then
                    local d = (cf.Position - hrpPos).Magnitude
                    if d < bestDist then
                        bestDist = d
                        bestRoom = rm
                        bestCF = cf
                        bestSz = sz
                    end
                end
            end
        end
        if bestRoom then
            if bestCF and bestSz then
                local marginX = math.max(30, math.min(bestSz.X * 0.42, 135))
                local marginZ = math.max(30, math.min(bestSz.Z * 0.42, 135))
                safeX = math.clamp(safeX, bestCF.Position.X - marginX, bestCF.Position.X + marginX)
                safeZ = math.clamp(safeZ, bestCF.Position.Z - marginZ, bestCF.Position.Z + marginZ)
            end
            local maxWallTopY = -math.huge
            for _, d in ipairs(bestRoom:GetDescendants()) do
                if d:IsA("BasePart") then
                    local nm = d.Name
                    local pNm = d.Parent and d.Parent.Name or ""
                    if nm:find("Seal", 1, true) or pNm:find("Seal", 1, true) or nm:find("ShaftCap", 1, true) then
                        local sy = d.Position.Y - (d.Size.Y * 0.5)
                        if sy > baseFloorY + 60 and (not maxOuterCapY or sy < maxOuterCapY) then
                            maxOuterCapY = sy
                        end
                    elseif nm:find("Wall", 1, true) or pNm:find("Wall", 1, true) or pNm == "InvisibleWalls" then
                        local ty = d.Position.Y + (d.Size.Y * 0.5)
                        if ty > maxWallTopY and ty < baseFloorY + 260 then
                            maxWallTopY = ty
                        end
                    end
                end
            end
            if maxWallTopY > -math.huge then
                targetTopY = math.max(targetTopY, maxWallTopY + 38.0)
            end
        end
    else
        local arena, aCF, aSz = RaidEngine.GetActiveBossArena()
        if arena and aCF and aSz then
            local marginX = math.max(25, math.min(aSz.X * 0.42, 110))
            local marginZ = math.max(25, math.min(aSz.Z * 0.42, 110))
            safeX = math.clamp(safeX, aCF.Position.X - marginX, aCF.Position.X + marginX)
            safeZ = math.clamp(safeZ, aCF.Position.Z - marginZ, aCF.Position.Z + marginZ)
            maxOuterCapY = aCF.Position.Y + (aSz.Y * 0.48)
        end
    end

    local safeY = targetTopY
    if maxOuterCapY then
        safeY = math.min(safeY, maxOuterCapY - 6.0)
    end
    safeY = math.max(safeY, hrpPos.Y + 55)
    return CFrame.new(safeX, safeY, safeZ), false
end

function RaidEngine.LeaveRun()
    RaidEngine.LastLeaveAttempt = os.clock()
    RaidEngine.RecoveringHP = false
    RaidEngine.SafeRegenCF = nil
    CombatEngine.ActiveStanceCF = nil
    if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
    if State.Humanoid then State.Humanoid.AutoRotate = true end
    if RaidEngine.IsInActiveRun() then
        if Remotes.AbyssLeave then
            pcall(function() return Remotes.AbyssLeave:InvokeServer() end)
        end
        if Remotes.RaidGoToIsland then
            pcall(function() return Remotes.RaidGoToIsland:InvokeServer() end)
        end
    elseif Remotes.PartyLeave then
        pcall(function() return Remotes.PartyLeave:InvokeServer() end)
    end
end

function RaidEngine.FindPartyPadForMode(mode)
    mode = RaidEngine.NormalizeMode(mode)
    local CS = game:GetService("CollectionService")
    local function extract(inst)
        local stand = inst:IsA("ProximityPrompt") and inst.Parent or inst
        local prompt = inst:IsA("ProximityPrompt") and inst or (stand and stand:FindFirstChildWhichIsA("ProximityPrompt", true))
        local pos = nil
        if stand then
            if stand:IsA("BasePart") then pos = stand.Position
            elseif stand:IsA("Model") then pos = stand:GetPivot().Position end
        end
        local maxDist = (prompt and prompt:IsA("ProximityPrompt") and tonumber(prompt.MaxActivationDistance)) or 6
        return stand, pos, prompt, maxDist
    end
    for _, v in ipairs(CS:GetTagged("PartyLocation")) do
        if v:GetAttribute("PartyType") == mode then
            local stand, pos, prompt, maxDist = extract(v)
            if pos then return stand, pos, prompt, maxDist end
        end
    end
    local prox = workspace:FindFirstChild("Proximity")
    if prox then
        for _, d in ipairs(prox:GetDescendants()) do
            if d:GetAttribute("PartyType") == mode then
                local stand, pos, prompt, maxDist = extract(d)
                if pos then return stand, pos, prompt, maxDist end
            end
        end
    end
    return nil
end


function RaidEngine.MoveToPartySpot(padPos, maxDist, modeLabel, isAuto)
    if not padPos or not State.HRP or not State.Humanoid then return false end
    if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false end
    local safeRadius = math.max(3.0, math.min(maxDist or 6, 8) - 2.0)
    local dist = (padPos - State.HRP.Position).Magnitude
    -- Never blindly tween thousands of studs across ocean/void from Boss Arena back to Island
    if dist > 2200 then
        return false
    end
    if dist > safeRadius then
        if State.HRP.Anchored then State.HRP.Anchored = false end
        State.Humanoid.AutoRotate = true
        RaidEngine.StatusText = "Gliding to " .. tostring(modeLabel or "Party") .. " Spot..."
        local ok = NavEngine.NavigateTo(CFrame.new(padPos + Vector3.new(0, 2.0, 0)), not isAuto)
        if not ok or not State.HRP then return false end
    end
    if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false end
    -- Settle right on the pad so server proximity validation always passes
    if State.HRP.Anchored then State.HRP.Anchored = false end
    State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
    task.wait(0.25)
    if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false end
    return State.HRP and (padPos - State.HRP.Position).Magnitude <= math.max(safeRadius + 3.0, (maxDist or 6) + 1.0)
end

function RaidEngine.CreateAndStartParty(mode, tier, activateNow, isAuto)
    mode = RaidEngine.NormalizeMode(mode or Config.RaidMode or "Abyss")
    tier = tonumber(tier or Config.RaidTier) or 1
    if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then
        return false, "Idle"
    end
    local now = os.clock()
    if (now - (RaidEngine.LastPartyAction or 0)) < 2.8 then
        return false, RaidEngine.StatusText or "Preparing Party..."
    end
    RaidEngine.LastPartyAction = now

    if RaidEngine.IsInActiveRun() then return false, "Already in Run" end

    if mode == "Abyss" and not RaidEngine.IsInAbyssLobby() then
        if Remotes.AbyssEnter and (now - (RaidEngine.LastEnterAttempt or 0)) >= 5.0 then
            RaidEngine.LastEnterAttempt = now
            pcall(function() return Remotes.AbyssEnter:InvokeServer() end)
            return true, "Entering Abyss Lobby..."
        end
        return false, "Wait AbyssEnter"
    elseif mode ~= "Abyss" and RaidEngine.IsInAbyssLobby() then
        local prox = workspace:FindFirstChild("Proximity")
        local retStand = prox and prox:FindFirstChild("AbyssReturn")
        if retStand and State.HRP then
            local retPos = retStand:IsA("BasePart") and retStand.Position or retStand:GetPivot().Position
            local retPrompt = retStand:FindFirstChildWhichIsA("ProximityPrompt", true)
            if RaidEngine.MoveToPartySpot(retPos, 8, "Abyss Exit", isAuto) and retPrompt then
                if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Stopped" end
                if typeof(fireproximityprompt) == "function" then
                    pcall(function() fireproximityprompt(retPrompt, 0) end)
                else
                    pcall(function() retPrompt:InputHoldBegin(); task.wait(0.25); retPrompt:InputHoldEnd() end)
                end
                return true, "Returning to Island from Abyss Lobby..."
            end
        end
        return false, "In Abyss Lobby (leave to island first)"
    end

    local pState = RaidEngine.GetPartyState()
    if pState.inParty and pState.partyType and pState.partyType ~= mode then
        if Remotes.PartyLeave then pcall(function() Remotes.PartyLeave:InvokeServer() end) end
        task.wait(0.35)
        if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Stopped" end
        pState = RaidEngine.GetPartyState()
    end

    local _, padPos, padPrompt, padMaxDist = RaidEngine.FindPartyPadForMode(mode)
    if padPos and State.HRP and (padPos - State.HRP.Position).Magnitude > 2200 then
        -- Still stranded out at Boss Arena coords (e.g. Phase 2 void kill or waiting server teleport back)
        RaidEngine.StrandedSince = RaidEngine.StrandedSince or os.clock()
        local strandedElapsed = os.clock() - RaidEngine.StrandedSince
        if strandedElapsed < 5.5 then
            return false, string.format("Waiting Return Teleport (%.1fs)", 5.5 - strandedElapsed)
        elseif strandedElapsed < 7.5 then
            if Remotes.RaidGoToIsland then pcall(function() return Remotes.RaidGoToIsland:InvokeServer() end) end
            if Remotes.PartyLeave then pcall(function() return Remotes.PartyLeave:InvokeServer() end) end
            return false, "Trying Server Exit..."
        else
            RaidEngine.StrandedSince = nil
            RaidEngine.Notify("Asura Boss Arena", "Server return teleport missed -> Respawning back to island!", 3.5)
            pcall(function() CombatEngine.ResetCharacterNow("Stranded in Boss Arena") end)
            return false, "Respawning on Island..."
        end
    end
    RaidEngine.StrandedSince = nil

    -- Pre-run Health Potion restock in Abyss Lobby / Island before walking to the gate and starting
    if Config.RaidAutoBuyHealthPots
        and not RaidEngine.IsInActiveRun()
        and not RaidEngine.GetActiveBossArena()
        and not RaidEngine.IsNearActiveBossMob(450)
        and os.clock() >= (RaidEngine.ArenaLoadingUntil or 0)
        and (not RaidEngine.LastBossArenaAt or (os.clock() - RaidEngine.LastBossArenaAt) >= 8.0)
        and (not RaidEngine.LastPartyActivateAt or (os.clock() - RaidEngine.LastPartyActivateAt) >= 15.0)
        and State.PotionEngine
        and type(State.PotionEngine.EnsureHealthPotionsStock) == "function" then
        local bought = State.PotionEngine.EnsureHealthPotionsStock(Config.RaidHealthPotStock or 5)
        if bought > 0 then
            RaidEngine.StatusText = string.format("Bought Health Potions x%d", bought)
            task.wait(0.2)
            if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Stopped" end
        end
    end

    if (not pState.inParty or activateNow) and padPos then
        local atSpot = RaidEngine.MoveToPartySpot(padPos, padMaxDist, mode, isAuto)
        if not atSpot then
            if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Idle" end
            return false, "Moving to " .. tostring(mode) .. " spot..."
        end
    end

    if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Stopped" end

    if not pState.inParty and Remotes.PartyCreate then
        pcall(function() return Remotes.PartyCreate:InvokeServer(mode) end)
        task.wait(0.35)
        if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Stopped" end
        pState = RaidEngine.GetPartyState()
        if not pState.inParty and padPrompt then
            if typeof(fireproximityprompt) == "function" then
                pcall(function() fireproximityprompt(padPrompt, 0) end)
            end
            task.wait(0.35)
            if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Stopped" end
            pState = RaidEngine.GetPartyState()
        end
    end

    local bossInfo = RaidEngine.GetBossWindowInfo(mode)
    if bossInfo and not bossInfo.open then
        return false, string.format("Waiting for %s (%ds)", tostring(mode), math.ceil(bossInfo.remainingSec or 0))
    end

    if pState.inParty and mode ~= "Abyss" and not bossInfo and Remotes.PartySetTier and pState.tier ~= tier then
        pcall(function() return Remotes.PartySetTier:InvokeServer(tier) end)
        task.wait(0.25)
        if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Stopped" end
    end

    if activateNow and pState.inParty and pState.isOwner ~= false and Remotes.PartyActivate then
        -- Re-confirm we are physically at the party spot right before Activate()
        if padPos and not RaidEngine.MoveToPartySpot(padPos, padMaxDist, mode, isAuto) then
            if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Stopped" end
            return false, "Aligning at party spot..."
        end
        if isAuto and (not Config.AutoRaid or not Config.RaidAutoCreateStart) then return false, "Stopped" end
        RaidEngine.LastPartyActivateAt = os.clock()
        local okCall, res = pcall(function() return Remotes.PartyActivate:InvokeServer() end)
        return okCall and (res ~= false), (okCall and res ~= false) and ("Starting " .. tostring(mode) .. "...") or "Waiting at Party Spot..."
    end
    return pState.inParty, pState.inParty and ("Party Ready: " .. tostring(mode)) or "Waiting Party"
end

-- Event hooks for Abyss & Raid lifecycle (Tier 2 raw RE, cleanup tracked automatically)
do
    if Remotes.AbyssCardOffer and Remotes.AbyssCardOffer:IsA("RemoteEvent") then
        trackConn(Remotes.AbyssCardOffer.OnClientEvent:Connect(function(offer)
            if not ScriptRunning or not Config.AutoPickAbyssCards then return end
            task.spawn(function()
                task.wait(0.18)
                if ScriptRunning and Config.AutoPickAbyssCards then
                    RaidEngine.HandleCardOffer(offer)
                end
            end)
        end))
    end
    if Remotes.AbyssCardPicked and Remotes.AbyssCardPicked:IsA("RemoteEvent") then
        trackConn(Remotes.AbyssCardPicked.OnClientEvent:Connect(function(cardId, pickerName)
            local idNum = tonumber(cardId) or cardId
            if idNum and (pickerName == nil or pickerName == LocalPlayer.Name) then
                local normNum = tonumber(idNum) or RaidEngine.CardNumById[idNum]
                RaidEngine.OwnedCards[idNum] = (RaidEngine.OwnedCards[idNum] or 0) + 1
                if normNum and normNum ~= idNum then
                    RaidEngine.OwnedCards[normNum] = (RaidEngine.OwnedCards[normNum] or 0) + 1
                end
            end
        end))
    end
    if Remotes.AbyssProgress and Remotes.AbyssProgress:IsA("RemoteEvent") then
        trackConn(Remotes.AbyssProgress.OnClientEvent:Connect(function(prog)
            CombatEngine.ClearStanceLock()
            CombatEngine.LastEngagedMob = nil
            if type(prog) == "table" and tonumber(prog.Floor) then
                RaidEngine.CurrentFloor = tonumber(prog.Floor)
            elseif tonumber(prog) then
                RaidEngine.CurrentFloor = tonumber(prog)
            end
            if ScriptRunning and Config.AutoRaid and (Config.AbyssStopFloor or 0) > 0 and (RaidEngine.CurrentFloor or 0) >= Config.AbyssStopFloor then
                if (os.clock() - (RaidEngine.LastLeaveAttempt or 0)) >= 4.0 then
                    RaidEngine.Notify("Auto Abyss", "Reached Target Floor " .. tostring(RaidEngine.CurrentFloor) .. " -> Leaving run!", 3.5)
                    task.spawn(RaidEngine.LeaveRun)
                end
            end
        end))
    end
    if Remotes.RaidCurrentWave and Remotes.RaidCurrentWave:IsA("RemoteEvent") then
        trackConn(Remotes.RaidCurrentWave.OnClientEvent:Connect(function(w)
            if tonumber(w) then RaidEngine.CurrentWave = tonumber(w) end
            RaidEngine.IsEnded = false
            CombatEngine.ClearStanceLock()
            CombatEngine.LastEngagedMob = nil
        end))
    end
    if Remotes.RaidEnemiesLeft and Remotes.RaidEnemiesLeft:IsA("RemoteEvent") then
        trackConn(Remotes.RaidEnemiesLeft.OnClientEvent:Connect(function(cnt)
            if tonumber(cnt) then RaidEngine.EnemiesLeft = tonumber(cnt) end
        end))
    end
    if Remotes.RaidReviveOffer and Remotes.RaidReviveOffer:IsA("RemoteEvent") then
        trackConn(Remotes.RaidReviveOffer.OnClientEvent:Connect(function()
            if not ScriptRunning or not Config.AutoRaid or not Config.AutoRaidRevive then return end
            local now = os.clock()
            if (now - (RaidEngine.LastReviveAttempt or 0)) < 1.5 then return end
            RaidEngine.LastReviveAttempt = now
            task.spawn(function()
                task.wait(0.25)
                if ScriptRunning and Config.AutoRaid and Config.AutoRaidRevive and Remotes.RaidRevive then
                    local ok = pcall(function() return Remotes.RaidRevive:InvokeServer() end)
                    if ok then RaidEngine.Notify("Auto Raid", "Auto-Revived!", 2.5) end
                end
            end)
        end))
    end
    if Remotes.BossArenaLoading and Remotes.BossArenaLoading:IsA("RemoteEvent") then
        trackConn(Remotes.BossArenaLoading.OnClientEvent:Connect(function()
            RaidEngine.ArenaLoadingUntil = os.clock() + 15.0
            if State.IsBuyingHealthPotions and NavEngine then
                NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
            end
        end))
    end
    if Remotes.RaidEnded and Remotes.RaidEnded:IsA("RemoteEvent") then
        trackConn(Remotes.RaidEnded.OnClientEvent:Connect(function(data)
            RaidEngine.IsEnded = true
            RaidEngine.ArenaLoadingUntil = 0
            if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
            if State.Humanoid then State.Humanoid.AutoRotate = true end
            if Config.WebhookEnabled and Config.WebhookNotifyRaidRewards and WebhookEngine and type(WebhookEngine.SendRaidRewardsReport) == "function" then
                task.spawn(function()
                    task.wait(0.65) -- Allow server chest/run rewards to replicate into PlayerData
                    WebhookEngine.SendRaidRewardsReport(data)
                end)
            end
            if not ScriptRunning or not Config.AutoRaid then return end
            local noRetry = type(data) == "table" and type(data.Summary) == "table" and data.Summary.NoRetry
            local isLeader = type(data) ~= "table" or data.Leader == nil or data.Leader == LocalPlayer.UserId
            task.spawn(function()
                task.wait(0.85)
                if not ScriptRunning or not Config.AutoRaid then return end
                if Config.AutoAsuraToTower then
                    local tCount = RaidEngine.GetRaidTicketCount()
                    if tCount >= 1 and Config.AutoRaidRetry and not noRetry and isLeader and Remotes.RaidRetry then
                        pcall(function() return Remotes.RaidRetry:InvokeServer() end)
                        RaidEngine.Notify("Auto Raid", string.format("Run ended (%d ticket%s left) -> Retrying Tower!", tCount, tCount == 1 and "" or "s"), 3.0)
                    elseif Remotes.RaidGoToIsland then
                        pcall(function() return Remotes.RaidGoToIsland:InvokeServer() end)
                        RaidEngine.Notify("Auto Raid", "Out of Raid Tickets -> Returning to Island to farm Asura!", 3.0)
                    end
                elseif Config.AutoRaidRetry and not noRetry and isLeader and Remotes.RaidRetry then
                    pcall(function() return Remotes.RaidRetry:InvokeServer() end)
                    RaidEngine.Notify("Auto Raid", "Run ended -> Retrying!", 3.0)
                elseif not Config.AutoRaidRetry and Remotes.RaidGoToIsland then
                    pcall(function() return Remotes.RaidGoToIsland:InvokeServer() end)
                end
            end)
        end))
    end
    if Remotes.RaidRestarted and Remotes.RaidRestarted:IsA("RemoteEvent") then
        trackConn(Remotes.RaidRestarted.OnClientEvent:Connect(function()
            RaidEngine.IsEnded = false
            RaidEngine.CurrentWave = 0
            RaidEngine.CurrentFloor = 0
            table.clear(RaidEngine.CrossedGates)
            table.clear(RaidEngine.OpenedChests)
            table.clear(RaidEngine.OwnedCards)
            table.clear(RaidEngine.BlacklistedMobs)
            if WebhookEngine and type(WebhookEngine.CaptureRaidSnapshot) == "function" then
                task.spawn(function()
                    task.wait(0.2)
                    WebhookEngine.CaptureRaidSnapshot(true)
                end)
            end
        end))
    end
end

-- Main Raid / Abyss Automation Loop
task.spawn(function()
    local lastStatusTick = 0
    while ScriptRunning do
        local loopDelay = 0.20
        local inRun = RaidEngine.IsInActiveRun()
        local inLobby = RaidEngine.IsInAbyssLobby()
        State.IsInRaid = (Config.AutoRaid and inRun) or false

        if Config.AutoRaid then
            if inRun then
                loopDelay = 0.10
                local activeArena = RaidEngine.GetActiveBossArena()
                local skipRoot = activeArena or (workspace:FindFirstChild("Assets") and workspace.Assets:FindFirstChild("BossArenas"))
                if skipRoot then
                    local reSkip = skipRoot:FindFirstChild("RE_SkipCutscene", true)
                    local skipParent = reSkip and reSkip.Parent or activeArena
                    local skipVal = skipParent and skipParent:FindFirstChild("SkipCutscene")
                    if reSkip and reSkip:IsA("RemoteEvent") and (not skipVal or skipVal.Value == false) then
                        if (os.clock() - (RaidEngine.LastBossCutsceneSkip or 0)) >= 0.35 then
                            RaidEngine.LastBossCutsceneSkip = os.clock()
                            pcall(function() reSkip:FireServer() end)
                        end
                    end
                end
                if activeArena then
                    pcall(NavEngine.PreventBossArenaVoidFall)
                end
                if (os.clock() - lastStatusTick) >= 0.6 then
                    lastStatusTick = os.clock()
                    if activeArena then
                        local bossMob = RaidEngine.FindClosestRaidEnemy()
                        local bHum = bossMob and bossMob:FindFirstChildOfClass("Humanoid")
                        local hpTxt = (bHum and bHum.MaxHealth > 0)
                            and string.format(" | Boss HP: %.0f%%", (bHum.Health / bHum.MaxHealth) * 100)
                            or ""
                        local skillTxt = CombatEngine.ActiveSkillName
                            and (" | EVADING: " .. tostring(CombatEngine.ActiveSkillName))
                            or ""
                        RaidEngine.StatusText = string.format("Boss Arena (%s)%s%s", tostring(activeArena.Name), hpTxt, skillTxt)
                    else
                        RaidEngine.StatusText = string.format("In Run | Floor %d | Wave %d%s",
                            RaidEngine.CurrentFloor or 0,
                            RaidEngine.CurrentWave or 0,
                            RaidEngine.EnemiesLeft and (" | Enemies: " .. tostring(RaidEngine.EnemiesLeft)) or "")
                    end
                end

                if (Config.AbyssStopFloor or 0) > 0 and (RaidEngine.CurrentFloor or 0) >= Config.AbyssStopFloor then
                    if (os.clock() - (RaidEngine.LastLeaveAttempt or 0)) >= 4.0 then
                        RaidEngine.StatusText = "Leaving at Floor " .. tostring(RaidEngine.CurrentFloor) .. "..."
                        RaidEngine.LeaveRun()
                    end
                    task.wait(0.4)
                    continue
                end

                if State.HRP and State.Humanoid and State.Humanoid.Health > 0 and not State.IsAutoForging and not State.IsCrafting and not State.IsBuyingPotions then
                    local isCorrupted, _, effectiveMaxHp = RaidEngine.GetCorruptedHpInfo()
                    local curHp = State.Humanoid.Health
                    local maxHp = math.max(1, State.Humanoid.MaxHealth)
                    local hpPct = (curHp / maxHp) * 100
                    local effHpPct = (curHp / effectiveMaxHp) * 100

                    -- 1. PRIORITY: Collect HealOrb in the room FIRST whenever healable below threshold (even before Low-HP sky retreat!)
                    local collectedOrb = false
                    local needsOrbHeal = (curHp < effectiveMaxHp - 25) and (effHpPct <= (Config.AbyssHealOrbHP or 75))
                    if Config.AbyssCollectHealOrbs and needsOrbHeal then
                        local orb = RaidEngine.FindBestHealOrb(280)
                        if orb then
                            RaidEngine.RecoveringHP = false
                            RaidEngine.SafeRegenCF = nil
                            RaidEngine.StatusText = string.format("Collecting HealOrb (HP %.0f%%)", effHpPct)
                            collectedOrb = RaidEngine.CollectHealOrb(orb)
                            curHp = State.Humanoid.Health
                            hpPct = (curHp / maxHp) * 100
                            effHpPct = (curHp / effectiveMaxHp) * 100
                        end
                    end

                    -- 2. If Boss applied purple/frozen HP corruption (e.g. Galaxite Behemoth), skip sky regen and fight the boss directly!
                    if isCorrupted then
                        if RaidEngine.RecoveringHP then
                            RaidEngine.RecoveringHP = false
                            RaidEngine.SafeRegenCF = nil
                            if State.HRP and State.HRP.Anchored then
                                State.HRP.Anchored = false
                                State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
                            end
                            if State.Humanoid then State.Humanoid.AutoRotate = true end
                            if not Config.Noclip and not NavEngine.IsGliding then
                                NavEngine.RestoreCharacterPhysics()
                            end
                            CombatEngine.EnsureWeaponEquipped()
                        end
                    elseif Config.RaidLowHpFallback then
                        -- 3. Normal Low-HP Fallback Regen: rise unanchored to top/far side of zone + drink Health Potions until recovered
                        local enterHp = math.clamp(tonumber(Config.RaidFallbackEnterHP) or 35, 5, 90)
                        local resumeHp = math.max(enterHp + 10, math.clamp(tonumber(Config.RaidFallbackResumeHP) or 85, 20, 100))

                        if not RaidEngine.RecoveringHP and hpPct <= enterHp then
                            RaidEngine.RecoveringHP = true
                            RaidEngine.SafeRegenCF = select(1, RaidEngine.GetSafeRegenSpot())
                        elseif RaidEngine.RecoveringHP and hpPct >= resumeHp then
                            RaidEngine.RecoveringHP = false
                            RaidEngine.SafeRegenCF = nil
                            if State.HRP and State.HRP.Anchored then
                                State.HRP.Anchored = false
                                State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
                            end
                            if State.Humanoid then State.Humanoid.AutoRotate = true end
                            if not Config.Noclip and not NavEngine.IsGliding then
                                NavEngine.RestoreCharacterPhysics()
                            end
                            CombatEngine.EnsureWeaponEquipped()
                        end

                        if RaidEngine.RecoveringHP then
                            CombatEngine.ClearStanceLock()
                            CombatEngine.LastEngagedMob = nil
                            local safeCF = RaidEngine.SafeRegenCF
                            if not safeCF or (safeCF.Position - State.HRP.Position).Magnitude > 350 then
                                safeCF = select(1, RaidEngine.GetSafeRegenSpot())
                                RaidEngine.SafeRegenCF = safeCF
                            end
                            local PE = State.PotionEngine
                            local potStr = PE and (" | Pots: " .. tostring(PE.GetTotalHealthPotionsOwned())) or ""
                            RaidEngine.StatusText = string.format("Top Regen (HP %.0f%% -> %.0f%%%s)", hpPct, resumeHp, potStr)

                            if safeCF and State.HRP then
                                local riseDist = (safeCF.Position - State.HRP.Position).Magnitude
                                if riseDist > 4.0 then
                                    -- Fly unanchored with live physics velocity so Roblox replicates our new high/far position to the server
                                    State.Humanoid.AutoRotate = true
                                    if State.HRP.Anchored then
                                        State.HRP.Anchored = false
                                    end
                                    local riseSpeed = math.max(Config.GlideSpeed or 55, 75)
                                    local riseStart = os.clock()
                                    local curP = State.HRP.Position
                                    while ScriptRunning and State.HRP and (os.clock() - riseStart) < 4.5 do
                                        local dt = RunService.Heartbeat:Wait()
                                        if not State.HRP or not Config.AutoRaid or not Config.RaidLowHpFallback then break end
                                        if State.Character then
                                            for _, pt in ipairs(State.Character:GetDescendants()) do
                                                if pt:IsA("BasePart") and pt.CanCollide then pt.CanCollide = false end
                                            end
                                        end
                                        if (State.HRP.Position - curP).Magnitude > 18 then
                                            curP = State.HRP.Position
                                        end
                                        local delta = safeCF.Position - curP
                                        local dMag = delta.Magnitude
                                        if dMag <= 3.0 then
                                            curP = safeCF.Position
                                            State.HRP.CFrame = safeCF
                                            State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
                                            break
                                        end
                                        local vel = delta.Unit * math.min(riseSpeed, math.max(25, dMag * 8))
                                        curP = curP + delta.Unit * math.min(dMag, riseSpeed * dt)
                                        State.HRP.CFrame = CFrame.new(curP) * safeCF.Rotation
                                        State.HRP.AssemblyLinearVelocity = vel
                                        State.HRP.AssemblyAngularVelocity = Vector3.zero
                                    end
                                    if State.HRP then
                                        State.HRP.CFrame = safeCF
                                        State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
                                        task.wait(0.06) -- Broadphase/network sync before anchoring
                                    end
                                end
                                if State.HRP then
                                    State.Humanoid.AutoRotate = false
                                    State.HRP.CFrame = safeCF
                                    State.HRP.Anchored = true
                                    State.HRP.AssemblyLinearVelocity = Vector3.zero
                                    State.HRP.AssemblyAngularVelocity = Vector3.zero
                                end
                            end

                            if PE and type(PE.UseBestHealthPotion) == "function" then
                                PE.UseBestHealthPotion()
                            end
                            task.wait(0.25)
                            continue
                        end
                    else
                        RaidEngine.RecoveringHP = false
                        RaidEngine.SafeRegenCF = nil
                    end

                    -- 4. Fight nearest Raid / Abyss enemy (prioritizing minions whenever Boss is shielded/immune!)
                    if not collectedOrb then
                        local enemy = RaidEngine.FindClosestRaidEnemy()
                        if enemy and CombatEngine.IsBossShielded(enemy) and RaidEngine.BreakBossShieldPumpkin() then
                            loopDelay = 0.08
                        elseif enemy then
                            local ok, atkDelay = CombatEngine.EngageTargetMob(enemy, { blacklistOnFail = false })
                            if not ok then
                                RaidEngine.BlacklistedMobs[enemy] = os.clock() + 4.0
                            else
                                loopDelay = atkDelay or 0.08
                            end
                        elseif not RaidEngine.BreakBossShieldPumpkin() then
                            -- 5. Wave transition: release hover lock so server wave/floor teleports are never rubberbanded back to Wave 1
                            CombatEngine.ClearStanceLock()
                            CombatEngine.LastEngagedMob = nil
                            local didUtilityMove = false
                            if Config.AbyssCollectHealOrbs and curHp < (effectiveMaxHp - 25) and effHpPct < 95 then
                                local orb = RaidEngine.FindBestHealOrb(280)
                                if orb then
                                    RaidEngine.CollectHealOrb(orb)
                                    didUtilityMove = true
                                    task.wait(0.08)
                                end
                            end
                            if not didUtilityMove then
                                if RaidEngine.HandleBossChestsAndGates() then
                                    didUtilityMove = true
                                end
                            end
                            if not didUtilityMove and Config.MobFarmPositionMode == "Behind Enemy" then
                                if State.HRP.Anchored then State.HRP.Anchored = false end
                                if State.Humanoid then State.Humanoid.AutoRotate = true end
                            end
                        end
                    end
                elseif State.Humanoid and State.Humanoid.Health <= 0 then
                    RaidEngine.RecoveringHP = false
                    CombatEngine.LastEngagedMob = nil
                    if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
                    RaidEngine.StatusText = Config.AutoRaidRevive and "Downed - Waiting Revive..." or "Downed"
                end
            else
                RaidEngine.RecoveringHP = false
                CombatEngine.LastEngagedMob = nil
                if State.IsInRaid then State.IsInRaid = false end
                if Config.RaidAutoCreateStart and not State.IsAutoForging and not State.IsCrafting and not State.IsBuyingPotions then
                    local effMode, curTickets, targetTickets = RaidEngine.ResolveEffectiveMode()
                    if RaidEngine.NormalizeMode(Config.RaidMode) ~= effMode then
                        Config.RaidMode = effMode
                        if RaidEngine.ModeDropdown and RaidEngine.ModeDropdown.SetValue then
                            pcall(function() RaidEngine.ModeDropdown:SetValue(RaidEngine.FormatModeLabel(effMode), true) end)
                        end
                    end
                    local _, msg = RaidEngine.CreateAndStartParty(effMode, Config.RaidTier, true, true)
                    if Config.AutoAsuraToTower then
                        local prefix = (effMode == "Tower Of Hell")
                            and string.format("Tickets %d/%d -> Tower", curTickets or 0, targetTickets or 5)
                            or string.format("Asura (%d/%d Tickets)", curTickets or 0, targetTickets or 5)
                        RaidEngine.StatusText = prefix .. " | " .. tostring(msg or "Preparing...")
                    else
                        RaidEngine.StatusText = msg or (inLobby and "In Abyss Lobby (Auto-Starting...)" or "On Island (Auto-Entering...)")
                    end
                    loopDelay = 1.2
                else
                    RaidEngine.StatusText = inLobby and "In Abyss Lobby (Standby)" or "Idle on Island"
                    loopDelay = 0.75
                end
            end
        else
            if State.IsInRaid or not Config.AutoMobFarm then
                CombatEngine.ActiveStanceCF = nil
            end
            State.IsInRaid = false
            State.IsBuyingHealthPotions = false
            RaidEngine.StatusText = inRun and "Standby (In Active Run)" or (inLobby and "Standby (In Abyss Lobby)" or "Idle")
            loopDelay = 0.6
        end

        task.wait(loopDelay)
    end
end)

-- ===================================================================
-- 11. CHARACTER & UTILITY HOOKS (WalkSpeed & Movement)
-- ===================================================================
do
    -- Hook MovementFeel.ApplySpeed so CustomWalkSpeed survives CharacterController's per-frame WalkSpeed writes
    task.spawn(function()
        pcall(function()
        local mfMod = ReplicatedStorage:FindFirstChild("Shared") and ReplicatedStorage.Shared:FindFirstChild("MovementFeel")
        if mfMod and mfMod:IsA("ModuleScript") then
            local mf = require(mfMod)
            if type(mf) == "table" and type(mf.ApplySpeed) == "function" then
                safeHook(mf, "ApplySpeed", "MovementFeelApplySpeed", function(speed, dt, dashing)
                    local adjusted = speed
                    if ScriptRunning and Config.CustomWalkSpeed and type(speed) == "number" and speed > 0 then
                        adjusted = math.max(speed, Config.WalkSpeedValue or 28)
                    end
                    return OrigHooks["MovementFeelApplySpeed"].orig(adjusted, dt, dashing)
                end)
            end
        end
        end)
    end)

    task.spawn(function()
        while ScriptRunning do
            if Config.CustomWalkSpeed and State.Humanoid and State.Humanoid.WalkSpeed ~= Config.WalkSpeedValue then
                State.Humanoid.WalkSpeed = Config.WalkSpeedValue
            end
            task.wait(0.25)
        end
    end)

    -- Anti-AFK (disables external LocalPlayer.Idled connections via getconnections + VirtualUser fallback)
    local ourIdledFn = nil
    local disabledIdledConns = {}
    local function applyAntiAFK(enable)
        if enable then
            if LocalPlayer and typeof(getconnections) == "function" then
                pcall(function()
                    for _, conn in ipairs(getconnections(LocalPlayer.Idled)) do
                        local isOurs = false
                        if ourIdledFn ~= nil then
                            local okFn, same = pcall(function() return conn.Function == ourIdledFn end)
                            if okFn and same then isOurs = true end
                        end
                        if not isOurs and type(conn.Disable) == "function" then
                            local ok = pcall(function() conn:Disable() end)
                            if ok then
                                disabledIdledConns[conn] = true
                            end
                        end
                    end
                end)
            end
        else
            for conn in pairs(disabledIdledConns) do
                pcall(function()
                    -- Only re-enable Lua connections; re-enabling C++/CoreScript Idled connections in some executors blocks later Disable()
                    if conn.LuaConnection == true and type(conn.Enable) == "function" then
                        conn:Enable()
                    end
                end)
            end
            table.clear(disabledIdledConns)
        end
    end
    State.ApplyAntiAFK = applyAntiAFK

    if LocalPlayer then
        ourIdledFn = function()
            if not ScriptRunning or not Config.AntiAFK then return end
            applyAntiAFK(true)
            local ok = pcall(function()
                local vu = game:GetService("VirtualUser")
                vu:CaptureController()
                vu:ClickButton2(Vector2.new())
            end)
            if not ok then
                pcall(function()
                    local vim = game:GetService("VirtualInputManager")
                    vim:SendKeyEvent(true, Enum.KeyCode.RightControl, false, game)
                    task.wait(0.05)
                    vim:SendKeyEvent(false, Enum.KeyCode.RightControl, false, game)
                end)
            end
        end
        trackConn(LocalPlayer.Idled:Connect(ourIdledFn))
        if Config.AntiAFK then
            applyAntiAFK(true)
        end
        task.spawn(function()
            while ScriptRunning do
                if Config.AntiAFK then
                    applyAntiAFK(true)
                end
                task.wait(15)
            end
        end)
    end
end

-- Noclip loop (runs on Stepped when user enables Noclip, during GlideTo, or during unanchored hover / Under Map stance so Head/Torso/HRP never hit walls)
do
    local wasNoclip = false
    trackConn(RunService.Stepped:Connect(function()
        local underMapActive = (Config.MobFarmPositionMode == "Under Map")
            and (Config.AutoMobFarm or Config.AutoRaid)
            and CombatEngine.ActiveStanceCF ~= nil
        local hoverActive = (CombatEngine and CombatEngine.HoverLockActive == true)
            and (Config.AutoMobFarm or Config.AutoRaid)
        local shouldNoclip = Config.Noclip or (NavEngine and NavEngine.IsGliding) or underMapActive or hoverActive
        if shouldNoclip and State.Character then
            wasNoclip = true
            for _, part in ipairs(State.Character:GetDescendants()) do
                if part:IsA("BasePart") and part.CanCollide then
                    part.CanCollide = false
                end
            end
        elseif wasNoclip then
            wasNoclip = false
            pcall(NavEngine.RestoreCharacterPhysics)
        end
    end))
end

-- Fullbright
local OriginalBrightness = game:GetService("Lighting").Brightness
local OriginalClockTime = game:GetService("Lighting").ClockTime
local OriginalFogEnd = game:GetService("Lighting").FogEnd

local function ApplyFullbright(enable)
    local Lighting = game:GetService("Lighting")
    if enable then
        Lighting.Brightness = 2
        Lighting.ClockTime = 14
        Lighting.FogEnd = 100000
        Lighting.GlobalShadows = false
    else
        Lighting.Brightness = OriginalBrightness
        Lighting.ClockTime = OriginalClockTime
        Lighting.FogEnd = OriginalFogEnd
        Lighting.GlobalShadows = true
    end
end

local MiscEngine = {
    SavedFogProps = {},
    SavedGfxProps = {},
    ClaimStatusLabel = nil,
    LastIndexClaimAt = 0,
    LastLevelClaimAt = 0,
    LastDailyClaimAt = 0,
}

function MiscEngine.ApplyRemoveFog(enable)
    local Lighting = game:GetService("Lighting")
    if enable then
        if not Config.Fullbright then
            MiscEngine.SavedFogProps.FogEnd = Lighting.FogEnd
            MiscEngine.SavedFogProps.FogStart = Lighting.FogStart
        end
        Lighting.FogStart = 0
        Lighting.FogEnd = 100000
        for _, inst in ipairs(Lighting:GetDescendants()) do
            if inst:IsA("Atmosphere") then
                if MiscEngine.SavedFogProps[inst] == nil then
                    MiscEngine.SavedFogProps[inst] = inst.Density
                end
                inst.Density = 0
            end
        end
    else
        if MiscEngine.SavedFogProps.FogEnd then Lighting.FogEnd = MiscEngine.SavedFogProps.FogEnd end
        if MiscEngine.SavedFogProps.FogStart then Lighting.FogStart = MiscEngine.SavedFogProps.FogStart end
        for inst, origDensity in pairs(MiscEngine.SavedFogProps) do
            if typeof(inst) == "Instance" and inst.Parent and inst:IsA("Atmosphere") then
                pcall(function() inst.Density = origDensity end)
            end
        end
    end
end

function MiscEngine.ApplyFpsBoost(enable)
    local Lighting = game:GetService("Lighting")
    if enable then
        MiscEngine.SavedGfxProps.GlobalShadows = Lighting.GlobalShadows
        Lighting.GlobalShadows = false
        for _, d in ipairs(workspace:GetDescendants()) do
            if d:IsA("ParticleEmitter") or d:IsA("Trail") or d:IsA("Beam") then
                if MiscEngine.SavedGfxProps[d] == nil then
                    MiscEngine.SavedGfxProps[d] = d.Enabled
                end
                d.Enabled = false
            end
        end
    else
        if MiscEngine.SavedGfxProps.GlobalShadows ~= nil then
            Lighting.GlobalShadows = MiscEngine.SavedGfxProps.GlobalShadows
        end
        for inst, origEnabled in pairs(MiscEngine.SavedGfxProps) do
            if typeof(inst) == "Instance" and inst.Parent then
                pcall(function() inst.Enabled = origEnabled end)
            end
        end
    end
end

MiscEngine.TeleportAlreadyQueued = false
MiscEngine.AutoExecStatusLabel = nil

function MiscEngine.SetAutoExecStatus(title, desc)
    if MiscEngine.AutoExecStatusLabel then
        pcall(function()
            MiscEngine.AutoExecStatusLabel:SetTitle(title)
            if desc ~= nil then MiscEngine.AutoExecStatusLabel:SetDesc(desc) end
        end)
    end
end

function MiscEngine.GetQueueTeleportFunc()
    local genv = (typeof(getgenv) == "function" and getgenv()) or nil
    if typeof(queue_on_teleport) == "function" then return queue_on_teleport end
    if typeof(queueonteleport) == "function" then return queueonteleport end
    if genv then
        if typeof(genv.queue_on_teleport) == "function" then return genv.queue_on_teleport end
        if typeof(genv.queueonteleport) == "function" then return genv.queueonteleport end
    end
    if syn and typeof(syn.queue_on_teleport) == "function" then return syn.queue_on_teleport end
    if fluxus and typeof(fluxus.queue_on_teleport) == "function" then return fluxus.queue_on_teleport end
    return nil
end

MiscEngine.DefaultLoaderCode = 'loadstring(game:HttpGet("https://getaether.cc/loader.lua"))()'

function MiscEngine.ResolveAutoExecPayload()
    local genv = (typeof(getgenv) == "function" and getgenv()) or nil
    local raw = tostring(Config.AutoExecLoader or ""):match("^%s*(.-)%s*$")
    local sourceLabel = "Custom Loader"

    if raw == "" then
        local globalCandidate = (genv and (genv.__AetherAutoExec or genv.__AetherLoaderCode or genv.__AetherLoaderUrl))
            or (_G and (_G.__AetherAutoExec or _G.__AetherLoaderCode or _G.__AetherLoaderUrl))
            or (shared and (shared.__AetherAutoExec or shared.__AetherLoaderCode or shared.__AetherLoaderUrl))
        if type(globalCandidate) == "string" and globalCandidate ~= "" then
            raw = globalCandidate:match("^%s*(.-)%s*$")
            sourceLabel = "Loader Global"
        end
    end

    if raw == "" and typeof(readfile) == "function" then
        local hasLocal = false
        pcall(function()
            hasLocal = (typeof(isfile) == "function" and isfile("TheForge.lua")) or false
        end)
        if hasLocal then
            local wrapped = table.concat({
                "task.spawn(function()",
                "    if typeof(getgenv) == 'function' and getgenv().__AetherAutoExecEnabled == false then return end",
                "    if not game:IsLoaded() then pcall(function() game.Loaded:Wait() end) end",
                "    local Players = game:GetService('Players')",
                "    while not Players.LocalPlayer do task.wait(0.1) end",
                "    task.wait(0.8)",
                "    if typeof(readfile) == 'function' then",
                "        local okU, uCode = pcall(readfile, 'UI.lua')",
                "        if okU and type(uCode) == 'string' and #uCode > 0 then",
                "            local okUF, uFn = pcall(loadstring, uCode)",
                "            if okUF and type(uFn) == 'function' then pcall(uFn) end",
                "        end",
                "        local okS, sCode = pcall(readfile, 'TheForge.lua')",
                "        if okS and type(sCode) == 'string' and #sCode > 0 then",
                "            local okSF, sFn = pcall(loadstring, sCode)",
                "            if okSF and type(sFn) == 'function' then pcall(sFn) end",
                "        end",
                "    end",
                "end)"
            }, "\n")
            return wrapped, "Local Disk (TheForge.lua)"
        end
    end

    if raw == "" then
        raw = MiscEngine.DefaultLoaderCode
        sourceLabel = "Aether Loader"
    end

    if raw ~= "" then
        local innerCode = raw
        if raw:match("^https?://") then
            innerCode = string.format('loadstring(game:HttpGet(%q))()', raw)
        end
        local wrapped = table.concat({
            "task.spawn(function()",
            "    if typeof(getgenv) == 'function' and getgenv().__AetherAutoExecEnabled == false then return end",
            "    if not game:IsLoaded() then pcall(function() game.Loaded:Wait() end) end",
            "    local Players = game:GetService('Players')",
            "    while not Players.LocalPlayer do task.wait(0.1) end",
            "    task.wait(0.8)",
            string.format("    if typeof(getgenv) == 'function' then getgenv().__AetherAutoExec = %q end", raw),
            "    local ok, fn = pcall(loadstring, " .. string.format("%q", innerCode) .. ")",
            "    if ok and type(fn) == 'function' then pcall(fn) end",
            "end)"
        }, "\n")
        return wrapped, sourceLabel
    end

    return nil, "None"
end

function MiscEngine.QueueOnTeleportNow(showNotify)
    local qFn = MiscEngine.GetQueueTeleportFunc()
    if not qFn then
        MiscEngine.SetAutoExecStatus("Auto-Exec: Unsupported", "Executor does not expose queue_on_teleport")
        if showNotify then
            Notify("Auto-Execute", "Your executor does not expose queue_on_teleport.", 3.5)
        end
        return false
    end

    local payload, srcLabel = MiscEngine.ResolveAutoExecPayload()
    if not payload then
        MiscEngine.SetAutoExecStatus("Auto-Exec: Needs Loader", "Paste your loader URL/code above (or set getgenv().__AetherAutoExec)")
        if showNotify then
            Notify("Auto-Execute", "Paste your loader URL / loadstring snippet first!", 3.5)
        end
        return false
    end

    local ok, err = pcall(qFn, payload)
    if ok then
        MiscEngine.TeleportAlreadyQueued = true
        MiscEngine.SetAutoExecStatus("Auto-Exec: Armed (" .. srcLabel .. ")", "Will re-execute automatically after server teleports")
        if showNotify then
            Notify("Auto-Execute", "Queued (" .. srcLabel .. ") for next teleport!", 3.0)
        end
        return true
    else
        MiscEngine.SetAutoExecStatus("Auto-Exec: Queue Error", tostring(err))
        if showNotify then
            Notify("Auto-Execute Error", tostring(err), 3.5)
        end
        return false
    end
end

if LocalPlayer then
    trackConn(LocalPlayer.OnTeleport:Connect(function(tpState)
        if not ScriptRunning then return end
        if tpState == Enum.TeleportState.Started then
            if Config.AutoExecOnTeleport and not MiscEngine.TeleportAlreadyQueued then
                MiscEngine.QueueOnTeleportNow(false)
            end
        elseif tpState == Enum.TeleportState.Failed then
            MiscEngine.TeleportAlreadyQueued = false
        end
    end))
end

function MiscEngine.ApplyNoclipCamera(enable)
    if not LocalPlayer then return end
    if enable then
        if MiscEngine.OriginalOcclusionMode == nil then
            pcall(function() MiscEngine.OriginalOcclusionMode = LocalPlayer.DevCameraOcclusionMode end)
        end
        pcall(function() LocalPlayer.DevCameraOcclusionMode = Enum.DevCameraOcclusionMode.Invisicam end)
    else
        pcall(function()
            LocalPlayer.DevCameraOcclusionMode = MiscEngine.OriginalOcclusionMode or Enum.DevCameraOcclusionMode.Zoom
        end)
    end
end

function MiscEngine.GetClaimableIndexCount()
    local count = 0
    pcall(function()
        local pd = State.PlayerData or ResolvePlayerData()
        if pd and pd.Data then
            local IndexCatalog = safeRequire(ReplicatedStorage.Shared.IndexCatalog)
            if IndexCatalog and type(IndexCatalog.Claimable) == "function" then
                count = tonumber(IndexCatalog.Claimable(pd.Data)) or 0
            end
        end
    end)
    return count
end

function MiscEngine.SetClaimStatus(title, desc)
    if MiscEngine.ClaimStatusLabel then
        pcall(function()
            MiscEngine.ClaimStatusLabel:SetTitle(title)
            if desc ~= nil then MiscEngine.ClaimStatusLabel:SetDesc(desc) end
        end)
    end
end

function MiscEngine.ClaimAllRewards(forceAll)
    local claimedAny = false
    local idxCount = MiscEngine.GetClaimableIndexCount()
    if (forceAll or Config.AutoClaimIndex) and idxCount > 0 and Remotes.CodexClaimAll then
        local ok = pcall(function() Remotes.CodexClaimAll:InvokeServer() end)
        if ok then claimedAny = true end
    end
    if (forceAll or Config.AutoClaimLevelRewards) and Remotes.LevelRewardsClaimAll then
        local ok = pcall(function() Remotes.LevelRewardsClaimAll:InvokeServer() end)
        if ok then claimedAny = true end
    end
    if (forceAll or Config.AutoClaimDaily) then
        if Remotes.ClaimDaily then pcall(function() Remotes.ClaimDaily:InvokeServer() end) end
        if Remotes.ClaimMailAll then pcall(function() Remotes.ClaimMailAll:InvokeServer() end) end
    end
    local afterCount = MiscEngine.GetClaimableIndexCount()
    MiscEngine.SetClaimStatus(
        "Index Claimable: " .. tostring(afterCount),
        claimedAny and ("Last claimed at " .. os.date("%H:%M:%S")) or "Tracks Codex Index, Level Rewards & Daily/Mail"
    )
    return claimedAny
end

task.spawn(function()
    while ScriptRunning do
        local now = os.clock()
        local idxCount = MiscEngine.GetClaimableIndexCount()
        if Config.AutoClaimIndex and idxCount > 0 and (now - MiscEngine.LastIndexClaimAt) >= 4.0 and Remotes.CodexClaimAll then
            MiscEngine.LastIndexClaimAt = now
            local ok = pcall(function() Remotes.CodexClaimAll:InvokeServer() end)
            if ok then
                task.wait(0.25)
                idxCount = MiscEngine.GetClaimableIndexCount()
                MiscEngine.SetClaimStatus("Index Claimable: " .. tostring(idxCount), "Auto-claimed Index rewards at " .. os.date("%H:%M:%S"))
            end
        elseif MiscEngine.ClaimStatusLabel then
            MiscEngine.SetClaimStatus("Index Claimable: " .. tostring(idxCount))
        end

        if Config.AutoClaimLevelRewards and (now - MiscEngine.LastLevelClaimAt) >= 20.0 and Remotes.LevelRewardsClaimAll then
            MiscEngine.LastLevelClaimAt = now
            pcall(function() Remotes.LevelRewardsClaimAll:InvokeServer() end)
        end

        if Config.AutoClaimDaily and (now - MiscEngine.LastDailyClaimAt) >= 60.0 then
            MiscEngine.LastDailyClaimAt = now
            if Remotes.ClaimDaily then pcall(function() Remotes.ClaimDaily:InvokeServer() end) end
            if Remotes.ClaimMailAll then pcall(function() Remotes.ClaimMailAll:InvokeServer() end) end
        end
        task.wait(4.0)
    end
end)

-- ===================================================================
-- 11B. DISCORD WEBHOOK ENGINE (Gold, Level & Rare Ore Alerts)
-- ===================================================================
WebhookEngine.RarityRank = {
    Common = 1,
    Uncommon = 2,
    Rare = 3,
    Epic = 4,
    Legendary = 5,
    Mythic = 6,
    Exotic = 7,
    Relic = 8,
    Divine = 9,
    Unobtainable = 10,
}
WebhookEngine.RarityColors = {
    Common = 9807270,
    Uncommon = 3066993,
    Rare = 3447003,
    Epic = 10181046,
    Legendary = 15844367,
    Mythic = 15158332,
    Exotic = 16744192,
    Relic = 15277667,
    Divine = 16766720,
    Unobtainable = 16711935,
}
WebhookEngine.LastOreCounts = {}
WebhookEngine.InitializedInventory = false
WebhookEngine.StartGold = nil
WebhookEngine.LastStatusSentAt = 0
WebhookEngine.LastPostAt = 0
WebhookEngine.RareFindsSession = 0
WebhookEngine.RaidRunsSession = 0
WebhookEngine.RaidGoldSession = 0
WebhookEngine.ActiveRaidSnapshot = nil
WebhookEngine.LastRaidRewardPayload = nil
WebhookEngine.LastRaidReportSentAt = 0
WebhookEngine.WasInRaidRun = false
WebhookEngine.StatusLabel = nil
WebhookEngine.RaidStatusLabel = nil

function WebhookEngine.GetRequestFunc()
    if typeof(request) == "function" then return request end
    if typeof(http_request) == "function" then return http_request end
    if typeof(http) == "table" and typeof(http.request) == "function" then return http.request end
    if typeof(syn) == "table" and typeof(syn.request) == "function" then return syn.request end
    if typeof(fluxus) == "table" and typeof(fluxus.request) == "function" then return fluxus.request end
    return nil
end


function WebhookEngine.SetStatus(title, desc)
    if WebhookEngine.StatusLabel then
        pcall(function()
            WebhookEngine.StatusLabel:SetTitle(title)
            if desc ~= nil then WebhookEngine.StatusLabel:SetDesc(desc) end
        end)
    end
end

function WebhookEngine.FormatNumber(n)
    local num = math.floor(tonumber(n) or 0)
    local str = tostring(num)
    local formatted = str:reverse():gsub("(%d%d%d)", "%1,"):reverse()
    return (formatted:gsub("^,", ""))
end

function WebhookEngine.GetPlayerStats()
    if not State.PlayerData then ResolvePlayerData() end
    local data = (State.PlayerData and State.PlayerData.Data) or {}
    local gold = tonumber(data.Gold) or 0
    if not WebhookEngine.StartGold and data.Gold ~= nil then
        WebhookEngine.StartGold = gold
    end
    local level = tonumber(data.Level) or (LocalPlayer and tonumber(LocalPlayer:GetAttribute("Level"))) or 1
    local island = tostring(data.CurrentIsland or workspace:GetAttribute("Island") or "Unknown")
    local used, cap = 0, 0
    if ForgeEngine and type(ForgeEngine.GetStashCapacityStatus) == "function" then
        local ok, u, c = pcall(ForgeEngine.GetStashCapacityStatus)
        if ok and type(u) == "number" and type(c) == "number" then
            used, cap = u, c
        end
    end
    local goldEarned = WebhookEngine.StartGold and math.max(0, math.floor(gold - WebhookEngine.StartGold)) or 0
    return {
        Gold = gold,
        GoldEarned = goldEarned,
        Level = level,
        Island = island,
        BagUsed = used,
        BagCap = cap,
        PlayerName = LocalPlayer and LocalPlayer.Name or "Player",
    }
end

function WebhookEngine.GetWatchOreSet()
    local set = {}
    local list = Config.WebhookWatchOres
    if type(list) == "table" then
        for k, v in pairs(list) do
            local name = (type(k) == "number") and tostring(v) or (v and tostring(k))
            if name and name ~= "" and name ~= "None" then
                set[name] = true
            end
        end
    end
    return set
end

function WebhookEngine.IsRareOreMatch(oreName, watchSet)
    local oreInfo = ForgeEngine and ForgeEngine.OreDataByName and ForgeEngine.OreDataByName[oreName]
    if not oreInfo then
        return false, nil
    end
    if watchSet and watchSet[oreName] then
        return true, oreInfo
    end
    local minTier = Config.WebhookPingMinRarity or "Mythic"
    local minRank = WebhookEngine.RarityRank[minTier]
    if minRank then
        local oreRank = WebhookEngine.RarityRank[tostring(oreInfo.Rarity or "Common")] or 1
        if oreRank >= minRank then
            return true, oreInfo
        end
    end
    return false, oreInfo
end

function WebhookEngine.BuildPingContent()
    if WebhookEngine.UserIdTextbox and type(WebhookEngine.UserIdTextbox.GetValue) == "function" then
        local ok, txt = pcall(WebhookEngine.UserIdTextbox.GetValue)
        if ok and type(txt) == "string" then
            Config.WebhookUserId = txt:match("^%s*(.-)%s*$")
        end
    end
    local raw = tostring(Config.WebhookUserId or ""):match("^%s*(.-)%s*$")
    if raw == "" then return nil end
    local lower = raw:lower()
    if lower == "everyone" or lower == "@everyone" then
        return "@everyone"
    elseif lower == "here" or lower == "@here" then
        return "@here"
    end
    local digits = raw:match("(%d+)")
    if digits then
        return "<@" .. digits .. ">"
    end
    return raw
end

function WebhookEngine.PostPayload(payload)
    local reqFn = WebhookEngine.GetRequestFunc()
    if not reqFn then
        WebhookEngine.SetStatus("Webhook: Unsupported", "Executor does not expose request/http_request")
        return false, "No executor HTTP request function"
    end
    if WebhookEngine.UrlTextbox and type(WebhookEngine.UrlTextbox.GetValue) == "function" then
        local ok, txt = pcall(WebhookEngine.UrlTextbox.GetValue)
        if ok and type(txt) == "string" then
            Config.WebhookUrl = txt:match("^%s*(.-)%s*$")
        end
    end
    local url = tostring(Config.WebhookUrl or ""):match("^%s*(.-)%s*$")
    if not url:match("^https?://") then
        WebhookEngine.SetStatus("Webhook: Invalid URL", "Enter a valid Discord webhook URL")
        return false, "Invalid URL"
    end
    local bodyOk, body = pcall(function()
        return HttpService:JSONEncode(payload)
    end)
    if not bodyOk or not body then
        return false, "JSON encode failed"
    end

    WebhookEngine.LastPostAt = os.clock()
    local ok, res = pcall(reqFn, {
        Url = url,
        Method = "POST",
        Headers = { ["Content-Type"] = "application/json" },
        Body = body,
    })
    if not ok then
        WebhookEngine.SetStatus("Webhook: Request Error", tostring(res))
        return false, tostring(res)
    end
    local statusCode = type(res) == "table" and tonumber(res.StatusCode or res.Status) or 0
    if statusCode >= 200 and statusCode < 300 then
        WebhookEngine.SetStatus(
            "Webhook: Sent (" .. tostring(statusCode) .. ")",
            "Last sent: " .. os.date("%H:%M:%S")
        )
        return true, statusCode
    else
        local msg = type(res) == "table" and tostring(res.Body or res.StatusMessage or statusCode) or tostring(res)
        WebhookEngine.SetStatus("Webhook: HTTP " .. tostring(statusCode), msg:sub(1, 80))
        return false, msg
    end
end

function WebhookEngine.SendStatusReport(isTest)
    local stats = WebhookEngine.GetPlayerStats()
    local modeFlags = {}
    if Config.AutoMine then table.insert(modeFlags, "Auto Mine") end
    if Config.AutoMobFarm then table.insert(modeFlags, "Auto Mob") end
    if Config.AutoForgeArmor then table.insert(modeFlags, "Auto Forge") end
    local activeStr = (#modeFlags > 0) and table.concat(modeFlags, ", ") or "Idle"
    local title = isTest and "The Forge - Test Status" or "The Forge - Progress Update"
    local embed = {
        title = title,
        color = 5793266, -- Discord Blurple (#5865F2)
        fields = {
            { name = "Money (Gold)", value = "$" .. WebhookEngine.FormatNumber(stats.Gold) .. " (+" .. WebhookEngine.FormatNumber(stats.GoldEarned) .. " session)", inline = true },
            { name = "Level", value = tostring(stats.Level), inline = true },
            { name = "Island", value = stats.Island, inline = true },
            { name = "Stash Bag", value = string.format("%d / %d", stats.BagUsed, stats.BagCap), inline = true },
            { name = "Active Tasks", value = activeStr, inline = true },
            { name = "Rare Finds", value = tostring(WebhookEngine.RareFindsSession or 0), inline = true },
        },
        footer = { text = "The Forge • " .. stats.PlayerName },
        timestamp = os.date("!%Y-%m-%dT%H:%M:%SZ"),
    }
    local payload = {
        username = "The Forge Tracker",
        embeds = { embed },
    }
    if isTest then
        local ping = WebhookEngine.BuildPingContent()
        if ping then payload.content = ping .. " Test webhook connected!" end
    end
    return WebhookEngine.PostPayload(payload)
end

function WebhookEngine.SendRareOreAlert(rareList)
    if not rareList or #rareList == 0 then return false end
    local stats = WebhookEngine.GetPlayerStats()
    local highestRank = 0
    local highestColor = 15844367
    local lines = {}
    for _, item in ipairs(rareList) do
        local info = item.info or {}
        local rName = tostring(info.Rarity or "Rare")
        local rRank = WebhookEngine.RarityRank[rName] or 0
        if rRank >= highestRank then
            highestRank = rRank
            highestColor = WebhookEngine.RarityColors[rName] or highestColor
        end
        local multStr = info.Multiplier and string.format(" | %.2fx", tonumber(info.Multiplier) or 0) or ""
        table.insert(lines, string.format("**%s** x%d (`%s`%s) — Bag: %d", item.name, item.delta, rName, multStr, item.total))
    end
    local ping = WebhookEngine.BuildPingContent()
    local embed = {
        title = (#rareList == 1) and ("Rare Ore Found: " .. rareList[1].name) or string.format("%d Rare Ores Found!", #rareList),
        description = table.concat(lines, "\n"),
        color = highestColor,
        fields = {
            { name = "Money (Gold)", value = "$" .. WebhookEngine.FormatNumber(stats.Gold), inline = true },
            { name = "Level", value = tostring(stats.Level), inline = true },
            { name = "Island", value = stats.Island, inline = true },
            { name = "Stash Bag", value = string.format("%d / %d", stats.BagUsed, stats.BagCap), inline = true },
        },
        footer = { text = "The Forge • " .. stats.PlayerName },
        timestamp = os.date("!%Y-%m-%dT%H:%M:%SZ"),
    }
    local payload = {
        username = "The Forge Ore Alert",
        embeds = { embed },
    }
    if ping then
        payload.content = ping
    end
    return WebhookEngine.PostPayload(payload)
end

function WebhookEngine.SendForgeCycleReport(stage, beforeSnap)
    local stats = WebhookEngine.GetPlayerStats()
    local usableNow = 0
    if ForgeEngine and type(ForgeEngine.GetPlayerOreInventory) == "function" then
        pcall(function()
            for _, ore in ipairs(ForgeEngine.GetPlayerOreInventory()) do
                usableNow = usableNow + (ore.Count or 0)
            end
        end)
    end

    local embed
    if stage == "before" then
        local b = beforeSnap or {}
        local bGold = b.gold or stats.Gold
        local bLevel = b.level or stats.Level
        local bUsed = b.bagUsed or stats.BagUsed
        local bCap = b.bagCap or stats.BagCap
        local bUsable = b.usableOres or usableNow
        embed = {
            title = "The Forge - Inventory Full (Before Forging)",
            color = 15844367, -- Gold/Amber
            fields = {
                { name = "Money (Gold)", value = "$" .. WebhookEngine.FormatNumber(bGold), inline = true },
                { name = "Level", value = tostring(bLevel), inline = true },
                { name = "Island", value = stats.Island, inline = true },
                { name = "Stash Bag", value = string.format("%d / %d [FULL]", bUsed, bCap), inline = true },
                { name = "Usable Ores", value = tostring(bUsable), inline = true },
            },
            footer = { text = "The Forge • " .. stats.PlayerName },
            timestamp = os.date("!%Y-%m-%dT%H:%M:%SZ"),
        }
    else
        local b = beforeSnap or {}
        local bGold = b.gold or stats.Gold
        local bLevel = b.level or stats.Level
        local bUsed = b.bagUsed or stats.BagUsed
        local bCap = b.bagCap or stats.BagCap
        local bUsable = b.usableOres or usableNow
        local goldDelta = math.floor(stats.Gold - bGold)
        local oresForged = math.max(0, bUsable - usableNow)
        local elapsedSec = b.startedAt and math.max(0, math.floor(os.clock() - b.startedAt)) or 0
        local goldSign = (goldDelta >= 0) and "+" or "-"
        local lvlText = (stats.Level ~= bLevel) and string.format("%d -> %d", bLevel, stats.Level) or tostring(stats.Level)
        embed = {
            title = "The Forge - Forge Cycle Complete (After)",
            color = 3066993, -- Emerald Green
            fields = {
                {
                    name = "Money (Before -> After)",
                    value = string.format("$%s -> $%s (%s$%s)", WebhookEngine.FormatNumber(bGold), WebhookEngine.FormatNumber(stats.Gold), goldSign, WebhookEngine.FormatNumber(math.abs(goldDelta))),
                    inline = false,
                },
                {
                    name = "Stash Bag (Before -> After)",
                    value = string.format("%d/%d -> %d/%d", bUsed, bCap, stats.BagUsed, stats.BagCap),
                    inline = true,
                },
                { name = "Ores Forged", value = tostring(oresForged), inline = true },
                { name = "Level", value = lvlText, inline = true },
                { name = "Cycle Time", value = string.format("%ds", elapsedSec), inline = true },
                { name = "Island", value = stats.Island, inline = true },
            },
            footer = { text = "The Forge • " .. stats.PlayerName },
            timestamp = os.date("!%Y-%m-%dT%H:%M:%SZ"),
        }
    end

    return WebhookEngine.PostPayload({
        username = "The Forge Tracker",
        embeds = { embed },
    })
end

function WebhookEngine.SetRaidStatus(title, desc)
    if WebhookEngine.RaidStatusLabel then
        pcall(function()
            WebhookEngine.RaidStatusLabel:SetTitle(title)
            if desc ~= nil then WebhookEngine.RaidStatusLabel:SetDesc(desc) end
        end)
    end
end

function WebhookEngine.CaptureRaidSnapshot(forceReset)
    if WebhookEngine.ActiveRaidSnapshot and not forceReset then
        return WebhookEngine.ActiveRaidSnapshot
    end
    local pd = State.PlayerData or ResolvePlayerData()
    local data = pd and pd.Data
    local inv = data and data.Inventory
    local snap = {
        startedAt = os.clock(),
        mode = (RaidEngine and RaidEngine.FormatModeLabel and RaidEngine.FormatModeLabel(Config.RaidMode)) or tostring(Config.RaidMode or "Abyss / Tower"),
        tier = tonumber(Config.RaidTier) or 1,
        gold = (data and tonumber(data.Gold)) or 0,
        level = (data and tonumber(data.Level)) or 1,
        counts = {},
        runeGuids = {},
        equipGuids = {},
    }
    if type(inv) == "table" then
        for k, v in pairs(inv) do
            if type(v) == "number" then
                snap.counts[k] = v
            elseif k == "Runes" and type(v) == "table" then
                for rk, rv in pairs(v) do
                    local gid = (type(rv) == "table" and (rv.GUID or rv.Id or rk)) or rk
                    snap.runeGuids[gid] = true
                end
            elseif k == "Equipments" and type(v) == "table" then
                for ek, ev in pairs(v) do
                    local gid = (type(ev) == "table" and (ev.GUID or ek)) or ek
                    snap.equipGuids[gid] = true
                end
            end
        end
    end
    WebhookEngine.ActiveRaidSnapshot = snap
    WebhookEngine.SetRaidStatus(
        "Tracking: " .. tostring(snap.mode),
        string.format("Tier %d | Snapshot captured (Gold: $%s)", snap.tier, WebhookEngine.FormatNumber(snap.gold))
    )
    return snap
end

function WebhookEngine.SendRaidRewardsReport(raidData, isTest)
    if not isTest then
        if not Config.WebhookEnabled or not Config.WebhookNotifyRaidRewards then return false end
        if (os.clock() - (WebhookEngine.LastRaidReportSentAt or 0)) < 3.0 then return false end
    end

    if isTest and WebhookEngine.LastRaidRewardPayload then
        return WebhookEngine.PostPayload(WebhookEngine.LastRaidRewardPayload)
    end

    local snap = WebhookEngine.ActiveRaidSnapshot
    local pd = State.PlayerData or ResolvePlayerData()
    local data = pd and pd.Data
    local inv = data and data.Inventory
    local stats = WebhookEngine.GetPlayerStats()

    local postGold = (data and tonumber(data.Gold)) or stats.Gold or 0
    local postLevel = (data and tonumber(data.Level)) or stats.Level or 1
    local preGold = (snap and snap.gold) or postGold
    local preLevel = (snap and snap.level) or postLevel
    local goldDelta = math.floor(postGold - preGold)

    local gainedItems = {}
    local gainedRunes = {}
    local gainedEquips = {}

    if snap and type(inv) == "table" then
        for k, v in pairs(inv) do
            if type(v) == "number" then
                local prev = snap.counts[k] or 0
                if v > prev then
                    table.insert(gainedItems, { name = tostring(k), qty = math.floor(v - prev), total = math.floor(v) })
                end
            elseif k == "Runes" and type(v) == "table" then
                for rk, rv in pairs(v) do
                    local gid = (type(rv) == "table" and (rv.GUID or rv.Id or rk)) or rk
                    if not snap.runeGuids[gid] then
                        local rName = (type(rv) == "table" and (rv.Name or rv.Id)) or tostring(rk)
                        table.insert(gainedRunes, tostring(rName))
                    end
                end
            elseif k == "Equipments" and type(v) == "table" then
                for ek, ev in pairs(v) do
                    local gid = (type(ev) == "table" and (ev.GUID or ek)) or ek
                    if not snap.equipGuids[gid] then
                        local eName = (type(ev) == "table" and (ev.Name or ev.Type)) or tostring(ek)
                        local eQty = (type(ev) == "table" and ev.Quality) and (" (" .. tostring(ev.Quality) .. ")") or ""
                        table.insert(gainedEquips, tostring(eName) .. eQty)
                    end
                end
            end
        end
    end

    table.sort(gainedItems, function(a, b)
        if a.qty ~= b.qty then return a.qty > b.qty end
        return a.name < b.name
    end)

    local hasAnyDrops = (#gainedItems > 0 or #gainedRunes > 0 or #gainedEquips > 0 or goldDelta > 0)
    if not isTest and not hasAnyDrops and not Config.WebhookRaidSendEmpty then
        WebhookEngine.ActiveRaidSnapshot = nil
        return false
    end

    local durSec = snap and math.max(0, math.floor(os.clock() - snap.startedAt)) or 0
    local durStr = string.format("%dm %02ds", math.floor(durSec / 60), durSec % 60)
    local modeStr = (snap and snap.mode) or (RaidEngine and RaidEngine.FormatModeLabel and RaidEngine.FormatModeLabel(Config.RaidMode)) or tostring(Config.RaidMode or "Abyss / Tower")
    local tierNum = (snap and snap.tier) or tonumber(Config.RaidTier) or 1
    local waveNum = (type(raidData) == "table" and (raidData.Wave or raidData.Floor or (type(raidData.Summary) == "table" and (raidData.Summary.Wave or raidData.Summary.Floor))))
        or (RaidEngine and ((RaidEngine.CurrentFloor and RaidEngine.CurrentFloor > 0 and RaidEngine.CurrentFloor) or RaidEngine.CurrentWave))
        or 0
    local won = (type(raidData) == "table" and (raidData.Victory == true or raidData.Won == true or (type(raidData.Summary) == "table" and (raidData.Summary.Victory == true or raidData.Summary.Cleared == true))))
    local outcomeStr = isTest and "Test Preview" or (won and "Victory / Cleared" or "Run Ended")

    local itemLines = {}
    local itemCharLen = 0
    for _, it in ipairs(gainedItems) do
        local line = string.format("• **+%d %s** (Total: %s)", it.qty, it.name, WebhookEngine.FormatNumber(it.total))
        if itemCharLen + #line + 1 > 950 then
            table.insert(itemLines, string.format("• ...and %d more item type(s)", #gainedItems - #itemLines))
            break
        end
        table.insert(itemLines, line)
        itemCharLen = itemCharLen + #line + 1
    end

    local gearLines = {}
    for idx, rn in ipairs(gainedRunes) do
        if idx <= 10 then table.insert(gearLines, "• Rune: **" .. rn .. "**") end
    end
    for idx, eq in ipairs(gainedEquips) do
        if idx <= 10 then table.insert(gearLines, "• Equip: **" .. eq .. "**") end
    end

    if not isTest then
        WebhookEngine.RaidRunsSession = (WebhookEngine.RaidRunsSession or 0) + 1
        if goldDelta > 0 then
            WebhookEngine.RaidGoldSession = (WebhookEngine.RaidGoldSession or 0) + goldDelta
        end
        WebhookEngine.LastRaidReportSentAt = os.clock()
    end

    local goldSign = goldDelta >= 0 and "+" or "-"
    local fields = {
        { name = "Mode & Outcome", value = string.format("%s (Tier %d)\n**%s**", modeStr, tierNum, outcomeStr), inline = true },
        { name = "Wave / Duration", value = string.format("Wave/Floor: **%d**\nTime: **%s**", tonumber(waveNum) or 0, durStr), inline = true },
        { name = "Gold & Level", value = string.format("%s$%s Gold (Now: $%s)\nLv. %d -> Lv. %d", goldSign, WebhookEngine.FormatNumber(math.abs(goldDelta)), WebhookEngine.FormatNumber(postGold), preLevel, postLevel), inline = true },
        { name = "Items & Ores Gained", value = (#itemLines > 0) and table.concat(itemLines, "\n") or "_None_", inline = false },
    }
    if #gearLines > 0 then
        table.insert(fields, { name = "Runes & Equipment Gained", value = table.concat(gearLines, "\n"), inline = false })
    end
    table.insert(fields, { name = "Session Raid Totals", value = string.format("Runs: %d | Gold Gained: +$%s", WebhookEngine.RaidRunsSession or 0, WebhookEngine.FormatNumber(WebhookEngine.RaidGoldSession or 0)), inline = false })

    local payload = {
        content = (not isTest and Config.WebhookRaidPing) and WebhookEngine.BuildPingContent() or nil,
        username = "The Forge • Raid Rewards",
        embeds = {
            {
                title = (isTest and "[TEST] " or "") .. modeStr .. " Rewards",
                color = won and 0x22C55E or 0xA855F7,
                fields = fields,
                footer = { text = string.format("Player: %s | The Forge", stats.Player) },
                timestamp = os.date("!%Y-%m-%dT%H:%M:%SZ"),
            }
        },
    }

    if not isTest then
        WebhookEngine.LastRaidRewardPayload = payload
        WebhookEngine.ActiveRaidSnapshot = nil
        WebhookEngine.SetRaidStatus(
            "Last Sent: " .. modeStr,
            string.format("%s$%s Gold | %d item type(s), %d rune(s), %d equip(s)", goldSign, WebhookEngine.FormatNumber(math.abs(goldDelta)), #gainedItems, #gainedRunes, #gainedEquips)
        )
    end
    return WebhookEngine.PostPayload(payload)
end

-- Background Webhook Inventory Diff & Periodic Status Loop
task.spawn(function()
    while ScriptRunning do
        local pd = State.PlayerData or ResolvePlayerData()
        local inv = pd and pd.Data and pd.Data.Inventory
        if type(inv) == "table" and ForgeEngine and next(ForgeEngine.OreDataByName) ~= nil then
            if pd.Data.Gold ~= nil and not WebhookEngine.StartGold then
                WebhookEngine.StartGold = tonumber(pd.Data.Gold) or 0
            end
            if not WebhookEngine.InitializedInventory then
                for itemName, qty in pairs(inv) do
                    if type(qty) == "number" and ForgeEngine.OreDataByName[itemName] then
                        WebhookEngine.LastOreCounts[itemName] = qty
                    end
                end
                WebhookEngine.InitializedInventory = true
                WebhookEngine.LastStatusSentAt = os.clock()
            else
                local gainedRare = {}
                local watchSet = WebhookEngine.GetWatchOreSet()
                for itemName, qty in pairs(inv) do
                    if type(qty) == "number" and ForgeEngine.OreDataByName[itemName] then
                        local prev = WebhookEngine.LastOreCounts[itemName] or 0
                        if qty > prev then
                            local delta = qty - prev
                            if Config.WebhookEnabled and Config.WebhookNotifyRareOres then
                                local isRare, info = WebhookEngine.IsRareOreMatch(itemName, watchSet)
                                if isRare then
                                    WebhookEngine.RareFindsSession = (WebhookEngine.RareFindsSession or 0) + delta
                                    table.insert(gainedRare, {
                                        name = itemName,
                                        delta = delta,
                                        total = qty,
                                        info = info,
                                    })
                                end
                            end
                        end
                        WebhookEngine.LastOreCounts[itemName] = qty
                    end
                end
                -- Reset counts for ores completely consumed/sold
                for knownName in pairs(WebhookEngine.LastOreCounts) do
                    if inv[knownName] == nil then
                        WebhookEngine.LastOreCounts[knownName] = 0
                    end
                end

                if #gainedRare > 0 and (os.clock() - WebhookEngine.LastPostAt) >= 2.0 then
                    task.spawn(function()
                        WebhookEngine.SendRareOreAlert(gainedRare)
                    end)
                end

                -- Standalone Bag-Full alert when Auto-Forge is OFF (Auto-Forge sends its own Before & After pair)
                if Config.WebhookEnabled and Config.WebhookNotifyForgeCycle and not Config.AutoForgeArmor and ForgeEngine and type(ForgeEngine.GetStashCapacityStatus) == "function" then
                    local okBag, used, cap, isFull, usableOres = pcall(ForgeEngine.GetStashCapacityStatus)
                    if okBag and isFull then
                        if not WebhookEngine.NotifiedBagFull then
                            WebhookEngine.NotifiedBagFull = true
                            task.spawn(WebhookEngine.SendForgeCycleReport, "before", {
                                gold = tonumber(pd.Data.Gold) or 0,
                                level = tonumber(pd.Data.Level) or 1,
                                bagUsed = used,
                                bagCap = cap,
                                usableOres = usableOres,
                            })
                        end
                    elseif okBag and not isFull then
                        WebhookEngine.NotifiedBagFull = false
                    end
                end

                local intervalMin = tonumber(Config.WebhookStatusIntervalMin) or 0
                if Config.WebhookEnabled and intervalMin > 0 then
                    if (os.clock() - WebhookEngine.LastStatusSentAt) >= (intervalMin * 60) and (os.clock() - WebhookEngine.LastPostAt) >= 2.0 then
                        WebhookEngine.LastStatusSentAt = os.clock()
                        task.spawn(function()
                            WebhookEngine.SendStatusReport(false)
                        end)
                    end
                end
            end
        end

        -- Track Tower of Hell / Abyss run transitions (works for both Manual & Auto Raid)
        if RaidEngine and type(RaidEngine.IsInActiveRun) == "function" then
            local inRunNow = RaidEngine.IsInActiveRun()
            if inRunNow and not WebhookEngine.WasInRaidRun then
                WebhookEngine.WasInRaidRun = true
                WebhookEngine.CaptureRaidSnapshot(true)
            elseif not inRunNow and WebhookEngine.WasInRaidRun then
                WebhookEngine.WasInRaidRun = false
                if Config.WebhookEnabled and Config.WebhookNotifyRaidRewards and WebhookEngine.ActiveRaidSnapshot then
                    task.spawn(function()
                        task.wait(0.8)
                        WebhookEngine.SendRaidRewardsReport(nil, false)
                    end)
                end
            end
        end
        task.wait(1.5)
    end
end)

-- ===================================================================
-- 11C. AUTO BUY & AUTO USE POTIONS ENGINE
-- ===================================================================
local PotionEngine = {
    IdToData = {},
    LabelToId = {},
    ShopPotionLabels = {},
    AllPotionLabels = {},
    LastUseAttempt = {},
    LastBuyFailAt = {},
    LastBuyCheckAt = 0,
    LastAbyssState = nil,
    BuyDropdown = nil,
    UseDropdown = nil,
    StatusLabel = nil,
}
State.PotionEngine = PotionEngine

function PotionEngine.RefreshWorldCatalog(forceUiRefresh)
    local hrp = State.HRP or (LocalPlayer and LocalPlayer.Character and LocalPlayer.Character:FindFirstChild("HumanoidRootPart"))
    local inAbyss = hrp and (hrp.Position.X < -5000 or hrp.Position.Z < -5000) or false
    if not forceUiRefresh and PotionEngine.LastAbyssState == inAbyss and #PotionEngine.ShopPotionLabels > 0 then
        return
    end
    PotionEngine.LastAbyssState = inAbyss

    local prox = workspace:FindFirstChild("Proximity")
    local ownedSet = {}
    local pd = State.PlayerData or ResolvePlayerData()
    local misc = pd and pd.Data and pd.Data.Inventory and pd.Data.Inventory.Misc
    if type(misc) == "table" then
        for _, item in ipairs(misc) do
            if type(item) == "table" and item.Name and (tonumber(item.Quantity) or 1) > 0 then
                ownedSet[item.Name] = true
            end
        end
    end

    local shopEntries = {}
    local extraEntries = {}
    local proxChildren = prox and prox:GetChildren() or {}
    for id, entry in pairs(PotionEngine.IdToData) do
        local inWorldShop = false
        for _, stand in ipairs(proxChildren) do
            if stand.Name == id then
                local standPos = stand:IsA("Model") and stand:GetPivot().Position or (stand:IsA("BasePart") and stand.Position) or nil
                if standPos and ((standPos.X < -5000 or standPos.Z < -5000) == inAbyss) then
                    inWorldShop = true
                    break
                end
            end
        end
        if inWorldShop then
            table.insert(shopEntries, entry)
        elseif ownedSet[id] then
            table.insert(extraEntries, entry)
        end
    end

    table.sort(shopEntries, function(a, b)
        if a.price ~= b.price then return a.price < b.price end
        return a.name < b.name
    end)
    table.sort(extraEntries, function(a, b) return a.name < b.name end)

    table.clear(PotionEngine.ShopPotionLabels)
    table.clear(PotionEngine.AllPotionLabels)
    for _, e in ipairs(shopEntries) do
        table.insert(PotionEngine.ShopPotionLabels, e.name)
        table.insert(PotionEngine.AllPotionLabels, e.name)
    end
    for _, e in ipairs(extraEntries) do
        table.insert(PotionEngine.AllPotionLabels, e.name)
    end

    if forceUiRefresh then
        if PotionEngine.BuyDropdown and type(PotionEngine.BuyDropdown.Refresh) == "function" then
            pcall(function() PotionEngine.BuyDropdown:Refresh(PotionEngine.ShopPotionLabels, Config.AutoBuyPotionTypes) end)
        end
        if PotionEngine.UseDropdown and type(PotionEngine.UseDropdown.Refresh) == "function" then
            pcall(function() PotionEngine.UseDropdown:Refresh(PotionEngine.AllPotionLabels, Config.AutoUsePotionTypes) end)
        end
    end
end

runIsolated(function()
    local okEx, Extras = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Extras) end)
    local okPx, ProxData = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Proximities) end)
    local shopMap = (okPx and type(ProxData) == "table" and type(ProxData.Shop) == "table") and ProxData.Shop or {}
    local potionsTbl = (okEx and type(Extras) == "table" and Extras.Items and Extras.Items.Potion) or {}

    for id, data in pairs(potionsTbl) do
        if type(data) == "table" and id ~= "AdLuckBoost" then
            local displayName = tostring(data.Name or id)
            local shopInfo = shopMap[id]
            local isHealth = id:find("^HealthPotion") ~= nil
            local entry = {
                id = id,
                name = displayName,
                price = shopInfo and tonumber(shopInfo.Price) or 0,
                maxLimit = shopInfo and tonumber(shopInfo.MaxPurchaseLimit) or 10,
                duration = tonumber(data.Duration) or 60,
                maxDuration = tonumber(data.MaxDuration) or tonumber(data.Duration) or 60,
                isHealth = isHealth,
                isShop = (shopInfo ~= nil) or (workspace:FindFirstChild("Proximity") and workspace.Proximity:FindFirstChild(id) ~= nil),
            }
            PotionEngine.IdToData[id] = entry
            PotionEngine.LabelToId[displayName] = id
            PotionEngine.LabelToId[id] = id
        end
    end
end)
PotionEngine.RefreshWorldCatalog(false)

function PotionEngine.SetStatus(title, desc)
    if PotionEngine.StatusLabel then
        pcall(function()
            PotionEngine.StatusLabel:SetTitle(title)
            if desc ~= nil then PotionEngine.StatusLabel:SetDesc(desc) end
        end)
    end
end

function PotionEngine.ResolveFamilyForWorld(potionId, shopOnly)
    local info = PotionEngine.IdToData[potionId]
    if not info then return nil end
    -- Prefer exact ID if sold in this realm (or if using owned inventory directly when shopOnly=false)
    if select(1, PotionEngine.FindShopStand(potionId)) ~= nil then
        return potionId
    end
    if not shopOnly and PotionEngine.GetOwnedCount(potionId) > 0 then
        return potionId
    end
    -- Cross-world fallback: e.g. MinerPotion1 -> MinerPotion2 on World 4 (Crimson Sakura)
    local family = potionId:match("^(.-)%d+$")
    if family and family ~= "" then
        local bestAlt, bestScore = nil, -math.huge
        for candId, candInfo in pairs(PotionEngine.IdToData) do
            if candId ~= potionId and candId:sub(1, #family) == family and candId:sub(#family + 1):match("^%d+$") then
                local hasStand = select(1, PotionEngine.FindShopStand(candId)) ~= nil
                local hasOwned = (not shopOnly) and PotionEngine.GetOwnedCount(candId) > 0
                if hasStand or hasOwned then
                    local tier = tonumber(candId:match("(%d+)$")) or 0
                    local sc = (hasStand and 100 or 0) + tier
                    if sc > bestScore then
                        bestScore = sc
                        bestAlt = candId
                    end
                end
            end
        end
        if bestAlt then return bestAlt end
    end
    if not shopOnly then return potionId end
    return info.isShop and potionId or nil
end

function PotionEngine.ResolveSelectedIds(selection, shopOnly)
    local ids = {}
    local seen = {}
    local function addName(raw)
        if type(raw) ~= "string" or raw == "" then return end
        local rawId = PotionEngine.LabelToId[raw] or raw
        local resolvedId = PotionEngine.ResolveFamilyForWorld(rawId, shopOnly)
        local info = resolvedId and PotionEngine.IdToData[resolvedId]
        if info and (not shopOnly or info.isShop) and not seen[resolvedId] then
            seen[resolvedId] = true
            table.insert(ids, resolvedId)
        end
    end
    if type(selection) == "table" then
        for k, v in pairs(selection) do
            if type(k) == "number" then
                addName(v)
            elseif v then
                addName(k)
            end
        end
    elseif type(selection) == "string" then
        addName(selection)
    end
    return ids
end

function PotionEngine.GetOwnedCount(potionId)
    local count = 0
    local pd = State.PlayerData or ResolvePlayerData()
    local misc = pd and pd.Data and pd.Data.Inventory and pd.Data.Inventory.Misc
    if type(misc) == "table" then
        for _, item in ipairs(misc) do
            if type(item) == "table" and item.Name == potionId then
                count = count + (tonumber(item.Quantity) or 1)
            end
        end
    end
    if count == 0 then
        local chTool = State.Character and State.Character:FindFirstChild(potionId)
        local bpTool = LocalPlayer and LocalPlayer:FindFirstChild("Backpack") and LocalPlayer.Backpack:FindFirstChild(potionId)
        local tool = chTool or bpTool
        if tool then
            count = tonumber(tool:GetAttribute("Count")) or 1
        end
    end
    return count
end

function PotionEngine.GetActiveRemaining(potionId)
    local pd = State.PlayerData or ResolvePlayerData()
    local active = pd and pd.Data and pd.Data.Potions
    if type(active) == "table" then
        return tonumber(active[potionId]) or 0
    end
    return 0
end

function PotionEngine.FindShopStand(potionId)
    local prox = workspace:FindFirstChild("Proximity")
    if not prox or not State.HRP then return nil, nil end
    local hrpPos = State.HRP.Position
    local inAbyss = (hrpPos.X < -5000 or hrpPos.Z < -5000)

    -- Iterate all children in Proximity so duplicate Main-Island vs AbyssLobby stands never shadow each other
    local bestStand, bestPos, bestDist = nil, nil, math.huge
    for _, stand in ipairs(prox:GetChildren()) do
        if stand.Name == potionId then
            local pos = stand:IsA("Model") and stand:GetPivot().Position or (stand:IsA("BasePart") and stand.Position)
            if pos then
                local standAbyss = (pos.X < -5000 or pos.Z < -5000)
                if standAbyss == inAbyss then
                    local d = (pos - hrpPos).Magnitude
                    if d < bestDist then
                        bestDist = d
                        bestStand = stand
                        bestPos = pos
                    end
                end
            end
        end
    end
    if not bestStand or not bestPos then return nil, nil end
    return bestStand, CFrame.new(bestPos + Vector3.new(0, 0.8, 3.0), bestPos)
end

function PotionEngine.RestoreFarmingTool()
    if not State.Character or not State.Humanoid or State.Humanoid.Health <= 0 then return end
    local restoreName = nil
    if Config.AutoMine then
        restoreName = "Pickaxe"
    elseif Config.AutoMobFarm or Config.AutoRaid then
        restoreName = "Weapon"
    end
    if restoreName then
        local bp = LocalPlayer and LocalPlayer:FindFirstChild("Backpack")
        local backTool = State.Character:FindFirstChild(restoreName) or (bp and bp:FindFirstChild(restoreName))
        if backTool and backTool.Parent ~= State.Character then
            pcall(function() State.Humanoid:EquipTool(backTool) end)
        end
    end
end

function PotionEngine.BuyNeededAtShop(forceNow, stayAtStand)
    if not State.HRP or not State.Humanoid or State.Humanoid.Health <= 0 then return 0 end
    if not Remotes.Purchase then return 0 end

    local selectedIds = PotionEngine.ResolveSelectedIds(Config.AutoBuyPotionTypes, true)
    if #selectedIds == 0 then return 0 end

    local pd = State.PlayerData or ResolvePlayerData()
    local gold = pd and pd.Data and tonumber(pd.Data.Gold) or 0
    local targetStock = math.clamp(math.floor(tonumber(Config.AutoBuyPotionStock) or 3), 1, 10)
    local used, cap = 0, 100
    if ForgeEngine and type(ForgeEngine.GetStashCapacityStatus) == "function" then
        local ok, u, c = pcall(ForgeEngine.GetStashCapacityStatus)
        if ok and type(u) == "number" and type(c) == "number" and c > 0 then
            used, cap = u, c
        end
    end
    local remainingCap = math.max(0, cap - used)
    if remainingCap <= 0 then
        PotionEngine.SetStatus("Potions: Stash Full", "Cannot buy more potions while stash is full")
        return 0
    end

    local shoppingList = {}
    local now = os.clock()
    for _, id in ipairs(selectedIds) do
        local info = PotionEngine.IdToData[id]
        local failUntil = PotionEngine.LastBuyFailAt[id] or 0
        if info and (forceNow or now >= failUntil) then
            local stand = select(1, PotionEngine.FindShopStand(id))
            if stand then
                local owned = PotionEngine.GetOwnedCount(id)
                local capNeed = math.min(targetStock, info.maxLimit) - owned
                local qty = math.min(capNeed, remainingCap)
                if info.price > 0 then
                    qty = math.min(qty, math.floor(gold / info.price))
                end
                if qty > 0 then
                    table.insert(shoppingList, { id = id, info = info, qty = qty })
                    remainingCap = remainingCap - qty
                    gold = gold - (qty * info.price)
                end
            end
        end
    end
    if #shoppingList == 0 then return 0 end

    local boughtTotal = 0
    if State.HRP and State.HRP.Anchored then
        if MiningEngine and MiningEngine.SafeUnanchor then
            MiningEngine.SafeUnanchor()
        else
            State.HRP.Anchored = false
        end
    end
    if State.Humanoid then State.Humanoid.AutoRotate = true end

    for _, order in ipairs(shoppingList) do
        if not ScriptRunning or (not Config.AutoBuyPotions and not forceNow) then break end
        if not State.HRP or not State.Humanoid or State.Humanoid.Health <= 0 then break end
        local stand, approachCF = PotionEngine.FindShopStand(order.id)
        if stand and approachCF then
            local standPos = stand:IsA("Model") and stand:GetPivot().Position or stand.Position
            if (State.HRP.Position - standPos).Magnitude > 11 then
                PotionEngine.SetStatus("Buying " .. order.info.name .. " x" .. order.qty, "Gliding to potion stand...")
                NavEngine.GlideTo(approachCF, Config.GlideSpeed or 55, true)
                task.wait(0.1)
            end
            -- If a shop roof or wall blocked final descent (e.g., Maria's Potion Shop roof at Y~34), settle cleanly at counter level
            if State.HRP and (State.HRP.Position - standPos).Magnitude > 11 then
                NavEngine.SafeTeleport(approachCF)
            end
            if State.HRP and (State.HRP.Position - standPos).Magnitude <= 18 then
                local okBuy, res = pcall(function()
                    return Remotes.Purchase:InvokeServer(order.id, order.qty)
                end)
                if okBuy and type(res) == "table" and res.Success then
                    boughtTotal = boughtTotal + order.qty
                    PotionEngine.SetStatus(
                        "Bought " .. order.info.name .. " x" .. order.qty,
                        "Owned: " .. tostring(PotionEngine.GetOwnedCount(order.id))
                    )
                else
                    PotionEngine.LastBuyFailAt[order.id] = os.clock() + 15.0
                    local reason = okBuy and (type(res) == "table" and res.WarnMessage or tostring(res)) or tostring(res)
                    PotionEngine.SetStatus("Buy Failed: " .. order.info.name, reason or "Rejected")
                end
            else
                PotionEngine.LastBuyFailAt[order.id] = os.clock() + 10.0
                PotionEngine.SetStatus("Buy Unreachable: " .. order.info.name, "Too far from stand (" .. string.format("%.1f", State.HRP and (State.HRP.Position - standPos).Magnitude or 999) .. " studs)")
            end
        end
    end

    if not stayAtStand and Config.AutoMine and Config.StayAtSavedPos and MiningEngine then
        local savedCF = MiningEngine.GetSavedSpotCFrame()
        if savedCF and State.HRP and (State.HRP.Position - savedCF.Position).Magnitude > 4 then
            NavEngine.GlideTo(savedCF, Config.GlideSpeed or 55, true)
        end
    end
    return boughtTotal
end

-- Full potion stand routine: Buy -> Drink to Max right at the stand -> Buy another spare set -> Resume mining
function PotionEngine.RunAutoBuyCycle(forceNow)
    if State.IsBuyingPotions or State.IsAutoForging or State.DrainingFullBag or State.SellingMobLoot then return 0 end
    if not State.HRP or not State.Humanoid or State.Humanoid.Health <= 0 then return 0 end
    if not Remotes.Purchase then return 0 end

    State.IsBuyingPotions = true
    local preTripCF = (not Config.StayAtSavedPos and (Config.AutoMine or Config.AutoMobFarm) and State.HRP) and State.HRP.CFrame or nil
    local boughtTotal = 0
    local okRun, errRun = pcall(function()
        -- 1. If we already have some in inventory, drink first if AutoUsePotions is on
        if Config.AutoUsePotions then
            PotionEngine.RunAutoUseCycle(false, true, true)
        end

        -- 2. Buy initial set at the shop stands (stay at stands)
        local b1 = PotionEngine.BuyNeededAtShop(forceNow, true)
        boughtTotal = boughtTotal + b1

        -- 3. Drink newly bought potions RIGHT AT THE STAND until stacked to max (before leaving!)
        if Config.AutoUsePotions then
            local drank = PotionEngine.RunAutoUseCycle(false, true, true)
            -- 4. Since we drank a few, buy another spare set right at the stand to save a future trip!
            if drank > 0 then
                task.wait(0.15)
                local b2 = PotionEngine.BuyNeededAtShop(forceNow, true)
                boughtTotal = boughtTotal + b2
            end
        end

        -- 5. Restore Pickaxe/Weapon once, then return to saved mining spot or pre-trip cave location
        PotionEngine.RestoreFarmingTool()
        if Config.AutoMine and Config.StayAtSavedPos and MiningEngine then
            local savedCF = MiningEngine.GetSavedSpotCFrame()
            if savedCF and State.HRP and (State.HRP.Position - savedCF.Position).Magnitude > 4 then
                NavEngine.GlideTo(savedCF, Config.GlideSpeed or 55, true)
            end
        elseif preTripCF and (Config.AutoMine or Config.AutoMobFarm) and State.HRP and (State.HRP.Position - preTripCF.Position).Magnitude > 14 then
            NavEngine.GlideTo(preTripCF, Config.GlideSpeed or 55, true)
        end
    end)
    State.IsBuyingPotions = false
    if not okRun then
        warn("[The Forge Potions]:", errRun)
    end
    return boughtTotal
end

function PotionEngine.UsePotion(potionId, allowDuringShop, skipRestoreTool)
    if not State.Character or not State.Humanoid or State.Humanoid.Health <= 0 then return false end
    if State.IsAutoForging or (not allowDuringShop and (State.IsBuyingPotions or State.IsBuyingHealthPotions)) or State.IsUsingPotion then return false end
    if CombatEngine and CombatEngine.IsBlockingOrParrying() then return false end
    if not Remotes.ToolActivated then return false end

    local statusFolder = State.Character:FindFirstChild("Status")
    if statusFolder and statusFolder:FindFirstChild("DisableBackpack") then
        return false
    end

    local now = os.clock()
    if (now - (PotionEngine.LastUseAttempt[potionId] or 0)) < 8.0 then
        return false
    end
    if PotionEngine.GetOwnedCount(potionId) <= 0 then
        return false
    end

    PotionEngine.LastUseAttempt[potionId] = now
    State.IsUsingPotion = true
    local confirmedUsed = false

    pcall(function()
        local prevTool = State.Character:FindFirstChildWhichIsA("Tool")
        local prevRem = PotionEngine.GetActiveRemaining(potionId)
        local prevOwned = PotionEngine.GetOwnedCount(potionId)
        local bp = LocalPlayer and LocalPlayer:FindFirstChild("Backpack")
        local tool = State.Character:FindFirstChild(potionId) or (bp and bp:FindFirstChild(potionId))

        if not tool and Remotes.EquipMisc then
            local okEq, eqState = pcall(function() return Remotes.EquipMisc:InvokeServer(potionId) end)
            -- CharacterService:EquipMisc toggles state: if server replied "UNEQUIPPED", invoke once more to equip
            if okEq and eqState == "UNEQUIPPED" then
                pcall(function() Remotes.EquipMisc:InvokeServer(potionId) end)
            end
            local t0 = os.clock()
            while (os.clock() - t0) < 1.5 do
                bp = LocalPlayer and LocalPlayer:FindFirstChild("Backpack")
                tool = (State.Character and State.Character:FindFirstChild(potionId)) or (bp and bp:FindFirstChild(potionId))
                if tool then break end
                task.wait(0.06)
            end
        end
        if not tool then return end

        if tool.Parent ~= State.Character and State.Humanoid then
            pcall(function() State.Humanoid:EquipTool(tool) end)
            local t0 = os.clock()
            while tool.Parent ~= State.Character and (os.clock() - t0) < 0.55 do
                task.wait(0.05)
            end
            task.wait(0.15)
        end

        if tool.Parent == State.Character then
            pcall(function()
                Remotes.ToolActivated:InvokeServer(potionId, false)
            end)

            -- Wait up to 2.5s for server drink animation & Replica.Data.Potions/Inventory update before swapping tools
            local drinkStart = os.clock()
            while ScriptRunning and (os.clock() - drinkStart) < 2.5 do
                if PotionEngine.GetActiveRemaining(potionId) > (prevRem + 1)
                    or PotionEngine.GetOwnedCount(potionId) < prevOwned then
                    confirmedUsed = true
                    break
                end
                task.wait(0.1)
            end
            task.wait(0.15)
        end

        -- Re-equip farming tool only after finishing drinking (or once at end of RunAutoUseCycle)
        if not skipRestoreTool and State.Character and State.Humanoid and State.Humanoid.Health > 0 then
            local restoreName = nil
            if Config.AutoMine then
                restoreName = "Pickaxe"
            elseif Config.AutoMobFarm or Config.AutoRaid then
                restoreName = "Weapon"
            elseif prevTool and prevTool.Parent and prevTool.Name ~= potionId then
                restoreName = prevTool.Name
            end
            if restoreName then
                bp = LocalPlayer and LocalPlayer:FindFirstChild("Backpack")
                local backTool = State.Character:FindFirstChild(restoreName) or (bp and bp:FindFirstChild(restoreName))
                if backTool and backTool.Parent ~= State.Character then
                    pcall(function() State.Humanoid:EquipTool(backTool) end)
                end
            end
        end
    end)

    State.IsUsingPotion = false

    if confirmedUsed then
        PotionEngine.LastUseAttempt[potionId] = 0 -- Allow immediate next stack drink when stacking to MaxDuration
        local info = PotionEngine.IdToData[potionId]
        PotionEngine.SetStatus("Used: " .. (info and info.name or potionId), "Active buff applied")
    end
    return confirmedUsed
end

function PotionEngine.RunAutoUseCycle(forceNow, allowDuringShop, skipRestoreTool)
    if not State.Character or not State.Humanoid or State.Humanoid.Health <= 0 then return 0 end
    if State.IsAutoForging or (not allowDuringShop and State.IsBuyingPotions) or State.DrainingFullBag or State.UsingPotionCycle then return 0 end
    if CombatEngine and CombatEngine.IsBlockingOrParrying() then return 0 end

    local selectedIds = PotionEngine.ResolveSelectedIds(Config.AutoUsePotionTypes, false)
    if #selectedIds == 0 then return 0 end

    State.UsingPotionCycle = true
    local usedCount = 0
    pcall(function()
        local hpPct = (State.Humanoid.Health / math.max(1, State.Humanoid.MaxHealth)) * 100
        local hpThreshold = tonumber(Config.AutoUseHealthPct) or 55

        for _, id in ipairs(selectedIds) do
            local info = PotionEngine.IdToData[id]
            if info and PotionEngine.GetOwnedCount(id) > 0 then
                if info.isHealth then
                    local rem = PotionEngine.GetActiveRemaining(id)
                    if (hpPct <= hpThreshold) and (rem <= 0.5) and PotionEngine.UsePotion(id, allowDuringShop, true) then
                        usedCount = usedCount + 1
                        task.wait(0.25)
                    end
                else
                    local dur = math.max(1, tonumber(info.duration) or 300)
                    local maxDur = math.max(dur, tonumber(info.maxDuration) or dur)
                    local stackCutoff = math.max(5.0, (maxDur - dur) + 5.0)
                    for _ = 1, 12 do
                        if not ScriptRunning or (not forceNow and not Config.AutoUsePotions) then break end
                        if PotionEngine.GetOwnedCount(id) <= 0 then break end
                        local rem = PotionEngine.GetActiveRemaining(id)
                        if rem > stackCutoff then break end
                        if not PotionEngine.UsePotion(id, allowDuringShop, true) then break end
                        usedCount = usedCount + 1
                        task.wait(0.25)
                    end
                end
            end
        end

        if usedCount > 0 and not skipRestoreTool then
            PotionEngine.RestoreFarmingTool()
        end
    end)
    State.UsingPotionCycle = false
    return usedCount
end

function PotionEngine.GetTotalHealthPotionsOwned()
    local total = 0
    for _, id in ipairs({ "HealthPotion3", "HealthPotion2", "HealthPotion1" }) do
        total = total + PotionEngine.GetOwnedCount(id)
    end
    return total
end

function PotionEngine.UseBestHealthPotion()
    if not State.Character or not State.Humanoid or State.Humanoid.Health <= 0 then return false end
    for _, id in ipairs({ "HealthPotion3", "HealthPotion2", "HealthPotion1" }) do
        if PotionEngine.GetOwnedCount(id) > 0 and PotionEngine.GetActiveRemaining(id) <= 0.5 then
            if PotionEngine.UsePotion(id, true, false) then
                return true, id
            end
        end
    end
    return false, nil
end

function PotionEngine.EnsureHealthPotionsStock(targetStock, isManual)
    targetStock = math.clamp(math.floor(tonumber(targetStock) or 5), 1, 10)
    if State.IsBuyingHealthPotions then return 0 end
    if State.IsInRaid then return 0 end
    if RaidEngine and (
        RaidEngine.IsInActiveRun()
        or RaidEngine.GetActiveBossArena() ~= nil
        or (RaidEngine.IsNearActiveBossMob and RaidEngine.IsNearActiveBossMob(450))
        or os.clock() < (RaidEngine.ArenaLoadingUntil or 0)
        or (not isManual and RaidEngine.LastBossArenaAt and (os.clock() - RaidEngine.LastBossArenaAt) < 8.0)
        or (not isManual and RaidEngine.LastPartyActivateAt and (os.clock() - RaidEngine.LastPartyActivateAt) < 15.0)
    ) then
        return 0
    end
    if not State.HRP or not State.Humanoid or State.Humanoid.Health <= 0 then return 0 end
    if not Remotes.Purchase then return 0 end
    local ownedTotal = PotionEngine.GetTotalHealthPotionsOwned()
    local needed = targetStock - ownedTotal
    if needed <= 0 then return 0 end

    PotionEngine.RefreshWorldCatalog(false)
    local bestId, bestStand, bestInfo = nil, nil, nil
    for _, id in ipairs({ "HealthPotion3", "HealthPotion2", "HealthPotion1" }) do
        local info = PotionEngine.IdToData[id]
        local failUntil = PotionEngine.LastBuyFailAt[id] or 0
        if info and os.clock() >= failUntil then
            local stand = select(1, PotionEngine.FindShopStand(id))
            if stand then
                bestId, bestStand, bestInfo = id, stand, info
                break
            end
        end
    end
    if not bestId then return 0 end

    local bestStandPos = bestStand:IsA("Model") and bestStand:GetPivot().Position or bestStand.Position
    if (State.HRP.Position - bestStandPos).Magnitude > 2600 then
        -- Player is in a remote boss arena / out-of-island zone; never glide out of bounds to buy potions
        return 0
    end

    local pd = State.PlayerData or ResolvePlayerData()
    local gold = pd and pd.Data and tonumber(pd.Data.Gold) or 0
    local maxHold = tonumber(bestInfo.maxLimit) or 10
    local qty = math.min(needed, math.max(0, maxHold - PotionEngine.GetOwnedCount(bestId)))
    if (bestInfo.price or 0) > 0 then
        qty = math.min(qty, math.floor(gold / bestInfo.price))
    end
    if qty <= 0 then return 0 end

    -- Wait briefly if Mining AutoBuyPotions is finishing a purchase so glides never cancel each other
    local waitT0 = os.clock()
    while State.IsBuyingPotions and (os.clock() - waitT0) < 6.0 do
        task.wait(0.2)
    end

    State.IsBuyingHealthPotions = true
    local bought = 0
    pcall(function()
        if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
        if State.Humanoid then State.Humanoid.AutoRotate = true end
        local standPos = bestStand:IsA("Model") and bestStand:GetPivot().Position or bestStand.Position
        local approachCF = CFrame.new(standPos + Vector3.new(0, 1.5, 2.5), standPos)
        if (State.HRP.Position - standPos).Magnitude > 9 then
            PotionEngine.SetStatus("Buying " .. bestInfo.name .. " x" .. qty, "Restocking Health Potions before run...")
            NavEngine.GlideTo(approachCF, Config.GlideSpeed or 55, true)
            if State.HRP then
                State.HRP.AssemblyLinearVelocity = Vector3.new(0, 1.5, 0)
            end
            task.wait(0.15)
        end
        if State.HRP and (State.HRP.Position - standPos).Magnitude <= 14
            and not (RaidEngine and (RaidEngine.IsInActiveRun() or RaidEngine.GetActiveBossArena() ~= nil or (RaidEngine.IsNearActiveBossMob and RaidEngine.IsNearActiveBossMob(450)))) then
            local okBuy, res = pcall(function() return Remotes.Purchase:InvokeServer(bestId, qty) end)
            if okBuy and type(res) == "table" and res.Success then
                bought = qty
                task.wait(0.15)
                PotionEngine.SetStatus("Restocked " .. bestInfo.name .. " x" .. qty, "Owned Health Pots: " .. tostring(PotionEngine.GetTotalHealthPotionsOwned()))
                local uiLib = Library or (typeof(getgenv) == "function" and getgenv().Library) or _G.Library
                if uiLib and type(uiLib.Notify) == "function" then
                    pcall(function()
                        uiLib:Notify({ Title = "Health Potions Bought", Desc = string.format("Purchased %s x%d (Total: %d)", bestInfo.name, qty, PotionEngine.GetTotalHealthPotionsOwned()), Duration = 3 })
                    end)
                end
            else
                PotionEngine.LastBuyFailAt[bestId] = os.clock() + 10.0
                local warnMsg = (type(res) == "table" and res.WarnMessage) or "Purchase rejected"
                PotionEngine.SetStatus("Health Pot Buy Failed", tostring(warnMsg))
            end
        end
    end)
    State.IsBuyingHealthPotions = false
    return bought
end

function PotionEngine.HasBuyableSelectedOrder()
    if not State.HRP or not State.Humanoid or State.Humanoid.Health <= 0 then return false end
    if not Remotes.Purchase then return false end
    local selectedIds = PotionEngine.ResolveSelectedIds(Config.AutoBuyPotionTypes, true)
    if #selectedIds == 0 then return false end
    local pd = State.PlayerData or ResolvePlayerData()
    local gold = pd and pd.Data and tonumber(pd.Data.Gold) or 0
    local targetStock = math.clamp(math.floor(tonumber(Config.AutoBuyPotionStock) or 3), 1, 10)
    local used, cap = 0, 100
    if ForgeEngine and type(ForgeEngine.GetStashCapacityStatus) == "function" then
        local ok, u, c = pcall(ForgeEngine.GetStashCapacityStatus)
        if ok and type(u) == "number" and type(c) == "number" and c > 0 then
            used, cap = u, c
        end
    end
    if math.max(0, cap - used) <= 0 then return false end
    local now = os.clock()
    for _, id in ipairs(selectedIds) do
        local info = PotionEngine.IdToData[id]
        if info and now >= (PotionEngine.LastBuyFailAt[id] or 0) then
            if select(1, PotionEngine.FindShopStand(id)) ~= nil then
                local capNeed = math.min(targetStock, info.maxLimit) - PotionEngine.GetOwnedCount(id)
                local qty = capNeed
                if (info.price or 0) > 0 then
                    qty = math.min(qty, math.floor(gold / info.price))
                end
                if qty > 0 then
                    return true
                end
            end
        end
    end
    return false
end

-- Checks specifically if any selected non-health shop potion has NO active effect AND 0 owned (ignoring totems/other buffs!)
function PotionEngine.HasMissingSelectedBuff()
    local buyIds = PotionEngine.ResolveSelectedIds(Config.AutoBuyPotionTypes, true)
    local now = os.clock()
    for _, id in ipairs(buyIds) do
        local info = PotionEngine.IdToData[id]
        if info and not info.isHealth and now >= (PotionEngine.LastBuyFailAt[id] or 0) then
            local stand = select(1, PotionEngine.FindShopStand(id))
            if stand then
                local owned = PotionEngine.GetOwnedCount(id)
                local rem = PotionEngine.GetActiveRemaining(id)
                if rem <= 10 and owned <= 0 then
                    return true, id
                end
            end
        end
    end
    return false, nil
end

-- True only when ALL selected non-health AutoBuy potions still have active buffs (> 10s), ignoring totems/unselected buffs
function PotionEngine.AreBuffsActive()
    local buyIds = PotionEngine.ResolveSelectedIds(Config.AutoBuyPotionTypes, true)
    local hasNonHealth = false
    for _, id in ipairs(buyIds) do
        local info = PotionEngine.IdToData[id]
        if info and not info.isHealth then
            hasNonHealth = true
            if PotionEngine.GetActiveRemaining(id) <= 10 then
                return false
            end
        end
    end
    return hasNonHealth
end

function PotionEngine.CheckOnAutoMineStart()
    if PotionEngine.CheckingOnStart then return end
    if Config.AutoRaid or State.IsInRaid or State.IsBuyingHealthPotions then return end
    PotionEngine.CheckingOnStart = true
    State.CheckingPotionsOnStart = true
    local ok, err = pcall(function()
        PotionEngine.RefreshWorldCatalog(false)

        if Config.AutoUsePotions then
            pcall(PotionEngine.RunAutoUseCycle, false)
        end

        if Config.AutoBuyPotions then
            local buyIds = PotionEngine.ResolveSelectedIds(Config.AutoBuyPotionTypes, true)
            local targetStock = math.clamp(math.floor(tonumber(Config.AutoBuyPotionStock) or 3), 1, 10)
            local needsRestock = false
            for _, id in ipairs(buyIds) do
                local info = PotionEngine.IdToData[id]
                local owned = PotionEngine.GetOwnedCount(id)
                if owned <= 0 or (info and not info.isHealth and PotionEngine.GetActiveRemaining(id) <= 10 and owned < targetStock) then
                    needsRestock = true
                    break
                end
            end

            local hasMissingBuff = PotionEngine.HasMissingSelectedBuff()
            if needsRestock then
                if not hasMissingBuff and Config.AutoBuyAfterBuffsExpire and PotionEngine.AreBuffsActive() then
                    PotionEngine.SetStatus("Buffs Active", "All selected potion buffs active; waiting for buffs before buying more")
                else
                    PotionEngine.LastBuyCheckAt = os.clock()
                    State.TargetRock = nil
                    State.CurrentRock = nil
                    if NavEngine then NavEngine.CachedRock = nil end
                    PotionEngine.RunAutoBuyCycle(false)
                end
            end
        elseif Config.AutoUsePotions then
            local useIds = PotionEngine.ResolveSelectedIds(Config.AutoUsePotionTypes, false)
            local totalOwned = 0
            local missingAny = false
            for _, id in ipairs(useIds) do
                local info = PotionEngine.IdToData[id]
                totalOwned = totalOwned + PotionEngine.GetOwnedCount(id)
                if info and not info.isHealth and PotionEngine.GetActiveRemaining(id) <= 10 then
                    missingAny = true
                end
            end
            if #useIds > 0 and totalOwned == 0 and missingAny then
                PotionEngine.SetStatus("Potions: 0 Owned", "Enable 'Auto Buy Potions' in Shop to restock!")
                pcall(Notify, "Potions", "You have 0 selected potions in inventory. Enable 'Auto Buy Potions' in Shop to restock!", 3)
            end
        end
    end)
    PotionEngine.CheckingOnStart = false
    State.CheckingPotionsOnStart = false
    if not ok then
        warn("[PotionEngine] CheckOnAutoMineStart error:", err)
    end
end

-- Background Auto-Buy & Auto-Use Potions Loop (isolated from Auto Raid / Health Potion restocking)
task.spawn(function()
    while ScriptRunning do
        if PotionEngine.CheckingOnStart then
            task.wait(0.25)
            continue
        end
        local hrp = State.HRP
        local inAbyss = hrp and (hrp.Position.X < -5000 or hrp.Position.Z < -5000) or false
        if PotionEngine.LastAbyssState ~= inAbyss then
            PotionEngine.RefreshWorldCatalog(true)
        end
        local raidBusy = Config.AutoRaid or State.IsInRaid or State.IsBuyingHealthPotions
            or (RaidEngine and RaidEngine.IsInActiveRun())
        if raidBusy then
            task.wait(1.0)
            continue
        end
        -- 1. Always drink any potions already owned in inventory first
        if Config.AutoUsePotions then
            pcall(PotionEngine.RunAutoUseCycle)
        end
        -- 2. Restock immediately if any selected buff is missing + 0 owned, or when interval/buff conditions pass
        if Config.AutoBuyPotions then
            local missingBuff = PotionEngine.HasMissingSelectedBuff()
            local intervalSec = math.max(8, (tonumber(Config.AutoBuyPotionIntervalMin) or 0) * 60)
            local isFirstCheck = (PotionEngine.LastBuyCheckAt == nil or PotionEngine.LastBuyCheckAt == 0)
            local elapsed = os.clock() - (PotionEngine.LastBuyCheckAt or 0)
            if missingBuff or isFirstCheck or elapsed >= intervalSec then
                if not missingBuff and Config.AutoBuyAfterBuffsExpire and PotionEngine.AreBuffsActive() then
                    -- Wait until selected active buff timers finish before leaving AutoMine
                    PotionEngine.LastBuyCheckAt = os.clock() - intervalSec + 5.0
                elseif PotionEngine.HasBuyableSelectedOrder() then
                    PotionEngine.LastBuyCheckAt = os.clock()
                    State.TargetRock = nil
                    State.CurrentRock = nil
                    if NavEngine then NavEngine.CachedRock = nil end
                    PotionEngine.RunAutoBuyCycle(false)
                else
                    PotionEngine.LastBuyCheckAt = os.clock() - intervalSec + 5.0
                end
            end
        end
        task.wait(1.5)
    end
end)

-- ===================================================================
-- 12. MULTI-MAP FAST TRAVEL, ISLANDS & WAYPOINTS ENGINE (Dynamic per World)
-- ===================================================================
TeleportEngine = TeleportEngine or {
    WaypointStatusLabel = nil,
}

function TeleportEngine.GetInstanceCFrame(inst)
    if not inst then return nil end
    if inst:IsA("Model") then
        local cf = inst:GetPivot()
        return CFrame.new(cf.Position + Vector3.new(0, 3.8, 0))
    elseif inst:IsA("BasePart") then
        return CFrame.new(inst.Position + Vector3.new(0, 3.8, 0))
    end
    local p = inst:FindFirstChildWhichIsA("BasePart", true)
    if p then
        return CFrame.new(p.Position + Vector3.new(0, 3.8, 0))
    end
    return nil
end


function TeleportEngine.GetCurrentIslandInfo()
    local res = { index = -1, name = "Current Map", placeId = game.PlaceId, data = nil }
    pcall(function()
        local Islands = safeRequire(ReplicatedStorage.Shared.Data.Islands)
        for idx, isl in ipairs(Islands.Data or {}) do
            if isl.PlaceId == game.PlaceId then
                res.index = idx
                res.name = isl.Name
                res.data = isl
                break
            end
        end
    end)
    return res
end

function TeleportEngine.GetIslandsList()
    local list = {}
    ResolvePlayerData()
    local pData = State.PlayerData and State.PlayerData.Data
    pcall(function()
        local Islands = safeRequire(ReplicatedStorage.Shared.Data.Islands)
        for idx, isl in ipairs(Islands.Data or {}) do
            local allMet = true
            if pData and type(Islands.GetRequirementStatus) == "function" then
                local okSt, st = pcall(Islands.GetRequirementStatus, isl.Name, pData)
                if okSt and type(st) == "table" then
                    allMet = (st.AllMet == true)
                end
            end
            table.insert(list, {
                index = idx,
                name = isl.Name,
                placeId = isl.PlaceId,
                unlocked = allMet,
                isCurrent = (isl.PlaceId == game.PlaceId),
                description = isl.Description or "",
            })
        end
    end)
    return list
end

function TeleportEngine.TeleportToIsland(islandName)
    if not islandName or islandName == "" then return false end
    local ok = false
    pcall(function()
        local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
        local PS = KnitPkg.GetService("PortalService")
        if PS and type(PS.TeleportToIsland) == "function" then
            PS:TeleportToIsland(islandName):await()
            ok = true
        end
    end)
    if not ok and Remotes.TeleportToIsland then
        pcall(function()
            Remotes.TeleportToIsland:InvokeServer(islandName)
            ok = true
        end)
    end
    return ok
end

function TeleportEngine.IsWaypointUnlocked(wpName)
    local unlocked = false
    pcall(function()
        local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
        local WC = SafeGetController("WaypointController", KnitPkg)
        if WC and type(WC.isWaypointUnlocked) == "function" then
            unlocked = not not WC:isWaypointUnlocked(wpName)
            return
        end
        ResolvePlayerData()
        local cur = TeleportEngine.GetCurrentIslandInfo()
        local pd = State.PlayerData and State.PlayerData.Data
        local WaypointsData = safeRequire(ReplicatedStorage.Shared.Data.Waypoints)
        local mapTbl = pd and pd.WaypointsUnlocked and pd.WaypointsUnlocked[tostring(cur.index)]
        if mapTbl and mapTbl[wpName] then
            unlocked = true
        elseif WaypointsData and WaypointsData.Defaults and WaypointsData.Defaults[cur.index] then
            unlocked = (table.find(WaypointsData.Defaults[cur.index], wpName) ~= nil)
        end
    end)
    return unlocked
end

function TeleportEngine.GetWaypointsList()
    local list = {}
    local wpFolder = workspace:FindFirstChild("Waypoints")
    if not wpFolder then return list end

    local WaypointsData = nil
    pcall(function() WaypointsData = safeRequire(ReplicatedStorage.Shared.Data.Waypoints) end)
    ResolvePlayerData()
    local pd = State.PlayerData and State.PlayerData.Data

    for _, wp in ipairs(wpFolder:GetChildren()) do
        local name = wp.Name
        local cf = TeleportEngine.GetInstanceCFrame(wp)
        local price = (WaypointsData and WaypointsData.Prices and WaypointsData.Prices[name]) or 0
        local freeQuest = WaypointsData and WaypointsData.FreeWithQuest and WaypointsData.FreeWithQuest[name]
        if freeQuest and pd and pd.Quests and pd.Quests[freeQuest] then
            price = 0
        end
        table.insert(list, {
            name = name,
            inst = wp,
            cframe = cf,
            unlocked = TeleportEngine.IsWaypointUnlocked(name),
            price = price,
        })
    end

    table.sort(list, function(a, b)
        if a.price ~= b.price then return a.price < b.price end
        return a.name < b.name
    end)
    return list
end

function TeleportEngine.UnlockWaypoint(wpName, allowGlide)
    if not wpName or wpName == "" then return false, "invalid" end
    if TeleportEngine.IsWaypointUnlocked(wpName) then
        return true, "already"
    end

    local function invokeUnlock()
        local invoked = false
        pcall(function()
            local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
            local WS = KnitPkg.GetService("WaypointService")
            if WS and type(WS.RequestUnlock) == "function" then
                WS:RequestUnlock(wpName):await()
                invoked = true
            end
        end)
        if not invoked and Remotes.RequestUnlock then
            pcall(function()
                Remotes.RequestUnlock:InvokeServer(wpName)
                invoked = true
            end)
        end
        task.wait(0.25)
        return TeleportEngine.IsWaypointUnlocked(wpName)
    end

    if invokeUnlock() then
        return true, "remote"
    end

    if allowGlide ~= false then
        local wpFolder = workspace:FindFirstChild("Waypoints")
        local wp = wpFolder and wpFolder:FindFirstChild(wpName)
        local cf = TeleportEngine.GetInstanceCFrame(wp)
        if wp and cf then
            NavEngine.GlideTo(cf, Config.GlideSpeed or 55, true)
            task.wait(0.2)
            if invokeUnlock() then
                return true, "glide_remote"
            end
            -- ProximityPrompt fallback
            local prompt = wp:FindFirstChildWhichIsA("ProximityPrompt", true)
            if prompt then
                if typeof(fireproximityprompt) == "function" then
                    pcall(function() fireproximityprompt(prompt, 0) end)
                else
                    pcall(function()
                        prompt:InputHoldBegin()
                        task.wait(prompt.HoldDuration or 0.25)
                        prompt:InputHoldEnd()
                    end)
                end
                task.wait(0.3)
            end
        end
    end

    return TeleportEngine.IsWaypointUnlocked(wpName), "attempted"
end

function TeleportEngine.UnlockAllWaypoints()
    local wps = TeleportEngine.GetWaypointsList()
    local unlockedNow = 0
    local already = 0
    local failed = 0
    for _, wp in ipairs(wps) do
        if wp.unlocked then
            already = already + 1
        else
            local ok = TeleportEngine.UnlockWaypoint(wp.name, true)
            if ok then
                unlockedNow = unlockedNow + 1
            else
                failed = failed + 1
            end
        end
    end
    return unlockedNow, already, failed, #wps
end

function TeleportEngine.TeleportToWaypoint(wpName)
    if not wpName or wpName == "" then return false end
    local wpFolder = workspace:FindFirstChild("Waypoints")
    local wp = wpFolder and wpFolder:FindFirstChild(wpName)
    local cf = TeleportEngine.GetInstanceCFrame(wp)

    if TeleportEngine.IsWaypointUnlocked(wpName) then
        local done = false
        local wcRef = nil
        runAsGameThread(function()
            local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
            local WC = SafeGetController("WaypointController", KnitPkg)
            wcRef = WC
            if WC and type(WC.Teleport) == "function" then
                WC:Teleport(wpName)
                done = true
            end
        end, 2.0)
        if not done and Remotes.RequestTeleport then
            pcall(function()
                Remotes.RequestTeleport:InvokeServer(wpName)
                done = true
            end)
        end
        if done then
            -- Wait cleanly for server waypoint teleport completion so multi-hop callers don't overwrite HRP mid-warp
            local wpPos = cf and cf.Position
            local waitStart = os.clock()
            while (os.clock() - waitStart) < 4.0 do
                task.wait(0.15)
                local settled = (wcRef == nil or not wcRef.IsTeleporting)
                if wpPos and State.HRP and (State.HRP.Position - wpPos).Magnitude <= 50 and settled then
                    break
                end
                if settled and (os.clock() - waitStart) >= 1.2 then
                    break
                end
            end
            task.wait(0.15)
            return true
        end
    end
    if cf then
        return NavEngine.GlideTo(cf, Config.GlideSpeed or 55, true)
    end
    return false
end

function TeleportEngine.GetZoneCFrame(zoneInst)
    if not zoneInst then return nil end
    local loc = zoneInst:FindFirstChild("SpawnLocation") or zoneInst:FindFirstChildWhichIsA("SpawnLocation", true)
    if loc and loc:IsA("BasePart") then
        return CFrame.new(loc.Position + Vector3.new(0, 4.0, 0))
    end
    local sum = Vector3.zero
    local count = 0
    for _, ch in ipairs(zoneInst:GetDescendants()) do
        if ch:IsA("BasePart") and (ch.Name == "Hitbox" or ch.Parent:GetAttribute("Health") ~= nil) then
            sum = sum + ch.Position
            count = count + 1
            if count >= 20 then break end
        end
    end
    if count > 0 then
        return CFrame.new((sum / count) + Vector3.new(0, 6.0, 0))
    end
    return nil
end


function TeleportEngine.GetMapLocations()
    local curPos = State.HRP and State.HRP.Position
    local inAbyss = curPos and (curPos.X < -5000 or curPos.Z < -5000) or false
    local quickStations = {}
    local zoneButtons = {}
    local allNames = {}
    local lookup = {}

    local function registerEntry(label, cf, groupList)
        if not label or not cf then return end
        if not lookup[label] then
            lookup[label] = cf
            table.insert(allNames, label)
            if groupList then
                table.insert(groupList, { name = label, cframe = cf })
            end
        end
    end

    -- 1. Mining Caves & Zones from workspace.Rocks (works dynamically on Islands 1, 2, 3, 4)
    local rocksFolder = workspace:FindFirstChild("Rocks")
    if rocksFolder then
        for _, zone in ipairs(rocksFolder:GetChildren()) do
            if zone.Name ~= "BossShields" and not zone.Name:find("Closed", 1, true) then
                local cf = TeleportEngine.GetZoneCFrame(zone)
                if cf then
                    local niceName = zone.Name:gsub("^Island%d+", "")
                    if niceName == "" then niceName = zone.Name end
                    registerEntry("Cave: " .. niceName, cf, zoneButtons)
                end
            end
        end
    end
    if not inAbyss and NavEngine and NavEngine.IsSecretMazeWorld and NavEngine.IsSecretMazeWorld() then
        registerEntry("Secret Maze Entrance (Dock)", CFrame.new(210, 18, -1680), zoneButtons)
        registerEntry("Secret Maze Shaft (Bottom)", CFrame.new(192, -120, -1815), zoneButtons)
    end

    -- 2. Key Core Stations + All Proximity NPCs/Shops on current map from workspace.Proximity
    local prox = workspace:FindFirstChild("Proximity")
    local keyStations = {
        "Forge", "AbyssForge", "Marbles", "Greedy Cey", "Enhancer", "Runemaker",
        "QuestBoard", "VaultKeeper", "Captain Rowan", "Wizard", "AbyssReturn", "AbyssShop"
    }
    local keySet = {}
    for _, k in ipairs(keyStations) do keySet[k] = true end

    if prox then
        -- Add core stations first in deterministic order
        for _, k in ipairs(keyStations) do
            local inst = prox:FindFirstChild(k)
            local cf = TeleportEngine.GetInstanceCFrame(inst)
            if cf then
                local isAbyssPoint = (cf.Position.X < -5000 or cf.Position.Z < -5000)
                if isAbyssPoint == inAbyss then
                    registerEntry(k, cf, quickStations)
                end
            end
        end

        -- Add Craft Stations, Gates, Boss Queues, and every NPC / Pickaxe / Potion / Shop dynamically
        local extraProx = {}
        for _, inst in ipairs(prox:GetChildren()) do
            if not keySet[inst.Name] then
                local cf = TeleportEngine.GetInstanceCFrame(inst)
                if cf then
                    local isAbyssPoint = (cf.Position.X < -5000 or cf.Position.Z < -5000)
                    if isAbyssPoint == inAbyss then
                        table.insert(extraProx, { name = inst.Name, cframe = cf, bbName = inst:GetAttribute("BillboardName") })
                    end
                end
            end
        end
        table.sort(extraProx, function(a, b) return a.name < b.name end)
        for _, entry in ipairs(extraProx) do
            if entry.name == "MazeMerchant" or entry.bbName == "Fungi" then
                registerEntry("Fungi (" .. entry.name .. ")", entry.cframe, nil)
            end
            if entry.name:find("Craft", 1, true) or entry.name:find("Pickaxe", 1, true)
                or entry.name:find("Door", 1, true) or entry.name:find("Gate", 1, true)
                or entry.name:find("CreateParty", 1, true) then
                registerEntry(entry.name, entry.cframe, quickStations)
            else
                registerEntry(entry.name, entry.cframe, nil)
            end
        end
    end

    table.sort(allNames)
    return {
        quickStations = quickStations,
        zoneButtons = zoneButtons,
        allNames = allNames,
        lookup = lookup,
    }
end

function NavEngine.ResolvePOICFrame(poiData)
    if type(poiData) == "Vector3" then
        return CFrame.new(poiData)
    elseif typeof(poiData) == "CFrame" then
        return poiData
    elseif type(poiData) == "string" then
        local locs = TeleportEngine.GetMapLocations()
        if locs.lookup[poiData] then return locs.lookup[poiData] end
        local prox = workspace:FindFirstChild("Proximity")
        local inst = prox and prox:FindFirstChild(poiData)
        return TeleportEngine.GetInstanceCFrame(inst)
    end
    if type(poiData) == "table" then
        if poiData.prox then
            local prox = workspace:FindFirstChild("Proximity")
            local inst = prox and prox:FindFirstChild(poiData.prox)
            local cf = TeleportEngine.GetInstanceCFrame(inst)
            if cf then return cf end
        elseif poiData.zone then
            local rocks = workspace:FindFirstChild("Rocks")
            local z = rocks and rocks:FindFirstChild(poiData.zone)
            local cf = TeleportEngine.GetZoneCFrame(z)
            if cf then return cf end
        end
        if poiData.pos then
            return CFrame.new(poiData.pos)
        end
    end
    return nil
end


-- ===================================================================
-- 12B. BEST RUNE ANALYZER, ATTACH COST & AUTO-PUT RUNES ENGINE
-- ===================================================================
local RuneEngine = {
    EquipLabelToObj = {},
    EquipLabels = {},
    RuneLabelToObj = {},
    RuneLabels = { "Auto: Best For Focus" },
    EquipDropdown = nil,
    RuneDropdown = nil,
    BestRuneLabel = nil,
    BestPickaxeRuneLabel = nil,
    CostInfoLabel = nil,
    ActionStatusLabel = nil,
    SellStatusLabel = nil,
    IsAttaching = false,
    LastAutoAttachAt = 0,
    PriceCache = {},
    RuneSellTypes = {},
    SellDisplayToIds = {},
    IdToDisplayName = {},
}

RuneEngine.FocusOptions = {
    "All (Highest % Overall)",
    "Fire",
    "Explosion",
    "Poison",
    "LifeSteal (Drain)",
    "Critical Chance",
    "Critical Damage",
    "Attack Speed",
    "Berserker",
    "Thorn",
    "Fracture (Stun)",
    "Movement / Dash",
    "Mining",
}

function RuneEngine.FormatStatShort(statKey, val, isRollPct)
    local clean = statKey:gsub("([a-z])([A-Z])", "%1 %2")
    return string.format("%s: %s (%d%% roll)", clean, tostring(val), math.round((tonumber(isRollPct) or 0) * 100))
end

function RuneEngine.MatchesFocus(traitId, statKey, traitFocus)
    if not traitFocus or traitFocus == "All (Highest % Overall)" then
        return true
    end
    local tid = tostring(traitId or ""):lower()
    local sk = tostring(statKey or ""):lower()
    if traitFocus == "Fire" then return tid:find("fire") ~= nil or sk:find("fire") ~= nil or sk:find("burn") ~= nil end
    if traitFocus == "Explosion" then return tid:find("explosion") ~= nil or sk:find("explosion") ~= nil end
    if traitFocus == "Poison" then return tid:find("poison") ~= nil or sk:find("poison") ~= nil end
    if traitFocus == "LifeSteal (Drain)" then return tid:find("lifesteal") ~= nil or sk:find("lifesteal") ~= nil end
    if traitFocus == "Critical Chance" then return tid:find("critical") ~= nil or sk:find("criticalchance") ~= nil end
    if traitFocus == "Critical Damage" then return sk:find("criticaldamage") ~= nil end
    if traitFocus == "Attack Speed" then return tid:find("attackspeed") ~= nil or sk:find("attackspeed") ~= nil end
    if traitFocus == "Berserker" then return tid:find("berserker") ~= nil or sk:find("berserker") ~= nil end
    if traitFocus == "Thorn" then return tid:find("thorn") ~= nil or sk:find("thorn") ~= nil end
    if traitFocus == "Fracture (Stun)" then return tid:find("stun") ~= nil or sk:find("stundamage") ~= nil end
    if traitFocus == "Movement / Dash" then return tid:find("dash") ~= nil or tid:find("stamina") ~= nil or sk:find("dash") ~= nil end
    if traitFocus == "Mining" then return tid:find("mine") ~= nil or tid:find("luck") ~= nil or tid:find("drop") ~= nil or sk:find("mine") ~= nil or sk:find("luck") ~= nil or sk:find("drop") ~= nil end
    return true
end

function RuneEngine.AnalyzeRune(runeObj, traitFocus)
    if type(runeObj) ~= "table" or not runeObj.Id or type(runeObj.Traits) ~= "table" then
        return nil
    end
    local okR, Runes = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Runes) end)
    local runeDef = okR and Runes and Runes.Runes and Runes.Runes[runeObj.Id]
    if not runeDef then return nil end

    local totalPct = 0
    local statCount = 0
    local maxPct = 0
    local focusMaxPct = -1
    local focusAvgSum = 0
    local focusAvgCount = 0
    local statSummaries = {}
    local maxTier = 1

    for _, tr in ipairs(runeObj.Traits) do
        if type(tr) == "table" and tr.Id then
            local tier = tonumber(tr.Tier) or 1
            if tier > maxTier then maxTier = tier end
            for k, rawVal in pairs(tr) do
                if k ~= "Id" and k ~= "Tier" and k ~= "Whitelist" and type(rawVal) == "number" then
                    local scaledVal = rawVal
                    if okR and Runes and type(Runes.GetStatValue) == "function" then
                        scaledVal = Runes:GetStatValue(k, rawVal, tier, tr.Id) or rawVal
                    end
                    if k:lower():find("count", 1, true) then
                        local clean = k:gsub("([a-z])([A-Z])", "%1 %2")
                        table.insert(statSummaries, string.format("%s: %s", clean, tostring(scaledVal)))
                    else
                        local rollPct = math.clamp(rawVal, 0, 1) * 100
                        totalPct = totalPct + rollPct
                        statCount = statCount + 1
                        if rollPct > maxPct then maxPct = rollPct end
                        if RuneEngine.MatchesFocus(tr.Id, k, traitFocus) then
                            if rollPct > focusMaxPct then focusMaxPct = rollPct end
                            focusAvgSum = focusAvgSum + rollPct
                            focusAvgCount = focusAvgCount + 1
                        end
                        table.insert(statSummaries, RuneEngine.FormatStatShort(k, scaledVal, rawVal))
                    end
                end
            end
        end
    end

    if statCount == 0 and #statSummaries == 0 then return nil end
    local avgPct = (statCount > 0) and (totalPct / statCount) or 0
    local focusScore = (focusAvgCount > 0) and (focusMaxPct * 1000 + (focusAvgSum / focusAvgCount) * 10 + maxTier) or -1
    local overallScore = maxPct * 1000 + avgPct * 10 + maxTier
    local displayName = tostring(runeDef.Name or runeObj.Id) .. " (T" .. tostring(maxTier) .. ")"

    return {
        rune = runeObj,
        def = runeDef,
        id = runeObj.Id,
        guid = runeObj.GUID,
        name = displayName,
        tier = maxTier,
        maxPct = math.round(maxPct),
        avgPct = math.round(avgPct),
        focusMaxPct = (focusMaxPct >= 0) and math.round(focusMaxPct) or -1,
        hasFocus = focusAvgCount > 0,
        score = (traitFocus and traitFocus ~= "All (Highest % Overall)") and focusScore or overallScore,
        overallScore = overallScore,
        statsText = table.concat(statSummaries, ", "),
        label = string.format("%s [%d%% Max | %d%% Avg] (%s)", displayName, math.round(maxPct), math.round(avgPct), tostring(runeObj.GUID):sub(1, 6)),
    }
end

function RuneEngine.GetInventoryEquipments()
    local pd = State.PlayerData or ResolvePlayerData()
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
    local list = {}
    local equippedKeys = {}
    if pd and pd.Data and pd.Data.Equipped then
        for _, eqItem in pairs(pd.Data.Equipped) do
            if type(eqItem) == "table" then
                local k = eqItem.GUID or eqItem.Name
                if k then equippedKeys[k] = true end
            end
        end
    end

    table.clear(RuneEngine.EquipLabelToObj)
    table.clear(RuneEngine.EquipLabels)

    local filterMode = Config.RuneEquipFilter or "Pickaxes & Weapons"
    for _, eq in ipairs((pd and pd.Data and pd.Data.Inventory and pd.Data.Inventory.Equipments) or {}) do
        if type(eq) == "table" then
            local itemName = tostring(eq.Type or eq.Name or "")
            local itemKey = eq.GUID or eq.Name
            if itemName ~= "" and itemKey then
                local itemCat = (okEQ and Equipments and Equipments.GetItemType) and Equipments:GetItemType(itemName) or "Unknown"
                local includeItem = (filterMode == "Pickaxes Only" and itemCat == "Pickaxe")
                    or (filterMode == "Weapons Only" and itemCat == "Weapon")
                    or (filterMode == "Armor Only" and itemCat == "Armor")
                    or (filterMode == "Pickaxes & Weapons" and (itemCat == "Pickaxe" or itemCat == "Weapon"))
                if includeItem then
                    local totalSlots = (okEQ and Equipments and Equipments.GetRuneSlots) and Equipments:GetRuneSlots(eq) or 0
                    local emptySlots = (okEQ and Equipments and Equipments.GetEmptyRuneSlots) and Equipments:GetEmptyRuneSlots(eq) or 0
                    local eqPrefix = equippedKeys[itemKey] and "[Eq] " or ""
                    local catTag = (itemCat == "Pickaxe") and "[Pickaxe] " or ((itemCat == "Armor") and "[Armor] " or "")
                    local upgTag = (tonumber(eq.Upgrade) or 0) > 0 and (" +" .. tostring(eq.Upgrade)) or ""
                    local idTag = eq.GUID and (" #" .. tostring(eq.GUID):sub(1, 4)) or ""
                    local lbl = string.format("%s%s%s%s (%d/%d)%s", eqPrefix, catTag, itemName, upgTag, emptySlots, totalSlots, idTag)
                    RuneEngine.EquipLabelToObj[lbl] = eq
                    RuneEngine.EquipLabelToObj[itemKey] = eq
                    table.insert(list, { label = lbl, eq = eq, isEquipped = equippedKeys[itemKey] == true, emptySlots = emptySlots, upgrade = tonumber(eq.Upgrade) or 0 })
                end
            end
        end
    end

    table.sort(list, function(a, b)
        if a.isEquipped ~= b.isEquipped then return a.isEquipped end
        if (a.emptySlots > 0) ~= (b.emptySlots > 0) then return a.emptySlots > 0 end
        if a.upgrade ~= b.upgrade then return a.upgrade > b.upgrade end
        return a.label < b.label
    end)

    for _, item in ipairs(list) do
        table.insert(RuneEngine.EquipLabels, item.label)
    end
    if #RuneEngine.EquipLabels == 0 then
        table.insert(RuneEngine.EquipLabels, "None Found")
    end
    return RuneEngine.EquipLabels
end

function RuneEngine.GetTargetEquipment()
    if #RuneEngine.EquipLabels == 0 then
        RuneEngine.GetInventoryEquipments()
    end
    local sel = Config.RuneTargetEquipment
    if type(sel) == "string" and RuneEngine.EquipLabelToObj[sel] then
        return RuneEngine.EquipLabelToObj[sel], sel
    end
    local firstLabel = RuneEngine.EquipLabels[1]
    if firstLabel and RuneEngine.EquipLabelToObj[firstLabel] then
        Config.RuneTargetEquipment = firstLabel
        return RuneEngine.EquipLabelToObj[firstLabel], firstLabel
    end
    return nil, "None"
end

function RuneEngine.GetRankedRunesForCategory(targetCat, traitFocus)
    local pd = State.PlayerData or ResolvePlayerData()
    local analyzed = {}

    for _, m in ipairs((pd and pd.Data and pd.Data.Inventory and pd.Data.Inventory.Misc) or {}) do
        if type(m) == "table" and m.Id and m.GUID and m.Traits then
            local a = RuneEngine.AnalyzeRune(m, traitFocus)
            if a then
                local compatible = true
                if targetCat and type(a.def.WhitelistedTypes) == "table" then
                    compatible = table.find(a.def.WhitelistedTypes, targetCat) ~= nil
                end
                if compatible then
                    table.insert(analyzed, a)
                end
            end
        end
    end

    table.sort(analyzed, function(a, b)
        if a.score ~= b.score then return a.score > b.score end
        if a.overallScore ~= b.overallScore then return a.overallScore > b.overallScore end
        return tostring(a.guid) < tostring(b.guid)
    end)

    return analyzed
end

function RuneEngine.GetRankedRunes(targetEq, traitFocus)
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
    local targetCat = (targetEq and okEQ and Equipments and Equipments.GetItemType) and Equipments:GetItemType(targetEq.Type or targetEq.Name) or nil
    return RuneEngine.GetRankedRunesForCategory(targetCat, traitFocus)
end

function RuneEngine.RefreshRunesAndInfo()
    local eq = select(1, RuneEngine.GetTargetEquipment())
    local ranked = RuneEngine.GetRankedRunes(eq, Config.RuneTraitFocus)

    table.clear(RuneEngine.RuneLabelToObj)
    table.clear(RuneEngine.RuneLabels)
    table.insert(RuneEngine.RuneLabels, "Auto: Best For Focus")
    for _, a in ipairs(ranked) do
        RuneEngine.RuneLabelToObj[a.label] = a
        table.insert(RuneEngine.RuneLabels, a.label)
    end

    if RuneEngine.RuneDropdown and type(RuneEngine.RuneDropdown.Refresh) == "function" then
        local curManual = (type(Config.RuneSelectedManual) == "string" and RuneEngine.RuneLabelToObj[Config.RuneSelectedManual])
            and Config.RuneSelectedManual
            or "Auto: Best For Focus"
        pcall(function() RuneEngine.RuneDropdown:Refresh(RuneEngine.RuneLabels, curManual) end)
    end
    RuneEngine.UpdateInfoWindow()
end

function RuneEngine.RefreshDropdownLists()
    local eqLabels = RuneEngine.GetInventoryEquipments()
    local _, selectedEqLabel = RuneEngine.GetTargetEquipment()

    if RuneEngine.EquipDropdown and type(RuneEngine.EquipDropdown.Refresh) == "function" then
        local validDefault = (type(selectedEqLabel) == "string" and RuneEngine.EquipLabelToObj[selectedEqLabel])
            and selectedEqLabel
            or eqLabels[1]
        pcall(function() RuneEngine.EquipDropdown:Refresh(eqLabels, validDefault) end)
    end
    RuneEngine.RefreshRunesAndInfo()
end

function RuneEngine.GetSelectedRuneAnalysis(ranked)
    if not ranked or #ranked == 0 then return nil end
    local manual = Config.RuneSelectedManual
    if manual and manual ~= "Auto: Best For Focus" and RuneEngine.RuneLabelToObj[manual] then
        return RuneEngine.RuneLabelToObj[manual]
    end
    return ranked[1]
end

function RuneEngine.UpdateInfoWindow()
    local eq = select(1, RuneEngine.GetTargetEquipment())
    local rankedAll = RuneEngine.GetRankedRunes(eq, "All (Highest % Overall)")
    local rankedFocus = RuneEngine.GetRankedRunes(eq, Config.RuneTraitFocus)
    local bestOverall = rankedAll[1]
    local chosen = RuneEngine.GetSelectedRuneAnalysis(rankedFocus)

    if RuneEngine.BestRuneLabel then
        if not chosen then
            pcall(function()
                RuneEngine.BestRuneLabel:SetTitle("BEST Rune: None Compatible in Bag")
                RuneEngine.BestRuneLabel:SetDesc("Farm enemies or mine drops to collect runes compatible with this weapon")
            end)
        else
            local topLines = {}
            for i = 1, math.min(3, #rankedFocus) do
                local r = rankedFocus[i]
                table.insert(topLines, string.format("#%d %s (%d%% Max / %d%% Avg)", i, r.name, r.maxPct, r.avgPct))
            end
            local focusHeader = (Config.RuneTraitFocus ~= "All (Highest % Overall)" and chosen.hasFocus)
                and string.format("Best [%s]: %s (%d%% Focus | %d%% Max)", Config.RuneTraitFocus, chosen.name, chosen.focusMaxPct, chosen.maxPct)
                or string.format("BEST Rune: %s (%d%% Max Roll | %d%% Avg)", chosen.name, chosen.maxPct, chosen.avgPct)
            local overallLine = bestOverall and string.format("Overall #1: %s (%d%%) | Top: %s", bestOverall.name, bestOverall.maxPct, table.concat(topLines, " • ")) or ""
            pcall(function()
                RuneEngine.BestRuneLabel:SetTitle(focusHeader)
                RuneEngine.BestRuneLabel:SetDesc(chosen.statsText .. "\n" .. overallLine)
            end)
        end
    end

    if RuneEngine.BestPickaxeRuneLabel then
        local rankedPick = RuneEngine.GetRankedRunesForCategory("Pickaxe", "All (Highest % Overall)")
        local bestPick = rankedPick[1]
        if not bestPick then
            pcall(function()
                RuneEngine.BestPickaxeRuneLabel:SetTitle("BEST Pickaxe Rune: None in Bag")
                RuneEngine.BestPickaxeRuneLabel:SetDesc("Mine rocks or defeat mobs to collect Miner Shard runes for Pickaxes")
            end)
        else
            local topPick = {}
            for i = 1, math.min(3, #rankedPick) do
                local r = rankedPick[i]
                table.insert(topPick, string.format("#%d %s (%d%%)", i, r.name, r.maxPct))
            end
            pcall(function()
                RuneEngine.BestPickaxeRuneLabel:SetTitle(string.format("BEST Pickaxe Rune: %s (%d%% Max | %d%% Avg)", bestPick.name, bestPick.maxPct, bestPick.avgPct))
                RuneEngine.BestPickaxeRuneLabel:SetDesc(bestPick.statsText .. "\nPickaxe Top: " .. table.concat(topPick, " • "))
            end)
        end
    end

    if RuneEngine.CostInfoLabel then
        if not eq then
            pcall(function()
                RuneEngine.CostInfoLabel:SetTitle("Target Equipment: None Found")
                RuneEngine.CostInfoLabel:SetDesc("Craft or equip a Weapon or Pickaxe in inventory first")
            end)
            return
        end
        local eqDisplayName = tostring(eq.Type or eq.Name or "Item")
        local eqKey = tostring(eq.GUID or eq.Name or "")
        local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
        local totalSlots = (okEQ and Equipments and Equipments.GetRuneSlots) and Equipments:GetRuneSlots(eq) or 0
        local emptySlots = (okEQ and Equipments and Equipments.GetEmptyRuneSlots) and Equipments:GetEmptyRuneSlots(eq) or 0
        local pd = State.PlayerData or ResolvePlayerData()
        local gold = pd and pd.Data and tonumber(pd.Data.Gold) or 0

        if emptySlots <= 0 then
            pcall(function()
                RuneEngine.CostInfoLabel:SetTitle(string.format("%s (+%d): 0/%d Open Rune Slots", eqDisplayName, tonumber(eq.Upgrade) or 0, totalSlots))
                RuneEngine.CostInfoLabel:SetDesc("Upgrade equipment at Enhancer (or detach an existing rune) to unlock an open slot")
            end)
            return
        end

        if not chosen then
            pcall(function()
                RuneEngine.CostInfoLabel:SetTitle(string.format("%s (+%d): %d/%d Open Slots", eqDisplayName, tonumber(eq.Upgrade) or 0, emptySlots, totalSlots))
                RuneEngine.CostInfoLabel:SetDesc("No compatible rune available to quote attach cost")
            end)
            return
        end

        local cacheKey = eqKey .. "::" .. tostring(chosen.guid)
        if RuneEngine.PriceCache[cacheKey] then
            local p = RuneEngine.PriceCache[cacheKey]
            local need = math.max(0, p - gold)
            local affordStr = (need == 0) and "Affordable Now!" or ("Need $" .. WebhookEngine.FormatNumber(need) .. " more")
            pcall(function()
                RuneEngine.CostInfoLabel:SetTitle(string.format("Attach Cost: $%s Gold (%s)", WebhookEngine.FormatNumber(p), affordStr))
                RuneEngine.CostInfoLabel:SetDesc(string.format("%s (+%d) [%d/%d Open] + %s | Your Gold: $%s", eqDisplayName, tonumber(eq.Upgrade) or 0, emptySlots, totalSlots, chosen.name, WebhookEngine.FormatNumber(gold)))
            end)
        end

        if Remotes.RuneGetPriceInfo then
            task.spawn(function()
                local okP, priceVal = pcall(function()
                    return Remotes.RuneGetPriceInfo:InvokeServer(eq.GUID or eq.Name, chosen.guid, "Attach")
                end)
                if okP and type(priceVal) == "number" then
                    RuneEngine.PriceCache[cacheKey] = priceVal
                    local pdNow = State.PlayerData or ResolvePlayerData()
                    local gNow = pdNow and pdNow.Data and tonumber(pdNow.Data.Gold) or 0
                    local need = math.max(0, priceVal - gNow)
                    local affordStr = (need == 0) and "Affordable Now!" or ("Need $" .. WebhookEngine.FormatNumber(need) .. " more")
                    pcall(function()
                        if RuneEngine.CostInfoLabel then
                            RuneEngine.CostInfoLabel:SetTitle(string.format("Attach Cost: $%s Gold (%s)", WebhookEngine.FormatNumber(priceVal), affordStr))
                            RuneEngine.CostInfoLabel:SetDesc(string.format("%s (+%d) [%d/%d Open] + %s | Your Gold: $%s", eqDisplayName, tonumber(eq.Upgrade) or 0, emptySlots, totalSlots, chosen.name, WebhookEngine.FormatNumber(gNow)))
                        end
                    end)
                end
            end)
        end
    end
end

function RuneEngine.AttachSelectedRune()
    if RuneEngine.IsAttaching or State.IsAutoForging or State.IsBuyingPotions then return false end
    if not Remotes.RunePurchaseAttach then return false end
    local eq = select(1, RuneEngine.GetTargetEquipment())
    local ranked = RuneEngine.GetRankedRunes(eq, Config.RuneTraitFocus)
    local chosen = RuneEngine.GetSelectedRuneAnalysis(ranked)
    if not eq or not chosen then
        if RuneEngine.ActionStatusLabel then
            pcall(function() RuneEngine.ActionStatusLabel:SetTitle("Runes: No Compatible Pair") end)
        end
        return false
    end

    local eqDisplayName = tostring(eq.Type or eq.Name or "Item")
    local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
    local emptySlots = (okEQ and Equipments and Equipments.GetEmptyRuneSlots) and Equipments:GetEmptyRuneSlots(eq) or 0
    if emptySlots <= 0 then
        if RuneEngine.ActionStatusLabel then
            pcall(function() RuneEngine.ActionStatusLabel:SetTitle("Runes: No Empty Slot on " .. eqDisplayName) end)
        end
        return false
    end

    RuneEngine.IsAttaching = true
    local attachedOk = false
    pcall(function()
        local cost = nil
        if Remotes.RuneGetPriceInfo then
            local okC, resC = pcall(function()
                return Remotes.RuneGetPriceInfo:InvokeServer(eq.GUID or eq.Name, chosen.guid, "Attach")
            end)
            if okC and type(resC) == "number" then cost = resC end
        end
        local pd = State.PlayerData or ResolvePlayerData()
        local gold = pd and pd.Data and tonumber(pd.Data.Gold) or 0
        if cost and gold < cost then
            if RuneEngine.ActionStatusLabel then
                pcall(function()
                    RuneEngine.ActionStatusLabel:SetTitle(string.format("Need $%s Gold (Have $%s)", WebhookEngine.FormatNumber(cost), WebhookEngine.FormatNumber(gold)))
                end)
            end
            return
        end

        local okAtt, resAtt = pcall(function()
            return Remotes.RunePurchaseAttach:InvokeServer(eq.GUID or eq.Name, chosen.guid)
        end)
        if okAtt and resAtt then
            attachedOk = true
            if RuneEngine.ActionStatusLabel then
                pcall(function()
                    RuneEngine.ActionStatusLabel:SetTitle("Attached: " .. chosen.name)
                    RuneEngine.ActionStatusLabel:SetDesc("Added remotely to " .. eqDisplayName .. (cost and (" ($" .. WebhookEngine.FormatNumber(cost) .. ")") or ""))
                end)
            end
        else
            if RuneEngine.ActionStatusLabel then
                pcall(function()
                    RuneEngine.ActionStatusLabel:SetTitle("Attach Failed / Declined")
                    RuneEngine.ActionStatusLabel:SetDesc("Check empty slots or Gold")
                end)
            end
        end
    end)
    RuneEngine.IsAttaching = false
    task.delay(0.3, RuneEngine.RefreshDropdownLists)
    return attachedOk
end

function RuneEngine.BuildRuneSellCatalog()
    table.clear(RuneEngine.RuneSellTypes)
    table.clear(RuneEngine.SellDisplayToIds)
    table.clear(RuneEngine.IdToDisplayName)

    local seenDisplay = {}
    local function registerRuneDef(id, def)
        if type(id) ~= "string" or id == "DevRune1" or id == "TestPickaxe_T1" then return end
        local disp = (type(def) == "table" and tostring(def.Name or id)) or tostring(id)
        RuneEngine.IdToDisplayName[id] = disp
        if not RuneEngine.SellDisplayToIds[disp] then
            RuneEngine.SellDisplayToIds[disp] = {}
        end
        RuneEngine.SellDisplayToIds[disp][id] = true
        if not seenDisplay[disp] then
            seenDisplay[disp] = true
            table.insert(RuneEngine.RuneSellTypes, disp)
        end
    end

    local runesMod = ReplicatedStorage:FindFirstChild("Shared")
        and ReplicatedStorage.Shared:FindFirstChild("Data")
        and ReplicatedStorage.Shared.Data:FindFirstChild("Runes")
    local okR, Runes = pcall(function() return runesMod and safeRequire(runesMod) end)
    if okR and Runes and type(Runes.Runes) == "table" then
        for id, def in pairs(Runes.Runes) do
            if type(def) == "table" then
                registerRuneDef(id, def)
            end
        end
    end
    -- Also scan child ModuleScripts under Shared.Data.Runes.Runes directly in case Runes was required before all 37 children replicated
    local subRunesFolder = runesMod and runesMod:FindFirstChild("Runes")
    if subRunesFolder then
        for _, desc in ipairs(subRunesFolder:GetDescendants()) do
            if desc:IsA("ModuleScript") and not RuneEngine.IdToDisplayName[desc.Name] then
                local def = safeRequire(desc)
                if type(def) == "table" then
                    if okR and Runes and type(Runes.Runes) == "table" then
                        Runes.Runes[desc.Name] = def
                    end
                    registerRuneDef(desc.Name, def)
                end
            end
        end
    end
    local catalogCount = #RuneEngine.RuneSellTypes
    RuneEngine.RunesModuleLoaded = (catalogCount >= 30)

    ResolvePlayerData()
    local misc = State.PlayerData and State.PlayerData.Data and State.PlayerData.Data.Inventory and State.PlayerData.Data.Inventory.Misc
    if type(misc) == "table" then
        local itemTypes = (typeof(getrenv) == "function" and getrenv()._G and getrenv()._G.ItemTypes) or _G.ItemTypes or {}
        for _, entry in pairs(misc) do
            if type(entry) == "table" and entry.Id and entry.GUID then
                local isRune = (itemTypes[entry.Id] and itemTypes[entry.Id][1] == "Rune")
                    or (okR and Runes and Runes.Runes and Runes.Runes[entry.Id] ~= nil)
                if isRune and not RuneEngine.IdToDisplayName[entry.Id] then
                    registerRuneDef(tostring(entry.Id), nil)
                end
            end
        end
    end

    table.sort(RuneEngine.RuneSellTypes)
    -- Ensure any saved RuneSellKeepList entries remain in RuneSellTypes so saved selections never vanish during early streaming
    if type(Config.RuneSellKeepList) == "table" then
        for k, v in pairs(Config.RuneSellKeepList) do
            local savedName = (type(k) == "number" and type(v) == "string" and v) or (type(k) == "string" and v == true and k) or nil
            if savedName and savedName ~= "" and not seenDisplay[savedName] then
                seenDisplay[savedName] = true
                table.insert(RuneEngine.RuneSellTypes, savedName)
            end
        end
    end
end
RuneEngine.BuildRuneSellCatalog()

function RuneEngine.IsRuneKept(runeId, guid)
    if not runeId then return true end
    local keep = Config.RuneSellKeepList
    local disp = RuneEngine.IdToDisplayName[runeId] or runeId
    if type(keep) == "table" then
        if keep[disp] == true or keep[runeId] == true then return true end
        for k, v in pairs(keep) do
            if type(k) == "number" and (v == disp or v == runeId) then
                return true
            elseif type(k) == "string" and (k == disp or k == runeId) and v == true then
                return true
            end
        end
    elseif type(keep) == "string" and keep ~= "" then
        return keep == disp or keep == runeId
    end
    return false
end

function RuneEngine.BuildSellRuneBasket()
    if #RuneEngine.RuneSellTypes == 0 or next(RuneEngine.IdToDisplayName) == nil then
        pcall(RuneEngine.BuildRuneSellCatalog)
    end
    ResolvePlayerData()
    local data = State.PlayerData and State.PlayerData.Data
    local inv = data and data.Inventory
    local misc = inv and inv.Misc
    if type(misc) ~= "table" then return {}, 0, 0 end

    local favs = data.Favorites or {}
    local itemTypes = (typeof(getrenv) == "function" and getrenv()._G and getrenv()._G.ItemTypes) or _G.ItemTypes or {}
    local okR, Runes = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Runes) end)

    local basket = {}
    local totalRunes = 0
    local sellableCount = 0

    for _, entry in pairs(misc) do
        if type(entry) == "table" and entry.Id and entry.GUID then
            local isRune = (itemTypes[entry.Id] and itemTypes[entry.Id][1] == "Rune")
                or (okR and Runes and Runes.Runes and Runes.Runes[entry.Id] ~= nil)
            if isRune then
                local qty = math.max(1, math.floor(entry.Quantity or 1))
                totalRunes = totalRunes + qty
                local isFav = favs[entry.GUID] or favs[entry.Id]
                if not isFav and not RuneEngine.IsRuneKept(entry.Id, entry.GUID) then
                    basket[entry.GUID] = qty
                    sellableCount = sellableCount + qty
                end
            end
        end
    end

    return basket, sellableCount, totalRunes
end

function RuneEngine.UpdateSellStatusLabel()
    if not RuneEngine.SellStatusLabel then return end
    pcall(function()
        local _, sellableCount, totalRunes = RuneEngine.BuildSellRuneBasket()
        local used, cap, isFull = ForgeEngine.GetStashCapacityStatus()
        local stateStr = State.IsSellingRunes and "Selling..."
            or (Config.AutoSellRunes and "Active (Instant Remote Sell)" or "Off")
        RuneEngine.SellStatusLabel:SetTitle(string.format("Auto Sell Runes: %s", stateStr))
        RuneEngine.SellStatusLabel:SetDesc(string.format("Runes: %d | Unchecked (Will Sell): %d | Bag: %d/%d%s",
            totalRunes, sellableCount, used, cap, isFull and " [FULL]" or ""))
    end)
end

function RuneEngine.SellUnkeptRunes(fromMobChain)
    if State.IsSellingRunes then
        if (os.clock() - (State.LastRuneSellAttempt or 0)) < 15.0 then
            return 0, false
        end
        State.IsSellingRunes = false
    end
    local basket, sellableCount, totalRunes = RuneEngine.BuildSellRuneBasket()
    if sellableCount <= 0 or not next(basket) then
        State.PendingRuneSell = false
        RuneEngine.UpdateSellStatusLabel()
        return 0, false
    end

    local hasPass = ForgeEngine.HasSellAnywherePass()
    local prox = workspace:FindFirstChild("Proximity")
    local hasCey = (prox and prox:FindFirstChild("Greedy Cey") ~= nil)
    if not hasPass and not hasCey then
        RuneEngine.UpdateSellStatusLabel()
        return 0, false
    end

    State.IsSellingRunes = true
    State.LastRuneSellAttempt = os.clock()
    RuneEngine.UpdateSellStatusLabel()

    -- Sells remotely in-place (or glides to Greedy Cey if needed and verifies removal via Replica)
    local soldOk = false
    local okExec, errExec = pcall(function()
        soldOk = ForgeEngine.SellMiscBasket(basket)
    end)
    if not okExec then
        warn("[The Forge Auto-Sell Runes]:", errExec)
    end

    State.StashFullSignalUntil = 0
    State.PendingRuneSell = false
    State.IsSellingRunes = false
    RuneEngine.UpdateSellStatusLabel()
    task.delay(0.35, RuneEngine.RefreshDropdownLists)

    if soldOk then
        Notify("Auto Sell Runes", string.format("Instantly sold %d unchecked rune(s) (%d total before sell)", sellableCount, totalRunes), 3.0)
    elseif not fromMobChain then
        Notify("Auto Sell Runes", "Failed to sell runes (check Greedy Cey availability)", 3.0)
    end
    return sellableCount, soldOk
end
State.SellUnkeptRunes = RuneEngine.SellUnkeptRunes

task.spawn(function()
    local statusCounter = 0
    while ScriptRunning do
        if Config.AutoPutRunes and (os.clock() - RuneEngine.LastAutoAttachAt) >= 4.0 then
            RuneEngine.LastAutoAttachAt = os.clock()
            local eq = select(1, RuneEngine.GetTargetEquipment())
            if eq then
                local okEQ, Equipments = pcall(function() return safeRequire(ReplicatedStorage.Shared.Data.Equipments) end)
                local emptySlots = (okEQ and Equipments and Equipments.GetEmptyRuneSlots) and Equipments:GetEmptyRuneSlots(eq) or 0
                if emptySlots > 0 then
                    RuneEngine.AttachSelectedRune()
                end
            end
        end

        statusCounter = statusCounter + 1
        if (statusCounter % 3) == 0 then
            RuneEngine.UpdateSellStatusLabel()
        end

        if Config.AutoSellRunes
            and not State.IsSellingRunes
            and not State.SellingMobLoot
            and not State.IsAutoForging
            and not State.DrainingFullBag
            and not State.IsCrafting
            and not State.IsBuyingPotions
            and State.HRP and State.Humanoid and State.Humanoid.Health > 0
            and (os.clock() - (State.LastRuneSellAttempt or 0)) >= 1.5 then
            local _, sellableCount = RuneEngine.BuildSellRuneBasket()
            if sellableCount > 0 then
                RuneEngine.SellUnkeptRunes(false)
            end
        end

        task.wait(0.35)
    end
end)

-- ===================================================================
-- 13. AETHER UI INITIALIZATION
-- ===================================================================
local Globals = (typeof(getgenv) == "function" and getgenv()) or (typeof(getgenv) == "table" and getgenv) or _G or {}

local function ResolveLibrary(timeout)
    timeout = timeout or 3.5
    local start = os.clock()
    while os.clock() - start < timeout do
        local lib = (Globals and (Globals.Library or Globals.Aether or Globals.AetherUI or Globals.AetherLibrary))
            or (typeof(getgenv) == "function" and (getgenv().Library or getgenv().Aether or getgenv().AetherUI or getgenv().AetherLibrary))
            or (_G and (_G.Library or _G.Aether or _G.AetherUI or _G.AetherLibrary))
            or (shared and (shared.Library or shared.Aether or shared.AetherUI or shared.AetherLibrary))

        if lib and type(lib) == "table" and (type(lib.Window) == "function" or type(lib.CreateWindow) == "function") then
            return lib
        end

        if typeof(readfile) == "function" then
            local hasFile = false
            pcall(function()
                hasFile = (typeof(isfile) == "function" and isfile("UI.lua")) or (typeof(isfile) ~= "function")
            end)
            if hasFile then
                local readOk, code = pcall(readfile, "UI.lua")
                if readOk and code and #code > 0 then
                    local loadOk, fn = pcall(loadstring, code)
                    if loadOk and type(fn) == "function" then
                        local runOk, res = pcall(fn)
                        if runOk and res then return res end
                    end
                end
            end
        end
        task.wait(0.1)
    end

    return (Globals and (Globals.Library or Globals.Aether or Globals.AetherUI or Globals.AetherLibrary))
        or (typeof(getgenv) == "function" and (getgenv().Library or getgenv().Aether or getgenv().AetherUI or getgenv().AetherLibrary))
        or (_G and (_G.Library or _G.Aether or _G.AetherUI or _G.AetherLibrary))
        or (shared and (shared.Library or shared.Aether or shared.AetherUI or shared.AetherLibrary))
end

local Library = ResolveLibrary(3.5)
if not Library then return end

local Window = nil
local function CreateWindow()
    local function wrapNoUiConfig(obj)
        if type(obj) ~= "table" then return obj end
        local function wrapCallbackForAutoSave(opts)
            if type(opts) ~= "table" then return end
            -- Allow UI.lua Settings -> Configuration Manager (Save/Load/Reset Config) to persist all controls alongside ConfigManager
            if opts.Flag == nil and type(opts.Title) == "string" and opts.Title ~= "" then
                opts.Flag = opts.Title
            end
            if type(opts.Callback) == "function" and not opts._aetherSaveWrapped then
                local origCb = opts.Callback
                opts._aetherSaveWrapped = true
                opts.Callback = function(...)
                    origCb(...)
                    if Config.AutoSaveConfig ~= false and ConfigManager.LoadedOk ~= false then
                        ConfigManager.ScheduleSave(0.08)
                    end
                end
            end
        end
        local function wrapSection(sec)
            if type(sec) ~= "table" then return sec end
            local proxy = {}
            for _, m in ipairs({ "Toggle", "Slider", "Dropdown", "Textbox", "Input", "Keybind", "Colorpicker" }) do
                local origFn = sec[m]
                if type(origFn) == "function" then
                    proxy[m] = function(_, opts, ...)
                        wrapCallbackForAutoSave(opts)
                        return origFn(sec, opts, ...)
                    end
                end
            end
            return setmetatable(proxy, {
                __index = function(_, k)
                    local v = sec[k]
                    if type(v) == "function" then
                        return function(_, ...) return v(sec, ...) end
                    end
                    return v
                end
            })
        end
        local function wrapContainer(cont)
            if type(cont) ~= "table" then return cont end
            local proxy = {}
            if type(cont.Section) == "function" then
                proxy.Section = function(_, ...) return wrapSection(cont:Section(...)) end
            end
            if type(cont.SubTab) == "function" then
                proxy.SubTab = function(_, ...) return wrapContainer(cont:SubTab(...)) end
            end
            for _, m in ipairs({ "Toggle", "Slider", "Dropdown", "Textbox", "Input", "Keybind", "Colorpicker" }) do
                local origFn = cont[m]
                if type(origFn) == "function" then
                    proxy[m] = function(_, opts, ...)
                        wrapCallbackForAutoSave(opts)
                        return origFn(cont, opts, ...)
                    end
                end
            end
            return setmetatable(proxy, {
                __index = function(_, k)
                    local v = cont[k]
                    if type(v) == "function" then
                        return function(_, ...) return v(cont, ...) end
                    end
                    return v
                end
            })
        end
        local winProxy = {}
        if type(obj.Tab) == "function" then
            winProxy.Tab = function(_, ...) return wrapContainer(obj:Tab(...)) end
        end
        return setmetatable(winProxy, {
            __index = function(_, k)
                local v = obj[k]
                if type(v) == "function" then
                    return function(_, ...) return v(obj, ...) end
                end
                return v
            end
        })
    end

    local ok, created = pcall(Library.Window, Library, {
        Title = "Aether",
        Desc = "Studios",
        Theme = "Default",
        Config = {
            Keybind = Enum.KeyCode.LeftControl,
            Size = UDim2.new(0, 840, 0, 480)
        }
    })
    if ok and created then return wrapNoUiConfig(created) end

    local ok2, fallback = pcall(Library.Window, Library, { Theme = "Default" })
    if ok2 and fallback then return wrapNoUiConfig(fallback) end
    return nil
end


Window = CreateWindow()
if not Window then return end

-- ===================================================================
-- 14. UI TABS & CONTROLS
-- ===================================================================

-- TAB 1: MINING (Sub-sections: Mining, Craft, Selling, Forge, Shop)
do
    local MiningTab = Window:Tab({ Title = "Mining", Icon = "zap" })
    local SubMining = (typeof(MiningTab.SubTab) == "function") and MiningTab:SubTab("Mining") or MiningTab
    local SubCraft = (typeof(MiningTab.SubTab) == "function") and MiningTab:SubTab("Craft") or MiningTab
    local SubSelling = (typeof(MiningTab.SubTab) == "function") and MiningTab:SubTab("Selling") or MiningTab
    local SubForge = (typeof(MiningTab.SubTab) == "function") and MiningTab:SubTab("Forge") or MiningTab
    local SubShop = (typeof(MiningTab.SubTab) == "function") and MiningTab:SubTab("Shop") or MiningTab

    -- ===================================================================
    -- SUB-SECTION 1B: CRAFT (AUTO ORE CRAFTING & MEDALLIONS)
    -- ===================================================================
    do
        CraftEngine.LoadCatalog(false)
        local CraftMainSection = SubCraft:Section({ Title = "Auto Craft (Ore Craft Stations)", Side = "Left" })
        local CraftSlotSection = SubCraft:Section({ Title = "Craft Slots & Settings", Side = "Right" })

        CraftEngine.StatusLabel = CraftMainSection:Label({
            Title = "Auto-Craft: Idle",
            Desc = "Priority: Auto Mine / Combat > Auto Craft > Auto Forge"
        })

        CraftMainSection:Toggle({
            Title = "Enable Auto Craft",
            Desc = "Queues selected recipes into open Craft slots (runs before Auto Forge on full bag, or when idle)",
            Value = Config.AutoCraft,
            Callback = function(val)
                Config.AutoCraft = val
                if not val then
                    State.IsCrafting = false
                    NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
                end
                CraftEngine.UpdateStatus()
            end
        })

        CraftEngine.RecipesDropdown = CraftMainSection:Dropdown({
            Title = "Recipes To Craft (Priority Order)",
            Desc = "Select Ores or Abyss Medallions to auto-craft using open Craft slots (Multi-Select)",
            List = (#CraftEngine.RecipeNames > 0) and CraftEngine.RecipeNames or { "Earthite" },
            Value = Config.CraftTargets,
            Multi = true,
            Callback = function(val)
                if type(val) == "table" then
                    Config.CraftTargets = val
                elseif type(val) == "string" then
                    Config.CraftTargets = { val }
                else
                    Config.CraftTargets = {}
                end
                CraftEngine.UpdateStatus()
            end
        })
        if #CraftEngine.RecipeNames == 0 then
            task.spawn(function()
                for _ = 1, 60 do
                    task.wait(0.5)
                    if not ScriptRunning then return end
                    if CraftEngine.LoadCatalog(true) and #CraftEngine.RecipeNames > 0 then
                        if CraftEngine.RecipesDropdown and type(CraftEngine.RecipesDropdown.Refresh) == "function" then
                            pcall(function() CraftEngine.RecipesDropdown:Refresh(CraftEngine.RecipeNames, Config.CraftTargets) end)
                        end
                        CraftEngine.UpdateStatus()
                        break
                    end
                end
            end)
        end

        CraftMainSection:Slider({
            Title = "Batch Amount Per Slot",
            Desc = "Quantity to craft per open slot (1 uses minimal materials)",
            Min = 1,
            Max = 25,
            Value = Config.CraftBatchAmount,
            Rounding = 0,
            Callback = function(val) Config.CraftBatchAmount = val end
        })

        CraftMainSection:Toggle({
            Title = "Craft Whenever Slot Opens (Interrupt Farm)",
            Desc = "OFF: Auto Mine/Combat > Auto Craft > Auto Forge (runs before Forge on full bag or when idle). ON: crafts anytime a slot opens",
            Value = Config.AutoCraftInterruptFarm,
            Callback = function(val) Config.AutoCraftInterruptFarm = val end
        })

        CraftMainSection:Button({
            Title = "Craft Open Slots Now",
            Desc = "Claims finished crafts and immediately queues selected recipes into free Craft slots",
            Callback = function()
                task.spawn(function()
                    local done = CraftEngine.RunAutoCraftCycle(true)
                    if done > 0 then
                        Notify("Auto Craft", string.format("Queued %d Craft slot(s)!", done), 3.0)
                    else
                        Notify("Auto Craft", "No open Craft slots or insufficient materials for selected recipes.", 3.0)
                    end
                end)
            end
        })

        CraftEngine.SlotInfoLabel = CraftSlotSection:Label({
            Title = "Craft Slots: Scanning...",
            Desc = "Shows live Craft slots (usually 2 unlocked free slots)"
        })

        CraftSlotSection:Toggle({
            Title = "Auto Claim Completed Crafts",
            Desc = "Automatically claims crafts the exact second their timer finishes (event + timer driven)",
            Value = Config.AutoClaimSmelt,
            Callback = function(val)
                Config.AutoClaimSmelt = val
                if val then
                    pcall(CraftEngine.ScheduleCompletionClaim)
                end
            end
        })

        CraftSlotSection:Button({
            Title = "Check / Claim Crafts Now",
            Desc = "Manually checks & claims any finished Craft slots now",
            Callback = function()
                if Remotes.CheckCraftCompletions then
                    pcall(function() Remotes.CheckCraftCompletions:InvokeServer() end)
                end
                task.delay(0.15, function() pcall(CraftEngine.UpdateStatus) end)
            end
        })

        pcall(CraftEngine.UpdateStatus)
    end

    -- ===================================================================
    -- SUB-SECTION 1: MINING (AUTOMATION & SAFETIES)
    -- ===================================================================
    local MainSection = SubMining:Section({ Title = "Auto Mine Settings", Side = "Left" })

    MainSection:Toggle({
        Title = "Enable Auto Mine",
        Desc = "Automatically mines target rocks safely on the ground",
        Value = Config.AutoMine,
        Callback = function(val)
            Config.AutoMine = val
            if val then
                State.CheckingPotionsOnStart = true
                task.spawn(PotionEngine.CheckOnAutoMineStart)
            else
                State.CheckingPotionsOnStart = false
                State.TargetRock = nil
                State.CurrentRock = nil
                State.LastMinedRock = nil
                State.ResumingAfterDeath = false
                NavEngine.CachedRock = nil
                if NavEngine.IsGliding and not Config.AutoMobFarm and not Config.AutoRaid and not State.IsAutoForging then
                    NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
                    pcall(NavEngine.RestoreCharacterPhysics)
                end
                if State.HRP and State.HRP.Anchored then
                    pcall(function()
                        if NavEngine.IsSunkenMode(Config.MinePositionMode) then
                            State.HRP.CFrame = CFrame.new(State.HRP.Position + Vector3.new(0, 4.0, 0))
                        end
                        State.HRP.Anchored = false
                    end)
                end
                if State.Humanoid then
                    pcall(function() State.Humanoid.AutoRotate = true end)
                end
            end
        end
    })

    MainSection:Toggle({
        Title = "Stay & Mine From Saved Spot",
        Desc = "Anchors you at your saved position & farms in-reach rocks (returns after Auto Forge)",
        Value = Config.StayAtSavedPos,
        Callback = function(val)
            Config.StayAtSavedPos = val
            if not val then
                if not Config.AutoMine or not NavEngine.IsSunkenMode(Config.MinePositionMode) then
                    if State.HRP and State.HRP.Anchored then
                        pcall(function() State.HRP.Anchored = false end)
                    end
                    if State.Humanoid then
                        pcall(function() State.Humanoid.AutoRotate = true end)
                    end
                end
            elseif not MiningEngine.GetSavedSpotCFrame() then
                MiningEngine.SaveCurrentSpot()
            end
        end
    })

    MainSection:Button({
        Title = "Save Current Position",
        Desc = "Saves your exact spot & facing for stationary Auto Mine",
        Callback = function()
            MiningEngine.SaveCurrentSpot()
        end
    })

    MiningEngine.SavedSpotLabel = MainSection:Label({
        Title = "Saved Spot: Not Set",
        Desc = "Stand where you want to mine and press 'Save Current Position'"
    })
    MiningEngine.UpdateSavedSpotLabel()

    MainSection:Dropdown({
        Title = "Target Rock Types",
        Desc = "Select one or multiple rock types from this world (or 'All')",
        List = RockTypes,
        Value = Config.MineTargets,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.MineTargets = val
            elseif type(val) == "string" then
                Config.MineTargets = { val }
            end
            -- Immediately drop current rock if it no longer matches the new selection so we go straight to the closest selected!
            if State.TargetRock and not MiningEngine.MatchesTargetRock(State.TargetRock.Name) then
                State.TargetRock = nil
                State.CurrentRock = nil
                NavEngine.CachedRock = nil
            end
        end
    })

    MainSection:Toggle({
        Title = "Prioritize Strongest Rock",
        Desc = "When multiple rocks are selected, mines strongest to weakest first (e.g. Soul Boulder -> Blossom Boulder -> Bamboo Rock)",
        Value = Config.PrioritizeStrongestRock,
        Callback = function(val)
            Config.PrioritizeStrongestRock = val
            State.TargetRock = nil
            State.CurrentRock = nil
            NavEngine.CachedRock = nil
        end
    })

    MainSection:Dropdown({
        Title = "Target Mining Zones",
        Desc = "Restrict Auto Mine to specific caves/zones (or 'Any' for all zones)",
        List = MineZones,
        Value = Config.MineZones,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.MineZones = val
            elseif type(val) == "string" then
                Config.MineZones = { val }
            end
            if State.TargetRock and not MiningEngine.MatchesTargetZone(State.TargetRock) then
                State.TargetRock = nil
                State.CurrentRock = nil
                NavEngine.CachedRock = nil
            end
        end
    })

    if Config.MinePositionMode == "Sunken Under-Ledge (Horizontal Up)" or Config.MinePositionMode == "Underground (Safe)" then
        Config.MinePositionMode = "Underground"
    elseif Config.MinePositionMode == "Adjacent Ground" then
        Config.MinePositionMode = "Next to Rock"
    elseif Config.MinePositionMode ~= "Underground" and Config.MinePositionMode ~= "Next to Rock" then
        Config.MinePositionMode = "Underground"
    end

    MainSection:Dropdown({
        Title = "Mining Position",
        Desc = "Where you stand while mining",
        List = { "Underground", "Next to Rock" },
        Value = Config.MinePositionMode,
        Callback = function(val) Config.MinePositionMode = val end
    })

    MainSection:Slider({
        Title = "Distance Below Rock",
        Desc = "How far underground you sit (default is best)",
        Min = 1.00,
        Max = 3.00,
        Value = Config.SunkenDepth,
        Rounding = 2,
        Callback = function(val) Config.SunkenDepth = val end
    })

    MainSection:Slider({
        Title = "Angle From Rock",
        Desc = "Body tilt in degrees (90 = flat, default is best)",
        Min = 45,
        Max = 90,
        Value = Config.SunkenPitch,
        Rounding = 0,
        Callback = function(val) Config.SunkenPitch = val end
    })

    MainSection:Slider({
        Title = "Horizontal Distance From Rock",
        Desc = "Sideways distance from the rock (default is best)",
        Min = 0.40,
        Max = 3.00,
        Value = Config.SunkenDistance,
        Rounding = 2,
        Callback = function(val) Config.SunkenDistance = val end
    })

    local SafetiesSection = SubMining:Section({ Title = "Safeties & Gear", Side = "Right" })

    local TargetStatusLabel = SafetiesSection:Label({
        Title = "Status: Idle",
        Desc = "Pickaxe: Stone Pickaxe [Power: 4]"
    })
    MiningEngine.StatusLabel = TargetStatusLabel

    SafetiesSection:Toggle({
        Title = "Auto Equip Pickaxe",
        Desc = "Ensures pickaxe is equipped before swinging",
        Value = Config.AutoEquipPickaxe,
        Callback = function(val) Config.AutoEquipPickaxe = val end
    })

    SafetiesSection:Toggle({
        Title = "Avoid Occupied Rocks (Players)",
        Desc = "Switches to another rock if another player is mining yours",
        Value = Config.AvoidPlayerOccupiedRocks,
        Callback = function(val) Config.AvoidPlayerOccupiedRocks = val end
    })

    SafetiesSection:Toggle({
        Title = "Spam Weapon While Mining",
        Desc = "Fires Weapon alongside Pickaxe to hit nearby mobs while mining",
        Value = Config.AutoDefense,
        Callback = function(val) Config.AutoDefense = val end
    })

    SafetiesSection:Toggle({
        Title = "Prioritize Critical Spots",
        Desc = "Aims for RockCritical indicators when active",
        Value = Config.PrioritizeCritical,
        Callback = function(val) Config.PrioritizeCritical = val end
    })

    SafetiesSection:Slider({
        Title = "Stuck Rock Timeout (s)",
        Desc = "Seconds of no damage before skipping to the next rock",
        Min = 3.0,
        Max = 10.0,
        Value = Config.StuckRockThreshold,
        Rounding = 1,
        Callback = function(val) Config.StuckRockThreshold = val end
    })

    SafetiesSection:Slider({
        Title = "Stubborn Rock Blacklist Time",
        Desc = "Seconds to ignore a stubborn rock before retrying",
        Min = 15,
        Max = 120,
        Value = Config.StuckRockBlacklistTime,
        Rounding = 0,
        Callback = function(val) Config.StuckRockBlacklistTime = val end
    })

    SafetiesSection:Button({
        Title = "Mine Target Once",
        Desc = "Fires a single pickaxe hit",
        Callback = function()
            MiningEngine.ExecuteMineHit()
        end
    })

    local PeekSection = SubMining:Section({ Title = "Ore Peek Filter (Skip Junk Rocks)", Side = "Right" })

    MiningEngine.PeekStatusLabel = PeekSection:Label({
        Title = "Ore Peek: Idle",
        Desc = "Cracks rocks until Ore spawns, skips rocks lacking Wanted Ores so they regenerate"
    })

    PeekSection:Toggle({
        Title = "Only Break Rocks With Wanted Ores",
        Desc = "Cracks rocks until Ore spawns; skips junk rocks until they regenerate to 100% HP",
        Value = Config.FilterRocksByOre,
        Callback = function(val)
            Config.FilterRocksByOre = val
            if not val then
                MiningEngine.ClearSkippedRocks()
                MiningEngine.SetPeekStatus("Ore Peek: Disabled", "Mining all matching target rocks")
            else
                MiningEngine.SetPeekStatus("Ore Peek: Active", "Automatically tracks rock regeneration (no timer needed)")
            end
        end
    })

    PeekSection:Dropdown({
        Title = "Wanted Ores Inside Rock",
        Desc = "Only finish breaking rocks containing at least one of these ores (Multi-Select)",
        List = ForgeEngine.ForgeableOreNames,
        Value = Config.WantedRockOres,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.WantedRockOres = val
            elseif type(val) == "string" then
                Config.WantedRockOres = { val }
            end
            -- Clear skip cache when wanted ore list changes so rocks can be re-evaluated
            MiningEngine.ClearSkippedRocks()
        end
    })

    PeekSection:Toggle({
        Title = "Clear Backup Rocks",
        Desc = "When all primary rocks are broken/skipped, fully break backup rocks to trigger respawns",
        Value = Config.FilterRocksByOreBackupEnabled,
        Callback = function(val)
            Config.FilterRocksByOreBackupEnabled = val
            if not val and MiningEngine.IsBackupTarget then
                MiningEngine.IsBackupTarget = false
                State.TargetRock = nil
                State.CurrentRock = nil
            end
        end
    })

    do
        local backupRockList = { "Auto (Rocks Lacking Wanted Ores)" }
        for _, rName in ipairs(RockTypes) do
            if rName ~= "All" and rName ~= "Lucky Block" then
                table.insert(backupRockList, rName)
            end
        end
        PeekSection:Dropdown({
            Title = "Backup / Filler Rocks (Full Break)",
            Desc = "Rocks to fully break when primary Ore Peek rocks are exhausted (Auto picks rocks that cannot drop Wanted Ores)",
            List = backupRockList,
            Value = Config.BackupMineTargets,
            Multi = true,
            Callback = function(val)
                if type(val) == "table" then
                    Config.BackupMineTargets = val
                elseif type(val) == "string" then
                    Config.BackupMineTargets = { val }
                end
            end
        })
    end

    -- ===================================================================
    -- SUB-SECTION 2: SELLING (ARMOR & SELECTIVE ORE WHITELIST)
    -- ===================================================================
    local ArmorSellSection = SubSelling:Section({ Title = "Armor Selling (Remote Marbles)", Side = "Left" })

    local ArmorSellStatusLabel = ArmorSellSection:Label({
        Title = "Armor Selling: Ready",
        Desc = "Auto-sell runs automatically inside Auto Forge"
    })
    ForgeEngine.ArmorSellStatusLabel = ArmorSellStatusLabel

    local StashSellSection = SubSelling:Section({ Title = "Selective Ore Selling (Protected)", Side = "Right" })

    local StashSellStatusLbl = StashSellSection:Label({
        Title = "Ore Filter: 0 Ores Selected",
        Desc = "ONLY ores picked below will be sold; non-picked ores are protected!"
    })
    ForgeEngine.StashSellStatusLabel = StashSellStatusLbl

    if #ForgeEngine.SellableOreNames == 0 then
        ForgeEngine.EnsureOreCatalog()
    end
    ForgeEngine.SellOresDropdown = StashSellSection:Dropdown({
        Title = "Target Ores to Sell",
        Desc = "Pick which ores you WANT to sell. Non-picked ores are NEVER sold!",
        List = ForgeEngine.SellableOreNames,
        Value = Config.SellOreWhitelist,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.SellOreWhitelist = val
            elseif type(val) == "string" then
                Config.SellOreWhitelist = { val }
            else
                Config.SellOreWhitelist = {}
            end
            local wl = ForgeEngine.GetSelectedSellOresList()
            if StashSellStatusLbl then
                if #wl == 0 then
                    StashSellStatusLbl:SetTitle("Ore Filter: None Selected (All Safe)")
                    StashSellStatusLbl:SetDesc("Pick ores above to enable selling; unselected ores stay safe")
                else
                    StashSellStatusLbl:SetTitle(string.format("Ore Filter: %d Type(s) Selected", #wl))
                    StashSellStatusLbl:SetDesc("Will sell ONLY: " .. table.concat(wl, ", "))
                end
            end
        end
    })
    -- Sync initial label + retry lazy catalog population if Raid/Island data finished loading slightly after UI
    do
        local wlInit = ForgeEngine.GetSelectedSellOresList()
        if #wlInit > 0 and StashSellStatusLbl then
            StashSellStatusLbl:SetTitle(string.format("Ore Filter: %d Type(s) Selected", #wlInit))
            StashSellStatusLbl:SetDesc("Will sell ONLY: " .. table.concat(wlInit, ", "))
        end
        if not ForgeEngine.OreCatalogComplete or #ForgeEngine.SellableOreNames == 0 then
            task.spawn(function()
                for _ = 1, 60 do
                    task.wait(0.5)
                    if not ScriptRunning then return end
                    local hasAny, grew = ForgeEngine.EnsureOreCatalog()
                    if hasAny and (grew or ForgeEngine.OreCatalogComplete) and #ForgeEngine.SellableOreNames > 0 then
                        if ForgeEngine.SellOresDropdown and type(ForgeEngine.SellOresDropdown.Refresh) == "function" then
                            pcall(function() ForgeEngine.SellOresDropdown:Refresh(ForgeEngine.SellableOreNames, Config.SellOreWhitelist) end)
                        end
                        if ForgeEngine.BlacklistOresDropdown and type(ForgeEngine.BlacklistOresDropdown.Refresh) == "function" then
                            pcall(function() ForgeEngine.BlacklistOresDropdown:Refresh(ForgeEngine.ForgeableOreNames, Config.ForgeOreBlacklist) end)
                        end
                        local hList = { "Auto: Best From Inventory" }
                        for _, oName in ipairs(ForgeEngine.AllOreNames or {}) do
                            if oName ~= "Auto: Best From Inventory" then table.insert(hList, oName) end
                        end
                        for slotIdx = 1, 4 do
                            local hd = ForgeEngine.HuntSlotDropdowns[slotIdx]
                            if hd and type(hd.Refresh) == "function" then
                                pcall(function() hd:Refresh(hList, Config["WeaponHuntOre" .. tostring(slotIdx)]) end)
                            end
                            local cd = ForgeEngine.SlotDropdowns[slotIdx]
                            if cd and type(cd.Refresh) == "function" then
                                pcall(function() cd:Refresh(ForgeEngine.AllOreNames, Config["CalcSlot" .. tostring(slotIdx) .. "Ore"]) end)
                            end
                        end
                        if ForgeEngine.OreCatalogComplete then
                            break
                        end
                    end
                end
            end)
        end
    end

    StashSellSection:Toggle({
        Title = "Auto-Sell Selected Ores",
        Desc = "Periodically sells ONLY the ores picked above (protects all other ores)",
        Value = Config.AutoSellStash,
        Callback = function(val) Config.AutoSellStash = val end
    })

    StashSellSection:Slider({
        Title = "Auto-Sell Ore Interval (s)",
        Desc = "Cooldown between whitelisted ore sales in seconds",
        Min = 10,
        Max = 300,
        Value = Config.SellStashInterval,
        Rounding = 0,
        Callback = function(val) Config.SellStashInterval = val end
    })

    StashSellSection:Button({
        Title = "Claim Daily & Mail All",
        Desc = "Claims all pending daily and mailbox rewards",
        Callback = function()
            if Remotes.ClaimDaily then pcall(function() Remotes.ClaimDaily:InvokeServer() end) end
            if Remotes.ClaimMailAll then pcall(function() Remotes.ClaimMailAll:InvokeServer() end) end
        end
    })

    -- ===================================================================
    -- SUB-SECTION 3: FORGE (AUTO-PROFIT & MINIGAMES)
    -- ===================================================================
    local ProfitSection = SubForge:Section({ Title = "Auto-Forge Armor Profit Engine", Side = "Left" })

    local AutoForgeStatusLbl = ProfitSection:Label({
        Title = "Auto-Forge: Idle",
        Desc = "Peak Efficiency: 31 Ores -> Heavy Armor ($37.88 * Mult / ore)"
    })
    ForgeEngine.AutoForgeStatusLabel = AutoForgeStatusLbl

    -- Always use the mathematical Gold/ore peak batch size (31 ores), draining leftovers down to 3 ores automatically
    Config.AutoForgeTargetOres = 31

    ForgeEngine.BlacklistOresDropdown = ProfitSection:Dropdown({
        Title = "Blacklist Ores From Forging",
        Desc = "Picked ores are strictly protected and NEVER used in any forge",
        List = ForgeEngine.ForgeableOreNames,
        Value = Config.ForgeOreBlacklist,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.ForgeOreBlacklist = val
            elseif type(val) == "string" then
                Config.ForgeOreBlacklist = { val }
            else
                Config.ForgeOreBlacklist = {}
            end
        end
    })

    ProfitSection:Toggle({
        Title = "Auto Forge & Sell Armor (When Full)",
        Desc = "Waits until threshold/full, crafts all usable non-blacklisted ores, equips best & sells crafted Armors",
        Value = Config.AutoForgeArmor,
        Callback = function(val)
            Config.AutoForgeArmor = val
            if not val then
                State.DrainingFullBag = false
                State.ManualDrainOnce = false
                State.StashFullSignalUntil = 0
                if State.IsAutoForging and not Config.WeaponHuntRunning then
                    NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
                    pcall(NavEngine.RestoreCharacterPhysics)
                end
                ForgeEngine.SetStatus("Auto-Forge: Off", "Idle")
            end
        end
    })

    ProfitSection:Toggle({
        Title = "Fast Forge (Gamepass)",
        Desc = "Uses your Fast Forge gamepass during Auto Forge to skip the Melt & Pour minigames (requires Fast Forge gamepass)",
        Value = Config.AutoFastForge,
        Callback = function(val)
            Config.AutoFastForge = val
            pcall(function()
                local UIC = SafeGetController("UIController")
                if UIC and UIC.Modules and UIC.Modules.Forge then
                    ForgeEngine.ApplyFastForgePreference(UIC.Modules.Forge)
                end
            end)
        end
    })

    ProfitSection:Slider({
        Title = "Auto-Forge Trigger Bag %",
        Desc = "Stash fill % at which Auto Forge starts draining all non-blacklisted ores (100% = full)",
        Min = 25,
        Max = 100,
        Value = Config.AutoForgeTriggerPct,
        Rounding = 0,
        Callback = function(val)
            Config.AutoForgeTriggerPct = val
        end
    })

    ProfitSection:Button({
        Title = "Forge All Remaining Ores Now",
        Desc = "Immediately drains ALL non-blacklisted ores (< 3 left), equips best & sells crafted Armor",
        Callback = function()
            local usable = 0
            for _, ore in ipairs(ForgeEngine.GetPlayerOreInventory()) do
                usable = usable + (ore.Count or 0)
            end
            if usable < 3 then
                Notify("Auto-Forge", string.format("Only %d non-blacklisted ore(s) available (need >= 3).", usable), 3.0)
                return
            end
            State.DidWeaponUpgradeCheckThisCycle = false
            State.ManualDrainOnce = true
            State.DrainingFullBag = true
            Notify("Auto-Forge", string.format("Draining all %d non-blacklisted ores in inventory...", usable), 3.5)
        end
    })

    ProfitSection:Toggle({
        Title = "Equip Best Then Sell",
        Desc = "Equips your highest-HP Helmet, Chestplate & Leggings (and better forged Weapon) before selling",
        Value = Config.AutoEquipBestArmor,
        Callback = function(val)
            Config.AutoEquipBestArmor = val
            if val then
                task.spawn(function()
                    State.LastBestArmorCheck = 0
                    ForgeEngine.EquipBestArmor()
                end)
            end
        end
    })

    ProfitSection:Toggle({
        Title = "Craft Best Weapon Before Armor",
        Desc = "Crafts 1 Weapon before Armor ONLY if calculator beats your current weapon (checks RNG class & stats)",
        Value = Config.AutoCraftBestWeaponFirst,
        Callback = function(val)
            Config.AutoCraftBestWeaponFirst = val
        end
    })

    ProfitSection:Dropdown({
        Title = "Weapon Class Filter",
        Desc = "Only equips this class & aims for its highest forge % (rejects off-class RNG rolls)",
        List = ForgeEngine.WeaponClassDisplayList,
        Value = Config.WeaponClassTarget,
        Callback = function(val)
            if type(val) == "string" then
                Config.WeaponClassTarget = val
                if Config.AutoEquipBestArmor then
                    task.spawn(function()
                        State.LastBestArmorCheck = 0
                        ForgeEngine.EquipBestArmor()
                    end)
                end
            end
        end
    })

    ProfitSection:Dropdown({
        Title = "Compare Weapons By",
        Desc = "DPS = Damage per second (fast weapons win) | Base Damage = biggest single hit",
        List = { "Base DPS", "Base Damage" },
        Value = Config.WeaponCompareMode,
        Callback = function(val)
            if type(val) == "string" then
                Config.WeaponCompareMode = val
                if Config.AutoEquipBestArmor then
                    task.spawn(function()
                        State.LastBestArmorCheck = 0
                        ForgeEngine.EquipBestArmor()
                    end)
                end
            end
        end
    })

    ForgeEngine.GetOwnedWeaponLabels()

    ProfitSection:Toggle({
        Title = "Protect Custom / Main Weapon",
        Desc = "Enable if you have a custom/meta weapon you NEVER want Auto-Forge to replace",
        Value = Config.ProtectCustomWeapon,
        Callback = function(val)
            Config.ProtectCustomWeapon = val
            if val and ForgeEngine.ProtectedWeaponDropdown and type(ForgeEngine.ProtectedWeaponDropdown.Refresh) == "function" then
                local labels = ForgeEngine.GetOwnedWeaponLabels()
                local curChoice = ForgeEngine.ProtectedWeaponMap[Config.ProtectedWeaponChoice] and Config.ProtectedWeaponChoice or "Auto (Equipped Weapon)"
                pcall(function() ForgeEngine.ProtectedWeaponDropdown:Refresh(labels, curChoice) end)
            end
        end
    })

    ForgeEngine.ProtectedWeaponDropdown = ProfitSection:Dropdown({
        Title = "Main Weapon To Keep",
        Desc = "Select your custom/main weapon that will never be replaced or crafted over",
        List = ForgeEngine.ProtectedWeaponLabels,
        Value = Config.ProtectedWeaponChoice,
        Callback = function(val)
            if type(val) == "string" then
                Config.ProtectedWeaponChoice = val
            end
        end
    })

    ForgeEngine.GetOwnedArmorLabels()

    ProfitSection:Toggle({
        Title = "Protect Custom / Main Armor",
        Desc = "Enable if you have custom/meta Armor pieces you NEVER want Equip Best Then Sell to replace",
        Value = Config.ProtectCustomArmor,
        Callback = function(val)
            Config.ProtectCustomArmor = val
            if val and ForgeEngine.ProtectedArmorDropdown and type(ForgeEngine.ProtectedArmorDropdown.Refresh) == "function" then
                local labels = ForgeEngine.GetOwnedArmorLabels()
                pcall(function() ForgeEngine.ProtectedArmorDropdown:Refresh(labels, Config.ProtectedArmorChoices) end)
            end
        end
    })

    ForgeEngine.ProtectedArmorDropdown = ProfitSection:Dropdown({
        Title = "Armor To Keep (Main Armor)",
        Desc = "Select custom Armor pieces to never replace or sell (or keep Auto = Equipped Armor)",
        List = ForgeEngine.ProtectedArmorLabels,
        Value = Config.ProtectedArmorChoices,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.ProtectedArmorChoices = val
            elseif type(val) == "string" then
                Config.ProtectedArmorChoices = { val }
            else
                Config.ProtectedArmorChoices = { "Auto (Equipped Armor)" }
            end
        end
    })

    local WeaponHuntSection = SubForge:Section({ Title = "1-1-1-1 Target Sword Hunt (Weapon Override)", Side = "Right" })

    ForgeEngine.WeaponHuntStatusLabel = WeaponHuntSection:Label({
        Title = "1-1-1-1 Hunt: Ready",
        Desc = "Select Target Sword + 4 distinct ores (1 in each slot) & toggle Hunt or Override"
    })

    WeaponHuntSection:Toggle({
        Title = "Override Every Weapon Forge (1-1-1-1)",
        Desc = "Overrules 'Craft Best Weapon Before Armor' & 'Craft Best Recipe (Weapon)' with 1-1-1-1 Hunt",
        Value = Config.WeaponHuntOverride,
        Callback = function(val) Config.WeaponHuntOverride = val end
    })

    WeaponHuntSection:Toggle({
        Title = "Auto 1-1-1-1 Hunt Now (Keep Rolling Until Got)",
        Desc = "Continuously crafts 1-1-1-1 Weapons right now until your target sword drops or ores run out",
        Value = Config.WeaponHuntRunning,
        Callback = function(val)
            Config.WeaponHuntRunning = val
            if val then
                local b, r = ForgeEngine.GetWeaponHuntBatch()
                if not b then
                    Notify("1-1-1-1 Weapon Hunt", r or "Configure 4 non-blacklisted ores first!", 3.5)
                end
            else
                if State.IsAutoForging and not Config.AutoForgeArmor and not State.ManualDrainOnce then
                    NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
                    pcall(NavEngine.RestoreCharacterPhysics)
                end
                ForgeEngine.SetWeaponHuntStatus("1-1-1-1 Hunt: Stopped", "Disabled by user")
            end
        end
    })

    local targetWeaponList = ForgeEngine.GetTargetWeaponOptions()
    WeaponHuntSection:Dropdown({
        Title = "Target Sword / Weapon",
        Desc = "Stops immediately as soon as this sword/weapon is forged",
        List = targetWeaponList,
        Value = Config.WeaponHuntTarget,
        Callback = function(val)
            if type(val) == "string" then Config.WeaponHuntTarget = val end
        end
    })

    local huntOreList = { "Auto: Best From Inventory" }
    for _, oName in ipairs(ForgeEngine.AllOreNames or {}) do
        if oName ~= "Auto: Best From Inventory" then
            table.insert(huntOreList, oName)
        end
    end

    for slotIdx = 1, 4 do
        local cfgKey = "WeaponHuntOre" .. tostring(slotIdx)
        if type(Config[cfgKey]) ~= "string" or Config[cfgKey] == "" or Config[cfgKey] == "None" then
            Config[cfgKey] = "Auto: Best From Inventory"
        end
        ForgeEngine.HuntSlotDropdowns[slotIdx] = WeaponHuntSection:Dropdown({
            Title = "1-1-1-1 Slot " .. tostring(slotIdx) .. " Ore (x1)",
            Desc = "Defaults to Auto: Best From Inventory (highest multiplier distinct ore), or pick a specific ore",
            List = huntOreList,
            Value = Config[cfgKey],
            Callback = function(val)
                if type(val) == "string" then Config[cfgKey] = val end
            end
        })
    end

    WeaponHuntSection:Toggle({
        Title = "Auto-Refund Ores on Failed Rolls",
        Desc = "Recycles unwanted 1-1-1-1 rolls immediately via InventoryService:RecycleBasket to refund your ores (costs Gold; falls back to Sell if out of Gold)",
        Value = Config.WeaponHuntRefundDuds,
        Callback = function(val) Config.WeaponHuntRefundDuds = val end
    })

    WeaponHuntSection:Toggle({
        Title = "Auto-Sell Failed Hunt Rolls",
        Desc = "Sells unwanted duds rolled during 1-1-1-1 hunt (never touches targets, favorites, or old gear)",
        Value = Config.WeaponHuntSellDuds,
        Callback = function(val) Config.WeaponHuntSellDuds = val end
    })

    WeaponHuntSection:Toggle({
        Title = "Reserve Hunt Ores From Armor Drain",
        Desc = "Protects the 4 chosen hunt ores from being spent on Auto-Forge Armor while Override/Hunt is ON",
        Value = Config.WeaponHuntReserveOres,
        Callback = function(val) Config.WeaponHuntReserveOres = val end
    })

    local MinigamesSection = SubForge:Section({ Title = "Manual Minigame Auto-Solvers", Side = "Right" })

    MinigamesSection:Toggle({
        Title = "Auto Hammer (Perfect QTE)",
        Desc = "For manual forging — Auto Forge always uses all solvers automatically",
        Value = Config.AutoHammer,
        Callback = function(val) Config.AutoHammer = val end
    })

    MinigamesSection:Toggle({
        Title = "Auto Pour Balance",
        Desc = "For manual forging — keeps pour centered at 60 FPS",
        Value = Config.AutoPour,
        Callback = function(val) Config.AutoPour = val end
    })

    MinigamesSection:Toggle({
        Title = "Auto Melt Heater",
        Desc = "For manual forging — pumps heater bellows automatically",
        Value = Config.AutoMelt,
        Callback = function(val) Config.AutoMelt = val end
    })

    MinigamesSection:Toggle({
        Title = "Auto Mold Breaker",
        Desc = "For manual forging — clicks anvil mold 3x automatically",
        Value = Config.AutoMoldBreak,
        Callback = function(val) Config.AutoMoldBreak = val end
    })

    MinigamesSection:Toggle({
        Title = "Auto Accept Forged Gear",
        Desc = "For manual forging — accepts stats on EndScreen showcase",
        Value = Config.AutoClaimForged,
        Callback = function(val) Config.AutoClaimForged = val end
    })

    MinigamesSection:Toggle({
        Title = "Auto Claim Completed Smelts",
        Desc = "Automatically claims finished smelts (always active during Auto Forge)",
        Value = Config.AutoClaimSmelt,
        Callback = function(val) Config.AutoClaimSmelt = val end
    })

    -- ===================================================================
    -- SUB-SECTION 4: SHOP (AUTO BUY & AUTO USE POTIONS)
    -- ===================================================================
    local PotionBuySection = SubShop:Section({ Title = "Auto Buy Potions", Side = "Left" })

    PotionEngine.StatusLabel = PotionBuySection:Label({
        Title = "Potions: Idle",
        Desc = "Auto-glides to current map Potion Shop to restock & maintains active potion buffs"
    })

    PotionBuySection:Toggle({
        Title = "Auto Buy Potions",
        Desc = "Smooth-glides to Potion Shop when owned stock drops below Target Stock",
        Value = Config.AutoBuyPotions,
        Callback = function(val)
            Config.AutoBuyPotions = val
            if val then
                PotionEngine.LastBuyCheckAt = 0
                table.clear(PotionEngine.LastBuyFailAt)
                if Config.AutoMine then
                    State.CheckingPotionsOnStart = true
                    task.spawn(PotionEngine.CheckOnAutoMineStart)
                end
            else
                State.CheckingPotionsOnStart = false
                if State.IsBuyingPotions then
                    State.IsBuyingPotions = false
                    NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
                    pcall(NavEngine.RestoreCharacterPhysics)
                end
            end
        end
    })

    PotionEngine.BuyDropdown = PotionBuySection:Dropdown({
        Title = "Potions To Buy",
        Desc = "Select shop potions to automatically restock on the active map (Multi-Select)",
        List = PotionEngine.ShopPotionLabels,
        Value = Config.AutoBuyPotionTypes,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.AutoBuyPotionTypes = val
            elseif type(val) == "string" then
                Config.AutoBuyPotionTypes = { val }
            end
            table.clear(PotionEngine.LastBuyFailAt)
        end
    })

    PotionBuySection:Slider({
        Title = "Target Stock Per Potion",
        Desc = "Restocks selected shop potions up to this quantity in bag",
        Min = 1,
        Max = 10,
        Value = Config.AutoBuyPotionStock,
        Rounding = 0,
        Callback = function(val) Config.AutoBuyPotionStock = val end
    })

    PotionBuySection:Toggle({
        Title = "Buy Only After Buffs Expire",
        Desc = "Waits until all active potion buffs finish before leaving Auto Mine to buy",
        Value = Config.AutoBuyAfterBuffsExpire,
        Callback = function(val) Config.AutoBuyAfterBuffsExpire = val end
    })

    PotionBuySection:Slider({
        Title = "Buy Every X Minutes",
        Desc = "Minimum minutes between potion shop runs (0 = no interval cooldown)",
        Min = 0,
        Max = 60,
        Value = Config.AutoBuyPotionIntervalMin,
        Rounding = 0,
        Callback = function(val) Config.AutoBuyPotionIntervalMin = val end
    })

    local PotionUseSection = SubShop:Section({ Title = "Auto Use Potions", Side = "Right" })

    PotionUseSection:Toggle({
        Title = "Auto Use Potions",
        Desc = "Automatically drinks & stacks selected potions up to max duration (or below HP% for healing)",
        Value = Config.AutoUsePotions,
        Callback = function(val)
            Config.AutoUsePotions = val
            if val and Config.AutoMine then
                State.CheckingPotionsOnStart = true
                task.spawn(PotionEngine.CheckOnAutoMineStart)
            end
        end
    })

    PotionEngine.UseDropdown = PotionUseSection:Dropdown({
        Title = "Potions To Use",
        Desc = "Select potions from stash/bag to keep active automatically (Multi-Select)",
        List = PotionEngine.AllPotionLabels,
        Value = Config.AutoUsePotionTypes,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.AutoUsePotionTypes = val
            elseif type(val) == "string" then
                Config.AutoUsePotionTypes = { val }
            end
        end
    })

    PotionUseSection:Slider({
        Title = "Health Potion Trigger HP %",
        Desc = "Auto-drinks selected Health Potions when HP drops to or below this percentage",
        Min = 10,
        Max = 95,
        Value = Config.AutoUseHealthPct,
        Rounding = 0,
        Callback = function(val) Config.AutoUseHealthPct = val end
    })
end

-- TAB 2: INTERACTIVE FORGE CALCULATOR (theforge.gg/calculator Replica)
do
    local CalcTab = Window:Tab({ Title = "Calculator", Icon = "calculator" })
    local ResultsSection = CalcTab:Section({ Title = "Live Forge Calculator Results (Armor & Weapon)", Side = "Right" })
    local SlotsSection = CalcTab:Section({ Title = "Category & Crucible Ore Slots (1-4)", Side = "Left" })
    local CategoryDropdown = nil

    ForgeEngine.CalcSummaryLabel = ResultsSection:Label({
        Title = "Est. Sell: --",
        Desc = "Adjust Category or Ore Slots 1-4 below"
    })

    ForgeEngine.CalcChancesLabel = ResultsSection:Label({
        Title = "Class Chances: --",
        Desc = "Active Traits: --"
    })

    ForgeEngine.CalcAltCompareLabel = ResultsSection:Label({
        Title = "Comparison Preview: --",
        Desc = "Both Weapon Damage & Armor Profit are calculated live"
    })

    local function fillCalculatorSlots(batch, categoryType)
        if not batch or not next(batch) then
            Notify("Forge Calculator", "Not enough non-blacklisted ores in inventory for " .. tostring(categoryType or "recipe") .. ".", 3.5)
            return
        end
        local entries = {}
        for k, v in pairs(batch) do
            local oData = ForgeEngine.OreDataByName[k]
            local multVal = oData and ((categoryType == "Weapon") and oData.Multiplier or (oData.PriceMultiplier or oData.Multiplier)) or 0
            table.insert(entries, { ore = k, count = v, pm = multVal })
        end
        table.sort(entries, function(a, b) return a.pm > b.pm end)
        for idx = 1, 4 do
            local e = entries[idx]
            local oreVal = e and e.ore or "None"
            local cntVal = e and e.count or 0
            Config["CalcSlot" .. idx .. "Ore"] = oreVal
            Config["CalcSlot" .. idx .. "Count"] = cntVal
            if ForgeEngine.SlotDropdowns[idx] and ForgeEngine.SlotDropdowns[idx].SetValue then
                pcall(function() ForgeEngine.SlotDropdowns[idx]:SetValue(oreVal, true) end)
            end
            if ForgeEngine.SlotSliders[idx] and ForgeEngine.SlotSliders[idx].SetValue then
                pcall(function() ForgeEngine.SlotSliders[idx]:SetValue(cntVal, true) end)
            end
        end
        Config.CalcItemType = categoryType
        if CategoryDropdown and CategoryDropdown.SetValue then
            pcall(function() CategoryDropdown:SetValue(categoryType, true) end)
        end
        ForgeEngine.UpdateCalculatorUI()
        if Config.AutoSaveConfig ~= false and ConfigManager.LoadedOk ~= false then
            ConfigManager.ScheduleSave(0.08)
        end
    end

    ResultsSection:Button({
        Title = "Load Best Profit Armor Recipe (From My Inventory)",
        Desc = "Auto-fills Slots 1-4 with your highest-efficiency Armor ore combo",
        Callback = function()
            local batch = ForgeEngine.GetOptimalArmorBatch(Config.AutoForgeTargetOres or 31)
            if not batch then
                batch = ForgeEngine.GetOptimalArmorBatch(3)
            end
            fillCalculatorSlots(batch, "Armor")
        end
    })

    ResultsSection:Button({
        Title = "Load Best Weapon Recipe (From My Inventory)",
        Desc = "Switches to Weapon mode & fills highest damage multiplier ores (or 1-1-1-1 Hunt ores if Override ON)",
        Callback = function()
            local batch = nil
            if Config.WeaponHuntOverride then
                batch = ForgeEngine.GetWeaponHuntBatch()
            end
            if not batch then
                batch = ForgeEngine.GetOptimalWeaponBatch(35)
            end
            fillCalculatorSlots(batch, "Weapon")
        end
    })

    ResultsSection:Button({
        Title = "Load Best Golem Recipe (From My Inventory)",
        Desc = "Switches to Golem mode & fills highest-rarity ores (10+ ores required)",
        Callback = function()
            local batch = ForgeEngine.GetOptimalGolemBatch(10)
            fillCalculatorSlots(batch, "Golem")
        end
    })

    ResultsSection:Dropdown({
        Title = "Craft Best Category",
        Desc = "Choose whether 'Craft Best Recipe' crafts Armor or Weapon",
        List = { "Armor", "Weapon" },
        Value = Config.CalcCraftCategory,
        Callback = function(val) Config.CalcCraftCategory = val end
    })

    ResultsSection:Dropdown({
        Title = "Armor Piece Selection",
        Desc = "Pick which Armor piece(s) to craft when Category is Armor",
        List = { "All 3 Armor Pieces", "Best Value Armor (Any)", "Helmet Only", "Chestplate Only", "Leggings Only" },
        Value = Config.CalcArmorPiece,
        Callback = function(val) Config.CalcArmorPiece = val end
    })

    ResultsSection:Button({
        Title = "Craft Best Recipe",
        Desc = "Crafts best Weapon or Armor piece(s) from inventory and KEEPS them (no auto-sell)",
        Callback = function()
            if State.IsAutoForging then return end
            task.spawn(function()
                local cat = (Config.CalcCraftCategory == "Weapon") and "Weapon" or "Armor"
                if cat == "Weapon" then
                    if Config.WeaponHuntOverride then
                        local hBatch = ForgeEngine.GetWeaponHuntBatch()
                        if hBatch then
                            fillCalculatorSlots(hBatch, "Weapon")
                        end
                        ForgeEngine.RunWeaponHunt1111({ triggeredByOverride = true })
                        return
                    end
                    local batch = ForgeEngine.GetOptimalWeaponBatch(35)
                    if batch then
                        fillCalculatorSlots(batch, "Weapon")
                        ForgeEngine.ExecuteArmorForgeAndSell(batch, "Weapon", true)
                    end
                    return
                end

                local pieceMode = Config.CalcArmorPiece or "All 3 Armor Pieces"
                if pieceMode == "All 3 Armor Pieces" then
                    local queue = ForgeEngine.BuildAll3ArmorQueue()
                    if #queue > 0 then
                        fillCalculatorSlots(queue[1].ores, "Armor")
                        ForgeEngine.ExecuteArmorForgeAndSell(queue, "Armor", true)
                    end
                elseif pieceMode == "Helmet Only" or pieceMode == "Chestplate Only" or pieceMode == "Leggings Only" then
                    local slotKind = pieceMode:gsub(" Only", "")
                    local batch, _, lClass = ForgeEngine.GetOptimalPieceBatch(slotKind)
                    if batch then
                        fillCalculatorSlots(batch, "Armor")
                        ForgeEngine.ExecuteArmorForgeAndSell(batch, "Armor", true, lClass)
                    end
                else
                    local batch = ForgeEngine.GetOptimalArmorBatch(Config.AutoForgeTargetOres or 31)
                    if not batch then
                        batch = ForgeEngine.GetOptimalArmorBatch(3)
                    end
                    if batch then
                        fillCalculatorSlots(batch, "Armor")
                        ForgeEngine.ExecuteArmorForgeAndSell(batch, "Armor", true)
                    end
                end
            end)
        end
    })

    CategoryDropdown = SlotsSection:Dropdown({
        Title = "Calculator Category",
        Desc = "Switch primary view between Armor, Weapon, and Golem",
        List = { "Armor", "Weapon", "Golem" },
        Value = Config.CalcItemType,
        Callback = function(val)
            Config.CalcItemType = val
            ForgeEngine.UpdateCalculatorUI()
        end
    })

    for idx = 1, 4 do
        local oreKey = "CalcSlot" .. idx .. "Ore"
        local cntKey = "CalcSlot" .. idx .. "Count"
        ForgeEngine.SlotDropdowns[idx] = SlotsSection:Dropdown({
            Title = "Slot " .. idx .. " Ore Type",
            Desc = "Select ore for Crucible Slot " .. idx,
            List = ForgeEngine.AllOreNames,
            Value = Config[oreKey],
            Callback = function(val)
                Config[oreKey] = val
                ForgeEngine.UpdateCalculatorUI()
            end
        })
        ForgeEngine.SlotSliders[idx] = SlotsSection:Slider({
            Title = "Slot " .. idx .. " Ore Count",
            Desc = "Number of ores in Slot " .. idx,
            Min = 0,
            Max = 50,
            Value = Config[cntKey],
            Rounding = 0,
            Callback = function(val)
                Config[cntKey] = val
                ForgeEngine.UpdateCalculatorUI()
            end
        })
    end

    ForgeEngine.UpdateCalculatorUI()
end

-- TAB 3: COMBAT & MOBS (SubTabs: Auto Farm, Auto Parry, Runes, Abyss / Raid)
do
    local CombatTab = Window:Tab({ Title = "Combat", Icon = "shield" })
    local SubAutoFarm = (typeof(CombatTab.SubTab) == "function") and CombatTab:SubTab("Auto Farm") or CombatTab
    local SubAutoParry = (typeof(CombatTab.SubTab) == "function") and CombatTab:SubTab("Auto Parry") or CombatTab
    local SubRunes = (typeof(CombatTab.SubTab) == "function") and CombatTab:SubTab("Runes") or CombatTab
    local SubAbyss = (typeof(CombatTab.SubTab) == "function") and CombatTab:SubTab("Abyss / Raid") or CombatTab

    -- ===================================================================
    -- SUB-TAB 1: AUTO FARM (Mob Automation & Stance Tuning)
    -- ===================================================================
    local MobSection = SubAutoFarm:Section({ Title = "Mob Hunting", Side = "Left" })
    local HitboxSection = SubAutoFarm:Section({ Title = "Position Sliders", Side = "Right" })

    MobSection:Toggle({
        Title = "Auto Farm Mobs",
        Desc = "Hunts selected enemies from your chosen attack position",
        Value = Config.AutoMobFarm,
        Callback = function(val)
            Config.AutoMobFarm = val
            if not val then
                State.TargetMob = nil
                State.CurrentMob = nil
                State.LastFarmedMob = nil
                CombatEngine.LastEngagedMob = nil
                CombatEngine.ClearStanceLock()
                if NavEngine.IsGliding and not Config.AutoMine and not Config.AutoRaid and not State.IsAutoForging then
                    NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
                    pcall(NavEngine.RestoreCharacterPhysics)
                end
                if not Config.AutoMine and State.HRP then
                    pcall(NavEngine.SurfaceIfUnderground)
                    if State.HRP.Anchored then
                        pcall(function() State.HRP.Anchored = false end)
                    end
                end
                if State.Humanoid then
                    pcall(function() State.Humanoid.AutoRotate = true end)
                end
                CombatEngine.IsParrying = false
                CombatEngine.ParryHoldUntil = 0
                State.TargetMob = nil
                State.CurrentMob = nil
                State.LastFarmedMob = nil
                State.ResumingMobAfterDeath = false
                pcall(function()
                    local TC = SafeGetController("ToolController")
                    if TC then TC.isBlocking = false end
                end)
            end
        end
    })

    MobSection:Toggle({
        Title = "Return to Same Mob After Death",
        Desc = "Always goes back to the exact mob you died fighting upon respawn",
        Value = Config.ResumeLastMobAfterDeath,
        Callback = function(val) Config.ResumeLastMobAfterDeath = val end
    })

    local targetMobsDropdown = nil
    targetMobsDropdown = MobSection:Dropdown({
        Title = "Target Mobs (Current Island)",
        Desc = "Filtered to enemies on this island (select All for any hostile mob)",
        List = MobTypes,
        Value = Config.MobFarmTargets,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.MobFarmTargets = val
            elseif type(val) == "string" then
                Config.MobFarmTargets = { val }
            else
                Config.MobFarmTargets = { "All" }
            end
            State.TargetMob = nil
        end
    })

    MobSection:Toggle({
        Title = "Prioritize Strongest Mob",
        Desc = "When multiple mobs are selected, kills strongest to weakest first (e.g. Hellbringer -> Spirit Golem -> Pale Oni)",
        Value = Config.PrioritizeStrongestMob,
        Callback = function(val)
            Config.PrioritizeStrongestMob = val
            State.TargetMob = nil
        end
    })

    MobSection:Toggle({
        Title = "Separate Limbo / Heaven / Hell Mobs (This World)",
        Desc = "When ON, normal targets skip Limbo/Heaven/Hell mutated mobs unless selected separately (per-world setting)",
        Value = Config.SplitEventVariantMobs,
        Callback = function(val)
            Config.SplitEventVariantMobs = val
            ConfigManager.RebuildMobTypes()
            Config.MobFarmTargets = ConfigManager.SanitizeMultiList(Config.MobFarmTargets, ConfigManager.ValidMobSet, "All")
            if targetMobsDropdown and type(targetMobsDropdown.Refresh) == "function" then
                pcall(function() targetMobsDropdown:Refresh(MobTypes, Config.MobFarmTargets) end)
            end
            State.TargetMob = nil
        end
    })

    if Config.MobFarmPositionMode == "Underground" or Config.MobFarmPositionMode == "Underneath Enemy" then
        Config.MobFarmPositionMode = "Under Map"
    elseif Config.MobFarmPositionMode ~= "Behind Enemy" and Config.MobFarmPositionMode ~= "Under Map" then
        Config.MobFarmPositionMode = "Above Enemy"
    end

    MobSection:Dropdown({
        Title = "Attack Position",
        Desc = "Stand above the enemy, behind the enemy's back, or under the map beneath them",
        List = { "Above Enemy", "Behind Enemy", "Under Map" },
        Value = Config.MobFarmPositionMode,
        Callback = function(val)
            local wasUnder = (Config.MobFarmPositionMode == "Under Map")
            Config.MobFarmPositionMode = val
            if val == "Behind Enemy" then
                CombatEngine.ClearStanceLock()
            elseif wasUnder and val ~= "Under Map" then
                CombatEngine.ClearStanceLock()
                pcall(NavEngine.SurfaceIfUnderground)
            end
        end
    })

    MobSection:Toggle({
        Title = "Auto-Sell Loot to Greedy Cey (Bag Full)",
        Desc = "When bag is full while farming mobs, remotely sells unblacklisted loot to Greedy Cey (chains into Auto Sell Runes if enabled)",
        Value = Config.AutoSellMobLootWhenFull,
        Callback = function(val) Config.AutoSellMobLootWhenFull = val end
    })

    ForgeEngine.LootBlacklistDropdown = MobSection:Dropdown({
        Title = "Blacklist Loot (Keep From Selling)",
        Desc = "Select items (fragments, shards, bones, essences, runes, ores) to NEVER sell",
        List = GreedyCeyLootTypes,
        Value = Config.MobLootSellBlacklist,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.MobLootSellBlacklist = val
            elseif type(val) == "string" then
                Config.MobLootSellBlacklist = { val }
            else
                Config.MobLootSellBlacklist = {}
            end
        end
    })
    if #GreedyCeyLootTypes == 0 then
        task.spawn(function()
            for _ = 1, 60 do
                task.wait(0.5)
                if not ScriptRunning then return end
                if ConfigManager.RebuildGreedyCeyLootTypes() and ForgeEngine.LootBlacklistDropdown and type(ForgeEngine.LootBlacklistDropdown.Refresh) == "function" then
                    pcall(function() ForgeEngine.LootBlacklistDropdown:Refresh(GreedyCeyLootTypes, Config.MobLootSellBlacklist) end)
                    break
                end
            end
        end)
    end

    HitboxSection:Slider({
        Title = "Above Enemy Height (studs)",
        Desc = "How high above the enemy you float (default: 5.5)",
        Min = 2.5,
        Max = 10.0,
        Value = Config.MobFarmVerticalOffset,
        Rounding = 1,
        Callback = function(val) Config.MobFarmVerticalOffset = val end
    })

    HitboxSection:Slider({
        Title = "Behind Enemy Distance (studs)",
        Desc = "How far behind the enemy you stand in Behind Enemy mode (default: 4.0)",
        Min = 2.4,
        Max = 10.0,
        Value = Config.MobFarmDistance,
        Rounding = 1,
        Callback = function(val) Config.MobFarmDistance = val end
    })

    HitboxSection:Slider({
        Title = "Under Map Depth (studs)",
        Desc = "How far below the enemy you sit in Under Map mode (default: 6.0)",
        Min = 2.5,
        Max = 14.0,
        Value = Config.MobFarmUnderOffset,
        Rounding = 1,
        Callback = function(val) Config.MobFarmUnderOffset = val end
    })

    -- ===================================================================
    -- SUB-TAB 2: AUTO PARRY (Parry, Dodge & Manual Controls)
    -- ===================================================================
    local ParrySettingsSection = SubAutoParry:Section({ Title = "Auto Parry & Dodge", Side = "Left" })
    local ParryTimingSection = SubAutoParry:Section({ Title = "Timing & Manual Actions", Side = "Right" })

    ParrySettingsSection:Toggle({
        Title = "Enable Auto Parry",
        Desc = "Automatically parries enemy attacks via ParryIndicator & animations",
        Value = Config.AutoParry,
        Callback = function(val) Config.AutoParry = val end
    })

    ParrySettingsSection:Toggle({
        Title = "Auto-Dodge Unparryable (Red / Bomber)",
        Desc = "Sidesteps unparryable red attacks & Bomber explosive charges",
        Value = Config.AutoDodgeUnparryable,
        Callback = function(val) Config.AutoDodgeUnparryable = val end
    })

    ParrySettingsSection:Toggle({
        Title = "Auto-Equip Weapon to Parry",
        Desc = "Automatically equips Weapon if attacked while holding Pickaxe",
        Value = Config.AutoParryEquipWeapon,
        Callback = function(val) Config.AutoParryEquipWeapon = val end
    })

    ParrySettingsSection:Toggle({
        Title = "Auto-Distance From Boss Skills (Asura)",
        Desc = "Backs away cleanly during dangerous Boss skills instead of facetanking AoE/counters",
        Value = Config.EvadeBossSkills,
        Callback = function(val) Config.EvadeBossSkills = val end
    })

    ParrySettingsSection:Toggle({
        Title = "Only Distance 360° / Counter Skills",
        Desc = "ON: keeps hitting behind frontal slashes, only retreats on 360°/counter moves (Buddha Whirlwind, Prophecy Vision, Phase 2, Barrage)",
        Value = Config.EvadeBossSkills360Only,
        Callback = function(val) Config.EvadeBossSkills360Only = val end
    })

    ParryTimingSection:Slider({
        Title = "Auto Parry Range (studs)",
        Desc = "Maximum distance to enemy to trigger Auto-Parry",
        Min = 6.0,
        Max = 30.0,
        Value = Config.AutoParryRange,
        Rounding = 1,
        Callback = function(val) Config.AutoParryRange = val end
    })

    ParryTimingSection:Slider({
        Title = "Parry Hold Window (s)",
        Desc = "Duration to hold StartBlock before releasing (default: 0.35s)",
        Min = 0.15,
        Max = 0.80,
        Value = Config.ParryHoldDuration,
        Rounding = 2,
        Callback = function(val) Config.ParryHoldDuration = val end
    })

    ParryTimingSection:Slider({
        Title = "Boss Skill Safe Distance (studs)",
        Desc = "How far to retreat when Asura / Boss triggers a skill move (default: 50)",
        Min = 25,
        Max = 85,
        Value = Config.BossSkillSafeDistance,
        Rounding = 0,
        Callback = function(val) Config.BossSkillSafeDistance = val end
    })

    ParryTimingSection:Button({
        Title = "Attack with Weapon",
        Desc = "Triggers a single melee weapon attack",
        Callback = function()
            if Remotes.ToolActivated then
                pcall(function() Remotes.ToolActivated:InvokeServer("Weapon", false) end)
            end
        end
    })

    ParryTimingSection:Button({
        Title = "Block Once",
        Desc = "Blocks with weapon or shield for 0.5s",
        Callback = function()
            if Remotes.StartBlock then
                pcall(function()
                    Remotes.StartBlock:InvokeServer()
                    task.wait(0.5)
                    Remotes.StopBlock:InvokeServer()
                end)
            end
        end
    })

    -- ===================================================================
    -- SUB-TAB 3: RUNES (Best Rune Optimizer, Attach Cost & Auto Put Runes)
    -- ===================================================================
    local RuneInfoSection = SubRunes:Section({ Title = "Best Rune & Attach Cost Calculator", Side = "Right" })
    local RuneControlSection = SubRunes:Section({ Title = "Auto Put Runes & Selection", Side = "Left" })

    RuneEngine.BestRuneLabel = RuneInfoSection:Label({
        Title = "BEST Rune: Scanning...",
        Desc = "Finds the highest-% roll rune in your bag (overall or per Focus Trait)"
    })

    RuneEngine.BestPickaxeRuneLabel = RuneInfoSection:Label({
        Title = "BEST Pickaxe Rune: Scanning...",
        Desc = "Finds the best Pickaxe rune (Miner Shard) currently in your inventory"
    })

    RuneEngine.CostInfoLabel = RuneInfoSection:Label({
        Title = "Attach Cost: --",
        Desc = "Select target equipment & check open slots + Gold cost"
    })

    RuneEngine.ActionStatusLabel = RuneInfoSection:Label({
        Title = "Runes: Ready",
        Desc = "Attaches runes remotely from anywhere without leaving your spot"
    })

    RuneEngine.RefreshDropdownLists()

    RuneControlSection:Dropdown({
        Title = "Equipment Filter",
        Desc = "Show Pickaxes & Weapons together, or filter to Pickaxes Only / Weapons Only / Armor Only",
        List = { "Pickaxes & Weapons", "Pickaxes Only", "Weapons Only", "Armor Only" },
        Value = Config.RuneEquipFilter,
        Callback = function(val)
            if type(val) == "string" then
                Config.RuneEquipFilter = val
                Config.RuneTargetEquipment = ""
                RuneEngine.RefreshDropdownLists()
            end
        end
    })

    RuneEngine.EquipDropdown = RuneControlSection:Dropdown({
        Title = "Target Equipment (Pickaxe / Weapon)",
        Desc = "Choose which Pickaxe, Weapon, or Armor to evaluate open rune slots & attach cost for",
        List = RuneEngine.EquipLabels,
        Value = Config.RuneTargetEquipment ~= "" and Config.RuneTargetEquipment or RuneEngine.EquipLabels[1],
        Callback = function(val)
            if type(val) == "string" then
                Config.RuneTargetEquipment = val
                RuneEngine.RefreshRunesAndInfo()
            end
        end
    })

    RuneControlSection:Dropdown({
        Title = "Trait Focus Filter",
        Desc = "Prioritizes runes with the highest % roll for this trait (e.g. Fire %, Mining)",
        List = RuneEngine.FocusOptions,
        Value = Config.RuneTraitFocus,
        Callback = function(val)
            if type(val) == "string" then
                Config.RuneTraitFocus = val
                RuneEngine.RefreshRunesAndInfo()
            end
        end
    })

    RuneEngine.RuneDropdown = RuneControlSection:Dropdown({
        Title = "Rune To Attach",
        Desc = "Sorted highest % first; keep 'Auto: Best For Focus' or pick manually",
        List = RuneEngine.RuneLabels,
        Value = Config.RuneSelectedManual,
        Callback = function(val)
            if type(val) == "string" then
                Config.RuneSelectedManual = val
                RuneEngine.UpdateInfoWindow()
            end
        end
    })

    RuneControlSection:Toggle({
        Title = "Auto Put Best Runes",
        Desc = "Automatically attaches your highest-% compatible rune whenever Target Weapon has an open slot",
        Value = Config.AutoPutRunes,
        Callback = function(val)
            Config.AutoPutRunes = val
            if val then
                task.spawn(RuneEngine.AttachSelectedRune)
            end
        end
    })

    RuneControlSection:Button({
        Title = "Refresh Weapons, Runes & Cost",
        Desc = "Re-scans inventory runes, weapon slots, and attach Gold quote",
        Callback = function()
            RuneEngine.RefreshDropdownLists()
            RuneEngine.UpdateSellStatusLabel()
        end
    })

    local RuneSellSection = SubRunes:Section({ Title = "Auto Sell Runes (Greedy Cey)", Side = "Right" })

    RuneEngine.SellStatusLabel = RuneSellSection:Label({
        Title = "Auto Sell Runes: Off",
        Desc = "Runes: -- | Unchecked (Will Sell): -- | Bag: --/--"
    })
    RuneEngine.UpdateSellStatusLabel()

    RuneSellSection:Toggle({
        Title = "Auto Sell Runes (Instant)",
        Desc = "Instantly sells any unchecked (non-favorited) runes remotely to Greedy Cey from anywhere without waiting for bag full",
        Value = Config.AutoSellRunes,
        Callback = function(val)
            Config.AutoSellRunes = val
            if not val then
                State.PendingRuneSell = false
            else
                task.spawn(function()
                    RuneEngine.SellUnkeptRunes(false)
                end)
            end
            RuneEngine.UpdateSellStatusLabel()
        end
    })

    if #RuneEngine.RuneSellTypes == 0 then
        pcall(RuneEngine.BuildRuneSellCatalog)
    end
    RuneEngine.KeepRunesDropdown = RuneSellSection:Dropdown({
        Title = "Keep Runes (Do Not Sell)",
        Desc = "Select runes you DO NOT want to sell; everything unchecked is sold automatically",
        List = RuneEngine.RuneSellTypes,
        Value = Config.RuneSellKeepList,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.RuneSellKeepList = val
            elseif type(val) == "string" then
                Config.RuneSellKeepList = { val }
            else
                Config.RuneSellKeepList = {}
            end
            RuneEngine.UpdateSellStatusLabel()
        end
    })
    if not RuneEngine.RunesModuleLoaded or #RuneEngine.RuneSellTypes == 0 then
        task.spawn(function()
            for _ = 1, 60 do
                task.wait(0.5)
                if not ScriptRunning then return end
                local prevCount = #RuneEngine.RuneSellTypes
                pcall(RuneEngine.BuildRuneSellCatalog)
                if (RuneEngine.RunesModuleLoaded or #RuneEngine.RuneSellTypes > prevCount) and #RuneEngine.RuneSellTypes > 0 then
                    if RuneEngine.KeepRunesDropdown and type(RuneEngine.KeepRunesDropdown.Refresh) == "function" then
                        pcall(function() RuneEngine.KeepRunesDropdown:Refresh(RuneEngine.RuneSellTypes, Config.RuneSellKeepList) end)
                    end
                    RuneEngine.UpdateSellStatusLabel()
                    if RuneEngine.RunesModuleLoaded then
                        break
                    end
                end
            end
        end)
    end

    RuneSellSection:Button({
        Title = "Sell Unchecked Runes Now",
        Desc = "Immediately sells all unchecked (non-favorited) runes to Greedy Cey",
        Callback = function()
            task.spawn(function()
                RuneEngine.SellUnkeptRunes(false)
            end)
        end
    })

    -- ===================================================================
    -- SUB-TAB 4: ABYSS / RAID AUTOMATION
    -- ===================================================================
    local RaidLeft = SubAbyss:Section({ Title = "Abyss & Raid Control", Side = "Left" })
    local RaidRight = SubAbyss:Section({ Title = "Cards, Heal Orbs & Gates", Side = "Right" })

    local initialAbyssInfo = RaidEngine.GetAbyssInfo()
    local initDesc = initialAbyssInfo
        and string.format("Abyss Best Floor: %s | Cap Island: %s | Unlocked: %s",
            tostring(initialAbyssInfo.Best or 0),
            tostring(initialAbyssInfo.CapIsland or "?"),
            tostring(initialAbyssInfo.Unlocked == true))
        or "Lobby / Raid status updates automatically"

    RaidEngine.StatusLabel = RaidLeft:Label({
        Title = "Raid / Abyss: " .. tostring(RaidEngine.StatusText or "Idle"),
        Desc = initDesc
    })

    task.spawn(function()
        while ScriptRunning do
            if RaidEngine.StatusLabel then
                pcall(function()
                    local tCount = RaidEngine.GetRaidTicketCount and RaidEngine.GetRaidTicketCount() or 0
                    RaidEngine.StatusLabel:SetTitle(string.format("Tickets: %d | Status: %s", tCount, tostring(RaidEngine.StatusText or "Idle")))
                end)
            end
            task.wait(0.8)
        end
    end)

    local raidAutoToggle, raidAutoEnterToggle, asuraTowerToggle = nil, nil, nil

    raidAutoToggle = RaidLeft:Toggle({
        Title = "Enable Auto Raid / Abyss",
        Desc = "Fights waves & Bosses (Asura), uses Auto Farm stance + Auto Parry, revives & retries",
        Value = Config.AutoRaid,
        Callback = function(val)
            Config.AutoRaid = val
            if val and (Config.AutoAsuraToTower or RaidEngine.NormalizeMode(Config.RaidMode) == "Asura's Incarnate") then
                Config.RaidAutoCreateStart = true
                if raidAutoEnterToggle and raidAutoEnterToggle.SetValue then
                    pcall(function() raidAutoEnterToggle:SetValue(true, true) end)
                end
            elseif not val then
                State.IsInRaid = false
                State.IsBuyingHealthPotions = false
                RaidEngine.RecoveringHP = false
                RaidEngine.StrandedSince = nil
                CombatEngine.ActiveStanceCF = nil
                CombatEngine.LastEngagedMob = nil
                NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
                pcall(NavEngine.PreventBossArenaVoidFall)
                pcall(NavEngine.SurfaceIfUnderground)
                pcall(NavEngine.RestoreCharacterPhysics)
                if State.HRP and State.HRP.Anchored then State.HRP.Anchored = false end
                if State.Humanoid then State.Humanoid.AutoRotate = true end
            end
        end
    })

    Config.RaidMode = RaidEngine.NormalizeMode(Config.RaidMode)
    local rawModes = RaidEngine.GetAvailablePartyModes()
    local availModes = {}
    local seenModes = {}
    for _, m in ipairs(rawModes) do
        local label = RaidEngine.FormatModeLabel(m)
        if not seenModes[label] then
            seenModes[label] = true
            table.insert(availModes, label)
        end
    end
    local currentModeLabel = RaidEngine.FormatModeLabel(Config.RaidMode)
    if not seenModes[currentModeLabel] then
        table.insert(availModes, currentModeLabel)
    end

    RaidEngine.ModeDropdown = RaidLeft:Dropdown({
        Title = "Mode",
        Desc = "Pick Abyss, Tower Of Hell, or Asura's Incarnate (farms Raid Tickets)",
        List = availModes,
        Value = currentModeLabel,
        Callback = function(val)
            if Config.AutoAsuraToTower then
                Config.AutoAsuraToTower = false
                if asuraTowerToggle and asuraTowerToggle.SetValue then
                    pcall(function() asuraTowerToggle:SetValue(false, true) end)
                end
            end
            local norm = RaidEngine.NormalizeMode(val)
            Config.RaidMode = norm
            if norm == "Asura's Incarnate" then
                Config.RaidAutoCreateStart = true
                if raidAutoEnterToggle and raidAutoEnterToggle.SetValue then
                    pcall(function() raidAutoEnterToggle:SetValue(true, true) end)
                end
                if raidAutoToggle and raidAutoToggle.SetValue then
                    pcall(function() raidAutoToggle:SetValue(true) end)
                else
                    Config.AutoRaid = true
                end
            end
        end
    })

    asuraTowerToggle = RaidLeft:Toggle({
        Title = "Asura Until X Tickets -> Tower Of Hell",
        Desc = "Auto-enables Auto Raid & Enter: farms Asura until Target Tickets is met, then runs Tower Of Hell until out of tickets (toggle Auto Raid OFF to pause)",
        Value = Config.AutoAsuraToTower,
        Callback = function(val)
            Config.AutoAsuraToTower = val
            if val then
                local effMode = RaidEngine.ResolveEffectiveMode()
                Config.RaidMode = effMode
                if RaidEngine.ModeDropdown and RaidEngine.ModeDropdown.SetValue then
                    pcall(function() RaidEngine.ModeDropdown:SetValue(RaidEngine.FormatModeLabel(effMode), true) end)
                end
                Config.RaidAutoCreateStart = true
                if raidAutoEnterToggle and raidAutoEnterToggle.SetValue then
                    pcall(function() raidAutoEnterToggle:SetValue(true, true) end)
                end
                if raidAutoToggle and raidAutoToggle.SetValue then
                    pcall(function() raidAutoToggle:SetValue(true) end)
                else
                    Config.AutoRaid = true
                end
            end
        end
    })

    RaidLeft:Slider({
        Title = "Target Tickets Before Tower Of Hell",
        Desc = "Number of Raid Tickets to farm from Asura before switching to Tower Of Hell",
        Min = 1,
        Max = 50,
        Value = Config.AsuraTargetTickets,
        Rounding = 0,
        Callback = function(val) Config.AsuraTargetTickets = val end
    })

    RaidLeft:Slider({
        Title = "Raid Tier (Non-Abyss)",
        Desc = "Difficulty tier for Tower Of Hell / tiered raids (1-15)",
        Min = 1,
        Max = 15,
        Value = Config.RaidTier,
        Rounding = 0,
        Callback = function(val) Config.RaidTier = val end
    })

    raidAutoEnterToggle = RaidLeft:Toggle({
        Title = "Auto Create Party & Enter Run",
        Desc = "Enters lobby, creates party for selected mode, and starts solo/leader run",
        Value = Config.RaidAutoCreateStart,
        Callback = function(val)
            Config.RaidAutoCreateStart = val
            if not val and not RaidEngine.IsInActiveRun() then
                RaidEngine.StrandedSince = nil
                NavEngine.ActiveGlideId = (NavEngine.ActiveGlideId or 0) + 1
                pcall(NavEngine.RestoreCharacterPhysics)
            end
        end
    })

    RaidLeft:Toggle({
        Title = "Auto Revive When Downed",
        Desc = "Automatically accepts ReviveOffer inside Raid / Abyss",
        Value = Config.AutoRaidRevive,
        Callback = function(val) Config.AutoRaidRevive = val end
    })

    RaidLeft:Toggle({
        Title = "Auto Retry After End",
        Desc = "Automatically retries on defeat/clear (if leader & retry allowed)",
        Value = Config.AutoRaidRetry,
        Callback = function(val) Config.AutoRaidRetry = val end
    })

    RaidLeft:Slider({
        Title = "Stop & Leave at Floor (0 = Endless)",
        Desc = "Automatically leaves Abyss upon reaching this floor (0 keeps pushing)",
        Min = 0,
        Max = 100,
        Value = Config.AbyssStopFloor,
        Rounding = 0,
        Callback = function(val) Config.AbyssStopFloor = val end
    })

    RaidLeft:Button({
        Title = "Teleport to Abyss Lobby",
        Desc = "Enters the Abyss Lobby from the current world",
        Callback = function()
            task.spawn(function()
                if RaidEngine.IsInAbyssLobby() then
                    RaidEngine.Notify("Auto Abyss", "Already in the Abyss Lobby!", 2.5)
                    return
                end
                if Remotes.AbyssEnter then
                    local ok = pcall(function() return Remotes.AbyssEnter:InvokeServer() end)
                    RaidEngine.Notify("Auto Abyss", ok and "Entering Abyss Lobby..." or "Failed to enter Abyss Lobby", 2.5)
                end
            end)
        end
    })

    RaidLeft:Button({
        Title = "Create Party & Start Run Now",
        Desc = "Creates a party for the selected Mode and activates it immediately",
        Callback = function()
            task.spawn(function()
                local ok, msg = RaidEngine.CreateAndStartParty(Config.RaidMode, Config.RaidTier, true)
                RaidEngine.Notify("Auto Raid", tostring(msg or (ok and "Party Started!" or "Failed")), 3.0)
            end)
        end
    })

    RaidLeft:Button({
        Title = "Leave Run / Return to Island",
        Desc = "Exits current Abyss / Raid run or leaves current Party",
        Callback = function()
            task.spawn(function()
                RaidEngine.LeaveRun()
                RaidEngine.Notify("Auto Raid", "Leaving active run / party...", 2.5)
            end)
        end
    })

    RaidRight:Toggle({
        Title = "Auto Pick Abyss Cards",
        Desc = "Automatically scores & selects best card when Abyss offers choices",
        Value = Config.AutoPickAbyssCards,
        Callback = function(val) Config.AutoPickAbyssCards = val end
    })

    RaidRight:Dropdown({
        Title = "Card Priority Strategy",
        Desc = "Balanced favors Fusions/Abyssals; Max DPS allows Cursed; Safe avoids Cursed",
        List = {
            "Balanced (Meta DPS+Survive)",
            "Max Damage (Glass Cannon OK)",
            "Safe Survival (No Risky Cursed)"
        },
        Value = Config.AbyssCardStrategy,
        Callback = function(val) Config.AbyssCardStrategy = val end
    })

    RaidRight:Toggle({
        Title = "Auto Collect Heal Orbs",
        Desc = "Glides to HealOrb drops when HP falls below threshold or after wave clears",
        Value = Config.AbyssCollectHealOrbs,
        Callback = function(val) Config.AbyssCollectHealOrbs = val end
    })

    RaidRight:Slider({
        Title = "Collect Heal Orb Below HP %",
        Desc = "Immediately fetches a HealOrb mid-wave when HP drops below this %",
        Min = 15,
        Max = 98,
        Value = Config.AbyssHealOrbHP,
        Rounding = 0,
        Callback = function(val) Config.AbyssHealOrbHP = val end
    })

    RaidRight:Toggle({
        Title = "Auto Open Spawned Boss Chests",
        Desc = "Walks to and opens Boss Chests spawned inside the current floor arena",
        Value = Config.AbyssAutoAdvanceGate,
        Callback = function(val) Config.AbyssAutoAdvanceGate = val end
    })

    RaidRight:Toggle({
        Title = "Low-HP Fallback Regen (Zone Top + Pots)",
        Desc = "Rises straight to the top of your current zone out of all enemy reach and drinks Health Potions",
        Value = Config.RaidLowHpFallback,
        Callback = function(val)
            Config.RaidLowHpFallback = val
            if not val then
                RaidEngine.RecoveringHP = false
                RaidEngine.SafeRegenCF = nil
            end
        end
    })

    RaidRight:Slider({
        Title = "Retreat Below HP %",
        Desc = "Rises to top of current zone & pops Health Potions at or below this HP %",
        Min = 10,
        Max = 75,
        Value = Config.RaidFallbackEnterHP,
        Rounding = 0,
        Callback = function(val) Config.RaidFallbackEnterHP = val end
    })

    RaidRight:Slider({
        Title = "Resume Combat At HP %",
        Desc = "Drops back down to fight once HP recovers to this %",
        Min = 40,
        Max = 100,
        Value = Config.RaidFallbackResumeHP,
        Rounding = 0,
        Callback = function(val) Config.RaidFallbackResumeHP = val end
    })

    RaidRight:Toggle({
        Title = "Auto Buy Health Potions Before Run",
        Desc = "Buys Health Potions in Abyss Lobby / Island if below target stock before starting",
        Value = Config.RaidAutoBuyHealthPots,
        Callback = function(val) Config.RaidAutoBuyHealthPots = val end
    })

    RaidRight:Slider({
        Title = "Target Health Potion Stock",
        Desc = "Minimum Health Potions to carry before entering Abyss / Raid",
        Min = 1,
        Max = 10,
        Value = Config.RaidHealthPotStock,
        Rounding = 0,
        Callback = function(val) Config.RaidHealthPotStock = val end
    })

    RaidRight:Button({
        Title = "Buy Health Potions Now",
        Desc = "Glides to Health Potion III/II/I stand in Lobby/Island and restocks",
        Callback = function()
            task.spawn(function()
                if RaidEngine.IsInActiveRun() then
                    RaidEngine.Notify("Health Potions", "Wait until you are in Lobby or Island to buy potions!", 3.0)
                    return
                end
                local PE = State.PotionEngine
                if PE and type(PE.EnsureHealthPotionsStock) == "function" then
                    local bought = PE.EnsureHealthPotionsStock(Config.RaidHealthPotStock or 5, true)
                    local owned = PE.GetTotalHealthPotionsOwned()
                    RaidEngine.Notify("Health Potions", string.format("Bought: %d | Total Owned: %d", bought, owned), 3.0)
                end
            end)
        end
    })

    RaidRight:Label({
        Title = "Combat Integration",
        Desc = "Uses your Auto Farm stance (Above / Behind / Under Map), Auto Parry, Auto Dodge, and Best Weapon settings automatically."
    })
end

-- TAB 4: TELEPORTS (Per-Map Dynamic POIs, Islands & Waypoints)
do
    local TeleportTab = Window:Tab({ Title = "Teleports", Icon = "layout-grid" })

    local curIsland = TeleportEngine.GetCurrentIslandInfo()
    local islandsList = TeleportEngine.GetIslandsList()
    local mapWaypoints = TeleportEngine.GetWaypointsList()
    local mapLocs = TeleportEngine.GetMapLocations()

    local LeftSec = TeleportTab:Section({
        Title = string.format("Islands & Waypoints (%s)", curIsland.name or "World"),
        Side = "Left"
    })

    local wpUnlockedCount = 0
    for _, wp in ipairs(mapWaypoints) do
        if wp.unlocked then wpUnlockedCount = wpUnlockedCount + 1 end
    end
    TeleportEngine.WaypointStatusLabel = LeftSec:Label({
        Title = string.format("Map: %s (%d/%d Waypoints)", curIsland.name or "Unknown", wpUnlockedCount, #mapWaypoints),
        Desc = "Use Island buttons to travel between worlds; Waypoints fast-travel within this map"
    })

    LeftSec:Button({
        Title = "Unlock All Current-Map Waypoints",
        Desc = "Unlocks all Waypoint shrines on this map (glides to locked shrines if proximity needed)",
        Callback = function()
            task.spawn(function()
                if TeleportEngine.WaypointStatusLabel then
                    TeleportEngine.WaypointStatusLabel:SetTitle("Unlocking Waypoints...")
                    TeleportEngine.WaypointStatusLabel:SetDesc("Attempting remote & shrine unlock across current map...")
                end
                local newUnl, already, failed, total = TeleportEngine.UnlockAllWaypoints()
                if TeleportEngine.WaypointStatusLabel then
                    TeleportEngine.WaypointStatusLabel:SetTitle(string.format("Waypoints: %d/%d Unlocked", newUnl + already, total))
                    TeleportEngine.WaypointStatusLabel:SetDesc(string.format("Newly unlocked: %d | Already unlocked: %d | Locked/No Gold: %d", newUnl, already, failed))
                end
            end)
        end
    })

    for _, isl in ipairs(islandsList) do
        local tag = isl.isCurrent and " [Current]" or (isl.unlocked and " [Travel]" or " [Locked]")
        LeftSec:Button({
            Title = "Island: " .. isl.name .. tag,
            Desc = isl.isCurrent and "You are currently on this map" or ("PortalService -> Teleport to " .. isl.name),
            Callback = function()
                if not isl.isCurrent then
                    task.spawn(function()
                        TeleportEngine.TeleportToIsland(isl.name)
                    end)
                end
            end
        })
    end

    for _, wp in ipairs(mapWaypoints) do
        local priceTag = wp.unlocked and "Unlocked" or (wp.price > 0 and ("Locked - $" .. tostring(wp.price)) or "Locked - Free")
        LeftSec:Button({
            Title = "Waypoint: " .. wp.name .. " (" .. priceTag .. ")",
            Desc = wp.unlocked and ("Fast-travel via WaypointService to " .. wp.name) or ("Smooth-glide to " .. wp.name .. " shrine & unlock"),
            Callback = function()
                task.spawn(function()
                    if not TeleportEngine.IsWaypointUnlocked(wp.name) then
                        local ok = TeleportEngine.UnlockWaypoint(wp.name, true)
                        if ok then return end
                    end
                    TeleportEngine.TeleportToWaypoint(wp.name)
                end)
            end
        })
    end

    local RightSec = TeleportTab:Section({
        Title = "Map Locations, Caves & Tween Settings",
        Side = "Right"
    })

    local selectedLoc = mapLocs.allNames[1] or ""
    RightSec:Dropdown({
        Title = "All Map Locations & NPCs (" .. tostring(#mapLocs.allNames) .. ")",
        Desc = "Pick any Station, Cave, Pickaxe, Shop, or NPC on the current map",
        List = (#mapLocs.allNames > 0) and mapLocs.allNames or { "None" },
        Value = (selectedLoc ~= "") and selectedLoc or "None",
        Callback = function(val)
            if type(val) == "string" then
                selectedLoc = val
            end
        end
    })

    RightSec:Button({
        Title = "Glide to Selected Location",
        Desc = "Smoothly glides to the location chosen in the dropdown above",
        Callback = function()
            task.spawn(function()
                local targetCF = NavEngine.ResolvePOICFrame(selectedLoc)
                if targetCF then
                    NavEngine.GlideTo(targetCF, Config.GlideSpeed or 55, true)
                end
            end)
        end
    })

    RightSec:Button({
        Title = "Tween to Spawn",
        Desc = "Smoothly glides back to map spawn location",
        Callback = function()
            task.spawn(function()
                local spawn = workspace:FindFirstChildOfClass("SpawnLocation")
                if spawn then
                    NavEngine.GlideTo(spawn.CFrame + Vector3.new(0, 3.8, 0), Config.GlideSpeed or 55, true)
                end
            end)
        end
    })

    RightSec:Button({
        Title = "Tween to Fungi NPC",
        Desc = "Smoothly glides to Fungi (MazeMerchant) on Frostspire Expanse",
        Callback = function()
            task.spawn(function()
                local prox = workspace:FindFirstChild("Proximity")
                local fungi = prox and prox:FindFirstChild("MazeMerchant")
                if not fungi and prox then
                    for _, ch in ipairs(prox:GetChildren()) do
                        if ch:GetAttribute("BillboardName") == "Fungi" then
                            fungi = ch
                            break
                        end
                    end
                end
                local cf = fungi and TeleportEngine.GetInstanceCFrame(fungi)
                if cf then
                    NavEngine.GlideTo(cf, Config.GlideSpeed or 55, true)
                else
                    Notify("Fungi NPC", "Fungi (MazeMerchant) not found on this island (Frostspire Expanse only).", 4)
                end
            end)
        end
    })

    RightSec:Slider({
        Title = "Cross-Map Tween Speed",
        Desc = "Safe movement speed in studs/sec (40-55 = anti-tp safe)",
        Min = 20,
        Max = 60,
        Value = Config.GlideSpeed,
        Rounding = 0,
        Callback = function(val) Config.GlideSpeed = val end
    })

    RightSec:Slider({
        Title = "Min Safe Tween Y",
        Desc = "Keeps character above this Y level during cross-map tweens",
        Min = -35,
        Max = 30,
        Value = Config.MinTweenY,
        Rounding = 0,
        Callback = function(val) Config.MinTweenY = val end
    })

    for _, zone in ipairs(mapLocs.zoneButtons) do
        RightSec:Button({
            Title = zone.name,
            Desc = "Smoothly glides to " .. zone.name .. " on current map",
            Callback = function()
                task.spawn(function()
                    NavEngine.GlideTo(zone.cframe, Config.GlideSpeed or 55, true)
                end)
            end
        })
    end

    for _, st in ipairs(mapLocs.quickStations) do
        RightSec:Button({
            Title = st.name,
            Desc = "Smoothly glides to " .. st.name .. " on current map",
            Callback = function()
                task.spawn(function()
                    NavEngine.GlideTo(st.cframe, Config.GlideSpeed or 55, true)
                end)
            end
        })
    end
end

-- TAB 5: MISC & QOL (SubTabs: General, Webhooks)
do
    local MiscTab = Window:Tab({ Title = "Misc & QoL", Icon = "activity" })
    local SubMisc = (typeof(MiscTab.SubTab) == "function") and MiscTab:SubTab("General") or MiscTab
    local SubWebhooks = (typeof(MiscTab.SubTab) == "function") and MiscTab:SubTab("Webhooks") or MiscTab

    local CharSection = SubMisc:Section({ Title = "Character Modifiers", Side = "Left" })

    CharSection:Toggle({
        Title = "Custom WalkSpeed",
        Desc = "Overrides character movement speed",
        Value = Config.CustomWalkSpeed,
        Callback = function(val)
            Config.CustomWalkSpeed = val
            if not val and State.Humanoid then
                State.Humanoid.WalkSpeed = State.OriginalWalkSpeed
            end
        end
    })

    CharSection:Slider({
        Title = "WalkSpeed Value",
        Desc = "Movement speed velocity multiplier",
        Min = 16,
        Max = 60,
        Value = Config.WalkSpeedValue,
        Rounding = 0,
        Callback = function(val)
            Config.WalkSpeedValue = val
            if Config.CustomWalkSpeed and State.Humanoid then
                State.Humanoid.WalkSpeed = val
            end
        end
    })

    CharSection:Toggle({
        Title = "Noclip",
        Desc = "Disables character collision on physical boundaries",
        Value = Config.Noclip,
        Callback = function(val)
            Config.Noclip = val
            if not val and not (NavEngine and NavEngine.IsGliding) then
                pcall(NavEngine.RestoreCharacterPhysics)
            end
        end
    })

    CharSection:Toggle({
        Title = "Noclip Camera",
        Desc = "Lets your camera pass through ground & walls freely (great when looking around underground in Auto Mine)",
        Value = Config.NoclipCamera,
        Callback = function(val)
            Config.NoclipCamera = val
            pcall(MiscEngine.ApplyNoclipCamera, val)
        end
    })
    if Config.NoclipCamera then
        pcall(MiscEngine.ApplyNoclipCamera, true)
    end

    State.AntiAFKToggle = CharSection:Toggle({
        Title = "Anti-AFK (No 20-Min Kick)",
        Desc = "Disables Idled signals via getconnections + VirtualUser fallback",
        Value = Config.AntiAFK,
        Callback = function(val)
            Config.AntiAFK = val
            if State.ApplyAntiAFK then
                pcall(State.ApplyAntiAFK, val)
            end
        end
    })

    local RewardsSection = SubMisc:Section({ Title = "Auto Claims & Rewards", Side = "Left" })

    MiscEngine.ClaimStatusLabel = RewardsSection:Label({
        Title = "Index Claimable: 0",
        Desc = "Tracks Codex Index, Level Rewards & Daily/Mail"
    })

    RewardsSection:Toggle({
        Title = "Auto Claim Index (Codex)",
        Desc = "Automatically claims newly unlocked Codex entries & section rewards",
        Value = Config.AutoClaimIndex,
        Callback = function(val)
            Config.AutoClaimIndex = val
            if val then
                task.spawn(function() MiscEngine.ClaimAllRewards(false) end)
            end
        end
    })

    RewardsSection:Toggle({
        Title = "Auto Claim Level Rewards",
        Desc = "Automatically claims unlocked Level milestone rewards",
        Value = Config.AutoClaimLevelRewards,
        Callback = function(val)
            Config.AutoClaimLevelRewards = val
            if val then
                task.spawn(function() MiscEngine.ClaimAllRewards(false) end)
            end
        end
    })

    RewardsSection:Toggle({
        Title = "Auto Claim Daily & Mail",
        Desc = "Periodically claims available Daily Login & Mailbox rewards",
        Value = Config.AutoClaimDaily,
        Callback = function(val) Config.AutoClaimDaily = val end
    })

    local VisualsSection = SubMisc:Section({ Title = "World, Visuals & Utilities", Side = "Right" })

    VisualsSection:Toggle({
        Title = "Fullbright",
        Desc = "Eliminates shadows and maximizes world ambient lighting",
        Value = Config.Fullbright,
        Callback = function(val)
            Config.Fullbright = val
            ApplyFullbright(val)
        end
    })

    VisualsSection:Toggle({
        Title = "Remove Fog",
        Desc = "Clears atmospheric fog and distance haze without changing brightness",
        Value = Config.RemoveFog,
        Callback = function(val)
            Config.RemoveFog = val
            MiscEngine.ApplyRemoveFog(val)
        end
    })

    VisualsSection:Toggle({
        Title = "FPS Boost (Low GFX)",
        Desc = "Disables world shadows, particles, trails, and beams for higher FPS",
        Value = Config.FpsBoost,
        Callback = function(val)
            Config.FpsBoost = val
            MiscEngine.ApplyFpsBoost(val)
        end
    })

    VisualsSection:Button({
        Title = "Respawn / Reset Character",
        Desc = "Cleanly resets your character if stuck in terrain or animations",
        Callback = function()
            if Remotes.ResetCharacter then
                pcall(function() Remotes.ResetCharacter:InvokeServer() end)
            elseif State.Humanoid then
                pcall(function() State.Humanoid.Health = 0 end)
            end
        end
    })

    local ConfigSaveSec = SubMisc:Section({ Title = "Script Config (Auto-Save)", Side = "Right" })

    ConfigSaveSec:Toggle({
        Title = "Auto-Save Script Config",
        Desc = "Automatically persists all script toggles, sliders, and dropdowns to Aether/Configs/TheForge.json",
        Value = Config.AutoSaveConfig ~= false,
        Callback = function(val)
            Config.AutoSaveConfig = val
            if val then
                ConfigManager.LoadedOk = true
                pcall(ConfigManager.Save)
            else
                pcall(ConfigManager.Save)
            end
        end
    })

    ConfigSaveSec:Button({
        Title = "Save Script Config Now",
        Desc = "Immediately saves all current toggles, dropdowns, and sliders (global + current island) to disk",
        Callback = function()
            ConfigManager.LoadedOk = true
            local ok = ConfigManager.Save()
            Notify("Script Config", ok and "Saved all settings to Aether/Configs/TheForge.json!" or "Save failed (filesystem unavailable).", 3.0)
        end
    })

    ConfigSaveSec:Button({
        Title = "Reset All Script Settings",
        Desc = "Restores all script settings to defaults and saves immediately",
        Callback = function()
            ConfigManager.Reset()
            Notify("Script Config", "Reset all script settings to default values.", 3.0)
        end
    })

    MiscEngine.AutoExecStatusLabel = ConfigSaveSec:Label({
        Title = "Auto-Exec: Standby",
        Desc = "Re-executes script after server teleports (Islands, Abyss, Tower)"
    })

    ConfigSaveSec:Toggle({
        Title = "Auto-Execute on Teleport",
        Desc = "Uses queue_on_teleport when switching islands, Abyss Lobby, or Tower of Hell",
        Value = Config.AutoExecOnTeleport,
        Callback = function(val)
            Config.AutoExecOnTeleport = val
            if typeof(getgenv) == "function" then getgenv().__AetherAutoExecEnabled = val end
            MiscEngine.TeleportAlreadyQueued = false
            if val then
                MiscEngine.QueueOnTeleportNow(false)
            else
                MiscEngine.SetAutoExecStatus("Auto-Exec: Disabled", "Enable to queue script across server teleports")
            end
        end
    })

    if typeof(getgenv) == "function" then
        getgenv().__AetherAutoExecEnabled = (Config.AutoExecOnTeleport == true)
    end
    if Config.AutoExecOnTeleport then
        task.defer(function()
            MiscEngine.QueueOnTeleportNow(false)
        end)
    end

    ConfigSaveSec:Textbox({
        Title = "Loader URL / Code (Optional)",
        Desc = "Leave blank to auto-use local TheForge.lua (dev) or getaether.cc/loader.lua (Junkie)",
        Placeholder = "https://getaether.cc/loader.lua",
        Value = Config.AutoExecLoader,
        ClearText = false,
        Callback = function(val)
            Config.AutoExecLoader = tostring(val or ""):match("^%s*(.-)%s*$")
            MiscEngine.TeleportAlreadyQueued = false
            if Config.AutoExecOnTeleport then
                MiscEngine.QueueOnTeleportNow(false)
            end
        end
    })

    ConfigSaveSec:Button({
        Title = "Test Queue on Teleport Now",
        Desc = "Verifies queue_on_teleport and queues your loader for the next teleport",
        Callback = function()
            MiscEngine.TeleportAlreadyQueued = false
            MiscEngine.QueueOnTeleportNow(true)
        end
    })

    local WebhookSection = SubWebhooks:Section({ Title = "Discord Webhook (Stats & Rare Ores)", Side = "Left" })

    WebhookEngine.StatusLabel = WebhookSection:Label({
        Title = "Webhook: Idle",
        Desc = "Reports Money (Gold), Level, and pings on Rare Ore finds"
    })

    WebhookSection:Toggle({
        Title = "Enable Discord Webhook",
        Desc = "Activates automatic rare ore alerts and periodic Gold/Level reports",
        Value = Config.WebhookEnabled,
        Callback = function(val)
            Config.WebhookEnabled = val
            if val then
                WebhookEngine.LastStatusSentAt = os.clock()
            end
        end
    })

    WebhookEngine.UrlTextbox = WebhookSection:Textbox({
        Title = "Webhook URL",
        Desc = "Paste your Discord channel Webhook URL",
        Placeholder = "https://discord.com/api/webhooks/...",
        Value = Config.WebhookUrl,
        ClearText = false,
        Callback = function(val)
            Config.WebhookUrl = tostring(val or ""):match("^%s*(.-)%s*$")
        end
    })

    WebhookEngine.UserIdTextbox = WebhookSection:Textbox({
        Title = "Discord User ID (For Ping)",
        Desc = "User ID for <@ID> ping on rare ores, or 'everyone' / leave blank for no ping",
        Placeholder = "Optional ID or 'everyone'",
        Value = Config.WebhookUserId,
        ClearText = false,
        Callback = function(val)
            Config.WebhookUserId = tostring(val or ""):match("^%s*(.-)%s*$")
        end
    })

    WebhookSection:Toggle({
        Title = "Ping on Rare Ore Find",
        Desc = "Sends instant Discord alert + ping when mining any matching rare ore",
        Value = Config.WebhookNotifyRareOres,
        Callback = function(val) Config.WebhookNotifyRareOres = val end
    })

    WebhookSection:Toggle({
        Title = "Webhook on Full Bag & After Forge",
        Desc = "Sends a Before status when inventory is full and an After comparison once Auto-Forge finishes",
        Value = Config.WebhookNotifyForgeCycle,
        Callback = function(val) Config.WebhookNotifyForgeCycle = val end
    })

    WebhookSection:Dropdown({
        Title = "Minimum Rare Ore Tier",
        Desc = "Automatically alert on ores at or above this rarity tier",
        List = { "Off (Custom List Only)", "Rare", "Epic", "Legendary", "Mythic", "Exotic", "Relic", "Divine" },
        Value = Config.WebhookPingMinRarity,
        Callback = function(val) Config.WebhookPingMinRarity = val end
    })

    WebhookSection:Dropdown({
        Title = "Extra Watched Rare Ores",
        Desc = "Always ping for these specific ores regardless of minimum tier",
        List = ForgeEngine.ForgeableOreNames,
        Value = Config.WebhookWatchOres,
        Multi = true,
        Callback = function(val)
            if type(val) == "table" then
                Config.WebhookWatchOres = val
            elseif type(val) == "string" then
                Config.WebhookWatchOres = { val }
            end
        end
    })

    WebhookSection:Slider({
        Title = "Status Update Interval (Min)",
        Desc = "Minutes between Gold & Level embeds (0 = rare ore alerts & test button only)",
        Min = 0,
        Max = 60,
        Value = Config.WebhookStatusIntervalMin,
        Rounding = 0,
        Callback = function(val) Config.WebhookStatusIntervalMin = val end
    })

    WebhookSection:Button({
        Title = "Send Test Webhook",
        Desc = "Sends your current Gold, Level, Island, and Bag status to Discord",
        Callback = function()
            task.spawn(function()
                WebhookEngine.SendStatusReport(true)
            end)
        end
    })

    local RaidWebhookSection = SubWebhooks:Section({ Title = "Tower of Hell & Abyss Rewards", Side = "Right" })

    WebhookEngine.RaidStatusLabel = RaidWebhookSection:Label({
        Title = "Raid Webhook: Ready",
        Desc = "Automatically tracks Gold, Items, Runes & Equipment per Tower/Abyss run"
    })

    RaidWebhookSection:Toggle({
        Title = "Send Tower of Hell / Abyss Rewards",
        Desc = "Posts a full reward & Gold breakdown embed when a Tower of Hell or Abyss run finishes",
        Value = Config.WebhookNotifyRaidRewards,
        Callback = function(val) Config.WebhookNotifyRaidRewards = val end
    })

    RaidWebhookSection:Toggle({
        Title = "Ping User ID on Raid Rewards",
        Desc = "Includes your configured Discord User ID / @everyone ping on Raid reward embeds",
        Value = Config.WebhookRaidPing,
        Callback = function(val) Config.WebhookRaidPing = val end
    })

    RaidWebhookSection:Toggle({
        Title = "Send Summary Even If 0 Items",
        Desc = "Still posts Gold, Wave/Floor, and Duration when no items/runes dropped",
        Value = Config.WebhookRaidSendEmpty,
        Callback = function(val) Config.WebhookRaidSendEmpty = val end
    })

    RaidWebhookSection:Button({
        Title = "Send Last / Test Raid Rewards",
        Desc = "Sends the last captured Tower/Abyss reward embed (or a live preview)",
        Callback = function()
            task.spawn(function()
                WebhookEngine.SendRaidRewardsReport(nil, true)
            end)
        end
    })
end

if Config.Theme and Config.Theme ~= "Halloween" and Library and Library.SetTheme then
    pcall(Library.SetTheme, Library, Config.Theme)
end

-- ===================================================================
-- 15. NON-DESTRUCTIVE CLEANUP HANDLER
-- ===================================================================
_G.__TheForge_Cleanup = function()
    if Config.AutoSaveConfig ~= false then
        pcall(ConfigManager.Save)
    end
    ScriptRunning = false

    -- Disconnect event connections
    pcall(function()
        RunService:UnbindFromRenderStep("Aether_ForgeMinigameDrive")
    end)
    for _, conn in ipairs(Connections) do
        if typeof(conn) == "RBXScriptConnection" and conn.Connected then
            pcall(function() conn:Disconnect() end)
        end
    end
    table.clear(Connections)

    -- Restore hooked functions
    for _, hookData in pairs(OrigHooks) do
        if hookData.target and hookData.method and hookData.orig then
            pcall(function()
                hookfunction(hookData.target[hookData.method], hookData.orig)
            end)
        elseif hookData.func and hookData.orig then
            pcall(function()
                hookfunction(hookData.func, hookData.orig)
            end)
        end
    end
    table.clear(OrigHooks)
    ForgeEngine.SuppressDialogueUntil = 0
    if ForgeEngine.HookedDialogueFns then
        table.clear(ForgeEngine.HookedDialogueFns)
    end

    -- Reset Lighting & Visuals
    pcall(function() ApplyFullbright(false) end)
    pcall(function() MiscEngine.ApplyRemoveFog(false) end)
    pcall(function() MiscEngine.ApplyFpsBoost(false) end)
    pcall(function() MiscEngine.ApplyNoclipCamera(false) end)

    -- Reset Humanoid WalkSpeed, PlatformStand, Collision & AutoRotate
    pcall(NavEngine.RestoreCharacterPhysics)
    if State.Humanoid then
        pcall(function() State.Humanoid.AutoRotate = true end)
        if State.OriginalWalkSpeed then
            pcall(function() State.Humanoid.WalkSpeed = State.OriginalWalkSpeed end)
        end
    end

    -- Restore HumanoidRootPart Anchored state
    if State.HRP and State.HRP.Anchored then
        pcall(function() State.HRP.Anchored = false end)
    end

    -- Restore any expanded mob hitboxes
    pcall(function()
        local living = workspace:FindFirstChild("Living")
        if living then
            for _, m in ipairs(living:GetChildren()) do
                if m:IsA("Model") and m:GetAttribute("IsNpc") == true then
                    local hrp = m:FindFirstChild("HumanoidRootPart")
                    if hrp and hrp:GetAttribute("Aether_HbSize") then
                        hrp.Size = Vector3.new(2.2, 2.2, 1.1)
                        hrp.CanCollide = false
                        hrp.Transparency = 1.0
                        hrp:SetAttribute("Aether_HbSize", nil)
                        hrp:SetAttribute("Aether_HbVis", nil)
                    end
                end
            end
        end
    end)

    -- Release any held input
    if State.PourMouseDown then
        State.PourMouseDown = false
        pcall(function()
            game:GetService("VirtualInputManager"):SendMouseButtonEvent(0, 0, 0, false, game, 0)
        end)
    end

    State.IsInRaid = false
    State.IsBuyingHealthPotions = false
    State.IsCrafting = false
    State.IsSellingRunes = false
    State.PendingRuneSell = false
    CombatEngine.ClearStanceLock()
    CombatEngine.ActiveSkillName = nil
    table.clear(CombatEngine.ActiveBossSkills)
    RaidEngine.StrandedSince = nil
    RaidEngine.LastBossArenaAt = nil
    RaidEngine.LastBossArenaPos = nil
    _G.__TheForge_Cleanup = nil
    _G.__TheForge_Config = nil
    _G.__TheForge_State = nil
    _G.__TheForge_RaidEngine = nil
end

_G.__TheForge_Config = Config
_G.__TheForge_State = State
_G.__TheForge_RaidEngine = RaidEngine

-- Auto-heal stuck ForgeUI.IsOpen state if an earlier Forge session exited without calling ForgeUI:Close()
task.spawn(function()
    pcall(function()
        local KnitPkg = safeRequire(ReplicatedStorage.Shared.Packages.Knit)
        local FC = SafeGetController("ForgeController", KnitPkg)
        local UIC = SafeGetController("UIController", KnitPkg)
        local ForgeUI = UIC and UIC.Modules and UIC.Modules.Forge
        if ForgeUI and ForgeUI.IsOpen and FC and not FC.ForgeActive then
            ForgeEngine.CloseForgeCleanly(FC, ForgeUI)
        end
    end)
end)
