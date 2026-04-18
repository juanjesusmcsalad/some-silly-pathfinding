local Players = game:GetService("Players")
local PathfindingService = game:GetService("PathfindingService")
local UserInputService = game:GetService("UserInputService")
local player = Players.LocalPlayer
local character = player.Character or player.CharacterAdded:Wait()
local humanoid = character:WaitForChild("Humanoid")
local root = character:WaitForChild("HumanoidRootPart")
local destination = Vector3.new(100.5, 107.5, -296.75)
local MAX_RETRIES = 5
local MOVE_TIMEOUT = 2
local ARRIVAL_RADIUS = 2.5
local PREJUMP_MIN = 1.5
local PREJUMP_MAX = 3.0

local stopFlag = false
UserInputService.InputBegan:Connect(function(input, gameProcessed)
    if not gameProcessed and input.KeyCode == Enum.KeyCode.RightBracket then
        stopFlag = true
        warn("Stop key pressed, pathing cancelled.")
    end
end)

humanoid.AutoJumpEnabled = true
humanoid.UseJumpPower = true
humanoid.JumpPower = 20

local rayParams = RaycastParams.new()
rayParams.FilterType = Enum.RaycastFilterType.Exclude
rayParams.FilterDescendantsInstances = { character }

local function cancelled()
    return stopFlag
end

local function atDestination()
    return (root.Position - destination).Magnitude <= ARRIVAL_RADIUS
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
    local result = workspace:Raycast(root.Position + Vector3.new(0, 2, 0), root.CFrame.LookVector * 5, rayParams)
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
    local finished, reached = false, false
    local conn = humanoid.MoveToFinished:Connect(function(ok)
        finished, reached = true, ok
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
while retries <= MAX_RETRIES and not cancelled() and not atDestination() do
    local path = buildPath()
    local ok = false

    if path.Status == Enum.PathStatus.Success then
        ok = true
        local waypoints = path:GetWaypoints()
        if #waypoints == 0 then
            forceJump()
            ok = moveTo(destination)
        else
            for _, wp in ipairs(waypoints) do
                if cancelled() then
                    ok = false
                    break
                end

                local dy = wp.Position.Y - root.Position.Y
                if wp.Action == Enum.PathWaypointAction.Jump or (dy >= PREJUMP_MIN and dy <= PREJUMP_MAX) or lowObstacleAhead() then
                    forceJump()
                end

                if not moveTo(wp.Position) then
                    forceJump()
                    ok = false
                    break
                end

                if atDestination() then
                    ok = true
                    break
                end
            end
        end
    else
        forceJump()
        ok = moveTo(destination)
    end

    if ok or atDestination() then
        break
    end

    retries = retries + 1
    if retries <= MAX_RETRIES then
        jumpRecovery()
    end
end