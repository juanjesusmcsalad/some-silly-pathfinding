if not game:IsLoaded() then
    pcall(function() game.Loaded:Wait() end)
end

local Players = game:GetService("Players")
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local RunService = game:GetService("RunService")
local UserInputService = game:GetService("UserInputService")
local VirtualUser = game:GetService("VirtualUser")
local TweenService = game:GetService("TweenService")
local HttpService = game:GetService("HttpService")

local LocalPlayer = Players.LocalPlayer
if not LocalPlayer then
    pcall(function()
        LocalPlayer = Players.PlayerAdded:Wait()
    end)
    LocalPlayer = LocalPlayer or Players.LocalPlayer
end
local PlayerGui = (LocalPlayer and LocalPlayer:WaitForChild("PlayerGui", 8)) or (LocalPlayer and LocalPlayer:FindFirstChildOfClass("PlayerGui"))

-- ===================================================================
-- 1. CLEANUP PREVIOUS INSTANCES
-- ===================================================================
if _G.__PeelThePotato_Cleanup then
    pcall(_G.__PeelThePotato_Cleanup)
end

local ScriptRunning = true
local Connections = {}
local function trackConn(conn)
    table.insert(Connections, conn)
    return conn
end
local clickScreenOnce = nil

-- Clean previous UI instances without destroying the Core ScreenGui container
local searchParents = {}
if typeof(gethui) == "function" then
    local h = gethui()
    if h then table.insert(searchParents, h) end
end
pcall(function()
    local cg = game:GetService("CoreGui")
    if cg and typeof(cg.GetChildren) == "function" then
        local dummyChildren = cg:GetChildren()
        if not table.find(searchParents, cg) then
            table.insert(searchParents, cg)
        end
    end
end)
if PlayerGui and not table.find(searchParents, PlayerGui) then
    table.insert(searchParents, PlayerGui)
end

for _, parent in ipairs(searchParents) do
    if parent then
        for _, ch in ipairs(parent:GetChildren()) do
            if ch.Name == "PeelThePotato_UI" or ch.Name == "Aether_MobileToggle" then
                pcall(function() ch:Destroy() end)
            elseif ch.Name == "Aether" or ch:GetAttribute("AetherUI") then
                -- Do NOT destroy the Core ScreenGui container; only remove previous window frames (Shadow) if re-executing
                for _, inner in ipairs(ch:GetChildren()) do
                    if inner.Name == "Shadow" or inner.Name == "DropdownSelect" then
                        pcall(function() inner:Destroy() end)
                    end
                end
            end
        end
    end
end

-- Remove previous ESP
local oldESP = workspace:FindFirstChild("KeyPotato_ESP_Highlight")
if oldESP then pcall(function() oldESP:Destroy() end) end

-- ===================================================================
-- 2. LOAD AETHER UI LIBRARY
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

-- fall back URL
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

-- Frame rate safety: Do NOT uncap FPS to 0 (per GEMINI.md guidelines to protect mobile tick-rate)

-- ===================================================================
-- 3. GAME RESOURCES, MODULES & PACKETS
-- ===================================================================
local function SafeRequire(...)
    local current = ReplicatedStorage
    for _, name in ipairs({...}) do
        if not current then return nil end
        current = current:FindFirstChild(name) or (current.WaitForChild and current:WaitForChild(name, 3))
    end
    if current and current:IsA("ModuleScript") then
        local ok, res = pcall(require, current)
        if ok then return res end
    end
    return nil
end

local Packets = SafeRequire("Modules", "Resources", "Packets")
local PlayerStateShared = SafeRequire("Shared", "PlayerStateShared")
local PotatoTableShared = SafeRequire("Shared", "PotatoTableShared")
local PotatoPileShared = SafeRequire("Shared", "PotatoPileShared")
local CarryUpgradeShared = SafeRequire("Shared", "CarryUpgradeShared")
local PeelToolShared = SafeRequire("Shared", "PeelToolShared")
local ToolUpgradeShared = SafeRequire("Shared", "ToolUpgradeShared")
local StoreShared = SafeRequire("Shared", "StoreShared")
local TutorialStackShared = SafeRequire("Shared", "TutorialStackShared")
local LobbyUpgradeShared = SafeRequire("Shared", "LobbyUpgradeShared")
local ChapterShared = SafeRequire("Shared", "ChapterShared")
local QueueShared = SafeRequire("Shared", "QueueShared")
local IntroShared = SafeRequire("Shared", "IntroShared")
local Prices = SafeRequire("Modules", "Resources", "Prices")
local StoreItemShared = SafeRequire("Shared", "StoreItemShared")
local StoreViewController = SafeRequire("Controllers", "StoreViewController")
local CutToolShared = SafeRequire("Shared", "CutToolShared")
local FireShared = SafeRequire("Shared", "FireShared")
local PackToolShared = SafeRequire("Shared", "PackToolShared")
local DeliveryShared = SafeRequire("Shared", "DeliveryShared")

local OrigCutTools = {}
pcall(function()
    if CutToolShared and CutToolShared.Tools then
        for _, tool in ipairs(CutToolShared.Tools) do
            OrigCutTools[tool.Id] = {
                Instant = tool.Instant,
                SlicesPerCut = tool.SlicesPerCut,
                MoveSpeed = tool.MoveSpeed,
                PotatoCount = tool.PotatoCount
            }
            tool.Instant = true
            tool.SlicesPerCut = 25
            tool.MoveSpeed = 60
            tool.PotatoCount = 3
        end
    end
end)

local PeelingController = SafeRequire("Controllers", "PeelingController")
local CuttingController = SafeRequire("Controllers", "CuttingController")
local PackingController = SafeRequire("Controllers", "PackingController")
local PalletController = SafeRequire("Controllers", "PalletController")
local DeliveryController = SafeRequire("Controllers", "DeliveryController")
local TruckController = SafeRequire("Controllers", "TruckController")
local Chapter3OutroController = SafeRequire("Controllers", "Chapter3OutroController")
local PotatoPileController = SafeRequire("Controllers", "PotatoPileController")
local ToolEquipController = SafeRequire("Controllers", "ToolEquipController")
local TutorialShared = SafeRequire("Shared", "TutorialShared")
local ToolUpgradeController = SafeRequire("Controllers", "ToolUpgradeController")
local UISpotlightController = SafeRequire("Controllers", "UISpotlightController")
local NotificationController = SafeRequire("Controllers", "NotificationController")
local BagController = SafeRequire("Controllers", "BagController")
local DeviceController = SafeRequire("Controllers", "DeviceController")
pcall(function()
    if DeviceController and typeof(DeviceController.UpdateDevice) == "function" then
        DeviceController.UpdateDevice()
    end
end)

local LastBagFullTriggerTime = 0
local BagFullReported = false

pcall(function()
    if NotificationController then
        if typeof(NotificationController.CreateError) == "function" then
            local oldCreateError = NotificationController.CreateError
            NotificationController.CreateError = function(msg, ...)
                if type(msg) == "string" and msg:lower():find("bag is full") then
                    BagFullReported = true
                    LastBagFullTriggerTime = os.clock()
                end
                return oldCreateError(msg, ...)
            end
        end
        if typeof(NotificationController.CreateNotification) == "function" then
            local oldCreateNotification = NotificationController.CreateNotification
            NotificationController.CreateNotification = function(info, ...)
                if type(info) == "table" and type(info.Text) == "string" and info.Text:lower():find("bag is full") then
                    BagFullReported = true
                    LastBagFullTriggerTime = os.clock()
                end
                return oldCreateNotification(info, ...)
            end
        end
    end
end)

pcall(function()
    if BagController and typeof(BagController.ShowFull) == "function" then
        local oldShowFull = BagController.ShowFull
        BagController.ShowFull = function(...)
            BagFullReported = true
            LastBagFullTriggerTime = os.clock()
            return oldShowFull(...)
        end
    end
end)

pcall(function()
    if Packets and Packets.NotificationCreated then
        Packets.NotificationCreated.OnClientEvent:Connect(function(info)
            if type(info) == "table" and type(info.Text) == "string" and info.Text:lower():find("bag is full") then
                BagFullReported = true
                LastBagFullTriggerTime = os.clock()
            elseif type(info) == "string" and info:lower():find("bag is full") then
                BagFullReported = true
                LastBagFullTriggerTime = os.clock()
            end
        end)
    end
end)

local STANDARD_GAME_GUIS = {
    "Bag", "Hotbar", "Cash", "Diamonds", "Pumpkins", "Timer",
    "UpgradesButton", "Crosshair", "SoldPopup",
    "QuestCard", "Tutorial", "EscapeGuide", "FriendBoost"
}
local STANDARD_GAME_GUIS_MAP = {}
for _, name in ipairs(STANDARD_GAME_GUIS) do STANDARD_GAME_GUIS_MAP[name] = true end

-- Hook DetachScreenGuis immediately at startup so game controllers NEVER un-parent gameplay GUIs to nil
pcall(function()
    local DetachScreenGuis = SafeRequire("Modules", "Helper", "DetachScreenGuis")
    if DetachScreenGuis and typeof(hookfunction) == "function" and not _G.__DetachHooked then
        _G.__DetachHooked = true
        hookfunction(DetachScreenGuis, function(p1)
            local hidden = {}
            local pGui = LocalPlayer and (LocalPlayer:FindFirstChild("PlayerGui") or LocalPlayer:FindFirstChildOfClass("PlayerGui"))
            if pGui then
                for _, name in ipairs(p1 or {}) do
                    local g = pGui:FindFirstChild(name)
                    if g and g:IsA("ScreenGui") and g.Enabled then
                        g.Enabled = false
                        table.insert(hidden, g)
                    end
                end
            end
            return function()
                for _, g in ipairs(hidden) do
                    pcall(function()
                        if pGui and g.Parent ~= pGui then g.Parent = pGui end
                        g.Enabled = true
                    end)
                end
            end
        end)
    end
end)

-- Universal Game GUI Restorer & Watchdog
-- Prevents DetachScreenGuis or minigame cleanup failures from stranding player HUD elements with Parent = nil
local function restoreGameGuis()
    local pGui = LocalPlayer and (LocalPlayer:FindFirstChild("PlayerGui") or LocalPlayer:FindFirstChildOfClass("PlayerGui"))
    if not pGui then return end

    -- Do not force restore while actively steering/driving in the Delivery minigame
    if DeliveryController and typeof(DeliveryController.IsActive) == "function" and DeliveryController.IsActive() then
        return
    end

    local nilInsts = (typeof(getnilinstances) == "function" and getnilinstances()) or {}

    -- 1. Resolve and anchor the authoritative Bag ScreenGui from BagController
    local realBag = nil
    pcall(function()
        local counter = BagController and BagController.GetCounter and BagController.GetCounter()
        realBag = counter and counter:FindFirstAncestorOfClass("ScreenGui")
    end)
    if realBag then
        if realBag.Parent ~= pGui then
            realBag.Parent = pGui
        end
        if not realBag.Enabled and not (PeelingController and PeelingController.IsPeeling) then
            realBag.Enabled = true
        end
    end

    -- 2. Resolve and anchor the authoritative Hotbar ScreenGui (the one with built slots)
    local realHotbar = nil
    pcall(function()
        for _, ch in ipairs(pGui:GetChildren()) do
            if ch.Name == "Hotbar" and ch:IsA("ScreenGui") and ch:FindFirstChild("Slots") and #ch.Slots:GetChildren() > 1 then
                realHotbar = ch
                break
            end
        end
    end)
    if not realHotbar and typeof(getnilinstances) == "function" then
        pcall(function()
            for _, inst in ipairs(getnilinstances()) do
                pcall(function()
                    if inst:IsA("ScreenGui") and inst.Name == "Hotbar" then
                        local slots = inst:FindFirstChild("Slots")
                        if slots and #slots:GetChildren() > 1 then
                            realHotbar = inst
                        end
                    end
                end)
                if realHotbar then break end
            end
        end)
    end
    if realHotbar then
        pcall(function()
            if realHotbar.Parent ~= pGui then
                realHotbar.Parent = pGui
            end
            if not realHotbar.Enabled and not (PeelingController and PeelingController.IsPeeling) then
                realHotbar.Enabled = true
            end
        end)
    end

    -- 3. Clean any duplicate copies of ALL standard gameplay GUIs
    local seen = {}
    local toDestroy = {}
    pcall(function()
        for _, ch in ipairs(pGui:GetChildren()) do
            pcall(function()
                if ch:IsA("ScreenGui") and STANDARD_GAME_GUIS_MAP[ch.Name] then
                    if seen[ch.Name] then
                        if ch.Name == "Bag" and realBag and ch == realBag then
                            table.insert(toDestroy, seen[ch.Name])
                            seen[ch.Name] = ch
                        elseif ch.Name == "Hotbar" and realHotbar and ch == realHotbar then
                            table.insert(toDestroy, seen[ch.Name])
                            seen[ch.Name] = ch
                        else
                            table.insert(toDestroy, ch)
                        end
                    else
                        seen[ch.Name] = ch
                    end
                end
            end)
        end
        for _, ch in ipairs(toDestroy) do
            pcall(function() ch:Destroy() end)
        end
    end)

    -- 4. Re-parent other standard gameplay HUD elements from nil instances if missing
    pcall(function()
        if typeof(getnilinstances) == "function" then
            for _, name in ipairs(STANDARD_GAME_GUIS) do
                if name ~= "Bag" and name ~= "Hotbar" and not pGui:FindFirstChild(name) then
                    for _, inst in ipairs(getnilinstances()) do
                        local match = false
                        pcall(function()
                            if inst:IsA("ScreenGui") and inst.Name == name then
                                inst.Parent = pGui
                                inst.Enabled = true
                                match = true
                            end
                        end)
                        if match then break end
                    end
                end
            end
        end
    end)

    -- 5. Fallback: If any simple standard GUI is still missing, clone from ReplicatedStorage.Gui template (Bag and Hotbar must never be blindly cloned)
    pcall(function()
        local rsGui = ReplicatedStorage:FindFirstChild("Gui")
        if rsGui then
            for _, name in ipairs(STANDARD_GAME_GUIS) do
                if name ~= "Bag" and name ~= "Hotbar" and not pGui:FindFirstChild(name) then
                    local template = rsGui:FindFirstChild(name)
                    if template then
                        local c = template:Clone()
                        c.Parent = pGui
                        c.Enabled = true
                    end
                end
            end
        end
    end)

    -- 6. Ensure all standard gameplay HUD elements in PlayerGui remain enabled (unless minigame is actively hiding them)
    if not (PeelingController and PeelingController.IsPeeling) then
        for _, name in ipairs(STANDARD_GAME_GUIS) do
            local g = pGui:FindFirstChild(name)
            if g and g:IsA("ScreenGui") and not g.Enabled then
                g.Enabled = true
            end
        end
    end

    -- 7. Ensure mobile-only GUIs (ToolUseButton) are strictly hidden on Computer/PC
    pcall(function()
        local isMobile = DeviceController and typeof(DeviceController.IsOnMobile) == "function" and DeviceController.IsOnMobile()
        if not isMobile then
            local tub = pGui:FindFirstChild("ToolUseButton")
            if tub and tub:IsA("ScreenGui") and tub.Enabled then
                tub.Enabled = false
            end
        end
    end)

    -- 8. If StoreView popup is stuck open ("CLICK ANYWHERE TO CLOSE"), dismiss it so game GUIs return
    pcall(function()
        local sv = pGui:FindFirstChild("StoreView")
        if sv and sv.Enabled and typeof(clickScreenOnce) == "function" then
            clickScreenOnce(0.05)
        end
    end)
end

-- UI Protection Shield: Prevents PeelingController & DeliveryController from ever hiding or detaching Aether or gameplay GUIs
local function protectAndRestoreGuis()
    restoreGameGuis()

    -- Auto-dismiss StoreView preview popup whenever it appears so gameplay HUD is restored
    pcall(function()
        local pGui = LocalPlayer and (LocalPlayer:FindFirstChild("PlayerGui") or LocalPlayer:FindFirstChildOfClass("PlayerGui"))
        local sv = pGui and pGui:WaitForChild("StoreView", 3)
        if sv then
            trackConn(sv:GetPropertyChangedSignal("Enabled"):Connect(function()
                if sv.Enabled then
                    task.delay(0.35, function()
                        if sv and sv.Enabled and typeof(clickScreenOnce) == "function" then
                            clickScreenOnce(0.05)
                        end
                    end)
                end
            end))
        end
    end)

    if not PeelingController then return end
    pcall(function()
        if typeof(getupvalues) ~= "function" then return end
        local ups = getupvalues(PeelingController.StartPeeling)
        local hideOtherGuis = ups and ups[13]
        local hideUps = hideOtherGuis and getupvalues(hideOtherGuis)
        local u2 = hideUps and hideUps[3]
        if u2 and type(u2) == "table" then
            local toWhitelist = {
                "Aether", "Aether Studios", "Core", "PeelThePotato_UI",
                "Bag", "Hotbar", "UpgradesButton", "ExpressivePromptsGui",
                "QuestCard", "Stats", "Diamonds", "Pumpkins", "Delivery"
            }
            for _, name in ipairs(toWhitelist) do
                if not table.find(u2, name) then
                    table.insert(u2, name)
                end
            end
        end
    end)
    
    pcall(function()
        if typeof(getupvalues) ~= "function" then return end
        local ups = getupvalues(PeelingController.StopPeeling)
        local showOtherGuis = ups and ups[9]
        if typeof(showOtherGuis) == "function" and typeof(PeelingController.IsPeeling) == "function" and PeelingController.IsPeeling() then
            showOtherGuis()
        end
        local showUps = showOtherGuis and getupvalues(showOtherGuis)
        local u30 = showUps and showUps[2]
        if u30 and type(u30) == "table" then
            for _, g in ipairs(u30) do
                if g and g.Parent == nil then
                    pcall(function() g.Parent = PlayerGui end)
                end
            end
            table.clear(u30)
        end
    end)
    
    -- Safe shield for TableBeaconController: intercepts internal Roblox engine parent-locked errors
    pcall(function()
        local TableBeaconController = require(ReplicatedStorage.Controllers.TableBeaconController)
        local ups = (typeof(getupvalues) == "function" and getupvalues(TableBeaconController.Start))
        local updateFn = ups and ups[8]
        if updateFn then
            if typeof(hookfunction) == "function" then
                local orig = updateFn
                hookfunction(updateFn, function(...)
                    pcall(orig, ...)
                end)
            elseif typeof(setupvalue) == "function" then
                local orig = updateFn
                setupvalue(TableBeaconController.Start, 8, function(...)
                    pcall(orig, ...)
                end)
            end
        end
    end)
end

protectAndRestoreGuis()

-- Extract PeelingController internal peeling function
local function getPeelingInternals()
    if not PeelingController then return { controller = nil } end
    local ok, ups = pcall(function()
        if typeof(getupvalues) == "function" then
            return getupvalues(PeelingController.Init)
        end
        return nil
    end)
    if not ok or not ups then return { controller = PeelingController } end
    local onPeelPressed = ups[2]
    if not onPeelPressed then return { controller = PeelingController } end
    local ok2, peelUps = pcall(function()
        return getupvalues(onPeelPressed)
    end)
    if not ok2 or not peelUps then return { controller = PeelingController } end
    local tryPeel = nil
    for _, up in ipairs(peelUps) do
        if type(up) == "function" then
            local tu = getupvalues(up)
            if tu and #tu >= 7 and typeof(tu[1]) == "Instance" then
                tryPeel = up
                break
            end
        end
    end
    tryPeel = tryPeel or peelUps[6] or peelUps[4]
    if not tryPeel then return { controller = PeelingController } end
    local ok3, tryUps = pcall(function()
        return (typeof(debug.getupvalues) == "function" and debug.getupvalues(tryPeel)) or getupvalues(tryPeel)
    end)
    if not ok3 or not tryUps then return { controller = PeelingController } end
    return {
        controller = PeelingController,
        getPotato = function() return tryUps[1] end,
        performPeel = tryUps[8]
    }
end

-- Extract PotatoPileController internal upvalues with memoization
local cachedTryPickUps = nil
local function getTryPickUpvalues()
    if cachedTryPickUps then return cachedTryPickUps end
    if not PotatoPileController then return nil end
    local ok, ups = pcall(function()
        if typeof(getupvalues) == "function" then
            return getupvalues(PotatoPileController.SetUseHeld)
        end
        return nil
    end)
    if not ok or not ups or not ups[3] then return nil end
    local ok2, tryUps = pcall(function()
        return (typeof(debug.getupvalues) == "function" and debug.getupvalues(ups[3])) or getupvalues(ups[3])
    end)
    if ok2 and tryUps then
        cachedTryPickUps = tryUps
        return cachedTryPickUps
    end
    return nil
end

-- Extract PotatoPileController internal potato map with structure validation
local function getPotatoPileMap()
    local tryUps = getTryPickUpvalues()
    if not tryUps then return {} end
    if typeof(tryUps[11]) == "table" then
        return tryUps[11]
    end
    for _, up in ipairs(tryUps) do
        if typeof(up) == "table" then
            for k, _ in pairs(up) do
                if typeof(k) == "Instance" and k:IsA("BasePart") then
                    return up
                end
                break
            end
        end
    end
    return {}
end

-- Extract PotatoPileController internal pile data (Seed, KeyPotatoId) with structure validation
local function getPileData()
    local tryUps = getTryPickUpvalues()
    if not tryUps then return nil end
    if typeof(tryUps[13]) == "table" and tryUps[13].PotatoSlots then
        return tryUps[13]
    end
    for _, up in ipairs(tryUps) do
        if typeof(up) == "table" and (up.PotatoSlots or up.Seed or up.KeyPotatoId) then
            return up
        end
    end
    return tryUps[13]
end

-- ===================================================================
-- 4. CONFIGURATION & STATE
-- ===================================================================
local Config = {
    -- Auto Farm
    MasterFarm = false,
    AutoCompleteTutorial = false,
    TutorialStepDelay = 1.25, -- Delay between tutorial button actions to allow animations and spotlight to settle
    MovementMode = "Teleport", -- "Teleport" or "Smooth"
    AutoPeelAtTable = false,
    SliceDelay = 0.01,
    AutoSell = true, -- Always enabled internally in the auto-farm loop
    
    -- Standalone Peeler
    StandalonePeeler = false,
    AutoStartTablePrompt = false,
    AutoLeaveStationOnEmpty = false,
    
    -- Machine Automation & Chapter 2 Cooling (Always Enabled Internally)
    AutoLoadHandMachine = true,
    AutoCrankHandMachine = true,
    AutoLoadAutoMachine = true,
    AutoLoadConveyor = true,
    AutoCoolFire = true,
    
    -- Chapter 3 Delivery & Pallets
    AutoLoadTruckPallet = true,
    AutoDeliverTruck = true,
    
    -- Key & Escape
    AutoGrabAndEscape = true, -- Auto enabled by default
    
    -- Lobby & Auto-Requeue
    AutoRequeue = false,
    AutoLobbyUpgrades = true,
    PreferredChapter = "Auto", -- "Auto", "Chapter 3", "Chapter 2", "Chapter 1"
    RequeuePartySize = 1,
    SkipCutscenes = true,
    
    -- Auto Upgrades & Store (Single Unified Toggle)
    AutoBuyAllUpgrades = false,
    PrioritizeHighRarity = true,
    
    -- Rewards (All auto-enabled by default)
    AutoClaimDaily = true,
    AutoClaimFreeCrates = true,
    AutoClaimBattlePass = true,
    AutoClaimAchievements = true,
    AutoClaimUpdateGift = true,
    AutoCollectPumpkins = false,
    AutoCollectDiamonds = false,
    
    -- Player & QoL
    WalkSpeedEnabled = false,
    WalkSpeed = 24,
    JumpPowerEnabled = false,
    JumpPower = 75,
    InfiniteJump = false,
    Noclip = false,
    AntiAFK = false
}

local RuntimeState = {
    CurrentPhase = "Idle",
    KeyDetectedId = nil,
    KeyPosition = nil,
    KeyStatus = "Locked 🔒",
    TotalSlicesPeeled = 0,
    TotalPotatoesPeeled = 0,
    SessionEarnings = 0,
    LastCash = 0,
    SweetSpotReach = 0
}

_G.PeelThePotato_Config = Config
_G.PeelThePotato_Runtime = RuntimeState

local UIControls = {}
local isAutoLoading = false
local saveDebounceTimer = nil



local SETTING_KEY_ALIASES = {
    MasterFarm = { "MasterFarm", "Potato Loop", "Master Auto-Farm Loop", "Auto Potato", "AutoPotato", "Auto-Potato", "Auto_Potato", "auto potato", "autopotato" },
    AutoPeelAtTable = { "AutoPeelAtTable" },
    StandalonePeeler = { "StandalonePeeler", "Standalone Fast Peeler" },
    SliceDelay = { "SliceDelay", "Slice Stripping Delay" },
    AutoLeaveStationOnEmpty = { "AutoLeaveStationOnEmpty", "Auto-Leave Station When Empty" },
    AutoLoadTruckPallet = { "AutoLoadTruckPallet", "Auto-Load Pallet to Truck" },
    AutoDeliverTruck = { "AutoDeliverTruck", "Auto-Deliver Military Truck" },
    AutoCoolFire = { "AutoCoolFire", "Auto-Cool Pit Fire (Chapter 2)" },
    AutoGrabAndEscape = { "AutoGrabAndEscape", "Auto Grab Key & Escape Bunker" },
    AutoRequeue = { "AutoRequeue", "Auto Requeue on Return to Lobby" },
    PreferredChapter = { "PreferredChapter", "Target Chapter" },
    RequeuePartySize = { "RequeuePartySize", "Queue Party Size" },
    SkipCutscenes = { "SkipCutscenes", "Skip Cutscenes" },
    AutoLobbyUpgrades = { "AutoLobbyUpgrades", "Auto-Buy Lobby Upgrades" },
    AutoBuyAllUpgrades = { "AutoBuyAllUpgrades", "Auto-Buy All Upgrades & Items" },
    AutoClaimDaily = { "AutoClaimDaily", "Auto-Claim Daily Login Streak" },
    AutoClaimFreeCrates = { "AutoClaimFreeCrates", "Auto-Claim Free Crates" },
    AutoClaimBattlePass = { "AutoClaimBattlePass", "Auto-Claim Battle Pass Tiers" },
    AutoClaimAchievements = { "AutoClaimAchievements", "Auto-Claim Achievements" },
    AutoClaimUpdateGift = { "AutoClaimUpdateGift", "Auto-Claim Update Gift & News" },
    WalkSpeedEnabled = { "WalkSpeedEnabled", "Enable WalkSpeed Boost" },
    WalkSpeed = { "WalkSpeed", "WalkSpeed Multiplier" },
    JumpPowerEnabled = { "JumpPowerEnabled", "Enable JumpPower Boost" },
    JumpPower = { "JumpPower", "JumpPower Multiplier" },
    InfiniteJump = { "InfiniteJump", "Infinite Jump" },
    Noclip = { "Noclip", "Collisionless Noclip" },
    AntiAFK = { "AntiAFK", "Built-in Anti-AFK Engine" }
}

-- Saves specifically to Aether/Configs/AutoPotato.json in the workspace
local function getGameConfigPath()
    return "Aether/Configs/AutoPotato.json"
end

local function ensureAetherFolder()
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if not hasFS then return false end
    pcall(function()
        if type(isfolder) == "function" and type(makefolder) == "function" then
            if not isfolder("Aether") then
                makefolder("Aether")
            end
            if not isfolder("Aether/Configs") then
                makefolder("Aether/Configs")
            end
        end
    end)
    return true
end

local function saveGameConfig()
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if not hasFS then return false end
    
    local ok, err = pcall(function()
        ensureAetherFolder()
        local filePath = getGameConfigPath()
        
        local dataToSave = {
            Settings = {},
            Elements = {}
        }
        
        for k, v in pairs(Config) do
            if type(v) == "boolean" or type(v) == "number" or type(v) == "string" then
                dataToSave.Settings[k] = v
                dataToSave.Elements[k] = v
                if SETTING_KEY_ALIASES[k] then
                    for _, alias in ipairs(SETTING_KEY_ALIASES[k]) do
                        dataToSave.Elements[alias] = v
                    end
                end
            end
        end

        for controlKey, controlObj in pairs(UIControls) do
            if controlObj and typeof(controlObj.GetValue) == "function" then
                local s, val = pcall(controlObj.GetValue, controlObj)
                if s and val ~= nil then
                    dataToSave.Elements[controlKey] = val
                    if SETTING_KEY_ALIASES[controlKey] then
                        for _, alias in ipairs(SETTING_KEY_ALIASES[controlKey]) do
                            dataToSave.Elements[alias] = val
                        end
                    end
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
    if saveDebounceTimer then
        task.cancel(saveDebounceTimer)
        saveDebounceTimer = nil
    end
    saveDebounceTimer = task.delay(0.4, function()
        saveGameConfig()
        saveDebounceTimer = nil
    end)
end

local function resolveValueFromDecoded(decoded, key)
    if type(decoded) ~= "table" then return nil end
    local aliases = SETTING_KEY_ALIASES[key] or { key }
    local allKeys = { key }
    for _, a in ipairs(aliases) do
        if a ~= key then table.insert(allKeys, a) end
    end
    
    -- Priority 1: Elements (where UI controls & explicit toggles save)
    if type(decoded.Elements) == "table" then
        for _, k in ipairs(allKeys) do
            if decoded.Elements[k] ~= nil then
                return decoded.Elements[k]
            end
        end
    end
    
    -- Priority 2: Settings table
    if type(decoded.Settings) == "table" then
        for _, k in ipairs(allKeys) do
            if decoded.Settings[k] ~= nil then
                return decoded.Settings[k]
            end
        end
    end
    
    -- Priority 3: Root table
    for _, k in ipairs(allKeys) do
        if decoded[k] ~= nil and type(decoded[k]) ~= "table" then
            return decoded[k]
        end
    end
    
    return nil
end

-- Pre-load saved configuration into Config table so UI controls initialize with their correct saved values
local function loadInitialConfig()
    local hasFS = (type(writefile) == "function" and type(readfile) == "function")
    if not hasFS then return false end
    
    local filePath = getGameConfigPath()
    local fileExists = false
    pcall(function()
        if type(isfile) == "function" and isfile(filePath) then
            fileExists = true
        end
    end)
    if not fileExists then return false end
    
    local content = nil
    pcall(function() content = readfile(filePath) end)
    if not content or content == "" then return false end
    
    local s, decoded = pcall(function() return HttpService:JSONDecode(content) end)
    if not s or type(decoded) ~= "table" then return false end
    
    for k, _ in pairs(Config) do
        local val = resolveValueFromDecoded(decoded, k)
        if val ~= nil and type(Config[k]) == type(val) then
            Config[k] = val
        end
    end
    return true
end

-- Run immediate pre-load so Config holds correct values before window & UI creation
loadInitialConfig()

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
        -- First time running in this game: save default settings to initialize Aether/Configs/AutoPotato.json
        saveGameConfig()
        return false
    end
    
    local content = nil
    pcall(function()
        content = readfile(filePath)
    end)
    if not content or content == "" then return false end
    
    local s, decoded = pcall(function()
        return HttpService:JSONDecode(content)
    end)
    if not s or type(decoded) ~= "table" then return false end
    
    isAutoLoading = true

    -- 1. Apply to Config table
    for k, _ in pairs(Config) do
        local val = resolveValueFromDecoded(decoded, k)
        if val ~= nil and type(Config[k]) == type(val) then
            Config[k] = val
        end
    end
    
    -- 2. Apply to UI controls (with skipCallback = true so visual toggle updates without side effects)
    for controlKey, controlObj in pairs(UIControls) do
        local savedVal = resolveValueFromDecoded(decoded, controlKey)
        if savedVal ~= nil and controlObj and typeof(controlObj.SetValue) == "function" then
            pcall(function()
                controlObj:SetValue(savedVal, true)
            end)
        end
    end
    
    task.delay(0.35, function()
        isAutoLoading = false
    end)
    
    return true
end

local isTutorialRunning = false
local tutorialCompletedOnce = false

-- Chapter Detection
local function isChapter3()
    local map = workspace:FindFirstChild("Map")
    return (map and (map:FindFirstChild("PackingTables") ~= nil or map:FindFirstChild("Trucks") ~= nil or map:FindFirstChild("Chapter2Ending") ~= nil))
        or (PotatoTableShared and (PotatoTableShared.IS_PACKING == true or PotatoTableShared.USES_PALLET == true))
        or (DeliveryShared and DeliveryShared.IS_ACTIVE == true)
end

local function isChapter2()
    if isChapter3() then return false end
    local map = workspace:FindFirstChild("Map")
    return (map and (map:FindFirstChild("Chapter1Ending") ~= nil or map:FindFirstChild("SellConveyor") ~= nil))
        or workspace:FindFirstChild("Chapter2Intro") ~= nil
        or (PotatoTableShared and PotatoTableShared.IS_CUTTING == true)
end

local function isChapter1()
    return not isChapter2() and not isChapter3()
end

local function getChapterNumber()
    if isChapter3() then return 3 end
    if isChapter2() then return 2 end
    return 1
end

-- Positions & Landmarks (Adaptive for Chapters 1, 2, and 3)
local Chapter1Landmarks = {
    PotatoPile = Vector3.new(-10.0, 14.5, -258.0),
    PeelingTable1 = Vector3.new(-17.84, -0.26, -152.31),
    PeelingTable1_Stand = Vector3.new(-21.44, 0.27, -152.31),
    PeelingTable2_Stand = Vector3.new(-21.44, 0.27, -143.81),
    PeelingTable3_Stand = Vector3.new(-14.24, 0.27, -152.31),
    PeelingTable4_Stand = Vector3.new(-14.24, 0.27, -143.81),
    PotatoCrate = Vector3.new(-11.84, -2.1, -154.8),
    SellCrate = Vector3.new(-11.84, 0.5, -141.31),
    Pallet = Vector3.new(-11.84, 0.5, -141.31),
    AutoPeelingMachine = Vector3.new(13.13, -3.0, -147.79),
    PeelingMachine = Vector3.new(13.13, 3.5, -158.29), -- Securely ABOVE machine (Prevents conveyor belt carry)
    ConveyorStore = Vector3.new(-22.47, 3.79, -188.98),
    Store_Peelers = Vector3.new(-23.6, -2.5, -164.2),
    Store_Storage = Vector3.new(-23.6, -2.5, -172.5),
    Store_Tools = Vector3.new(-23.6, -2.5, -180.8),
    Store_Boxes = Vector3.new(-23.6, -2.5, -164.2),
    VaultDoor = Vector3.new(-32.9, 4.1, -131.6)
}

local Chapter2Landmarks = {
    PotatoPile = Vector3.new(161.5, 138.0, 1165.0),
    PeelingTable1 = Vector3.new(155.5, 128.7, 1270.74),
    PeelingTable1_Stand = Vector3.new(152.0, 128.7, 1270.74),
    PeelingTable2_Stand = Vector3.new(152.0, 128.7, 1279.24),
    PeelingTable3_Stand = Vector3.new(171.0, 128.7, 1279.24),
    PeelingTable4_Stand = Vector3.new(171.0, 128.7, 1270.74),
    PotatoCrate = Vector3.new(161.5, 128.0, 1266.0),
    SellCrate = Vector3.new(161.5, 128.0, 1279.5),
    Pallet = Vector3.new(161.5, 128.0, 1279.5),
    AutoPeelingMachine = Vector3.new(182.0, 128.0, 1282.0),
    PeelingMachine = Vector3.new(184.62, 135.5, 1263.25), -- Securely ABOVE machine (Prevents conveyor belt carry)
    ConveyorStore = Vector3.new(146.0, 128.5, 1208.0),
    Store_Peelers = Vector3.new(139.5, 128.0, 1275.0),
    Store_Storage = Vector3.new(174.5, 128.0, 1225.0),
    Store_Tools = Vector3.new(147.0, 128.0, 1230.0),
    Store_Boxes = Vector3.new(139.5, 128.0, 1275.0),
    WaterValve = Vector3.new(148.60, 131.65, 1198.69),
    VaultDoor = Vector3.new(124.06, 135.2, 1310.26)
}

local Chapter3Landmarks = {
    PotatoPile = Vector3.new(1205.50, 126.00, 2302.60),
    PeelingTable1 = Vector3.new(1197.88, 126.21, 2387.22),
    PeelingTable1_Stand = Vector3.new(1197.88, 126.21, 2389.00),
    PeelingTable2_Stand = Vector3.new(1197.88, 126.21, 2396.50),
    PeelingTable3_Stand = Vector3.new(1213.50, 126.21, 2389.00),
    PeelingTable4_Stand = Vector3.new(1213.50, 126.21, 2396.50),
    PotatoCrate = Vector3.new(1205.75, 127.71, 2399.99),
    SellCrate = Vector3.new(1205.75, 127.71, 2399.99),
    Pallet = Vector3.new(1205.75, 127.71, 2399.99),
    AutoPeelingMachine = Vector3.new(1228.56, 130.61, 2398.03),
    PeelingMachine = Vector3.new(1227.71, 133.00, 2383.61), -- Securely ABOVE Hand Packer crank
    ConveyorStore = Vector3.new(1184.24, 127.77, 2346.41),
    Store_Peelers = Vector3.new(1186.32, 126.94, 2390.08), -- Boxes stall
    Store_Storage = Vector3.new(1184.24, 127.77, 2346.41),
    Store_Tools = Vector3.new(1183.49, 130.24, 2367.16),
    Store_Boxes = Vector3.new(1186.32, 126.94, 2390.08),
    Tracker = Vector3.new(1194.30, 127.50, 2426.30),
    VaultDoor = Vector3.new(1194.30, 127.50, 2426.30), -- Target escape for tracker cut
    MilitaryTruck = Vector3.new(1190.08, 134.10, 2424.83) -- Exact DeliveryPrompt driver door position
}

local function getLandmark(key)
    if isChapter3() then
        return Chapter3Landmarks[key] or Chapter1Landmarks[key]
    elseif isChapter2() then
        return Chapter2Landmarks[key] or Chapter1Landmarks[key]
    end
    return Chapter1Landmarks[key]
end

local Landmarks = setmetatable({}, {
    __index = function(_, key)
        return getLandmark(key)
    end
})

-- ===================================================================
-- 5. HELPER FUNCTIONS
-- ===================================================================
local function getCharacter()
    return LocalPlayer.Character or LocalPlayer.CharacterAdded:Wait()
end

local function getHRP()
    local char = getCharacter()
    return char and char:FindFirstChild("HumanoidRootPart")
end

local function getHumanoid()
    local char = getCharacter()
    return char and char:FindFirstChildOfClass("Humanoid")
end

local isCoolingActive = false
local isDelivering = false

local function isDeliveryMinigameActive()
    if DeliveryController and typeof(DeliveryController.IsActive) == "function" and DeliveryController.IsActive() then
        return true
    end
    if workspace:FindFirstChild("DeliveryWorld") ~= nil or workspace:FindFirstChild("TruckScene") ~= nil then
        return true
    end
    local pGui = LocalPlayer and (LocalPlayer:FindFirstChild("PlayerGui") or LocalPlayer:FindFirstChildOfClass("PlayerGui"))
    local deliveryGui = pGui and pGui:FindFirstChild("Delivery")
    if deliveryGui and deliveryGui.Enabled == true then
        return true
    end
    return false
end

local function resetCamera()
    pcall(function()
        local cam = workspace.CurrentCamera
        local hum = getHumanoid()
        if cam then
            if cam.CameraType ~= Enum.CameraType.Custom then
                cam.CameraType = Enum.CameraType.Custom
            end
            if hum and cam.CameraSubject ~= hum then
                cam.CameraSubject = hum
            end
        end
    end)
end

local function isAetherOpen()
    local cg = game:GetService("CoreGui")
    local gh = typeof(gethui) == "function" and gethui()
    local pg = LocalPlayer and LocalPlayer:FindFirstChild("PlayerGui")
    local aether = (cg and cg:FindFirstChild("Aether")) or (gh and gh:FindFirstChild("Aether")) or (pg and pg:FindFirstChild("Aether"))
    local shadow = aether and aether:FindFirstChild("Shadow")
    return shadow and shadow.Visible == true
end

local function ensurePlayerControls()
    pcall(function()
        -- 1. Ensure Player WASD controls are enabled so movement is never locked
        local pm = LocalPlayer and LocalPlayer:FindFirstChild("PlayerScripts") and LocalPlayer.PlayerScripts:FindFirstChild("PlayerModule")
        if pm then
            local ok, m = pcall(require, pm)
            if ok and m and m.GetControls then
                local controls = m:GetControls()
                if controls and not controls.controlsEnabled then
                    if not (PeelingController and PeelingController.IsPeeling) and not isDeliveryMinigameActive() and not isDelivering then
                        controls:Enable(true)
                    end
                end
            end
        end
        -- 2. Ensure Humanoid is not stuck in Climbing or disabled states
        local hum = getHumanoid()
        if hum then
            if hum:GetStateEnabled(Enum.HumanoidStateType.Climbing) then
                hum:SetStateEnabled(Enum.HumanoidStateType.Climbing, false)
            end
            if hum:GetState() == Enum.HumanoidStateType.Climbing then
                hum:ChangeState(Enum.HumanoidStateType.Running)
            end
            if hum.PlatformStand then hum.PlatformStand = false end
            if hum.Sit then hum.Sit = false end
        end
        -- 3. Camera & Mouse Cursor Guardian: Maintain default first-person view and suppress cursor leaks
        if not (PeelingController and PeelingController.IsPeeling) and not isCoolingActive and not isDeliveryMinigameActive() and not isDelivering then
            local rep = game:GetService("ReplicatedStorage")
            local ccModule = rep:FindFirstChild("Controllers") and rep.Controllers:FindFirstChild("CameraController")
            local uiOpen = isAetherOpen()
            if ccModule then
                local ok, cc = pcall(require, ccModule)
                if ok and cc then
                    if typeof(cc.SetMouseUnlocked) == "function" then
                        cc.SetMouseUnlocked("StoreView", false)
                        cc.SetMouseUnlocked("ToolUpgrades", false)
                        cc.SetMouseUnlocked("Delivery", false)
                        cc.SetMouseUnlocked("Alt", false)
                        if uiOpen then
                            cc.SetMouseUnlocked("AetherUI", true)
                        else
                            cc.SetMouseUnlocked("AetherUI", false)
                        end
                    end
                    if typeof(cc.SetCameraFree) == "function" then
                        cc.SetCameraFree("Inspect", false)
                        cc.SetCameraFree("ToolUpgrades", false)
                    end
                    if typeof(cc.SetCursorVisible) == "function" then
                        cc.SetCursorVisible("Inspect", false)
                    end
                end
            end
            local inspectModule = rep:FindFirstChild("Controllers") and rep.Controllers:FindFirstChild("InspectController")
            if inspectModule then
                local ok, inspect = pcall(require, inspectModule)
                if ok and inspect and typeof(inspect.StopInspect) == "function" then
                    pcall(inspect.StopInspect)
                end
            end
            resetCamera()
            if LocalPlayer and LocalPlayer.CameraMode ~= Enum.CameraMode.LockFirstPerson then
                LocalPlayer.CameraMode = Enum.CameraMode.LockFirstPerson
            end
            if not uiOpen then
                local uis = game:GetService("UserInputService")
                if uis.MouseBehavior ~= Enum.MouseBehavior.LockCenter then
                    uis.MouseBehavior = Enum.MouseBehavior.LockCenter
                end
                if uis.MouseIconEnabled then
                    uis.MouseIconEnabled = false
                end
            end
        end
    end)
end
local ensureControlsAndCamera = ensurePlayerControls

local function getPlayerState()
    if not PlayerStateShared then return {} end
    return PlayerStateShared.GetPlayerStateAsync(LocalPlayer)
end

local function getBagCapacity(passedState)
    local state = passedState or getPlayerState()
    local cap = 10
    local carryLevel = 1
    if state and CarryUpgradeShared then
        local ok, res = pcall(function()
            local lobbyUpgrades = (state.SaveData and state.SaveData.LobbyUpgrades) or {}
            local gamepasses = state.Gamepasses or {}
            carryLevel = state.CarryLevel or 1
            return CarryUpgradeShared.GetPlayerCapacity(carryLevel, gamepasses, lobbyUpgrades)
        end)
        if ok and res then cap = res end
    end
    -- In Chapter 3 only: Hands (CarryLevel 1) is strictly capped at 60 potatoes on the server
    if isChapter3() and carryLevel == 1 and cap > 60 then
        cap = 60
    end
    -- Handle infinite storage gamepass (returns math.huge or 1/0)
    if cap == math.huge or cap > 100000 then
        return 250 -- Safe batch threshold for infinite storage
    end
    return cap or 10
end

local function getBagCount(passedState)
    if BagController and type(BagController.GetKinds) == "function" then
        local ok, kinds = pcall(BagController.GetKinds)
        if ok and type(kinds) == "table" then
            return #kinds
        end
    end
    
    local state = passedState or getPlayerState()
    if state and type(state.Potatoes) == "number" then
        return state.Potatoes
    end
    
    local guiCount = nil
    pcall(function()
        local counter = (BagController and BagController.GetCounter and BagController.GetCounter())
        if not counter then
            local pGui = LocalPlayer and (LocalPlayer:FindFirstChild("PlayerGui") or LocalPlayer:FindFirstChildOfClass("PlayerGui"))
            local bagGui = pGui and pGui:FindFirstChild("Bag")
            counter = bagGui and bagGui:FindFirstChild("Counter", true)
        end
        local valLabel = counter and counter:FindFirstChild("Value")
        if valLabel and valLabel.Text then
            local c = valLabel.Text:match("(%d+)%s*/")
            if c then guiCount = tonumber(c) end
        end
    end)
    
    return guiCount or 0
end

local function getInventoryAudit(passedState)
    local state = passedState or getPlayerState()
    local count = getBagCount(state)
    local capacity = getBagCapacity(state)
    
    local isMaxed = count >= capacity
    local isEmpty = count <= 0
    
    return {
        count = count,
        capacity = capacity,
        isMaxed = isMaxed,
        isEmpty = isEmpty
    }
end

-- ===================================================================
-- BAG FULL CHECK
-- Real-time check backed by BagController.GetKinds() and BagFullReported sentinel
-- ===================================================================
local PickGainHistory = {} -- [toolName] = { recent single-pick gains }

local function recordPickGain(toolName, gain)
    if not toolName or not gain or gain <= 0 then return end
    local hist = PickGainHistory[toolName] or {}
    table.insert(hist, gain)
    while #hist > 5 do table.remove(hist, 1) end
    PickGainHistory[toolName] = hist
end

local function getExpectedPickGain(toolName)
    if not toolName then
        pcall(function() toolName = ToolEquipController and ToolEquipController.GetEquippedTool() end)
    end
    -- 1. Check observed history first
    local hist = toolName and PickGainHistory[toolName]
    if hist and #hist > 0 then
        local counts, best, bestCount = {}, 0, 0
        for _, g in ipairs(hist) do
            counts[g] = (counts[g] or 0) + 1
            if counts[g] > bestCount or (counts[g] == bestCount and g > best) then
                best, bestCount = g, counts[g]
            end
        end
        if best > 0 then return best end
    end
    
    -- 2. Fallback to theoretical calculation from ToolUpgradeShared & PotatoKindShared
    local theoretical = nil
    pcall(function()
        if ToolUpgradeShared and ToolEquipController then
            local upgs = ToolEquipController.GetEquippedUpgrades()
            local classId = ToolEquipController.GetClassId()
            local slices = (PotatoKindShared and PotatoKindShared.SLICES_PER_PICK) or 1
            local amount = ToolUpgradeShared.GetPickAmount(upgs, toolName or "Hand", classId) or 1
            theoretical = amount * slices
        end
    end)
    return theoretical or 1
end

local function isBagFull(toolName, passedState)
    local state = (type(toolName) == "table" and toolName) or passedState
    
    -- 1. If game server/UI recently reported "Your bag is full", treat as full immediately (2.0s reactive guard)
    if BagFullReported or (os.clock() - LastBagFullTriggerTime < 2.0) then
        return true, getInventoryAudit(state)
    end
    
    local audit = getInventoryAudit(state)
    if audit.isMaxed then
        return true, audit
    end
    
    return false, audit
end

local function isBagMaxed()
    return (isBagFull())
end

local function isBagEmpty()
    return getInventoryAudit().isEmpty
end

-- Raycast to ground to ensure player NEVER clips into ground or falls through map
local function getSafeSurfacePosition(targetPos, isPileArea)
    local char = getCharacter()
    local x = targetPos.X
    local z = targetPos.Z
    local ch3 = isChapter3()
    local ch2 = isChapter2()
    
    local searchTop = targetPos.Y + 12
    if isPileArea then
        searchTop = ch3 and 145 or (ch2 and 150 or 45)
    end
    
    local rayOrigin = Vector3.new(x, searchTop, z)
    local rayDir = Vector3.new(0, -60, 0)
    local rayParams = RaycastParams.new()
    rayParams.FilterType = Enum.RaycastFilterType.Exclude
    if char then rayParams.FilterDescendantsInstances = { char } end
    
    local hit = workspace:Raycast(rayOrigin, rayDir, rayParams)
    if hit then
        local minGround = (ch3 or ch2) and 120 or -5
        if hit.Position.Y > minGround then
            return Vector3.new(x, hit.Position.Y + 3.2, z)
        end
    end
    
    if isPileArea then
        return ch3 and Vector3.new(x, 126.0, z) or (ch2 and Vector3.new(x, 138.0, z) or Vector3.new(x, 4.8, z))
    end
    return targetPos
end

-- Sweet spot dynamic calculator: scans potato pile and finds spot with MAX potatoes in reach
local cachedSweetSpot = nil
local cachedPotatoesInReach = 0

local function getPotatoPileInstance()
    local pile = workspace:FindFirstChild("PotatoPile")
    if pile and #pile:GetChildren() > 0 then
        return pile
    end
    local map = workspace:FindFirstChild("Map")
    local mapPile = map and map:FindFirstChild("PotatoPile")
    if mapPile and #mapPile:GetChildren() > 0 then
        return mapPile
    end
    return pile or mapPile
end

local function calculateBestPotatoSpot(searchDist)
    searchDist = searchDist or 18
    local pile = getPotatoPileInstance()
    local ch3 = isChapter3()
    local ch2 = isChapter2()
    local defaultSweetSpot = ch3 and Vector3.new(1205.5, 126.0, 2302.6) or (ch2 and Vector3.new(161.5, 138.0, 1165.0) or Vector3.new(-10.0, 0, -258.0))
    
    if not pile then return cachedSweetSpot or defaultSweetSpot, cachedPotatoesInReach end
    
    local pileChildren = {}
    for _, ch in ipairs(pile:GetChildren()) do
        if ch:IsA("BasePart") then
            table.insert(pileChildren, ch)
        end
    end
    
    local candidates
    if ch3 then
        candidates = {
            Vector3.new(1205.5, 126, 2302.6),
            Vector3.new(1205.5, 126, 2307),
            Vector3.new(1208.5, 126, 2302),
            Vector3.new(1202.5, 126, 2302),
            Vector3.new(1205.5, 126, 2298)
        }
    elseif ch2 then
        candidates = {
            Vector3.new(161.5, 138, 1165),
            Vector3.new(161.5, 138, 1170),
            Vector3.new(163.5, 138, 1168),
            Vector3.new(158.5, 138, 1168),
            Vector3.new(161.5, 138, 1162),
            Vector3.new(165.5, 138, 1165),
            Vector3.new(156.5, 138, 1165)
        }
    else
        candidates = {
            Vector3.new(-10, 0, -258),
            Vector3.new(-12, 0, -259),
            Vector3.new(-14, 0, -260),
            Vector3.new(-16, 0, -262),
            Vector3.new(-8, 0, -258),
            Vector3.new(-6, 0, -255),
            Vector3.new(-16, 0, -265),
            Vector3.new(-12, 0, -265),
            Vector3.new(-10, 0, -255)
        }
    end
    
    local bestSpot = defaultSweetSpot
    local maxCount = 0
    
    for _, c in ipairs(candidates) do
        local safePos = getSafeSurfacePosition(c, true)
        local count = 0
        for _, part in ipairs(pileChildren) do
            if (part.Position - safePos).Magnitude <= searchDist then
                count = count + 1
            end
        end
        if count > maxCount then
            maxCount = count
            bestSpot = safePos
        end
    end
    
    if maxCount > 0 then
        cachedSweetSpot = bestSpot
        cachedPotatoesInReach = maxCount
        RuntimeState.SweetSpotReach = maxCount
    else
        cachedSweetSpot = defaultSweetSpot
    end
    return cachedSweetSpot, cachedPotatoesInReach
end

isCoolingActive = false

local function moveToPosition(targetPos, isPileArea)
    if isCoolingActive or isDeliveryMinigameActive() or isDelivering then return end
    local hrp = getHRP()
    if not hrp then return end
    
    local ch3 = isChapter3()
    local ch2 = isChapter2()
    local safePos = targetPos
    if isPileArea or (ch3 and targetPos.Z >= 2280 and targetPos.Z <= 2325 and math.abs(targetPos.X - 1205.5) <= 25)
        or (ch2 and targetPos.Z <= 1180 and targetPos.Z >= 1140 and math.abs(targetPos.X - 161.5) <= 25)
        or (not ch2 and not ch3 and targetPos.Z <= -242) then
        safePos = getSafeSurfacePosition(targetPos, true)
    end
    
    if Config.MovementMode == "Teleport" then
        hrp.CFrame = CFrame.new(safePos)
        task.wait(0.1)
    else
        local hum = getHumanoid()
        if hum then
            hum:MoveTo(safePos)
            local start = os.clock()
            while (hrp.Position - safePos).Magnitude > 3 and os.clock() - start < 8 do
                task.wait(0.1)
            end
        end
    end
end

local function firePromptCompat(prompt: ProximityPrompt, holdTime: number?)
    local fn: any = fireproximityprompt
    if typeof(fn) == "function" and prompt then
        if holdTime ~= nil then
            pcall(function() fn(prompt, holdTime) end)
        else
            pcall(function() fn(prompt) end)
        end
    end
end

local function safeFirePrompt(prompt, holdTime)
    if not prompt or not prompt.Enabled then return false end
    if isDeliveryMinigameActive() and not isDelivering then return false end
    
    local hrp = getHRP()
    local promptPart = (prompt.Parent and prompt.Parent:IsA("BasePart") and prompt.Parent)
        or (prompt.Parent and prompt.Parent:IsA("Attachment") and prompt.Parent.Parent:IsA("BasePart") and prompt.Parent.Parent)
    if hrp and promptPart then
        pcall(function()
            hrp.CFrame = CFrame.new(hrp.Position, Vector3.new(promptPart.Position.X, hrp.Position.Y, promptPart.Position.Z))
        end)
    end
    
    if holdTime and holdTime > 0 then
        pcall(function() prompt:InputHoldBegin() end)
        firePromptCompat(prompt, holdTime)
        task.wait(holdTime)
        pcall(function() prompt:InputHoldEnd() end)
        firePromptCompat(prompt, 0)
    else
        firePromptCompat(prompt, 0)
        pcall(function()
            prompt:InputHoldBegin()
            task.wait(0.05)
            prompt:InputHoldEnd()
        end)
    end
    return true
end

-- Find the best available peeling table peel stand
local function getBestPeelStand()
    local map = workspace:FindFirstChild("Map")
    if not map then return Landmarks.PeelingTable1_Stand, nil end
    local folderName = (PotatoTableShared and PotatoTableShared.TABLES_FOLDER_NAME) or (isChapter3() and "PackingTables" or "PeelingMachines")
    local container = map:FindFirstChild(folderName) or map:FindFirstChild("PackingTables") or map:FindFirstChild("PeelingMachines")
    if not container then return Landmarks.PeelingTable1_Stand, nil end
    
    local prefix = isChapter3() and "PackingTable" or "PeelingTable"
    local stands = {
        { table = container:FindFirstChild(prefix .. "1") or container:FindFirstChild("PackingTable1") or container:FindFirstChild("PeelingTable1"), stand = Landmarks.PeelingTable1_Stand },
        { table = container:FindFirstChild(prefix .. "2") or container:FindFirstChild("PackingTable2") or container:FindFirstChild("PeelingTable2"), stand = Landmarks.PeelingTable2_Stand },
        { table = container:FindFirstChild(prefix .. "3") or container:FindFirstChild("PackingTable3") or container:FindFirstChild("PeelingTable3"), stand = Landmarks.PeelingTable3_Stand },
        { table = container:FindFirstChild(prefix .. "4") or container:FindFirstChild("PackingTable4") or container:FindFirstChild("PeelingTable4"), stand = Landmarks.PeelingTable4_Stand },
    }
    
    for _, item in ipairs(stands) do
        if item.table then
            local prompt = item.table:FindFirstChildWhichIsA("ProximityPrompt", true)
            if prompt and prompt:GetAttribute(PotatoTableShared.IN_USE_ATTRIBUTE) ~= true then
                return item.stand, prompt
            end
        end
    end
    
    local firstTable = container:FindFirstChild(prefix .. "1") or container:FindFirstChild("PackingTable1") or container:FindFirstChild("PeelingTable1")
    local defPrompt = firstTable and firstTable:FindFirstChildWhichIsA("ProximityPrompt", true)
    return Landmarks.PeelingTable1_Stand, defPrompt
end

-- ===================================================================
-- 6. CORE AUTOMATION ENGINES
-- ===================================================================

-- [ENGINE 1]: FAST POTATO SLICE PEELER, CUTTER & PACKER
local function peelActivePotatoOnce()
    -- Chapter 2: Cutting Table Minigame Handling (CuttingController)
    if PotatoTableShared.IS_CUTTING and CuttingController then
        CuttingController.SetAuto(true)
        CuttingController.SetHeld(true)
        
        local ok, ups = pcall(function()
            return (typeof(debug.getupvalues) == "function" and debug.getupvalues(CuttingController.SetHeld))
        end)
        local tryCut = ok and ups and ups[5]
        if tryCut then
            -- Reset debounce cooldown in tryCut (upvalue 1 is u20)
            pcall(function()
                if typeof(debug.setupvalue) == "function" then
                    debug.setupvalue(tryCut, 1, 0)
                end
            end)
            
            -- Fast potato alignment & tool guarantee: snap potato to Target position so Magnitude > 0.3 check doesn't block cut
            pcall(function()
                if typeof(debug.getupvalues) == "function" then
                    local cutUps = debug.getupvalues(tryCut)
                    local potList = cutUps and cutUps[2] -- u8 is upvalue 2
                    if potList and #potList > 0 then
                        for _, pData in ipairs(potList) do
                            if pData and pData.Target and pData.CFrame then
                                pData.CFrame = pData.Target
                            end
                        end
                    end
                    local curKnife = cutUps and cutUps[3] -- u11 is upvalue 3
                    if curKnife then
                        curKnife.Instant = true
                        curKnife.SlicesPerCut = 25
                        curKnife.MoveSpeed = 60
                    end
                end
            end)
            
            pcall(tryCut)
        end
        RuntimeState.TotalSlicesPeeled = RuntimeState.TotalSlicesPeeled + 1
        return CuttingController.GetPotato() ~= nil
    end

    -- Chapter 3: Packing Table Minigame Handling (PackingController)
    if (PotatoTableShared.IS_PACKING or isChapter3()) and PackingController then
        PackingController.SetAuto(true)
        PackingController.SetHeld(true)
        RuntimeState.TotalSlicesPeeled = RuntimeState.TotalSlicesPeeled + 1
        return PackingController.GetPotato() ~= nil
    end

    -- Chapter 1: Fast Peel Execution
    local internals = getPeelingInternals()
    local performPeel = internals.performPeel
    
    -- Find potato model in workspace
    local potatoModel = internals.getPotato and internals.getPotato()
    if not (potatoModel and potatoModel.Parent) then
        potatoModel = workspace:FindFirstChild("UnpeeledPotatoForMinigame")
    end
    
    if not potatoModel then return false end
    
    local peels = {}
    for _, ch in ipairs(potatoModel:GetChildren()) do
        if (ch.Name == "Peel" or ch.Name == "PotatoSlice") and ch:IsA("BasePart") and ch.Parent == potatoModel then
            table.insert(peels, ch)
        end
    end
    
    if #peels == 0 then return false end
    
    -- Rapid peel execution
    if performPeel then
        for _, peelPart in ipairs(peels) do
            if peelPart and peelPart.Parent == potatoModel then
                pcall(function()
                    performPeel(potatoModel, peelPart, peelPart.Position)
                end)
                RuntimeState.TotalSlicesPeeled = RuntimeState.TotalSlicesPeeled + 1
                if Config.SliceDelay > 0 then
                    task.wait(Config.SliceDelay)
                else
                    task.wait()
                end
            end
        end
    else
        for idx, peelPart in ipairs(peels) do
            if peelPart and peelPart.Parent == potatoModel then
                pcall(function()
                    Packets.PeelingPeelRemoved:Fire(idx, peelPart.Position)
                end)
                RuntimeState.TotalSlicesPeeled = RuntimeState.TotalSlicesPeeled + 1
                task.wait(Config.SliceDelay)
            end
        end
        pcall(function() Packets.PeelingFinished:Fire() end)
    end
    
    RuntimeState.TotalPotatoesPeeled = RuntimeState.TotalPotatoesPeeled + 1
    return true
end

-- ===================================================================
-- TOOL MANAGEMENT & DYNAMITE CYCLER
-- ===================================================================
local lastDynamiteThrowTime = 0

local function getMainHarvestTool()
    local state = getPlayerState()
    local tools = (state and state.Tools) or {}
    if tools["PotatoSucker"] or ToolEquipController.OwnsTool("PotatoSucker") then
        return "PotatoSucker"
    elseif tools["Shovel"] or ToolEquipController.OwnsTool("Shovel") then
        return "Shovel"
    else
        return "Hand"
    end
end

local function ownsDynamite()
    local state = getPlayerState()
    local tools = (state and state.Tools) or {}
    return tools["Dynamite"] == true or ToolEquipController.OwnsTool("Dynamite")
end

local function isDynamiteReady()
    if not ownsDynamite() then return false end
    local full, audit = isBagFull("Dynamite")
    if full or (audit.capacity - audit.count < 35) then
        return false
    end
    local equippedUpgrades = ToolEquipController.GetEquippedUpgrades()
    local classId = ToolEquipController.GetClassId()
    local dynInterval = ToolUpgradeShared.GetPickInterval(equippedUpgrades, "Dynamite", classId, false) or 9
    if os.clock() - lastDynamiteThrowTime < dynInterval then
        return false
    end
    local remCd = PotatoPileController.GetPickCooldown("Dynamite")
    return (remCd or 0) <= 0
end

local function throwDynamiteAtPile(targetId)
    if not isDynamiteReady() then return false end
    
    ToolEquipController.Equip("Dynamite")
    task.wait(0.15)
    
    pcall(function()
        ToolEquipController.Use()
        Packets.DynamiteThrow:Fire(targetId)
    end)
    lastDynamiteThrowTime = os.clock()
    task.wait(0.35)
    
    local mainTool = getMainHarvestTool()
    ToolEquipController.Equip(mainTool)
    task.wait(0.15)
    return true
end

-- ===================================================================
-- CHAPTER 2 PIT FIRE COOLING ENGINE
-- ===================================================================
local function checkAndCoolPitHeat(force)
    if not isChapter2() then return true end
    if not (Config.AutoCoolFire or force) then return true end
    if isCoolingActive or isDeliveryMinigameActive() then return true end
    
    local map = workspace:FindFirstChild("Map")
    local fire = map and map:FindFirstChild("Fire")
    local water = map and map:FindFirstChild("Water")
    if not (fire and water) then return true end
    
    local heat = fire:GetAttribute("Heat") or 0
    local isFlowing = water:GetAttribute("Flowing") == true
    local threshold = force and 0.0 or 0.35
    
    if (heat >= threshold or force) and not isFlowing then
        isCoolingActive = true
        local char = LocalPlayer and LocalPlayer.Character
        local hrp = getHRP()
        local head = char and char:FindFirstChild("Head")
        local cam = workspace.CurrentCamera
        local prevCFrame = char and char:GetPivot() or (hrp and hrp.CFrame)
        local prevCamCFrame = cam and cam.CFrame
        
        RuntimeState.CurrentPhase = string.format("Cooling Pit Fire (Heat: %.0f%%)...", heat * 100)
        
        -- Guarantee ProximityPromptService is active
        pcall(function()
            game:GetService("ProximityPromptService").Enabled = true
        end)
        
        local terminal = (water and water:FindFirstChild("WaterTerminal")) or (map and map:FindFirstChild("WaterTerminal", true))
        local button = (terminal and terminal:FindFirstChild("Button", true))
            or (terminal and terminal:FindFirstChildWhichIsA("BasePart", true))
            or (water and water:FindFirstChild("Button", true))
        local prompt = (button and button:FindFirstChildOfClass("ProximityPrompt"))
            or (terminal and terminal:FindFirstChildWhichIsA("ProximityPrompt", true))
            or (water and water:FindFirstChildWhichIsA("ProximityPrompt", true))
            
        local btnPos = (prompt and prompt.Parent and prompt.Parent:IsA("BasePart") and prompt.Parent.Position)
            or (button and button.Position)
            or Vector3.new(148.60, 130.15, 1195.69)
        local standPos = Vector3.new(btnPos.X, 129.21, btnPos.Z + 2.5)
        
        local restoreState = function()
            pcall(function()
                if char and prevCFrame then
                    char:PivotTo(prevCFrame)
                elseif hrp and prevCFrame then
                    hrp.CFrame = prevCFrame
                end
                if cam then
                    local fwd = hrp and (hrp.CFrame.LookVector * Vector3.new(1, 0, 1)).Unit or Vector3.new(0, 0, -1)
                    local eye = head and head.Position or (hrp and hrp.Position + Vector3.new(0, 1.5, 0)) or (cam.CFrame.Position)
                    cam.CameraType = Enum.CameraType.Scriptable
                    cam.CFrame = CFrame.lookAt(eye, eye + fwd)
                    task.wait(0.04)
                    cam.CameraType = Enum.CameraType.Custom
                    local hum = getHumanoid()
                    if hum and cam.CameraSubject ~= hum then
                        cam.CameraSubject = hum
                    end
                end
            end)
            task.wait(0.15)
            isCoolingActive = false
            ensureControlsAndCamera()
        end
        
        pcall(function()
            if char then
                char:PivotTo(CFrame.new(standPos, Vector3.new(btnPos.X, standPos.Y, btnPos.Z)))
            elseif hrp then
                hrp.CFrame = CFrame.new(standPos, btnPos)
            end
            task.wait(0.06)
            
            -- Look directly at the water valve button while interacting with water cooler
            if cam and btnPos then
                local eyePos = (head and head.Position) or (char and char:GetPivot().Position + Vector3.new(0, 1.8, 0)) or (standPos + Vector3.new(0, 1.8, 0))
                cam.CameraType = Enum.CameraType.Scriptable
                cam.CFrame = CFrame.lookAt(eyePos, btnPos)
            end
            task.wait(0.12)
            
            -- ProximityPrompt interaction (Rule H) without physical keyboard
            if typeof(fireproximityprompt) == "function" and prompt then
                pcall(function() fireproximityprompt(prompt, 0) end)
            end
            pcall(function()
                if prompt then prompt:InputHoldBegin() end
            end)
            
            task.wait((prompt and prompt.HoldDuration) or 5.0)
            
            pcall(function()
                if prompt then prompt:InputHoldEnd() end
            end)
            task.wait(0.4)
            
            -- Fallback retry if heat didn't drop
            local checkHeat = fire:GetAttribute("Heat") or 0
            if checkHeat >= 0.25 and prompt and prompt.Enabled then
                if typeof(fireproximityprompt) == "function" then
                    pcall(function() fireproximityprompt(prompt, 0) end)
                end
                pcall(function() prompt:InputHoldBegin() end)
                task.wait((prompt and prompt.HoldDuration) or 5.0)
                pcall(function() prompt:InputHoldEnd() end)
                task.wait(0.4)
            end
            
            local newHeat = fire:GetAttribute("Heat") or 0
            if Window then
                Window:Notify({ Title = "Pit Cooled 💧", Desc = string.format("Water valve activated! Pit heat reset to %.0f%%.", newHeat * 100), Time = 3.5 })
            end
        end)
        
        restoreState()
    end
    return true
end

-- Dedicated Chapter 2 Background Heat Guardian
task.spawn(function()
    while ScriptRunning do
        if isChapter2() and Config.AutoCoolFire and not isCoolingActive then
            if not (PeelingController and PeelingController.IsPeeling) then
                local map = workspace:FindFirstChild("Map")
                local fire = map and map:FindFirstChild("Fire")
                local heat = fire and fire:GetAttribute("Heat") or 0
                if heat >= 0.50 then
                    checkAndCoolPitHeat()
                end
            end
        end
        task.wait(2)
    end
end)

-- [ENGINE 2]: GATHER POTATOES AT PILE
local function gatherPotatoesUntilFull(minAmount)
    if isDeliveryMinigameActive() or isDelivering then return true end
    local curCount = getBagCount()
    local cap = getBagCapacity()
    if curCount < cap - 2 then
        BagFullReported = false
        LastBagFullTriggerTime = 0
    end
    local full, audit = isBagFull(getMainHarvestTool())
    local targetCapacity = minAmount or audit.capacity
    if audit.count >= targetCapacity or (not minAmount and full) or full then
        RuntimeState.CurrentPhase = string.format("Bag Full (%d/%d) - Skipping Gather", audit.count, audit.capacity)
        return true
    end
    
    local bestSpot, inReach = calculateBestPotatoSpot(18)
    RuntimeState.CurrentPhase = string.format("Positioning at Sweet Spot (%d in reach)...", inReach)
    moveToPosition(bestSpot, true)
    task.wait(0.2)
    
    -- Ensure player is holding their primary harvesting tool (Shovel/Hand)
    local mainTool = getMainHarvestTool()
    ToolEquipController.Equip(mainTool)
    task.wait(0.15)
    
    RuntimeState.CurrentPhase = "Gathering Potatoes..."
    local hrp = getHRP()
    if not hrp then return false end
    
    local maxWait = 45
    local startTime = os.clock()
    local failedPicks = {} -- Track failed attempts per potato ID: [id] = failCount
    local noGainStreak = 0 -- Consecutive picks that added nothing (server rejecting = bag effectively full)
    
    while (Config.MasterFarm or isTutorialRunning) and os.clock() - startTime < maxWait do
        if isDeliveryMinigameActive() or isDelivering then
            ToolEquipController.SetUsing(false)
            break
        end
        while isCoolingActive do
            ToolEquipController.SetUsing(false)
            task.wait(0.2)
        end
        full, audit = isBagFull(getMainHarvestTool())
        if audit.count >= targetCapacity or (not minAmount and full) or full then
            ToolEquipController.SetUsing(false)
            break
        end
        
        -- Chapter 2: Pit Fire Heat Check during prolonged harvesting
        if isChapter2() and Config.AutoCoolFire then
            local map = workspace:FindFirstChild("Map")
            local fire = map and map:FindFirstChild("Fire")
            if fire and (fire:GetAttribute("Heat") or 0) >= 0.40 then
                ToolEquipController.SetUsing(false)
                checkAndCoolPitHeat()
                moveToPosition(bestSpot, true)
                task.wait(0.2)
            end
        end
        
        mainTool = getMainHarvestTool()
        
        -- Find highest rarity potato in reach (Rainbow > Blue/Diamond > Gold > Normal)
        -- Filters:
        -- 1. Ignore potatoes that failed to be collected 2+ times
        -- 2. Reject potatoes that are too far down vertically (dy < -5.5 studs below player's feet)
        -- 3. Require potato to be exposed on the surface (IsExposed), or fallback if none found
        local potatoMap = getPotatoPileMap()
        local pileData = getPileData()
        local bestId, bestWeight, bestDist = nil, -1, math.huge
        local currentHRP = getHRP()
        local hrpPos = currentHRP and currentHRP.Position or hrp.Position
        
        -- Primary Pass: Exposed surface potatoes within reachable height (dy >= -5.5 and dy <= 5)
        for part, id in pairs(potatoMap) do
            if part and part.Parent and (failedPicks[id] or 0) < 2 then
                local dist = (part.Position - hrpPos).Magnitude
                local dy = part.Position.Y - hrpPos.Y
                
                if dist <= 18 and dy >= -5.5 and dy <= 5 then
                    local slot = pileData and PotatoPileShared.GetPotatoSlot(pileData, id)
                    local isExposed = (not slot) or PotatoPileShared.IsExposed(pileData, slot)
                    
                    if isExposed then
                        local kind = (pileData and PotatoPileShared.GetKind(pileData, id)) or 1
                        local weight = 100
                        if kind == 5 then
                            weight = 1000000 -- Key Potato
                        elseif kind == 4 then
                            weight = 100000  -- Rainbow (10x Multiplier)
                        elseif kind == 3 then
                            weight = 50000   -- Diamond / Blue (6x Multiplier)
                        elseif kind == 2 then
                            weight = 10000   -- Golden (3x Multiplier)
                        elseif kind == 1 then
                            weight = 100     -- Normal (1x Multiplier)
                        elseif kind == 6 then
                            weight = 10      -- Charred (0.5x Multiplier)
                        end
                        
                        if weight > bestWeight or (weight == bestWeight and dist < bestDist) then
                            bestWeight = weight
                            bestDist = dist
                            bestId = id
                        end
                    end
                end
            end
        end
        
        -- Fallback Pass: If no exposed potato found, pick any non-blacklisted potato within reachable height
        if not bestId then
            for part, id in pairs(potatoMap) do
                if part and part.Parent and (failedPicks[id] or 0) < 2 then
                    local dist = (part.Position - hrpPos).Magnitude
                    local dy = part.Position.Y - hrpPos.Y
                    if dist <= 18 and dy >= -7 and dy <= 6 then
                        if dist < bestDist then
                            bestDist = dist
                            bestId = id
                        end
                    end
                end
            end
        end
        
        -- DYNAMITE CHECK: If off cooldown, throw dynamite once, then swap back to shovel/hand
        if isDynamiteReady() and bestId then
            throwDynamiteAtPile(bestId)
            task.wait(0.2)
            full, audit = isBagFull(mainTool)
            if audit.count >= targetCapacity or (not minAmount and full) then
                ToolEquipController.SetUsing(false)
                break
            end
        end
        
        -- POTATO SUCKER (P0T4T0-3000) & MAIN TOOL CONTROLLER:
        -- P0T4T0-3000 is an automatic vacuum that must be HELD (SetUsing(true)).
        -- If it overheats, temporarily swap to Shovel while it rapidly cools down!
        local currentActiveTool = mainTool
        if mainTool == "PotatoSucker" then
            if ToolEquipController.IsOverheated() or not ToolEquipController.HasCharge() then
                ToolEquipController.SetUsing(false)
                -- Temporary fallback to Shovel during cooldown
                if ToolEquipController.OwnsTool("Shovel") then
                    currentActiveTool = "Shovel"
                    if ToolEquipController.GetEquippedTool() ~= "Shovel" then
                        ToolEquipController.Equip("Shovel")
                        task.wait(0.15)
                    end
                else
                    task.wait(0.3)
                end
            else
                if ToolEquipController.GetEquippedTool() ~= "PotatoSucker" then
                    ToolEquipController.Equip("PotatoSucker")
                    task.wait(0.15)
                end
                ToolEquipController.SetUsing(true)
            end
        else
            ToolEquipController.SetUsing(false)
            if ToolEquipController.GetEquippedTool() ~= mainTool then
                ToolEquipController.Equip(mainTool)
                task.wait(0.15)
            end
        end
        
        -- Respect tool cooldown for active tool
        local remCooldown, pickInterval = PotatoPileController.GetPickCooldown(currentActiveTool)
        if remCooldown and remCooldown > 0 then
            task.wait(math.min(remCooldown, 0.2))
        end
        
        -- Re-verify bag status before firing pick packet to prevent "Your bag is full"
        full, audit = isBagFull(currentActiveTool)
        if audit.count >= targetCapacity or (not minAmount and full) or full or BagFullReported then
            RuntimeState.CurrentPhase = string.format("Bag Full (%d/%d)", audit.count, audit.capacity)
            ToolEquipController.SetUsing(false)
            break
        end
        
        if bestId then
            local countBefore = audit.count
            pcall(function()
                Packets.PotatoPick:Fire(bestId)
            end)
            
            local waitTime = (currentActiveTool == "PotatoSucker") and math.max(pickInterval or 0.25, 0.15) or math.max(pickInterval or 0.4, 0.1)
            task.wait(waitTime)
            
            -- Verify if the potato was successfully picked
            local newAudit = getInventoryAudit()
            local gained = newAudit.count - countBefore
            if gained > 0 then
                -- Succeeded! Clear failure count and remember this tool's real pick yield
                failedPicks[bestId] = nil
                noGainStreak = 0
                if currentActiveTool ~= "PotatoSucker" then
                    recordPickGain(currentActiveTool, gained)
                end
            else
                -- Pick failed / rejected / out of reach
                failedPicks[bestId] = (failedPicks[bestId] or 0) + 1
                noGainStreak = noGainStreak + 1
                
                -- Breakout immediately if game says "Your bag is full"
                local recentlyRejected = BagFullReported or (os.clock() - LastBagFullTriggerTime < 2.0)
                if recentlyRejected then
                    RuntimeState.CurrentPhase = string.format("Bag Full (%d/%d) - Next pick rejected", newAudit.count, newAudit.capacity)
                    ToolEquipController.SetUsing(false)
                    break
                end
            end
            
            full, newAudit = isBagFull(currentActiveTool)
            if newAudit.count >= targetCapacity or (not minAmount and full) or full or BagFullReported then
                RuntimeState.CurrentPhase = string.format("Bag Full (%d/%d)", newAudit.count, newAudit.capacity)
                ToolEquipController.SetUsing(false)
                break
            end
        else
            local newSpot, newReach = calculateBestPotatoSpot(18)
            if newReach > 0 and (newSpot - hrpPos).Magnitude > 3 then
                bestSpot = newSpot
                moveToPosition(bestSpot, true)
            else
                moveToPosition(cachedSweetSpot, true)
            end
            task.wait(0.3)
        end
    end
    
    -- Release hold and equip best main tool
    ToolEquipController.SetUsing(false)
    ToolEquipController.Equip(getMainHarvestTool())
    return (minAmount and audit.count >= minAmount) or isBagMaxed()
end

-- Helper: Check remaining potatoes in the cutting table area (PotatoCrate / PotatoArea + Tables)
local function getCuttingTablePotatoCount()
    local map = workspace:FindFirstChild("Map")
    local machines = map and map:FindFirstChild("PeelingMachines")
    if not machines then return 0 end
    
    local total = 0
    local crate = machines:FindFirstChild("PotatoCrate")
    local area = crate and crate:FindFirstChild("PotatoArea")
    if area then
        local count = area:GetAttribute("PotatoCount") or 0
        total = total + count
    end
    
    for _, child in ipairs(machines:GetChildren()) do
        if child.Name:find("PeelingTable") then
            local count = (PotatoTableShared and PotatoTableShared.GetCount and PotatoTableShared.GetCount(child)) or 0
            total = total + count
        end
    end
    
    return total
end

local function isCuttingTableAreaEmpty()
    return getCuttingTablePotatoCount() <= 0
end

-- [ENGINE 3]: PEEL AT TABLE (BATCH)
local function peelBatchAtTable()
    if isDeliveryMinigameActive() then return true end
    local audit = getInventoryAudit()
    local tableCount = getCuttingTablePotatoCount()
    if audit.isEmpty and tableCount <= 0 then return true end
    
    RuntimeState.CurrentPhase = "Moving to Peeling Stand..."
    local standPos, prompt = getBestPeelStand()
    moveToPosition(standPos)
    task.wait(0.25)
    
    -- Trigger peel prompt to enter minigame
    if not PeelingController.IsPeeling then
        if prompt and prompt.Enabled then
            safeFirePrompt(prompt)
            task.wait(0.4)
        end
    end
    
    RuntimeState.CurrentPhase = isChapter3() and "Fast Packing In Progress..." or ((PotatoTableShared.IS_CUTTING and "Fast Cutting In Progress...") or "Fast Peeling In Progress...")
    local timeout = 60
    local startPeelTime = os.clock()
    
    if isChapter3() and PackingController then
        PackingController.SetAuto(true)
        PackingController.SetHeld(true)
    elseif PotatoTableShared.IS_CUTTING and CuttingController then
        CuttingController.SetAuto(true)
        CuttingController.SetHeld(true)
    end
    
    while (Config.MasterFarm or isTutorialRunning) and os.clock() - startPeelTime < timeout do
        if PeelingController.IsPeeling then
            if isChapter3() and PackingController then
                local pot = PackingController.GetPotato()
                if pot then
                    peelActivePotatoOnce()
                    task.wait(Config.SliceDelay > 0 and Config.SliceDelay or 0.08)
                else
                    task.wait(0.25)
                    local curAudit = getInventoryAudit()
                    if curAudit.isEmpty and getCuttingTablePotatoCount() <= 0 and PackingController.GetPotato() == nil then
                        break
                    end
                end
            elseif PotatoTableShared.IS_CUTTING and CuttingController then
                local pot = CuttingController.GetPotato()
                if pot then
                    peelActivePotatoOnce()
                    task.wait(Config.SliceDelay > 0 and Config.SliceDelay or 0.02)
                else
                    task.wait(0.12)
                    local curAudit = getInventoryAudit()
                    if curAudit.isEmpty and getCuttingTablePotatoCount() <= 0 and CuttingController.GetPotato() == nil then
                        break
                    end
                end
            else
                peelActivePotatoOnce()
                task.wait(0.08)
            end
        else
            local curAudit = getInventoryAudit()
            local remainingTableCount = getCuttingTablePotatoCount()
            if (not curAudit.isEmpty) or remainingTableCount > 0 then
                standPos, prompt = getBestPeelStand()
                moveToPosition(standPos)
                task.wait(0.2)
                if prompt and prompt.Enabled then
                    safeFirePrompt(prompt)
                    task.wait(0.4)
                end
            else
                break
            end
        end
    end
    
    if isChapter3() and PackingController then
        pcall(function()
            PackingController.SetHeld(false)
            PackingController.SetAuto(false)
        end)
    elseif PotatoTableShared.IS_CUTTING and CuttingController then
        pcall(function()
            CuttingController.SetHeld(false)
            CuttingController.SetAuto(false)
        end)
    end
    
    -- Cleanly exit peeling station if still inside
    if PeelingController and PeelingController.IsPeeling then
        pcall(function()
            local pGui = LocalPlayer and (LocalPlayer:FindFirstChild("PlayerGui") or LocalPlayer:FindFirstChildOfClass("PlayerGui"))
            local PeelingGui = pGui and pGui:FindFirstChild("Peeling")
            local cancelBtn = PeelingGui and PeelingGui:FindFirstChild("Cancel")
            if cancelBtn and cancelBtn.Visible and typeof(getconnections) == "function" then
                for _, conn in ipairs(getconnections(cancelBtn.Activated)) do
                    pcall(function() conn:Fire() end)
                end
            end
            PeelingController.StopPeeling()
        end)
    end
    protectAndRestoreGuis()
    pcall(ensureControlsAndCamera)
    if isBagEmpty() then
        LastBagFullTriggerTime = 0
        BagFullReported = false
    end
    
    return true
end

-- [ENGINE 3.5]: PEEL AT HAND CRANKER MACHINE (LEVEL >= 2)
local function isHandCrankerLevel2()
    if isChapter3() then return false end -- Chapter 3 uses packing tables, NEVER hand peeler!
    local state = getPlayerState()
    if not (state and state.HasHandPeeler) then return false end
    local upgTable = state.Upgrades or {}
    local peelAmountLvl = upgTable["HandPeeler_PeelAmount"] or 0
    return peelAmountLvl >= 2
end

local function peelBatchWithHandCranker()
    if isChapter3() then return false end -- Never run hand cranker in Chapter 3
    -- Simple Check: Ensure all potatoes in cutting table area are fully empty before moving to manual cutter
    if not isCuttingTableAreaEmpty() then
        peelBatchAtTable()
        if not isCuttingTableAreaEmpty() then
            return false -- Wait until fully cleared before proceeding to manual cutter
        end
    end

    local audit = getInventoryAudit()
    local map = workspace:FindFirstChild("Map")
    local machines = map and map:FindFirstChild("PeelingMachines")
    local machine = machines and machines:FindFirstChild("PeelingMachine")
    if not machine then return false end
    
    local cube = machine:FindFirstChild("Hopper") or machine:FindFirstChild("Cube.033")
    local loadPrompt = cube and (cube:FindFirstChild("Load") or cube:FindFirstChildWhichIsA("ProximityPrompt", true))
    local crankPart = machine:FindFirstChild("CrankPromptPart") or machine:FindFirstChild("Crank")
    local crankPrompt = crankPart and (crankPart:FindFirstChild("Crank") or crankPart:FindFirstChildWhichIsA("ProximityPrompt", true))
    
    local inMachineCount = cube and PotatoTableShared.GetCount(cube) or 0
    if audit.isEmpty and inMachineCount <= 0 then return true end
    
    RuntimeState.CurrentPhase = "Moving to Hand Cranker..."
    moveToPosition(Landmarks.PeelingMachine)
    task.wait(0.25)
    
    -- Load all potatoes from bag into Hand Cranker
    if not audit.isEmpty then
        RuntimeState.CurrentPhase = "Loading Potatoes into Hand Cranker..."
        if loadPrompt and loadPrompt.Enabled then
            safeFirePrompt(loadPrompt)
            task.wait(0.35)
        end
    end
    
    RuntimeState.CurrentPhase = "Cranking Hand Peeler..."
    local startCrankTime = os.clock()
    local crankTimeout = 50
    
    while (Config.MasterFarm or isTutorialRunning) and os.clock() - startCrankTime < crankTimeout do
        inMachineCount = cube and PotatoTableShared.GetCount(cube) or 0
        audit = getInventoryAudit()
        
        -- If player still has unpeeled potatoes in bag, load them
        if not audit.isEmpty and loadPrompt and loadPrompt.Enabled then
            safeFirePrompt(loadPrompt)
            task.wait(0.2)
        end
        
        if inMachineCount <= 0 and audit.isEmpty then
            break
        end
        
        if crankPrompt and crankPrompt.Enabled then
            safeFirePrompt(crankPrompt)
            RuntimeState.TotalPotatoesPeeled = RuntimeState.TotalPotatoesPeeled + 3
            task.wait(0.12)
        else
            task.wait(0.1)
        end
    end
    
    RuntimeState.CurrentPhase = "Hand Cranking Complete!"
    return true
end

-- Chapter 3 Truck & Pallet Automation Helpers
local function getMyTruck()
    local map = workspace:FindFirstChild("Map")
    local trucks = map and map:FindFirstChild("Trucks")
    if not trucks then return nil end
    local myUserId = LocalPlayer.UserId
    for _, truck in ipairs(trucks:GetChildren()) do
        if truck:IsA("Model") and truck:GetAttribute(DeliveryShared and DeliveryShared.TRUCK_OWNER_ATTRIBUTE or "TruckOwner") == myUserId then
            return truck
        end
    end
    for _, truck in ipairs(trucks:GetChildren()) do
        if truck:IsA("Model") and truck:FindFirstChild("DeliveryPrompt", true) then
            return truck
        end
    end
    return nil
end

local function getTruckCrateCount()
    local truck = getMyTruck()
    if not truck then return 0, 10, false end
    local cap = truck:GetAttribute(DeliveryShared and DeliveryShared.TRUCK_CAPACITY_ATTRIBUTE or "TruckCapacity") or 10
    local crates = truck:GetAttribute(DeliveryShared and DeliveryShared.TRUCK_CRATES_ATTRIBUTE or "TruckCrates")
    local count = (type(crates) == "string" and #crates) or 0
    return count, cap, count >= cap
end

local function isTruckFull()
    local crateCount, capacity, full = getTruckCrateCount()
    return full
end

local function getPalletCrateCount(palletPart)
    if not palletPart then return 0 end
    local count = (PotatoTableShared and PotatoTableShared.GetCount and PotatoTableShared.GetCount(palletPart))
    if count and count > 0 then return count end
    local attr = palletPart:GetAttribute(DeliveryShared and DeliveryShared.PALLET_CRATES_ATTRIBUTE or "PalletCrates")
    if type(attr) == "string" then return #attr end
    return 0
end

local function loadPalletCratesToTruck()
    if isDeliveryMinigameActive() or isDelivering then return false end
    -- Check if truck is already full (10/10 crates)
    if isTruckFull() then return false end
    
    local map = workspace:FindFirstChild("Map")
    local machines = map and map:FindFirstChild("PeelingMachines")
    if not machines then return false end
    
    -- Load ONLY from the normal central Pallet (completely ignores hand and auto peeler pallets)
    local centralPallet = machines:FindFirstChild("Pallet")
    local palletPart = centralPallet and (centralPallet:FindFirstChild("PotatoArea") or centralPallet)
    if not palletPart then return false end
    
    local prompt = palletPart:FindFirstChildWhichIsA("ProximityPrompt", true)
    if prompt and prompt.Enabled and (getPalletCrateCount(palletPart) > 0 or prompt.ActionText:upper():find("LOAD")) then
        RuntimeState.CurrentPhase = "Loading Crates into Truck..."
        local pPos = (prompt.Parent:IsA("Attachment") and prompt.Parent.WorldPosition) or (prompt.Parent:IsA("BasePart") and prompt.Parent.Position) or palletPart.Position
        moveToPosition(pPos + Vector3.new(0, 2, 0))
        task.wait(0.2)
        safeFirePrompt(prompt)
        task.wait(0.35)
        return true
    end
    return false
end

local deliverTruck = nil

if Packets and Packets.DeliveryStarted then
    trackConn(Packets.DeliveryStarted.OnClientEvent:Connect(function(truckName, seed, totalBoxes, boxesPerStop, crates)
        pcall(function()
            if DeliveryShared and DeliveryShared.GetPlan then
                activeDeliveryPlan = DeliveryShared.GetPlan(seed, totalBoxes, boxesPerStop)
            end
            if not isDelivering and deliverTruck then
                task.spawn(function()
                    deliverTruck()
                end)
            end
        end)
    end))
end

if Packets and Packets.DeliveryPaid then
    trackConn(Packets.DeliveryPaid.OnClientEvent:Connect(function(cash)
        activeDeliveryPlan = nil
    end))
end

if Packets and Packets.DeliveryLeave then
    trackConn(Packets.DeliveryLeave.OnClientEvent:Connect(function()
        activeDeliveryPlan = nil
    end))
end

deliverTruck = function()
    if isDelivering then return false end
    isDelivering = true
    
    local alreadyActive = (DeliveryController and typeof(DeliveryController.IsActive) == "function" and DeliveryController.IsActive())
        or workspace:FindFirstChild("DeliveryWorld") ~= nil
        or workspace:FindFirstChild("TruckScene") ~= nil
    
    if not alreadyActive then
        local truck = getMyTruck()
        if not truck then
            isDelivering = false
            return false
        end
        
        local cratesAttr = truck:GetAttribute(DeliveryShared and DeliveryShared.TRUCK_CRATES_ATTRIBUTE or "TruckCrates")
        local crateCount = (type(cratesAttr) == "string" and #cratesAttr) or 0
        if crateCount <= 0 then
            isDelivering = false
            return false
        end
        
        local promptPart = truck:FindFirstChild("DeliveryPrompt", true) or truck:FindFirstChild("PromptPart", true)
        local prompt = promptPart and promptPart:FindFirstChildWhichIsA("ProximityPrompt", true)
        if not prompt then
            isDelivering = false
            return false
        end
        
        RuntimeState.CurrentPhase = string.format("Starting Truck Delivery (%d Crates)...", crateCount)
        
        -- Move character directly adjacent to DeliveryPrompt part (ensures 0 distance check)
        local targetPos = (promptPart:IsA("BasePart") and promptPart.Position) or (truck.PrimaryPart and truck.PrimaryPart.Position) or truck:GetPivot().Position
        moveToPosition(targetPos + Vector3.new(0, 1.5, 0))
        task.wait(0.25)
        
        local hrp = getHRP()
        if hrp and promptPart:IsA("BasePart") then
            pcall(function()
                hrp.CFrame = CFrame.new(hrp.Position, Vector3.new(promptPart.Position.X, hrp.Position.Y, promptPart.Position.Z))
            end)
        end
        
        safeFirePrompt(prompt)
        firePromptCompat(prompt, 0)
        
        local startWait = os.clock()
        while not (DeliveryController and DeliveryController.IsActive()) and not workspace:FindFirstChild("TruckScene") and not workspace:FindFirstChild("DeliveryWorld") and os.clock() - startWait < 10 do
            if prompt and prompt.Enabled then
                firePromptCompat(prompt, 0)
            end
            task.wait(0.3)
        end
    end
    
    -- Wait for departure cutscene (TruckScene) to finish transition into delivery driving if active
    local depWait = os.clock()
    while not (DeliveryController and DeliveryController.IsActive()) and (workspace:FindFirstChild("TruckScene") or workspace:FindFirstChild("DeliveryWorld")) and os.clock() - depWait < 8 do
        task.wait(0.2)
    end
    
    if (DeliveryController and DeliveryController.IsActive()) or workspace:FindFirstChild("DeliveryWorld") then
        RuntimeState.CurrentPhase = "Truck Delivery Active (Dodging Obstacles & Collecting Yellow Boxes)..."
        local deliveryTimeout = 75
        local deliveryStart = os.clock()
        local Forest = nil
        pcall(function()
            Forest = require(ReplicatedStorage.Modules.DeliveryModes.Forest)
        end)
        
        -- Hook Packets.DeliveryCrash to prevent any crash packets from sending to server
        local origCrashFire = nil
        if Packets and Packets.DeliveryCrash and type(Packets.DeliveryCrash.Fire) == "function" then
            origCrashFire = Packets.DeliveryCrash.Fire
            Packets.DeliveryCrash.Fire = function(...) end
        end
        
        -- Allow departure animation to commence
        task.wait(1.2)
        
        while ((DeliveryController and DeliveryController.IsActive()) or workspace:FindFirstChild("DeliveryWorld") ~= nil) and os.clock() - deliveryStart < deliveryTimeout do
            pcall(function()
                if not Forest then
                    Forest = require(ReplicatedStorage.Modules.DeliveryModes.Forest)
                end
                if not Forest or not Forest.Update then return end
                local forestUps = debug.getupvalues(Forest.Update)
                if not forestUps then return end
                
                local currentDistance = forestUps[1] or 0
                local currentLane = forestUps[11] or 2
                
                -- 1. Query targets list (u13), obstacles list (u14), and pickups list (u15)
                local checkTargetsFn = forestUps[16]
                local targetUps = checkTargetsFn and debug.getupvalues(checkTargetsFn)
                local targetsTable = targetUps and targetUps[1]
                local resolveTargetFn = targetUps and targetUps[3]
                
                -- Neutralize internal controller crash handler if present
                if resolveTargetFn then
                    local rUps = debug.getupvalues(resolveTargetFn)
                    local ctrlInterface = rUps and rUps[1]
                    if ctrlInterface and type(ctrlInterface) == "table" and ctrlInterface.Crash then
                        ctrlInterface.Crash = function(...) end
                    end
                end
                
                local updateObsFn = forestUps[13]
                local obsUps = updateObsFn and debug.getupvalues(updateObsFn)
                local obsTable = obsUps and obsUps[2]
                
                local updatePickupsFn = forestUps[14]
                local pickupUps = updatePickupsFn and debug.getupvalues(updatePickupsFn)
                local pickupsTable = pickupUps and pickupUps[2]
                
                -- 2. God-mode neutralization: prevent any obstacle from dealing damage/hitting
                if obsTable and type(obsTable) == "table" then
                    for _, obs in ipairs(obsTable) do
                        if obs and type(obs) == "table" then
                            obs.IsHit = true -- completely prevents hit(obs) from triggering crash/box loss
                        end
                    end
                end
                
                -- 3. Find closest upcoming yellow reward (Delivery Stop OR Yellow Boost Pickup)
                local nearestReward = nil
                local minRewardDist = math.huge
                
                -- 3A. Yellow Delivery Stops
                if targetsTable and type(targetsTable) == "table" then
                    for i, slot in pairs(targetsTable) do
                        local target = slot and slot.Target
                        local targetDist = i * 26
                        if target and not target.IsDone and targetDist > currentDistance then
                            local d = targetDist - currentDistance
                            if d < minRewardDist then
                                minRewardDist = d
                                nearestReward = {
                                    Type = "Target",
                                    Lane = target.Lane or (target.Side and (target.Side < 0 and 1 or 2)) or 1,
                                    Distance = targetDist,
                                    Obj = target
                                }
                            end
                        end
                    end
                end
                
                -- 3B. Yellow Boost Pickups (Bonus Money & Speed Multiplier)
                if pickupsTable and type(pickupsTable) == "table" then
                    for _, pickup in ipairs(pickupsTable) do
                        if pickup and not pickup.IsTaken and pickup.Distance and pickup.Distance > currentDistance then
                            local d = pickup.Distance - currentDistance
                            if d < minRewardDist then
                                minRewardDist = d
                                nearestReward = {
                                    Type = "Pickup",
                                    Lane = pickup.Lane or 1,
                                    Distance = pickup.Distance,
                                    Obj = pickup
                                }
                            end
                        end
                    end
                end
                
                -- 4. Find closest upcoming obstacle
                local nearestObs = nil
                local minObsDist = math.huge
                if obsTable and type(obsTable) == "table" then
                    for _, obs in ipairs(obsTable) do
                        if obs and obs.Distance and obs.Distance > currentDistance then
                            local d = obs.Distance - currentDistance
                            if d < minObsDist then
                                minObsDist = d
                                nearestObs = obs
                            end
                        end
                    end
                end
                
                -- 5. Autonomous Steering Decision Engine:
                local desiredLane = currentLane
                
                -- Case A: Imminent obstacle directly in current lane (< 35 studs)
                -- and no reward right at this exact spot (< 20 studs in current lane)
                if nearestObs and minObsDist < 35 and nearestObs.Lane == currentLane and (not nearestReward or minRewardDist > minObsDist + 8) then
                    -- Dodge oncoming obstacle!
                    desiredLane = (currentLane == 1) and 2 or 1
                -- Case B: Approaching a yellow box (Delivery Stop or Boost Pickup within 120 studs)
                elseif nearestReward and minRewardDist < 120 then
                    desiredLane = nearestReward.Lane
                -- Case C: Obstacle ahead within 45 studs in our current lane with no reward ahead
                elseif nearestObs and minObsDist < 45 and nearestObs.Lane == currentLane then
                    desiredLane = (currentLane == 1) and 2 or 1
                end
                
                -- Execute Steering Change
                if desiredLane and desiredLane ~= currentLane then
                    if Forest.OnDirection and DeliveryShared then
                        Forest.OnDirection(desiredLane == 1 and DeliveryShared.Directions.Left or DeliveryShared.Directions.Right)
                    end
                    pcall(function()
                        debug.setupvalue(Forest.Update, 11, desiredLane)
                    end)
                end
                
                -- Proactive Delivery Resolution / Boost Pickup Guarantee
                if nearestReward then
                    if nearestReward.Type == "Target" and minRewardDist < 35 and nearestReward.Obj.StopIndex then
                        pcall(function()
                            Packets.DeliveryResolve:Fire(nearestReward.Obj.StopIndex, true)
                        end)
                    elseif nearestReward.Type == "Pickup" and minRewardDist < 10 then
                        pcall(function()
                            nearestReward.Obj.IsTaken = true
                        end)
                    end
                end
            end)
            task.wait(0.06)
        end
        
        -- Restore original crash fire handler if hooked
        if origCrashFire and Packets and Packets.DeliveryCrash then
            pcall(function()
                Packets.DeliveryCrash.Fire = origCrashFire
            end)
        end
        
        local returnWait = os.clock()
        while (workspace:FindFirstChild("TruckScene") ~= nil or (DeliveryController and DeliveryController.IsActive())) and os.clock() - returnWait < 8 do
            task.wait(0.2)
        end
        task.wait(0.5)
        restoreGameGuis()
        RuntimeState.CurrentPhase = "Truck Delivery Complete!"
    end
    
    isDelivering = false
    activeDeliveryPlan = nil
    restoreGameGuis()
    return true
end

local function handleChapter3Selling()
    if isDeliveryMinigameActive() or isDelivering then return true end
    -- Step 1: Load waiting crates from normal central Pallet into Truck (only if truck is not full)
    if (Config.AutoLoadTruckPallet or Config.MasterFarm) and not isTruckFull() then
        loadPalletCratesToTruck()
    end
    
    -- Step 2: Deliver truck if it is full (e.g. 10/10 crates) OR if AutoDeliverTruck toggle is enabled
    local crateCount, capacity, isFull = getTruckCrateCount()
    if crateCount > 0 and (isFull or Config.AutoDeliverTruck) then
        deliverTruck()
        return true
    end
    return true
end

-- [ENGINE 4]: SELL PEELED POTATOES
local function sellPeeledPotatoes()
    -- Chapter 3: Pallet Loading & Military Truck Delivery Engine (Run immediately without bag check)
    if isChapter3() then
        return handleChapter3Selling()
    end
    
    -- Inventory Check 1: If player still holds raw potatoes in bag, do NOT sell! They must be peeled first.
    if not isBagEmpty() then
        return false
    end
    
    local map = workspace:FindFirstChild("Map")
    local sellCrate = map and map:FindFirstChild("PeelingMachines") and map.PeelingMachines:FindFirstChild("SellCrate")
    local potatoArea = sellCrate and sellCrate:FindFirstChild("PotatoArea")
    local prompt = (potatoArea and potatoArea:FindFirstChild("SellPrompt")) 
        or (sellCrate and sellCrate:FindFirstChildWhichIsA("ProximityPrompt", true))
    
    -- Inventory Check 2: Verify if there are peeled potatoes waiting in the Sell Crate
    local hasItemsToSell = false
    if prompt and prompt.Enabled then
        hasItemsToSell = true
    elseif potatoArea and TutorialStackShared and TutorialStackShared.GetCount then
        hasItemsToSell = (TutorialStackShared.GetCount(potatoArea) or 0) > 0
    end
    
    if not hasItemsToSell then
        return false
    end
    
    RuntimeState.CurrentPhase = "Selling Peeled Potatoes..."
    moveToPosition(Landmarks.SellCrate)
    task.wait(0.3)
    
    if prompt and prompt.Enabled then
        safeFirePrompt(prompt)
        task.wait(0.4)
    end
    LastBagFullTriggerTime = 0
    BagFullReported = false
    
    return true
end

-- [ENGINE 5]: KEY RADAR & ESCAPE
local function findEscapePrompt()
    if isChapter3() then
        local map = workspace:FindFirstChild("Map")
        local escape = map and map:FindFirstChild("Escape")
        local tracker = escape and escape:FindFirstChild("Tracker") and escape.Tracker:FindFirstChild("Tracker")
        local trackerBox = tracker and tracker:FindFirstChild("TrackerBox")
        local prompt = trackerBox and trackerBox:FindFirstChildWhichIsA("ProximityPrompt", true)
        if prompt then return prompt end
    elseif isChapter2() then
        local cutscene = workspace:FindFirstChild("Map") and workspace.Map:FindFirstChild("Cutscene")
        local crane = cutscene and cutscene:FindFirstChild("WreckingBallCrane")
        local prompt = crane and crane:FindFirstChildWhichIsA("ProximityPrompt", true)
        if prompt then return prompt end
    else
        local map = workspace:FindFirstChild("Map")
        local inside = map and map:FindFirstChild("Map") and map.Map:FindFirstChild("InsideBunker")
        local vault = inside and inside:FindFirstChild("VaultDoor")
        local prompt = vault and vault:FindFirstChildWhichIsA("ProximityPrompt", true)
        if prompt then return prompt end
    end
    -- Fallback across workspace
    for _, desc in ipairs(workspace:GetDescendants()) do
        if desc:IsA("ProximityPrompt") and desc.Enabled then
            local act = (desc.ActionText or ""):lower()
            local obj = (desc.ObjectText or ""):lower()
            if act:find("cut") or act:find("drive") or act:find("escape") or act:find("unlock") or obj:find("tracker") or obj:find("crane") or obj:find("vault") or obj:find("door") then
                return desc
            end
        end
    end
    return nil
end

local isEscaping = false
local function checkAutoEscape(force)
    local state = getPlayerState()
    if not (state and state.HasKey) and not force then return false end
    if not (Config.AutoGrabAndEscape or force) then return false end
    if isEscaping then return false end
    isEscaping = true

    local targetName = isChapter3() and "Military Truck Tracker" or (isChapter2() and "Crane" or "Vault Door")
    RuntimeState.CurrentPhase = string.format("ESCAPING VIA %s...", targetName:upper())
    
    local prompt = findEscapePrompt()
    local promptPos = prompt and ((prompt.Parent:IsA("Attachment") and prompt.Parent.WorldPosition) or (prompt.Parent:IsA("BasePart") and prompt.Parent.Position))
    local escapePos = promptPos or Landmarks.VaultDoor
    
    moveToPosition(escapePos)
    task.wait(0.3)
    
    prompt = findEscapePrompt()
    if prompt and prompt.Enabled then
        promptPos = (prompt.Parent:IsA("Attachment") and prompt.Parent.WorldPosition) or (prompt.Parent:IsA("BasePart") and prompt.Parent.Position)
        local hrp = getHRP()
        if hrp and promptPos then
            hrp.CFrame = CFrame.new(hrp.Position, Vector3.new(promptPos.X, hrp.Position.Y, promptPos.Z))
        end
        task.wait(0.2)
        
        local holdTime = (prompt.HoldDuration and prompt.HoldDuration > 0) and (prompt.HoldDuration + 0.35) or 1.3
        
        -- Hold 'E' via VirtualInputManager
        local vim = pcall(function() return game:GetService("VirtualInputManager") end) and game:GetService("VirtualInputManager")
        if vim then
            pcall(function()
                vim:SendKeyEvent(true, Enum.KeyCode.E, false, game)
                task.wait(holdTime)
                vim:SendKeyEvent(false, Enum.KeyCode.E, false, game)
            end)
            task.wait(0.3)
        end
        
        -- Safe fallback triggers
        if prompt and prompt.Enabled then
            safeFirePrompt(prompt, holdTime)
        end
        
        if Window then
            Window:Notify({ Title = "ESCAPED!", Desc = string.format("Successfully escaped via %s!", targetName), Time = 6 })
        end
        task.wait(2)
    end
    isEscaping = false
    return true
end

local function updateKeyTracker()
    local pileData = getPileData()
    local keyId = RuntimeState.KeyDetectedId or (pileData and pileData.KeyPotatoId)
    
    if keyId and keyId ~= 0 then
        RuntimeState.KeyDetectedId = keyId
        RuntimeState.KeyStatus = string.format("Detected (ID: %d) 🔑", keyId)
        
        -- Find MeshPart in pile
        local potatoMap = getPotatoPileMap()
        local keyPart = nil
        for part, id in pairs(potatoMap) do
            if id == keyId and part and part.Parent then
                keyPart = part
                break
            end
        end
        
        if keyPart then
            RuntimeState.KeyPosition = keyPart.Position
            
            -- Auto-Grab Key & Immediately Escape
            if Config.AutoGrabAndEscape then
                local hrp = getHRP()
                if hrp then
                    if (hrp.Position - keyPart.Position).Magnitude > 18 then
                        moveToPosition(keyPart.Position + Vector3.new(0, 3, 0))
                        task.wait(0.2)
                    end
                    pcall(function() Packets.PotatoPick:Fire(keyId) end)
                    task.wait(0.35)
                    checkAutoEscape()
                end
            end
        end
    else
        local state = getPlayerState()
        if state.HasKey then
            RuntimeState.KeyStatus = "KEY IN HAND! 🚪"
            if Config.AutoGrabAndEscape then
                checkAutoEscape()
            end
        end
    end
end

local function isTutorialCompleted()
    local state = getPlayerState()
    local step = (state and state.TutorialStep) or 0
    return tutorialCompletedOnce or step == 0 or step >= 9
end

-- Simulates a safe screen click to dismiss purchase popups/modals ("CLICK ANYWHERE TO CLOSE")
-- STRICT REQUIREMENT: Only executes AFTER tutorial is completed.
clickScreenOnce = function(customDelay)
    if not isTutorialCompleted() then return end
    task.wait(customDelay or 0.65) -- Ample delay for item unlock popup animation to settle

    -- 1. Game Controller Native Close (Direct & Safe)
    pcall(function()
        local svc = _StoreViewController or SafeRequire("Controllers", "StoreViewController")
        if svc and typeof(svc.IsOpen) == "function" and svc.IsOpen() then
            svc.Close()
        end
    end)
    pcall(function()
        local tuc = ToolUpgradeController or SafeRequire("Controllers", "ToolUpgradeController")
        if tuc and typeof(tuc.IsOpen) == "function" and tuc.IsOpen() then
            tuc.SetOpen(false)
        end
    end)

    -- 2. Target ONLY StoreView.Backdrop directly (no generic button iteration)
    pcall(function()
        local pGui = LocalPlayer and LocalPlayer:FindFirstChild("PlayerGui")
        local sv = pGui and pGui:FindFirstChild("StoreView")
        if sv and sv.Enabled then
            local bd = sv:FindFirstChild("Backdrop")
            if bd and bd:IsA("GuiButton") and typeof(getconnections) == "function" then
                for _, conn in ipairs(getconnections(bd.Activated)) do
                    conn:Fire()
                end
            end
        end
    end)

    -- 3. Physical Simulated Click at Safe Off-Center Coordinate
    -- Targets empty 3D space on far left (X: 8%, Y: 25%), well away from center Aether UI, topbar, and HUD
    pcall(function()
        local vim = game:GetService("VirtualInputManager")
        local vp = (workspace.CurrentCamera and workspace.CurrentCamera.ViewportSize) or Vector2.new(1280, 720)
        local safeX = math.clamp(math.floor(vp.X * 0.08), 50, 150)
        local safeY = math.clamp(math.floor(vp.Y * 0.25), 100, 250)
        vim:SendMouseButtonEvent(safeX, safeY, 0, true, game, 0)
        task.wait(0.04)
        vim:SendMouseButtonEvent(safeX, safeY, 0, false, game, 0)
    end)
    pcall(function()
        local vp = (workspace.CurrentCamera and workspace.CurrentCamera.ViewportSize) or Vector2.new(1280, 720)
        local safeX = math.clamp(math.floor(vp.X * 0.08), 50, 150)
        local safeY = math.clamp(math.floor(vp.Y * 0.25), 100, 250)
        VirtualUser:CaptureController()
        VirtualUser:ClickButton1(Vector2.new(safeX, safeY))
    end)
    -- 4. Clean up any leftover CameraController lock flags and restore default first person view
    pcall(function()
        local rep = game:GetService("ReplicatedStorage")
        local ccModule = rep:FindFirstChild("Controllers") and rep.Controllers:FindFirstChild("CameraController")
        if ccModule then
            local ok, cc = pcall(require, ccModule)
            if ok and cc and typeof(cc.SetMouseUnlocked) == "function" then
                cc.SetMouseUnlocked("StoreView", false)
                cc.SetMouseUnlocked("ToolUpgrades", false)
            end
        end
    end)
    pcall(ensurePlayerControls)
end

-- Store item purchase executor using dynamic game packets & buffer synchronization
local function buyStoreItem(stall, itemName, category, cost, noReturn)
    local state = getPlayerState()
    if StoreItemShared.IsOwned(stall, state) then return false end
    if (category == "Peelers" or category == "Knives") and ((state.PeelTools and state.PeelTools[itemName]) or (state.CutTools and state.CutTools[itemName])) then return false end
    if category == "Boxes" and ((state.PackTools and (state.PackTools[itemName] or table.find(state.PackTools, itemName))) or (state.Boxes and state.Boxes[itemName])) then return false end
    if category == "Tools" and state.Tools and (state.Tools[itemName] or (itemName == "P0T4T0-3000" and state.Tools["PotatoSucker"])) then return false end
    
    local hrp = getHRP()
    local prevPos = hrp and hrp.Position
    local prompt = stall:FindFirstChildWhichIsA("ProximityPrompt", true)
    local targetPos = (prompt and prompt.Parent and prompt.Parent:IsA("Attachment") and prompt.Parent.WorldPosition) 
        or (stall.PrimaryPart and stall.PrimaryPart.Position) 
        or (stall:FindFirstChild("Price") and stall.Price.Position)
        
    if targetPos then
        moveToPosition(targetPos + Vector3.new(0, 2, 0))
        task.wait(0.2)
    end
    
    -- 1. Official Game Packet: StorePurchase (dynamically resolves buffer & synced server packet ID)
    pcall(function()
        Packets.StorePurchase:Fire(stall, "Buy")
    end)
    
    -- 2. If it is a peel/cut/tool/box item, also send ToolPurchase
    if category == "Peelers" or category == "Tools" or category == "Knives" or category == "Boxes" then
        pcall(function()
            Packets.ToolPurchase:Fire(itemName)
        end)
    end
    
    -- 3. Fire proximity prompt in case server requires prompt interaction
    if prompt and prompt.Enabled then
        firePromptCompat(prompt)
    end
    
    task.wait(0.35)
    local newState = getPlayerState()
    local success = (stall and StoreItemShared.IsOwned(stall, newState))
        or ((category == "Peelers" or category == "Knives") and ((newState.PeelTools and newState.PeelTools[itemName]) or (newState.CutTools and newState.CutTools[itemName])))
        or (category == "Boxes" and ((newState.PackTools and (newState.PackTools[itemName] or table.find(newState.PackTools, itemName))) or (newState.Boxes and newState.Boxes[itemName])))
        or (category == "Tools" and newState.Tools and (newState.Tools[itemName] or (itemName == "P0T4T0-3000" and newState.Tools["PotatoSucker"])))
        or (category == "Storage" and (newState.CarryLevel or 0) > (state.CarryLevel or 0))
        or (category == "Machines" and ((itemName == "AutoPeeler" and newState.HasAutoPeeler) or (itemName == "HandPeeler" and newState.HasHandPeeler)))

    if not noReturn and prevPos then
        moveToPosition(prevPos)
        task.wait(0.15)
    end
    if success then
        clickScreenOnce()
    end
    return success
end

-- [ENGINE 5.5]: AUTONOMOUS TUTORIAL SOLVER & SPOTLIGHT CLEARER
local function runTutorialSolver()
    if not isChapter1() then return true end
    if tutorialCompletedOnce then return true end
    if isTutorialRunning then return false end
    isTutorialRunning = true

    local function solveStep()
        local state = getPlayerState()
        local step = state.TutorialStep or 0
        if step >= 9 or step == 0 then
            tutorialCompletedOnce = true
            pcall(function()
                if UISpotlightController and UISpotlightController.IsSpotlightActive() then
                    UISpotlightController.RemoveSpotlight()
                end
            end)
            return true
        end

        -- Step 1: COLLECT (Gathers 10 potatoes for a full bag & sufficient funds)
        if step == 1 or ((state.Potatoes or 0) == 0 and step <= 3) then
            local targetCount = (step == 1) and 10 or 5
            RuntimeState.CurrentPhase = string.format("Tutorial Step 1: Gathering %d Potatoes...", targetCount)
            gatherPotatoesUntilFull(targetCount)
            task.wait(0.3)
            state = getPlayerState()
            step = state.TutorialStep or 0
        end

        -- Steps 2 & 3: PLACE & PEEL (Peels all held potatoes at the table)
        if (state.Potatoes or 0) > 0 or step == 2 or step == 3 then
            RuntimeState.CurrentPhase = "Tutorial Steps 2-3: Fast Peeling at Table..."
            peelBatchAtTable()
            task.wait(0.3)
            state = getPlayerState()
            step = state.TutorialStep or 0
        end

        -- Step 4: SELL PEELED POTATOES
        if step == 4 or (state.Cash or 0) < 0.06 then
            RuntimeState.CurrentPhase = "Tutorial Step 4: Selling Peeled Potatoes..."
            sellPeeledPotatoes()
            task.wait(0.5)
            state = getPlayerState()
            step = state.TutorialStep or 0
        end

        -- Step 5: BUY KNIFE ($0.01) -> STAY AT STALL -> WAIT FOR CONFIRMATION
        if step == 5 or (step < 6 and (state.Cash or 0) >= 0.01 and not (state.PeelTools and state.PeelTools.Knife)) then
            RuntimeState.CurrentPhase = "Tutorial Step 5: Purchasing Knife..."
            local map = workspace:FindFirstChild("Map") or workspace:WaitForChild("Map", 3)
            local storeItems = map and (map:FindFirstChild("StoreItems") or map:WaitForChild("StoreItems", 3))
            local peelers = storeItems and (storeItems:FindFirstChild("Peelers") or storeItems:FindFirstChild("Knives"))
            local knifeStall = peelers and (peelers:FindFirstChild("Knife") or peelers:FindFirstChildWhichIsA("Model"))
            if knifeStall then
                local prompt = knifeStall:FindFirstChildWhichIsA("ProximityPrompt", true)
                local targetPos = (prompt and prompt.Parent and prompt.Parent:IsA("Attachment") and prompt.Parent.WorldPosition) 
                    or (knifeStall.PrimaryPart and knifeStall.PrimaryPart.Position) 
                    or (knifeStall:FindFirstChild("Price") and knifeStall.Price.Position)

                local hrp = getHRP()
                if targetPos and hrp then
                    hrp.CFrame = CFrame.new(targetPos + Vector3.new(0, 2, 0))
                    task.wait(0.25)
                end

                pcall(function() Packets.StorePurchase:Fire(knifeStall, "Buy") end)
                pcall(function() Packets.ToolPurchase:Fire("Knife") end)
                if prompt and prompt.Enabled then
                    firePromptCompat(prompt)
                end

                local startWait = os.clock()
                while os.clock() - startWait < 2.0 do
                    task.wait(0.2)
                    state = getPlayerState()
                    step = state.TutorialStep or 0
                    if (state.PeelTools and state.PeelTools.Knife) or step >= 6 then
                        break
                    end
                end
            end
            state = getPlayerState()
            step = state.TutorialStep or 0
        end

        -- Step 6: OPEN UPGRADES -> WAIT FOR CONFIRMATION
        if step == 6 then
            RuntimeState.CurrentPhase = "Tutorial Step 6: Opening Upgrades..."
            pcall(function() ToolUpgradeController.SetOpen(true) end)
            pcall(function() Packets.TutorialUpgradesOpened:Fire() end)
            local startWait = os.clock()
            while os.clock() - startWait < 2.0 do
                task.wait(0.2)
                state = getPlayerState()
                step = state.TutorialStep or 0
                if step >= 7 then break end
            end
        end

        -- Step 7: SELECT HAND TAB -> WAIT FOR CONFIRMATION
        if step == 7 then
            RuntimeState.CurrentPhase = "Tutorial Step 7: Selecting Hand Tab..."
            pcall(function() ToolUpgradeController.SelectTool("Hand") end)
            pcall(function() Packets.TutorialHandSelected:Fire() end)
            local startWait = os.clock()
            while os.clock() - startWait < 2.0 do
                task.wait(0.2)
                state = getPlayerState()
                step = state.TutorialStep or 0
                if step >= 8 then break end
            end
        end

        -- Step 8: BUY PICK SPEED UPGRADE ($0.05) -> CLOSE UPGRADES & CLEAR SPOTLIGHT
        if step == 8 then
            RuntimeState.CurrentPhase = "Tutorial Step 8: Upgrading Pick Speed..."
            state = getPlayerState()
            if (state.Cash or 0) >= 0.05 then
                pcall(function() Packets.ToolUpgradePurchase:Fire("Hand", "PickSpeed") end)
                local startWait = os.clock()
                while os.clock() - startWait < 2.0 do
                    task.wait(0.2)
                    state = getPlayerState()
                    step = state.TutorialStep or 0
                    if step >= 9 then break end
                end
            end

            -- Cleanly close upgrades menu
            pcall(function() ToolUpgradeController.SetOpen(false) end)
            pcall(function()
                if UISpotlightController and UISpotlightController.IsSpotlightActive() then
                    UISpotlightController.RemoveSpotlight()
                end
            end)
        end

        state = getPlayerState()
        step = state.TutorialStep or 0
        if step >= 9 or step == 0 then
            tutorialCompletedOnce = true
            if Window then
                Window:Notify({ Title = "Tutorial Complete!", Desc = "Bought Knife & Pick Speed! Seamlessly continuing autofarm...", Time = 5 })
            end
            task.wait(0.5)
            return true
        end

        return false
    end

    local ok, res = pcall(solveStep)
    isTutorialRunning = false
    return ok and res
end

-- [ENGINE 6]: AUTO-BUY UPGRADES & STORE ITEMS (HIGH RARITY PRIORITY)
local function runAutoPurchases()
    if not Config.AutoBuyAllUpgrades then return end
    
    local state = getPlayerState()
    local step = state.TutorialStep or 0
    -- In Chapter 1 tutorial (steps 1-7), ensure we don't spend below $0.10 if Knife is not yet acquired
    if isChapter1() and step > 0 and step < 8 and not (state.PeelTools and state.PeelTools.Knife) then
        if (state.Cash or 0) < 0.10 then
            return
        end
    end
    local cash = state.Cash or 0
    local upgTable = state.Upgrades or {}
    
    -- 1. AUTO-UPGRADE ALL CATEGORIES (Body, Truck, Tools, Machines)
    local categories = {
        { id = "Body", owns = true },
        { id = "Truck", owns = isChapter3() },
        { id = "PotatoSucker", owns = (state.Tools and state.Tools["PotatoSucker"]) or (ToolEquipController and ToolEquipController.OwnsTool("PotatoSucker")) },
        { id = "Dynamite", owns = (state.Tools and state.Tools["Dynamite"]) or (ToolEquipController and ToolEquipController.OwnsTool("Dynamite")) },
        { id = "Shovel", owns = (state.Tools and state.Tools["Shovel"]) or (ToolEquipController and ToolEquipController.OwnsTool("Shovel")) },
        { id = "Hand", owns = true },
        { id = "AutoPeeler", owns = state.HasAutoPeeler == true },
        { id = "HandPeeler", owns = state.HasHandPeeler == true }
    }
    
    if ToolUpgradeShared then
        for _, cat in ipairs(categories) do
            if cat.owns then
                local obj = ToolUpgradeShared.GetUpgradeable(cat.id)
                if obj and obj.Upgrades then
                    for _, u in ipairs(obj.Upgrades) do
                        -- Ignore WalkSpeed and Luck on Body per original design
                        if not (cat.id == "Body" and (u.Id == "WalkSpeed" or u.Id == "Luck")) then
                            local curLvl = upgTable[cat.id .. "_" .. u.Id] or 0
                            local maxLvl = ToolUpgradeShared.GetMaxLevel(u)
                            if curLvl < maxLvl then
                                local cost = ToolUpgradeShared.GetNextCost(LocalPlayer, cat.id, u, curLvl)
                                if cost and cash >= cost then
                                    pcall(function() Packets.ToolUpgradePurchase:Fire(cat.id, u.Id) end)
                                    cash = cash - cost
                                    upgTable[cat.id .. "_" .. u.Id] = curLvl + 1
                                    task.wait(0.08)
                                end
                            end
                        end
                    end
                end
            end
        end
    end
    
    local map = workspace:FindFirstChild("Map")
    local store = map and map:FindFirstChild("StoreItems")
    if not store then return end

    -- 3. AUTO-BUY PEELERS / KNIVES / BOXES (STRICT PROGRESSION ORDER: NEVER SKIP PREREQUISITES)
    local toolProgression
    local toolFolder
    if isChapter3() then
        toolFolder = store:FindFirstChild("Boxes")
        toolProgression = {
            { name = "WoodCrate",     title = "Wood Crate",     fallbackCost = 2.00,  rarity = "Rare" },
            { name = "MetalCase",     title = "Metal Case",     fallbackCost = 8.00,  rarity = "Epic" },
            { name = "TreasureChest", title = "Treasure Chest", fallbackCost = 30.00, rarity = "Legendary" }
        }
    elseif isChapter2() then
        toolFolder = store:FindFirstChild("Knives") or store:FindFirstChild("Peelers")
        toolProgression = {
            { name = "BigKnife",  title = "Big Knife",  fallbackCost = 1.25,  rarity = "Common" },
            { name = "Cleaver",   title = "Cleaver",    fallbackCost = 3.75,  rarity = "Epic" },
            { name = "LongBlade", title = "Long Blade", fallbackCost = 10.00, rarity = "Rare" },
            { name = "Mezzaluna", title = "Mezzaluna",  fallbackCost = 27.50, rarity = "Legendary" }
        }
    else
        toolFolder = store:FindFirstChild("Peelers") or store:FindFirstChild("Knives")
        toolProgression = {
            { name = "Knife",        title = "Knife",         fallbackCost = 0.01,  rarity = "Common" },
            { name = "SwissKnife",   title = "Swiss Knife",   fallbackCost = 1.50,  rarity = "Rare" },
            { name = "Peeler",       title = "Peeler",        fallbackCost = 4.00,  rarity = "Epic" },
            { name = "TriplePeeler", title = "Triple Peeler", fallbackCost = 25.00, rarity = "Legendary" }
        }
    end

    if toolFolder and toolProgression then
        for _, t in ipairs(toolProgression) do
            local stall = toolFolder:FindFirstChild(t.name)
            local isOwned = (stall and StoreItemShared.IsOwned(stall, state))
                or (state.CutTools and state.CutTools[t.name])
                or (state.PeelTools and state.PeelTools[t.name])
                or (state.PackTools and (state.PackTools[t.name] or table.find(state.PackTools, t.name)))
                or (state.Boxes and state.Boxes[t.name])
            
            if not isOwned then
                -- This is the next required tool/box in progressive unlock order!
                if stall then
                    local info = pcall(function() return StoreItemShared.GetInfo(LocalPlayer, stall, state) end) and StoreItemShared.GetInfo(LocalPlayer, stall, state)
                    local reqMet = not (info and info.Requirement)
                    
                    local actualCost = t.fallbackCost
                    for _, opt in ipairs(info and info.Options or {}) do
                        if opt.Kind == "Cash" or opt.Cash then
                            actualCost = opt.Cash
                            break
                        end
                    end
                    
                    if reqMet and cash >= actualCost then
                        local bought = buyStoreItem(stall, t.name, toolFolder.Name, actualCost, t.name == "Knife" or t.name == "BigKnife")
                        if bought then
                            if Window then
                                Window:Notify({ Title = (toolFolder.Name == "Boxes" and "Box Acquired!" or "Tool Acquired!"), Desc = string.format("Unlocked %s (%s)!", t.title, t.rarity), Time = 4 })
                            end
                            cash = cash - actualCost
                            state = getPlayerState()
                        end
                    end
                end
                
                -- CRITICAL: Always break on the first unowned tool so we NEVER skip ahead (e.g. skipping Cleaver)
                break
            end
        end
    end
    
    -- 4. AUTO-BUY STORAGE UPGRADES (PRIORITIZING HIGHEST CAPACITY FIRST)
    local storageFolder = store:FindFirstChild("Storage")
    if storageFolder and state.CarryLevel < 4 then
        local storagePriority = {
            { name = "MechaCapsule", level = 4, cost = 20.00, cap = 250 },
            { name = "PotatoSack",   level = 3, cost = 3.50,  cap = 125 },
            { name = "Bucket",       level = 2, cost = 0.50,  cap = 50 }
        }
        for _, s in ipairs(storagePriority) do
            local stall = storageFolder:FindFirstChild(s.name)
            if stall and state.CarryLevel < s.level and not StoreItemShared.IsOwned(stall, state) then
                local info = pcall(function() return StoreItemShared.GetInfo(LocalPlayer, stall, state) end) and StoreItemShared.GetInfo(LocalPlayer, stall, state)
                local reqMet = not (info and info.Requirement)
                local actualCost = s.cost
                for _, opt in ipairs(info and info.Options or {}) do
                    if opt.Kind == "Cash" or opt.Cash then actualCost = opt.Cash; break end
                end
                if reqMet and cash >= actualCost then
                    local bought = buyStoreItem(stall, s.name, "Storage", actualCost)
                    if bought then
                        if Window then
                            Window:Notify({ Title = "Storage Upgraded!", Desc = string.format("Upgraded to %s (Cap: %d)!", s.name, s.cap), Time = 3.5 })
                        end
                        cash = cash - actualCost
                        state = getPlayerState()
                        break
                    end
                end
            end
        end
    end
    
    -- 5. AUTO-BUY DIGGING TOOLS (PRIORITIZING HIGHEST RARITY FIRST)
    local toolsFolder = store:FindFirstChild("Tools")
    if toolsFolder then
        local toolsPriority = {
            { name = "P0T4T0-3000", toolId = "PotatoSucker", rarity = "Legendary", cost = 30.00 },
            { name = "Dynamite",    toolId = "Dynamite",    rarity = "Epic",      cost = 5.00 },
            { name = "Shovel",      toolId = "Shovel",      rarity = "Rare",      cost = 1.00 }
        }
        for _, t in ipairs(toolsPriority) do
            local stall = toolsFolder:FindFirstChild(t.name)
            local isOwned = (stall and StoreItemShared.IsOwned(stall, state))
                or (state.Tools and (state.Tools[t.name] or (t.toolId and state.Tools[t.toolId])))
                or (t.toolId and ToolEquipController.OwnsTool(t.toolId))
            if stall and not isOwned then
                local info = pcall(function() return StoreItemShared.GetInfo(LocalPlayer, stall, state) end) and StoreItemShared.GetInfo(LocalPlayer, stall, state)
                local reqMet = not (info and info.Requirement)
                local actualCost = t.cost
                for _, opt in ipairs(info and info.Options or {}) do
                    if opt.Kind == "Cash" or opt.Cash then actualCost = opt.Cash; break end
                end
                if reqMet and cash >= actualCost then
                    local bought = buyStoreItem(stall, t.name, "Tools", actualCost)
                    if bought then
                        if Window then
                            Window:Notify({ Title = "Tool Purchased!", Desc = string.format("Acquired %s!", t.name), Time = 3.5 })
                        end
                        cash = cash - actualCost
                        state = getPlayerState()
                        local mainTool = getMainHarvestTool()
                        ToolEquipController.Equip(mainTool)
                        break
                    end
                end
            end
        end
    end
    
    -- 6. AUTO-BUY MACHINES (Adaptive for Chapters 1, 2, and 3 costs & requirements)
    local machines = map:FindFirstChild("PeelingMachines")
    if machines then
        local isCh3 = isChapter3()
        local autoMach = machines:FindFirstChild("AutoPeelingMachine")
        if not state.HasAutoPeeler and autoMach then
            local info = pcall(function() return StoreItemShared.GetInfo(LocalPlayer, autoMach, state) end) and StoreItemShared.GetInfo(LocalPlayer, autoMach, state)
            local cost = (info and info.Options and info.Options[1] and info.Options[1].Cash) or (isCh3 and 60.00 or 15.00)
            local reqMet = not (info and info.Requirement)
            if reqMet and cash >= cost then
                local bought = buyStoreItem(autoMach, "AutoPeeler", "Machines", cost)
                if bought then
                    cash = cash - cost
                end
            end
        end
        local handMach = machines:FindFirstChild("PeelingMachine")
        if not state.HasHandPeeler and handMach then
            local info = pcall(function() return StoreItemShared.GetInfo(LocalPlayer, handMach, state) end) and StoreItemShared.GetInfo(LocalPlayer, handMach, state)
            local cost = (info and info.Options and info.Options[1] and info.Options[1].Cash) or (isCh3 and 8.00 or 2.50)
            local reqMet = not (info and info.Requirement)
            if reqMet and cash >= cost then
                local bought = buyStoreItem(handMach, "HandPeeler", "Machines", cost)
                if bought then
                    cash = cash - cost
                end
            end
        end
    end
    
    -- 7. LOBBY UPGRADES (GEMS)
    if state.SaveData and state.SaveData.LobbyUpgrades then
        pcall(function()
            local upgrades = {
                LobbyUpgradeShared.BAG_SIZE,
                LobbyUpgradeShared.GRAB_AMOUNT,
                LobbyUpgradeShared.CASH_BOOST,
                LobbyUpgradeShared.GEM_BOOST
            }
            local gems = (state.SaveData and state.SaveData.Gems) or state.Gems or 0
            for _, upgId in ipairs(upgrades) do
                local upgObj = LobbyUpgradeShared.GetUpgrade(upgId)
                if upgObj then
                    local lvl = LobbyUpgradeShared.GetLevel(state.SaveData.LobbyUpgrades, upgId)
                    local nextCost = LobbyUpgradeShared.GetNextCost(upgObj, lvl)
                    if nextCost and gems >= nextCost then
                        Packets.LobbyUpgradePurchase:Fire(upgId)
                        gems = gems - nextCost
                        task.wait(0.1)
                        clickScreenOnce()
                    end
                end
            end
        end)
    end
end
_G.__runAutoPurchases = runAutoPurchases

-- [ENGINE 7]: REWARDS AUTO-CLAIMER
local function claimAllRewards()
    if Config.AutoClaimDaily then
        pcall(function() Packets.DailyRewardClaim:Fire() end)
    end
    if Config.AutoClaimFreeCrates then
        pcall(function() Packets.FreeCratesClaim:Fire() end)
        pcall(function() Packets.CrateOpenFree:Fire("Free") end)
    end
    if Config.AutoClaimBattlePass then
        pcall(function() Packets.BattlePassClaimAll:Fire() end)
    end
    if Config.AutoClaimAchievements then
        pcall(function()
            for achId = 1, 25 do
                Packets.AchievementClaim:Fire(achId)
            end
        end)
    end
    if Config.AutoClaimUpdateGift then
        pcall(function() Packets.UpdateGiftClaim:Fire() end)
        pcall(function() Packets.ChangelogSeen:Fire() end)
    end
end

-- [ENGINE 8]: MACHINE AUTOMATION
local function runMachineAutomation()
    if isChapter3() then return end -- Chapter 3 uses packing tables, NEVER hand/auto peeler machines!
    local map = workspace:FindFirstChild("Map")
    local machines = map and map:FindFirstChild("PeelingMachines")
    if not machines then return end
    
    local state = getPlayerState()
    
    -- Auto-Crank Hand Machine
    if Config.AutoCrankHandMachine and state.HasHandPeeler then
        local crank = machines.PeelingMachine:FindFirstChild("CrankPromptPart")
        local prompt = crank and crank:FindFirstChild("Crank")
        if prompt and prompt.Enabled then
            safeFirePrompt(prompt)
        end
    end
    
    -- Auto-Load Hand Machine
    if Config.AutoLoadHandMachine and state.HasHandPeeler and state.Potatoes > 0 then
        local loadPrompt = machines.PeelingMachine["Cube.033"]:FindFirstChild("Load")
        if loadPrompt and loadPrompt.Enabled then
            safeFirePrompt(loadPrompt)
        end
    end
    
    -- Auto-Load Auto Machine
    if Config.AutoLoadAutoMachine and state.HasAutoPeeler and state.Potatoes > 0 then
        local loadPrompt = machines.AutoPeelingMachine["Cube.042"]:FindFirstChild("Load")
        if loadPrompt and loadPrompt.Enabled then
            safeFirePrompt(loadPrompt)
        end
    end
    
    -- Auto-Load Conveyor
    if Config.AutoLoadConveyor and state.HasConveyor and state.Potatoes > 0 then
        pcall(function() Packets.ConveyorLoad:Fire() end)
    end
end

-- ===================================================================
-- 6.5 LOBBY AUTOMATION & AUTO-REQUEUE ENGINE
-- ===================================================================
local function isLobby()
    local map = workspace:FindFirstChild("Map")
    local zones = map and map:FindFirstChild("Zones")
    local matchmaking = zones and zones:FindFirstChild("MatchmakingZones")
    return matchmaking ~= nil and getPotatoPileInstance() == nil
end

local function isBunkerArea()
    return getPotatoPileInstance() ~= nil
end

-- 1. Auto-Purchase Lobby Diamond Upgrades (Cash Boost > Bag Size > Hand Grab Amount > Gem Boost)
local function runLobbyUpgrades()
    local state = getPlayerState()
    if not state then return end
    local saveData = state.SaveData
    if not saveData or not state._SaveDataLoaded then return end

    -- Check for free new-player Hand Grab Amount upgrade
    if saveData.GuideFreeUpgrade then
        pcall(function() Packets.LobbyUpgradePurchase:Fire("GrabAmount") end)
        if Window then
            Window:Notify({ Title = "Free Upgrade Claimed!", Desc = "Claimed free Hand Grab Amount!", Time = 4 })
        end
        task.wait(0.3)
        state = getPlayerState()
        saveData = state and state.SaveData
        if not saveData then return end
    end

    local diamonds = saveData.Diamonds or 0
    if diamonds <= 0 then return end

    -- Priority: Cash Boost > Bag Size > Hand Grab Amount > Gem Boost
    local priorityList = {
        { id = "CashBoost",  name = "Cash Boost" },
        { id = "BagSize",    name = "Bag Size" },
        { id = "GrabAmount", name = "Hand Grab Amount" },
        { id = "GemBoost",   name = "Gem Boost" },
    }

    local currentUpgrades = saveData.LobbyUpgrades or {}

    for _, item in ipairs(priorityList) do
        local upgDef = LobbyUpgradeShared.GetUpgrade(item.id)
        if upgDef then
            local curLvl = LobbyUpgradeShared.GetLevel(currentUpgrades, item.id)
            local maxLvl = LobbyUpgradeShared.GetMaxLevel(upgDef)
            
            while curLvl < maxLvl do
                local nextCost = LobbyUpgradeShared.GetNextCost(upgDef, curLvl)
                if nextCost and diamonds >= nextCost then
                    pcall(function() Packets.LobbyUpgradePurchase:Fire(item.id) end)
                    diamonds = diamonds - nextCost
                    curLvl = curLvl + 1
                    currentUpgrades[item.id] = curLvl
                    if Window then
                        Window:Notify({ Title = "Lobby Upgrade Bought!", Desc = string.format("%s upgraded to Lvl %d!", item.name, curLvl), Time = 3.5 })
                    end
                    task.wait(0.2)
                    clickScreenOnce()
                else
                    break
                end
            end
        end
    end
end

-- 2. Scan & Find Open / Unoccupied Elevator
local function findOpenElevator()
    local map = workspace:FindFirstChild("Map")
    local zones = map and map:FindFirstChild("Zones")
    local mm = zones and zones:FindFirstChild("MatchmakingZones")
    if not mm then return nil, nil end

    local allPlayers = Players:GetPlayers()

    for _, zone in ipairs(mm:GetChildren()) do
        local state = zone:GetAttribute(QueueShared.STATE_ATTRIBUTE)
        -- Elevator must be in Empty state
        if state == QueueShared.EMPTY_STATE or state == "Empty" then
            local pad = QueueShared.FindPad(zone) or zone:FindFirstChild("Part", true) or zone:FindFirstChild("Cube")
            if pad and pad:IsA("BasePart") then
                -- Check if any OTHER player is standing in or near this elevator
                local occupiedByPlayer = false
                for _, otherPlayer in ipairs(allPlayers) do
                    if otherPlayer ~= LocalPlayer and otherPlayer.Character then
                        local oHrp = otherPlayer.Character:FindFirstChild("HumanoidRootPart")
                        if oHrp and (oHrp.Position - pad.Position).Magnitude < 14 then
                            occupiedByPlayer = true
                            break
                        end
                    end
                end

                if not occupiedByPlayer then
                    return zone, pad
                end
            end
        end
    end

    return nil, nil
end

-- 3. Auto-Requeue into Elevator
local isRequeueing = false
local function runAutoRequeue()
    if isRequeueing then return end
    if not isLobby() then return end

    local state = getPlayerState()
    if not state then return end

    -- Determine Target Chapter: Chapter 3 if unlocked, otherwise Chapter 2, otherwise Chapter 1
    local isCh3Unlocked = false
    local isCh2Unlocked = false
    if state then
        local ok3, res3 = pcall(function() return ChapterShared.IsUnlocked(state, 3) end)
        if ok3 and res3 then
            isCh3Unlocked = true
        elseif state.SaveData and state.SaveData.PermanentUnlocks and state.SaveData.PermanentUnlocks.Chapter3 then
            isCh3Unlocked = true
        end
        local ok2, res2 = pcall(function() return ChapterShared.IsUnlocked(state, 2) end)
        if ok2 and res2 then
            isCh2Unlocked = true
        elseif state.SaveData and ((state.SaveData.Wins and state.SaveData.Wins > 0) or (state.SaveData.PermanentUnlocks and state.SaveData.PermanentUnlocks.Chapter2)) then
            isCh2Unlocked = true
        end
    end

    local targetChapter = 1
    if Config.PreferredChapter == "Chapter 3" then
        targetChapter = 3
    elseif Config.PreferredChapter == "Chapter 2" then
        targetChapter = 2
    elseif Config.PreferredChapter == "Chapter 1" then
        targetChapter = 1
    else
        targetChapter = isCh3Unlocked and 3 or (isCh2Unlocked and 2 or 1)
    end

    -- Determine Skip Cutscene eligibility
    local canSkip = false
    if state and state.SaveData then
        local ok, res = pcall(function() return IntroShared.CanSkip(state.SaveData, targetChapter) end)
        canSkip = ok and (res == true)
    end
    local skipIntro = Config.SkipCutscenes and canSkip

    -- Find an open elevator
    local zone, pad = findOpenElevator()
    if not zone or not pad then
        RuntimeState.CurrentPhase = "Waiting for an open elevator..."
        return
    end

    isRequeueing = true
    RuntimeState.CurrentPhase = string.format("Requeueing into Elevator %s (Chapter %d)...", zone.Name, targetChapter)

    -- Teleport to Elevator Pad
    local hrp = getHRP()
    if hrp then
        hrp.CFrame = pad.CFrame + Vector3.new(0, 3, 0)
    end
    task.wait(0.4)

    -- Dismiss in-game chapter selection GUI if opened
    pcall(function()
        local qGui = PlayerGui:FindFirstChild("Queue")
        if qGui then
            if qGui:FindFirstChild("Setup") then qGui.Setup.Visible = false end
            for _, ch in ipairs(qGui:GetChildren()) do
                if ch:IsA("GuiObject") and ch.Name ~= "Leave" then
                    ch.Visible = false
                end
            end
        end
        local BlurController = require(ReplicatedStorage.Controllers.BlurController)
        if BlurController then BlurController.SetBlurred("Queue", false) end
    end)

    -- Fire Queue Creation packet (1 player, skipIntro, friendsOnly = false, targetChapter)
    pcall(function()
        Packets.QueueCreate:Fire(Config.RequeuePartySize or 1, skipIntro, false, targetChapter)
    end)
    if Window then
        Window:Notify({ Title = "Queue Started!", Desc = string.format("Queued Chapter %d (1 Player)!", targetChapter), Time = 4 })
    end

    -- Wait for elevator teleport
    task.wait(3)
    isRequeueing = false
end

-- ===================================================================
-- 7. BACKGROUND WORKER THREADS
-- ===================================================================

-- Master Farm & Lobby Loop
task.spawn(function()
    while ScriptRunning do
        if isLobby() then
            -- =======================================================
            -- LOBBY MODE: Run Diamond Upgrades and Auto-Requeue
            -- =======================================================
            if Config.AutoLobbyUpgrades then
                pcall(runLobbyUpgrades)
            end
            if Config.AutoRequeue then
                pcall(runAutoRequeue)
            end
            task.wait(1.5)
        elseif Config.MasterFarm then
            -- =======================================================
            -- BUNKER MODE: Master Farm Cycle
            -- =======================================================
            local success, err = pcall(function()
                -- Phase 0: Auto-Complete Tutorial if pending (Chapter 1 only)
                if isChapter1() then
                    local state = getPlayerState()
                    local step = state.TutorialStep or 0
                    if not tutorialCompletedOnce and step > 0 and step < 9 then
                        runTutorialSolver()
                        return
                    end
                end

                -- Chapter 3: If delivery minigame is currently active, pause farm loop until delivery finishes
                if isDeliveryMinigameActive() or isDelivering then
                    RuntimeState.CurrentPhase = "Delivery Minigame Active..."
                    task.wait(0.5)
                    return
                end

                -- Phase 1: Check Auto-Escape
                checkAutoEscape()
                
                -- Phase 1.5: Pit Fire Heat Check & Water Valve Cool (Chapter 2)
                checkAndCoolPitHeat()
                
                -- Phase 2: Gather Potatoes (Gathers until bag is 100% full)
                gatherPotatoesUntilFull()
                
                -- Phase 3: Fast Peel / Box Packing
                if Config.AutoPeelAtTable or Config.MasterFarm then
                    if isChapter3() then
                        -- Chapter 3: Always pack at the packing table (NEVER touch hand peeler or auto peeler)
                        if isBagMaxed() or not isBagEmpty() then
                            peelBatchAtTable()
                        end
                    else
                        local tableRemaining = getCuttingTablePotatoCount()
                        if isBagMaxed() or not isBagEmpty() or tableRemaining > 0 then
                            if not isCuttingTableAreaEmpty() then
                                peelBatchAtTable()
                            end
                            
                            if isCuttingTableAreaEmpty() and isHandCrankerLevel2() then
                                peelBatchWithHandCranker()
                            elseif not isHandCrankerLevel2() and (isBagMaxed() or not isBagEmpty()) then
                                peelBatchAtTable()
                            end
                        end
                    end
                end
                
                -- In Chapter 3: Check Pallet and Delivery immediately after table packing
                if isChapter3() and (Config.AutoLoadTruckPallet or Config.AutoDeliverTruck or Config.MasterFarm) then
                    handleChapter3Selling()
                end
                
                -- Phase 4: Sell Peeled Potatoes (Ch1/2) or Pallet/Truck Delivery (Ch3)
                if Config.AutoSell or Config.MasterFarm or (isChapter3() and (Config.AutoDeliverTruck or Config.AutoLoadTruckPallet)) then
                    if isChapter3() or isBagEmpty() then
                        sellPeeledPotatoes()
                    end
                end
                
                -- Phase 5: Auto-Buy Upgrades
                runAutoPurchases()
                
                -- Phase 6: Claim Free Rewards
                claimAllRewards()
                
                RuntimeState.CurrentPhase = "Cycle Complete! Restarting..."
            end)
            if not success then
                task.wait(1)
            end
        else
            -- Standalone Peeler Mode (works whenever user sits at table)
            if Config.StandalonePeeler and PeelingController.IsPeeling then
                peelActivePotatoOnce()
                task.wait(0.08)
            end
        end
        task.wait(0.2)
    end
end)

-- Machine, Auto-Save & Background Rewards Thread
task.spawn(function()
    local rewardTimer = 0
    local autoSaveTimer = 0
    while ScriptRunning do
        task.wait(0.5)
        runMachineAutomation()
        updateKeyTracker()
        -- Ensure camera and controls remain active and normal outside of active minigames
        pcall(ensureControlsAndCamera)
        
        if Config.AutoCompleteTutorial and not Config.MasterFarm and isChapter1() then
            local state = getPlayerState()
            local step = state.TutorialStep or 0
            if not tutorialCompletedOnce and step > 0 and step < 9 then
                pcall(runTutorialSolver)
            end
        end
        
        -- Auto-save configuration to Aether/Configs/AutoPotato.json every 10 seconds
        autoSaveTimer = autoSaveTimer + 0.5
        if autoSaveTimer >= 10 then
            autoSaveTimer = 0
            saveGameConfig()
        end
        
        rewardTimer = rewardTimer + 0.5
        if rewardTimer >= 15 then
            rewardTimer = 0
            claimAllRewards()
            if not Config.MasterFarm then
                runAutoPurchases()
            end
        end
    end
end)

-- Anti-AFK Engine
trackConn(LocalPlayer.Idled:Connect(function()
    if Config.AntiAFK then
        pcall(function()
            VirtualUser:CaptureController()
            VirtualUser:ClickButton1(Vector2.new(50, 50))
        end)
    end
end))

-- Persistent Movement Physics (WalkSpeed & JumpPower) & Controls Guardian
local lastControlGuardianTick = 0
trackConn(RunService.Heartbeat:Connect(function()
    local hum = getHumanoid()
    if hum then
        if Config.WalkSpeedEnabled then
            hum.WalkSpeed = Config.WalkSpeed
        end
        if Config.JumpPowerEnabled then
            hum.JumpPower = Config.JumpPower
        end
    end
    local now = os.clock()
    if now - lastControlGuardianTick >= 0.25 then
        lastControlGuardianTick = now
        ensureControlsAndCamera()
    end
end))

-- Noclip Engine
trackConn(RunService.Stepped:Connect(function()
    if Config.Noclip then
        local char = getCharacter()
        if char then
            for _, part in ipairs(char:GetDescendants()) do
                if part:IsA("BasePart") and part.CanCollide then
                    part.CanCollide = false
                end
            end
        end
    end
end))

-- Infinite Jump Engine
trackConn(UserInputService.JumpRequest:Connect(function()
    if Config.InfiniteJump then
        local hum = getHumanoid()
        if hum then
            hum:ChangeState(Enum.HumanoidStateType.Jumping)
        end
    end
end))

-- Packet Listeners for Real-time Key & Rewards
if Packets then
    if Packets.PotatoKeyMoved then
        trackConn(Packets.PotatoKeyMoved.OnClientEvent:Connect(function(keyId)
            RuntimeState.KeyDetectedId = keyId
            updateKeyTracker()
        end))
    end

    if Packets.KeyRevealed then
        trackConn(Packets.KeyRevealed.OnClientEvent:Connect(function()
            RuntimeState.KeyStatus = "KEY REVEALED! 🔑"
            Library:Notify({ Title = "KEY REVEALED!", Desc = "The bunker key has been found!", Time = 5 })
            if Config.AutoGrabAndEscape then
                task.spawn(checkAutoEscape)
            end
        end))
    end

    if Packets.Escaped then
        trackConn(Packets.Escaped.OnClientEvent:Connect(function(timeElapsed)
            RuntimeState.KeyStatus = "ESCAPED! 🏆"
            Config.MasterFarm = false
            Library:Notify({ Title = "RUN COMPLETE", Desc = string.format("Escaped bunker in %.1fs!", timeElapsed or 0), Time = 8 })
        end))
    end

    if Packets.PeelPaid then
        trackConn(Packets.PeelPaid.OnClientEvent:Connect(function(peels, cashEarned)
            RuntimeState.SessionEarnings = RuntimeState.SessionEarnings + (cashEarned or 0)
        end))
    end
end

-- ===================================================================
-- 8. INITIALIZE AETHER INTERFACE
-- ===================================================================
local function CreateWindow()
    if not Library then
        Library = ResolveLibrary(2)
        if not Library then return nil end
    end
    local ok, created = pcall(Library.Window, Library, {
        Title = "Aether",
        Desc = "Studios",
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
    return
end

-- Immediately listen for Aether window minimize/restore to lock/unlock mouse smoothly
pcall(function()
    local cg = game:GetService("CoreGui")
    local gh = typeof(gethui) == "function" and gethui()
    local pg = LocalPlayer and LocalPlayer:FindFirstChild("PlayerGui")
    local aether = (cg and cg:FindFirstChild("Aether")) or (gh and gh:FindFirstChild("Aether")) or (pg and pg:FindFirstChild("Aether"))
    local shadow = aether and aether:FindFirstChild("Shadow")
    if shadow then
        trackConn(shadow:GetPropertyChangedSignal("Visible"):Connect(function()
            pcall(ensureControlsAndCamera)
        end))
    end
end)

-- UI Tabs
local DashboardTab = Window:Tab({ Title = "Dashboard", Icon = "activity" })
local AutoFarmTab  = Window:Tab({ Title = "Auto Farm", Icon = "zap" })
local LobbyTab     = Window:Tab({ Title = "Lobby & Requeue", Icon = "repeat" })
local KeyHunterTab = Window:Tab({ Title = "Key & Escape", Icon = "key" })
local UpgradesTab  = Window:Tab({ Title = "Upgrades", Icon = "shopping-bag" })
local RewardsTab   = Window:Tab({ Title = "Rewards", Icon = "gift" })
local TeleportsTab = Window:Tab({ Title = "Teleports", Icon = "layout-grid" })
local PlayerTab    = Window:Tab({ Title = "Player", Icon = "user" })

-- -------------------------------------------------------------------
-- TAB 1: DASHBOARD
-- -------------------------------------------------------------------
local DashStatsSec = DashboardTab:Section({ Title = "LIVE BUNKER STATS" })

local StatCash = DashStatsSec:Label({ Title = "Cash Balance", Desc = "$0.00" })
local StatBag = DashStatsSec:Label({ Title = "Bag Inventory", Desc = "0 / 10 Potatoes" })
local StatCarry = DashStatsSec:Label({ Title = "Carry Equipment", Desc = "Hand (Level 1)" })
local StatPeels = DashStatsSec:Label({ Title = "Peeling Telemetry", Desc = "0 slices stripped ($0.00 earned)" })
local StatKey = DashStatsSec:Label({ Title = "Key & Bunker Escape", Desc = "Locked 🔒" })
local StatPhase = DashStatsSec:Label({ Title = "Farm State", Desc = "Idle" })
local StatSweetSpot = DashStatsSec:Label({ Title = "Potato Sweet Spot Reach", Desc = "7,789 potatoes in range (Safe Ground)" })

-- Dashboard Update Function (Active real-time updates with differential dirty-checking)
local lastDashboardCache: { [string]: string } = {
    Cash = "",
    Bag = "",
    Carry = "",
    Peels = "",
    Key = "",
    Phase = "",
    SweetSpot = ""
}
local function updateDashboardStats()
    local ok, err = pcall(function()
        local setid = (typeof(setthreadidentity) == "function" and setthreadidentity)
            or (typeof(set_thread_identity) == "function" and set_thread_identity)
            or (typeof(setidentity) == "function" and setidentity)

        local state = getPlayerState()
        local capacity = getBagCapacity()
        
        -- Restore thread identity to 8 in case module calls/yields stripped Plugin capability
        if setid then pcall(setid, 8) end

        local newCash, newBag
        if isLobby() then
            local diamonds = (state and state.SaveData and state.SaveData.Diamonds) or 0
            newCash = string.format("Lobby: %d Diamonds 💎", diamonds)
            newBag = "Lobby: Ready to Queue 🚀"
        else
            local cash = (state and state.Cash) or 0
            local potatoes = (state and state.Potatoes) or 0
            newCash = string.format("$%.2f", cash)
            newBag = string.format("%d / %d Potatoes", potatoes, capacity or 10)
            local okFull, bagFull = pcall(isBagFull)
            if okFull and bagFull then
                newBag = newBag .. " (FULL)"
            end
        end
        
        local carryLvl = (state and state.CarryLevel) or 1
        local storageNames = { "Hand (Level 1)", "Bucket (Level 2)", "Potato Sack (Level 3)", "Mecha Capsule (Level 4)" }
        local newCarry = storageNames[carryLvl] or string.format("Level %d", carryLvl)
        
        local newPeels = string.format("%d slices (%d potatoes) | +$%.2f session", 
            RuntimeState.TotalSlicesPeeled or 0, RuntimeState.TotalPotatoesPeeled or 0, RuntimeState.SessionEarnings or 0)
            
        local newKey = RuntimeState.KeyStatus or "Locked 🔒"
        local newPhase = RuntimeState.CurrentPhase or "Idle"
        
        local reach = RuntimeState.SweetSpotReach or 7789
        local newSweetSpot = string.format("%d potatoes in reach (Safe Ground)", reach)

        -- Differential Dirty Checking: Only call :SetDesc if the text actually changed
        -- This avoids invoking TextService:GetTextSize and frame recalculations when idle (<0.005ms)
        if lastDashboardCache.Cash ~= newCash then
            lastDashboardCache.Cash = newCash
            StatCash:SetDesc(newCash)
        end
        if lastDashboardCache.Bag ~= newBag then
            lastDashboardCache.Bag = newBag
            StatBag:SetDesc(newBag)
        end
        if lastDashboardCache.Carry ~= newCarry then
            lastDashboardCache.Carry = newCarry
            StatCarry:SetDesc(newCarry)
        end
        if lastDashboardCache.Peels ~= newPeels then
            lastDashboardCache.Peels = newPeels
            StatPeels:SetDesc(newPeels)
        end
        if lastDashboardCache.Key ~= newKey then
            lastDashboardCache.Key = newKey
            StatKey:SetDesc(newKey)
        end
        if lastDashboardCache.Phase ~= newPhase then
            lastDashboardCache.Phase = newPhase
            StatPhase:SetDesc(newPhase)
        end
        if lastDashboardCache.SweetSpot ~= newSweetSpot then
            lastDashboardCache.SweetSpot = newSweetSpot
            StatSweetSpot:SetDesc(newSweetSpot)
        end
    end)
end
_G.__updateDashboardStats = updateDashboardStats

-- Initial update immediately upon interface creation
task.defer(updateDashboardStats)

-- Dashboard Update Loop (Active 1-second refresh rate; virtually 0ms CPU load thanks to dirty-checking)
task.spawn(function()
    while ScriptRunning do
        task.wait(1)
        local setid = (typeof(setthreadidentity) == "function" and setthreadidentity)
            or (typeof(set_thread_identity) == "function" and set_thread_identity)
            or (typeof(setidentity) == "function" and setidentity)
        if setid then pcall(setid, 8) end
        updateDashboardStats()
        restoreGameGuis()
    end
end)

-- -------------------------------------------------------------------
-- TAB 2: AUTO FARM & PEELER
-- -------------------------------------------------------------------
local FarmMainSec = AutoFarmTab:Section({ Title = "POTATO LOOP" })

UIControls.MasterFarm = FarmMainSec:Toggle({
    Title = "Potato Loop",
    Desc = "Fully automated infinite cycle: Pick from Pile -> Move -> Fast Peel -> Move -> Sell",
    Value = Config.MasterFarm,
    Flag = "MasterFarm",
    Callback = function(val)
        Config.MasterFarm = val
        debouncedSaveGameConfig()
        RuntimeState.CurrentPhase = val and "Starting Potato Loop..." or "Idle"
        Window:Notify({ Title = "Potato Loop " .. (val and "Active" or "Stopped"), Desc = val and "Autonomous potato loop running!" or "Potato loop stopped.", Time = 3 })
    end
})


FarmMainSec:Button({
    Title = "Warp to Potato Sweet Spot (7,700+ Reach)",
    Desc = "Calculates highest density potato spot and warps safely above surface (No falling!)",
    Callback = function()
        local bestSpot, count = calculateBestPotatoSpot(18)
        moveToPosition(bestSpot, true)
        Window:Notify({ Title = "Sweet Spot Reached", Desc = string.format("Positioned safely at sweet spot! %d potatoes in pick range.", count), Time = 4 })
    end
})

local PeelerSec = AutoFarmTab:Section({ Title = "HIGH-SPEED PEELER SETTINGS" })

UIControls.StandalonePeeler = PeelerSec:Toggle({
    Title = "Standalone Fast Peeler",
    Desc = "Rapidly strips all peels whenever you are peeling at a table (works even if Master Farm is OFF)",
    Value = Config.StandalonePeeler,
    Callback = function(val)
        Config.StandalonePeeler = val
        debouncedSaveGameConfig()
    end
})

UIControls.SliceDelay = PeelerSec:Slider({
    Title = "Slice Stripping Delay",
    Desc = "Time in seconds between peel strikes (0.01s = Instant, 0.04s = Balanced)",
    Min = 0.01,
    Max = 0.20,
    Value = Config.SliceDelay,
    Rounding = 2,
    Callback = function(val)
        Config.SliceDelay = val
        debouncedSaveGameConfig()
    end
})

UIControls.AutoLeaveStationOnEmpty = PeelerSec:Toggle({
    Title = "Auto-Leave Station When Empty",
    Desc = "Exits the peeling table interface as soon as all held potatoes are finished",
    Value = Config.AutoLeaveStationOnEmpty,
    Callback = function(val)
        Config.AutoLeaveStationOnEmpty = val
        debouncedSaveGameConfig()
    end
})


-- (Machine automations & Pit Fire cooling run 100% automatically under the hood)


-- -------------------------------------------------------------------
-- TAB 2.5: LOBBY & AUTO-REQUEUE
-- -------------------------------------------------------------------
local LobbyRequeueSec = LobbyTab:Section({ Title = "AUTO REQUEUE & ELEVATOR" })

UIControls.AutoRequeue = LobbyRequeueSec:Toggle({
    Title = "Auto Requeue on Return to Lobby",
    Desc = "Automatically finds an open unoccupied elevator and queues for the next match",
    Value = Config.AutoRequeue,
    Callback = function(val)
        Config.AutoRequeue = val
        debouncedSaveGameConfig()
    end
})

UIControls.PreferredChapter = LobbyRequeueSec:Dropdown({
    Title = "Target Chapter",
    Desc = "Select chapter to queue (Auto selects highest unlocked Chapter)",
    List = { "Auto", "Chapter 3", "Chapter 2", "Chapter 1" },
    Values = { "Auto", "Chapter 3", "Chapter 2", "Chapter 1" },
    Value = Config.PreferredChapter,
    Callback = function(val)
        Config.PreferredChapter = val
        debouncedSaveGameConfig()
    end
})

UIControls.RequeuePartySize = LobbyRequeueSec:Slider({
    Title = "Queue Party Size",
    Desc = "Target squad size for the elevator (1 = Solo)",
    Min = 1,
    Max = 4,
    Value = Config.RequeuePartySize,
    Rounding = 0,
    Callback = function(val)
        Config.RequeuePartySize = val
        debouncedSaveGameConfig()
    end
})

UIControls.SkipCutscenes = LobbyRequeueSec:Toggle({
    Title = "Skip Cutscenes",
    Desc = "Automatically skips intros and cutscenes upon elevator launch",
    Value = Config.SkipCutscenes,
    Callback = function(val)
        Config.SkipCutscenes = val
        debouncedSaveGameConfig()
    end
})

LobbyRequeueSec:Button({
    Title = "Requeue Into Elevator Now",
    Desc = "Manually finds an open elevator, teleports inside, and creates the queue",
    Callback = function()
        pcall(runAutoRequeue)
    end
})

local LobbyUpgradeSec = LobbyTab:Section({ Title = "LOBBY DIAMOND UPGRADES" })

UIControls.AutoLobbyUpgrades = LobbyUpgradeSec:Toggle({
    Title = "Auto-Buy Lobby Upgrades",
    Desc = "Claims free Hand Grab upgrade & auto-buys: Cash Boost > Bag Size > Hand Grab Amount > Gem Boost",
    Value = Config.AutoLobbyUpgrades,
    Callback = function(val)
        Config.AutoLobbyUpgrades = val
        debouncedSaveGameConfig()
        if val then
            pcall(runLobbyUpgrades)
        end
    end
})

-- -------------------------------------------------------------------
-- TAB 3: KEY HUNTER & ESCAPE
-- -------------------------------------------------------------------
local KeySec = KeyHunterTab:Section({ Title = "KEY RADAR & ESCAPE SUITE" })

UIControls.AutoGrabAndEscape = KeySec:Toggle({
    Title = "Auto Grab Key & Escape Bunker",
    Desc = "Automatically grabs the key potato when detected in the pile, warps to the Vault Door, and unlocks the bunker escape!",
    Value = Config.AutoGrabAndEscape,
    Callback = function(val)
        Config.AutoGrabAndEscape = val
        debouncedSaveGameConfig()
    end
})

KeySec:Button({
    Title = "Teleport to Key Potato",
    Desc = "Instantly warps your character directly to the detected key potato in the pile",
    Callback = function()
        if RuntimeState.KeyPosition then
            moveToPosition(RuntimeState.KeyPosition, true)
            if Window then
                Window:Notify({ Title = "Teleported to Key", Desc = "Arrived safely above Key Potato coordinates!", Time = 3 })
            end
        else
            if Window then
                Window:Notify({ Title = "Key Unknown", Desc = "Key potato has not been revealed or detected yet.", Time = 3 })
            end
        end
    end
})

KeySec:Button({
    Title = isChapter3() and "Teleport to Military Truck & CUT TRACKER Escape" or (isChapter2() and "Teleport to Crane & DRIVE Escape" or "Teleport to Vault Door & Escape"),
    Desc = isChapter3() and "Warps directly to the Chapter 3 Tracker Box on the Military Truck and triggers escape" or (isChapter2() and "Warps directly to the Chapter 2 Crane DRIVE prompt and triggers escape" or "Warps directly to the Bunker Vault Door Keyhole and attempts escape"),
    Callback = function()
        checkAutoEscape(true)
    end
})

-- -------------------------------------------------------------------
-- TAB 4: UPGRADES & STORE MANAGER
-- -------------------------------------------------------------------
local AutoBuySec = UpgradesTab:Section({ Title = "AUTO UPGRADES & AUTO STORE BUYER" })

UIControls.AutoBuyAllUpgrades = AutoBuySec:Toggle({
    Title = "Auto-Buy All Upgrades & Items",
    Desc = "Safely automates upgrades without opening UI modals: Potato Worth (General), Tool Stats, Trucks (Chapter 3), Peelers, Storage, Digging Tools, Machines, and Lobby Upgrades",
    Value = Config.AutoBuyAllUpgrades,
    Callback = function(val)
        Config.AutoBuyAllUpgrades = val
        debouncedSaveGameConfig()
        Window:Notify({ Title = "Auto-Upgrades " .. (val and "Active" or "Disabled"), Desc = val and "Automating safe upgrades and equipment purchases!" or "Auto-upgrades stopped.", Time = 3 })
    end
})

AutoBuySec:Button({
    Title = "Dismiss Stuck Spotlight Overlay",
    Desc = "Instantly clears any stuck tutorial spotlight mask overlay without clicking any buttons",
    Callback = function()
        pcall(function()
            if UISpotlightController and UISpotlightController.IsSpotlightActive() then
                UISpotlightController.RemoveSpotlight()
            end
        end)
        if Window then
            Window:Notify({ Title = "Spotlight Dismissed", Desc = "Cleared tutorial spotlight mask overlay!", Time = 3 })
        end
    end
})

-- -------------------------------------------------------------------
-- TAB 5: REWARDS & EVENTS
-- -------------------------------------------------------------------
local RewardsSec = RewardsTab:Section({ Title = "FREE REWARDS & EVENTS" })

UIControls.AutoClaimDaily = RewardsSec:Toggle({
    Title = "Auto-Claim Daily Login Streak",
    Desc = "Automatically collects the daily calendar gift upon availability",
    Value = Config.AutoClaimDaily,
    Callback = function(val)
        Config.AutoClaimDaily = val
        debouncedSaveGameConfig()
    end
})

UIControls.AutoClaimFreeCrates = RewardsSec:Toggle({
    Title = "Auto-Claim Free Crates",
    Desc = "Automatically retrieves and unlocks free crate roll timers",
    Value = Config.AutoClaimFreeCrates,
    Callback = function(val)
        Config.AutoClaimFreeCrates = val
        debouncedSaveGameConfig()
    end
})

UIControls.AutoClaimBattlePass = RewardsSec:Toggle({
    Title = "Auto-Claim Battle Pass Tiers",
    Desc = "Automatically claims unlocked Battle Pass rewards (Tiers 1-100)",
    Value = Config.AutoClaimBattlePass,
    Callback = function(val)
        Config.AutoClaimBattlePass = val
        debouncedSaveGameConfig()
    end
})

UIControls.AutoClaimAchievements = RewardsSec:Toggle({
    Title = "Auto-Claim Achievements",
    Desc = "Automatically collects rewards for all completed in-game achievements",
    Value = Config.AutoClaimAchievements,
    Callback = function(val)
        Config.AutoClaimAchievements = val
        debouncedSaveGameConfig()
    end
})

UIControls.AutoClaimUpdateGift = RewardsSec:Toggle({
    Title = "Auto-Claim Update Gift & News",
    Desc = "Automatically claims the update gift package and marks news as seen",
    Value = Config.AutoClaimUpdateGift,
    Callback = function(val)
        Config.AutoClaimUpdateGift = val
        debouncedSaveGameConfig()
    end
})

RewardsSec:Button({
    Title = "Claim All Free Rewards Now",
    Desc = "Immediately executes all daily, crate, battle pass, and achievement claims",
    Callback = function()
        claimAllRewards()
        Window:Notify({ Title = "Claimed", Desc = "All available free gifts claimed!", Time = 3 })
    end
})

-- -------------------------------------------------------------------
-- TAB 6: TELEPORTS & WAYPOINTS
-- -------------------------------------------------------------------
local TeleportFarmingSec = TeleportsTab:Section({ Title = "FARMING STATIONS" })

TeleportFarmingSec:Button({
    Title = "Potato Sweet Spot (7,700+ in reach)",
    Desc = "Calculates highest potato density spot and warps safely above surface (No falling!)",
    Callback = function()
        local bestSpot, count = calculateBestPotatoSpot(18)
        moveToPosition(bestSpot, true)
        Window:Notify({ Title = "Sweet Spot Reached", Desc = string.format("Arrived safely at sweet spot! %d potatoes in range.", count), Time = 4 })
    end
})

TeleportFarmingSec:Button({
    Title = isChapter3() and "Packing Table 1 (Default)" or "Peeling Table 1 (Default)",
    Desc = isChapter3() and "Warp to Table 1 Box Packing Stand" or "Warp to Table 1 Peel Stand",
    Callback = function() moveToPosition(Landmarks.PeelingTable1_Stand) end
})

TeleportFarmingSec:Button({
    Title = isChapter3() and "Pallet (Truck Loading)" or "Sell Crate (Bank Cash)",
    Desc = isChapter3() and "Warp to Pallet where packed boxes load onto the military truck" or "Warp directly in front of the Sell Crate",
    Callback = function() moveToPosition(Landmarks.Pallet or Landmarks.SellCrate) end
})

TeleportFarmingSec:Button({
    Title = isChapter3() and "Military Delivery Truck" or "Potato Input Crate",
    Desc = isChapter3() and "Warp to your Military Delivery Truck driver side" or "Warp in front of the raw potato storage crate",
    Callback = function()
        if isChapter3() then
            local truck = getMyTruck()
            if truck and truck.PrimaryPart then
                moveToPosition(truck.PrimaryPart.Position)
            else
                moveToPosition(Landmarks.MilitaryTruck)
            end
        else
            moveToPosition(Landmarks.PotatoCrate)
        end
    end
})

local TeleportMachinesSec = TeleportsTab:Section({ Title = "MACHINES & VAULT" })

TeleportMachinesSec:Button({
    Title = isChapter3() and "Auto Packer Machine" or "Auto Peeling Machine",
    Desc = isChapter3() and "Warp to motorized Auto Box Packer Machine" or "Warp to motorized Auto Peeling Machine",
    Callback = function() moveToPosition(Landmarks.AutoPeelingMachine) end
})

TeleportMachinesSec:Button({
    Title = isChapter3() and "Hand Packer Machine" or "Hand Peeling Machine",
    Desc = isChapter3() and "Warp to Hand Packer Crank Station" or "Warp to Hand Peeler Crank Station",
    Callback = function() moveToPosition(Landmarks.PeelingMachine) end
})

TeleportMachinesSec:Button({
    Title = "Conveyor Loading Station",
    Desc = "Warp to Conveyor Store & Loading Station",
    Callback = function() moveToPosition(Landmarks.ConveyorStore) end
})

TeleportMachinesSec:Button({
    Title = isChapter3() and "Tracker Escape (Cut Tracker)" or (isChapter2() and "Water Valve (Cool Fire - Ch2)" or "Water Valve (Cool Fire)"),
    Desc = isChapter3() and "Warp directly to Military Truck Tracker Box bunker escape" or "Warp directly to Chapter 2 Water Valve Cooling Terminal",
    Callback = function()
        if isChapter3() then
            moveToPosition(Landmarks.Tracker or Landmarks.VaultDoor)
        else
            moveToPosition(Landmarks.WaterValve or Landmarks.VaultDoor)
        end
    end
})

TeleportMachinesSec:Button({
    Title = isChapter3() and "Military Truck Tracker (Escape)" or (isChapter2() and "Crane (Drive to Escape)" or "Vault Door (Escape Keyhole)"),
    Desc = isChapter3() and "Warp directly to Chapter 3 Tracker Box (CUT TRACKER to Escape)" or (isChapter2() and "Warp directly to Chapter 2 Wrecking Ball Crane escape location" or "Warp straight to the final bunker exit vault door"),
    Callback = function() moveToPosition(Landmarks.Tracker or Landmarks.VaultDoor) end
})

local TeleportStoreSec = TeleportsTab:Section({ Title = "STORES & STALLS" })

TeleportStoreSec:Button({
    Title = isChapter3() and "Boxes Shop Stall" or "Peelers Shop Stall",
    Desc = isChapter3() and "Warp to Wood Crate, Metal Case, and Treasure Chest box merchant" or "Warp to knife and peeler tool merchants",
    Callback = function() moveToPosition(isChapter3() and Landmarks.Store_Boxes or Landmarks.Store_Peelers) end
})

TeleportStoreSec:Button({
    Title = "Storage Upgrades Stall",
    Desc = "Warp to buckets, sacks, and capsule merchants",
    Callback = function() moveToPosition(Landmarks.Store_Storage) end
})

TeleportStoreSec:Button({
    Title = "Digging Tools Stall",
    Desc = "Warp to shovel, dynamite, and sucker merchants",
    Callback = function() moveToPosition(Landmarks.Store_Tools) end
})

-- -------------------------------------------------------------------
-- TAB 7: PLAYER & QUALITY OF LIFE
-- -------------------------------------------------------------------
local MovementSec = PlayerTab:Section({ Title = "MOVEMENT MODIFIERS" })

UIControls.WalkSpeedEnabled = MovementSec:Toggle({
    Title = "Enable WalkSpeed Boost",
    Desc = "Maintains enhanced sprint speed across map",
    Value = Config.WalkSpeedEnabled,
    Callback = function(val)
        Config.WalkSpeedEnabled = val
        debouncedSaveGameConfig()
        if not val then
            local hum = getHumanoid()
            if hum then hum.WalkSpeed = 16 end
        end
    end
})

UIControls.WalkSpeed = MovementSec:Slider({
    Title = "WalkSpeed Multiplier",
    Desc = "Movement speed value (Default: 16)",
    Min = 16,
    Max = 120,
    Value = Config.WalkSpeed,
    Rounding = 0,
    Callback = function(val)
        Config.WalkSpeed = val
        debouncedSaveGameConfig()
    end
})

UIControls.JumpPowerEnabled = MovementSec:Toggle({
    Title = "Enable JumpPower Boost",
    Desc = "Allows higher jumping over obstacles and the potato mountain",
    Value = Config.JumpPowerEnabled,
    Callback = function(val)
        Config.JumpPowerEnabled = val
        debouncedSaveGameConfig()
        if not val then
            local hum = getHumanoid()
            if hum then hum.JumpPower = 50 end
        end
    end
})

UIControls.JumpPower = MovementSec:Slider({
    Title = "JumpPower Multiplier",
    Desc = "Jump power value (Default: 50)",
    Min = 50,
    Max = 250,
    Value = Config.JumpPower,
    Rounding = 0,
    Callback = function(val)
        Config.JumpPower = val
        debouncedSaveGameConfig()
    end
})

UIControls.InfiniteJump = MovementSec:Toggle({
    Title = "Infinite Jump",
    Desc = "Allows continuous jumping in mid-air",
    Value = Config.InfiniteJump,
    Callback = function(val)
        Config.InfiniteJump = val
        debouncedSaveGameConfig()
    end
})

UIControls.Noclip = MovementSec:Toggle({
    Title = "Collisionless Noclip",
    Desc = "Walk seamlessly through barriers, walls, and potato piles without getting stuck",
    Value = Config.Noclip,
    Callback = function(val)
        Config.Noclip = val
        debouncedSaveGameConfig()
    end
})

local AntiAFKSec = PlayerTab:Section({ Title = "SESSION PROTECTION" })

UIControls.AntiAFK = AntiAFKSec:Toggle({
    Title = "Built-in Anti-AFK Engine",
    Desc = "Prevents Roblox 20-minute idle disconnects by dispatching synthetic virtual input",
    Value = Config.AntiAFK,
    Callback = function(val)
        Config.AntiAFK = val
        debouncedSaveGameConfig()
    end
})

-- ===================================================================
-- 9. CLEANUP REGISTRATION & NOTIFICATION
-- ===================================================================
_G.__PeelThePotato_Cleanup = function()
    ScriptRunning = false
    protectAndRestoreGuis()
    for _, conn in ipairs(Connections) do
        pcall(function() conn:Disconnect() end)
    end
    table.clear(Connections)
    local esp = workspace:FindFirstChild("KeyPotato_ESP_Highlight")
    if esp then pcall(function() esp:Destroy() end) end
    local oldMobile = (typeof(gethui) == "function" and gethui() and gethui():FindFirstChild("Aether_MobileToggle")) or game:GetService("CoreGui"):FindFirstChild("Aether_MobileToggle") or (LocalPlayer:FindFirstChild("PlayerGui") and LocalPlayer.PlayerGui:FindFirstChild("Aether_MobileToggle"))
    if oldMobile then pcall(function() oldMobile:Destroy() end) end
    local hum = getHumanoid()
    if hum then
        hum:SetStateEnabled(Enum.HumanoidStateType.Climbing, true)
        hum.WalkSpeed = 16
        hum.JumpPower = 50
        hum.PlatformStand = false
        hum.Sit = false
    end
    isCoolingActive = false
    isDelivering = false
    activeDeliveryPlan = nil
    ensureControlsAndCamera()
    if OrigCutTools and CutToolShared and CutToolShared.Tools then
        pcall(function()
            for _, tool in ipairs(CutToolShared.Tools) do
                local orig = OrigCutTools[tool.Id]
                if orig then
                    tool.Instant = orig.Instant
                    tool.SlicesPerCut = orig.SlicesPerCut
                    tool.MoveSpeed = orig.MoveSpeed
                    tool.PotatoCount = orig.PotatoCount
                end
            end
        end)
    end
end

-- Auto-Load Game Profile from Aether/Configs/AutoPotato.json on startup
task.defer(function()
    task.wait(0.2)
    loadGameConfig()
end)
