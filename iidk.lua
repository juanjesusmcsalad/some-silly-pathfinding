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
        Main = Window:AddTab({ Title = "Main", Icon = "home" }),
        Eggs = Window:AddTab({ Title = "EGGS", Icon = "egg" }),
        Merchants = Window:AddTab({ Title = "MERCHANTS", Icon = "banknote" }),
        Tests = Window:AddTab({ Title = "TESTS", Icon = "flask-conical" }),
        Settings = Window:AddTab({ Title = "Settings", Icon = "settings" })
    }

    -- FEATURES --
    local ReplicatedStorage = game:GetService("ReplicatedStorage")
    local Players = game:GetService("Players")
    local UserInputService = game:GetService("UserInputService")
    local PathfindingService = game:GetService("PathfindingService")
    local VirtualInputManager = game:GetService("VirtualInputManager")
    local HttpService = game:GetService("HttpService")

    local Fishing = require(ReplicatedStorage.Packets.Fishing)
    local rollp = require(game:GetService("ReplicatedStorage"):WaitForChild("Packets"):WaitForChild("Rolling"))
    local player = Players.LocalPlayer
    local merchantStore = nil
    local exchangeList = nil

    pcall(function()
        merchantStore = require(ReplicatedStorage:WaitForChild("Modules"):WaitForChild("UI"):WaitForChild("Quad")).Store("merchant_store")
    end)

    pcall(function()
        exchangeList = require(ReplicatedStorage:WaitForChild("Storage"):WaitForChild("ExchangeList"))
    end)

    local isFishing = false
    local autoFishEnabled = false
    local cycleInTransit = false
    local pathfindingBusy = false
    local stopPathfinding = false

    local totalFishCaught = 0
    local cycleFishCaught = 0
    local targetFishCount = 1
    local fishByType = {}
    local currentFishingSpotId = nil
    local rollpLoopEnabled = false
    local rollpLoopRunning = false

    local merchantConfigDir = "FluentScriptHub"
    local merchantConfigPath = merchantConfigDir .. "/merchant_roles.json"
    local MERCHANT_ROLES = {
        mari = "1495337413411864606",
        jester = "1495337444143399005",
        rin = "1495337466201247836"
    }
    local MERCHANT_MESSAGES = {
        mari = "mari merchant spawned for {player}",
        jester = "jester merchant spawned for {player}",
        rin = "rin merchant spawned for {player}"
    }
    local merchantPingEnabled = true
    local merchantWebhookUrl = ""
    local merchantStockPingEnabled = true

    local function saveMerchantRoles()
        if not writefile then
            return
        end

        local ok, err = pcall(function()
            if makefolder and not (isfolder and isfolder(merchantConfigDir)) then
                makefolder(merchantConfigDir)
            end

            writefile(merchantConfigPath, HttpService:JSONEncode({
                roles = MERCHANT_ROLES,
                messages = MERCHANT_MESSAGES,
                enabled = merchantPingEnabled,
                stockEnabled = merchantStockPingEnabled,
                webhookUrl = merchantWebhookUrl
            }))
        end)

        if not ok then
            warn("[Merchant] Failed to save role config:", err)
        end
    end

    local function loadMerchantRoles()
        if not readfile or not isfile or not isfile(merchantConfigPath) then
            return
        end

        local ok, decoded = pcall(function()
            return HttpService:JSONDecode(readfile(merchantConfigPath))
        end)

        if not ok or typeof(decoded) ~= "table" then
            warn("[Merchant] Invalid merchant role config, using defaults.")
            return
        end

        local rolesSource = (typeof(decoded.roles) == "table") and decoded.roles or decoded
        local messagesSource = (typeof(decoded.messages) == "table") and decoded.messages or nil

        for key, value in pairs(rolesSource) do
            if MERCHANT_ROLES[key] ~= nil and typeof(value) == "string" then
                MERCHANT_ROLES[key] = value
            end
        end

        if messagesSource then
            for key, value in pairs(messagesSource) do
                if MERCHANT_MESSAGES[key] ~= nil and typeof(value) == "string" then
                    MERCHANT_MESSAGES[key] = value
                end
            end
        end

        if typeof(decoded.enabled) == "boolean" then
            merchantPingEnabled = decoded.enabled
        end

        if typeof(decoded.stockEnabled) == "boolean" then
            merchantStockPingEnabled = decoded.stockEnabled
        end

        if typeof(decoded.webhookUrl) == "string" then
            merchantWebhookUrl = decoded.webhookUrl
        end
    end

    loadMerchantRoles()

    local SELL_DESTINATION = Vector3.new(100.5, 107.5, -296.75)
    local RETURN_DESTINATION = Vector3.new(55, 97.5000153, -280)

    local MAX_RETRIES = 5
    local MOVE_TIMEOUT = 2
    local ARRIVAL_RADIUS = 2.5
    local RELAXED_ARRIVAL_RADIUS = 10
    local PREJUMP_MIN = 1.5
    local PREJUMP_MAX = 3.0
    local START_RETRY_DELAY = 0.7
    local STALE_FISHING_TIMEOUT = 8
    local lastStartAttempt = 0
    local lastFishingActivity = time()

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
        lastFishingActivity = time()

        if not autoFishEnabled and isFishing then
            Fishing.quit.send()
        end

        if not autoFishEnabled then
            isFishing = false
            lastStartAttempt = 0
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

    local function getSpotById(spotId)
        if not spotId then
            return nil
        end

        for _, spot in ipairs(workspace.Map.Miscs.FishingPoints:GetChildren()) do
            if spot:GetAttribute("spotId") == spotId then
                return spot
            end
        end

        return nil
    end

    local function resolveFishingSpotId()
        local nearby = select(1, findSpot(20))
        if nearby then
            return nearby
        end

        local currentSpot = getSpotById(currentFishingSpotId)
        if currentSpot then
            return currentFishingSpotId
        end

        local anyUnoccupied = select(1, findSpot(nil))
        if anyUnoccupied then
            return anyUnoccupied
        end

        local points = workspace.Map.Miscs.FishingPoints:GetChildren()
        local fallbackSpot = points[1]
        return fallbackSpot and fallbackSpot:GetAttribute("spotId") or nil
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
        for i, fishName in ipairs(AllFishes) do
            Fishing.sellAllFish.send(fishName)
            Fishing.sellAllFishConfirm.send(true)
            if i % 10 == 0 then
                task.wait()
            end
        end
        print("[Fishing] Sell sequence complete.")
    end

    local function runSellCycle()
        if cycleInTransit then
            return
        end

        local shouldResumeAutoFish = autoFishEnabled

        cycleInTransit = true
        local ok, err = pcall(function()
            setAutoFishEnabled(false, "sell cycle")
            isFishing = false
            lastStartAttempt = 0
            currentFishingSpotId = nil
            Fishing.quit.send()

            local reachedSeller = pathfindTo(SELL_DESTINATION, "[Pathfinding] Seller path")
            if reachedSeller then
                runSellSequence()
                pathfindTo(RETURN_DESTINATION, "[Pathfinding] Return path")
            end

            cycleFishCaught = 0
            player:SetAttribute("FishCaughtCycle", cycleFishCaught)
        end)

        cycleInTransit = false
        pathfindingBusy = false
        stopPathfinding = false

        if not ok then
            warn("[Fishing] Sell cycle error:", err)
        end

        if shouldResumeAutoFish then
            setAutoFishEnabled(true, ok and "sell cycle complete" or "sell cycle recovered")
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

        if autoFishEnabled and targetFishCount > 0 and cycleFishCaught >= targetFishCount and not cycleInTransit then
            task.spawn(runSellCycle)
        end
    end

    local function runRollpLoop()
        if rollpLoopRunning then
            return
        end

        rollpLoopRunning = true
        while rollpLoopEnabled do
            rollp.SendResponse.send(1)
            task.wait(0.001)
        end
        rollpLoopRunning = false
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
        lastStartAttempt = 0
        lastFishingActivity = time()
    end)

    Fishing.beginNetFishing.listen(function()
        isFishing = true
        lastStartAttempt = 0
        lastFishingActivity = time()
    end)

    Fishing.reward.listen(function(data)
        onFishCaught(data)
        isFishing = false
        lastStartAttempt = 0
        lastFishingActivity = time()
    end)

    Fishing.quit.listen(function()
        isFishing = false
        lastStartAttempt = 0
        lastFishingActivity = time()
    end)

    Fishing.playGame.listen(function(data)
        if not autoFishEnabled or cycleInTransit then
            return
        end

        lastFishingActivity = time()

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
                if time() - lastStartAttempt < START_RETRY_DELAY then
                    continue
                end

                local id = resolveFishingSpotId()
                if id then
                    currentFishingSpotId = id
                    lastStartAttempt = time()
                    lastFishingActivity = time()
                    Fishing.start.send(id)
                end
            end
        end
    end)

    task.spawn(function()
        while true do
            task.wait(1)

            if not autoFishEnabled or cycleInTransit then
                continue
            end

            if isFishing and (time() - lastFishingActivity) > STALE_FISHING_TIMEOUT then
                warn("[Fishing] Stale fishing state detected, recovering.")
                isFishing = false
                lastStartAttempt = 0
                lastFishingActivity = time()
            end
        end
    end)

    -- MAIN TAB UI
    Tabs.Main:AddParagraph({
        Title = "feesh",
        Content = "set the amount then enable ez"
    })

    local autoFishToggle = Tabs.Main:AddToggle("AutoFishToggle", {
        Title = "auto fish",
        Description = "auto fishes duhh",
        Default = autoFishEnabled
    })

    autoFishToggle:OnChanged(function(value)
        setAutoFishEnabled(value, "ui")
    end)

    local targetInput = Tabs.Main:AddInput("FishTargetCount", {
        Title = "amount of fish b4 selling",
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
        Title = "if you want to sell without waiting for target count enable this",
        Description = "title",
        Callback = function()
            task.spawn(runSellCycle)
        end
    })

    Tabs.Main:AddButton({
        Title = "doops the current aura",
        Description = "doop",
        Callback = function()
            rollp.SendResponse.send(1)
        end
    })

    Tabs.Main:AddToggle("DoopLoopToggle", {
        Title = "doop loop",
        Description = "super very super fast doop loop oke thx be careful",
        Default = false
    }):OnChanged(function(value)
        rollpLoopEnabled = value

        if rollpLoopEnabled then
            task.spawn(runRollpLoop)
        end
    end)

    Tabs.Merchants:AddParagraph({
        Title = "merchant pings",
        Content = "set role ids here. values are saved and auto-loaded next launch"
    })

    Tabs.Merchants:AddToggle("MerchantPingEnabledToggle", {
        Title = "merchant spawn ping enabled",
        Description = "if off, no webhook is sent on merchant spawn",
        Default = merchantPingEnabled
    }):OnChanged(function(value)
        merchantPingEnabled = value
        saveMerchantRoles()
    end)

    Tabs.Merchants:AddToggle("MerchantStockPingEnabledToggle", {
        Title = "include merchant stock in ping",
        Description = "adds detected stock list to merchant spawn message",
        Default = merchantStockPingEnabled
    }):OnChanged(function(value)
        merchantStockPingEnabled = value
        saveMerchantRoles()
    end)

    Tabs.Merchants:AddInput("MerchantWebhookUrl", {
        Title = "webhook url",
        Default = merchantWebhookUrl,
        Placeholder = "paste discord webhook link",
        Numeric = false,
        Finished = true,
        Callback = function(value)
            merchantWebhookUrl = tostring(value or "")
            saveMerchantRoles()
        end
    })

    Tabs.Merchants:AddInput("MerchantMariRoleId", {
        Title = "mari role id",
        Default = MERCHANT_ROLES.mari,
        Placeholder = "discord role id",
        Numeric = true,
        Finished = true,
        Callback = function(value)
            MERCHANT_ROLES.mari = tostring(value or "")
            saveMerchantRoles()
        end
    })

    Tabs.Merchants:AddInput("MerchantJesterRoleId", {
        Title = "jester role id",
        Default = MERCHANT_ROLES.jester,
        Placeholder = "discord role id",
        Numeric = true,
        Finished = true,
        Callback = function(value)
            MERCHANT_ROLES.jester = tostring(value or "")
            saveMerchantRoles()
        end
    })

    Tabs.Merchants:AddInput("MerchantRinRoleId", {
        Title = "rin role id",
        Default = MERCHANT_ROLES.rin,
        Placeholder = "discord role id",
        Numeric = true,
        Finished = true,
        Callback = function(value)
            MERCHANT_ROLES.rin = tostring(value or "")
            saveMerchantRoles()
        end
    })

    Tabs.Merchants:AddParagraph({
        Title = "merchant messages",
        Content = "custom message per merchant. use {player} and {merchant} placeholders"
    })

    Tabs.Merchants:AddInput("MerchantMariMessage", {
        Title = "mari message",
        Default = MERCHANT_MESSAGES.mari,
        Placeholder = "mari merchant spawned for {player}",
        Numeric = false,
        Finished = true,
        Callback = function(value)
            MERCHANT_MESSAGES.mari = tostring(value or "")
            saveMerchantRoles()
        end
    })

    Tabs.Merchants:AddInput("MerchantJesterMessage", {
        Title = "jester message",
        Default = MERCHANT_MESSAGES.jester,
        Placeholder = "jester merchant spawned for {player}",
        Numeric = false,
        Finished = true,
        Callback = function(value)
            MERCHANT_MESSAGES.jester = tostring(value or "")
            saveMerchantRoles()
        end
    })

    Tabs.Merchants:AddInput("MerchantRinMessage", {
        Title = "rin message",
        Default = MERCHANT_MESSAGES.rin,
        Placeholder = "rin merchant spawned for {player}",
        Numeric = false,
        Finished = true,
        Callback = function(value)
            MERCHANT_MESSAGES.rin = tostring(value or "")
            saveMerchantRoles()
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


    -- webhook section -- 
    local globalEnv = (getgenv and getgenv()) or _G
    if globalEnv.__MerchantWebhookNotifierLoaded then
        warn("[Merchant] Notifier already running. Skipping duplicate instance.")
        return
    end
    globalEnv.__MerchantWebhookNotifierLoaded = true

    local SPAWN_PING_COOLDOWN = 5
    local SPAWN_GLOBAL_DEDUPE_WINDOW = 2

    local requestFn = request
    if not requestFn then
        warn("[Webhook] request() is not available in this environment.")
        return
    end

    local merchantFolder = ReplicatedStorage:WaitForChild("Remote"):WaitForChild("Merchant")
    local lastSpawnPingAt = {}
    local lastSpawnPingGlobalAt = 0

    local function sendWebhook(content, roleId)
        local allowedMentions = { parse = {} }
        if roleId then
            allowedMentions.roles = { roleId }
        end

        local payload = {
            content = content,
            username = "merchant notifier",
            allowed_mentions = allowedMentions
        }

        if merchantWebhookUrl == "" then
            warn("[Webhook] Set webhook url in MERCHANTS tab before sending.")
            return
        end

        local ok, response = pcall(function()
            return requestFn({
                Url = merchantWebhookUrl,
                Method = "POST",
                Headers = {
                    ["Content-Type"] = "application/json"
                },
                Body = HttpService:JSONEncode(payload)
            })
        end)

        if not ok then
            warn("[Webhook] Request failed:", response)
            return
        end

        local statusCode = response and response.StatusCode
        if not statusCode or statusCode < 200 or statusCode >= 300 then
            warn("[Webhook] Ping may have failed. Status:", statusCode, "Body:", response and response.Body)
        end
    end

    local function formatMerchantMessage(merchantName)
        local template = MERCHANT_MESSAGES[merchantName]
        if not template or template == "" then
            template = merchantName .. " merchant spawned for {player}"
        end

        local playerName = player and player.Name or "unknown"
        local message = template
        message = message:gsub("{merchant}", merchantName)
        message = message:gsub("{player}", playerName)
        return message
    end

    local function sendMerchantTestPing(merchantName)
        local roleId = MERCHANT_ROLES[merchantName]
        local message = formatMerchantMessage(merchantName)
        if roleId and roleId ~= "" then
            sendWebhook("<@&" .. roleId .. "> " .. message, roleId)
        else
            sendWebhook(message)
        end
    end

    Tabs.Tests:AddParagraph({
        Title = "webhook tests",
        Content = "sends a test spawn ping for each merchant using role ids from merchants tab"
    })

    Tabs.Tests:AddButton({
        Title = "test mari ping",
        Description = "uses mari role id from merchants tab",
        Callback = function()
            sendMerchantTestPing("mari")
        end
    })

    Tabs.Tests:AddButton({
        Title = "test jester ping",
        Description = "uses jester role id from merchants tab",
        Callback = function()
            sendMerchantTestPing("jester")
        end
    })

    Tabs.Tests:AddButton({
        Title = "test rin ping",
        Description = "uses rin role id from merchants tab",
        Callback = function()
            sendMerchantTestPing("rin")
        end
    })

    local function detectMerchantNameFromValue(value)
        if value == nil then
            return nil
        end

        local valueType = typeof(value)
        if valueType == "string" then
            local s = string.lower(value)
            if s:find("mari", 1, true) then
                return "mari"
            end
            if s:find("jester", 1, true) then
                return "jester"
            end
            if s:find("rin", 1, true) then
                return "rin"
            end
            return nil
        end

        if valueType == "Instance" then
            return detectMerchantNameFromValue(value.Name)
        end

        if valueType ~= "table" then
            return detectMerchantNameFromValue(tostring(value))
        end

        for _, v in pairs(value) do
            local found = detectMerchantNameFromValue(v)
            if found then
                return found
            end
        end

        for k, v in pairs(value) do
            local foundKey = detectMerchantNameFromValue(k)
            if foundKey then
                return foundKey
            end

            local foundValue = detectMerchantNameFromValue(v)
            if foundValue then
                return foundValue
            end
        end

        return nil
    end

    local function detectMerchantName(...)
        local args = { ... }
        for _, arg in ipairs(args) do
            local found = detectMerchantNameFromValue(arg)
            if found then
                return found
            end
        end
        return nil
    end

    local function getMerchantDataRemote()
        local remote = merchantFolder:FindFirstChild("GetData") or merchantFolder:WaitForChild("GetData", 3)
        if remote and remote:IsA("RemoteFunction") then
            return remote
        end

        return nil
    end

    local function getMerchantProductList(merchantName)
        if typeof(merchantStore) ~= "table" then
            return nil
        end

        local infos = merchantStore.merchant_infos
        local info = infos and infos[merchantName]
        local productList = info and info.product_list
        if typeof(productList) ~= "table" then
            return nil
        end

        return productList
    end

    local function getProductDisplayName(productData, fallbackKey)
        if typeof(productData) == "table" then
            local name = productData.ProductName or productData.productName or productData.Name or productData.name
            if typeof(name) == "string" and name ~= "" then
                return name
            end
        end

        if typeof(fallbackKey) == "string" and fallbackKey ~= "" then
            return fallbackKey
        end

        return "Unknown"
    end

    local function getMerchantStockEntries(merchantName)
        local remote = getMerchantDataRemote()
        local productList = getMerchantProductList(merchantName)
        if not remote or not productList then
            return nil
        end

        local entries = {}

        for productIndex, productData in pairs(productList) do
            local ok, quantityInfo = pcall(function()
                return remote:InvokeServer("GetShopProductQuantityInfo", merchantName, productIndex)
            end)

            if ok and quantityInfo ~= nil then
                local productName = getProductDisplayName(productData, tostring(productIndex))
                local amount = nil

                if typeof(quantityInfo) == "number" then
                    amount = math.max(0, math.floor(quantityInfo))
                elseif typeof(quantityInfo) == "table" then
                    amount = tonumber(quantityInfo.Amount or quantityInfo.amount or quantityInfo.Quantity or quantityInfo.quantity or quantityInfo.Count or quantityInfo.count)
                    if amount then
                        amount = math.max(0, math.floor(amount))
                    end
                end

                if amount ~= nil then
                    entries[#entries + 1] = {
                        name = productName,
                        count = amount
                    }
                end
            end
        end

        table.sort(entries, function(a, b)
            return string.lower(a.name) < string.lower(b.name)
        end)

        return entries
    end

    local function formatMerchantStockSuffix(entries)
        if not entries or #entries == 0 then
            return ""
        end

        local parts = {}
        local maxItems = 8

        for i, entry in ipairs(entries) do
            if i > maxItems then
                parts[#parts + 1] = "+" .. tostring(#entries - maxItems) .. " more"
                break
            end

            if entry.count and entry.count > 1 then
                parts[#parts + 1] = entry.name .. " x" .. tostring(entry.count)
            else
                parts[#parts + 1] = entry.name
            end
        end

        return " | stock: " .. table.concat(parts, ", ")
    end

    local merchantSpawned = merchantFolder:FindFirstChild("MerchantSpawned") or merchantFolder:WaitForChild("MerchantSpawned", 5)
    if not merchantSpawned or not merchantSpawned:IsA("RemoteEvent") then
        warn("[Merchant] Missing or invalid RemoteEvent: MerchantSpawned")
        return
    end

    merchantSpawned.OnClientEvent:Connect(function(...)
        if not merchantPingEnabled then
            return
        end

        local now = time()
        if now - lastSpawnPingGlobalAt < SPAWN_GLOBAL_DEDUPE_WINDOW then
            return
        end

        local merchantName = detectMerchantName(...)
        local cooldownKey = merchantName or "unknown"
        local lastPing = lastSpawnPingAt[cooldownKey] or 0

        if now - lastPing < SPAWN_PING_COOLDOWN then
            return
        end

        local stockSuffix = ""
        if merchantStockPingEnabled and merchantName then
            local stockEntries = getMerchantStockEntries(merchantName)
            if stockEntries then
                stockSuffix = formatMerchantStockSuffix(stockEntries)
            end
        end

        lastSpawnPingGlobalAt = now
        lastSpawnPingAt[cooldownKey] = now

        if merchantName then
            local roleId = MERCHANT_ROLES[merchantName]
            local message = formatMerchantMessage(merchantName) .. stockSuffix
            if roleId then
                sendWebhook("<@&" .. roleId .. "> " .. message, roleId)
            else
                sendWebhook(message)
            end
            print("[Merchant] Spawned event received for", merchantName, stockSuffix ~= "" and stockSuffix or "")
        else
            local playerName = player and player.Name or "unknown"
            sendWebhook("merchant spawned unknown type for " .. playerName .. stockSuffix)
            warn("[Merchant] Spawned event received but merchant type was not recognized.")
        end
    end)

    print("[Merchant] Spawn webhook notifier loaded.")

    -- eggs section --
    local EGG_NAMES = {
        "point_egg_1",
        "point_egg_2",
        "point_egg_3",
        "point_egg_4",
        "point_egg_5",
        "point_egg_6"
    }

    local eggAutoEnabled = false
    local eggScannerRunning = false
    local eggStopRequested = false
    local eggScanInterval = 1

    local EGG_MOVE_TIMEOUT = 3
    local EGG_ARRIVAL_RADIUS = 4
    local EGG_RELAXED_ARRIVAL_RADIUS = 10
    local EGG_MAX_RETRIES = 5
    local EGG_PREJUMP_MIN = 1.5
    local EGG_PREJUMP_MAX = 3.0
    local EGG_PICKUP_WAIT = 1.25
    local EGG_BETWEEN_WAIT = 0.15
    local EGG_PROMPT_ATTEMPTS = 3
    local EGG_PROMPT_RETRY_DELAY = 0.2

    local function shouldStopEggs()
        return eggStopRequested or not eggAutoEnabled
    end

    local function setEggScanInterval(rawValue)
        local parsed = tonumber(rawValue)
        if not parsed then
            warn("[Egg] Invalid scan interval:", rawValue)
            return
        end

        eggScanInterval = math.max(0.2, parsed)
        print("[Egg] Scan interval set to", eggScanInterval)
    end

    local function getIgnoredEgglandFolder()
        local biomeHolder = workspace:FindFirstChild("BiomeStructureHolder")
        if not biomeHolder then
            return nil
        end

        return biomeHolder:FindFirstChild("Eggland")
    end

    local function isIgnoredEggInstance(instance)
        if not instance then
            return false
        end

        local ignoredFolder = getIgnoredEgglandFolder()
        return ignoredFolder ~= nil and instance:IsDescendantOf(ignoredFolder)
    end

    local function getEggInstance(eggName)
        local egg = workspace:FindFirstChild(eggName)
        if egg and not isIgnoredEggInstance(egg) then
            return egg
        end

        for _, child in ipairs(workspace:GetChildren()) do
            if child.Name == eggName and not isIgnoredEggInstance(child) then
                return child
            end
        end

        for _, descendant in ipairs(workspace:GetDescendants()) do
            if descendant.Name == eggName and not isIgnoredEggInstance(descendant) then
                return descendant
            end
        end

        return nil
    end

    local function getEggPart(egg)
        if not egg then
            return nil
        end

        if egg:IsA("BasePart") then
            return egg
        end

        if egg:IsA("Model") then
            if egg.PrimaryPart then
                return egg.PrimaryPart
            end

            local part = egg:FindFirstChildWhichIsA("BasePart", true)
            if part then
                return part
            end
        end

        return nil
    end

    local function eggPathfindTo(destination, label)
        if shouldStopEggs() then
            return false
        end

        local character = player.Character or player.CharacterAdded:Wait()
        local humanoid = character:WaitForChild("Humanoid")
        local root = character:WaitForChild("HumanoidRootPart")

        humanoid.AutoJumpEnabled = true
        humanoid.UseJumpPower = true
        humanoid.JumpPower = 56

        local rayParams = RaycastParams.new()
        rayParams.FilterType = Enum.RaycastFilterType.Exclude
        rayParams.FilterDescendantsInstances = { character }

        local function atDestination(strict)
            local radius = strict and EGG_ARRIVAL_RADIUS or EGG_RELAXED_ARRIVAL_RADIUS
            return (root.Position - destination).Magnitude <= radius
        end

        local function forceJump()
            humanoid.Jump = true
            humanoid:ChangeState(Enum.HumanoidStateType.Jumping)
        end

        local function jumpRecovery()
            for _ = 1, 2 do
                if shouldStopEggs() then
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
            return climb >= EGG_PREJUMP_MIN and climb <= 6.5
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
                if shouldStopEggs() then
                    break
                end
                task.wait()
            end

            conn:Disconnect()
            return finished and reached
        end

        local function moveTo(position)
            humanoid:MoveTo(position)
            return waitMove(EGG_MOVE_TIMEOUT)
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
        while retries <= EGG_MAX_RETRIES and not atDestination(true) do
            if shouldStopEggs() then
                warn(label, "stopped.")
                return false
            end

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
                        if shouldStopEggs() then
                            warn(label, "stopped.")
                            return false
                        end

                        local dy = waypoint.Position.Y - root.Position.Y
                        if waypoint.Action == Enum.PathWaypointAction.Jump
                            or (dy >= EGG_PREJUMP_MIN and dy <= EGG_PREJUMP_MAX)
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
            if retries <= EGG_MAX_RETRIES then
                jumpRecovery()
            end
        end

        local reached = atDestination(true) or atDestination(false)
        if reached then
            print(label, atDestination(true) and "arrived." or "arrived (relaxed check).")
        elseif retries > EGG_MAX_RETRIES then
            warn(label, "stopped after max retries.")
        else
            warn(label, "stopped before arrival.")
        end

        return reached
    end

    local function holdEggPrompt(prompt)
        local holdTime = tonumber(prompt and prompt.HoldDuration) or 0
        holdTime = math.max(holdTime, 0.25)

        if typeof(prompt) == "Instance" and prompt:IsA("ProximityPrompt") then
            local ok = pcall(function()
                prompt:FireProximityPrompt(holdTime + 0.1)
            end)

            if ok then
                return true
            end

            local held = pcall(function()
                prompt:InputHoldBegin()
                task.wait(holdTime)
                prompt:InputHoldEnd()
            end)

            if held then
                return true
            end
        end

        local began = false
        pcall(function()
            VirtualInputManager:SendKeyEvent(true, Enum.KeyCode.E, false, game)
            began = true
        end)

        if began then
            local deadline = time() + holdTime
            while time() < deadline do
                if shouldStopEggs() then
                    break
                end
                task.wait()
            end

            pcall(function()
                VirtualInputManager:SendKeyEvent(false, Enum.KeyCode.E, false, game)
            end)

            pcall(function()
                VirtualInputManager:SendKeyEvent(true, Enum.KeyCode.E, false, game)
                task.wait(0.12)
                VirtualInputManager:SendKeyEvent(false, Enum.KeyCode.E, false, game)
            end)

            return true
        end

        return false
    end

    local function triggerEggPrompts(egg)
        local found = false
        local success = false

        for _, descendant in ipairs(egg:GetDescendants()) do
            if descendant:IsA("ProximityPrompt") then
                found = true
                local ok = false
                pcall(function()
                    ok = holdEggPrompt(descendant)
                end)
                if ok then
                    success = true
                end
            end
        end

        return found, success
    end

    local function fallbackEggPickup(egg)
        local clicked = false
        if typeof(fireclickdetector) == "function" then
            for _, descendant in ipairs(egg:GetDescendants()) do
                if descendant:IsA("ClickDetector") then
                    pcall(function()
                        fireclickdetector(descendant)
                    end)
                    clicked = true
                end
            end
        end

        if typeof(firetouchinterest) == "function" then
            local part = getEggPart(egg)
            local character = player.Character
            local root = character and character:FindFirstChild("HumanoidRootPart")
            if root and part then
                pcall(function()
                    firetouchinterest(root, part, 0)
                    firetouchinterest(root, part, 1)
                end)
            end
        end

        return clicked
    end

    local function collectEgg(egg)
        if shouldStopEggs() or not egg or not egg.Parent then
            return false
        end

        local part = getEggPart(egg)
        if not part then
            return false
        end

        local target = part.Position + Vector3.new(0, 2.5, 0)
        if not eggPathfindTo(target, "[Egg] Path") then
            return false
        end

        local attempt = 0
        while attempt < EGG_PROMPT_ATTEMPTS and egg.Parent do
            if shouldStopEggs() then
                return false
            end

            attempt += 1
            local foundPrompt, promptSuccess = triggerEggPrompts(egg)
            if not foundPrompt then
                fallbackEggPickup(egg)
            end

            if not egg.Parent then
                return true
            end

            task.wait(promptSuccess and EGG_PICKUP_WAIT or EGG_PROMPT_RETRY_DELAY)
        end

        local start = time()
        while time() - start < EGG_PICKUP_WAIT do
            if shouldStopEggs() then
                return false
            end

            if not egg.Parent then
                return true
            end

            task.wait()
        end

        return not egg.Parent
    end

    local function collectAllEggs()
        local seen = {}
        local collected = 0

        for _, eggName in ipairs(EGG_NAMES) do
            if shouldStopEggs() then
                return collected
            end

            local egg = getEggInstance(eggName)
            if egg and not seen[egg] then
                seen[egg] = true
                print("[Egg] Heading to", eggName)
                if collectEgg(egg) then
                    collected += 1
                end
                task.wait(EGG_BETWEEN_WAIT)
            end
        end

        return collected
    end

    local function runEggScanner()
        if eggScannerRunning then
            return
        end

        eggScannerRunning = true
        task.spawn(function()
            while eggAutoEnabled and not eggStopRequested do
                local collectedThisScan = collectAllEggs()
                if collectedThisScan > 0 then
                    print("[Egg] Scan complete. Collected:", collectedThisScan)
                end

                local deadline = time() + eggScanInterval
                while time() < deadline do
                    if shouldStopEggs() then
                        break
                    end
                    task.wait()
                end
            end

            eggScannerRunning = false
            if eggStopRequested then
                print("[Egg] Scanner stopped by user.")
            else
                print("[Egg] Scanner stopped.")
            end
        end)
    end

    local function setEggAutoEnabled(enabled, source)
        eggAutoEnabled = enabled
        eggStopRequested = not enabled
        print("[Egg] Auto collector", eggAutoEnabled and "ENABLED" or "DISABLED", source and ("(" .. source .. ")") or "")

        if eggAutoEnabled then
            runEggScanner()
        end
    end

    UserInputService.InputBegan:Connect(function(input, gameProcessed)
        if gameProcessed then
            return
        end

        if input.KeyCode == Enum.KeyCode.T and eggAutoEnabled then
            setEggAutoEnabled(false, "T key")
        end
    end)

    Tabs.Eggs:AddParagraph({
        Title = "egg collector",
        Content = "one click toggle: on to run, off to stop"
    })

    Tabs.Eggs:AddToggle("EggAutoCollectorToggle", {
        Title = "auto egg collector",
        Description = "one click on/off",
        Default = eggAutoEnabled
    }):OnChanged(function(value)
        setEggAutoEnabled(value, "ui")
    end)
