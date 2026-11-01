local Players = game:GetService("Players")
local Workspace = game:GetService("Workspace")
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local ReplicatedFirst = game:GetService("ReplicatedFirst")
local RunService = game:GetService("RunService")
local UserInputService = game:GetService("UserInputService")
local CollectionService = game:GetService("CollectionService")
local CoreGui = game:GetService("CoreGui")
local HttpService = game:GetService("HttpService")

local LocalPlayer = Players.LocalPlayer
if not LocalPlayer then
    pcall(function()
        LocalPlayer = Players.PlayerAdded:Wait()
    end)
    LocalPlayer = LocalPlayer or Players.LocalPlayer
end

local Camera = Workspace.CurrentCamera or Workspace:WaitForChild("Camera", 5)
local PlayerGui = (LocalPlayer and LocalPlayer:WaitForChild("PlayerGui", 8)) or (LocalPlayer and LocalPlayer:FindFirstChildOfClass("PlayerGui"))

-- Compatibility typings for Luau language server / LSP
local firetouchinterest: any = firetouchinterest
local fireproximityprompt: any = fireproximityprompt

-- ===================================================================
-- 1. PREVIOUS INSTANCE CLEANUP
-- ===================================================================
if _G.__Nullscape_Cleanup then
    pcall(_G.__Nullscape_Cleanup)
end

local ScriptRunning = true
local Connections = {}
local function trackConn(conn)
    if conn then
        table.insert(Connections, conn)
    end
    return conn
end

-- ===================================================================
-- 1B. CLIENTEVENT REMOTE BUFFER DRAINER (Prevents 256 Queue Exhaustion)
-- ===================================================================
local function drainClientEvent(re)
    if re and re:IsA("RemoteEvent") and re.Name == "ClientEvent" and not re:GetAttribute("Aether_Drained") then
        re:SetAttribute("Aether_Drained", true)
        pcall(function()
            trackConn(re.OnClientEvent:Connect(function() end))
        end)
    end
end

for _, desc in ipairs(Workspace:GetDescendants()) do
    if desc:IsA("RemoteEvent") and desc.Name == "ClientEvent" then
        drainClientEvent(desc)
    end
end
for _, desc in ipairs(ReplicatedStorage:GetDescendants()) do
    if desc:IsA("RemoteEvent") and desc.Name == "ClientEvent" then
        drainClientEvent(desc)
    end
end
trackConn(Workspace.DescendantAdded:Connect(function(desc)
    if desc:IsA("RemoteEvent") and desc.Name == "ClientEvent" then
        drainClientEvent(desc)
    end
end))
trackConn(ReplicatedStorage.DescendantAdded:Connect(function(desc)
    if desc:IsA("RemoteEvent") and desc.Name == "ClientEvent" then
        drainClientEvent(desc)
    end
end))

-- Global Auto-Farm & Sweep State
local IsSweeping = false
local SweepStartTime = 0

-- Safely resolve all possible UI search parents
local searchParents = {}
pcall(function()
    if typeof(gethui) == "function" then
        local gh = gethui()
        if gh then table.insert(searchParents, gh) end
    end
end)
pcall(function()
    local cg = game:GetService("CoreGui")
    if cg and not table.find(searchParents, cg) then
        table.insert(searchParents, cg)
    end
end)
if PlayerGui and not table.find(searchParents, PlayerGui) then
    table.insert(searchParents, PlayerGui)
end

-- Clean previous window frames (Shadow, DropdownSelect) without destroying root container 
for _, parent in ipairs(searchParents) do
    if parent then
        pcall(function()
            for _, ch in ipairs(parent:GetChildren()) do
                if ch.Name == "Nullscape_UI" or ch.Name == "Nullscape_HUD" then
                    pcall(function() ch:Destroy() end)
                elseif ch.Name == "Aether" or ch:GetAttribute("AetherUI") then
                    for _, inner in ipairs(ch:GetChildren()) do
                        if inner.Name == "Shadow" or inner.Name == "DropdownSelect" or (inner.Name == "Frame" and inner:FindFirstChildOfClass("ScrollingFrame")) then
                            pcall(function() inner:Destroy() end)
                        end
                    end
                end
            end
        end)
    end
end

-- ===================================================================
-- 2. CONFIGURATION & STATE
-- ===================================================================
local Config = {
    -- Master ESP Toggles (Disabled by default)
    PresentsESP = false,
    EnemyESP = false,
    StatueESP = false,
    CadenceESP = false,
    PylonESP = false,
    PylonShowCompleted = false,

    -- Auto Automation Toggles (Disabled by default)
    AutoCollect = false,
    AutoStatue = false,
    AutoVoteStatue = false,
    SafeStatuesOnly = false,

    -- Statue Blacklist Dictionary
    BlacklistedStatues = {
        Chaos = true,
        Curses = true
    },

    -- Movement & Class Modifiers
    GliderInfCharge = false,
    GrapplerInfCharge = false,
    GrapplerNoCD = false,
    GrappleRange = 90,
    ChargerNoCD = false,
    DiverNoCD = false,
    DiverFreeMovement = false,
    SpiritInfCharge = false,
    SpeedMultiplier = 1.0,
    NoDiveBonk = false,
    BypassMovementCurses = false,
    SelectedClass = "Charger",

    -- Threat & Hazard Disablers (Safety enabled by default)
    TripmineDisabler = true,                       -- Disable tripmines (Enabled by default)
    ProjectileDisabler = true,                     -- Block enemy attacks & black holes (Enabled by default)
    AutoDisableEnemies = false,
    EnemyDamageImmunity = true,                    -- Keeps enemies in workspace but prevents any damage/kills (Enabled by default)
    SelectedImmunityEnemies = {
        Baby = true,
        BeaconImp = true,
        Celestial = true,
        DOG = true,
        Flesh = true,
        Fleshling = true,
        Guardian = true,
        Husk = true,
        ICBM = true,
        Kolona = true,
        Mart = true,
        Mirage = true,
        Need = true,
        Oblivion = true,
        OldMart = true,
        Operator = true,
        Ponderer = true,
        Quartz = true,
        Razorbloom = true,
        RedMart = true,
        Scrapmaw = true,
        ShadowBaby = true,
        ShadowGuardian = true,
        Sigil = true,
        Skinwalker = true,
        Slicer = true,
        Springer = true,
        Telefragger = true,
        VBOperator = true,
        VoidboundICBM = true,
        Voidbreaker = true,
        Worm = true
    },
    SelectedEnemiesToDelete = {
        Baby = true,
        BeaconImp = false,
        Celestial = false,
        DOG = true,
        Flesh = true,
        Fleshling = true,
        Guardian = true,
        Husk = true,
        ICBM = true,
        Kolona = true,
        Mart = true,
        Mirage = false,
        Need = true,
        Oblivion = true,
        OldMart = true,
        Operator = true,
        Ponderer = true,
        Quartz = true,
        Razorbloom = false,
        RedMart = false,
        Scrapmaw = true,
        ShadowBaby = true,
        ShadowGuardian = true,
        Sigil = true,
        Skinwalker = true,
        Slicer = true,
        Springer = false,
        Telefragger = true,
        VBOperator = true,
        VoidboundICBM = true,
        Voidbreaker = true,
        Worm = true
    },

    -- Upgrade Spoofer Settings
    SpoofUpgrades = false,
    DefaultUpgradeStack = 1,
    ActiveUpgrades = {
        DoubleJump = false,
        RealWings = false,
        SportShoes = false,
        SpringShoes = false,
        SwiftnessRing = false,
        IceSkates = false,
        AdvancedGravityCoil = false,
        MatrixTetrahedron = false,                 -- Invincibility / Matrix evasion
        Helmet = false,
        Shield = false,
        SubspacialBarrier = false,
        MiniatureHourglass = false,
        GiftMagnet = false,
        GiftIdol = false,
        NinjaBelt = false,
        SharkTail = false,
        DefuseKit = false,
        GraceWings = false,
        JetBoosters = false,
        PropellerHat = false,
        Adrenaline = false,
        StarMarts = false,
        TheOrb = false
    },
    UpgradeStacks = {
        DoubleJump = 3,
        RealWings = 1,
        SportShoes = 2,
        SpringShoes = 1,
        SwiftnessRing = 2,
        IceSkates = 1,
        AdvancedGravityCoil = 1,
        MatrixTetrahedron = 1,
        Helmet = 1,
        Shield = 1,
        SubspacialBarrier = 2,
        MiniatureHourglass = 1,
        GiftMagnet = 1,
        GiftIdol = 1,
        NinjaBelt = 1,
        SharkTail = 1,
        DefuseKit = 1,
        GraceWings = 1,
        JetBoosters = 1,
        PropellerHat = 1,
        Adrenaline = 1,
        StarMarts = 1,
        TheOrb = 1
    },

    -- Highlight Colors
    ColorGift = Color3.fromRGB(0, 230, 255),       -- Cyan for regular gifts
    ColorGolden = Color3.fromRGB(255, 215, 0),     -- Gold for golden gifts
    ColorPassage = Color3.fromRGB(210, 50, 255),   -- Violet for passage gifts
    ColorTripmine = Color3.fromRGB(255, 45, 45),    -- Bright Red for tripmines
    ColorEnemy = Color3.fromRGB(255, 100, 0),       -- Bright Orange-Red for enemies
    ColorCadence = Color3.fromRGB(255, 20, 147),    -- Neon Pink/Magenta for Cadence instruments

    -- Highlight Aesthetics
    FillTransparency = 1,                          -- 1 = Clean Outline, < 1 = Glowing body
    OutlineTransparency = 0,                       -- 0 = Vivid solid outline

    -- Auto-Collect Settings
    CollectRadius = 35,                            -- Studs radius (15 to 45, safe server threshold)
    CollectBatchSize = 6,                          -- Presents to collect per cycle
    CollectInterval = 0.12,                        -- Delay between collect cycles (seconds)
    PrioritizeGold = true,                         -- Collect golden gifts first (Automatic)

    -- Auto-Statue Settings
    StatueRadius = 35,                             -- Reach radius for using statues (10 to 80 studs)
    StatueCooldown = 2.0,                          -- Debounce per statue (seconds)

    -- Flash Teleport Sweeper Settings
    TeleportSweeper = false,                       -- Continuous auto-sweep loop
    TeleportSweepDelay = 0.03,                     -- Delay per hop (0.01 to 0.20s)
    MaxHopsPerSweep = 50,                          -- Max hops per cycle
    ReturnToOrigin = true,                         -- Return to starting position after sweep
    SweepGoldenFirst = true,                       -- Prioritize Golden / Passage gifts (Automatic)
    SweepGetStarMart = false,                      -- Touch StarMart (+Speed, +2 Range, 25s Debuff Immunity)
    SweepGetCadence = false,                       -- Auto-collect Cadence instruments
    SweepGetStatues = false,                       -- Auto-activate non-blacklisted statues
    AutoDispelWitherrot = false,                   -- Emergency: Auto-snap to JumpPad if Witherrot / Razorbloom attacks
    AntiVoid = true,                               -- Anti-Void: Auto-teleports to Beacon when falling towards void kill plane (Enabled by default)
    AntiVoidBuffer = 140,                          -- Distance above KillVoid to trigger (e.g. -610.5 + 140 = -470.5)
    GoldGatedBeacon = false,                       -- Lock Beacon touch until all gold presents are collected (Disabled by default)
    AutoSaveConfig = true                          -- Automatically persist toggles & sliders to disk
}

-- Default Configuration Snapshot for Factory Reset
local DefaultConfig = {
    PresentsESP = false,
    EnemyESP = false,
    StatueESP = false,
    CadenceESP = false,
    PylonESP = false,
    PylonShowCompleted = false,
    AutoCollect = false,
    AutoStatue = false,
    AutoVoteStatue = false,
    SafeStatuesOnly = false,
    BlacklistedStatues = {
        Chaos = true,
        Curses = true
    },
    GliderInfCharge = false,
    GrapplerInfCharge = false,
    GrapplerNoCD = false,
    GrappleRange = 90,
    ChargerNoCD = false,
    DiverNoCD = false,
    DiverFreeMovement = false,
    SpiritInfCharge = false,
    SpeedMultiplier = 1.0,
    NoDiveBonk = false,
    BypassMovementCurses = false,
    SelectedClass = "Charger",
    TripmineDisabler = true,
    ProjectileDisabler = true,
    AutoDisableEnemies = false,
    EnemyDamageImmunity = true,
    SelectedImmunityEnemies = {
        Baby = true,
        BeaconImp = true,
        Celestial = true,
        DOG = true,
        Flesh = true,
        Fleshling = true,
        Guardian = true,
        Husk = true,
        ICBM = true,
        Kolona = true,
        Mart = true,
        Mirage = true,
        Need = true,
        Oblivion = true,
        OldMart = true,
        Operator = true,
        Ponderer = true,
        Quartz = true,
        Razorbloom = true,
        RedMart = true,
        Scrapmaw = true,
        ShadowBaby = true,
        ShadowGuardian = true,
        Sigil = true,
        Skinwalker = true,
        Slicer = true,
        Springer = true,
        Telefragger = true,
        VBOperator = true,
        VoidboundICBM = true,
        Voidbreaker = true,
        Worm = true
    },
    SelectedEnemiesToDelete = {
        Baby = true,
        BeaconImp = false,
        Celestial = false,
        DOG = true,
        Flesh = true,
        Fleshling = true,
        Guardian = true,
        Husk = true,
        ICBM = true,
        Kolona = true,
        Mart = true,
        Mirage = false,
        Need = true,
        Oblivion = true,
        OldMart = true,
        Operator = true,
        Ponderer = true,
        Quartz = true,
        Razorbloom = false,
        RedMart = false,
        Scrapmaw = true,
        ShadowBaby = true,
        ShadowGuardian = true,
        Sigil = true,
        Skinwalker = true,
        Slicer = true,
        Springer = false,
        Telefragger = true,
        VBOperator = true,
        VoidboundICBM = true,
        Voidbreaker = true,
        Worm = true
    },
    SpoofUpgrades = false,
    DefaultUpgradeStack = 1,
    ActiveUpgrades = {
        DoubleJump = false,
        RealWings = false,
        SportShoes = false,
        SpringShoes = false,
        SwiftnessRing = false,
        IceSkates = false,
        AdvancedGravityCoil = false,
        MatrixTetrahedron = false,
        Helmet = false,
        Shield = false,
        SubspacialBarrier = false,
        MiniatureHourglass = false,
        GiftMagnet = false,
        GiftIdol = false,
        NinjaBelt = false,
        SharkTail = false,
        DefuseKit = false,
        GraceWings = false,
        JetBoosters = false,
        PropellerHat = false,
        Adrenaline = false,
        StarMarts = false,
        TheOrb = false
    },
    UpgradeStacks = {
        DoubleJump = 3,
        RealWings = 1,
        SportShoes = 2,
        SpringShoes = 1,
        SwiftnessRing = 2,
        IceSkates = 1,
        AdvancedGravityCoil = 1,
        MatrixTetrahedron = 1,
        Helmet = 1,
        Shield = 1,
        SubspacialBarrier = 2,
        MiniatureHourglass = 1,
        GiftMagnet = 1,
        GiftIdol = 1,
        NinjaBelt = 1,
        SharkTail = 1,
        DefuseKit = 1,
        GraceWings = 1,
        JetBoosters = 1,
        PropellerHat = 1,
        Adrenaline = 1,
        StarMarts = 1,
        TheOrb = 1
    },
    FillTransparency = 1,
    OutlineTransparency = 0,
    CollectRadius = 35,
    CollectBatchSize = 6,
    CollectInterval = 0.12,
    PrioritizeGold = true,
    TeleportSweeper = false,
    TeleportSweepDelay = 0.05,
    MaxHopsPerSweep = 45,
    ReturnToOrigin = true,
    SweepGoldenFirst = true,
    SweepGetStarMart = false,
    SweepGetCadence = false,
    SweepGetStatues = false,
    AutoDispelWitherrot = false,
    AntiVoid = true,
    AntiVoidBuffer = 140,
    GoldGatedBeacon = false,
    StatueRadius = 35,
    StatueCooldown = 2.0,
    AutoSaveConfig = true
}

local canonicalEnemyNames = {
    "Baby",
    "BeaconImp",
    "Celestial",
    "DOG",
    "Flesh",
    "Fleshling",
    "Guardian",
    "Husk",
    "ICBM",
    "Kolona",
    "Mart",
    "Mirage",
    "Need",
    "Oblivion",
    "OldMart",
    "Operator",
    "Ponderer",
    "Quartz",
    "Razorbloom",
    "RedMart",
    "Scrapmaw",
    "ShadowBaby",
    "ShadowGuardian",
    "Sigil",
    "Skinwalker",
    "Slicer",
    "Springer",
    "Telefragger",
    "VBOperator",
    "VoidboundICBM",
    "Voidbreaker",
    "Worm"
}
table.sort(canonicalEnemyNames, function(a, b)
    return string.lower(a) < string.lower(b)
end)

-- ===================================================================
-- 2.1 CONFIG PERSISTENCE ENGINE (Auto-Save, Load, Reset)
-- ===================================================================
local UIControls = {}
local isAutoLoading = false
local saveDebounceTimer = nil

local function getGameConfigPath()
    return "Aether/Configs/Nullscape.json"
end

local function ensureAetherFolder()
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

local function saveGameConfig()
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if not hasFS then return false end

    local ok, _ = pcall(function()
        ensureAetherFolder()
        local filePath = getGameConfigPath()

        local dataToSave = {
            Settings = {},
            Elements = {}
        }

        for k, v in pairs(Config) do
            if type(v) == "boolean" or type(v) == "number" or type(v) == "string" or type(v) == "table" then
                dataToSave.Settings[k] = v
                dataToSave.Elements[k] = v
            end
        end

        for controlKey, controlObj in pairs(UIControls) do
            if controlObj and typeof(controlObj.GetValue) == "function" then
                local s, val = pcall(controlObj.GetValue, controlObj)
                if s and val ~= nil then
                    dataToSave.Elements[controlKey] = val
                end
            end
        end

        local encoded = HttpService:JSONEncode(dataToSave)
        writefile(filePath, encoded)
    end)

    return ok
end

local function debouncedSaveGameConfig()
    if isAutoLoading then return end
    if Config.AutoSaveConfig == false then return end
    if saveDebounceTimer then
        task.cancel(saveDebounceTimer)
        saveDebounceTimer = nil
    end
    saveDebounceTimer = task.delay(0.4, function()
        saveGameConfig()
        saveDebounceTimer = nil
    end)
end

local updateCharacterVoidScript = nil

local function loadGameConfig()
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if not hasFS then return false end

    local filePath = getGameConfigPath()
    local fileExists = false
    pcall(function()
        if type(isfile) == "function" and isfile(filePath) then
            fileExists = true
        end
    end)

    if not fileExists then
        saveGameConfig()
        return false
    end

    local content = nil
    pcall(function() content = readfile(filePath) end)
    if not content or content == "" then return false end

    local s, decoded = pcall(function() return HttpService:JSONDecode(content) end)
    if not s or type(decoded) ~= "table" then return false end

    local settings = decoded.Settings or decoded.Elements or decoded
    if type(settings) ~= "table" then return false end

    isAutoLoading = true

    -- 1. Apply to Config table
    for k, v in pairs(Config) do
        if settings[k] ~= nil then
            if type(v) == "table" and type(settings[k]) == "table" then
                table.clear(Config[k])
                for subK, subV in pairs(settings[k]) do
                    Config[k][subK] = subV
                end
            elseif type(v) == type(settings[k]) then
                Config[k] = settings[k]
            end
        end
    end

    -- Normalize Config.BlacklistedStatues into boolean map { [name] = true }
    if type(Config.BlacklistedStatues) == "table" then
        local normalized = {}
        for k, v in pairs(Config.BlacklistedStatues) do
            if type(k) == "string" and (v == true or v == 1) then
                normalized[k] = true
            elseif type(v) == "string" then
                normalized[v] = true
            end
        end
        table.clear(Config.BlacklistedStatues)
        for k, v in pairs(normalized) do
            Config.BlacklistedStatues[k] = v
        end
    end

    -- Normalize Config.SelectedEnemiesToDelete into boolean map { [name] = true }
    if type(Config.SelectedEnemiesToDelete) == "table" then
        local normalized = {}
        for k, v in pairs(Config.SelectedEnemiesToDelete) do
            if type(k) == "string" and (v == true or v == 1) then
                normalized[k] = true
            elseif type(v) == "string" then
                normalized[v] = true
            end
        end
        table.clear(Config.SelectedEnemiesToDelete)
        for k, v in pairs(normalized) do
            Config.SelectedEnemiesToDelete[k] = v
        end
    end

    -- Normalize Config.SelectedImmunityEnemies into boolean map { [name] = true }
    if type(Config.SelectedImmunityEnemies) == "table" then
        local normalized = {}
        for k, v in pairs(Config.SelectedImmunityEnemies) do
            if type(k) == "string" and (v == true or v == 1) then
                normalized[k] = true
            elseif type(v) == "string" then
                normalized[v] = true
            end
        end
        table.clear(Config.SelectedImmunityEnemies)
        for k, v in pairs(normalized) do
            Config.SelectedImmunityEnemies[k] = v
        end
    end

    -- 2. Apply to UI controls
    for controlKey, controlObj in pairs(UIControls) do
        if controlKey ~= "BlacklistedStatues" and controlKey ~= "SelectedEnemiesToDelete" and controlKey ~= "SelectedImmunityEnemies" then
            local savedVal = settings[controlKey]
            if savedVal ~= nil and controlObj and typeof(controlObj.SetValue) == "function" then
                pcall(function()
                    controlObj:SetValue(savedVal, true)
                end)
            end
        end
    end

    -- 3. Synchronize core feature controls
    if UIControls.PylonESP and typeof(UIControls.PylonESP.SetValue) == "function" then
        pcall(function() UIControls.PylonESP:SetValue(Config.PylonESP, true) end)
    end
    if UIControls.PylonShowCompleted and typeof(UIControls.PylonShowCompleted.SetValue) == "function" then
        pcall(function() UIControls.PylonShowCompleted:SetValue(Config.PylonShowCompleted, true) end)
    end
    if Config.CollectRadius then
        Config.CollectRadius = math.clamp(Config.CollectRadius, 15, 45)
    end
    if UIControls.EnemyDamageImmunity and typeof(UIControls.EnemyDamageImmunity.SetValue) == "function" then
        pcall(function() UIControls.EnemyDamageImmunity:SetValue(Config.EnemyDamageImmunity, true) end)
    end
    if UIControls.GoldGatedBeacon and typeof(UIControls.GoldGatedBeacon.SetValue) == "function" then
        pcall(function() UIControls.GoldGatedBeacon:SetValue(Config.GoldGatedBeacon, true) end)
    end

    -- 4. Synchronize individual upgrades
    if Config.ActiveUpgrades then
        for upgName, isAct in pairs(Config.ActiveUpgrades) do
            local ctrl = UIControls["Upgrade_" .. upgName]
            if ctrl and typeof(ctrl.SetValue) == "function" then
                pcall(function() ctrl:SetValue(isAct, true) end)
            end
        end
    end

    -- 5. Synchronize BlacklistedStatues dropdown specifically
    if UIControls.BlacklistedStatues and typeof(UIControls.BlacklistedStatues.SetValue) == "function" then
        local blList = {}
        for name, bl in pairs(Config.BlacklistedStatues) do
            if bl then table.insert(blList, name) end
        end
        pcall(function() UIControls.BlacklistedStatues:SetValue(blList, true) end)
    end

    -- 5B. Synchronize SelectedImmunityEnemies dropdown specifically
    if UIControls.SelectedImmunityEnemies and typeof(UIControls.SelectedImmunityEnemies.SetValue) == "function" then
        local immList = {}
        for name, imm in pairs(Config.SelectedImmunityEnemies) do
            if imm then table.insert(immList, name) end
        end
        table.sort(immList, function(a, b) return string.lower(a) < string.lower(b) end)
        pcall(function() UIControls.SelectedImmunityEnemies:SetValue(immList, true) end)
    end

    -- 5C. Synchronize SelectedEnemiesToDelete dropdown specifically
    if UIControls.SelectedEnemiesToDelete and typeof(UIControls.SelectedEnemiesToDelete.SetValue) == "function" then
        local delList = {}
        for name, del in pairs(Config.SelectedEnemiesToDelete) do
            if del then table.insert(delList, name) end
        end
        table.sort(delList, function(a, b) return string.lower(a) < string.lower(b) end)
        pcall(function() UIControls.SelectedEnemiesToDelete:SetValue(delList, true) end)
    end

    -- 6. Refresh ESP Visuals, Shields & Pacify Enemies
    pcall(refreshAllStatueESP)
    pcall(refreshAllPylonESP)
    pcall(refreshEnemyHighlights)
    pcall(updateBeaconTouchState)
    if typeof(updateCharacterVoidScript) == "function" then
        pcall(updateCharacterVoidScript)
    end
    pcall(pacifyAllEnemies)
    for hl in pairs(ActiveHighlights) do
        if hl and hl.Parent then
            hl.Enabled = Config.PresentsESP
            hl.FillTransparency = Config.FillTransparency
            hl.OutlineTransparency = Config.OutlineTransparency
        end
    end

    task.delay(0.35, function()
        isAutoLoading = false
    end)

    return true
end

local function resetGameConfig()
    isAutoLoading = true

    for k, v in pairs(DefaultConfig) do
        if type(v) == "table" then
            Config[k] = {}
            for subK, subV in pairs(v) do
                Config[k][subK] = subV
            end
        else
            Config[k] = v
        end
    end

    for controlKey, controlObj in pairs(UIControls) do
        local defVal = DefaultConfig[controlKey]
        if defVal ~= nil and controlObj and typeof(controlObj.SetValue) == "function" then
            pcall(function()
                controlObj:SetValue(defVal, true)
            end)
        end
    end

    if UIControls.PylonESP and typeof(UIControls.PylonESP.SetValue) == "function" then
        pcall(function() UIControls.PylonESP:SetValue(DefaultConfig.PylonESP, true) end)
    end
    if UIControls.PylonShowCompleted and typeof(UIControls.PylonShowCompleted.SetValue) == "function" then
        pcall(function() UIControls.PylonShowCompleted:SetValue(DefaultConfig.PylonShowCompleted, true) end)
    end
    if UIControls.AutoDispelWitherrot and typeof(UIControls.AutoDispelWitherrot.SetValue) == "function" then
        pcall(function() UIControls.AutoDispelWitherrot:SetValue(DefaultConfig.AutoDispelWitherrot, true) end)
    end
    if UIControls.GoldGatedBeacon and typeof(UIControls.GoldGatedBeacon.SetValue) == "function" then
        pcall(function() UIControls.GoldGatedBeacon:SetValue(DefaultConfig.GoldGatedBeacon, true) end)
    end

    -- Reset individual upgrades
    for upgName, isAct in pairs(DefaultConfig.ActiveUpgrades) do
        local ctrl = UIControls["Upgrade_" .. upgName]
        if ctrl and typeof(ctrl.SetValue) == "function" then
            pcall(function() ctrl:SetValue(isAct, true) end)
        end
    end

    -- Reset Blacklist
    if UIControls.BlacklistedStatues and typeof(UIControls.BlacklistedStatues.SetValue) == "function" then
        pcall(function() UIControls.BlacklistedStatues:SetValue({ "Chaos", "Curses" }, true) end)
    end

    -- Reset SelectedImmunityEnemies
    if UIControls.SelectedImmunityEnemies and typeof(UIControls.SelectedImmunityEnemies.SetValue) == "function" then
        pcall(function() UIControls.SelectedImmunityEnemies:SetValue(canonicalEnemyNames, true) end)
    end

    -- Reset SelectedEnemiesToDelete
    if UIControls.SelectedEnemiesToDelete and typeof(UIControls.SelectedEnemiesToDelete.SetValue) == "function" then
        pcall(function() UIControls.SelectedEnemiesToDelete:SetValue({}, true) end)
    end

    pcall(refreshAllStatueESP)
    pcall(refreshAllPylonESP)
    pcall(refreshEnemyHighlights)
    if typeof(updateCharacterVoidScript) == "function" then
        pcall(updateCharacterVoidScript)
    end
    for hl in pairs(ActiveHighlights) do
        if hl and hl.Parent then
            hl.Enabled = DefaultConfig.PresentsESP
            hl.FillTransparency = DefaultConfig.FillTransparency
            hl.OutlineTransparency = DefaultConfig.OutlineTransparency
        end
    end

    task.delay(0.35, function()
        isAutoLoading = false
        saveGameConfig()
    end)
end

-- Pre-load initial settings before UI window and components are created
pcall(function()
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if not hasFS then return end
    local filePath = getGameConfigPath()
    local fileExists = false
    pcall(function()
        if type(isfile) == "function" and isfile(filePath) then
            fileExists = true
        end
    end)
    if not fileExists then return end

    local content = nil
    pcall(function() content = readfile(filePath) end)
    if not content or content == "" then return end

    local s, decoded = pcall(function() return HttpService:JSONDecode(content) end)
    if not s or type(decoded) ~= "table" then return end

    local settings = decoded.Settings or decoded.Elements or decoded
    if type(settings) ~= "table" then return end

    for k, v in pairs(Config) do
        if settings[k] ~= nil then
            if type(v) == "table" and type(settings[k]) == "table" then
                table.clear(Config[k])
                for subK, subV in pairs(settings[k]) do
                    Config[k][subK] = subV
                end
            elseif type(v) == type(settings[k]) then
                Config[k] = settings[k]
            end
        end
    end
end)

-- ===================================================================
-- 3. GAME REFERENCE RESOLUTION & MODULE LOADING
-- ===================================================================
local GiftClientHandler = nil
pcall(function()
    GiftClientHandler = require(ReplicatedFirst.ClientModules.GiftClient.GiftClientHandler)
end)

local GiftCounters = ReplicatedStorage:FindFirstChild("GiftCounters")
local InRound = ReplicatedStorage:FindFirstChild("InRound")
local LevelVal = ReplicatedStorage:FindFirstChild("Level")
local GoldenGiftsVal = ReplicatedStorage:FindFirstChild("GoldenGifts")

local ItemPools = Workspace:FindFirstChild("Item_Pools")
local EnemiesFolder = Workspace:FindFirstChild("Enemies")

-- Movement Modules
local chargerMod, gliderMod, diverMod, grapplerMod, spiritMod = nil, nil, nil, nil, nil
pcall(function() chargerMod = require(ReplicatedStorage.Movement.Specials.Charger) end)
pcall(function() gliderMod = require(ReplicatedStorage.Movement.Specials.Glider) end)
pcall(function() diverMod = require(ReplicatedStorage.Movement.Specials.Dive) end)
pcall(function() grapplerMod = require(ReplicatedStorage.Movement.Specials.Grapple) end)
pcall(function() spiritMod = require(ReplicatedStorage.Movement.Specials.Spirit) end)

local MovementCore = nil
pcall(function()
    local mv = ReplicatedStorage:FindFirstChild("Movement")
    if mv and mv:FindFirstChild("MovementCore") then
        MovementCore = require(mv.MovementCore)
    end
end)

-- Inventory & Upgrades Modules
local inventoryHandler = nil
pcall(function()
    inventoryHandler = require(ReplicatedFirst.ClientModules.Core.PlayerData.InventoryHandler)
end)

local upgradeHandler = nil
pcall(function()
    upgradeHandler = require(ReplicatedFirst.ClientModules.UpgradeHandler)
end)

-- Threat Modules
local TripmineMod = nil
pcall(function()
    local gc = ReplicatedFirst.ClientModules:FindFirstChild("GiftClient")
    if gc then
        TripmineMod = require(gc.GiftClientHandler.Gifts.TripmineClient)
    end
end)

local ProjectilesMod = nil
pcall(function()
    local pc = ReplicatedFirst.ClientModules:FindFirstChild("ProjectileClient")
    if pc then
        ProjectilesMod = require(pc.ProjectileClientHandler)
    end
end)

-- Canonical Altar Colors
local CanonicalAltarColors = {
    Protection   = Color3.fromRGB(0, 255, 255),
    Wishes       = Color3.fromRGB(255, 255, 255),
    Purification = Color3.fromRGB(255, 0, 255),
    Echo         = Color3.fromRGB(128, 85, 255),
    Chaos        = Color3.fromRGB(180, 20, 20),
    Delusion     = Color3.fromRGB(0, 255, 255),
    Reroll       = Color3.fromRGB(50, 180, 255),
    Perfection   = Color3.fromRGB(255, 255, 255),
    Chance       = Color3.fromRGB(255, 255, 180),
    Passage      = Color3.fromRGB(180, 0, 0),
    Purgatory    = Color3.fromRGB(255, 130, 0),
    Curses       = Color3.fromRGB(160, 20, 20),
    Duality      = Color3.fromRGB(18, 238, 212)
}
pcall(function()
    if ReplicatedStorage:FindFirstChild("Altars") and ReplicatedStorage.Altars:FindFirstChild("AltarColors") then
        local mod = require(ReplicatedStorage.Altars.AltarColors)
        for k, col in pairs(mod) do
            if typeof(col) == "Color3" then
                CanonicalAltarColors[k] = col
            end
        end
    end
end)

local VoteRemote = nil
pcall(function()
    if ReplicatedStorage:FindFirstChild("Events") then
        VoteRemote = ReplicatedStorage.Events:FindFirstChild("Vote")
    end
end)

-- ===================================================================
-- 4. FUNCTION HOOKS ENGINE
-- ===================================================================
local OrigHooks = {}
local function safeHook(targetTable, methodName, key, hookFn)
    if not targetTable or typeof(targetTable[methodName]) ~= "function" then return end
    if typeof(hookfunction) ~= "function" then return end
    local orig = targetTable[methodName]
    local old = hookfunction(orig, hookFn)
    OrigHooks[key or methodName] = { target = targetTable, method = methodName, orig = old or orig }
end

-- 1. Glider Infinite Charge
if gliderMod and typeof(gliderMod.WallCheck) == "function" then
    safeHook(gliderMod, "WallCheck", "glider_WallCheck", function(self, ...)
        if Config.GliderInfCharge and self.TimeClamp then
            self.Time = self.TimeClamp
        end
        local h = OrigHooks["glider_WallCheck"]
        if h and h.orig then return h.orig(self, ...) end
    end)
end

-- 2. Grappler Infinite Charge & Zero Cooldown
if grapplerMod and typeof(grapplerMod.Step) == "function" then
    safeHook(grapplerMod, "Step", "grappler_Step", function(self, ...)
        if Config.GrapplerInfCharge and self.TimeClamp then
            self.Time = self.TimeClamp
        end
        if Config.GrapplerNoCD then
            self.reelCooldown = false
            self.Charges = 3
        end
        local h = OrigHooks["grappler_Step"]
        if h and h.orig then return h.orig(self, ...) end
    end)
end

-- 3. Grappler Extended Range (Environment Hook)
local oldGrapplerEnv = nil
if grapplerMod and typeof(grapplerMod.Cast) == "function" and typeof(getfenv) == "function" and typeof(setfenv) == "function" then
    pcall(function()
        oldGrapplerEnv = getfenv(grapplerMod.Cast)
        local fakeWorkspace = setmetatable({}, {
            __index = function(self, index)
                if index == "Raycast" then
                    return function(_, origin, direction, parameters)
                        local mult = (Config.GrappleRange and Config.GrappleRange > 90) and (Config.GrappleRange / 90) or 1
                        return Workspace:Raycast(origin, direction * mult, parameters)
                    end
                end
                return Workspace[index]
            end
        })
        local newenv = setmetatable({}, {
            __index = function(self, index)
                if index == "workspace" then
                    return fakeWorkspace
                end
                return oldGrapplerEnv[index]
            end
        })
        setfenv(grapplerMod.Cast, newenv)
    end)
end

-- 4. Spirit Infinite Stamina
if spiritMod and typeof(spiritMod.Step) == "function" then
    safeHook(spiritMod, "Step", "spirit_Step", function(self, ...)
        if Config.SpiritInfCharge and self.maxTime then
            self.stamina = self.maxTime
        end
        local h = OrigHooks["spirit_Step"]
        if h and h.orig then return h.orig(self, ...) end
    end)
end

-- 5. Charger Zero Cooldown
if chargerMod and typeof(chargerMod.End) == "function" then
    safeHook(chargerMod, "End", "charger_End", function(self, ...)
        local h = OrigHooks["charger_End"]
        local result = h and h.orig and h.orig(self, ...)
        if Config.ChargerNoCD then
            self.debounce = false
        end
        return result
    end)
end

-- 6. Diver Zero Cooldown & Free Camera Steering
if diverMod and typeof(diverMod.Step) == "function" then
    safeHook(diverMod, "Step", "diver_Step", function(data, ...)
        if Config.DiverNoCD and data and data.Movement then
            data.Movement.slideJumped = false
        end
        if Config.DiverFreeMovement and data and data.RootPart and data.Movement and data.Movement.Camera then
            local diveVelocity = data.RootPart:FindFirstChild("DiveVelocity")
            if diveVelocity then
                local direction = data.Movement.Camera.CFrame.LookVector * Vector3.new(1, 0, 1)
                if direction.Magnitude > 0.001 then
                    direction = direction.Unit
                    data.Movement:SetMoveDirection(direction)
                    local speed = (data.RootPart.Velocity * Vector3.new(1, 0, 1)).Magnitude
                    diveVelocity.Velocity = direction * speed
                    if data.Animator and data.Animator.FaceMoveDirection then
                        data.Animator:FaceMoveDirection()
                    end
                end
            end
        end
        local h = OrigHooks["diver_Step"]
        if h and h.orig then return h.orig(data, ...) end
    end)
end

-- 7. Tripmine Neutralizer / Disabler
if TripmineMod and typeof(TripmineMod.Collect) == "function" then
    safeHook(TripmineMod, "Collect", "tripmine_Collect", function(...)
        if Config.TripmineDisabler then
            return nil
        end
        local h = OrigHooks["tripmine_Collect"]
        if h and h.orig then return h.orig(...) end
    end)
end

-- 8. Hostile Projectile Disabler
if ProjectilesMod and typeof(ProjectilesMod.new) == "function" then
    safeHook(ProjectilesMod, "new", "projectiles_New", function(...)
        if Config.ProjectileDisabler then
            return nil
        end
        local h = OrigHooks["projectiles_New"]
        if h and h.orig then return h.orig(...) end
    end)
end

-- 9. Client Upgrade Spoofer
if upgradeHandler and typeof(upgradeHandler.IsUpgradeEnabled) == "function" then
    safeHook(upgradeHandler, "IsUpgradeEnabled", "upgrade_IsUpgradeEnabled", function(name)
        if Config.SpoofUpgrades and name and Config.ActiveUpgrades[name] then
            return true
        end
        local h = OrigHooks["upgrade_IsUpgradeEnabled"]
        if h and h.orig then return h.orig(name) end
        return false
    end)
end

if upgradeHandler and typeof(upgradeHandler.GetUpgradeStack) == "function" then
    safeHook(upgradeHandler, "GetUpgradeStack", "upgrade_GetUpgradeStack", function(name)
        if Config.SpoofUpgrades and name and Config.ActiveUpgrades[name] then
            return Config.UpgradeStacks[name] or Config.DefaultUpgradeStack or 1
        end
        local h = OrigHooks["upgrade_GetUpgradeStack"]
        if h and h.orig then return h.orig(name) end
        return 0
    end)
end

-- 10. MovementCore Speed Multiplier Hook
if MovementCore and typeof(MovementCore.UpdateSpeed) == "function" then
    safeHook(MovementCore, "UpdateSpeed", "movement_UpdateSpeed", function(self, ...)
        local h = OrigHooks["movement_UpdateSpeed"]
        if h and h.orig then h.orig(self, ...) end
        if Config.SpeedMultiplier and Config.SpeedMultiplier > 1 then
            self.movementSpeedMulti = (self.movementSpeedMulti or 1) * Config.SpeedMultiplier
        end
    end)
end

-- 11. MovementCore DiveBonk Nullifier (No Wall Stun)
if MovementCore and typeof(MovementCore.DiveBonk) == "function" then
    safeHook(MovementCore, "DiveBonk", "movement_DiveBonk", function(self, ...)
        if Config.NoDiveBonk then
            return
        end
        local h = OrigHooks["movement_DiveBonk"]
        if h and h.orig then return h.orig(self, ...) end
    end)
end

-- 12. MovementCore Curse Bypass Hook
if MovementCore and typeof(MovementCore.HasCurse) == "function" then
    safeHook(MovementCore, "HasCurse", "movement_HasCurse", function(self, curseName, ...)
        if Config.BypassMovementCurses then
            return false, 0
        end
        local h = OrigHooks["movement_HasCurse"]
        if h and h.orig then return h.orig(self, curseName, ...) end
    end)
end

-- ===================================================================
-- 4B. SERVER-CONFIRMED PRESENT TRACKER ENGINE
-- ===================================================================
local GiftTracker = {
    Pending = {},       -- [giftId] = os.clock() (time request was sent to server)
    Failed = {},        -- [giftId] = { time = os.clock(), dist = number, failCount = number }
    Confirmed = {},     -- [giftId] = true (verified accepted by server)
    Timeout = 0.45,     -- RTT latency window before un-rejected gift is considered confirmed
    MaxRange = 45.0     -- Hard maximum collection distance accepted by game server
}

function GiftTracker:IsConfirmed(gift)
    if not gift or not gift.Collected then return false end
    local id = gift.Id
    if not id then return gift.Collected end

    local fail = self.Failed[id]
    if fail and (os.clock() - fail.time < 2.0) then
        return false
    end

    if self.Confirmed[id] then
        return true
    end

    local reqTime = self.Pending[id]
    if reqTime then
        if os.clock() - reqTime >= self.Timeout then
            self.Confirmed[id] = true
            self.Pending[id] = nil
            return true
        end
        return false
    end

    return true
end

function GiftTracker:HasPending()
    local now = os.clock()
    for id, reqTime in pairs(self.Pending) do
        if now - reqTime < self.Timeout then
            return true
        else
            self.Pending[id] = nil
            self.Confirmed[id] = true
        end
    end
    return false
end

function GiftTracker:CanAttempt(gift, myPos, maxRadius)
    if not gift or gift.Collected or gift.Destroyed or not gift.Position then
        return false
    end

    local dist = (gift.Position - myPos).Magnitude
    if dist > maxRadius or dist > self.MaxRange then
        return false
    end

    local id = gift.Id
    local fail = id and self.Failed[id]
    if fail then
        local elapsed = os.clock() - fail.time
        if elapsed < 1.5 then
            if dist >= (fail.dist - 8.0) then
                return false
            end
        end
    end

    return true
end

-- Hook GiftClientHandler.RemoveCollection to catch server rejections (CollectionFails) immediately
if GiftClientHandler and typeof(GiftClientHandler.RemoveCollection) == "function" then
    safeHook(GiftClientHandler, "RemoveCollection", "gift_RemoveCollection", function(p20)
        if p20 and p20.Id then
            local id = p20.Id
            local now = os.clock()
            local char = LocalPlayer.Character
            local hrp = char and char:FindFirstChild("HumanoidRootPart")
            local dist = (hrp and p20.Position) and (p20.Position - hrp.Position).Magnitude or 50

            GiftTracker.Pending[id] = nil
            GiftTracker.Confirmed[id] = nil

            local existing = GiftTracker.Failed[id]
            GiftTracker.Failed[id] = {
                time = now,
                dist = dist,
                failCount = (existing and existing.failCount or 0) + 1
            }
        end
        local h = OrigHooks["gift_RemoveCollection"]
        if h and h.orig then return h.orig(p20) end
    end)
end

-- ===================================================================
-- 12B. ANTI-VOID & BEACON SAFETY SYSTEM
-- ===================================================================
local LastAntiVoidTeleport = 0

local function GetBeaconCFrame()
    -- Offset 22 studs South of the Beacon center (Z + 22) onto the spawn platform, well outside the 7-stud Beacon touch radius!
    local targetPos = Vector3.new(0, 49.0, 22)
    local beacon = Workspace:FindFirstChild("Beacon")
    if beacon then
        local bPos = beacon:IsA("BasePart") and beacon.Position or beacon:GetPivot().Position
        local rayParams = RaycastParams.new()
        rayParams.FilterType = Enum.RaycastFilterType.Exclude
        if LocalPlayer.Character then
            rayParams.FilterDescendantsInstances = { LocalPlayer.Character, beacon }
        end
        local ray = Workspace:Raycast(Vector3.new(bPos.X, 100, bPos.Z + 22), Vector3.new(0, -200, 0), rayParams)
        if ray and ray.Position and ray.Position.Y > 0 then
            targetPos = ray.Position + Vector3.new(0, 3.0, 0)
        else
            targetPos = Vector3.new(bPos.X, 49.0, bPos.Z + 22)
        end
    end
    -- Face towards the Beacon from safe distance
    local beaconPos = (beacon and beacon:IsA("BasePart") and beacon.Position) or Vector3.new(0, 50, 0)
    return CFrame.lookAt(targetPos, Vector3.new(beaconPos.X, targetPos.Y, beaconPos.Z))
end

local function TeleportToBeaconSafety(sourceReason)
    local now = os.clock()
    if now - LastAntiVoidTeleport < 1.0 then return false end

    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    local humanoid = char and char:FindFirstChild("Humanoid")
    local isDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
    if not hrp or not hrp.Parent or not humanoid or humanoid.Health <= 0 or isDead then
        return false
    end

    LastAntiVoidTeleport = now
    IsSweeping = false
    local safeCFrame = GetBeaconCFrame()

    pcall(function()
        hrp.CFrame = safeCFrame
        hrp.AssemblyLinearVelocity = Vector3.zero
        hrp.AssemblyAngularVelocity = Vector3.zero
    end)

    if MovementCore and MovementCore.ActiveSpecial and typeof(MovementCore.ActiveSpecial.End) == "function" then
        pcall(function() MovementCore.ActiveSpecial:End() end)
    end

    if Window then
        pcall(function()
            Window:Notify({
                Title = "Anti-Void Activated!",
                Desc = string.format("Rescued from void (%s) and returned to Beacon!", sourceReason or "Fall detected"),
                Time = 3.5
            })
        end)
    end
    return true
end

-- 12C. Character Void Death Script Manager (Syncs native void death script with AntiVoid setting)
updateCharacterVoidScript = function(char)
    char = char or LocalPlayer.Character
    if not char then return end
    local vScript = char:FindFirstChild("Void")
    if vScript and vScript:IsA("LocalScript") then
        pcall(function()
            vScript.Enabled = not Config.AntiVoid
        end)
    else
        task.spawn(function()
            local vs = char:WaitForChild("Void", 3)
            if vs and vs:IsA("LocalScript") then
                pcall(function()
                    vs.Enabled = not Config.AntiVoid
                end)
            end
        end)
    end
end
if LocalPlayer.Character then updateCharacterVoidScript(LocalPlayer.Character) end
trackConn(LocalPlayer.CharacterAdded:Connect(updateCharacterVoidScript))

-- 12D. AUTOMATIC SERVER-CONFIRMED BEACON ENTRY (Lock Beacon touch until presents are confirmed collected)
local lastBeaconNotifiedLevel = -1

local function GetRemainingGoldGiftsCount()
    local count = 0
    if GiftClientHandler and GiftClientHandler.Gifts then
        for _, g in pairs(GiftClientHandler.Gifts) do
            if not g.Destroyed and g.Position then
                local isGold = (g.Golden or g.Type == "GoldenGift" or g.Type == "PassageGift")
                if isGold then
                    if not GiftTracker:IsConfirmed(g) then
                        count = count + 1
                    end
                end
            end
        end
    end
    return count
end

local function GetRemainingRegularGiftsCount()
    local count = 0
    if GiftClientHandler and GiftClientHandler.Gifts then
        for _, g in pairs(GiftClientHandler.Gifts) do
            if not g.Destroyed and g.Position then
                local isRegular = not (g.Golden or g.Type == "GoldenGift" or g.Type == "PassageGift" or g.Tripmine or g.Type == "Tripmine")
                if isRegular then
                    if not GiftTracker:IsConfirmed(g) then
                        count = count + 1
                    end
                end
            end
        end
    end
    return count
end

local function GetTotalGoldGiftsCountOnMap()
    local count = 0
    if GiftClientHandler and GiftClientHandler.Gifts then
        for _, g in pairs(GiftClientHandler.Gifts) do
            if not g.Destroyed and g.Position then
                local isGold = (g.Golden or g.Type == "GoldenGift" or g.Type == "PassageGift")
                if isGold then count = count + 1 end
            end
        end
    end
    return count
end

local function updateBeaconTouchState()
    local beacon = Workspace:FindFirstChild("Beacon")
    if not beacon then return end

    local isRoundActive = (not InRound) or (InRound.Value == true)
    local currentLevel = (LevelVal and LevelVal.Value) or 1

    local totalGoldOnMap = GetTotalGoldGiftsCountOnMap()
    local goldRemaining = GetRemainingGoldGiftsCount()
    local hasPending = GiftTracker:HasPending()

    local canTouch = true

    if Config.GoldGatedBeacon and isRoundActive then
        if hasPending then
            -- Never touch beacon while collections are pending in-flight
            canTouch = false
        elseif totalGoldOnMap > 0 then
            -- Map has gold presents: strictly gate until every gold present is server-confirmed
            if goldRemaining > 0 then
                canTouch = false
            else
                canTouch = true
            end
        else
            -- Map has NO gold presents (e.g. Round 1 / smaller maps):
            -- Gate on regular presents so player cannot accidentally escape with 0 money!
            local regularRemaining = GetRemainingRegularGiftsCount()
            local gc = GiftCounters and GiftCounters:FindFirstChild("Gift")
            local serverCollected = gc and gc:GetAttribute("Collected") or 0
            local maxGifts = gc and gc:GetAttribute("MaxGifts") or 0

            if maxGifts > 0 and (serverCollected == 0 or regularRemaining > 0) then
                canTouch = false
            else
                canTouch = true
            end
        end
    else
        canTouch = true
    end

    if beacon:IsA("BasePart") then
        if beacon.CanTouch ~= canTouch then
            beacon.CanTouch = canTouch
        end
    end
    for _, ch in ipairs(beacon:GetDescendants()) do
        if ch:IsA("BasePart") and ch.CanTouch ~= canTouch then
            ch.CanTouch = canTouch
        end
    end

    -- Notify when beacon unlocks once requirements are ACTUALLY met and server confirmed
    if Config.GoldGatedBeacon and isRoundActive and canTouch and lastBeaconNotifiedLevel ~= currentLevel then
        local gc = GiftCounters and GiftCounters:FindFirstChild("Gift")
        local collected = gc and gc:GetAttribute("Collected") or 0
        if collected > 0 or totalGoldOnMap > 0 then
            lastBeaconNotifiedLevel = currentLevel
            if Window then
                local descMsg = (totalGoldOnMap > 0)
                    and "All gold presents confirmed collected! You may now exit through the Beacon."
                    or "All presents confirmed collected! You may now exit through the Beacon."
                Window:Notify({
                    Title = "Beacon Open",
                    Desc = descMsg,
                    Time = 4.0
                })
            end
        end
    end
end

task.spawn(function()
    while ScriptRunning do
        pcall(updateBeaconTouchState)
        task.wait(0.25)
    end
end)

-- Forward declarations for Enemy Pacification & IFRAMES spoofing
local ensureClientIFrames, pacifyEnemy, pacifyAllEnemies

-- 13. Fatal Death Packet Blocker (All Enemies, Hazards, VoidImplosion, Oblivion, Seamine, Void)
local enemyDeathReasons = {
    ["Skinwalker"]     = true,
    ["Cadence"]        = true,
    ["Voidbreaker"]    = true,
    ["Guardian"]       = true,
    ["ShadowGuardian"] = true,
    ["Oblivion"]       = true,
    ["ICBM"]           = true,
    ["VoidboundICBM"]  = true,
    ["Scrapmaw"]       = true,
    ["Slicer"]         = true,
    ["Baby"]           = true,
    ["ShadowBaby"]     = true,
    ["Operator"]       = true,
    ["Telefragger"]    = true,
    ["Ponderer"]       = true,
    ["Quartz"]         = true,
    ["Sigil"]          = true,
    ["Flesh"]          = true,
    ["Fleshling"]      = true,
    ["Kolona"]         = true,
    ["Springer"]       = true,
    ["RedMart"]        = true,
    ["Celestial"]      = true,
    ["Worm"]           = true,
    ["DOG"]            = true,
    ["Need"]           = true,
    ["Husk"]           = true,
    ["Mart"]           = true,
    ["OldMart"]        = true,
    ["BeaconImp"]      = true,
    ["Mirage"]         = true,
    ["Razorbloom"]     = true,
    ["VBOperator"]     = true,
    ["Ouroboros"]      = true,
    ["Minion"]         = true,
    ["LaserBullet"]    = true,
    ["Homer"]          = true,
    ["LockIn"]         = true
}

local fatalReasons = {
    ["VoidImplosion"] = true,
    ["Oblivion"]      = true,
    ["Seamine"]       = true,
    ["Hazard"]        = true
}

local function IsImmuneToEnemy(nameOrReason)
    if not Config.EnemyDamageImmunity then return false end
    if not nameOrReason then return true end
    local str = tostring(nameOrReason)
    if not Config.SelectedImmunityEnemies or type(Config.SelectedImmunityEnemies) ~= "table" then
        return true
    end

    -- Direct key match
    if Config.SelectedImmunityEnemies[str] ~= nil then
        return Config.SelectedImmunityEnemies[str] == true
    end

    -- Known aliases & pairings
    local lower = string.lower(str)
    if (str == "Skinwalker" or string.find(lower, "skinwalker")) and Config.SelectedImmunityEnemies["Husk"] ~= nil then
        return Config.SelectedImmunityEnemies["Husk"] == true
    end
    if (str == "Husk" or string.find(lower, "husk")) and Config.SelectedImmunityEnemies["Skinwalker"] ~= nil then
        return Config.SelectedImmunityEnemies["Skinwalker"] == true
    end
    if str == "OldMart" and Config.SelectedImmunityEnemies["Mart"] ~= nil then
        return Config.SelectedImmunityEnemies["Mart"] == true
    end
    if str == "Mart" and Config.SelectedImmunityEnemies["OldMart"] ~= nil then
        return Config.SelectedImmunityEnemies["OldMart"] == true
    end
    if str == "VoidboundICBM" and Config.SelectedImmunityEnemies["ICBM"] ~= nil then
        return Config.SelectedImmunityEnemies["ICBM"] == true
    end
    if str == "ShadowGuardian" and Config.SelectedImmunityEnemies["Guardian"] ~= nil then
        return Config.SelectedImmunityEnemies["Guardian"] == true
    end
    if str == "ShadowBaby" and Config.SelectedImmunityEnemies["Baby"] ~= nil then
        return Config.SelectedImmunityEnemies["Baby"] == true
    end
    if str == "VBOperator" and Config.SelectedImmunityEnemies["Operator"] ~= nil then
        return Config.SelectedImmunityEnemies["Operator"] == true
    end

    -- Substring match against active canonical selections
    for enemyName, selected in pairs(Config.SelectedImmunityEnemies) do
        if selected == true and string.find(lower, string.lower(enemyName)) then
            return true
        end
    end

    -- If this is an enemy death reason or canonical enemy but explicitly not matched/selected above:
    if enemyDeathReasons[str] then
        return false
    end
    for _, cName in ipairs(canonicalEnemyNames) do
        if string.find(lower, string.lower(cName)) then
            return false
        end
    end

    return true
end

local diedRemote = ReplicatedStorage:FindFirstChild("Events") and ReplicatedStorage.Events:FindFirstChild("Died")

-- Hook __namecall to intercept method-style :FireServer calls from game scripts (e.g. Void.client.lua, enemy attacks)
if typeof(hookmetamethod) == "function" and diedRemote then
    local oldNamecall
    oldNamecall = hookmetamethod(game, "__namecall", function(self, ...)
        local method = getnamecallmethod()
        if (method == "FireServer" or method == "fireServer") and (self == diedRemote or (self and self.Name == "Died")) then
            local reason = ...
            local strReason = tostring(reason)
            if strReason == "Void" then
                if Config.AntiVoid then
                    TeleportToBeaconSafety("Void Packet Intercepted")
                    return nil
                end
            end
            if Config.EnemyDamageImmunity then
                -- Block fatal attack packets from enemies/projectiles IF immune to this enemy
                if strReason ~= "The press of a button" and strReason ~= "Void" then
                    if IsImmuneToEnemy(strReason) then
                        return nil
                    end
                end
            end
            if fatalReasons[strReason] then
                if Config.AutoDisableEnemies or Config.ProjectileDisabler then
                    return nil
                end
            end
        end
        if (method == "InvokeServer" or method == "invokeServer") and (self and self.Name == "DiedFunc") then
            if Config.EnemyDamageImmunity then
                local reason = ...
                local strReason = tostring(reason)
                if strReason ~= "The press of a button" and strReason ~= "Void" then
                    if IsImmuneToEnemy(strReason) then
                        return nil
                    end
                end
            end
        end
        if (method == "FireServer" or method == "fireServer") and (self and (self.Name == "ClientEvent" or self.Name == "ShockwaveEvent")) then
            if Config.EnemyDamageImmunity then
                local enemyName = self.Parent and self.Parent.Name
                if IsImmuneToEnemy(enemyName) then
                    local action = ...
                    local strAction = tostring(action):lower()
                    if strAction == "kill" or strAction == "combust" or strAction == "damage" or strAction == "explode" or strAction == "debuff" then
                        return nil
                    end
                end
            end
        end
        return oldNamecall(self, ...)
    end)
    OrigHooks["game_namecall"] = { target = game, method = "__namecall", orig = oldNamecall }
end

-- Fallback safeHook on Died.FireServer closure
if diedRemote and typeof(diedRemote.FireServer) == "function" then
    safeHook(diedRemote, "FireServer", "died_FireServer", function(self, reason, ...)
        local strReason = tostring(reason)
        if strReason == "Void" then
            if Config.AntiVoid then
                TeleportToBeaconSafety("Void Event Intercepted")
                return nil
            end
        end
        if Config.EnemyDamageImmunity then
            if strReason ~= "The press of a button" and strReason ~= "Void" then
                if IsImmuneToEnemy(strReason) then
                    return nil
                end
            end
        end
        if fatalReasons[strReason] then
            if Config.AutoDisableEnemies or Config.ProjectileDisabler then
                return nil
            end
        end
        local h = OrigHooks["died_FireServer"]
        if h and h.orig then return h.orig(self, reason, ...) end
    end)
end

-- Continuous Anti-Void Heartbeat Watchdog
trackConn(RunService.Heartbeat:Connect(function()
    if not ScriptRunning or not Config.AntiVoid then return end
    local now = os.clock()
    if now - LastAntiVoidTeleport < 1.0 then return end

    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    local humanoid = char and char:FindFirstChild("Humanoid")
    local isDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
    if not hrp or not hrp.Parent or not humanoid or humanoid.Health <= 0 or isDead then return end

    local killVoid = Workspace:FindFirstChild("KillVoid")
    local killY = (killVoid and killVoid.Position.Y) or -264.5
    local buffer = Config.AntiVoidBuffer or 140
    local thresholdY = killY + buffer

    local currentY = hrp.Position.Y
    local velY = hrp.AssemblyLinearVelocity.Y

    -- Dynamic Anti-Void Safety Trigger:
    -- 1. Approaching kill plane (currentY <= killY + 70, e.g. Y <= -540.5) regardless of velocity
    -- 2. Falling deep in the void abyss (currentY <= thresholdY with velY < -10, e.g. Y <= -470.5)
    -- NOTE: Dungeon rooms go down to Y = -108.9. Normal room traversal never triggers this.
    if (currentY <= killY + 70) or (currentY <= thresholdY and velY < -10) then
        TeleportToBeaconSafety(string.format("Y=%.1f (KillPlane=%.1f)", currentY, killY))
    end
end))

-- 14. Cadence Aura Spoof (Keeps player permanently recognized as near an instrument)
local cadenceAuraOrb = nil
local function updateCadenceAura()
    if not ScriptRunning then return end
    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    if not hrp then return end

    if not cadenceAuraOrb or not cadenceAuraOrb.Parent then
        cadenceAuraOrb = Instance.new("Part")
        cadenceAuraOrb.Name = "CadenceAuraProtection"
        cadenceAuraOrb.Size = Vector3.new(2, 2, 2)
        cadenceAuraOrb.Transparency = 1
        cadenceAuraOrb.CanCollide = false
        cadenceAuraOrb.CanTouch = true
        cadenceAuraOrb.Anchored = false
        cadenceAuraOrb.CFrame = hrp.CFrame

        local weld = Instance.new("WeldConstraint")
        weld.Part0 = hrp
        weld.Part1 = cadenceAuraOrb
        weld.Parent = cadenceAuraOrb

        cadenceAuraOrb.Parent = char
        CollectionService:AddTag(cadenceAuraOrb, "CadenceOrb")
    end
end

local function setupCharacterDeathListeners(char)
    if not char then return end
    local hum = char:WaitForChild("Humanoid", 4)
    if hum then
        trackConn(hum.Died:Connect(function()
            IsSweeping = false
            SweepStartTime = 0
        end))
    end
end

local function protectPlayerCharacter(char)
    if not char then return end
    pcall(function()
        local hitCache = require(game.ReplicatedFirst.ClientModules.ProjectileClient.ProjectileClientHandler.PlayerHitboxCache)
        if hitCache and hitCache.Hitboxes then
            if Config.EnemyDamageImmunity and IsImmuneToEnemy("ICBM") then
                hitCache.Hitboxes[LocalPlayer] = nil
            else
                local hb = char:FindFirstChild("Hitbox")
                if hb then
                    hitCache.Hitboxes[LocalPlayer] = hb
                end
            end
        end
    end)
end

if LocalPlayer.Character then
    setupCharacterDeathListeners(LocalPlayer.Character)
    task.spawn(function()
        task.wait(0.5)
        pcall(updateCadenceAura)
        pcall(ensureClientIFrames)
        pcall(protectPlayerCharacter, LocalPlayer.Character)
        pcall(pacifyAllEnemies)
    end)
end

trackConn(LocalPlayer.CharacterAdded:Connect(function(newChar)
    IsSweeping = false
    SweepStartTime = 0
    setupCharacterDeathListeners(newChar)
    task.wait(0.8)
    pcall(updateCadenceAura)
    pcall(ensureClientIFrames)
    pcall(protectPlayerCharacter, newChar)
    pcall(pacifyAllEnemies)
end))

local deadVal = LocalPlayer:FindFirstChild("Dead")
if deadVal and deadVal:IsA("BoolValue") then
    trackConn(deadVal.Changed:Connect(function()
        IsSweeping = false
        SweepStartTime = 0
    end))
end

-- ===================================================================
-- 5. CLASS CHANGER & ENTITY REMOVAL HELPERS
-- ===================================================================
local function SetClass(className)
    local rep = inventoryHandler and inventoryHandler.GetReplica and inventoryHandler.GetReplica()
    local ev = ReplicatedStorage:FindFirstChild("RemoteEvents") and ReplicatedStorage.RemoteEvents:FindFirstChild("ReplicaSet")
    if rep and ev and typeof(firesignal) == "function" then
        firesignal(ev.OnClientEvent, rep.Id, { "Class", "Equipped" }, "class/" .. className)
        Config.SelectedClass = className
        return true
    end
    return false
end

local BeneficialWhitelist = {
    ["Cadence"]    = true,  -- Melody Boss / Instrument Spawner (Encore beacon unlock)
    ["CadenceOld"] = true,
    ["StarMart"]   = true,  -- +Speed, +2 Collection Range, 25s Debuff Immunity
    ["Bell"]       = true,  -- Cleanses Flesh, Joined Symphony curse progression
    ["BellOld"]    = true,
    ["Hunk"]       = true,  -- Ouroboros curse survival objective (Collect or Die)
}

-- Client-side IFRAMES spoofing: neutralizes enemy attack routines that inspect LocalPlayer:FindFirstChild("IFRAMES")
ensureClientIFrames = function()
    if not LocalPlayer then return end
    local anyVulnerable = false
    if Config.SelectedImmunityEnemies and type(Config.SelectedImmunityEnemies) == "table" then
        for _, cName in ipairs(canonicalEnemyNames) do
            if Config.SelectedImmunityEnemies[cName] == false then
                anyVulnerable = true
                break
            end
        end
    end

    if Config.EnemyDamageImmunity then
        if not LocalPlayer:FindFirstChild("IFRAMES") then
            local f = Instance.new("Folder")
            f.Name = "IFRAMES"
            f.Parent = LocalPlayer
        end
        if not anyVulnerable then
            if not LocalPlayer:FindFirstChild("TRUEIFRAMES") then
                local f = Instance.new("Folder")
                f.Name = "TRUEIFRAMES"
                f.Parent = LocalPlayer
            end
        else
            local trueIframe = LocalPlayer:FindFirstChild("TRUEIFRAMES")
            if trueIframe and trueIframe:IsA("Folder") then
                pcall(function() trueIframe:Destroy() end)
            end
        end
    else
        local iframe = LocalPlayer:FindFirstChild("IFRAMES")
        if iframe and iframe:IsA("Folder") then
            pcall(function() iframe:Destroy() end)
        end
        local trueIframe = LocalPlayer:FindFirstChild("TRUEIFRAMES")
        if trueIframe and trueIframe:IsA("Folder") then
            pcall(function() trueIframe:Destroy() end)
        end
    end
end

do
    local MIN_PART_SIZE = Vector3.new(0.001, 0.001, 0.001)

    local function isNonCollidableEnemyPart(partName)
        local lower = string.lower(partName)
        return lower == "icbm" 
            or lower == "voidboundicbm" 
            or lower == "indicator"
            or lower == "ride"
            or lower == "collide"
            or lower == "collide2"
            or lower == "demonshockwave"
            or lower == "firearea"
            or string.find(lower, "mark") ~= nil
    end

    local function unpacifyEnemyPart(part)
        if not part or not part:IsA("BasePart") then return end
        local origSize = part:GetAttribute("PacifiedOrigSize")
        if origSize then
            part.Size = origSize
        end
        -- In Nullscape, enemies are NEVER solid platforms.
        -- Only restore CanCollide = true if the original template had CanCollide == true,
        -- AND never for ICBM, marks, indicators, shockwaves, or Ride.
        local origCanCollide = part:GetAttribute("PacifiedOrigCanCollide")
        if origCanCollide == true and not isNonCollidableEnemyPart(part.Name) then
            part.CanCollide = true
        else
            part.CanCollide = false
        end
        local origCanTouch = part:GetAttribute("PacifiedOrigCanTouch")
        if origCanTouch ~= nil then
            part.CanTouch = origCanTouch
        else
            part.CanTouch = true
        end
        part.CanQuery = true
    end

    local function pacifyEnemyPart(part, enemyName)
        if not part or not part:IsA("BasePart") then return end

        if not part:GetAttribute("PacifiedOrigCanCollide") then
            local trueOrigCollide = part.CanCollide
            if isNonCollidableEnemyPart(part.Name) then
                trueOrigCollide = false
            end
            part:SetAttribute("PacifiedOrigCanCollide", trueOrigCollide)
            part:SetAttribute("PacifiedOrigCanTouch", part.CanTouch)
            part:SetAttribute("PacifiedOrigCanQuery", part.CanQuery)
            part:SetAttribute("PacifiedOrigSize", part.Size)
        end

        part.CanCollide = false
        part.CanTouch = false
        part.CanQuery = false

        -- Neutralize Springer invisible barrier & sky catapult:
        -- Ride: shrunk to minimum part size (0.001 studs) so Springer_ClientAI never detects player overlap.
        -- Collide & Collide2: invisible barrier parts; shrunk to 0.001 studs + CanCollide = false.
        -- NOTE: We ONLY set Size once if Magnitude > 0.005 to completely avoid recursive SizeChanged re-entrancy!
        if part.Name == "Ride" or part.Name == "Collide" or part.Name == "Collide2" then
            if part.Size.Magnitude > 0.005 then
                part.Size = MIN_PART_SIZE
            end
        end

        -- Persistent Property Change Listeners:
        -- Springer_ClientAI u35/u54 and other AI scripts re-enable CanCollide / CanTouch on jump/land.
        -- These listeners instantly force them back to false while immune!
        -- Also guards ICBM, marks, and indicators so they NEVER become floating platforms even if game scripts touch CanCollide!
        if not part:GetAttribute("PacifiedPropListener") then
            part:SetAttribute("PacifiedPropListener", true)
            local isSettingProp = false
            trackConn(part:GetPropertyChangedSignal("CanCollide"):Connect(function()
                if isSettingProp then return end
                if Config.EnemyDamageImmunity and IsImmuneToEnemy(enemyName) then
                    if part.CanCollide then
                        isSettingProp = true
                        part.CanCollide = false
                        isSettingProp = false
                    end
                elseif isNonCollidableEnemyPart(part.Name) and part.CanCollide then
                    isSettingProp = true
                    part.CanCollide = false
                    isSettingProp = false
                end
            end))
            trackConn(part:GetPropertyChangedSignal("CanTouch"):Connect(function()
                if isSettingProp then return end
                if Config.EnemyDamageImmunity and IsImmuneToEnemy(enemyName) then
                    if part.CanTouch then
                        isSettingProp = true
                        part.CanTouch = false
                        isSettingProp = false
                    end
                end
            end))
            trackConn(part:GetPropertyChangedSignal("CanQuery"):Connect(function()
                if isSettingProp then return end
                if Config.EnemyDamageImmunity and IsImmuneToEnemy(enemyName) then
                    if part.CanQuery then
                        isSettingProp = true
                        part.CanQuery = false
                        isSettingProp = false
                    end
                end
            end))
        end
    end

    -- Pacify enemy: eliminates physical collisions, invisible barriers, and fatal hitboxes while preserving 100% of visual meshes, animations, SFX, and VFX
    pacifyEnemy = function(enemy)
        if not enemy or not enemy.Parent then return end
        local name = enemy.Name
        if BeneficialWhitelist[name] then return end

        local immune = IsImmuneToEnemy(name)
        if not immune then
            -- Player is VULNERABLE to this enemy: restore its physical collisions & hitboxes so it can touch and harm player!
            pcall(function()
                if enemy:IsA("BasePart") then
                    unpacifyEnemyPart(enemy)
                end
                for _, desc in ipairs(enemy:GetDescendants()) do
                    if desc:IsA("BasePart") then
                        unpacifyEnemyPart(desc)
                    end
                end
            end)
            return
        end

        pcall(function()
            -- 1. Pacify root part
            if enemy:IsA("BasePart") then
                pacifyEnemyPart(enemy, name)
            end

            -- 2. Pacify all existing descendants (hitboxes, ride parts, collision boxes, shockwaves)
            for _, desc in ipairs(enemy:GetDescendants()) do
                if desc:IsA("BasePart") then
                    pacifyEnemyPart(desc, name)
                end
            end

            -- 3. Hook enemy RemoteEvents (ClientEvent, ShockwaveEvent, etc.)
            local function hookEnemyRemote(ev)
                if ev and ev:IsA("RemoteEvent") and typeof(ev.FireServer) == "function" then
                    local key = "EnemyRemote_" .. tostring(enemy.Name) .. "_" .. tostring(ev.Name) .. "_" .. tostring(ev:GetDebugId())
                    if not OrigHooks[key] then
                        safeHook(ev, "FireServer", key, function(s, action, ...)
                            if Config.EnemyDamageImmunity and IsImmuneToEnemy(enemy.Name) then
                                local strAction = tostring(action):lower()
                                if strAction == "kill" or strAction == "combust" or strAction == "damage" or strAction == "explode" or strAction == "debuff" then
                                    return nil
                                end
                            end
                            local h = OrigHooks[key]
                            if h and h.orig then return h.orig(s, action, ...) end
                        end)
                    end
                end
            end

            for _, desc in ipairs(enemy:GetDescendants()) do
                if desc:IsA("RemoteEvent") then hookEnemyRemote(desc) end
            end

            -- 4. Watch for any newly added descendants (e.g. shockwaves, dynamic hitboxes, remotes)
            trackConn(enemy.DescendantAdded:Connect(function(desc)
                if desc:IsA("BasePart") then
                    pacifyEnemyPart(desc, name)
                elseif desc:IsA("RemoteEvent") then
                    hookEnemyRemote(desc)
                end
            end))
        end)
    end
end

pacifyAllEnemies = function()
    ensureClientIFrames()
    protectPlayerCharacter(LocalPlayer.Character)
    local ef = Workspace:FindFirstChild("Enemies")
    if ef then
        for _, ch in ipairs(ef:GetChildren()) do
            pacifyEnemy(ch)
        end
    end
    local sw = Workspace:FindFirstChild("Skinwalkers")
    if sw then
        for _, ch in ipairs(sw:GetChildren()) do
            local isHuskImmune = IsImmuneToEnemy("Husk") or IsImmuneToEnemy("Skinwalker")
            pcall(function()
                if ch.Name == "Part2" then
                    if isHuskImmune then ch:Destroy() end
                elseif ch:IsA("BasePart") then
                    ch.CanTouch = not isHuskImmune
                    ch.CanCollide = not isHuskImmune
                else
                    pacifyEnemy(ch)
                end
            end)
        end
    end
end

local function RemoveEnemy(enemy)
    if not enemy or not enemy.Parent then return end
    local name = enemy.Name

    -- Protect beneficial / objective entities (Cadence, StarMart, Bell, Hunk)
    if BeneficialWhitelist[name] then
        pcall(function() highlightEnemy(enemy) end)
        return
    end

    -- Dropdown selections ONLY delete if AutoDisableEnemies toggle is actively enabled!
    if not Config.AutoDisableEnemies then
        if Config.EnemyDamageImmunity then
            pacifyEnemy(enemy)
        end
        pcall(function() highlightEnemy(enemy) end)
        return
    end

    -- Selective deletion check: only destroy if user explicitly selected this enemy type in dropdown
    local shouldDelete = false
    if Config.SelectedEnemiesToDelete and type(Config.SelectedEnemiesToDelete) == "table" then
        if Config.SelectedEnemiesToDelete[name] == true then
            shouldDelete = true
        elseif (name == "Skinwalker" or string.find(string.lower(name), "skinwalker")) and Config.SelectedEnemiesToDelete["Husk"] == true then
            shouldDelete = true
        elseif (name == "Husk" or string.find(string.lower(name), "husk")) and Config.SelectedEnemiesToDelete["Skinwalker"] == true then
            shouldDelete = true
        elseif name == "OldMart" and Config.SelectedEnemiesToDelete["Mart"] == true then
            shouldDelete = true
        elseif name == "Mart" and Config.SelectedEnemiesToDelete["OldMart"] == true then
            shouldDelete = true
        end
    end

    if shouldDelete then
        pcall(function()
            local ai = enemy:FindFirstChild(string.format("%s_ClientAI", enemy.Name))
            if ai then ai.Disabled = true end
            enemy:Destroy()
        end)
    else
        -- User opted to KEEP this enemy! Pacify it so it cannot harm the player!
        if Config.EnemyDamageImmunity then
            pacifyEnemy(enemy)
        end
        pcall(function() highlightEnemy(enemy) end)
    end
end

-- ===================================================================
-- 5B. HAZARD REMOVAL (SEA MINES & BLACK HOLES / VOID IMPLOSIONS)
-- ===================================================================
local function RemoveSeamine(instance)
    if not instance or not instance.Parent then return end
    pcall(function()
        if instance:IsA("BasePart") then
            instance.CanTouch = false
            instance.CanCollide = false
            instance.CanQuery = false
            instance.Transparency = 1
        end
        for _, desc in ipairs(instance:GetDescendants()) do
            if desc:IsA("BasePart") then
                desc.CanTouch = false
                desc.CanCollide = false
                desc.CanQuery = false
                desc.Transparency = 1
            elseif desc:IsA("Script") or desc:IsA("LocalScript") then
                desc.Disabled = true
            elseif desc:IsA("Sound") then
                desc:Stop()
            end
        end
        instance:Destroy()
    end)
end

local function ClearAllSeamines()
    if not (Config.AutoDisableEnemies or Config.ProjectileDisabler) then return end
    pcall(function()
        for _, s in ipairs(CollectionService:GetTagged("Seamine")) do
            RemoveSeamine(s)
        end
    end)
    pcall(function()
        local jp = Workspace:FindFirstChild("JumpPads")
        if jp then
            for _, d in ipairs(jp:GetDescendants()) do
                if d.Name == "Seamine" or d.Name == "SeamineGoback" or d.Name == "SeamineUNC" or string.find(string.lower(d.Name), "seamine") then
                    RemoveSeamine(d)
                end
            end
        end
    end)
    pcall(function()
        local rooms = Workspace:FindFirstChild("CurrentRooms")
        if rooms then
            for _, d in ipairs(rooms:GetDescendants()) do
                if d.Name == "Seamine" or d.Name == "SeamineGoback" or d.Name == "SeamineUNC" or string.find(string.lower(d.Name), "seamine") then
                    RemoveSeamine(d)
                end
            end
        end
    end)
end

local function RemoveBlackHole(instance)
    if not instance or not instance.Parent then return end
    pcall(function()
        if instance:IsA("BasePart") then
            instance.CanTouch = false
            instance.CanCollide = false
            instance.Transparency = 1
        end
        for _, desc in ipairs(instance:GetDescendants()) do
            if desc:IsA("BasePart") then
                desc.CanTouch = false
                desc.CanCollide = false
                desc.Transparency = 1
            elseif desc:IsA("Sound") then
                desc:Stop()
            elseif desc:IsA("ParticleEmitter") then
                desc.Enabled = false
            end
        end
        instance:Destroy()
    end)
end

local function ClearAllBlackHoles()
    if not (Config.AutoDisableEnemies or Config.ProjectileDisabler) then return end
    pcall(function()
        local destroyFolder = Workspace:FindFirstChild("DestroyFolder")
        if destroyFolder then
            for _, ch in ipairs(destroyFolder:GetChildren()) do
                if ch.Name == "VoidExplosion" or string.find(ch.Name, "Void") or string.find(ch.Name, "Implos") or ch:FindFirstChild("VoidInner") or ch:FindFirstChild("VoidImplode") then
                    RemoveBlackHole(ch)
                end
            end
        end
    end)
    pcall(function()
        local cam = Workspace.CurrentCamera
        if cam then
            for _, ch in ipairs(cam:GetChildren()) do
                if ch.Name == "OblivionAttack" or ch.Name == "OblivionAmbient" or ch.Name == "OblivionSky" then
                    pcall(function() ch:Destroy() end)
                end
            end
        end
    end)
end

-- ===================================================================
-- 6. HIGHLIGHT & VISUALS CONTROLLER (Zero-lag pool adornment engine)
-- ===================================================================
local ActiveHighlights = {}
local EnemyHighlights = {}
local ActiveStatueHighlights = {}
local ActiveStatueBillboards = {}
local ActiveCadenceHighlights = {}
local ActiveCadenceBillboards = {}
local ActiveMedalHighlights = {}
local ActiveMedalBillboards = {}
local ActivePylonHighlights = {}
local ActivePylonBillboards = {}
local KnownMedals = {}
local LastMedalTouch = 0

local function applyHighlight(model, color, name)
    if not model or not model.Parent then return nil end
    local hl = model:FindFirstChild(name)
    if not hl then
        hl = Instance.new("Highlight")
        hl.Name = name
        hl.DepthMode = Enum.HighlightDepthMode.AlwaysOnTop
        hl.Parent = model
    end
    hl.OutlineColor = color
    hl.OutlineTransparency = Config.OutlineTransparency
    hl.FillTransparency = Config.FillTransparency
    hl.FillColor = color
    hl.Enabled = Config.PresentsESP
    ActiveHighlights[hl] = true
    return hl
end

local function refreshAllHighlights()
    if not ItemPools or not ItemPools.Parent then
        ItemPools = Workspace:FindFirstChild("Item_Pools")
    end
    if not ItemPools then return end

    -- 1. Regular Gift Pool
    local giftModel = ItemPools:FindFirstChild("Gift_Model")
    if giftModel then
        applyHighlight(giftModel, Config.ColorGift, "Aether_Gift_HL")
    end

    -- 2. Golden Gift Pool
    local goldModel = ItemPools:FindFirstChild("GoldenGift_Model")
    if goldModel then
        applyHighlight(goldModel, Config.ColorGolden, "Aether_Golden_HL")
    end

    -- 3. Passage Gift Pool
    local passModel = ItemPools:FindFirstChild("PassageGift_Model")
    if passModel then
        applyHighlight(passModel, Config.ColorPassage, "Aether_Passage_HL")
    end

    -- 4. Tripmine Trap Pool
    local tripModel = ItemPools:FindFirstChild("Tripmine_Model")
    if tripModel then
        applyHighlight(tripModel, Config.ColorTripmine, "Aether_Tripmine_HL")
    end
end

local function highlightEnemy(enemyPart)
    if not enemyPart or not enemyPart.Parent or enemyPart:FindFirstChild("Aether_Enemy_HL") then return end
    local name = enemyPart.Name
    local hlColor = Config.ColorEnemy
    if name == "StarMart" then
        hlColor = Color3.fromRGB(255, 215, 0) -- Bright Gold
    elseif name == "Bell" or name == "BellOld" then
        hlColor = Color3.fromRGB(0, 220, 255) -- Cyan
    elseif name == "Hunk" then
        hlColor = Color3.fromRGB(255, 100, 100) -- Coral Red
    elseif name == "Cadence" or name == "CadenceOld" then
        hlColor = Color3.fromRGB(255, 20, 147) -- Neon Pink
    elseif name == "Celestial" or name == "CelestialBoss" then
        hlColor = Color3.fromRGB(255, 30, 160) -- Celestial Hot Pink Boss
    end

    local hl = Instance.new("Highlight")
    hl.Name = "Aether_Enemy_HL"
    hl.OutlineColor = hlColor
    hl.FillColor = hlColor
    hl.OutlineTransparency = Config.OutlineTransparency
    hl.FillTransparency = math.clamp(Config.FillTransparency, 0.4, 0.85)
    hl.DepthMode = Enum.HighlightDepthMode.AlwaysOnTop
    hl.Enabled = Config.EnemyESP
    hl.Parent = enemyPart
    EnemyHighlights[hl] = enemyPart
end

local function refreshEnemyHighlights()
    if not EnemiesFolder or not EnemiesFolder.Parent then
        EnemiesFolder = Workspace:FindFirstChild("Enemies")
    end
    if not EnemiesFolder then return end

    for _, enemy in ipairs(EnemiesFolder:GetChildren()) do
        if Config.AutoDisableEnemies then
            RemoveEnemy(enemy)
        else
            if Config.EnemyDamageImmunity then
                pacifyEnemy(enemy)
            end
            highlightEnemy(enemy)
        end
    end
end

-- ===================================================================
-- 7. STATUE / ALTAR DETECTION & ESP ENGINE
-- ===================================================================
local LastAltarTrigger = {}

local function IsStatueBlacklisted(altarName)
    if not altarName or #altarName == 0 then return false end
    if not Config.BlacklistedStatues then return false end

    -- Direct match if stored as dictionary boolean map { [name] = true }
    if Config.BlacklistedStatues[altarName] == true then
        return true
    end

    local lowerTarget = string.lower(altarName)
    for k, v in pairs(Config.BlacklistedStatues) do
        local checkName = nil
        if type(k) == "string" and (v == true or v == 1) then
            checkName = k
        elseif type(v) == "string" then
            checkName = v
        end

        if checkName and (checkName == altarName or string.find(lowerTarget, string.lower(checkName), 1, true)) then
            return true
        end
    end
    return false
end

local function GetAltarRoomNumber(model)
    local cur = model
    while cur and cur ~= Workspace do
        local n = tonumber(cur.Name)
        if n then return n end
        cur = cur.Parent
    end
    return nil
end

local function GetAltarInfo(model)
    if not model or not model.Parent then return nil end

    -- Exclude models in DestroyFolder (collapsed/destroyed rooms)
    local destroyFolder = Workspace:FindFirstChild("DestroyFolder")
    if (destroyFolder and model:IsDescendantOf(destroyFolder)) or model:FindFirstAncestor("DestroyFolder") then
        return nil
    end

    local killVoid = Workspace:FindFirstChild("KillVoid")
    local killY = (killVoid and killVoid.Position.Y) or -264.5
    local modelPivot = model:GetPivot()
    if modelPivot.Position.Y < (killY + 50) or modelPivot.Position.Y < -50 then
        return nil -- Altar is falling into the void or below map!
    end

    local realName = ""
    local nameVal = model:FindFirstChild("RealName")
    if nameVal and nameVal:IsA("StringValue") and #nameVal.Value > 0 then
        realName = nameVal.Value
    elseif model:GetAttribute("Name") and #tostring(model:GetAttribute("Name")) > 0 then
        realName = tostring(model:GetAttribute("Name"))
    elseif model.Name ~= "AltarModel" and not string.find(string.lower(model.Name), "altar") then
        realName = model.Name
    else
        local p = model:FindFirstChildWhichIsA("ProximityPrompt", true)
        if p and p.ActionText and #p.ActionText > 0 then
            local clean = string.gsub(p.ActionText, "^[Aa]ltar%s+[Oo]f%s+", "")
            clean = string.gsub(clean, "^[Aa]lter%s+[Oo]f%s+", "")
            clean = string.gsub(clean, "^[Aa]ltar%s+", "")
            realName = clean
        end
    end
    if #realName == 0 then realName = "Altar" end

    local prompt = model:FindFirstChildWhichIsA("ProximityPrompt", true)
    local hitbox = model:FindFirstChild("Hitbox") or model:FindFirstChild("HitBox")
    if not hitbox and prompt and prompt.Parent and prompt.Parent:IsA("BasePart") then
        hitbox = prompt.Parent
    end
    if not hitbox then
        hitbox = model.PrimaryPart or model:FindFirstChildWhichIsA("BasePart")
    end

    -- If hitbox exists and is unanchored, the room is actively collapsing / falling!
    if hitbox and not hitbox.Anchored then
        return nil
    end

    local roomNum = GetAltarRoomNumber(model)
    local isActivated = model:GetAttribute("Activated") == true 
        or model:GetAttribute("AltarUsed") == true
        or (prompt and not prompt.Enabled)
    local isEnabled = (prompt and prompt.Enabled and not isActivated)

    local color = CanonicalAltarColors[realName] or Color3.fromRGB(255, 255, 180)

    return {
        Model = model,
        Name = realName,
        Hitbox = hitbox,
        Prompt = prompt,
        RoomNumber = roomNum,
        IsActivated = isActivated,
        IsEnabled = isEnabled,
        Color = color
    }
end

local function GetAllAltars()
    local results = {}
    local seen = {}
    local destroyFolder = Workspace:FindFirstChild("DestroyFolder")

    -- 1. Check CollectionService tagged "Altar"
    pcall(function()
        for _, obj in ipairs(CollectionService:GetTagged("Altar")) do
            local targetModel = obj:IsA("Model") and obj or obj.Parent
            if targetModel and targetModel:IsA("Model") and not seen[targetModel] then
                if not (destroyFolder and targetModel:IsDescendantOf(destroyFolder)) then
                    seen[targetModel] = true
                    local info = GetAltarInfo(targetModel)
                    if info then table.insert(results, info) end
                end
            end
        end
    end)

    -- 2. Check workspace.CurrentRooms hierarchy
    pcall(function()
        local rooms = Workspace:FindFirstChild("CurrentRooms")
        if rooms then
            for _, desc in ipairs(rooms:GetDescendants()) do
                if desc:IsA("Model") and not seen[desc] then
                    if not (destroyFolder and desc:IsDescendantOf(destroyFolder)) then
                        local isAltar = desc.Name == "AltarModel"
                            or desc:FindFirstChild("RealName") ~= nil
                            or desc:GetAttribute("Altar") ~= nil
                            or desc:GetAttribute("ChanceType") ~= nil
                            or string.find(string.lower(desc.Name), "altar") ~= nil
                            or CanonicalAltarColors[desc.Name] ~= nil
                        if isAltar then
                            seen[desc] = true
                            local info = GetAltarInfo(desc)
                            if info then table.insert(results, info) end
                        end
                    end
                end
            end
        end
    end)

    -- 3. Fallback: ProximityPrompts with altar-related text
    pcall(function()
        for _, p in ipairs(CollectionService:GetTagged("Prompt")) do
            if p:IsA("ProximityPrompt") then
                local act = string.lower(p.ActionText or "")
                local obj = string.lower(p.ObjectText or "")
                if string.find(act, "altar") or string.find(act, "alter") or string.find(obj, "altar") or string.find(obj, "alter") then
                    local model = p:FindFirstAncestorWhichIsA("Model")
                    if model and not seen[model] and model ~= Workspace then
                        if not (destroyFolder and model:IsDescendantOf(destroyFolder)) then
                            seen[model] = true
                            local info = GetAltarInfo(model)
                            if info then table.insert(results, info) end
                        end
                    end
                end
            end
        end
    end)

    return results
end

local function applyAltarESP(info)
    if not info or not info.Model or not info.Model.Parent then return end
    local model = info.Model
    local color = info.Color or Color3.fromRGB(255, 255, 180)
    local isBlacklisted = IsStatueBlacklisted(info.Name)

    -- 1. Model Highlight
    local hl = model:FindFirstChild("Aether_Altar_HL")
    if not hl then
        hl = Instance.new("Highlight")
        hl.Name = "Aether_Altar_HL"
        hl.DepthMode = Enum.HighlightDepthMode.AlwaysOnTop
        hl.Parent = model
    end
    hl.OutlineColor = isBlacklisted and Color3.fromRGB(255, 60, 60) or color
    hl.FillColor = isBlacklisted and Color3.fromRGB(150, 40, 40) or color
    hl.OutlineTransparency = Config.OutlineTransparency
    hl.FillTransparency = info.IsActivated and 0.85 or math.clamp(Config.FillTransparency, 0.4, 0.7)
    hl.Enabled = Config.StatueESP
    ActiveStatueHighlights[hl] = model

    -- 2. 3D Floating Billboard Label
    local anchor = info.Hitbox or (info.Prompt and info.Prompt.Parent)
    if anchor and anchor:IsA("BasePart") then
        local bb = anchor:FindFirstChild("Aether_Altar_ESP")
        if not bb then
            bb = Instance.new("BillboardGui")
            bb.Name = "Aether_Altar_ESP"
            bb.Size = UDim2.new(0, 180, 0, 48)
            bb.StudsOffset = Vector3.new(0, 4.5, 0)
            bb.AlwaysOnTop = true
            bb.MaxDistance = 500

            local nameLabel = Instance.new("TextLabel")
            nameLabel.Name = "AltarTitle"
            nameLabel.Size = UDim2.new(1, 0, 0.55, 0)
            nameLabel.Position = UDim2.new(0, 0, 0, 0)
            nameLabel.BackgroundTransparency = 1
            nameLabel.Font = Enum.Font.GothamBold
            nameLabel.TextSize = 14
            nameLabel.TextStrokeTransparency = 0.2
            nameLabel.TextStrokeColor3 = Color3.new(0, 0, 0)
            nameLabel.Parent = bb

            local subLabel = Instance.new("TextLabel")
            subLabel.Name = "AltarSub"
            subLabel.Size = UDim2.new(1, 0, 0.45, 0)
            subLabel.Position = UDim2.new(0, 0, 0.55, 0)
            subLabel.BackgroundTransparency = 1
            subLabel.Font = Enum.Font.GothamMedium
            subLabel.TextSize = 11
            subLabel.TextStrokeTransparency = 0.3
            subLabel.TextStrokeColor3 = Color3.new(0, 0, 0)
            subLabel.Parent = bb

            bb.Parent = anchor
        end

        local title = bb:FindFirstChild("AltarTitle")
        local sub = bb:FindFirstChild("AltarSub")

        if title then
            title.Text = string.format("Altar of %s", info.Name or "Unknown")
            title.TextColor3 = isBlacklisted and Color3.fromRGB(255, 90, 90) or color
        end

        if sub then
            local dist = 0
            local char = LocalPlayer.Character
            local hrp = char and char:FindFirstChild("HumanoidRootPart")
            if hrp then
                dist = math.floor((anchor.Position - hrp.Position).Magnitude)
            end
            local roomTag = info.RoomNumber and string.format("Room %d • ", info.RoomNumber) or ""
            if isBlacklisted then
                sub.Text = string.format("[BLACKLISTED] • %s%dm", roomTag, dist)
                sub.TextColor3 = Color3.fromRGB(255, 75, 75)
            elseif info.IsActivated then
                sub.Text = string.format("[ACTIVATED] • %s%dm", roomTag, dist)
                sub.TextColor3 = Color3.fromRGB(160, 160, 160)
            elseif not info.IsEnabled then
                sub.Text = string.format("[DISABLED] • %s%dm", roomTag, dist)
                sub.TextColor3 = Color3.fromRGB(170, 140, 60)
            else
                sub.Text = string.format("[READY] • %s%dm", roomTag, dist)
                sub.TextColor3 = Color3.fromRGB(0, 255, 170)
            end
        end

        bb.Enabled = Config.StatueESP
        ActiveStatueBillboards[bb] = anchor
    end
end

local function refreshAllStatueESP()
    local altars = GetAllAltars()
    for _, info in ipairs(altars) do
        applyAltarESP(info)
    end
end

-- ===================================================================
-- 7C. CADENCE INSTRUMENT ESP & TELEPORT ENGINE
-- ===================================================================
local function GetAllCadenceOrbs()
    local list = {}
    local char = LocalPlayer.Character
    pcall(function()
        for _, orb in ipairs(CollectionService:GetTagged("CadenceOrb")) do
            if orb and orb:IsA("BasePart") and orb.Parent and orb.Name ~= "CadenceAuraProtection" and not (char and orb:IsDescendantOf(char)) then
                table.insert(list, orb)
            end
        end
    end)
    if #list == 0 then
        pcall(function()
            for _, d in ipairs(Workspace:GetDescendants()) do
                if d.Name == "CadenceOrb" and d:IsA("BasePart") and not (char and d:IsDescendantOf(char)) and not table.find(list, d) then
                    table.insert(list, d)
                end
            end
        end)
    end
    return list
end

local function applyCadenceESP(orb)
    if not orb or not orb.Parent or not orb:IsA("BasePart") then return end
    if orb.Name == "CadenceAuraProtection" or (LocalPlayer.Character and orb:IsDescendantOf(LocalPlayer.Character)) then return end

    local instName = "Cadence Instrument"
    local icon = ""
    if orb:FindFirstChild("violin") then
        instName = "Cadence Violin"
        icon = ""
    elseif orb:FindFirstChild("harp") then
        instName = "Cadence Harp"
        icon = ""
    end

    local pinkColor = Config.ColorCadence or Color3.fromRGB(255, 20, 147)
    local activeColor = Color3.fromRGB(255, 0, 128)

    -- 1. Highlight
    local hl = orb:FindFirstChild("Aether_Cadence_HL")
    if not hl then
        hl = Instance.new("Highlight")
        hl.Name = "Aether_Cadence_HL"
        hl.DepthMode = Enum.HighlightDepthMode.AlwaysOnTop
        hl.Parent = orb
    end
    hl.OutlineColor = pinkColor
    hl.OutlineTransparency = 0
    hl.FillColor = activeColor
    hl.FillTransparency = 0.55
    hl.Enabled = Config.CadenceESP
    ActiveCadenceHighlights[hl] = orb

    -- 2. 3D Floating Billboard Label
    local bb = orb:FindFirstChild("Aether_Cadence_ESP")
    if not bb then
        bb = Instance.new("BillboardGui")
        bb.Name = "Aether_Cadence_ESP"
        bb.Size = UDim2.new(0, 200, 0, 52)
        bb.StudsOffset = Vector3.new(0, 6.5, 0)
        bb.AlwaysOnTop = true
        bb.MaxDistance = 3000

        local frame = Instance.new("Frame")
        frame.Name = "Card"
        frame.Size = UDim2.new(1, 0, 1, 0)
        frame.BackgroundColor3 = Color3.fromRGB(16, 12, 22)
        frame.BackgroundTransparency = 0.25
        frame.BorderSizePixel = 0

        local corner = Instance.new("UICorner")
        corner.CornerRadius = UDim.new(0, 6)
        corner.Parent = frame

        local stroke = Instance.new("UIStroke")
        stroke.Name = "BorderStroke"
        stroke.Color = pinkColor
        stroke.Thickness = 1.5
        stroke.Parent = frame

        local titleLabel = Instance.new("TextLabel")
        titleLabel.Name = "InstrumentTitle"
        titleLabel.Size = UDim2.new(1, 0, 0.52, 0)
        titleLabel.BackgroundTransparency = 1
        titleLabel.Font = Enum.Font.BuilderSansBold
        titleLabel.TextSize = 14
        titleLabel.TextColor3 = pinkColor
        titleLabel.Text = instName
        titleLabel.TextXAlignment = Enum.TextXAlignment.Center
        titleLabel.Parent = frame

        local subLabel = Instance.new("TextLabel")
        subLabel.Name = "InstrumentSub"
        subLabel.Position = UDim2.new(0, 0, 0.5, 0)
        subLabel.Size = UDim2.new(1, 0, 0.48, 0)
        subLabel.BackgroundTransparency = 1
        subLabel.Font = Enum.Font.BuilderSansMedium
        subLabel.TextSize = 12
        subLabel.TextColor3 = Color3.fromRGB(255, 75, 75)
        subLabel.Text = "[COLLECT OR DIE] • 0m"
        subLabel.TextXAlignment = Enum.TextXAlignment.Center
        subLabel.Parent = frame

        frame.Parent = bb
        bb.Parent = orb
    end

    local sub = bb:FindFirstChild("Card") and bb.Card:FindFirstChild("InstrumentSub")
    if sub then
        local dist = 0
        local char = LocalPlayer.Character
        local hrp = char and char:FindFirstChild("HumanoidRootPart")
        if hrp then
            dist = math.floor((orb.Position - hrp.Position).Magnitude)
        end
        if orb.CanTouch then
            sub.Text = string.format("[COLLECT OR DIE] • %dm", dist)
            sub.TextColor3 = Color3.fromRGB(255, 80, 80)
        else
            sub.Text = string.format("[COLLECTED] • %dm", dist)
            sub.TextColor3 = Color3.fromRGB(150, 150, 160)
        end
    end

    bb.Enabled = Config.CadenceESP
    ActiveCadenceBillboards[bb] = orb
end

local function refreshAllCadenceESP()
    local orbs = GetAllCadenceOrbs()
    for _, orb in ipairs(orbs) do
        applyCadenceESP(orb)
    end
end

local function TeleportToNearestCadenceInstrument()
    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    if not hrp then return false, "No character found." end

    local orbs = GetAllCadenceOrbs()
    local best = nil
    local minDist = math.huge
    local myPos = hrp.Position

    for _, orb in ipairs(orbs) do
        if orb.CanTouch then
            local d = (orb.Position - myPos).Magnitude
            if d < minDist then
                minDist = d
                best = orb
            end
        end
    end

    if best then
        hrp.CFrame = CFrame.new(best.Position + Vector3.new(0, 2, 0))
        hrp.AssemblyLinearVelocity = Vector3.zero
        hrp.AssemblyAngularVelocity = Vector3.zero
        local instName = best:FindFirstChild("violin") and "Violin" or (best:FindFirstChild("harp") and "Harp" or "Instrument")
        return true, string.format("Teleported to Cadence %s (%.0fm away)!", instName, minDist)
    else
        return false, "No active uncollected Cadence instruments found."
    end
end

-- Cadence CollectionService event listeners
trackConn(CollectionService:GetInstanceAddedSignal("CadenceOrb"):Connect(function(orb)
    if orb.Name == "CadenceAuraProtection" or (LocalPlayer.Character and orb:IsDescendantOf(LocalPlayer.Character)) then return end
    task.wait(0.1)
    applyCadenceESP(orb)
    if Window and Config.CadenceESP then
        pcall(function()
            Window:Notify({
                Title = "Cadence Instrument Spawned!",
                Desc = "A Cadence instrument has appeared! Collect it to avoid enrage / death.",
                Time = 4.5
            })
        end)
    end
end))

trackConn(CollectionService:GetInstanceRemovedSignal("CadenceOrb"):Connect(function(orb)
    for hl, target in pairs(ActiveCadenceHighlights) do
        if target == orb then
            pcall(function() hl:Destroy() end)
            ActiveCadenceHighlights[hl] = nil
        end
    end
    for bb, target in pairs(ActiveCadenceBillboards) do
        if target == orb then
            pcall(function() bb:Destroy() end)
            ActiveCadenceBillboards[bb] = nil
        end
    end
end))

-- Cadence distance update background loop
task.spawn(function()
    while ScriptRunning do
        if Config.CadenceESP then
            for bb, orb in pairs(ActiveCadenceBillboards) do
                if bb and bb.Parent and orb and orb.Parent then
                    local sub = bb:FindFirstChild("Card") and bb.Card:FindFirstChild("InstrumentSub")
                    if sub then
                        local char = LocalPlayer.Character
                        local hrp = char and char:FindFirstChild("HumanoidRootPart")
                        local dist = hrp and math.floor((orb.Position - hrp.Position).Magnitude) or 0
                        if orb.CanTouch then
                            sub.Text = string.format("[COLLECT OR DIE] • %dm", dist)
                            sub.TextColor3 = Color3.fromRGB(255, 80, 80)
                        else
                            sub.Text = string.format("[COLLECTED] • %dm", dist)
                            sub.TextColor3 = Color3.fromRGB(150, 150, 160)
                        end
                    end
                end
            end
        end
        task.wait(0.3)
    end
end)

-- ===================================================================
-- 7D. MEDAL ESP & HELPER ENGINE (Collapse & Upgrade Medals)
-- ===================================================================
local function IsMedalHeld(medal)
    if not medal or not medal.Parent then return true end
    local char = LocalPlayer.Character
    if char and medal:IsDescendantOf(char) then return true end

    for _, p in ipairs(Players:GetPlayers()) do
        if p.Character and medal:IsDescendantOf(p.Character) then
            return true
        end
    end

    local holder = medal:FindFirstChild("Holder")
    if holder and holder:IsA("ObjectValue") and holder.Value ~= nil then
        return true
    end

    return false
end

local function PlayerHasMedal()
    local char = LocalPlayer.Character
    if char and char:FindFirstChild("Medal") then
        return true
    end
    if MovementCore and MovementCore.Status and typeof(MovementCore.Status.HasStatus) == "function" then
        local ok, has = pcall(function() return MovementCore.Status.HasStatus("Medal") end)
        if ok and has then return true end
    end
    return false
end

local function GetAllActiveMedals()
    local medals = {}
    local seen = {}

    -- 1. Check event-tracked medals from KnownMedals (O(1))
    for medal in pairs(KnownMedals) do
        if medal and medal.Parent and not seen[medal] and not IsMedalHeld(medal) and medal.Position.Y > -200 then
            seen[medal] = true
            table.insert(medals, medal)
        end
    end

    -- 2. Check active billboards
    for bb, target in pairs(ActiveMedalBillboards) do
        if target and target.Parent and not seen[target] and not IsMedalHeld(target) and target.Position.Y > -200 then
            seen[target] = true
            table.insert(medals, target)
        end
    end

    -- 3. Fast check CollectionService tagged "Medal" (O(1) table lookup)
    pcall(function()
        for _, obj in ipairs(CollectionService:GetTagged("Medal")) do
            local part = obj:IsA("BasePart") and obj or obj:FindFirstChildWhichIsA("BasePart")
            if part and not seen[part] and part:IsDescendantOf(Workspace) and not IsMedalHeld(part) and part.Position.Y > -200 then
                seen[part] = true
                table.insert(medals, part)
            end
        end
    end)

    -- 4. Fast shallow check direct Workspace children
    pcall(function()
        for _, child in ipairs(Workspace:GetChildren()) do
            if child.Name == "Medal" and child:IsA("BasePart") and not seen[child] and not IsMedalHeld(child) and child.Position.Y > -200 then
                seen[child] = true
                table.insert(medals, child)
            end
        end
    end)

    return medals
end

local function applyMedalESP(medal)
    if not medal or not medal.Parent or not medal:IsA("BasePart") then return end
    KnownMedals[medal] = true
    if LocalPlayer.Character and medal:IsDescendantOf(LocalPlayer.Character) then return end

    local goldColor = Color3.fromRGB(255, 215, 0)
    local brightGold = Color3.fromRGB(255, 235, 70)

    -- 1. Highlight
    local hl = medal:FindFirstChild("Aether_Medal_HL")
    if not hl then
        hl = Instance.new("Highlight")
        hl.Name = "Aether_Medal_HL"
        hl.DepthMode = Enum.HighlightDepthMode.AlwaysOnTop
        hl.Parent = medal
    end
    hl.OutlineColor = goldColor
    hl.OutlineTransparency = 0
    hl.FillColor = brightGold
    hl.FillTransparency = 0.5
    hl.Enabled = Config.PresentsESP
    ActiveMedalHighlights[hl] = medal

    -- 2. 3D Floating Billboard Label
    local bb = medal:FindFirstChild("Aether_Medal_ESP")
    if not bb then
        bb = Instance.new("BillboardGui")
        bb.Name = "Aether_Medal_ESP"
        bb.Size = UDim2.new(0, 190, 0, 48)
        bb.StudsOffset = Vector3.new(0, 3.2, 0)
        bb.AlwaysOnTop = true
        bb.MaxDistance = 800
        bb.ResetOnSpawn = false
        bb.Parent = medal

        local card = Instance.new("Frame")
        card.Name = "Card"
        card.Size = UDim2.new(1, 0, 1, 0)
        card.BackgroundColor3 = Color3.fromRGB(15, 12, 5)
        card.BackgroundTransparency = 0.25
        card.BorderSizePixel = 0
        card.Parent = bb

        local corner = Instance.new("UICorner")
        corner.CornerRadius = UDim.new(0, 8)
        corner.Parent = card

        local stroke = Instance.new("UIStroke")
        stroke.Color = goldColor
        stroke.Thickness = 1.6
        stroke.Transparency = 0.2
        stroke.Parent = card

        local header = Instance.new("TextLabel")
        header.Name = "Header"
        header.Size = UDim2.new(1, 0, 0, 20)
        header.Position = UDim2.new(0, 0, 0, 3)
        header.BackgroundTransparency = 1
        header.Font = Enum.Font.GothamBold
        header.Text = "MEDAL"
        header.TextColor3 = brightGold
        header.TextSize = 13
        header.Parent = card

        local sub = Instance.new("TextLabel")
        sub.Name = "MedalSub"
        sub.Size = UDim2.new(1, 0, 0, 18)
        sub.Position = UDim2.new(0, 0, 0, 24)
        sub.BackgroundTransparency = 1
        sub.Font = Enum.Font.GothamMedium
        sub.Text = "[BRING TO BEACON]"
        sub.TextColor3 = Color3.fromRGB(255, 230, 130)
        sub.TextSize = 10.5
        sub.Parent = card
    end

    bb.Enabled = Config.PresentsESP
    ActiveMedalBillboards[bb] = medal
end

local function cleanupMedalESP(medal)
    KnownMedals[medal] = nil
    for hl, target in pairs(ActiveMedalHighlights) do
        if target == medal then
            pcall(function() hl:Destroy() end)
            ActiveMedalHighlights[hl] = nil
        end
    end
    for bb, target in pairs(ActiveMedalBillboards) do
        if target == medal then
            pcall(function() bb:Destroy() end)
            ActiveMedalBillboards[bb] = nil
        end
    end
end

local function refreshAllMedalESP()
    local medals = GetAllActiveMedals()
    for _, medal in ipairs(medals) do
        applyMedalESP(medal)
    end
end

-- Medal distance update background loop
task.spawn(function()
    while ScriptRunning do
        if Config.PresentsESP then
            for bb, medal in pairs(ActiveMedalBillboards) do
                if bb and bb.Parent and medal and medal.Parent then
                    local sub = bb:FindFirstChild("Card") and bb.Card:FindFirstChild("MedalSub")
                    if sub then
                        local char = LocalPlayer.Character
                        local hrp = char and char:FindFirstChild("HumanoidRootPart")
                        local dist = hrp and math.floor((medal.Position - hrp.Position).Magnitude) or 0
                        if IsMedalHeld(medal) then
                            sub.Text = string.format("[HELD] • %dm", dist)
                            sub.TextColor3 = Color3.fromRGB(150, 150, 160)
                        else
                            sub.Text = string.format("[BRING TO BEACON] • %dm", dist)
                            sub.TextColor3 = Color3.fromRGB(255, 230, 130)
                        end
                    end
                end
            end
        end
        task.wait(0.3)
    end
end)

-- Initial application
refreshAllHighlights()
refreshEnemyHighlights()
refreshAllStatueESP()
refreshAllCadenceESP()
refreshAllMedalESP()

local function bindEnemiesFolder(folder)
    if not folder then return end
    EnemiesFolder = folder
    trackConn(folder.ChildAdded:Connect(function(child)
        task.wait(0.05)
        if Config.AutoDisableEnemies then
            RemoveEnemy(child)
        else
            if Config.EnemyDamageImmunity then
                pacifyEnemy(child)
            end
            highlightEnemy(child)
        end
    end))
    if Config.AutoDisableEnemies then
        for _, ch in ipairs(folder:GetChildren()) do
            RemoveEnemy(ch)
        end
    elseif Config.EnemyDamageImmunity then
        for _, ch in ipairs(folder:GetChildren()) do
            pacifyEnemy(ch)
        end
    end
end

local function bindSkinwalkersFolder(folder)
    if not folder then return end
    trackConn(folder.ChildAdded:Connect(function(child)
        task.wait(0.05)
        if Config.AutoDisableEnemies then
            RemoveEnemy(child)
        elseif Config.EnemyDamageImmunity then
            pcall(function()
                if child.Name == "Part2" then
                    child:Destroy()
                elseif child:IsA("BasePart") then
                    child.CanTouch = false
                    child.CanCollide = false
                else
                    pacifyEnemy(child)
                end
            end)
        else
            highlightEnemy(child)
        end
    end))
    if Config.AutoDisableEnemies then
        for _, ch in ipairs(folder:GetChildren()) do
            RemoveEnemy(ch)
        end
    elseif Config.EnemyDamageImmunity then
        for _, ch in ipairs(folder:GetChildren()) do
            pcall(function()
                if ch.Name == "Part2" then
                    ch:Destroy()
                elseif ch:IsA("BasePart") then
                    ch.CanTouch = false
                    ch.CanCollide = false
                else
                    pacifyEnemy(ch)
                end
            end)
        end
    end
end

-- Listen to workspace changes for auto-healing across rounds
trackConn(Workspace.ChildAdded:Connect(function(child)
    if child.Name == "Item_Pools" then
        ItemPools = child
        task.wait(0.2)
        refreshAllHighlights()
    elseif child.Name == "Enemies" then
        bindEnemiesFolder(child)
        task.wait(0.2)
        refreshEnemyHighlights()
    elseif child.Name == "Skinwalkers" then
        bindSkinwalkersFolder(child)
    elseif child.Name == "CurrentRooms" then
        task.wait(0.5)
        refreshAllStatueESP()
        refreshAllMedalESP()
    elseif child.Name == "Medal" and child:IsA("BasePart") then
        KnownMedals[child] = true
        task.wait(0.1)
        applyMedalESP(child)
    elseif (child.Name == "Indicator" or string.find(string.lower(child.Name), "icbmmark")) and child:IsA("BasePart") then
        child.CanCollide = false
        child.CanQuery = false
    end
end))

trackConn(Workspace.DescendantAdded:Connect(function(desc)
    if desc.Name == "Medal" and desc:IsA("BasePart") then
        KnownMedals[desc] = true
        task.wait(0.1)
        applyMedalESP(desc)
    elseif (desc.Name == "Indicator" or string.find(string.lower(desc.Name), "icbmmark")) and desc:IsA("BasePart") then
        desc.CanCollide = false
        desc.CanQuery = false
    end
end))

trackConn(Workspace.DescendantRemoving:Connect(function(desc)
    if desc.Name == "Medal" then
        KnownMedals[desc] = nil
        cleanupMedalESP(desc)
    end
end))

-- Listen for Collapse event (Medal Spawner trigger)
pcall(function()
    local events = ReplicatedStorage:FindFirstChild("Events")
    local collapseEvent = events and events:FindFirstChild("Collapse")
    if collapseEvent and collapseEvent:IsA("RemoteEvent") then
        trackConn(collapseEvent.OnClientEvent:Connect(function()
            task.wait(0.5)
            refreshAllMedalESP()
            if Window then
                pcall(function()
                    Window:Notify({
                        Title = "Collapse Started!",
                        Desc = "Medal has spawned! Teleport sweeper will collect it.",
                        Time = 5.0
                    })
                end)
            end
        end))
    end
end)

-- ===================================================================
-- 7E. CELESTIAL PYLON ESP ENGINE (Secret Boss Summoning Crystals)
-- ===================================================================
local function GetPylonAdornee(pylon)
    if not pylon or not pylon.Parent then return nil end
    local stayNear = pylon:FindFirstChild("StayNear")
    if stayNear and stayNear:IsA("BasePart") then return stayNear end
    local hitbox = pylon:FindFirstChild("HitBox") or pylon:FindFirstChild("Hitbox")
    if hitbox and hitbox:IsA("BasePart") then return hitbox end
    local base = pylon:FindFirstChild("BasePlatform") or pylon:FindFirstChild("InnerBase")
    if base and base:IsA("BasePart") then return base end
    if pylon.PrimaryPart then return pylon.PrimaryPart end
    return pylon:FindFirstChildWhichIsA("BasePart")
end

local function GetAllPylons()
    local results = {}
    local seen = {}

    pcall(function()
        for _, obj in ipairs(CollectionService:GetTagged("Pylon")) do
            local mod = obj:IsA("Model") and obj or obj:FindFirstAncestorOfClass("Model")
            if mod and not seen[mod] then
                seen[mod] = true
                table.insert(results, mod)
            end
        end
    end)

    pcall(function()
        local rooms = Workspace:FindFirstChild("CurrentRooms")
        if rooms then
            for _, desc in ipairs(rooms:GetChildren()) do
                if desc:IsA("Model") and not seen[desc] then
                    if desc.Name == "CellPlatform" or desc:GetAttribute("PylonType") == "Pylon" or desc:GetAttribute("Pylon") ~= nil or desc:FindFirstChild("CellPlatformHandlerClient") then
                        seen[desc] = true
                        table.insert(results, desc)
                    end
                end
            end
        end
    end)

    return results
end

local function applyPylonESP(pylon)
    if not pylon or not pylon.Parent then return end
    local adornee = GetPylonAdornee(pylon)
    if not adornee then return end

    local isCompleted = pylon:GetAttribute("Completed") == true
    local isProgressing = pylon:GetAttribute("Progressing") == true
    local timeElapsed = tonumber(pylon:GetAttribute("TimeElapsed")) or 0
    local timeRequired = tonumber(pylon:GetAttribute("TimeRequired")) or 10
    local currentPlayers = tonumber(pylon:GetAttribute("CurrentPlayers")) or 0

    local magentaColor = Color3.fromRGB(255, 60, 180)  -- Unactivated Hot Pink / Magenta
    local chargingColor = Color3.fromRGB(255, 205, 50)  -- Progressing Gold / Yellow
    local completedColor = Color3.fromRGB(60, 255, 120) -- Completed Neon Green

    local stateColor = isCompleted and completedColor or (isProgressing and chargingColor or magentaColor)
    local shouldShow = Config.PylonESP and (Config.PylonShowCompleted or not isCompleted)

    -- 1. Highlight
    local hl = pylon:FindFirstChild("Aether_Pylon_HL")
    if not hl then
        hl = Instance.new("Highlight")
        hl.Name = "Aether_Pylon_HL"
        hl.DepthMode = Enum.HighlightDepthMode.AlwaysOnTop
        hl.Parent = pylon
    end
    hl.Adornee = pylon
    hl.OutlineColor = stateColor
    hl.OutlineTransparency = 0.1
    hl.FillColor = stateColor
    hl.FillTransparency = isCompleted and 0.8 or 0.55
    hl.Enabled = shouldShow
    ActivePylonHighlights[hl] = pylon

    -- 2. 3D Billboard Label
    local bb = adornee:FindFirstChild("Aether_Pylon_ESP")
    if not bb then
        bb = Instance.new("BillboardGui")
        bb.Name = "Aether_Pylon_ESP"
        bb.Size = UDim2.new(0, 220, 0, 56)
        bb.StudsOffset = Vector3.new(0, 9.5, 0)
        bb.AlwaysOnTop = true
        bb.MaxDistance = 6000

        local frame = Instance.new("Frame")
        frame.Name = "Card"
        frame.Size = UDim2.new(1, 0, 1, 0)
        frame.BackgroundColor3 = Color3.fromRGB(16, 10, 22)
        frame.BackgroundTransparency = 0.2
        frame.BorderSizePixel = 0

        local corner = Instance.new("UICorner")
        corner.CornerRadius = UDim.new(0, 6)
        corner.Parent = frame

        local stroke = Instance.new("UIStroke")
        stroke.Name = "BorderStroke"
        stroke.Color = stateColor
        stroke.Thickness = 1.5
        stroke.Parent = frame

        local titleLabel = Instance.new("TextLabel")
        titleLabel.Name = "PylonTitle"
        titleLabel.Size = UDim2.new(1, 0, 0.5, 0)
        titleLabel.BackgroundTransparency = 1
        titleLabel.Font = Enum.Font.BuilderSansBold
        titleLabel.TextSize = 14
        titleLabel.TextColor3 = stateColor
        titleLabel.Text = "CELESTIAL PYLON"
        titleLabel.TextXAlignment = Enum.TextXAlignment.Center
        titleLabel.Parent = frame

        local subLabel = Instance.new("TextLabel")
        subLabel.Name = "PylonSub"
        subLabel.Position = UDim2.new(0, 0, 0.48, 0)
        subLabel.Size = UDim2.new(1, 0, 0.5, 0)
        subLabel.BackgroundTransparency = 1
        subLabel.Font = Enum.Font.BuilderSansMedium
        subLabel.TextSize = 12
        subLabel.TextColor3 = stateColor
        subLabel.Text = "[STAND TO CHARGE] • 0m"
        subLabel.TextXAlignment = Enum.TextXAlignment.Center
        subLabel.Parent = frame

        frame.Parent = bb
        bb.Parent = adornee
    end

    local card = bb:FindFirstChild("Card")
    local stroke = card and card:FindFirstChild("BorderStroke")
    local title = card and card:FindFirstChild("PylonTitle")
    local sub = card and card:FindFirstChild("PylonSub")

    if stroke then stroke.Color = stateColor end
    if title then title.TextColor3 = stateColor end

    if sub then
        local char = LocalPlayer.Character
        local hrp = char and char:FindFirstChild("HumanoidRootPart")
        local cam = Workspace.CurrentCamera
        local myPos = (hrp and hrp.Position) or (cam and cam.CFrame.Position)
        local dist = myPos and math.floor((adornee.Position - myPos).Magnitude) or 0

        if isCompleted then
            sub.Text = string.format("[COMPLETED] • %dm", dist)
            sub.TextColor3 = completedColor
        elseif isProgressing then
            sub.Text = string.format("[CHARGING: %.1fs / %ds%s] • %dm", timeElapsed, timeRequired, currentPlayers > 0 and string.format(" • %dP", currentPlayers) or "", dist)
            sub.TextColor3 = chargingColor
        else
            sub.Text = string.format("[STAND TO CHARGE: %.1fs / %ds] • %dm", timeElapsed, timeRequired, dist)
            sub.TextColor3 = magentaColor
        end
    end

    bb.Enabled = shouldShow
    ActivePylonBillboards[bb] = pylon
end

local function cleanupPylonESP(pylon)
    for hl, target in pairs(ActivePylonHighlights) do
        if target == pylon or (pylon and pylon.Parent and target:IsDescendantOf(pylon)) then
            pcall(function() hl:Destroy() end)
            ActivePylonHighlights[hl] = nil
        end
    end
    for bb, target in pairs(ActivePylonBillboards) do
        if target == pylon or (pylon and pylon.Parent and target:IsDescendantOf(pylon)) then
            pcall(function() bb:Destroy() end)
            ActivePylonBillboards[bb] = nil
        end
    end
end

local function trackPylonSignals(pylon)
    if not pylon or not pylon:IsA("Model") then return end
    trackConn(pylon:GetAttributeChangedSignal("Completed"):Connect(function()
        applyPylonESP(pylon)
    end))
    trackConn(pylon:GetAttributeChangedSignal("Progressing"):Connect(function()
        applyPylonESP(pylon)
    end))
    trackConn(pylon:GetAttributeChangedSignal("TimeElapsed"):Connect(function()
        applyPylonESP(pylon)
    end))
    trackConn(pylon:GetAttributeChangedSignal("CurrentPlayers"):Connect(function()
        applyPylonESP(pylon)
    end))
end

local function refreshAllPylonESP()
    for hl, pylon in pairs(ActivePylonHighlights) do
        if hl and hl.Parent then
            local isComp = pylon and pylon.Parent and pylon:GetAttribute("Completed") == true
            hl.Enabled = Config.PylonESP and (Config.PylonShowCompleted or not isComp)
        end
    end
    for bb, pylon in pairs(ActivePylonBillboards) do
        if bb and bb.Parent then
            local isComp = pylon and pylon.Parent and pylon:GetAttribute("Completed") == true
            bb.Enabled = Config.PylonESP and (Config.PylonShowCompleted or not isComp)
        end
    end

    local pylons = GetAllPylons()
    for _, pylon in ipairs(pylons) do
        applyPylonESP(pylon)
        trackPylonSignals(pylon)
    end
end

local function TeleportToNearestPylon()
    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    if not hrp then return false, "No character found." end

    local pylons = GetAllPylons()
    local best = nil
    local minDist = math.huge
    local myPos = hrp.Position

    for _, pylon in ipairs(pylons) do
        local isComp = pylon:GetAttribute("Completed") == true
        if not isComp then
            local adornee = GetPylonAdornee(pylon)
            if adornee then
                local d = (adornee.Position - myPos).Magnitude
                if d < minDist then
                    minDist = d
                    best = adornee
                end
            end
        end
    end

    if best then
        pcall(function()
            local targetPos = best.Position + Vector3.new(0, 3.5, 0)
            hrp.CFrame = CFrame.new(targetPos)
            hrp.AssemblyLinearVelocity = Vector3.zero
            hrp.AssemblyAngularVelocity = Vector3.zero
        end)
        return true, string.format("Teleported to Pylon (%.0fm away)!", minDist)
    end

    return false, "No uncompleted pylons found."
end

-- Pylon dynamic listeners
trackConn(CollectionService:GetInstanceAddedSignal("Pylon"):Connect(function(inst)
    local mod = inst:IsA("Model") and inst or inst:FindFirstAncestorOfClass("Model")
    if mod then
        task.wait(0.1)
        applyPylonESP(mod)
        trackPylonSignals(mod)
    end
end))

trackConn(CollectionService:GetInstanceRemovedSignal("Pylon"):Connect(function(inst)
    cleanupPylonESP(inst)
end))

pcall(function()
    local rooms = Workspace:WaitForChild("CurrentRooms", 5)
    if rooms then
        trackConn(rooms.ChildAdded:Connect(function(ch)
            if ch.Name == "CellPlatform" or ch:GetAttribute("PylonType") == "Pylon" then
                task.wait(0.1)
                applyPylonESP(ch)
                trackPylonSignals(ch)
            end
        end))
        trackConn(rooms.ChildRemoved:Connect(function(ch)
            if ch.Name == "CellPlatform" then
                cleanupPylonESP(ch)
            end
        end))
    end
end)

-- Pylon distance & live state background loop
task.spawn(function()
    while ScriptRunning do
        if Config.PylonESP then
            local count = 0
            for bb, pylon in pairs(ActivePylonBillboards) do
                if bb and bb.Parent and pylon and pylon.Parent then
                    applyPylonESP(pylon)
                    count = count + 1
                else
                    ActivePylonBillboards[bb] = nil
                end
            end
            if count == 0 then
                pcall(refreshAllPylonESP)
            end
        end
        task.wait(0.25)
    end
end)

pcall(refreshAllPylonESP)

if ItemPools then
    trackConn(ItemPools.ChildAdded:Connect(function()
        task.wait(0.1)
        refreshAllHighlights()
    end))
end

local currentEnemies = Workspace:FindFirstChild("Enemies")
if currentEnemies then
    bindEnemiesFolder(currentEnemies)
end

local skinwalkersFolder = Workspace:FindFirstChild("Skinwalkers")
if skinwalkersFolder then
    bindSkinwalkersFolder(skinwalkersFolder)
end

trackConn(CollectionService:GetInstanceAddedSignal("Altar"):Connect(function()
    task.wait(0.2)
    refreshAllStatueESP()
end))

if InRound then
    trackConn(InRound.Changed:Connect(function(val)
        if val == true then
            lastBeaconNotifiedLevel = -1
            table.clear(LastAltarTrigger)
            table.clear(GiftTracker.Pending)
            table.clear(GiftTracker.Failed)
            table.clear(GiftTracker.Confirmed)
            task.wait(0.5)
            refreshAllStatueESP()
            refreshAllPylonESP()
        end
    end))
end

if LevelVal then
    trackConn(LevelVal.Changed:Connect(function()
        task.wait(0.3)
        refreshAllStatueESP()
        refreshAllPylonESP()
    end))
end

-- Seamine reactive listeners
trackConn(CollectionService:GetInstanceAddedSignal("Seamine"):Connect(function(inst)
    if Config.AutoDisableEnemies or Config.ProjectileDisabler then
        task.wait(0.02)
        RemoveSeamine(inst)
    end
end))

local jumpPadsFolder = Workspace:FindFirstChild("JumpPads")
if jumpPadsFolder then
    trackConn(jumpPadsFolder.DescendantAdded:Connect(function(d)
        if Config.AutoDisableEnemies or Config.ProjectileDisabler then
            if d.Name == "Seamine" or d.Name == "SeamineGoback" or d.Name == "SeamineUNC" or string.find(string.lower(d.Name), "seamine") then
                task.wait(0.02)
                RemoveSeamine(d)
            end
        end
    end))
end

-- Black Hole (Void Implosion & Oblivion) listeners
local destroyFolder = Workspace:FindFirstChild("DestroyFolder")
if destroyFolder then
    trackConn(destroyFolder.ChildAdded:Connect(function(ch)
        if Config.AutoDisableEnemies or Config.ProjectileDisabler then
            if ch.Name == "VoidExplosion" or string.find(ch.Name, "Void") or string.find(ch.Name, "Implos") or ch:FindFirstChild("VoidInner") or ch:FindFirstChild("VoidImplode") then
                RemoveBlackHole(ch)
            end
        end
    end))
end

local cameraInstance = Workspace.CurrentCamera
if cameraInstance then
    trackConn(cameraInstance.ChildAdded:Connect(function(ch)
        if Config.AutoDisableEnemies or Config.ProjectileDisabler then
            if ch.Name == "OblivionAttack" or ch.Name == "OblivionAmbient" or ch.Name == "OblivionSky" then
                pcall(function() ch:Destroy() end)
            end
        end
    end))
end

-- Initial sweep for hazards
if Config.AutoDisableEnemies or Config.ProjectileDisabler then
    task.spawn(ClearAllSeamines)
    task.spawn(ClearAllBlackHoles)
end

-- ===================================================================
-- 8. INTELLIGENT AUTO-COLLECT ENGINE (Safe from Tripmines & _G Hook)
-- ===================================================================
task.spawn(function()
    while ScriptRunning do
        pcall(function()
            if Config.AutoCollect then
                local effectiveRadius = math.min(math.clamp(Config.CollectRadius or 35, 15, 80), GiftTracker.MaxRange)
                _G.PICKUP_BONUS = math.min(effectiveRadius, 38)
                local isRoundActive = (not InRound) or (InRound.Value == true)
                local char = LocalPlayer.Character
                local hrp = char and char:FindFirstChild("HumanoidRootPart")

                if isRoundActive and hrp and GiftClientHandler and GiftClientHandler.Gifts then
                    local myPos = hrp.Position
                    local candidates = {}

                    for id, gift in pairs(GiftClientHandler.Gifts) do
                        local isTripmine = gift.Tripmine or gift.Type == "Tripmine"
                        local isSafe = (not isTripmine) or Config.TripmineDisabler

                        if isSafe and GiftTracker:CanAttempt(gift, myPos, effectiveRadius) then
                            local dist = (gift.Position - myPos).Magnitude
                            table.insert(candidates, {
                                gift = gift,
                                dist = dist,
                                golden = (gift.Golden or gift.Type == "GoldenGift" or gift.Type == "PassageGift")
                            })
                        end
                    end

                    if #candidates > 0 then
                        table.sort(candidates, function(a, b)
                            if a.golden ~= b.golden then
                                return a.golden and not b.golden
                            end
                            return a.dist < b.dist
                        end)

                        local collectCount = math.min(#candidates, Config.CollectBatchSize)
                        for i = 1, collectCount do
                            local cand = candidates[i]
                            if cand and cand.gift and not cand.gift.Collected then
                                local gId = cand.gift.Id
                                if gId then
                                    GiftTracker.Pending[gId] = os.clock()
                                end
                                pcall(function()
                                    cand.gift:Collect(false, true, 1)
                                end)
                            end
                        end
                    end

                    -- Fast O(1) proximity pickup for active Medals
                    if not PlayerHasMedal() then
                        for medal in pairs(KnownMedals) do
                            if medal and medal.Parent and not IsMedalHeld(medal) and medal.Position and medal.Position.Y > -200 then
                                local dist = (medal.Position - myPos).Magnitude
                                if dist <= effectiveRadius then
                                    triggerTouchInterest(hrp, medal)
                                    if MovementCore and typeof(MovementCore.OnHitboxTouch) == "function" then
                                        pcall(function() MovementCore:OnHitboxTouch(medal) end)
                                    end
                                end
                            end
                        end
                    end
                end
            else
                _G.PICKUP_BONUS = 0
            end
        end)
        task.wait(Config.CollectInterval)
    end
end)

-- ===================================================================
-- 8B. ALL-IN-ONE FLASH TELEPORT SWEEPER ENGINE & WITHERROT DEFENSE
-- ===================================================================
-- IsSweeping and SweepStartTime declared at top-level scope
local LastStarMartTouch = 0

local LastRazorbloomDispel = 0

-- 1. Precision Razorbloom (Bird / Witherrot) Detection
local function GetAttackingRazorbloom()
    local char = LocalPlayer.Character
    if not char then return nil end

    local rb = char:FindFirstChild("Razorbloom")
    if not rb then
        for _, child in ipairs(char:GetChildren()) do
            local lower = string.lower(child.Name)
            if string.find(lower, "razorbloom") or string.find(lower, "witherbloom") then
                rb = child
                break
            end
        end
    end

    if not rb or not rb:IsA("Model") then return nil end

    -- 1. Check Visibility: When dispelled/dead, all parts are made 100% transparent by the game
    local hasVisiblePart = false
    for _, part in ipairs(rb:GetChildren()) do
        if part:IsA("BasePart") and part.Name ~= "RootPart" and part.Transparency < 0.9 then
            hasVisiblePart = true
            break
        end
    end
    if not hasVisiblePart then
        return nil -- Bird is dead / dispelled / fully transparent!
    end

    -- 2. Check if screaming or in open-petal attack state
    local rp = rb:FindFirstChild("RootPart")
    local scream = rp and rp:FindFirstChild("scream")
    local screamHard = rp and rp:FindFirstChild("scream_hard")
    local isScreaming = (scream and scream.IsPlaying) or (screamHard and screamHard.IsPlaying)

    local openPetals = rb:FindFirstChild("OpenPetals")
    local hasOpenPetals = (openPetals and openPetals:IsA("BasePart") and openPetals.Transparency < 0.5)

    local head = char:FindFirstChild("Head") or char:FindFirstChild("HumanoidRootPart")
    local prim = rb.PrimaryPart or rb:FindFirstChildWhichIsA("BasePart")
    local rbPos = prim and prim.Position or rb:GetPivot().Position
    local distToHead = head and (rbPos - head.Position).Magnitude or 100

    -- Only consider attacking when actively screaming, has open attack petals, or is landed tightly on head
    if not isScreaming and not hasOpenPetals and distToHead > 3.5 then
        return nil
    end

    return rb
end

local function HasRazorbloom()
    return GetAttackingRazorbloom() ~= nil
end

local function GetClosestJumpPad()
    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    if not hrp then return nil end

    local myPos = hrp.Position
    local bestPad = nil
    local bestDist = math.huge

    local jpFolder = Workspace:FindFirstChild("JumpPads")
    if jpFolder then
        for _, pad in ipairs(jpFolder:GetChildren()) do
            if pad.Name == "JumpPad" and pad:IsA("BasePart") then
                local dist = (pad.Position - myPos).Magnitude
                if dist < bestDist then
                    bestDist = dist
                    bestPad = pad
                end
            end
        end
    end

    if not bestPad then
        for _, d in ipairs(Workspace:GetDescendants()) do
            if d.Name == "JumpPad" and d:IsA("BasePart") and d.Position.Y > -200 then
                local dist = (d.Position - myPos).Magnitude
                if dist < bestDist then
                    bestDist = dist
                    bestPad = d
                end
            end
        end
    end

    return bestPad
end

local function triggerTouchInterest(part1, part2)
    local fti: any = firetouchinterest
    if typeof(fti) == "function" and part1 and part2 then
        pcall(function()
            fti(part1, part2, 0)
            task.wait(0.02)
            fti(part1, part2, 1)
        end)
    end
end

local function getBestStandPosition(anchor, model)
    local anchorPos = (anchor:IsA("BasePart") and anchor.Position) or (model and model:GetPivot().Position) or Vector3.zero
    local baseCFrame = (anchor:IsA("BasePart") and anchor.CFrame) or (model and model:GetPivot()) or CFrame.new(anchorPos)

    local killVoid = Workspace:FindFirstChild("KillVoid")
    local killY = (killVoid and killVoid.Position.Y) or -264.5

    -- Never allow stand positions near or below the void kill plane
    if anchorPos.Y < (killY + 50) or anchorPos.Y < -50 then
        return nil, nil
    end

    local dirs = {
        baseCFrame.LookVector,
        -baseCFrame.LookVector,
        baseCFrame.RightVector,
        -baseCFrame.RightVector
    }

    local params = RaycastParams.new()
    params.FilterType = Enum.RaycastFilterType.Exclude
    local filterList = {}
    if LocalPlayer.Character then table.insert(filterList, LocalPlayer.Character) end
    if model then table.insert(filterList, model) end
    if anchor and anchor.Parent and anchor.Parent ~= model then table.insert(filterList, anchor.Parent) end
    local df = Workspace:FindFirstChild("DestroyFolder")
    if df then table.insert(filterList, df) end
    params.FilterDescendantsInstances = filterList

    for _, dir in ipairs(dirs) do
        local horizDir = Vector3.new(dir.X, 0, dir.Z)
        if horizDir.Magnitude > 0.001 then
            horizDir = horizDir.Unit
            local testPos = anchorPos + (horizDir * 4.3)

            -- Check horizontal clearance (ensure no wall blocks this side)
            local wallRay = Workspace:Raycast(anchorPos, horizDir * 4.8, params)
            local isWallBlocked = wallRay and wallRay.Distance < 4.0

            if not isWallBlocked then
                -- Check for solid, collidable, anchored floor beneath the candidate stand position
                local floorRay = Workspace:Raycast(Vector3.new(testPos.X, anchorPos.Y + 3, testPos.Z), Vector3.new(0, -15, 0), params)
                if floorRay and floorRay.Instance and floorRay.Instance.CanCollide and floorRay.Instance.Anchored and floorRay.Position.Y > (killY + 40) then
                    local standY = floorRay.Position.Y + 3.0
                    return Vector3.new(testPos.X, standY, testPos.Z), anchorPos
                end
            end
        end
    end

    -- Direct floor check beneath anchor as fallback
    local directFloor = Workspace:Raycast(Vector3.new(anchorPos.X, anchorPos.Y + 3, anchorPos.Z), Vector3.new(0, -15, 0), params)
    if directFloor and directFloor.Instance and directFloor.Instance.CanCollide and directFloor.Instance.Anchored and directFloor.Position.Y > (killY + 40) then
        return Vector3.new(anchorPos.X, directFloor.Position.Y + 3.0, anchorPos.Z), anchorPos
    end

    -- If no confirmed solid floor exists, NEVER return thin air! Return nil to abort unsafe teleport.
    return nil, nil
end

local function triggerPrompt(prompt, customHold)
    if not prompt or not prompt.Parent then return end
    local fpp: any = fireproximityprompt
    local holdDuration = customHold or prompt.HoldDuration or 0.25

    -- 1. Signal hold begin to Roblox engine
    pcall(function()
        prompt:InputHoldBegin()
    end)

    -- 2. If executor has fireproximityprompt, call with 0 or hold duration
    if typeof(fpp) == "function" then
        pcall(function()
            fpp(prompt, 0)
        end)
    end

    -- 3. Wait for the required hold time + buffer so server registers completion
    local holdWait = math.clamp(holdDuration, 0.2, 0.5) + 0.08
    task.wait(holdWait)

    -- 4. Signal hold end to Roblox engine
    pcall(function()
        prompt:InputHoldEnd()
    end)

    -- 5. Final executor trigger call
    if typeof(fpp) == "function" then
        pcall(function()
            fpp(prompt, holdDuration)
        end)
    end
end

local function HandleRazorbloomEmergency()
    if not Config.AutoDispelWitherrot then return false end

    local now = os.clock()
    if now - LastRazorbloomDispel < 5.0 then
        return false
    end

    local bloomObj = GetAttackingRazorbloom()
    if not bloomObj then return false end

    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    local humanoid = char and char:FindFirstChild("Humanoid")
    if not hrp or not humanoid or humanoid.Health <= 0 then return false end

    local preEmergencyCFrame = hrp.CFrame

    -- 1. Direct Network & Bindable Dispel
    local cb = bloomObj:FindFirstChild("ClientBindable")
    if cb and typeof(cb.Fire) == "function" then
        pcall(function() cb:Fire("Disable") end)
    end

    -- 2. JumpPad Touch Dispel
    local pad = GetClosestJumpPad()
    if pad then
        local hb = char:FindFirstChild("Hitbox")
        -- Graze the pad directly
        hrp.CFrame = CFrame.new(pad.Position + Vector3.new(0, 2.0, 0))
        if hb then triggerTouchInterest(hb, pad) end
        triggerTouchInterest(hrp, pad)

        -- Trigger disable again on pad touch
        if cb and typeof(cb.Fire) == "function" then
            pcall(function() cb:Fire("Disable") end)
        end

        task.wait(0.12)

        -- CRITICAL: Move player completely OFF the pad back to preEmergencyCFrame!
        -- Prevents falling back down onto the pad and getting trapped in an infinite jump loop!
        if hrp and humanoid and humanoid.Health > 0 then
            hrp.CFrame = preEmergencyCFrame
            hrp.AssemblyLinearVelocity = Vector3.zero
            hrp.AssemblyAngularVelocity = Vector3.zero
        end
    end

    LastRazorbloomDispel = now

    if Window then
        pcall(function()
            Window:Notify({
                Title = "Bird Dispelled!",
                Desc = "Scared Razorbloom away and returned to safety!",
                Time = 3.0
            })
        end)
    end

    return true
end

-- 2. Star-Mart Auto-Buff Engine
local function IsStarMartValid(sm)
    if not sm or not sm.Parent or not sm:IsA("BasePart") then return false end
    if sm:GetAttribute("Despawned") == true then return false end
    local pos = sm.Position
    -- NaN checks
    if pos.X ~= pos.X or pos.Y ~= pos.Y or pos.Z ~= pos.Z then return false end
    -- Infinite / despawn checks (StarMart moves to inf when despawned)
    if pos.X == math.huge or pos.X == -math.huge then return false end
    if pos.Y == math.huge or pos.Y == -math.huge then return false end
    if pos.Z == math.huge or pos.Z == -math.huge then return false end
    -- Real bounds check in game
    if pos.Y <= -200 or pos.Y > 3000 then return false end
    if math.abs(pos.X) > 25000 or math.abs(pos.Z) > 25000 then return false end
    return true
end

local function GetActiveStarMart()
    if not EnemiesFolder or not EnemiesFolder.Parent then
        EnemiesFolder = Workspace:FindFirstChild("Enemies")
    end
    if EnemiesFolder then
        local sm = EnemiesFolder:FindFirstChild("StarMart")
        if IsStarMartValid(sm) then
            return sm
        end
    end
    for _, child in ipairs(Workspace:GetChildren()) do
        if child.Name == "StarMart" and IsStarMartValid(child) then
            return child
        end
    end
    local rooms = Workspace:FindFirstChild("CurrentRooms")
    if rooms then
        for _, d in ipairs(rooms:GetDescendants()) do
            if d.Name == "StarMart" and IsStarMartValid(d) then
                return d
            end
        end
    end
    for _, d in ipairs(Workspace:GetDescendants()) do
        if d.Name == "StarMart" and IsStarMartValid(d) then
            return d
        end
    end
    return nil
end

local function CollectStarMart()
    local sm = GetActiveStarMart()
    if not sm then return false, "No active Star-Mart found." end

    local now = os.clock()
    if now - LastStarMartTouch < 1.5 then return false, "Star-Mart attempt on cooldown." end

    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    local humanoid = char and char:FindFirstChild("Humanoid")
    if not hrp or not humanoid or humanoid.Health <= 0 then return false, "No character found." end

    -- Teleport directly into the center of the StarMart (12x12x12 sphere)
    hrp.CFrame = CFrame.new(sm.Position)
    hrp.AssemblyLinearVelocity = Vector3.zero
    hrp.AssemblyAngularVelocity = Vector3.zero

    -- Collect all parts of the character that can trigger touch interest
    local candidateParts = {
        hrp,
        char:FindFirstChild("Hitbox"),
        char:FindFirstChild("Torso") or char:FindFirstChild("UpperTorso"),
        char:FindFirstChild("Head")
    }

    for _, part in ipairs(candidateParts) do
        if part and part:IsA("BasePart") then
            triggerTouchInterest(part, sm)
        end
    end

    if MovementCore and typeof(MovementCore.OnHitboxTouch) == "function" then
        pcall(function() MovementCore:OnHitboxTouch(sm) end)
    end

    -- Replication wait for physics acknowledgement
    task.wait(0.14)

    -- Check if StarMart has despawned (confirming collection)
    local collected = not IsStarMartValid(sm)
    if not collected and IsStarMartValid(sm) then
        -- Second attempt with updated position if moving/lerping
        hrp.CFrame = CFrame.new(sm.Position)
        for _, part in ipairs(candidateParts) do
            if part and part:IsA("BasePart") then
                triggerTouchInterest(part, sm)
            end
        end
        if MovementCore and typeof(MovementCore.OnHitboxTouch) == "function" then
            pcall(function() MovementCore:OnHitboxTouch(sm) end)
        end
        task.wait(0.10)
        collected = not IsStarMartValid(sm)
    end

    if collected then
        LastStarMartTouch = now + 120 -- Long backoff since StarMarts respawn after 3 minutes
        if Window then
            pcall(function()
                Window:Notify({
                    Title = "Star-Mart Collected!",
                    Desc = "Gained Movement Speed, +2 Collection Range & 25s Debuff Immunity!",
                    Time = 4.0
                })
            end)
        end
        return true, "Touched Star-Mart! Gained Speed, +2 Range, and 25s Debuff Immunity!"
    else
        LastStarMartTouch = now -- Short 1.5s retry debounce
        return false, "Touch sent, awaiting server acknowledgement."
    end
end

-- 3. Cadence Instruments Auto-Collection
local function CollectCadenceInstruments()
    local orbs = GetAllCadenceOrbs()
    if not orbs or #orbs == 0 then return false end

    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    local humanoid = char and char:FindFirstChild("Humanoid")
    local isDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
    if not hrp or not hrp.Parent or not humanoid or humanoid.Health <= 0 or isDead then return false end

    local collectedAny = false
    for _, orb in ipairs(orbs) do
        local curChar = LocalPlayer.Character
        local curHrp = curChar and curChar:FindFirstChild("HumanoidRootPart")
        local curHum = curChar and curChar:FindFirstChild("Humanoid")
        local curDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
        if not curHrp or not curHrp.Parent or not curHum or curHum.Health <= 0 or curDead then
            break
        end

        if orb and orb.Parent and orb:IsA("BasePart") and orb.CanTouch then
            pcall(function()
                curHrp.CFrame = CFrame.new(orb.Position + Vector3.new(0, 2, 0))
                curHrp.AssemblyLinearVelocity = Vector3.zero
                curHrp.AssemblyAngularVelocity = Vector3.zero
            end)

            triggerTouchInterest(curHrp, orb)

            collectedAny = true
            task.wait(0.06)
        end
    end
    return collectedAny
end

-- 3B. Medals Auto-Collection (Collapse & Upgrades)
local function CollectMedals()
    if PlayerHasMedal() then return false end
    local medals = GetAllActiveMedals()
    if not medals or #medals == 0 then return false end

    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    local humanoid = char and char:FindFirstChild("Humanoid")
    local isDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
    if not hrp or not hrp.Parent or not humanoid or humanoid.Health <= 0 or isDead then return false end

    local collectedAny = false
    for _, medal in ipairs(medals) do
        local curChar = LocalPlayer.Character
        local curHrp = curChar and curChar:FindFirstChild("HumanoidRootPart")
        local curHum = curChar and curChar:FindFirstChild("Humanoid")
        local curDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
        if not curHrp or not curHrp.Parent or not curHum or curHum.Health <= 0 or curDead then
            break
        end

        if medal and medal.Parent and not IsMedalHeld(medal) and medal.Position.Y > -200 then
            local medalPos = medal.Position
            pcall(function()
                curHrp.CFrame = CFrame.new(medalPos + Vector3.new(0, 1.5, 0))
                curHrp.AssemblyLinearVelocity = Vector3.zero
                curHrp.AssemblyAngularVelocity = Vector3.zero
            end)

            triggerTouchInterest(curHrp, medal)

            if MovementCore and typeof(MovementCore.OnHitboxTouch) == "function" then
                pcall(function() MovementCore:OnHitboxTouch(medal) end)
            end

            local prompt = medal:FindFirstChildWhichIsA("ProximityPrompt", true)
            if prompt then
                triggerPrompt(prompt)
            end

            collectedAny = true
            LastMedalTouch = os.clock()
            task.wait(0.12)

            if PlayerHasMedal() then
                cleanupMedalESP(medal)
                if Window then
                    pcall(function()
                        Window:Notify({
                            Title = "Medal Picked Up!",
                            Desc = "Carrying Medal! Bring it to the Beacon for Golden Gifts.",
                            Time = 4.0
                        })
                    end)
                end
                break
            end
        end
    end
    return collectedAny
end

-- 4. Non-Blacklisted Statues / Altars Auto-Activation
local function ActivateAvailableStatues()
    local altars = GetAllAltars()
    if not altars or #altars == 0 then return false end

    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    local humanoid = char and char:FindFirstChild("Humanoid")
    local isDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
    if not hrp or not hrp.Parent or not humanoid or humanoid.Health <= 0 or isDead then return false end

    local now = os.clock()
    local activatedAny = false

    for _, info in ipairs(altars) do
        local curChar = LocalPlayer.Character
        local curHrp = curChar and curChar:FindFirstChild("HumanoidRootPart")
        local curHum = curChar and curChar:FindFirstChild("Humanoid")
        local curDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
        if not curHrp or not curHrp.Parent or not curHum or curHum.Health <= 0 or curDead then
            return activatedAny
        end

        if info.IsEnabled and info.Prompt and not IsStatueBlacklisted(info.Name) then
            local last = LastAltarTrigger[info.Model] or 0
            if now - last >= 3.0 then
                local anchor = (info.Prompt.Parent and info.Prompt.Parent:IsA("BasePart") and info.Prompt.Parent) or info.Hitbox
                if anchor then
                    local standPos, targetLook = getBestStandPosition(anchor, info.Model)
                    if not standPos then
                        -- Unsafe statue (falling, in void, or no solid floor): back off and skip!
                        LastAltarTrigger[info.Model] = now + 15.0
                    else
                        pcall(function()
                            curHrp.CFrame = CFrame.lookAt(standPos, Vector3.new(targetLook.X, standPos.Y, targetLook.Z))
                            curHrp.AssemblyLinearVelocity = Vector3.zero
                            curHrp.AssemblyAngularVelocity = Vector3.zero
                        end)

                        -- Wait for physics position replication to server
                        task.wait(0.15)

                        -- Trigger prompt with full hold simulation
                        triggerPrompt(info.Prompt)

                        -- Wait brief moment for server replication
                        task.wait(0.12)

                        -- Verify activation status
                        local isNowActivated = false
                        pcall(function()
                            isNowActivated = (info.Model:GetAttribute("Activated") == true)
                                or (info.Model:GetAttribute("AltarUsed") == true)
                                or (info.Prompt and not info.Prompt.Enabled)
                        end)

                        if isNowActivated then
                            LastAltarTrigger[info.Model] = math.huge
                            activatedAny = true
                            if Window then
                                pcall(function()
                                    Window:Notify({
                                        Title = "Statue Activated!",
                                        Desc = string.format("Activated %s Statue!", info.Name),
                                        Time = 3.0
                                    })
                                end)
                            end
                        else
                            -- Not yet consumed, on cooldown, or vote pending: back off for 6 seconds
                            LastAltarTrigger[info.Model] = now + 6.0
                        end
                    end
                end
            end
        end
    end

    if activatedAny then
        task.delay(0.4, refreshAllStatueESP)
    end
    return activatedAny
end

-- 5. Main All-In-One Sweep Routine (Raw Logic)
local function RawPerformPresentSweep(overrideHops, overrideDelay, overrideReturn)
    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    local humanoid = char and char:FindFirstChild("Humanoid")
    local isDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
    if not hrp or not hrp.Parent or not humanoid or humanoid.Health <= 0 or isDead then
        return false, "Character not ready"
    end
    if not GiftClientHandler or not GiftClientHandler.Gifts then return false, "Gift handler not ready" end

    local startCFrame = hrp.CFrame
    local startCollected = 0
    pcall(function()
        for _, g in pairs(GiftClientHandler.Gifts) do
            if g.Collected then startCollected = startCollected + 1 end
        end
    end)

    local maxHops = overrideHops or Config.MaxHopsPerSweep or 50
    local hopDelay = overrideDelay or Config.TeleportSweepDelay or 0.03
    local returnOrigin = (overrideReturn ~= nil and overrideReturn) or Config.ReturnToOrigin
    local prioritizeGold = true

    local killVoid = Workspace:FindFirstChild("KillVoid")
    local killY = (killVoid and killVoid.Position.Y) or -264.5

    -- Helper to calculate the safest walkable ground position to stand on for collecting a gift
    local function getSafeGiftStandPosition(pos)
        if not pos or pos.Y <= (killY + 80) then return nil end

        local rayParams = RaycastParams.new()
        rayParams.FilterType = Enum.RaycastFilterType.Exclude
        local excludeList = {}
        if LocalPlayer.Character then table.insert(excludeList, LocalPlayer.Character) end
        local enemies = Workspace:FindFirstChild("Enemies")
        if enemies then table.insert(excludeList, enemies) end
        local skinwalkers = Workspace:FindFirstChild("Skinwalkers")
        if skinwalkers then table.insert(excludeList, skinwalkers) end
        local killVoid = Workspace:FindFirstChild("KillVoid")
        if killVoid then table.insert(excludeList, killVoid) end
        local bodyCatcher = Workspace:FindFirstChild("BodyCatcher")
        if bodyCatcher then table.insert(excludeList, bodyCatcher) end
        rayParams.FilterDescendantsInstances = excludeList

        local df = Workspace:FindFirstChild("DestroyFolder")

        -- 1. Check directly underneath the gift (raycast down 24 studs)
        local ray = Workspace:Raycast(Vector3.new(pos.X, pos.Y + 3, pos.Z), Vector3.new(0, -24, 0), rayParams)
        if ray and ray.Instance and ray.Instance.CanCollide and ray.Instance.Anchored then
            if not (df and ray.Instance:IsDescendantOf(df)) then
                return ray.Position + Vector3.new(0, 3.0, 0)
            end
        end

        -- 2. If gift is floating over a void gap / chasm, search radial offsets (radii: 4, 8, 12, 16 studs)
        -- to find the nearest solid floor so player can stand safely and collect via vacuum/Collect()
        local radialOffsets = {
            Vector3.new(4, 0, 0), Vector3.new(-4, 0, 0), Vector3.new(0, 0, 4), Vector3.new(0, 0, -4),
            Vector3.new(8, 0, 0), Vector3.new(-8, 0, 0), Vector3.new(0, 0, 8), Vector3.new(0, 0, -8),
            Vector3.new(6, 0, 6), Vector3.new(-6, 0, 6), Vector3.new(6, 0, -6), Vector3.new(-6, 0, -6),
            Vector3.new(12, 0, 0), Vector3.new(-12, 0, 0), Vector3.new(0, 0, 12), Vector3.new(0, 0, -12),
            Vector3.new(16, 0, 0), Vector3.new(-16, 0, 0), Vector3.new(0, 0, 16), Vector3.new(0, 0, -16)
        }

        for _, off in ipairs(radialOffsets) do
            local testPos = pos + off
            local r = Workspace:Raycast(Vector3.new(testPos.X, testPos.Y + 3, testPos.Z), Vector3.new(0, -24, 0), rayParams)
            if r and r.Instance and r.Instance.CanCollide and r.Instance.Anchored then
                if not (df and r.Instance:IsDescendantOf(df)) then
                    return r.Position + Vector3.new(0, 3.0, 0)
                end
            end
        end

        return nil -- No safe floor within 16 studs (e.g. completely in the abyss)
    end

    -- Helper to verify a gift has a safe ground stand position
    local function isGiftPositionSafe(pos)
        if not pos or pos.Y <= (killY + 80) then
            return false
        end

        -- Beacon Exclusion Zone: Never teleport within 16 studs of the Beacon!
        -- Prevents accidentally clipping or touching the Beacon during sweeps.
        local beacon = Workspace:FindFirstChild("Beacon")
        local bPos = beacon and (beacon:IsA("BasePart") and beacon.Position or beacon:GetPivot().Position) or Vector3.new(0, 544.5, 0)
        local distToBeacon2D = math.sqrt((pos.X - bPos.X)^2 + (pos.Z - bPos.Z)^2)
        if distToBeacon2D < 16.0 then
            return false -- Too close to Beacon, collect via vacuum instead!
        end

        return getSafeGiftStandPosition(pos) ~= nil
    end

    -- Helper to instantly vacuum presents around any position (e.g. room cluster or start position)
    local function vacuumPresentsNear(pos, radius)
        if not pos or not GiftClientHandler or not GiftClientHandler.Gifts then return 0 end
        radius = math.min(math.clamp(radius or Config.CollectRadius or 35, 15, 80), GiftTracker.MaxRange)
        local count = 0
        pcall(function()
            for id, g in pairs(GiftClientHandler.Gifts) do
                local isTripmine = g.Tripmine or g.Type == "Tripmine"
                local isSafe = (not isTripmine) or Config.TripmineDisabler
                local isAboveVoid = g.Position and g.Position.Y > (killY + 45)
                if isSafe and isAboveVoid and GiftTracker:CanAttempt(g, pos, radius) then
                    if g.Id then
                        GiftTracker.Pending[g.Id] = os.clock()
                    end
                    pcall(function() g:Collect(false, true, 1) end)
                    count = count + 1
                end
            end
        end)
        return count
    end

    -- Phase 0: Immediate Present Vacuum around start position
    -- Instantly grabs all presents near where player is standing BEFORE teleporting away to statues or Star-Mart!
    vacuumPresentsNear(startCFrame.Position, Config.CollectRadius or 45)

    -- Phase 0B: Emergency Witherrot Check before sweep starts
    if Config.AutoDispelWitherrot and (os.clock() - LastRazorbloomDispel >= 8.0) and HasRazorbloom() then
        pcall(HandleRazorbloomEmergency)
    end

    -- Phase 1: High Priority Objectives (Statues First)
    -- 1. Activate all safe non-blacklisted Statues / Altars
    if Config.SweepGetStatues then
        pcall(ActivateAvailableStatues)
    end

    -- Phase 2: Secondary Objectives (Star-Mart, Medals, Cadence Instruments)
    -- 2. Grab Star-Mart if available (+Speed, +2 Collection Range, 25s Debuff Immunity)
    if Config.SweepGetStarMart then
        pcall(CollectStarMart)
    end

    -- 3. Grab Medals if available (spawns during collapse or with Medal upgrade)
    pcall(CollectMedals)

    -- 4. Grab any active Cadence Instruments
    if Config.SweepGetCadence then
        pcall(CollectCadenceInstruments)
    end

    local hops = 0
    local startTime = os.clock()

    while ScriptRunning and hops < maxHops do
        local curChar = LocalPlayer.Character
        local curHrp = curChar and curChar:FindFirstChild("HumanoidRootPart")
        local curHum = curChar and curChar:FindFirstChild("Humanoid")
        local curDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
        if not curChar or not curHrp or not curHrp.Parent or not curHum or curHum.Health <= 0 or curDead then
            break
        end

        -- In-Sweep Continuous Witherrot Safety Interruption (Guarded by 8s debounce)
        if Config.AutoDispelWitherrot and (os.clock() - LastRazorbloomDispel >= 8.0) and HasRazorbloom() then
            pcall(HandleRazorbloomEmergency)
        end

        local myPos = curHrp.Position
        local bestGift = nil
        local bestDist = math.huge
        local bestIsGold = false

        -- Direct global nearest-neighbor pathing (automatically prioritizing gold gifts)
        for id, g in pairs(GiftClientHandler.Gifts) do
            if not g.Collected and g.Position then
                local isTripmine = g.Tripmine or g.Type == "Tripmine"
                local isSafe = (not isTripmine) or Config.TripmineDisabler
                local isAboveVoid = isGiftPositionSafe(g.Position)

                if isSafe and isAboveVoid then
                    local isGold = (g.Golden or g.Type == "GoldenGift" or g.Type == "PassageGift")
                    local dist = (g.Position - myPos).Magnitude

                    if isGold and not bestIsGold then
                        bestIsGold = true
                        bestDist = dist
                        bestGift = g
                    elseif isGold and bestIsGold then
                        if dist < bestDist then
                            bestDist = dist
                            bestGift = g
                        end
                    elseif not isGold and not bestIsGold then
                        if dist < bestDist then
                            bestDist = dist
                            bestGift = g
                        end
                    end
                end
            end
        end

        if not bestGift or not bestGift.Position then break end
        local standTargetPos = getSafeGiftStandPosition(bestGift.Position)
        if not standTargetPos then break end

        -- Teleport safely onto the grounded stand position
        pcall(function()
            if curHrp and curHrp.Parent then
                curHrp.CFrame = CFrame.new(standTargetPos)
                curHrp.AssemblyLinearVelocity = Vector3.zero
                curHrp.AssemblyAngularVelocity = Vector3.zero
            end
        end)

        pcall(function()
            bestGift:Collect(false, true, 1)
        end)

        -- Proximity cluster vacuum at this hop location (collects all gifts in this room cluster)
        vacuumPresentsNear(standTargetPos, Config.CollectRadius or 45)

        hops = hops + 1
        if hopDelay > 0 then
            task.wait(hopDelay)
        else
            RunService.Heartbeat:Wait()
        end
    end

    -- Return safely to origin if requested
    if returnOrigin then
        local curChar = LocalPlayer.Character
        local curHrp = curChar and curChar:FindFirstChild("HumanoidRootPart")
        local curHum = curChar and curChar:FindFirstChild("Humanoid")
        local curDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
        if curHrp and curHrp.Parent and curHum and curHum.Health > 0 and not curDead then
            local originPos = startCFrame.Position
            local rayParams = RaycastParams.new()
            rayParams.FilterType = Enum.RaycastFilterType.Exclude
            if curChar then rayParams.FilterDescendantsInstances = { curChar } end
            local groundRay = Workspace:Raycast(Vector3.new(originPos.X, originPos.Y + 4, originPos.Z), Vector3.new(0, -25, 0), rayParams)
            local safeOriginCFrame = startCFrame
            if not groundRay or not groundRay.Instance or not groundRay.Instance.CanCollide or not groundRay.Instance.Anchored then
                -- Start position has no floor (e.g. room collapsed while sweeping)! Fall back to Beacon safety platform!
                safeOriginCFrame = GetBeaconCFrame()
            end
            pcall(function()
                curHrp.CFrame = safeOriginCFrame
                curHrp.AssemblyLinearVelocity = Vector3.zero
                curHrp.AssemblyAngularVelocity = Vector3.zero
            end)
            vacuumPresentsNear(safeOriginCFrame.Position, Config.CollectRadius or 45)
        end
    end

    task.wait(0.15)
    local endCollected = 0
    pcall(function()
        for _, g in pairs(GiftClientHandler.Gifts) do
            if g.Collected then endCollected = endCollected + 1 end
        end
    end)
    local gained = math.max(0, endCollected - startCollected)
    local elapsed = os.clock() - startTime

    return true, {
        hops = hops,
        gained = gained,
        duration = elapsed
    }
end

-- 5B. Safe Protected Sweep Wrapper with Guaranteed State Cleanup
local function PerformPresentSweep(overrideHops, overrideDelay, overrideReturn)
    if IsSweeping then return false, "A sweep is already in progress" end
    local isDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
    if isDead then return false, "Player is dead" end

    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    local humanoid = char and char:FindFirstChild("Humanoid")
    if not hrp or not hrp.Parent or not humanoid or humanoid.Health <= 0 then
        return false, "Character not ready"
    end
    if not GiftClientHandler or not GiftClientHandler.Gifts then
        return false, "Gift handler not ready"
    end

    IsSweeping = true
    SweepStartTime = os.clock()

    local success, res1, res2 = pcall(RawPerformPresentSweep, overrideHops, overrideDelay, overrideReturn)
    IsSweeping = false
    SweepStartTime = 0

    if not success then
        return false, tostring(res1)
    end
    return res1, res2
end

local function HasSweepTargets()
    local hasTargets = false
    pcall(function()
        -- 1. Check for unactivated, non-blacklisted statues (Highest Priority)
        if Config.SweepGetStatues then
            local altars = GetAllAltars()
            local now = os.clock()
            for _, info in ipairs(altars) do
                if info.IsEnabled and info.Prompt and not IsStatueBlacklisted(info.Name) then
                    local last = LastAltarTrigger[info.Model] or 0
                    if now - last >= 3.0 then
                        hasTargets = true
                        return
                    end
                end
            end
        end

        -- 2. Check for active uncollected StarMart
        if Config.SweepGetStarMart and (os.clock() - LastStarMartTouch >= 1.5) then
            local sm = GetActiveStarMart()
            if sm then
                hasTargets = true
                return
            end
        end

        -- 3. Check for active uncollected Medals
        if not PlayerHasMedal() then
            local medals = GetAllActiveMedals()
            if medals and #medals > 0 then
                hasTargets = true
                return
            end
        end

        -- 4. Check for Cadence instruments
        if Config.SweepGetCadence then
            local orbs = GetAllCadenceOrbs()
            if orbs and #orbs > 0 then
                hasTargets = true
                return
            end
        end

        -- 5. Check for uncollected gifts
        if GiftClientHandler and GiftClientHandler.Gifts then
            local killVoid = Workspace:FindFirstChild("KillVoid")
            local killY = (killVoid and killVoid.Position.Y) or -264.5
            for _, g in pairs(GiftClientHandler.Gifts) do
                if not g.Collected and not g.Destroyed and g.Position then
                    local isTripmine = g.Tripmine or g.Type == "Tripmine"
                    local isSafe = (not isTripmine) or Config.TripmineDisabler
                    local isAboveVoid = g.Position.Y > (killY + 80)
                    if isSafe and isAboveVoid then
                        hasTargets = true
                        return
                    end
                end
            end
        end
    end)
    return hasTargets
end

-- Continuous auto-sweep background loop (All-in-One Engine with Death Auto-Recovery)
task.spawn(function()
    while ScriptRunning do
        pcall(function()
            -- Watchdog: force clear stuck sweep after 15 seconds
            if IsSweeping and (os.clock() - SweepStartTime > 15.0) then
                IsSweeping = false
                SweepStartTime = 0
            end

            -- Continuous standalone Witherrot defense (even when not sweeping, guarded by 8s debounce)
            if Config.AutoDispelWitherrot and (os.clock() - LastRazorbloomDispel >= 8.0) and HasRazorbloom() then
                pcall(HandleRazorbloomEmergency)
            end

            if Config.TeleportSweeper and not IsSweeping then
                local isRoundActive = (not InRound) or (InRound.Value == true)
                local isDead = LocalPlayer:FindFirstChild("Dead") and LocalPlayer.Dead.Value == true
                local char = LocalPlayer.Character
                local humanoid = char and char:FindFirstChild("Humanoid")
                local hrp = char and char:FindFirstChild("HumanoidRootPart")

                if isRoundActive and not isDead and humanoid and humanoid.Health > 0 and hrp and hrp.Parent then
                    if HasSweepTargets() then
                        PerformPresentSweep(Config.MaxHopsPerSweep, Config.TeleportSweepDelay, Config.ReturnToOrigin)
                    end
                end
            end
        end)
        task.wait(0.5)
    end
end)

-- ===================================================================
-- 9. AUTO-STATUE (ALTAR) & AUTO-VOTE ENGINE (With Blacklist)
-- ===================================================================
local function CheckAndUseAltars()
    if not Config.AutoStatue then return end
    local char = LocalPlayer.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    if not hrp then return end

    local myPos = hrp.Position
    local altars = GetAllAltars()
    local now = os.clock()

    for _, info in ipairs(altars) do
        if info.IsEnabled and info.Prompt then
            -- Blacklist & Safe mode check
            if not IsStatueBlacklisted(info.Name) then
                local anchor = (info.Prompt.Parent and info.Prompt.Parent:IsA("BasePart") and info.Prompt.Parent) or info.Hitbox
                if anchor then
                    local dist = (anchor.Position - myPos).Magnitude
                    if dist <= Config.StatueRadius then
                        local lastTime = LastAltarTrigger[info.Model] or 0
                        if now - lastTime >= Config.StatueCooldown then
                            LastAltarTrigger[info.Model] = now

                            -- Trigger Proximity Prompt with hold duration
                            triggerPrompt(info.Prompt)

                            task.wait(0.12)
                            local isNowActivated = false
                            pcall(function()
                                isNowActivated = (info.Model:GetAttribute("Activated") == true)
                                    or (info.Model:GetAttribute("AltarUsed") == true)
                                    or (info.Prompt and not info.Prompt.Enabled)
                            end)
                            if isNowActivated then
                                LastAltarTrigger[info.Model] = math.huge
                            end

                            task.delay(0.4, refreshAllStatueESP)
                        end
                    end
                end
            end
        end
    end
end

-- Auto-Statue Heartbeat Loop
task.spawn(function()
    while ScriptRunning do
        pcall(CheckAndUseAltars)
        task.wait(0.25)
    end
end)

-- Auto-Vote Remote Listener
if VoteRemote then
    trackConn(VoteRemote.OnClientEvent:Connect(function(action, title, canVote)
        if action == "New" or action == "CanVote" then
            if Config.AutoVoteStatue then
                task.wait(0.1)
                pcall(function()
                    VoteRemote:FireServer(true)
                end)
            end
        end
    end))
end

-- Sanity check loop to maintain outlines across map loading and purge hazards
task.spawn(function()
    while ScriptRunning do
        task.wait(2)
        pcall(refreshAllHighlights)
        pcall(refreshEnemyHighlights)
        pcall(refreshAllStatueESP)
        if Config.AutoDisableEnemies or Config.ProjectileDisabler then
            pcall(ClearAllSeamines)
            pcall(ClearAllBlackHoles)
        end
    end
end)

-- ===================================================================
-- 10. ASYNCHRONOUS AETHER LIBRARY RESOLUTION
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

    -- Online Raw Fallback for testers without UI.lua in workspace (auto-caches locally)
    local rawUrl = "https://pastebin.com/raw/ypqqtv7J"
    local getOk, rawCode = pcall(function()
        if typeof(game.HttpGet) == "function" then
            return game:HttpGet(rawUrl)
        end
        return nil
    end)
    if getOk and rawCode and #rawCode > 0 then
        pcall(function()
            if typeof(writefile) == "function" then
                writefile("UI.lua", rawCode)
            end
        end)
        local loadOk, fn = pcall(loadstring, rawCode)
        if loadOk and type(fn) == "function" then
            local runOk, res = pcall(fn)
            if runOk and res and type(res) == "table" and (type(res.Window) == "function" or type(res.CreateWindow) == "function") then
                return res
            end
        end
    end

    return (Globals and (Globals.Library or Globals.Aether or Globals.AetherUI or Globals.AetherLibrary))
        or (typeof(getgenv) == "function" and (getgenv().Library or getgenv().Aether or getgenv().AetherUI or getgenv().AetherLibrary))
        or (_G and (_G.Library or _G.Aether or _G.AetherUI or _G.AetherLibrary))
        or (shared and (shared.Library or shared.Aether or shared.AetherUI or shared.AetherLibrary))
end

local Library = ResolveLibrary(3.5)
local Window = nil

-- ===================================================================
-- 11. INITIALIZE AETHER INTERFACE)
-- ===================================================================
local function CreateWindow()
    if not Library then
        Library = ResolveLibrary(3.5)
        if not Library then return nil end
    end
    local ok, created = pcall(Library.Window, Library, {
        Title = "Nullscape",
        Desc = "Aether Studios",
        Theme = "Halloween",
        Config = {
            Keybind = Enum.KeyCode.LeftControl,
            Size = UDim2.new(0, 840, 0, 480)
        }
    })
    if ok and created then return created end

    local ok2, fallback = pcall(Library.Window, Library, { Theme = "Default" })
    if ok2 and fallback then return fallback end
    return nil
end

Window = CreateWindow()
if not Window then
    warn("[Nullscape] Failed to create Aether Window.")
    return
end

-- ===================================================================
-- 12. STREAMLINED 4-TAB HIERARCHY WITH SUBTABS
-- ===================================================================
local DashboardTab  = Window:Tab({ Title = "Overview", Icon = "activity" })
local AutomationTab = Window:Tab({ Title = "Auto-Farm", Icon = "gift" })
local CharacterTab  = Window:Tab({ Title = "Player", Icon = "shield" })
local VisualsTab    = Window:Tab({ Title = "Visuals", Icon = "eye" })

-- -------------------------------------------------------------------
-- TAB 1: HOME (Stats & Quick Toggles)
-- -------------------------------------------------------------------
local DashStatsSec = DashboardTab:Section({ Title = "GAME STATS" })

local StatLevel     = DashStatsSec:Label({ Title = "Current Level", Desc = "Level 1" })
local StatGifts     = DashStatsSec:Label({ Title = "Presents Collected", Desc = "0 / 0" })
local StatRemaining = DashStatsSec:Label({ Title = "Presents Left", Desc = "0" })
local StatGold      = DashStatsSec:Label({ Title = "Gold Presents", Desc = "0.00" })
local StatEnemies   = DashStatsSec:Label({ Title = "Nearby Monsters", Desc = "None lurking" })
local StatAltar     = DashStatsSec:Label({ Title = "Nearest Statue", Desc = "Scanning..." })



-- Real-time Dashboard Updates
local lastDashCache = { Level = "", Gifts = "", Remaining = "", Gold = "", Enemies = "", Altar = "" }

task.spawn(function()
    while ScriptRunning do
        pcall(function()
            local gc = GiftCounters
            local regCol = gc and gc:FindFirstChild("Gift") and gc.Gift:GetAttribute("Collected") or 0
            local regMax = gc and gc:FindFirstChild("Gift") and gc.Gift:GetAttribute("MaxGifts") or 0
            local goldCol = gc and gc:FindFirstChild("GoldenGift") and gc.GoldenGift:GetAttribute("Collected") or 0
            local goldMax = gc and gc:FindFirstChild("GoldenGift") and gc.GoldenGift:GetAttribute("MaxGifts") or 0
            local passCol = gc and gc:FindFirstChild("PassageGift") and gc.PassageGift:GetAttribute("Collected") or 0
            local passMax = gc and gc:FindFirstChild("PassageGift") and gc.PassageGift:GetAttribute("MaxGifts") or 0
            local totalConfirmed = regCol + goldCol + passCol
            local totalMax = regMax + goldMax + passMax
            local lvl = LevelVal and LevelVal.Value or 1
            local remaining = math.max(0, totalMax - totalConfirmed)
            local goldBank = GoldenGiftsVal and GoldenGiftsVal.Value or 0

            local newLvl = string.format("Level %s", lvl)
            local newGifts = string.format("%d / %d Verified", totalConfirmed, totalMax)
            local newRemaining = string.format("%d presents to collect", remaining)
            local newGold = string.format("%.2f Gold Gifts", goldBank)

            local enemyCount = (EnemiesFolder and #EnemiesFolder:GetChildren()) or 0
            local newEnemies = enemyCount > 0 and string.format("%d monsters detected!", enemyCount) or "No monsters detected"

            local altars = GetAllAltars()
            local char = LocalPlayer.Character
            local hrp = char and char:FindFirstChild("HumanoidRootPart")
            local newAltar = "No statues in loaded rooms"

            if hrp and #altars > 0 then
                local nearest = nil
                local minDist = math.huge
                for _, info in ipairs(altars) do
                    local pos = info.Hitbox and info.Hitbox.Position or (info.Prompt and info.Prompt.Parent and info.Prompt.Parent:IsA("BasePart") and info.Prompt.Parent.Position)
                    if pos then
                        local d = (pos - hrp.Position).Magnitude
                        if d < minDist then
                            minDist = d
                            nearest = info
                        end
                    end
                end
                if nearest then
                    local isBl = IsStatueBlacklisted(nearest.Name)
                    local status = isBl and "Blacklisted" or (nearest.IsEnabled and "Ready" or (nearest.IsActivated and "Activated" or "Inactive"))
                    newAltar = string.format("%s (%.0fm) [%s]", nearest.Name, minDist, status)
                end
            end

            if lastDashCache.Level ~= newLvl then
                lastDashCache.Level = newLvl
                StatLevel:SetDesc(newLvl)
            end
            if lastDashCache.Gifts ~= newGifts then
                lastDashCache.Gifts = newGifts
                StatGifts:SetDesc(newGifts)
            end
            if lastDashCache.Remaining ~= newRemaining then
                lastDashCache.Remaining = newRemaining
                StatRemaining:SetDesc(newRemaining)
            end
            if lastDashCache.Gold ~= newGold then
                lastDashCache.Gold = newGold
                StatGold:SetDesc(newGold)
            end
            if lastDashCache.Enemies ~= newEnemies then
                lastDashCache.Enemies = newEnemies
                StatEnemies:SetDesc(newEnemies)
            end
            if lastDashCache.Altar ~= newAltar then
                lastDashCache.Altar = newAltar
                StatAltar:SetDesc(newAltar)
            end
            if Config.EnemyDamageImmunity then
                pcall(ensureClientIFrames)
                pcall(protectPlayerCharacter, LocalPlayer.Character)
            end
        end)
        task.wait(0.5)
    end
end)

-- -------------------------------------------------------------------
-- TAB 2: AUTOMATION (SubTabs: Presents & Statues)
-- -------------------------------------------------------------------
do
local SubPresents = AutomationTab:SubTab({ Title = "Presents" })
local SubStatues  = AutomationTab:SubTab({ Title = "Statues" })

-- --- SUBTAB: PRESENTS ---
local CollectSec = SubPresents:Section({ Title = "AUTO-PICKUP" })

UIControls.AutoCollect = CollectSec:Toggle({
    Title = "Pickup Nearby Presents",
    Desc = "Automatically grab presents near you (avoids tripmines)",
    Value = Config.AutoCollect,
    Callback = function(val)
        Config.AutoCollect = val
        debouncedSaveGameConfig()
    end
})

UIControls.CollectRadius = CollectSec:Slider({
    Title = "Pickup Range (Studs)",
    Desc = "Distance to pick up presents (45 max safe server limit)",
    Min = 15,
    Max = 45,
    Value = math.clamp(Config.CollectRadius or 35, 15, 45),
    Rounding = 0,
    Callback = function(val)
        Config.CollectRadius = val
        debouncedSaveGameConfig()
    end
})

UIControls.CollectBatchSize = CollectSec:Slider({
    Title = "Presents Per Pulse",
    Desc = "Number of gifts grabbed at once",
    Min = 1,
    Max = 15,
    Value = Config.CollectBatchSize,
    Rounding = 0,
    Callback = function(val)
        Config.CollectBatchSize = val
        debouncedSaveGameConfig()
    end
})

UIControls.CollectInterval = CollectSec:Slider({
    Title = "Pickup Delay (Seconds)",
    Desc = "Time between pickups (e.g. 0.1s, 0.2s)",
    Min = 0.05,
    Max = 1.0,
    Value = Config.CollectInterval,
    Rounding = 2,
    Callback = function(val)
        Config.CollectInterval = val
        debouncedSaveGameConfig()
    end
})

local SafeSec = SubPresents:Section({ Title = "TRIPMINE SAFETY" })
SafeSec:Label({
    Title = "Tripmine Safety",
    Desc = "ACTIVE - Tripmines are ignored so you never blow up"
})

local TeleportSec = SubPresents:Section({ Title = "TELEPORT FARM" })

UIControls.TeleportSweeper = TeleportSec:Toggle({
    Title = "Auto-Farm Loop (Teleport)",
    Desc = "Continuously teleports to presents, medals, statues, Star-Mart & instruments",
    Value = Config.TeleportSweeper,
    Callback = function(val)
        Config.TeleportSweeper = val
        debouncedSaveGameConfig()
    end
})

UIControls.TeleportSweepDelay = TeleportSec:Slider({
    Title = "Teleport Delay (Seconds)",
    Desc = "Time between each teleport (0.01s = fast, 0.05s = smooth)",
    Min = 0.01,
    Max = 0.20,
    Value = Config.TeleportSweepDelay,
    Rounding = 2,
    Callback = function(val)
        Config.TeleportSweepDelay = val
        debouncedSaveGameConfig()
    end
})

UIControls.MaxHopsPerSweep = TeleportSec:Slider({
    Title = "Max Teleports Per Run",
    Desc = "Maximum teleports in a single farm run",
    Min = 10,
    Max = 150,
    Value = Config.MaxHopsPerSweep,
    Rounding = 0,
    Callback = function(val)
        Config.MaxHopsPerSweep = val
        debouncedSaveGameConfig()
    end
})

UIControls.ReturnToOrigin = TeleportSec:Toggle({
    Title = "Return to Start After Farm",
    Desc = "Brings you back to where you started so you don't fall",
    Value = Config.ReturnToOrigin,
    Callback = function(val)
        Config.ReturnToOrigin = val
        debouncedSaveGameConfig()
    end
})

UIControls.AutoDispelWitherrot = TeleportSec:Toggle({
    Title = "Auto-Scare Bird (Jump Pad)",
    Desc = "Bounces on Jump Pad only when Razorbloom attacks your head",
    Value = Config.AutoDispelWitherrot,
    Callback = function(val)
        Config.AutoDispelWitherrot = val
        debouncedSaveGameConfig()
    end
})

-- --- SUBTAB: STATUES ---
local StatueAutoSec = SubStatues:Section({ Title = "AUTO-ACTIVATE STATUES" })

UIControls.AutoStatue = StatueAutoSec:Toggle({
    Title = "Auto-Activate Statues",
    Desc = "Automatically use nearby safe statues",
    Value = Config.AutoStatue,
    Callback = function(val)
        Config.AutoStatue = val
        debouncedSaveGameConfig()
    end
})

UIControls.AutoVoteStatue = StatueAutoSec:Toggle({
    Title = "Auto-Vote YES",
    Desc = "Automatically vote YES whenever a statue vote starts",
    Value = Config.AutoVoteStatue,
    Callback = function(val)
        Config.AutoVoteStatue = val
        debouncedSaveGameConfig()
    end
})

UIControls.StatueRadius = StatueAutoSec:Slider({
    Title = "Activation Range (Studs)",
    Desc = "Distance to statue before activating",
    Min = 10,
    Max = 80,
    Value = Config.StatueRadius,
    Rounding = 0,
    Callback = function(val)
        Config.StatueRadius = val
        debouncedSaveGameConfig()
    end
})

UIControls.StatueCooldown = StatueAutoSec:Slider({
    Title = "Statue Cooldown (Seconds)",
    Desc = "Wait time before using the same statue again",
    Min = 0.5,
    Max = 5.0,
    Value = Config.StatueCooldown,
    Rounding = 1,
    Callback = function(val)
        Config.StatueCooldown = val
        debouncedSaveGameConfig()
    end
})

-- WORKING MULTI-SELECT DROPDOWN MENU FOR STATUE BLACKLIST
local StatueBlacklistSec = SubStatues:Section({ Title = "STATUE BLACKLIST" })

local canonicalStatueNames = {
    "Chaos",
    "Curses",
    "Purgatory",
    "Delusion",
    "Chance",
    "Protection",
    "Echo",
    "Purification",
    "Wishes",
    "Passage",
    "Perfection",
    "Reroll",
    "Duality"
}

local initialBl = {}
for name, bl in pairs(Config.BlacklistedStatues) do
    if bl == true then table.insert(initialBl, name) end
end

local StatueBlacklistDropdown = StatueBlacklistSec:Dropdown({
    Title = "Ignored Statues",
    Desc = "Select statues to NEVER activate (Multi-select)",
    List = canonicalStatueNames,
    Values = canonicalStatueNames,
    Value = initialBl,
    Multi = true,
    Callback = function(selectedList)
        table.clear(Config.BlacklistedStatues)
        if type(selectedList) == "table" then
            for _, name in ipairs(selectedList) do
                Config.BlacklistedStatues[name] = true
            end
        end
        refreshAllStatueESP()
        debouncedSaveGameConfig()
    end
})
UIControls.BlacklistedStatues = StatueBlacklistDropdown
StatueBlacklistDropdown:SetValue(initialBl, true)

StatueBlacklistSec:Button({
    Title = "Ignore Bad Statues (Chaos & Curses)",
    Desc = "Blocks harmful statues that ruin runs (Recommended)",
    Callback = function()
        StatueBlacklistDropdown:SetValue({ "Chaos", "Curses" })
        Window:Notify({ Title = "Blacklist Updated", Desc = "Blacklisted: Chaos, Curses", Time = 3 })
    end
})

StatueBlacklistSec:Button({
    Title = "Allow All Statues",
    Desc = "Permits activating every statue",
    Callback = function()
        StatueBlacklistDropdown:SetValue({})
        Window:Notify({ Title = "Blacklist Cleared", Desc = "Allowed all statues.", Time = 3 })
    end
})

local StatueActionSec = SubStatues:Section({ Title = "INSTANT ACTIONS" })

StatueActionSec:Button({
    Title = "Use Closest Statue",
    Desc = "Instantly uses the nearest safe statue",
    Callback = function()
        local char = LocalPlayer.Character
        local hrp = char and char:FindFirstChild("HumanoidRootPart")
        if not hrp then return end

        local altars = GetAllAltars()
        local nearest = nil
        local minDist = math.huge

        for _, info in ipairs(altars) do
            if info.IsEnabled and info.Prompt then
                local pos = info.Hitbox and info.Hitbox.Position or (info.Prompt.Parent and info.Prompt.Parent:IsA("BasePart") and info.Prompt.Parent.Position)
                if pos then
                    local d = (pos - hrp.Position).Magnitude
                    if d < minDist then
                        minDist = d
                        nearest = info
                    end
                end
            end
        end

        if nearest and nearest.Prompt then
            triggerPrompt(nearest.Prompt)
            Window:Notify({
                Title = "Statue Triggered",
                Desc = string.format("Triggered %s (%.0f studs away)", nearest.Name, minDist),
                Time = 3
            })
            task.delay(0.5, refreshAllStatueESP)
        else
            Window:Notify({
                Title = "No Statues Ready",
                Desc = "No unactivated statues found nearby.",
                Time = 3
            })
        end
    end
})

StatueActionSec:Button({
    Title = "Teleport to Closest Statue",
    Desc = "Teleports right in front of the nearest active statue",
    Callback = function()
        local char = LocalPlayer.Character
        local hrp = char and char:FindFirstChild("HumanoidRootPart")
        if not hrp then return end

        local altars = GetAllAltars()
        local nearest = nil
        local minDist = math.huge

        for _, info in ipairs(altars) do
            if info.IsEnabled and info.Hitbox then
                local d = (info.Hitbox.Position - hrp.Position).Magnitude
                if d < minDist then
                    minDist = d
                    nearest = info
                end
            end
        end

        if nearest then
            local anchor = (nearest.Prompt and nearest.Prompt.Parent and nearest.Prompt.Parent:IsA("BasePart") and nearest.Prompt.Parent) or nearest.Hitbox
            if anchor then
                local standPos, targetLook = getBestStandPosition(anchor, nearest.Model)
                if standPos then
                    hrp.CFrame = CFrame.lookAt(standPos, Vector3.new(targetLook.X, standPos.Y, targetLook.Z))
                    hrp.AssemblyLinearVelocity = Vector3.zero
                    hrp.AssemblyAngularVelocity = Vector3.zero
                    Window:Notify({
                        Title = "Teleported",
                        Desc = string.format("Teleported to %s (%.0f studs)", nearest.Name, minDist),
                        Time = 3
                    })
                else
                    Window:Notify({
                        Title = "Teleport Aborted",
                        Desc = string.format("%s Statue is over the void or in a collapsed room!", nearest.Name),
                        Time = 3.5
                    })
                end
            end
        else
            Window:Notify({
                Title = "Teleport Failed",
                Desc = "No active unconsumed statues found.",
                Time = 3
            })
        end
    end
})

local StatueTrackerSec = SubStatues:Section({ Title = "STATUE TRACKER" })
local StatTrackerLabel = StatueTrackerSec:Label({
    Title = "Nearby Statues",
    Desc = "Scanning rooms..."
})

task.spawn(function()
    while ScriptRunning do
        pcall(function()
            local altars = GetAllAltars()
            local char = LocalPlayer.Character
            local hrp = char and char:FindFirstChild("HumanoidRootPart")
            if #altars == 0 then
                StatTrackerLabel:SetDesc("No statues detected in active rooms.")
            else
                local lines = {}
                for _, info in ipairs(altars) do
                    local dist = 0
                    if hrp and info.Hitbox then
                        dist = math.floor((info.Hitbox.Position - hrp.Position).Magnitude)
                    end
                    local isBl = IsStatueBlacklisted(info.Name)
                    local status = isBl and "Blacklisted" or (info.IsEnabled and "Ready" or (info.IsActivated and "Activated" or "Disabled"))
                    table.insert(lines, string.format("• %s (%dm) [%s]", info.Name, dist, status))
                end
                StatTrackerLabel:SetDesc(table.concat(lines, "  |  "))
            end
        end)
        task.wait(1.0)
    end
end)
end

-- -------------------------------------------------------------------
-- TAB 3: PLAYER (SubTabs: Movement, Defense, Upgrades)
-- -------------------------------------------------------------------
do
local SubMovement = CharacterTab:SubTab({ Title = "Movement" })
local SubThreats  = CharacterTab:SubTab({ Title = "Defense" })
local SubUpgrades = CharacterTab:SubTab({ Title = "Upgrades" })

-- --- SUBTAB: MOVEMENT ---
local MoveModSec = SubMovement:Section({ Title = "ABILITIES & SPEED" })

UIControls.GliderInfCharge = MoveModSec:Toggle({
    Title = "Infinite Glider",
    Desc = "Fly forever without losing height or falling",
    Value = Config.GliderInfCharge,
    Callback = function(val)
        Config.GliderInfCharge = val
        debouncedSaveGameConfig()
    end
})

UIControls.GrapplerInfCharge = MoveModSec:Toggle({
    Title = "Infinite Grapple Shots",
    Desc = "Never run out of grapple shots",
    Value = Config.GrapplerInfCharge,
    Callback = function(val)
        Config.GrapplerInfCharge = val
        debouncedSaveGameConfig()
    end
})

UIControls.GrapplerNoCD = MoveModSec:Toggle({
    Title = "Grapple No Cooldown",
    Desc = "Grapple continuously with no delay",
    Value = Config.GrapplerNoCD,
    Callback = function(val)
        Config.GrapplerNoCD = val
        debouncedSaveGameConfig()
    end
})

UIControls.GrappleRange = MoveModSec:Slider({
    Title = "Grapple Reach (Studs)",
    Desc = "How far your grapple shoots (90 = normal, 300+ = long)",
    Min = 90,
    Max = 600,
    Value = Config.GrappleRange,
    Rounding = 0,
    Callback = function(val)
        Config.GrappleRange = val
        debouncedSaveGameConfig()
    end
})

UIControls.ChargerNoCD = MoveModSec:Toggle({
    Title = "Charge No Cooldown",
    Desc = "Charge rush continuously with zero wait",
    Value = Config.ChargerNoCD,
    Callback = function(val)
        Config.ChargerNoCD = val
        debouncedSaveGameConfig()
    end
})

UIControls.DiverNoCD = MoveModSec:Toggle({
    Title = "Dive No Cooldown",
    Desc = "Slide-dive continuously without delay",
    Value = Config.DiverNoCD,
    Callback = function(val)
        Config.DiverNoCD = val
        debouncedSaveGameConfig()
    end
})

UIControls.DiverFreeMovement = MoveModSec:Toggle({
    Title = "Free Dive Steering",
    Desc = "Steer your dive freely using your camera",
    Value = Config.DiverFreeMovement,
    Callback = function(val)
        Config.DiverFreeMovement = val
        debouncedSaveGameConfig()
    end
})

UIControls.NoDiveBonk = MoveModSec:Toggle({
    Title = "No Wall Crash Stun",
    Desc = "Never get stunned when diving into walls",
    Value = Config.NoDiveBonk,
    Callback = function(val)
        Config.NoDiveBonk = val
        debouncedSaveGameConfig()
    end
})

UIControls.SpiritInfCharge = MoveModSec:Toggle({
    Title = "Infinite Spirit Stamina",
    Desc = "Never run out of stamina in Spirit mode",
    Value = Config.SpiritInfCharge,
    Callback = function(val)
        Config.SpiritInfCharge = val
        debouncedSaveGameConfig()
    end
})

UIControls.SpeedMultiplier = MoveModSec:Slider({
    Title = "Speed Multiplier",
    Desc = "Multiplies walk and sprint speed",
    Min = 1.0,
    Max = 3.0,
    Value = Config.SpeedMultiplier,
    Rounding = 1,
    Callback = function(val)
        Config.SpeedMultiplier = val
        debouncedSaveGameConfig()
    end
})

UIControls.BypassMovementCurses = MoveModSec:Toggle({
    Title = "Ignore Slowdown Curses",
    Desc = "Prevents curses and debuffs from slowing you down",
    Value = Config.BypassMovementCurses,
    Callback = function(val)
        Config.BypassMovementCurses = val
        debouncedSaveGameConfig()
    end
})

local ClassChangeSec = SubMovement:Section({ Title = "CHANGE CLASS" })
local ClassDropdown = ClassChangeSec:Dropdown({
    Title = "Select Class",
    Desc = "Choose a movement class to play as",
    List = {"Charger", "Diver", "Grappler", "Glider", "Spirit", "Phoon", "Wanted", "Wicked"},
    Values = {"Charger", "Diver", "Grappler", "Glider", "Spirit", "Phoon", "Wanted", "Wicked"},
    Value = Config.SelectedClass,
    Callback = function(val)
        Config.SelectedClass = val
        debouncedSaveGameConfig()
    end
})
UIControls.SelectedClass = ClassDropdown

ClassChangeSec:Button({
    Title = "Equip Class Now",
    Desc = "Instantly switch to selected class",
    Callback = function()
        local success = SetClass(Config.SelectedClass)
        if success then
            Window:Notify({ Title = "Class Switched", Desc = "Equipped class: " .. Config.SelectedClass, Time = 3 })
        else
            Window:Notify({ Title = "Error", Desc = "Failed to switch class. In-game replica not ready.", Time = 3 })
        end
    end
})

-- --- SUBTAB: DEFENSE ---
local ThreatHazardsSec = SubThreats:Section({ Title = "TRIPMINES & PROJECTILES" })

UIControls.TripmineDisabler = ThreatHazardsSec:Toggle({
    Title = "Disable Tripmines",
    Desc = "Walk over tripmines safely without dying",
    Value = Config.TripmineDisabler,
    Callback = function(val)
        Config.TripmineDisabler = val
        debouncedSaveGameConfig()
    end
})

UIControls.ProjectileDisabler = ThreatHazardsSec:Toggle({
    Title = "Block Enemy Attacks & Black Holes",
    Desc = "Blocks enemy fireballs, spit, and deadly black hole attacks",
    Value = Config.ProjectileDisabler,
    Callback = function(val)
        Config.ProjectileDisabler = val
        if val then
            pcall(ClearAllBlackHoles)
        end
        debouncedSaveGameConfig()
    end
})

local ThreatImmunitySec = SubThreats:Section({ Title = "ENEMY DAMAGE IMMUNITY (INVULNERABILITY)" })

UIControls.EnemyDamageImmunity = ThreatImmunitySec:Toggle({
    Title = "Enemy Damage Immunity",
    Desc = "Prevents selected monsters from damaging or killing you while keeping them visible",
    Value = Config.EnemyDamageImmunity,
    Callback = function(val)
        Config.EnemyDamageImmunity = val
        pcall(pacifyAllEnemies)
        debouncedSaveGameConfig()
    end
})

local initialImmunityEnemies = {}
if type(Config.SelectedImmunityEnemies) == "table" then
    for name, sel in pairs(Config.SelectedImmunityEnemies) do
        if sel then table.insert(initialImmunityEnemies, name) end
    end
    table.sort(initialImmunityEnemies, function(a, b) return string.lower(a) < string.lower(b) end)
end

UIControls.SelectedImmunityEnemies = ThreatImmunitySec:Dropdown({
    Title = "Select Invulnerable Enemies",
    Desc = "Choose which enemies cannot hurt you (uncheck to take damage)",
    List = canonicalEnemyNames,
    Values = canonicalEnemyNames,
    Value = initialImmunityEnemies,
    Multi = true,
    Callback = function(val)
        if type(val) == "table" then
            table.clear(Config.SelectedImmunityEnemies)
            for k, v in pairs(val) do
                if type(k) == "string" and (v == true or v == 1) then
                    Config.SelectedImmunityEnemies[k] = true
                elseif type(v) == "string" then
                    Config.SelectedImmunityEnemies[v] = true
                end
            end
        end
        pcall(pacifyAllEnemies)
        debouncedSaveGameConfig()
    end
})

ThreatImmunitySec:Button({
    Title = "Select All (Immune to All)",
    Desc = "Makes you completely invulnerable to all monster attacks",
    Callback = function()
        for _, name in ipairs(canonicalEnemyNames) do
            Config.SelectedImmunityEnemies[name] = true
        end
        if UIControls.SelectedImmunityEnemies and typeof(UIControls.SelectedImmunityEnemies.SetValue) == "function" then
            pcall(function() UIControls.SelectedImmunityEnemies:SetValue(canonicalEnemyNames) end)
        end
        pcall(pacifyAllEnemies)
        debouncedSaveGameConfig()
        if Window then
            Window:Notify({
                Title = "Immunity Enabled",
                Desc = "You are now invulnerable to all monster damage.",
                Time = 2.5
            })
        end
    end
})

ThreatImmunitySec:Button({
    Title = "Deselect All (Vulnerable to All)",
    Desc = "Allows all monster attacks to harm you normally",
    Callback = function()
        table.clear(Config.SelectedImmunityEnemies)
        if UIControls.SelectedImmunityEnemies and typeof(UIControls.SelectedImmunityEnemies.SetValue) == "function" then
            pcall(function() UIControls.SelectedImmunityEnemies:SetValue({}) end)
        end
        pcall(pacifyAllEnemies)
        debouncedSaveGameConfig()
        if Window then
            Window:Notify({
                Title = "Immunity Cleared",
                Desc = "You are now vulnerable to all monster damage.",
                Time = 2.5
            })
        end
    end
})

local ThreatDeletionSec = SubThreats:Section({ Title = "ENEMY DELETION (WORKSPACE REMOVAL)" })

UIControls.AutoDisableEnemies = ThreatDeletionSec:Toggle({
    Title = "Auto-Delete Selected Enemies",
    Desc = "Deletes selected enemy types from workspace when they spawn",
    Value = Config.AutoDisableEnemies,
    Callback = function(val)
        Config.AutoDisableEnemies = val
        if val then
            pcall(function()
                local ef = Workspace:FindFirstChild("Enemies")
                if ef then
                    for _, ch in ipairs(ef:GetChildren()) do
                        RemoveEnemy(ch)
                    end
                end
                local sw = Workspace:FindFirstChild("Skinwalkers")
                if sw then
                    for _, ch in ipairs(sw:GetChildren()) do
                        RemoveEnemy(ch)
                    end
                end
            end)
            pcall(ClearAllSeamines)
            pcall(ClearAllBlackHoles)
        else
            if Config.EnemyDamageImmunity then
                pcall(pacifyAllEnemies)
            end
        end
        debouncedSaveGameConfig()
    end
})

local initialSelectedEnemies = {}
if type(Config.SelectedEnemiesToDelete) == "table" then
    for name, sel in pairs(Config.SelectedEnemiesToDelete) do
        if sel then table.insert(initialSelectedEnemies, name) end
    end
    table.sort(initialSelectedEnemies, function(a, b) return string.lower(a) < string.lower(b) end)
end

UIControls.SelectedEnemiesToDelete = ThreatDeletionSec:Dropdown({
    Title = "Select Enemies to Delete",
    Desc = "Choose which enemy types to delete from workspace",
    List = canonicalEnemyNames,
    Values = canonicalEnemyNames,
    Value = initialSelectedEnemies,
    Multi = true,
    Callback = function(val)
        if type(val) == "table" then
            table.clear(Config.SelectedEnemiesToDelete)
            for k, v in pairs(val) do
                if type(k) == "string" and (v == true or v == 1) then
                    Config.SelectedEnemiesToDelete[k] = true
                elseif type(v) == "string" then
                    Config.SelectedEnemiesToDelete[v] = true
                end
            end
        end
        -- Only delete enemies if AutoDisableEnemies toggle is actively enabled!
        if Config.AutoDisableEnemies then
            pcall(function()
                local ef = Workspace:FindFirstChild("Enemies")
                if ef then
                    for _, ch in ipairs(ef:GetChildren()) do
                        RemoveEnemy(ch)
                    end
                end
                local sw = Workspace:FindFirstChild("Skinwalkers")
                if sw then
                    for _, ch in ipairs(sw:GetChildren()) do
                        RemoveEnemy(ch)
                    end
                end
            end)
        end
        debouncedSaveGameConfig()
    end
})

local confirmSelectAllUntil = 0
local btnSelectAll = nil
btnSelectAll = ThreatDeletionSec:Button({
    Title = "Select All Enemies (Delete All)",
    Desc = "Selects all enemies to be deleted",
    Callback = function()
        local now = os.clock()
        if now > confirmSelectAllUntil then
            confirmSelectAllUntil = now + 3.0
            pcall(function()
                btnSelectAll:SetTitle("Click Again to Confirm (Delete All)")
                btnSelectAll:SetDesc("Press again within 3s to delete all enemies!")
            end)
            if Window then
                Window:Notify({
                    Title = "Confirm Selection",
                    Desc = "Click again within 3s to confirm selecting all enemies for deletion.",
                    Time = 3.0
                })
            end
            task.delay(3.0, function()
                if os.clock() >= confirmSelectAllUntil then
                    pcall(function()
                        btnSelectAll:SetTitle("Select All Enemies (Delete All)")
                        btnSelectAll:SetDesc("Selects all enemies to be deleted")
                    end)
                end
            end)
            return
        end

        confirmSelectAllUntil = 0
        pcall(function()
            btnSelectAll:SetTitle("Select All Enemies (Delete All)")
            btnSelectAll:SetDesc("Selects all enemies to be deleted")
        end)

        for _, name in ipairs(canonicalEnemyNames) do
            Config.SelectedEnemiesToDelete[name] = true
        end
        if UIControls.SelectedEnemiesToDelete and typeof(UIControls.SelectedEnemiesToDelete.SetValue) == "function" then
            pcall(function() UIControls.SelectedEnemiesToDelete:SetValue(canonicalEnemyNames) end)
        end
        if Config.AutoDisableEnemies then
            pcall(function()
                local ef = Workspace:FindFirstChild("Enemies")
                if ef then
                    for _, ch in ipairs(ef:GetChildren()) do
                        RemoveEnemy(ch)
                    end
                end
                local sw = Workspace:FindFirstChild("Skinwalkers")
                if sw then
                    for _, ch in ipairs(sw:GetChildren()) do
                        RemoveEnemy(ch)
                    end
                end
            end)
        end
        debouncedSaveGameConfig()
        if Window then
            Window:Notify({
                Title = "All Enemies Selected",
                Desc = "All enemies selected for deletion.",
                Time = 2.5
            })
        end
    end
})

ThreatDeletionSec:Button({
    Title = "Deselect All Enemies (Keep All)",
    Desc = "Deselects all enemies so they stay in workspace",
    Callback = function()
        table.clear(Config.SelectedEnemiesToDelete)
        if UIControls.SelectedEnemiesToDelete and typeof(UIControls.SelectedEnemiesToDelete.SetValue) == "function" then
            pcall(function() UIControls.SelectedEnemiesToDelete:SetValue({}) end)
        end
        if Config.EnemyDamageImmunity then
            pcall(pacifyAllEnemies)
        end
        debouncedSaveGameConfig()
    end
})

local ThreatSafetySec = SubThreats:Section({ Title = "HAZARD & BEACON SAFETY" })

UIControls.AntiVoid = ThreatSafetySec:Toggle({
    Title = "Anti-Void (Beacon Return)",
    Desc = "Teleports you back to the Beacon if you fall deep into the void abyss",
    Value = Config.AntiVoid,
    Callback = function(val)
        Config.AntiVoid = val
        if typeof(updateCharacterVoidScript) == "function" then
            pcall(updateCharacterVoidScript)
        end
        debouncedSaveGameConfig()
    end
})

UIControls.AntiVoidBuffer = ThreatSafetySec:Slider({
    Title = "Anti-Void Trigger Distance",
    Desc = "Distance above the game kill plane (-610.5) to catch you (default 140 studs = Y -470.5)",
    Min = 80,
    Max = 250,
    Value = Config.AntiVoidBuffer,
    Rounding = 0,
    Callback = function(val)
        Config.AntiVoidBuffer = val
        debouncedSaveGameConfig()
    end
})

UIControls.GoldGatedBeacon = ThreatSafetySec:Toggle({
    Title = "Gold-Gated Beacon Entry",
    Desc = "Blocks touching Beacon until all presents are server-confirmed (protects money)",
    Value = Config.GoldGatedBeacon,
    Callback = function(val)
        Config.GoldGatedBeacon = val
        pcall(updateBeaconTouchState)
        debouncedSaveGameConfig()
    end
})

-- --- SUBTAB: UPGRADES ---
local UpgradeSpooferSec = SubUpgrades:Section({ Title = "FREE UPGRADES" })

UIControls.SpoofUpgrades = UpgradeSpooferSec:Toggle({
    Title = "Unlock Upgrades",
    Desc = "Enables perks and upgrades without needing to buy them",
    Value = Config.SpoofUpgrades,
    Callback = function(val)
        Config.SpoofUpgrades = val
        debouncedSaveGameConfig()
    end
})

UIControls.DefaultUpgradeStack = UpgradeSpooferSec:Slider({
    Title = "Upgrade Power / Stacks",
    Desc = "Multiplier for strength of your unlocked upgrades",
    Min = 1,
    Max = 10,
    Value = Config.DefaultUpgradeStack,
    Rounding = 0,
    Callback = function(val)
        Config.DefaultUpgradeStack = val
        for k in pairs(Config.UpgradeStacks) do
            Config.UpgradeStacks[k] = val
        end
        debouncedSaveGameConfig()
    end
})

UpgradeSpooferSec:Button({
    Title = "Speed & Mobility Pack",
    Desc = "Enables Blossom, Double Jump, Sport Shoes, Swiftness Ring, Ice Skates & Gift Magnet",
    Callback = function()
        Config.SpoofUpgrades = true
        if UIControls.SpoofUpgrades then pcall(function() UIControls.SpoofUpgrades:SetValue(true, true) end) end
        local pack = { "DoubleJump", "RealWings", "SportShoes", "SwiftnessRing", "IceSkates", "GiftMagnet" }
        for _, k in ipairs(pack) do
            Config.ActiveUpgrades[k] = true
            local ctrl = UIControls["Upgrade_" .. k]
            if ctrl and typeof(ctrl.SetValue) == "function" then
                pcall(function() ctrl:SetValue(true, true) end)
            end
        end
        debouncedSaveGameConfig()
        Window:Notify({ Title = "Mobility Pack Enabled", Desc = "Equipped Blossom, Double Jump, Sport Shoes, Swiftness Ring, Ice Skates & Gift Magnet!", Time = 3 })
    end
})

UpgradeSpooferSec:Button({
    Title = "God Mode Defense Pack",
    Desc = "Enables Matrix Tetrahedron, Helmet, Shield, Miniature Hourglass & Subspacial Barrier",
    Callback = function()
        Config.SpoofUpgrades = true
        if UIControls.SpoofUpgrades then pcall(function() UIControls.SpoofUpgrades:SetValue(true, true) end) end
        local pack = { "MatrixTetrahedron", "Helmet", "Shield", "MiniatureHourglass", "SubspacialBarrier" }
        for _, k in ipairs(pack) do
            Config.ActiveUpgrades[k] = true
            local ctrl = UIControls["Upgrade_" .. k]
            if ctrl and typeof(ctrl.SetValue) == "function" then
                pcall(function() ctrl:SetValue(true, true) end)
            end
        end
        debouncedSaveGameConfig()
        Window:Notify({ Title = "Defense Pack Enabled", Desc = "Equipped Matrix Tetrahedron, Helmet, Shield, Hourglass & Subspacial Barrier!", Time = 3 })
    end
})

local IndivUpgradesSec = SubUpgrades:Section({ Title = "SINGLE UPGRADES" })
local upgradeList = {
    { Key = "Adrenaline", Name = "Adrenaline" },
    { Key = "AdvancedGravityCoil", Name = "Advanced Gravity Coil" },
    { Key = "RealWings", Name = "Blossom" },
    { Key = "DefuseKit", Name = "Defuse Kit" },
    { Key = "DoubleJump", Name = "Double Jump" },
    { Key = "GiftIdol", Name = "Gift Idol" },
    { Key = "GiftMagnet", Name = "Gift Magnet" },
    { Key = "GraceWings", Name = "Grace Wings" },
    { Key = "Helmet", Name = "Helmet" },
    { Key = "IceSkates", Name = "Ice Skates" },
    { Key = "JetBoosters", Name = "Jet Boosters" },
    { Key = "MatrixTetrahedron", Name = "Matrix Tetrahedron" },
    { Key = "MiniatureHourglass", Name = "Miniature Hourglass" },
    { Key = "NinjaBelt", Name = "Ninja Belt" },
    { Key = "TheOrb", Name = "Orb" },
    { Key = "PropellerHat", Name = "Propeller Hat" },
    { Key = "SharkTail", Name = "Shark Tail" },
    { Key = "Shield", Name = "Shield" },
    { Key = "SportShoes", Name = "Sport Shoes" },
    { Key = "SpringShoes", Name = "Spring Shoes" },
    { Key = "StarMarts", Name = "Star Marts" },
    { Key = "SubspacialBarrier", Name = "Subspacial Barrier" },
    { Key = "SwiftnessRing", Name = "Swiftness Ring" }
}

for _, upg in ipairs(upgradeList) do
    UIControls["Upgrade_" .. upg.Key] = IndivUpgradesSec:Toggle({
        Title = upg.Name,
        Desc = "Toggle perk: " .. upg.Name,
        Value = Config.ActiveUpgrades[upg.Key] or false,
        Callback = function(val)
            Config.ActiveUpgrades[upg.Key] = val
            debouncedSaveGameConfig()
        end
    })
end
end

-- -------------------------------------------------------------------
-- TAB 4: VISUALS
-- -------------------------------------------------------------------
do
local SubEsp = VisualsTab

-- --- SUBTAB: ESP ---
local EspSec = SubEsp:Section({ Title = "PRESENTS ESP" })

UIControls.PresentsESP = EspSec:Toggle({
    Title = "See Presents (ESP)",
    Desc = "Highlight all presents through walls",
    Value = Config.PresentsESP,
    Callback = function(val)
        Config.PresentsESP = val
        for hl in pairs(ActiveHighlights) do
            if hl and hl.Parent then hl.Enabled = val end
        end
        for hl in pairs(ActiveMedalHighlights) do
            if hl and hl.Parent then hl.Enabled = val end
        end
        for bb in pairs(ActiveMedalBillboards) do
            if bb and bb.Parent then bb.Enabled = val end
        end
        debouncedSaveGameConfig()
    end
})

UIControls.FillTransparency = EspSec:Slider({
    Title = "Body Glow",
    Desc = "1 = Outline only, 0.5 = Glowing body",
    Min = 0,
    Max = 1,
    Value = Config.FillTransparency,
    Rounding = 2,
    Callback = function(val)
        Config.FillTransparency = val
        for hl in pairs(ActiveHighlights) do
            if hl and hl.Parent then hl.FillTransparency = val end
        end
        debouncedSaveGameConfig()
    end
})

UIControls.OutlineTransparency = EspSec:Slider({
    Title = "Outline Brightness",
    Desc = "0 = Solid bright outline, 0.5 = Soft glow",
    Min = 0,
    Max = 1,
    Value = Config.OutlineTransparency,
    Rounding = 2,
    Callback = function(val)
        Config.OutlineTransparency = val
        for hl in pairs(ActiveHighlights) do
            if hl and hl.Parent then hl.OutlineTransparency = val end
        end
        debouncedSaveGameConfig()
    end
})

local StatueEspSec = SubEsp:Section({ Title = "STATUES ESP" })
UIControls.StatueESP = StatueEspSec:Toggle({
    Title = "See Statues (ESP)",
    Desc = "Show color outlines and distance tags above statues",
    Value = Config.StatueESP,
    Callback = function(val)
        Config.StatueESP = val
        for hl in pairs(ActiveStatueHighlights) do
            if hl and hl.Parent then hl.Enabled = val end
        end
        for bb in pairs(ActiveStatueBillboards) do
            if bb and bb.Parent then bb.Enabled = val end
        end
        debouncedSaveGameConfig()
    end
})

local CadenceEspSec = SubEsp:Section({ Title = "INSTRUMENTS ESP" })
UIControls.CadenceESP = CadenceEspSec:Toggle({
    Title = "See Cadence Instruments (ESP)",
    Desc = "Highlights active violins and harps in neon pink",
    Value = Config.CadenceESP,
    Callback = function(val)
        Config.CadenceESP = val
        for hl in pairs(ActiveCadenceHighlights) do
            if hl and hl.Parent then hl.Enabled = val end
        end
        for bb in pairs(ActiveCadenceBillboards) do
            if bb and bb.Parent then bb.Enabled = val end
        end
        debouncedSaveGameConfig()
    end
})

CadenceEspSec:Button({
    Title = "Teleport to Closest Instrument",
    Desc = "Instantly teleports to the nearest active instrument",
    Callback = function()
        local ok, msg = TeleportToNearestCadenceInstrument()
        if Window then
            Window:Notify({
                Title = ok and "Teleported!" or "Notice",
                Desc = msg,
                Time = 3.5
            })
        end
    end
})

local EnemyEspSec = SubEsp:Section({ Title = "MONSTERS ESP" })
UIControls.EnemyESP = EnemyEspSec:Toggle({
    Title = "See Monsters (ESP)",
    Desc = "Highlight all lurking monsters in bright orange",
    Value = Config.EnemyESP,
    Callback = function(val)
        Config.EnemyESP = val
        for hl in pairs(EnemyHighlights) do
            if hl and hl.Parent then hl.Enabled = val end
        end
        debouncedSaveGameConfig()
    end
})

local PylonEspSec = SubEsp:Section({ Title = "CELESTIAL PYLONS ESP" })
UIControls.PylonESP = PylonEspSec:Toggle({
    Title = "See Celestial Pylons (ESP)",
    Desc = "Highlights pylons and displays 3D charge progress & distance",
    Value = Config.PylonESP,
    Callback = function(val)
        Config.PylonESP = val
        refreshAllPylonESP()
        debouncedSaveGameConfig()
    end
})

UIControls.PylonShowCompleted = PylonEspSec:Toggle({
    Title = "Show Completed Pylons",
    Desc = "Keep showing pylons in green after reaching 100% charge",
    Value = Config.PylonShowCompleted,
    Callback = function(val)
        Config.PylonShowCompleted = val
        refreshAllPylonESP()
        debouncedSaveGameConfig()
    end
})

PylonEspSec:Button({
    Title = "Teleport to Nearest Pylon",
    Desc = "Instantly teleports to the nearest uncompleted pylon platform",
    Callback = function()
        local ok, msg = TeleportToNearestPylon()
        if Window then
            Window:Notify({
                Title = ok and "Teleported to Pylon!" or "Notice",
                Desc = msg,
                Time = 3.5
            })
        end
    end
})

local LegendSec = SubEsp:Section({ Title = "ESP COLORS GUIDE" })
LegendSec:Label({ Title = "Regular Presents", Desc = "Cyan Outline" })
LegendSec:Label({ Title = "Golden Presents", Desc = "Gold Outline" })
LegendSec:Label({ Title = "Passage Presents", Desc = "Purple Outline" })
LegendSec:Label({ Title = "Tripmines", Desc = "Red Warning Outline" })
LegendSec:Label({ Title = "Statues", Desc = "Altar Colors (Red = Ignored)" })
LegendSec:Label({ Title = "Cadence Instruments", Desc = "Neon Pink [COLLECT OR DIE]" })
LegendSec:Label({ Title = "Celestial Pylons", Desc = "Magenta (Unactivated) / Gold (Charging) / Green (Done)" })
end



-- ===================================================================
-- 13. NON-DESTRUCTIVE UNLOAD HANDLER
-- ===================================================================
_G.__Nullscape_Cleanup = function()
    ScriptRunning = false

    -- Disconnect event connections
    for _, conn in ipairs(Connections) do
        pcall(function() conn:Disconnect() end)
    end
    table.clear(Connections)

    -- Restore function and metamethod hooks
    for name, data in pairs(OrigHooks) do
        pcall(function()
            if data.method == "__namecall" and typeof(hookmetamethod) == "function" then
                hookmetamethod(data.target, "__namecall", data.orig)
            elseif data.target and data.method and data.orig then
                hookfunction(data.target[data.method], data.orig)
            end
        end)
    end
    table.clear(OrigHooks)

    -- Restore grappler environment
    if oldGrapplerEnv and grapplerMod and grapplerMod.Cast then
        pcall(function()
            setfenv(grapplerMod.Cast, oldGrapplerEnv)
        end)
    end

    -- Remove highlights & ESP
    for hl in pairs(ActiveHighlights) do
        pcall(function() hl:Destroy() end)
    end
    table.clear(ActiveHighlights)

    for hl in pairs(EnemyHighlights) do
        pcall(function() hl:Destroy() end)
    end
    table.clear(EnemyHighlights)

    for hl in pairs(ActiveStatueHighlights) do
        pcall(function() hl:Destroy() end)
    end
    table.clear(ActiveStatueHighlights)

    for bb in pairs(ActiveStatueBillboards) do
        pcall(function() bb:Destroy() end)
    end
    table.clear(ActiveStatueBillboards)

    for hl in pairs(ActiveCadenceHighlights) do
        pcall(function() hl:Destroy() end)
    end
    table.clear(ActiveCadenceHighlights)

    for bb in pairs(ActiveCadenceBillboards) do
        pcall(function() bb:Destroy() end)
    end
    table.clear(ActiveCadenceBillboards)

    for hl in pairs(ActiveMedalHighlights) do
        pcall(function() hl:Destroy() end)
    end
    table.clear(ActiveMedalHighlights)

    for bb in pairs(ActiveMedalBillboards) do
        pcall(function() bb:Destroy() end)
    end
    table.clear(ActiveMedalBillboards)
    table.clear(KnownMedals)

    for hl in pairs(ActivePylonHighlights) do
        pcall(function() hl:Destroy() end)
    end
    table.clear(ActivePylonHighlights)

    for bb in pairs(ActivePylonBillboards) do
        pcall(function() bb:Destroy() end)
    end
    table.clear(ActivePylonBillboards)

    if cadenceAuraOrb then
        pcall(function() cadenceAuraOrb:Destroy() end)
        cadenceAuraOrb = nil
    end

    pcall(function()
        local beacon = Workspace:FindFirstChild("Beacon")
        if beacon then
            if beacon:IsA("BasePart") then
                beacon.CanTouch = true
            end
            for _, ch in ipairs(beacon:GetDescendants()) do
                if ch:IsA("BasePart") then
                    ch.CanTouch = true
                end
            end
        end
    end)

    -- Restore character hitbox and PlayerHitboxCache
    pcall(function()
        local char = LocalPlayer and LocalPlayer.Character
        local hitbox = char and char:FindFirstChild("Hitbox")
        if hitbox and hitbox:IsA("BasePart") then
            hitbox.CanTouch = true
            hitbox.CanCollide = false
            hitbox.CanQuery = true
            hitbox.Size = Vector3.new(2, 2, 1)
        end
        local hitCache = require(game.ReplicatedFirst.ClientModules.ProjectileClient.ProjectileClientHandler.PlayerHitboxCache)
        if hitCache and hitCache.Hitboxes and hitbox then
            hitCache.Hitboxes[LocalPlayer] = hitbox
        end
    end)

    -- Restore IFRAMES spoofing and enemy collisions
    pcall(function()
        if LocalPlayer then
            local iframe = LocalPlayer:FindFirstChild("IFRAMES")
            if iframe and iframe:IsA("Folder") then iframe:Destroy() end
            local trueIframe = LocalPlayer:FindFirstChild("TRUEIFRAMES")
            if trueIframe and trueIframe:IsA("Folder") then trueIframe:Destroy() end
        end
    end)
    pcall(function()
        local ef = Workspace:FindFirstChild("Enemies")
        if ef then
            for _, ch in ipairs(ef:GetChildren()) do
                for _, desc in ipairs(ch:GetDescendants()) do
                    if desc:IsA("BasePart") then
                        local origSize = desc:GetAttribute("PacifiedOrigSize")
                        if origSize then desc.Size = origSize end
                        local origCanCollide = desc:GetAttribute("PacifiedOrigCanCollide")
                        local lower = string.lower(desc.Name)
                        if origCanCollide == true and not (lower == "icbm" or lower == "voidboundicbm" or lower == "indicator" or lower == "ride" or lower == "collide" or lower == "collide2" or lower == "demonshockwave" or lower == "firearea" or string.find(lower, "mark") ~= nil) then
                            desc.CanCollide = true
                        else
                            desc.CanCollide = false
                        end
                        desc.CanTouch = true
                        desc.CanQuery = true
                    end
                end
                if ch:IsA("BasePart") then
                    ch.CanCollide = false
                    ch.CanTouch = true
                    ch.CanQuery = true
                end
            end
        end
        local sw = Workspace:FindFirstChild("Skinwalkers")
        if sw then
            for _, ch in ipairs(sw:GetChildren()) do
                for _, desc in ipairs(ch:GetDescendants()) do
                    if desc:IsA("BasePart") then
                        local origSize = desc:GetAttribute("PacifiedOrigSize")
                        if origSize then desc.Size = origSize end
                        desc.CanTouch = true
                        desc.CanCollide = false
                        desc.CanQuery = true
                    end
                end
                if ch:IsA("BasePart") then
                    ch.CanTouch = true
                    ch.CanCollide = false
                    ch.CanQuery = true
                end
            end
        end
    end)

    -- Restore character void death script
    pcall(function()
        local char = LocalPlayer and LocalPlayer.Character
        local vScript = char and char:FindFirstChild("Void")
        if vScript and vScript:IsA("LocalScript") then
            vScript.Enabled = true
        end
    end)

    _G.PICKUP_BONUS = 0
    table.clear(GiftTracker.Pending)
    table.clear(GiftTracker.Failed)
    table.clear(GiftTracker.Confirmed)
    IsSweeping = false
    Config.TeleportSweeper = false
    Config.AntiVoid = false
    _G.__Nullscape_Cleanup = nil
end

-- Periodic Auto-Save Engine (every 12 seconds)
task.spawn(function()
    while ScriptRunning do
        task.wait(12)
        if Config.AutoSaveConfig and not isAutoLoading then
            pcall(saveGameConfig)
        end
    end
end)

-- Ensure default configuration exists on disk if first execution
task.defer(function()
    task.wait(0.25)
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if hasFS then
        local fp = getGameConfigPath()
        local exists = false
        pcall(function()
            if type(isfile) == "function" and isfile(fp) then exists = true end
        end)
        if not exists then
            saveGameConfig()
        end
    end
end)
