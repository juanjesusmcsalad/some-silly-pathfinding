-- LIBRARY --
local Fluent = loadstring(game:HttpGet("https://github.com/dawid-scripts/Fluent/releases/latest/download/main.lua"))()
local SaveManager = loadstring(game:HttpGet("https://raw.githubusercontent.com/dawid-scripts/Fluent/master/Addons/SaveManager.lua"))()
local InterfaceManager = loadstring(game:HttpGet("https://raw.githubusercontent.com/dawid-scripts/Fluent/master/Addons/InterfaceManager.lua"))()

local Window = Fluent:CreateWindow({
    Title = "juanhub",
    SubTitle = "feesh and doop",
    TabWidth = 160,
    Size = UDim2.fromOffset(580, 460),
    Acrylic = false,
    Theme = "Dark",
    MinimizeKey = Enum.KeyCode.LeftControl
})

local Tabs = {
    Main = Window:AddTab({ Title = "Main", Icon = "fish" }),
    Settings = Window:AddTab({ Title = "Settings", Icon = "settings" })
}

-- FEATURES --
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local Players = game:GetService("Players")
local UserInputService = game:GetService("UserInputService")
local PathfindingService = game:GetService("PathfindingService")
local VirtualInputManager = game:GetService("VirtualInputManager")

local Fishing = require(ReplicatedStorage.Packets.Fishing)
local player = Players.LocalPlayer

local isFishing = false
local autoFishEnabled = false
local cycleInTransit = false
local pathfindingBusy = false
local stopPathfinding = false

local totalFishCaught = 0
local cycleFishCaught = 0
local targetFishCount = 1
local fishByType = {}

local SELL_DESTINATION = Vector3.new(100.5, 107.5, -296.75)
local RETURN_DESTINATION = Vector3.new(55, 97.5000153, -280)

local MAX_RETRIES = 5
local MOVE_TIMEOUT = 2
local ARRIVAL_RADIUS = 2.5
local RELAXED_ARRIVAL_RADIUS = 10
local PREJUMP_MIN = 1.5
local PREJUMP_MAX = 3.0

local AllFishes = {
    "Tire", "Rusty Can", "Old Boot",
    "Flounder", "Stonefish", "Silverfin", "Emberfin", "Rosefin", "Shrimprite",
    "Piranha", "Tunavax", "Zebream", "Coelacanth", "Flying Fish",
    "Skull Piranha", "Ghost Shark", "Vanta Sunfish", "Wolf Eel",
    "Gwrasse", "Bluetang", "Moonfish", "Duskray", "Octo", "Squibby",
    "Crabbie", "Turtlo", "Wahoo", "Bluefin Tuna", "Monk Fish",
    "Obsidian Tiger Oscar", "Tripod Fish", "Skull Seahorse", "Hand Fish",
    "Ocean Sunfish Fry", "Marine Anglefish",
    "Spiraldrift", "Spinepuff", "Marlin", "Sunfish", "Pelican Eel",
    "Skull Mackerel", "Magma Discus", "Barreleye", "Freshwater Angelfish",
    "Betta", "Saw Shark", "Sailfish",
    "Sharko", "Striped Marlin", "Goblin Shark", "Dragon Fish",
    "Antarctic Icefish", "Ripsaw Catfish", "Angel Shark", "Starfish",
    "Vanta Ray", "Skull Moray Eel", "Umbrella Octopus", "Clione",
    "Sea Pearl", "Abyssal Sea Pearl"
}

local function extractFishName(data)
    if typeof(data) ~= "table" then
        return "Unknown"
    end

    return tostring(
        data.fishName
        or data.fish
        or data.itemName
        or data.item
        or data.rewardName
        or data.reward
        or data.name
        or data.type
        or "Unknown"
    )
end

local function setAutoFishEnabled(enabled, source)
    autoFishEnabled = enabled
    print("[Fishing] Auto-fishing", autoFishEnabled and "ENABLED" or "DISABLED", source and ("(" .. source .. ")") or "")

    if not autoFishEnabled and isFishing then
        Fishing.quit.send()
    end
end

local function setTargetFishCount(rawValue)
    local parsed = tonumber(rawValue)
    if not parsed then
        warn("[Fishing] Invalid target fish count:", rawValue)
        return
    end

    targetFishCount = math.max(0, math.floor(parsed))
    player:SetAttribute("FishTargetCount", targetFishCount)
    print("[Fishing] Target fish count set to", targetFishCount)
end

local function findSpot(maxDistance)
    local char = player.Character
    local root = char and char:FindFirstChild("HumanoidRootPart")
    if not root then
        return nil, nil
    end

    local closestSpotId = nil
    local closestPos = nil
    local closestDist = math.huge

    for _, spot in ipairs(workspace.Map.Miscs.FishingPoints:GetChildren()) do
        if not spot:GetAttribute("occupied") then
            local pos = spot:GetPivot().Position
            local dist = (pos - root.Position).Magnitude
            if (not maxDistance or dist <= maxDistance) and dist < closestDist then
                closestDist = dist
                closestSpotId = spot:GetAttribute("spotId")
                closestPos = pos
            end
        end
    end

    return closestSpotId, closestPos
end

local function clickCloseButton()
    task.wait(0.5)

    local playerGui = player:FindFirstChild("PlayerGui")
    local mainInterface = playerGui and playerGui:FindFirstChild("MainInterface")
    if not mainInterface then
        return false
    end

    local function tryClickButton()
        local ok, target = pcall(function()
            return mainInterface:GetChildren()[61]:GetChildren()[7].ImageButton
        end)

        if not ok or not target then
            return false
        end

        if not target:IsA("GuiButton") then
            local ancestor = target
            while ancestor and not ancestor:IsA("GuiButton") do
                ancestor = ancestor.Parent
            end
            target = ancestor
        end

        if not target or not target:IsA("GuiButton") then
            return false
        end

        local center = target.AbsolutePosition + (target.AbsoluteSize / 2)

        if typeof(mousemoveabs) == "function" and typeof(mouse1click) == "function" then
            mousemoveabs(center.X, center.Y)
            task.wait(0.08)
            mouse1click()
            return true
        end

        VirtualInputManager:SendMouseMoveEvent(center.X, center.Y, 0)
        VirtualInputManager:SendMouseButtonEvent(center.X, center.Y, 0, true, game, 0)
        task.wait(0.1)
        VirtualInputManager:SendMouseButtonEvent(center.X, center.Y, 0, false, game, 0)
        return true
    end

    for _ = 1, 3 do
        if tryClickButton() then
            return true
        end
        task.wait(0.15)
    end

    return false
end

local function pathfindTo(destination, label)
    if pathfindingBusy then
        return false
    end

    pathfindingBusy = true

    local character = player.Character or player.CharacterAdded:Wait()
    local humanoid = character:WaitForChild("Humanoid")
    local root = character:WaitForChild("HumanoidRootPart")

    humanoid.AutoJumpEnabled = true
    humanoid.UseJumpPower = true
    humanoid.JumpPower = 56

    local rayParams = RaycastParams.new()
    rayParams.FilterType = Enum.RaycastFilterType.Exclude
    rayParams.FilterDescendantsInstances = { character }

    local function cancelled()
        return stopPathfinding
    end

    local function atDestination(strict)
        local radius = strict and ARRIVAL_RADIUS or RELAXED_ARRIVAL_RADIUS
        return (root.Position - destination).Magnitude <= radius
    end

    local function forceJump()
        humanoid.Jump = true
        humanoid:ChangeState(Enum.HumanoidStateType.Jumping)
    end

    local function jumpRecovery()
        for _ = 1, 2 do
            if cancelled() then
                return
            end
            forceJump()
            task.wait(0.08)
        end
    end

    local function lowObstacleAhead()
        local result = workspace:Raycast(
            root.Position + Vector3.new(0, 2, 0),
            root.CFrame.LookVector * 5,
            rayParams
        )

        if not result or not result.Instance or not result.Instance.CanCollide then
            return false
        end

        local part = result.Instance
        local topY = part.Position.Y + part.Size.Y * 0.5
        local feetY = root.Position.Y - 3
        local climb = topY - feetY
        return climb >= PREJUMP_MIN and climb <= 6.5
    end

    local function waitMove(timeout)
        local finished = false
        local reached = false
        local conn = humanoid.MoveToFinished:Connect(function(ok)
            finished = true
            reached = ok
        end)

        local deadline = time() + timeout
        while not finished and time() < deadline do
            if cancelled() then
                break
            end
            task.wait()
        end

        conn:Disconnect()
        return finished and reached
    end

    local function moveTo(position)
        humanoid:MoveTo(position)
        return waitMove(MOVE_TIMEOUT)
    end

    local function buildPath()
        local path = PathfindingService:CreatePath({
            AgentRadius = 1.5,
            AgentHeight = 5,
            AgentCanJump = true,
            AgentJumpHeight = 14,
            AgentMaxSlope = 45,
            WaypointSpacing = 2
        })
        path:ComputeAsync(root.Position, destination)
        return path
    end

    local retries = 0
    while retries <= MAX_RETRIES and not cancelled() and not atDestination(true) do
        local path = buildPath()
        local ok = false

        if path.Status == Enum.PathStatus.Success then
            ok = true
            local waypoints = path:GetWaypoints()

            if #waypoints == 0 then
                forceJump()
                ok = moveTo(destination)
            else
                for _, waypoint in ipairs(waypoints) do
                    if cancelled() then
                        ok = false
                        break
                    end

                    local dy = waypoint.Position.Y - root.Position.Y
                    if waypoint.Action == Enum.PathWaypointAction.Jump
                        or (dy >= PREJUMP_MIN and dy <= PREJUMP_MAX)
                        or lowObstacleAhead() then
                        forceJump()
                    end

                    if not moveTo(waypoint.Position) then
                        forceJump()
                        ok = false
                        break
                    end

                    if atDestination(true) then
                        ok = true
                        break
                    end
                end
            end
        else
            forceJump()
            ok = moveTo(destination)
        end

        if ok or atDestination(true) then
            break
        end

        retries += 1
        if retries <= MAX_RETRIES then
            jumpRecovery()
        end
    end

    local reached = atDestination(true) or atDestination(false)
    if reached then
        print(label, atDestination(true) and "arrived." or "arrived (relaxed check).")
    elseif retries > MAX_RETRIES then
        warn(label, "stopped after max retries.")
    else
        warn(label, "stopped before arrival.")
    end

    pathfindingBusy = false
    stopPathfinding = false
    return reached
end

local function runSellSequence()
    print("[Fishing] Selling fish inventory...")
    for _, fishName in ipairs(AllFishes) do
        Fishing.sellAllFish.send(fishName)
        Fishing.sellAllFishConfirm.send(true)
    end
    print("[Fishing] Sell sequence complete.")
end

local function runSellCycle()
    if cycleInTransit then
        return
    end

    local shouldResumeAutoFish = autoFishEnabled

    cycleInTransit = true
    setAutoFishEnabled(false, "sell cycle")
    isFishing = false
    Fishing.quit.send()

    local reachedSeller = pathfindTo(SELL_DESTINATION, "[Pathfinding] Seller path")
    if reachedSeller then
        runSellSequence()
        pathfindTo(RETURN_DESTINATION, "[Pathfinding] Return path")
    end

    cycleFishCaught = 0
    player:SetAttribute("FishCaughtCycle", cycleFishCaught)
    cycleInTransit = false

    if shouldResumeAutoFish then
        setAutoFishEnabled(true, "sell cycle complete")
        print("[Fishing] Resuming auto-fishing.")
    else
        print("[Fishing] Sell cycle complete. Auto-fishing remains off.")
    end
end

local function onFishCaught(data)
    local fishName = extractFishName(data)
    totalFishCaught += 1
    cycleFishCaught += 1
    fishByType[fishName] = (fishByType[fishName] or 0) + 1

    player:SetAttribute("FishCaughtTotal", totalFishCaught)
    player:SetAttribute("FishCaughtCycle", cycleFishCaught)
    player:SetAttribute("FishCaught_" .. fishName, fishByType[fishName])

    print("[Fishing] Fish caught:", fishName)
    print("[Fishing] Total:", totalFishCaught, "| Cycle:", cycleFishCaught, "|", fishName .. ":", fishByType[fishName])

    if cycleFishCaught >= targetFishCount and not cycleInTransit then
        task.spawn(runSellCycle)
    end
end

-- Input controls
UserInputService.InputBegan:Connect(function(input, gameProcessed)
    if gameProcessed then
        return
    end

    if input.KeyCode == Enum.KeyCode.RightBracket then
        stopPathfinding = true
        warn("[Pathfinding] Stop key pressed, pathing cancelled.")
    end
end)

-- Fishing events
Fishing.beginFishing.listen(function()
    isFishing = true
end)

Fishing.beginNetFishing.listen(function()
    isFishing = true
end)

Fishing.reward.listen(function(data)
    onFishCaught(data)
    task.spawn(clickCloseButton)
    isFishing = false
end)

Fishing.quit.listen(function()
    isFishing = false
end)

Fishing.playGame.listen(function(data)
    if not autoFishEnabled or cycleInTransit then
        return
    end

    local endTime = tonumber(data.endTimestamp)
    if not endTime then
        return
    end

    while true do
        local remaining = endTime - workspace:GetServerTimeNow()
        if remaining <= 1.5 then
            break
        end
        task.wait()
    end

    Fishing.gameResult.send(true)
end)

-- Old stable loop (kept simple)
task.spawn(function()
    while true do
        task.wait(0.5)

        if autoFishEnabled and not isFishing and not cycleInTransit then
            local id = select(1, findSpot(20))
            if id then
                Fishing.start.send(id)
                isFishing = true
            end
        end
    end
end)

-- MAIN TAB UI
Tabs.Main:AddParagraph({
    Title = "feesh",
    Content = "Set fish target, then enable Auto Fish"
})

local autoFishToggle = Tabs.Main:AddToggle("AutoFishToggle", {
    Title = "Auto Fish",
    Description = "Toggle fishing loop on/off",
    Default = autoFishEnabled
})

autoFishToggle:OnChanged(function(value)
    setAutoFishEnabled(value, "ui")
end)

local targetInput = Tabs.Main:AddInput("FishTargetCount", {
    Title = "Fish Target Before Sell",
    Default = tostring(targetFishCount),
    Placeholder = "Enter 0 or more",
    Numeric = true,
    Finished = true,
    Callback = function(value)
        setTargetFishCount(value)
    end
})

targetInput:OnChanged(function()
    if tonumber(targetInput.Value) then
        setTargetFishCount(targetInput.Value)
    end
end)

Tabs.Main:AddButton({
    Title = "Run Sell Cycle Now",
    Description = "Path to seller, sell, return",
    Callback = function()
        task.spawn(runSellCycle)
    end
})

Fluent:Notify({
    Title = "Fishing Script",
    Content = "Loaded. Configure target and enable Auto Fish.",
    Duration = 6
})

-- Addons
SaveManager:SetLibrary(Fluent)
InterfaceManager:SetLibrary(Fluent)
SaveManager:IgnoreThemeSettings()
SaveManager:SetIgnoreIndexes({})
InterfaceManager:SetFolder("FluentScriptHub")
SaveManager:SetFolder("FluentScriptHub/specific-game")
InterfaceManager:BuildInterfaceSection(Tabs.Settings)
SaveManager:BuildConfigSection(Tabs.Settings)

Window:SelectTab(1)
SaveManager:LoadAutoloadConfig()
