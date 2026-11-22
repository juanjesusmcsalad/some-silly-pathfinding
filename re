local genv = (typeof(getgenv) == "function" and getgenv()) or _G
local Session = genv.AetherSession
if type(Session) ~= "table" then
	Session = genv.AetherRuntime and genv.AetherRuntime.new()
	if not Session then
		Session = { task = task }
		function Session:Track(x) return x end
		function Session:Service(n) return game:GetService(n) end
	end
	genv.AetherSession = Session
end
local task = Session.task


if Session.OnCleanup then
	Session:OnCleanup(function()
		for _, n in ipairs({ "Library", "Aether", "AetherUI", "AetherLibrary", "SaveTheme", "CurrentWindow" }) do
			genv[n] = nil
			if typeof(_G) == "table" then _G[n] = nil end
			if typeof(shared) == "table" then shared[n] = nil end
		end
	end)
end

local Library = Library or {}
local SaveTheme = SaveTheme or {}

local function exportGlobals(lib)
	if typeof(getgenv) == "function" then
		local genv = getgenv()
		genv.Library = lib
		genv.Aether = lib
		genv.AetherUI = lib
		genv.AetherLibrary = lib
		genv.SaveTheme = SaveTheme
	end
	if typeof(_G) == "table" then
		_G.Library = lib
		_G.Aether = lib
		_G.AetherUI = lib
		_G.AetherLibrary = lib
		_G.SaveTheme = SaveTheme
	end
	if typeof(shared) == "table" then
		shared.Library = lib
		shared.Aether = lib
		shared.AetherUI = lib
		shared.AetherLibrary = lib
		shared.SaveTheme = SaveTheme
	end
	pcall(function()
		local env = (getfenv and (getfenv(0) or getfenv(1)))
		if env then
			env.Library = lib
			env.Aether = lib
			env.AetherUI = lib
			env.AetherLibrary = lib
			env.SaveTheme = SaveTheme
		end
	end)
	Library = lib
	Aether = lib
	AetherUI = lib
	AetherLibrary = lib
end

exportGlobals(Library)

local themes = {
	index = {'Default', 'Halloween'},
	Default = {
		['Shadow'] = Color3.fromRGB(0, 0, 0),
		['Background'] = Color3.fromRGB(12, 12, 14),
		['Page'] = Color3.fromRGB(12, 12, 14),
		['Sidebar'] = Color3.fromRGB(14, 14, 17),
		['Card'] = Color3.fromRGB(19, 19, 23),
		['CardBorder'] = Color3.fromRGB(36, 36, 44),
		['Main'] = Color3.fromRGB(255, 255, 255),
		['MainDark'] = Color3.fromRGB(30, 30, 38),
		['MainLight'] = Color3.fromRGB(230, 230, 235),
		['Text & Icon'] = Color3.fromRGB(255, 255, 255),
		['SubText'] = Color3.fromRGB(140, 140, 150),
		['Success'] = Color3.fromRGB(34, 197, 94),
		['Function'] = {
			['Toggle'] = {
				['Background'] = Color3.fromRGB(19, 19, 23),
				['True'] = {
					['Toggle Background'] = Color3.fromRGB(255, 255, 255),
					['Toggle Value'] = Color3.fromRGB(14, 14, 17),
				},
				['False'] = {
					['Toggle Background'] = Color3.fromRGB(36, 36, 46),
					['Toggle Value'] = Color3.fromRGB(150, 150, 160),
				}
			},
			['Label'] = {
				['Background'] = Color3.fromRGB(19, 19, 23),
			},
			['Dropdown'] = {
				['Background'] = Color3.fromRGB(19, 19, 23),
				['Value Background'] = Color3.fromRGB(15, 15, 18),
				['Value Stroke'] = Color3.fromRGB(44, 44, 54),
				['Dropdown Select'] = {
					['Background'] = Color3.fromRGB(15, 15, 18),
					['Search'] = Color3.fromRGB(24, 24, 30),
					['Item Background'] = Color3.fromRGB(24, 24, 30),
				}
			},
			['Slider'] = {
				['Background'] = Color3.fromRGB(19, 19, 23),
				['Value Background'] = Color3.fromRGB(15, 15, 18),
				['Value Stroke'] = Color3.fromRGB(44, 44, 54),
				['Slider Bar'] = Color3.fromRGB(36, 36, 46),
				['Slider Bar Value'] = Color3.fromRGB(255, 255, 255),
				['Circle Value'] = Color3.fromRGB(255, 255, 255)
			},
			['Code'] = {
				['Background'] = ColorSequence.new{ColorSequenceKeypoint.new(0, Color3.fromRGB(19, 19, 23)), ColorSequenceKeypoint.new(1, Color3.fromRGB(19, 19, 23))},
				['Background Code'] = Color3.fromRGB(24, 24, 30),
				['Background Code Value'] = Color3.fromRGB(15, 15, 18),
				['ScrollingFrame Code'] = Color3.fromRGB(255, 255, 255)
			},
			['Button'] = {
				['Background'] = Color3.fromRGB(19, 19, 23),
				['Border'] = Color3.fromRGB(255, 255, 255),
				['Click'] = Color3.fromRGB(255, 255, 255)
			},
			['Textbox'] = {
				['Background'] = Color3.fromRGB(19, 19, 23),
				['Value Background'] = Color3.fromRGB(15, 15, 18),
				['Value Stroke'] = Color3.fromRGB(44, 44, 54),
			},
			['Keybind'] = {
				['Background'] = Color3.fromRGB(19, 19, 23),
				['Value Background'] = Color3.fromRGB(15, 15, 18),
				['Value Stroke'] = Color3.fromRGB(44, 44, 54),
				['True'] = {
					['Toggle Background'] = Color3.fromRGB(255, 255, 255),
					['Toggle Value'] = Color3.fromRGB(14, 14, 17),
				},
				['False'] = {
					['Toggle Background'] = Color3.fromRGB(36, 36, 46),
					['Toggle Value'] = Color3.fromRGB(150, 150, 160),
				}
			},
			['Color Picker'] = {
				['Background'] = Color3.fromRGB(19, 19, 23),
				['Color Select'] = {
					['Background'] = Color3.fromRGB(15, 15, 18),
					['UIStroke'] = Color3.fromRGB(44, 44, 54),
				}
			}
		}
	},
	Halloween = {
		['Shadow'] = Color3.fromRGB(200, 70, 0),
		['Background'] = Color3.fromRGB(13, 11, 12),
		['Page'] = Color3.fromRGB(13, 11, 12),
		['Sidebar'] = Color3.fromRGB(16, 13, 15),
		['Card'] = Color3.fromRGB(22, 18, 20),
		['CardBorder'] = Color3.fromRGB(56, 38, 30),
		['Main'] = Color3.fromRGB(255, 115, 20),
		['MainDark'] = Color3.fromRGB(58, 28, 12),
		['MainLight'] = Color3.fromRGB(255, 145, 45),
		['Text & Icon'] = Color3.fromRGB(255, 245, 235),
		['SubText'] = Color3.fromRGB(175, 150, 140),
		['Success'] = Color3.fromRGB(255, 140, 0),
		['Function'] = {
			['Toggle'] = {
				['Background'] = Color3.fromRGB(22, 18, 20),
				['True'] = {
					['Toggle Background'] = Color3.fromRGB(255, 115, 20),
					['Toggle Value'] = Color3.fromRGB(255, 255, 255),
				},
				['False'] = {
					['Toggle Background'] = Color3.fromRGB(40, 32, 35),
					['Toggle Value'] = Color3.fromRGB(150, 130, 120),
				}
			},
			['Label'] = {
				['Background'] = Color3.fromRGB(22, 18, 20),
			},
			['Dropdown'] = {
				['Background'] = Color3.fromRGB(22, 18, 20),
				['Value Background'] = Color3.fromRGB(18, 14, 16),
				['Value Stroke'] = Color3.fromRGB(56, 40, 34),
				['Dropdown Select'] = {
					['Background'] = Color3.fromRGB(18, 14, 16),
					['Search'] = Color3.fromRGB(28, 22, 25),
					['Item Background'] = Color3.fromRGB(28, 22, 25),
				}
			},
			['Slider'] = {
				['Background'] = Color3.fromRGB(22, 18, 20),
				['Value Background'] = Color3.fromRGB(18, 14, 16),
				['Value Stroke'] = Color3.fromRGB(56, 40, 34),
				['Slider Bar'] = Color3.fromRGB(45, 34, 30),
				['Slider Bar Value'] = Color3.fromRGB(255, 115, 20),
				['Circle Value'] = Color3.fromRGB(255, 255, 255)
			},
			['Code'] = {
				['Background'] = ColorSequence.new{ColorSequenceKeypoint.new(0, Color3.fromRGB(22, 18, 20)), ColorSequenceKeypoint.new(1, Color3.fromRGB(22, 18, 20))},
				['Background Code'] = Color3.fromRGB(28, 22, 25),
				['Background Code Value'] = Color3.fromRGB(18, 14, 16),
				['ScrollingFrame Code'] = Color3.fromRGB(255, 115, 20)
			},
			['Button'] = {
				['Background'] = Color3.fromRGB(22, 18, 20),
				['Border'] = Color3.fromRGB(255, 115, 20),
				['Click'] = Color3.fromRGB(255, 255, 255)
			},
			['Textbox'] = {
				['Background'] = Color3.fromRGB(22, 18, 20),
				['Value Background'] = Color3.fromRGB(18, 14, 16),
				['Value Stroke'] = Color3.fromRGB(56, 40, 34),
			},
			['Keybind'] = {
				['Background'] = Color3.fromRGB(22, 18, 20),
				['Value Background'] = Color3.fromRGB(18, 14, 16),
				['Value Stroke'] = Color3.fromRGB(56, 40, 34),
				['True'] = {
					['Toggle Background'] = Color3.fromRGB(255, 115, 20),
					['Toggle Value'] = Color3.fromRGB(255, 255, 255),
				},
				['False'] = {
					['Toggle Background'] = Color3.fromRGB(40, 32, 35),
					['Toggle Value'] = Color3.fromRGB(150, 130, 120),
				}
			},
			['Color Picker'] = {
				['Background'] = Color3.fromRGB(22, 18, 20),
				['Color Select'] = {
					['Background'] = Color3.fromRGB(18, 14, 16),
					['UIStroke'] = Color3.fromRGB(56, 40, 34),
				}
			}
		}
	}
}

local function deepCopy(orig)
	local orig_type = type(orig)
	local copy
	if orig_type == 'table' then
		copy = {}
		for orig_key, orig_value in next, orig, nil do
			copy[deepCopy(orig_key)] = deepCopy(orig_value)
		end
	else
		copy = orig
	end
	return copy
end

local CoreGui = game:GetService("CoreGui")
local Players = game:GetService("Players")
local HttpService = game:GetService("HttpService")
local U, Tw = Session:Service("UserInputService"), game:GetService("TweenService")
local SoundService = game:GetService("SoundService")
local LocalPlayer = Players.LocalPlayer

local function attachAutoScale(frame, baseW, baseH, fill)
	fill = fill or 0.9
	local scale = Instance.new("UIScale")
	scale.Name = "AutoScale"
	scale.Parent = frame
	local cam
	local camConn
	local function update()
		local c = workspace.CurrentCamera
		local vp = c and c.ViewportSize or Vector2.new(1920, 1080)
		if vp.X < 2 or vp.Y < 2 then return end
		scale.Scale = math.clamp(math.min((vp.X * fill) / baseW, (vp.Y * fill) / baseH, 1), 0.35, 1)
	end
	local function bindCamera()
		if camConn then camConn:Disconnect() end
		cam = workspace.CurrentCamera
		if cam then
			camConn = Session:Track(cam:GetPropertyChangedSignal("ViewportSize"):Connect(update))
		end
		update()
	end
	Session:Track(workspace:GetPropertyChangedSignal("CurrentCamera"):Connect(bindCamera))
	frame.Destroying:Connect(function() if camConn then camConn:Disconnect() end end)
	bindCamera()
	return scale
end

local function getGuiParent()
	local success, _ = pcall(function() return CoreGui:GetChildren() end)
	if success then return CoreGui end
	return LocalPlayer and LocalPlayer:FindFirstChildOfClass("PlayerGui") or game:GetService("StarterGui")
end

local parentGui = getGuiParent()
pcall(function()
	local parentsToSearch = {parentGui}
	if CoreGui and not table.find(parentsToSearch, CoreGui) then table.insert(parentsToSearch, CoreGui) end
	if typeof(gethui) == "function" then
		local h = gethui()
		if h and not table.find(parentsToSearch, h) then table.insert(parentsToSearch, h) end
	end
	local lp = Players.LocalPlayer
	local pg = lp and lp:FindFirstChildOfClass("PlayerGui")
	if pg and not table.find(parentsToSearch, pg) then table.insert(parentsToSearch, pg) end

	for _, p in ipairs(parentsToSearch) do
		for _, name in ipairs({"Aether", "Core"}) do
			local ex = p:FindFirstChild(name)
			if ex then
				pcall(function() ex:Destroy() end)
			end
		end
	end
end)

local ScreenGui = Instance.new("ScreenGui")
Session:Track(ScreenGui)
ScreenGui.Name = "Aether"
ScreenGui.ResetOnSpawn = false
ScreenGui.ZIndexBehavior = Enum.ZIndexBehavior.Sibling
ScreenGui.DisplayOrder = 999
ScreenGui.IgnoreGuiInset = false
ScreenGui:SetAttribute("AetherUI", true)
ScreenGui.Parent = parentGui

local FileSystem = {
	HasFS = (type(writefile) == "function" and type(readfile) == "function"),
	MemStore = { Configs = {}, Themes = {}, Wallpapers = {}, Autoload = "", WindowSize = nil }
}

function FileSystem:Init()
	if not self.HasFS then return end
	pcall(function()
		if type(isfolder) == "function" and type(makefolder) == "function" then
			if not isfolder("Aether") then makefolder("Aether") end
			if not isfolder("Aether/Configs") then makefolder("Aether/Configs") end
			if not isfolder("Aether/Themes") then makefolder("Aether/Themes") end
			if not isfolder("Aether/Themes/Wallpaper") then makefolder("Aether/Themes/Wallpaper") end
			if not isfolder("Aether/Themes/Sounds") then makefolder("Aether/Themes/Sounds") end
			if not isfolder("Aether/Themes/Cursors") then makefolder("Aether/Themes/Cursors") end
		end
	end)
end

function FileSystem:ListWallpapers()
	local list = {}
	if self.HasFS then
		pcall(function()
			self:Init()
			local dirs = { "Aether/Themes/Wallpaper", "Aether/Themes/Wallpapers", "Aether/Wallpaper" }
			if type(listfiles) == "function" and type(isfolder) == "function" then
				for _, dir in ipairs(dirs) do
					if isfolder(dir) then
						for _, path in ipairs(listfiles(dir)) do
							local fileName = path:match("([^/\\]+%.[pP][nN][gG])$")
								or path:match("([^/\\]+%.[jJ][pP][gG])$")
								or path:match("([^/\\]+%.[jJ][pP][eE][gG])$")
								or path:match("([^/\\]+%.[wW][eE][bB][pP])$")
								or path:match("([^/\\]+%.[bB][mM][pP])$")
							if fileName and not table.find(list, fileName) then
								table.insert(list, fileName)
							end
						end
					end
				end
			end
		end)
	else
		for k, _ in pairs(self.MemStore.Wallpapers or {}) do
			table.insert(list, k)
		end
	end
	return list
end

function FileSystem:GetWallpaperAsset(val)
	if not val or val == "" then return "" end
	local sVal = tostring(val)
	if sVal:match("^%d+$") then
		return "rbxassetid://" .. sVal
	elseif sVal:match("^rbxassetid://") or sVal:match("^rbxasset://") or sVal:match("^https?://") then
		return sVal
	end

	if self.MemStore and self.MemStore.Wallpapers and self.MemStore.Wallpapers[sVal] then
		return self.MemStore.Wallpapers[sVal]
	end

	local getAsset = (type(getcustomasset) == "function" and getcustomasset)
		or (type(getsynasset) == "function" and getsynasset)

	if getAsset and self.HasFS then
		local dirs = { "Aether/Themes/Wallpaper/", "Aether/Themes/Wallpapers/", "Aether/Wallpaper/" }
		for _, dir in ipairs(dirs) do
			local p = dir .. sVal
			if type(isfile) == "function" and isfile(p) then
				local s, res = pcall(function() return getAsset(p) end)
				if s and res then return res end
			end
		end
		if type(isfile) == "function" and isfile(sVal) then
			local s, res = pcall(function() return getAsset(sVal) end)
			if s and res then return res end
		end
	end

	return sVal
end

function FileSystem:ListSounds()
	local list = {}
	if self.HasFS then
		pcall(function()
			self:Init()
			local dirs = { "Aether/Themes/Sounds", "Aether/Sounds" }
			if type(listfiles) == "function" and type(isfolder) == "function" then
				for _, dir in ipairs(dirs) do
					if isfolder(dir) then
						for _, path in ipairs(listfiles(dir)) do
							local fileName = path:match("([^/\\]+%.[mM][pP]3)$")
								or path:match("([^/\\]+%.[oO][gG][gG])$")
								or path:match("([^/\\]+%.[wW][aA][vV])$")
							if fileName and not table.find(list, fileName) then
								table.insert(list, fileName)
							end
						end
					end
				end
			end
		end)
	end
	return list
end

function FileSystem:GetAudioAsset(val)
	if not val or val == "" then return "" end
	local sVal = tostring(val)
	if sVal:match("^%d+$") then
		return "rbxassetid://" .. sVal
	elseif sVal:match("^rbxassetid://") or sVal:match("^rbxasset://") or sVal:match("^https?://") then
		return sVal
	end

	local getAsset = (type(getcustomasset) == "function" and getcustomasset)
		or (type(getsynasset) == "function" and getsynasset)

	if getAsset and self.HasFS then
		local dirs = { "Aether/Themes/Sounds/", "Aether/Sounds/" }
		for _, dir in ipairs(dirs) do
			local p = dir .. sVal
			if type(isfile) == "function" and isfile(p) then
				local s, res = pcall(function() return getAsset(p) end)
				if s and res then return res end
			end
		end
		if type(isfile) == "function" and isfile(sVal) then
			local s, res = pcall(function() return getAsset(sVal) end)
			if s and res then return res end
		end
	end

	return sVal
end

function FileSystem:ListCursors()
	local list = {}
	if self.HasFS then
		pcall(function()
			self:Init()
			local dirs = { "Aether/Themes/Cursors", "Aether/Cursors" }
			if type(listfiles) == "function" and type(isfolder) == "function" then
				for _, dir in ipairs(dirs) do
					if isfolder(dir) then
						for _, path in ipairs(listfiles(dir)) do
							local fileName = path:match("([^/\\]+%.[pP][nN][gG])$")
								or path:match("([^/\\]+%.[jJ][pP][gG])$")
								or path:match("([^/\\]+%.[iI][cC][oO])$")
							if fileName and not table.find(list, fileName) then
								table.insert(list, fileName)
							end
						end
					end
				end
			end
		end)
	end
	return list
end

function FileSystem:GetCursorAsset(val)
	if not val or val == "" or val == "None" or val == "Default" or val == "Default (Roblox)" then return "" end
	local sVal = tostring(val)
	if sVal:match("^%d+$") then
		return "rbxassetid://" .. sVal
	elseif sVal:match("^rbxassetid://") or sVal:match("^rbxasset://") or sVal:match("^https?://") then
		return sVal
	end

	local getAsset = (type(getcustomasset) == "function" and getcustomasset)
		or (type(getsynasset) == "function" and getsynasset)

	if getAsset and self.HasFS then
		local dirs = { "Aether/Themes/Cursors/", "Aether/Cursors/" }
		for _, dir in ipairs(dirs) do
			local p = dir .. sVal
			if type(isfile) == "function" and isfile(p) then
				local s, res = pcall(function() return getAsset(p) end)
				if s and res then return res end
			end
		end
		if type(isfile) == "function" and isfile(sVal) then
			local s, res = pcall(function() return getAsset(sVal) end)
			if s and res then return res end
		end
	end

	return sVal
end

function FileSystem:SaveConfig(name, data)
	local encoded = HttpService:JSONEncode(data)
	if self.HasFS then
		pcall(function()
			self:Init()
			writefile("Aether/Configs/" .. name .. ".json", encoded)
		end)
	else
		self.MemStore.Configs[name] = encoded
	end
end

function FileSystem:LoadConfig(name)
	local content = nil
	if self.HasFS then
		pcall(function()
			if type(isfile) == "function" and isfile("Aether/Configs/" .. name .. ".json") then
				content = readfile("Aether/Configs/" .. name .. ".json")
			end
		end)
	else
		content = self.MemStore.Configs[name]
	end
	if content then
		local s, decoded = pcall(function() return HttpService:JSONDecode(content) end)
		if s and type(decoded) == "table" then return decoded end
	end
	return nil
end

function FileSystem:DeleteConfig(name)
	if self.HasFS then
		pcall(function()
			if type(delfile) == "function" and type(isfile) == "function" and isfile("Aether/Configs/" .. name .. ".json") then
				delfile("Aether/Configs/" .. name .. ".json")
			end
		end)
	else
		self.MemStore.Configs[name] = nil
	end
end

function FileSystem:ListConfigs()
	local list = {}
	if self.HasFS then
		pcall(function()
			self:Init()
			if type(listfiles) == "function" then
				for _, path in ipairs(listfiles("Aether/Configs")) do
					local fileName = path:match("([^/\\]+)%.json$")
					if fileName then table.insert(list, fileName) end
				end
			end
		end)
	else
		for name, _ in pairs(self.MemStore.Configs) do table.insert(list, name) end
	end
	return list
end

function FileSystem:SaveWindowSize(width, height)
	local data = { Width = math.round(width), Height = math.round(height) }
	local encoded = HttpService:JSONEncode(data)
	if self.HasFS then
		pcall(function()
			self:Init()
			writefile("Aether/WindowSize.json", encoded)
		end)
	else
		self.MemStore.WindowSize = encoded
	end
end

function FileSystem:LoadWindowSize()
	local content = nil
	if self.HasFS then
		pcall(function()
			if type(isfile) == "function" and isfile("Aether/WindowSize.json") then
				content = readfile("Aether/WindowSize.json")
			end
		end)
	else
		content = self.MemStore.WindowSize
	end
	if content then
		local s, decoded = pcall(function() return HttpService:JSONDecode(content) end)
		if s and type(decoded) == "table" and decoded.Width and decoded.Height then
			return decoded.Width, decoded.Height
		end
	end
	return nil, nil
end

function FileSystem:ResetWindowSize()
	if self.HasFS then
		pcall(function()
			if type(delfile) == "function" and type(isfile) == "function" and isfile("Aether/WindowSize.json") then
				delfile("Aether/WindowSize.json")
			end
		end)
	else
		self.MemStore.WindowSize = nil
	end
end

function FileSystem:SaveTheme(name, data)
	local encoded = HttpService:JSONEncode(data)
	if self.HasFS then
		pcall(function()
			self:Init()
			writefile("Aether/Themes/" .. name .. ".json", encoded)
		end)
	else
		self.MemStore.Themes[name] = encoded
	end
end

function FileSystem:LoadTheme(name)
	local content = nil
	if self.HasFS then
		pcall(function()
			if type(isfile) == "function" and isfile("Aether/Themes/" .. name .. ".json") then
				content = readfile("Aether/Themes/" .. name .. ".json")
			end
		end)
	else
		content = self.MemStore.Themes[name]
	end
	if content then
		local s, decoded = pcall(function() return HttpService:JSONDecode(content) end)
		if s and type(decoded) == "table" then return decoded end
	end
	return nil
end

function FileSystem:ListThemes()
	local list = {}
	if self.HasFS then
		pcall(function()
			if type(listfiles) == "function" and type(isfolder) == "function" and isfolder("Aether/Themes") then
				for _, path in ipairs(listfiles("Aether/Themes")) do
					local name = path:match("([^/\\]+)%.json$")
					if name then table.insert(list, name) end
				end
			end
		end)
	else
		for k, _ in pairs(self.MemStore.Themes) do table.insert(list, k) end
	end
	return list
end

function FileSystem:SetAutoload(name)
	if self.HasFS then
		pcall(function()
			self:Init()
			writefile("Aether/autoload.txt", name or "")
		end)
	else
		self.MemStore.Autoload = name or ""
	end
end

function FileSystem:GetAutoload()
	local name = ""
	if self.HasFS then
		pcall(function()
			if type(isfile) == "function" and isfile("Aether/autoload.txt") then
				name = readfile("Aether/autoload.txt")
			end
		end)
	else
		name = self.MemStore.Autoload
	end
	return (name and name ~= "") and name or nil
end

FileSystem:Init()

local currentWindowTheme = {
	name = "Default",
	colors = deepCopy(themes.Default),
	cards = {},
	sections = {},
	controls = {},
	tabs = {},
	topTabs = {},
	subTabs = {},
}

do
	function addToTheme(name, obj)
		if not SaveTheme[name] then
			SaveTheme[name] = {}
		end
		table.insert(SaveTheme[name], obj)
	end

	function getColorFromPath(tbl, path)
		local result = tbl
		for _, part in ipairs(string.split(path, ".")) do
			result = result and result[part]
		end
		return result
	end

	local IconList = nil
	pcall(function()
		IconList = loadstring(game:HttpGet('https://raw.githubusercontent.com/DuxiiT/Aether/refs/heads/main/Icons.lua'))()
	end)

	local IconAliases = {
		["home"] = "house",
		["scripts"] = "code",
		["script"] = "code",
		["games"] = "gamepad-2",
		["game"] = "gamepad-2",
		["executor"] = "package",
		["modules"] = "layout-grid",
		["module"] = "layout-grid",
		["settings"] = "settings",
		["gear"] = "settings",
		["profile"] = "user",
		["pulse"] = "activity",
		["timer"] = "clock",
		["lightning"] = "zap",
		["bolt"] = "zap",
		["tuning"] = "sliders",
	}

	local FallbackAssetIcons = {
		["house"] = "rbxassetid://10723407389",
		["code"] = "rbxassetid://10709790644",
		["gamepad-2"] = "rbxassetid://10709798000",
		["package"] = "rbxassetid://10709819149",
		["layout-grid"] = "rbxassetid://10709810948",
		["settings"] = "rbxassetid://10734950309",
		["zap"] = "rbxassetid://10709819266",
		["user"] = "rbxassetid://10747373176",
		["clock"] = "rbxassetid://10709789965",
		["activity"] = "rbxassetid://10709788052",
		["shield"] = "rbxassetid://10734952480",
		["file-text"] = "rbxassetid://10723387563",
		["link"] = "rbxassetid://10709810817",
		["play"] = "rbxassetid://10734942127",
		["bell"] = "rbxassetid://10709790100",
		["monitor"] = "rbxassetid://10709811124",
		["search"] = "rbxassetid://10734943902",
		["copy"] = "rbxassetid://10709790387",
		["check"] = "rbxassetid://10709790202",
		["chevron-right"] = "rbxassetid://10709791437",
		["discord"] = "rbxassetid://119690296342461",
	}

	function gl(i)
		if type(i) == "string" then
			local key = string.lower(i)
			if IconAliases[key] then key = IconAliases[key] end
			if key == "discord" then
				return { Image = "rbxassetid://119690296342461", ImageRectSize = Vector2.new(0, 0), ImageRectPosition = Vector2.new(0, 0) }
			end
			if IconList and IconList.Icons and IconList.Icons[key] then
				local iconData = IconList.Icons[key]
				local spriteSheet = IconList.Spritesheets and IconList.Spritesheets[tostring(iconData.Image)]
				if spriteSheet then
					return { Image = spriteSheet, ImageRectSize = iconData.ImageRectSize, ImageRectPosition = iconData.ImageRectPosition }
				end
			end
			if FallbackAssetIcons[key] then
				return { Image = FallbackAssetIcons[key], ImageRectSize = Vector2.new(0, 0), ImageRectPosition = Vector2.new(0, 0) }
			end
		end
		if type(i) == 'string' and not i:find('rbxassetid://') and tonumber(i) then
			return { Image = "rbxassetid://".. i, ImageRectSize = Vector2.new(0, 0), ImageRectPosition = Vector2.new(0, 0) }
		elseif type(i) == 'number' then
			return { Image = "rbxassetid://".. i, ImageRectSize = Vector2.new(0, 0), ImageRectPosition = Vector2.new(0, 0) }
		else
			return { Image = (type(i) == 'string' and i ~= '') and i or "rbxassetid://10723407389", ImageRectSize = Vector2.new(0, 0), ImageRectPosition = Vector2.new(0, 0) }
		end
	end

	function tw(info)
		if not info or not info.v or typeof(info.v) ~= "Instance" then
			return {Play = function() end, Cancel = function() end, Pause = function() end}
		end
		local ok, anim = pcall(Tw.Create, Tw, info.v, TweenInfo.new(info.t or 0.2, info.s or Enum.EasingStyle.Quad, Enum.EasingDirection[info.d or "Out"]), info.g)
		if ok and anim then
			return anim
		end
		return {Play = function() end, Cancel = function() end, Pause = function() end}
	end

	function changecanvas(ScrollingFrame, UIListLayout, Plus)
		UIListLayout:GetPropertyChangedSignal("AbsoluteContentSize"):Connect(function()
			ScrollingFrame.CanvasSize = UDim2.new(0, 0, 0, UIListLayout.AbsoluteContentSize.Y + (Plus or 10))
		end)
	end

	function lak(t, o)
		local a, b, c, d
		local function u(i)
			local dt = i.Position - c
			tw({v = o, t = 0.04, s = Enum.EasingStyle.Linear, d = "InOut", g = {Position = UDim2.new(d.X.Scale, math.round(d.X.Offset + dt.X), d.Y.Scale, math.round(d.Y.Offset + dt.Y))}}):Play()
		end
		t.InputBegan:Connect(function(i)
			if i.UserInputType == Enum.UserInputType.MouseButton1 or i.UserInputType == Enum.UserInputType.Touch then
				a = true
				c = i.Position
				d = o.Position
				i.Changed:Connect(function()
					if i.UserInputState == Enum.UserInputState.End then a = false end
				end)
			end
		end)
		t.InputChanged:Connect(function(i)
			if i.UserInputType == Enum.UserInputType.MouseMovement or i.UserInputType == Enum.UserInputType.Touch then b = i end
		end)
		U.InputChanged:Connect(function(i)
			if i == b and a then u(i) end
		end)
	end

	local playClickSound = function() end

	function click(p)
		local Click = Instance.new("TextButton")
		Click.Name = "Click"
		Click.Parent = p
		Click.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
		Click.BackgroundTransparency = 1
		Click.BorderSizePixel = 0
		Click.Size = UDim2.new(1, 0, 1, 0)
		Click.Font = Enum.Font.SourceSans
		Click.Text = ""
		Click.ZIndex = 5
		Click.MouseButton1Click:Connect(function()
			pcall(playClickSound)
		end)
		return Click
	end

	function jc(c, p)
		local Mouse = LocalPlayer:GetMouse()
		local relativeX = Mouse.X - c.AbsolutePosition.X
		local relativeY = Mouse.Y - c.AbsolutePosition.Y
		if relativeX < 0 or relativeY < 0 or relativeX > c.AbsoluteSize.X or relativeY > c.AbsoluteSize.Y then return end

		local Ripple = Instance.new("Frame")
		Ripple.Parent = p
		Ripple.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
		Ripple.BackgroundTransparency = 0.7
		Ripple.BorderSizePixel = 0
		Ripple.AnchorPoint = Vector2.new(0.5, 0.5)
		Ripple.Position = UDim2.new(0, relativeX, 0, relativeY)
		Ripple.Size = UDim2.new(0, 0, 0, 0)
		Ripple.ZIndex = 8

		local UICorner = Instance.new("UICorner")
		UICorner.CornerRadius = UDim.new(1, 0)
		UICorner.Parent = Ripple

		local expandTween = Tw:Create(Ripple, TweenInfo.new(0.5, Enum.EasingStyle.Quad, Enum.EasingDirection.Out), {
			Size = UDim2.new(0, c.AbsoluteSize.X * 1.5, 0, c.AbsoluteSize.X * 1.5),
			BackgroundTransparency = 1
		})
		expandTween.Completed:Connect(function() Ripple:Destroy() end)
		expandTween:Play()
	end

	function background(pl, t, d, i, ty)
		local Background = Instance.new("Frame")
		local UICorner_1 = Instance.new("UICorner")
		local UIStroke_1 = Instance.new("UIStroke")
		local T_1 = Instance.new("Frame")
		local UIListLayout_2 = Instance.new("UIListLayout")
		local UIPadding_3 = Instance.new("UIPadding")
		local TextLabel_1 = Instance.new("TextLabel")
		local TextLabel_2 = Instance.new("TextLabel")

		local curColors = (currentWindowTheme and currentWindowTheme.colors) or themes.Default
		local curTrans = (currentWindowTheme and currentWindowTheme.transparency) or {}

		Background.Name = "Background"
		Background.Parent = pl
		Background.BackgroundColor3 = curColors['Card']
		Background.BackgroundTransparency = (curTrans.Card or 0) / 100
		Background.BorderSizePixel = 0
		Background.Size = UDim2.new(1, 0, 0, (ty == "Label" or not (d and d ~= "")) and 48 or 54)
		Background.ClipsDescendants = true

		UICorner_1.CornerRadius = UDim.new(0, 6)
		UICorner_1.Parent = Background

		UIStroke_1.Color = curColors['CardBorder']
		UIStroke_1.Thickness = 1
		UIStroke_1.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
		UIStroke_1.Parent = Background

		T_1.Name = "T"
		T_1.Parent = Background
		T_1.AnchorPoint = Vector2.new(0, 0.5)
		T_1.BackgroundTransparency = 1
		T_1.BorderSizePixel = 0
		T_1.Position = UDim2.new(0, 0, 0.5, 0)
		T_1.Size = UDim2.new(1, 0, 1, 0)

		UIListLayout_2.Parent = T_1
		UIListLayout_2.SortOrder = Enum.SortOrder.LayoutOrder
		UIListLayout_2.VerticalAlignment = Enum.VerticalAlignment.Center
		UIListLayout_2.Padding = UDim.new(0, 4)

		UIPadding_3.Parent = T_1
		UIPadding_3.PaddingLeft = UDim.new(0, 12)
		UIPadding_3.PaddingRight = UDim.new(0, 80)

		TextLabel_2.Name = 'Title'
		TextLabel_2.Parent = T_1
		TextLabel_2.BackgroundTransparency = 1
		TextLabel_2.BorderSizePixel = 0
		TextLabel_2.Size = UDim2.new(1, 0, 0, 20)
		TextLabel_2.Font = Enum.Font.BuilderSansBold
		TextLabel_2.RichText = true
		TextLabel_2.Text = tostring(t)
		TextLabel_2.TextColor3 = curColors['Text & Icon']
		TextLabel_2.TextSize = 16
		TextLabel_2.TextWrapped = true
		TextLabel_2.TextXAlignment = Enum.TextXAlignment.Left
		TextLabel_2.AutomaticSize = Enum.AutomaticSize.Y

		TextLabel_1.Name = 'Desc'
		TextLabel_1.Parent = (ty == "Label") and Background or T_1
		TextLabel_1.BackgroundTransparency = 1
		TextLabel_1.BorderSizePixel = 0
		TextLabel_1.LayoutOrder = 1
		TextLabel_1.Font = (ty == "Label") and Enum.Font.BuilderSansBold or Enum.Font.BuilderSansMedium
		TextLabel_1.RichText = false
		TextLabel_1.Text = tostring(d)
		TextLabel_1.TextColor3 = (ty == "Label") and curColors['Main'] or curColors['SubText']
		TextLabel_1.TextSize = (ty == "Label") and 18 or 14
		TextLabel_1.TextWrapped = true
		TextLabel_1.TextXAlignment = (ty == "Label") and Enum.TextXAlignment.Right or Enum.TextXAlignment.Left
		TextLabel_1.Visible = (d and d ~= "")

		if ty == "Label" then
			TextLabel_1.AnchorPoint = Vector2.new(1, 0.5)
			TextLabel_1.Position = UDim2.new(1, -16, 0.5, 0)
			TextLabel_1.Size = UDim2.new(0.55, -20, 0, 20)
			TextLabel_1.TextWrapped = true
			UIPadding_3.PaddingRight = UDim.new(0.58, 0)
		else
			TextLabel_1.Size = UDim2.new(1, 0, 0, 18)
			TextLabel_1.AutomaticSize = Enum.AutomaticSize.Y
		end

		local cardObj = {
			Background = Background,
			UIStroke = UIStroke_1,
			Title = TextLabel_2,
			Desc = TextLabel_1,
		}
		table.insert(currentWindowTheme.cards, cardObj)

		if i and i ~= "" then
			UIPadding_3.PaddingLeft = UDim.new(0, 42)
			local Icon_1 = Instance.new("ImageLabel")
			Icon_1.Name = "Icon"
			Icon_1.Parent = Background
			Icon_1.AnchorPoint = Vector2.new(0, 0.5)
			Icon_1.BackgroundTransparency = 1
			Icon_1.Position = UDim2.new(0, 12, 0.5, 0)
			Icon_1.Size = UDim2.new(0, 20, 0, 20)
			local iconData = gl(i)
			Icon_1.Image = iconData.Image
			Icon_1.ImageRectSize = iconData.ImageRectSize
			Icon_1.ImageRectOffset = iconData.ImageRectPosition
			Icon_1.ImageColor3 = curColors['Main']
			cardObj.Icon = Icon_1
		end

		local function updateSize()
			task.defer(function()
				if not Background or not Background.Parent then return end
				local titleHeight = UIListLayout_2.AbsoluteContentSize.Y + 22
				local valueHeight = 0
				if ty == "Label" and TextLabel_1.Visible and TextLabel_1.Text ~= "" then
					local availWidth = math.max(20, Background.AbsoluteSize.X * 0.55 - 20)
					local textHeight = game:GetService("TextService"):GetTextSize(
						TextLabel_1.Text,
						TextLabel_1.TextSize,
						TextLabel_1.Font,
						Vector2.new(availWidth, 10000)
					).Y
					TextLabel_1.Size = UDim2.new(0.55, -20, 0, textHeight)
					valueHeight = textHeight + 22
				end
				local newSize = math.max(48, titleHeight, valueHeight)
				if Background.Size.Y.Offset ~= newSize then
					Background.Size = UDim2.new(1, 0, 0, newSize)
				end
			end)
		end

		task.delay(0.05, updateSize)
		UIListLayout_2:GetPropertyChangedSignal("AbsoluteContentSize"):Connect(updateSize)
		if ty == "Label" then
			Background:GetPropertyChangedSignal("AbsoluteSize"):Connect(updateSize)
			TextLabel_1:GetPropertyChangedSignal("Text"):Connect(updateSize)
			TextLabel_1:GetPropertyChangedSignal("Visible"):Connect(updateSize)
		end

		Background.MouseEnter:Connect(function()
			local c = currentWindowTheme.colors or curColors
			tw({v = Background, t = 0.15, g = {BackgroundColor3 = c['Card']:Lerp(c['Main'], 0.08)}}):Play()
			tw({v = UIStroke_1, t = 0.15, g = {Color = c['CardBorder']:Lerp(c['Main'], 0.4)}}):Play()
		end)
		Background.MouseLeave:Connect(function()
			local c = currentWindowTheme.colors or curColors
			tw({v = Background, t = 0.2, g = {BackgroundColor3 = c['Card']}}):Play()
			tw({v = UIStroke_1, t = 0.2, g = {Color = c['CardBorder']}}):Play()
		end)

		local f = {}
		function f:SetTextTransparencyTitle(vs)
			tw({v = TextLabel_2, t = 0.15, g = {TextTransparency = vs}}):Play()
			local icon = Background:FindFirstChild("Icon")
			if icon then tw({v = icon, t = 0.15, g = {ImageTransparency = vs}}):Play() end
		end
		function f:SetSizeT(vs) UIPadding_3.PaddingRight = UDim.new(0, vs) end
		function f:SetTitle(vs)
			TextLabel_2.Text = tostring(vs)
			updateSize()
		end
		function f:SetDesc(vs)
			TextLabel_1.Text = tostring(vs)
			TextLabel_1.Visible = (vs and vs ~= "")
			updateSize()
		end
		return Background, f
	end

	function addDropdownSelect(p, p2, Multi, Callback, Value, List)
		local F = Instance.new("Frame")
		local UIListLayout_1 = Instance.new("UIListLayout")
		local UIPadding_1 = Instance.new("UIPadding")
		local DropdownValue = Instance.new("Frame")
		local UICorner_1 = Instance.new("UICorner")
		local UIStroke_1 = Instance.new("UIStroke")
		local TextLabelValue_1 = Instance.new("TextLabel")
		local UIPadding_2 = Instance.new("UIPadding")
		local ImageLabel_1 = Instance.new("ImageLabel")

		F.Name = "F"
		F.Parent = p
		F.AnchorPoint = Vector2.new(1, 0.5)
		F.BackgroundTransparency = 1
		F.BorderSizePixel = 0
		F.Position = UDim2.new(1, 0, 0.5, 0)
		F.Size = UDim2.new(0, 140, 0.85, 0)

		UIListLayout_1.Parent = F
		UIListLayout_1.HorizontalAlignment = Enum.HorizontalAlignment.Right
		UIListLayout_1.SortOrder = Enum.SortOrder.LayoutOrder
		UIListLayout_1.VerticalAlignment = Enum.VerticalAlignment.Center

		UIPadding_1.Parent = F
		UIPadding_1.PaddingRight = UDim.new(0, 12)

		local curColors = currentWindowTheme.colors
		DropdownValue.Parent = F
		DropdownValue.BackgroundColor3 = curColors['Function']['Dropdown']['Value Background']
		DropdownValue.BorderSizePixel = 0
		DropdownValue.Size = UDim2.new(0, 120, 0, 22)

		UICorner_1.CornerRadius = UDim.new(0, 4)
		UICorner_1.Parent = DropdownValue

		UIStroke_1.Color = curColors['Function']['Dropdown']['Value Stroke']
		UIStroke_1.Thickness = 1
		UIStroke_1.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
		UIStroke_1.Parent = DropdownValue

		TextLabelValue_1.Parent = DropdownValue
		TextLabelValue_1.BackgroundTransparency = 1
		TextLabelValue_1.BorderSizePixel = 0
		TextLabelValue_1.Size = UDim2.new(0.8, 0, 1, 0)
		TextLabelValue_1.Font = Enum.Font.BuilderSansMedium
		TextLabelValue_1.Text = "--"
		TextLabelValue_1.TextColor3 = curColors['Text & Icon']
		TextLabelValue_1.TextSize = 13
		TextLabelValue_1.TextXAlignment = Enum.TextXAlignment.Left
		TextLabelValue_1.TextTruncate = Enum.TextTruncate.AtEnd

		UIPadding_2.Parent = DropdownValue
		UIPadding_2.PaddingLeft = UDim.new(0, 8)
		UIPadding_2.PaddingRight = UDim.new(0, 5)

		ImageLabel_1.Parent = DropdownValue
		ImageLabel_1.AnchorPoint = Vector2.new(1, 0.5)
		ImageLabel_1.BackgroundTransparency = 1
		ImageLabel_1.BorderSizePixel = 0
		ImageLabel_1.Position = UDim2.new(1, 0, 0.5, 0)
		ImageLabel_1.Size = UDim2.new(0, 16, 0, 16)
		ImageLabel_1.Image = "rbxassetid://10709791437"
		ImageLabel_1.ImageColor3 = curColors['SubText']

		local DropdownSelect = Instance.new("Frame")
		DropdownSelect.Name = "DropdownSelect"
		local UICorner_2 = Instance.new("UICorner")
		local UIStrokeDropdown_1 = Instance.new("UIStroke")
		local UIPadding_3 = Instance.new("UIPadding")
		local Search_1 = Instance.new("Frame")
		local UICorner_3 = Instance.new("UICorner")
		local UIStrokeSearch = Instance.new("UIStroke")
		local TextBox_1 = Instance.new("TextBox")
		local ScrollingFrame_1 = Instance.new("ScrollingFrame")
		local UIListLayout_SF = Instance.new("UIListLayout")
		local UIPadding_SF = Instance.new("UIPadding")

		DropdownSelect.Parent = ScreenGui
		DropdownSelect.BackgroundColor3 = curColors['Card']
		DropdownSelect.BorderSizePixel = 0
		DropdownSelect.Size = UDim2.new(0, 150, 0, 0)
		DropdownSelect.ClipsDescendants = true
		DropdownSelect.Visible = false
		DropdownSelect.ZIndex = 60

		UICorner_2.CornerRadius = UDim.new(0, 6)
		UICorner_2.Parent = DropdownSelect

		UIStrokeDropdown_1.Color = Color3.fromRGB(70, 70, 80)
		UIStrokeDropdown_1.Thickness = 1
		UIStrokeDropdown_1.Parent = DropdownSelect

		UIPadding_3.Parent = DropdownSelect
		UIPadding_3.PaddingBottom = UDim.new(0, 6)
		UIPadding_3.PaddingLeft = UDim.new(0, 6)
		UIPadding_3.PaddingRight = UDim.new(0, 6)
		UIPadding_3.PaddingTop = UDim.new(0, 6)

		Search_1.Name = "Search"
		Search_1.Parent = DropdownSelect
		Search_1.BackgroundColor3 = curColors['Function']['Dropdown']['Dropdown Select']['Search']
		Search_1.BorderSizePixel = 0
		Search_1.Size = UDim2.new(1, 0, 0, 22)

		UICorner_3.CornerRadius = UDim.new(0, 4)
		UICorner_3.Parent = Search_1

		UIStrokeSearch.Color = curColors['CardBorder']
		UIStrokeSearch.Thickness = 1
		UIStrokeSearch.Parent = Search_1

		TextBox_1.Parent = Search_1
		TextBox_1.BackgroundTransparency = 1
		TextBox_1.BorderSizePixel = 0
		TextBox_1.Size = UDim2.new(1, -8, 1, 0)
		TextBox_1.Position = UDim2.new(0, 6, 0, 0)
		TextBox_1.Font = Enum.Font.BuilderSansMedium
		TextBox_1.PlaceholderColor3 = curColors['SubText']
		TextBox_1.PlaceholderText = "Search..."
		TextBox_1.Text = ""
		TextBox_1.TextColor3 = curColors['Text & Icon']
		TextBox_1.TextSize = 12
		TextBox_1.TextXAlignment = Enum.TextXAlignment.Left

		table.insert(currentWindowTheme.controls, {
			type = "dropdown",
			ValueBox = DropdownValue,
			Stroke = UIStroke_1,
			Text = TextLabelValue_1,
			Arrow = ImageLabel_1,
			Popup = DropdownSelect,
			Search = Search_1,
			SearchStroke = UIStrokeSearch,
			SearchInput = TextBox_1,
		})

		ScrollingFrame_1.Name = "ScrollingFrame"
		ScrollingFrame_1.Parent = DropdownSelect
		ScrollingFrame_1.Active = true
		ScrollingFrame_1.BackgroundTransparency = 1
		ScrollingFrame_1.BorderSizePixel = 0
		ScrollingFrame_1.Position = UDim2.new(0, 0, 0, 28)
		ScrollingFrame_1.Size = UDim2.new(1, 0, 1, -28)
		ScrollingFrame_1.ClipsDescendants = true
		ScrollingFrame_1.ScrollBarThickness = 2
		ScrollingFrame_1.ScrollBarImageColor3 = Color3.fromRGB(200, 200, 200)

		UIListLayout_SF.Parent = ScrollingFrame_1
		UIListLayout_SF.SortOrder = Enum.SortOrder.LayoutOrder
		UIListLayout_SF.Padding = UDim.new(0, 3)

		UIPadding_SF.Parent = ScrollingFrame_1
		UIPadding_SF.PaddingRight = UDim.new(0, 4)

		local Click = click(p2)
		local isopen = false

		local function updateDropdownSize()
			if not isopen then return end
			local contentHeight = math.clamp(UIListLayout_SF.AbsoluteContentSize.Y + 40, 50, 180)
			tw({v = DropdownSelect, t = 0.15, g = {Size = UDim2.new(0, 150, 0, contentHeight)}}):Play()
		end

		TextBox_1.Changed:Connect(function()
			local search = string.lower(TextBox_1.Text)
			local hasSearch = (search ~= "")
			for _, v in ipairs(ScrollingFrame_1:GetChildren()) do
				if v:IsA("Frame") then
					local tl = v:FindFirstChild("TextLabel")
					if tl then
						v.Visible = not hasSearch or (string.find(string.lower(tl.Text), search, 1, true) ~= nil)
					end
				end
			end
			updateDropdownSize()
		end)

		local function open()
			if isopen then return end
			DropdownSelect.Visible = true
			local targetX = DropdownValue.AbsolutePosition.X - DropdownSelect.Parent.AbsolutePosition.X + DropdownValue.Size.X.Offset - 150
			local targetY = DropdownValue.AbsolutePosition.Y - DropdownSelect.Parent.AbsolutePosition.Y + DropdownValue.Size.Y.Offset + 4
			DropdownSelect.Position = UDim2.new(0, targetX, 0, targetY)

			local contentHeight = math.clamp(UIListLayout_SF.AbsoluteContentSize.Y + 40, 50, 180)
			tw({v = DropdownSelect, t = 0.15, g = {Size = UDim2.new(0, 150, 0, contentHeight)}}):Play()
			tw({v = ImageLabel_1, t = 0.15, g = {Rotation = 90}}):Play()
			isopen = true
		end

		local function close()
			if not isopen then return end
			tw({v = ImageLabel_1, t = 0.15, g = {Rotation = 0}}):Play()
			local gf = tw({v = DropdownSelect, t = 0.15, g = {Size = UDim2.new(0, 150, 0, 0)}})
			gf:Play()
			gf.Completed:Connect(function()
				DropdownSelect.Visible = false
				isopen = false
			end)
		end

		U.InputBegan:Connect(function(A)
			if (A.UserInputType == Enum.UserInputType.MouseButton1 or A.UserInputType == Enum.UserInputType.Touch) and isopen then
				local B, C = DropdownSelect.AbsolutePosition, DropdownSelect.AbsoluteSize
				local MousePos = LocalPlayer:GetMouse()
				if MousePos.X < B.X or MousePos.X > B.X + C.X or MousePos.Y < (DropdownValue.AbsolutePosition.Y) or MousePos.Y > B.Y + C.Y then
					close()
				end
			end
		end)

		Click.MouseButton1Click:Connect(function()
			if not isopen then open() else close() end
		end)

		local itemslist = {}
		local selectedValues = {}
		local itemEntries = {}

		function itemslist:Clear()
			for _, v in ipairs(ScrollingFrame_1:GetChildren()) do
				if v:IsA("Frame") then v:Destroy() end
			end
			table.clear(itemEntries)
			table.clear(selectedValues)
			TextLabelValue_1.Text = "--"
			Value = nil
		end

		function itemslist:Add(text)
			local Item_1 = Instance.new("Frame")
			local TextLabel_1 = Instance.new("TextLabel")
			local ItemCorner = Instance.new("UICorner")

			Item_1.Name = "Item"
			Item_1.Parent = ScrollingFrame_1
			Item_1.BackgroundColor3 = curColors['Function']['Dropdown']['Dropdown Select']['Item Background']
			Item_1.BorderSizePixel = 0
			Item_1.Size = UDim2.new(1, 0, 0, 22)

			ItemCorner.CornerRadius = UDim.new(0, 4)
			ItemCorner.Parent = Item_1

			TextLabel_1.Parent = Item_1
			TextLabel_1.BackgroundTransparency = 1
			TextLabel_1.BorderSizePixel = 0
			TextLabel_1.Size = UDim2.new(1, -8, 1, 0)
			TextLabel_1.Position = UDim2.new(0, 6, 0, 0)
			TextLabel_1.Font = Enum.Font.BuilderSansMedium
			TextLabel_1.Text = tostring(text)
			TextLabel_1.TextColor3 = curColors['SubText']
			TextLabel_1.TextSize = 12
			TextLabel_1.TextXAlignment = Enum.TextXAlignment.Left

			local ClickItem = click(Item_1)

			local function unselect()
				local c = currentWindowTheme.colors or curColors
				local itemBg = (c['Function'] and c['Function']['Dropdown'] and c['Function']['Dropdown']['Dropdown Select'] and c['Function']['Dropdown']['Dropdown Select']['Item Background']) or c['Card']
				tw({v = TextLabel_1, t = 0.12, g = {TextColor3 = c['SubText']}}):Play()
				tw({v = Item_1, t = 0.12, g = {BackgroundColor3 = itemBg}}):Play()
			end

			local function hasselect()
				local c = currentWindowTheme.colors or curColors
				tw({v = TextLabel_1, t = 0.12, g = {TextColor3 = c['Background']}}):Play()
				tw({v = Item_1, t = 0.12, g = {BackgroundColor3 = c['Main']}}):Play()
			end

			local strText = tostring(text)
			itemEntries[strText] = { select = hasselect, unselect = unselect }

			ClickItem.MouseEnter:Connect(function()
				if (Multi and not selectedValues[strText]) or (not Multi and Value ~= strText) then
					local c = currentWindowTheme.colors or curColors
					local itemBg = (c['Function'] and c['Function']['Dropdown'] and c['Function']['Dropdown']['Dropdown Select'] and c['Function']['Dropdown']['Dropdown Select']['Item Background']) or c['Card']
					tw({v = Item_1, t = 0.12, g = {BackgroundColor3 = itemBg:Lerp(c['Main'], 0.1)}}):Play()
					tw({v = TextLabel_1, t = 0.12, g = {TextColor3 = c['Text & Icon']}}):Play()
				end
			end)

			ClickItem.MouseLeave:Connect(function()
				if (Multi and not selectedValues[strText]) or (not Multi and Value ~= strText) then
					local c = currentWindowTheme.colors or curColors
					local itemBg = (c['Function'] and c['Function']['Dropdown'] and c['Function']['Dropdown']['Dropdown Select'] and c['Function']['Dropdown']['Dropdown Select']['Item Background']) or c['Card']
					tw({v = Item_1, t = 0.15, g = {BackgroundColor3 = itemBg}}):Play()
					tw({v = TextLabel_1, t = 0.15, g = {TextColor3 = c['SubText']}}):Play()
				end
			end)

			ClickItem.MouseButton1Click:Connect(function()
				if Multi then
					if selectedValues[strText] then
						selectedValues[strText] = nil
						unselect()
					else
						selectedValues[strText] = true
						hasselect()
					end
					local selectedList = {}
					for i, _ in pairs(selectedValues) do table.insert(selectedList, i) end
					TextLabelValue_1.Text = (#selectedList > 0) and table.concat(selectedList, ", ") or "--"
					pcall(Callback, selectedList)
				else
					for itemText, entry in pairs(itemEntries) do
						if itemText == strText then
							entry.select()
						else
							entry.unselect()
						end
					end
					Value = strText
					TextLabelValue_1.Text = strText
					pcall(Callback, TextLabelValue_1.Text)
					close()
				end
			end)

			if Multi then
				if selectedValues[strText] then
					hasselect()
				else
					unselect()
				end
			else
				if strText == tostring(Value) then
					hasselect()
					TextLabelValue_1.Text = strText
				end
			end
		end

		function itemslist:SetValue(value, skipCallback)
			if Multi then
				table.clear(selectedValues)
				if type(value) == "table" then
					for k, val in pairs(value) do
						if type(k) == "number" and val ~= nil then
							selectedValues[tostring(val)] = true
						elseif type(k) == "string" and (val == true or val == 1) then
							selectedValues[k] = true
						end
					end
				elseif value ~= nil then
					selectedValues[tostring(value)] = true
				end

				for itemText, entry in pairs(itemEntries) do
					if selectedValues[itemText] then
						entry.select()
					else
						entry.unselect()
					end
				end

				local selectedList = {}
				for k, _ in pairs(selectedValues) do table.insert(selectedList, k) end
				TextLabelValue_1.Text = (#selectedList > 0) and table.concat(selectedList, ", ") or "--"
				if not skipCallback then
					pcall(Callback, selectedList)
				end
			else
				Value = (value ~= nil) and tostring(value) or nil
				for itemText, entry in pairs(itemEntries) do
					if Value ~= nil and itemText == Value then
						entry.select()
					else
						entry.unselect()
					end
				end
				TextLabelValue_1.Text = (Value ~= nil and Value ~= "") and Value or "--"
				if not skipCallback then
					pcall(Callback, Value)
				end
			end
		end

		function itemslist:GetValue()
			if Multi then
				local selectedList = {}
				for k, _ in pairs(selectedValues) do table.insert(selectedList, k) end
				return selectedList
			end
			return Value
		end

		if Multi and Value ~= nil then
			if type(Value) == "table" then
				for k, val in pairs(Value) do
					if type(k) == "number" and val ~= nil then
						selectedValues[tostring(val)] = true
					elseif type(k) == "string" and (val == true or val == 1) then
						selectedValues[k] = true
					end
				end
			else
				selectedValues[tostring(Value)] = true
			end
			local initialList = {}
			for k, _ in pairs(selectedValues) do table.insert(initialList, k) end
			TextLabelValue_1.Text = (#initialList > 0) and table.concat(initialList, ", ") or "--"
		end

		for _, v in ipairs(List) do itemslist:Add(v) end
		changecanvas(ScrollingFrame_1, UIListLayout_SF, 5)
		return itemslist
	end
end

function Library:Window(p)
	if self ~= Library and type(p) ~= "table" and type(self) == "table" and (self.Title or self.Config or self.Theme) then
		p = self
	end
	p = p or {}
	local Title = p.Title or 'Aether'
	local Desc = p.Desc or 'Studios'
	local Theme = p.Theme or 'Default'
	local Keybind = (p.Config and p.Config.Keybind) or Enum.KeyCode.LeftControl
	local defaultSize = (p.Config and p.Config.Size) or UDim2.new(0, 840, 0, 480)
	local Size = defaultSize
	local savedW, savedH = FileSystem:LoadWindowSize()
	if savedW and savedH and not (p.Config and p.Config.IgnoreSavedSize) then
		Size = UDim2.new(0, savedW, 0, savedH)
	end
	local DiscordLink = p.DiscordLink or "https://discord.gg/getaether"

	currentWindowTheme.name = (themes[Theme] and Theme) or 'Default'
	currentWindowTheme.colors = deepCopy(themes[currentWindowTheme.name])
	currentWindowTheme.cards = {}
	currentWindowTheme.sections = {}
	currentWindowTheme.controls = {}
	currentWindowTheme.tabs = {}
	currentWindowTheme.topTabs = {}
	currentWindowTheme.subTabs = {}
	currentWindowTheme.transparency = {
		Window = 0,
		Sidebar = 0,
		Section = 0,
		Card = 0,
		RealBackground = 100
	}

	local curColors = currentWindowTheme.colors

	local Shadow_1 = Instance.new("ImageLabel")
	local Background_1 = Instance.new("Frame")
	local UICorner_1 = Instance.new("UICorner")
	local UIStroke_Win = Instance.new("UIStroke")

	Shadow_1.Name = "Shadow"
	Shadow_1.Image = "rbxassetid://1316045217"
	Shadow_1.ImageColor3 = Color3.fromRGB(0, 0, 0)
	Shadow_1.ScaleType = Enum.ScaleType.Slice
	Shadow_1.SliceCenter = Rect.new(10, 10, 118, 118)
	Shadow_1.ImageTransparency = 1
	Shadow_1.Parent = ScreenGui
	Shadow_1.AnchorPoint = Vector2.new(0.5, 0.5)
	Shadow_1.BackgroundColor3 = curColors['Background']
	Shadow_1.BackgroundTransparency = 1
	Shadow_1.Position = UDim2.new(0.5, 0, 0.5, 0)
	Shadow_1.Size = Size
	Shadow_1.Visible = true
	local WindowScale = attachAutoScale(Shadow_1, defaultSize.X.Offset, defaultSize.Y.Offset)

	local UIPadding_Shadow = Instance.new("UIPadding")
	UIPadding_Shadow.Parent = Shadow_1
	UIPadding_Shadow.PaddingBottom = UDim.new(0, 8)
	UIPadding_Shadow.PaddingLeft = UDim.new(0, 8)
	UIPadding_Shadow.PaddingRight = UDim.new(0, 8)
	UIPadding_Shadow.PaddingTop = UDim.new(0, 8)

	Background_1.Name = "Background"
	Background_1.Parent = Shadow_1
	Background_1.AnchorPoint = Vector2.new(0.5, 0.5)
	Background_1.BackgroundColor3 = curColors['Background']
	Background_1.BorderSizePixel = 0
	Background_1.Position = UDim2.new(0.5, 0, 0.5, 0)
	Background_1.Size = UDim2.new(1, 0, 1, 0)
	Background_1.ClipsDescendants = true

	UICorner_1.CornerRadius = UDim.new(0, 10)
	UICorner_1.Parent = Background_1

	UIStroke_Win.Color = curColors['CardBorder']
	UIStroke_Win.Thickness = 1.2
	UIStroke_Win.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
	UIStroke_Win.Transparency = 1
	UIStroke_Win.Parent = Background_1

	local WindowBackgroundImg = Instance.new("ImageLabel")
	WindowBackgroundImg.Name = "WindowBackgroundImg"
	WindowBackgroundImg.Parent = Background_1
	WindowBackgroundImg.BackgroundTransparency = 1
	WindowBackgroundImg.Size = UDim2.new(1, 0, 1, 0)
	WindowBackgroundImg.ScaleType = Enum.ScaleType.Crop
	WindowBackgroundImg.ImageTransparency = 1
	WindowBackgroundImg.ZIndex = 1

	local WinBgCorner = Instance.new("UICorner")
	WinBgCorner.CornerRadius = UDim.new(0, 10)
	WinBgCorner.Parent = WindowBackgroundImg

	local WindowBackgroundDim = Instance.new("Frame")
	WindowBackgroundDim.Name = "WindowBackgroundDim"
	WindowBackgroundDim.Parent = Background_1
	WindowBackgroundDim.BackgroundColor3 = Color3.fromRGB(0, 0, 0)
	WindowBackgroundDim.BackgroundTransparency = 1
	WindowBackgroundDim.Size = UDim2.new(1, 0, 1, 0)
	WindowBackgroundDim.BorderSizePixel = 0
	WindowBackgroundDim.ZIndex = 2

	local WinDimCorner = Instance.new("UICorner")
	WinDimCorner.CornerRadius = UDim.new(0, 10)
	WinDimCorner.Parent = WindowBackgroundDim

	local ResizeHandle = Instance.new("ImageButton")
	local ResizeVisual = Instance.new("ImageLabel")

	ResizeHandle.Name = "ResizeHandle"
	ResizeHandle.Parent = Shadow_1
	ResizeHandle.AnchorPoint = Vector2.new(0.5, 0.5)
	ResizeHandle.Position = UDim2.new(1, 0, 1, 0)
	ResizeHandle.Size = UDim2.new(0, 32, 0, 32)
	ResizeHandle.BackgroundTransparency = 1
	ResizeHandle.BorderSizePixel = 0
	ResizeHandle.ZIndex = 80
	ResizeHandle.AutoButtonColor = false

	ResizeVisual.Name = "Visual"
	ResizeVisual.Parent = ResizeHandle
	ResizeVisual.AnchorPoint = Vector2.new(0.5, 0.5)
	ResizeVisual.Position = UDim2.new(0.5, 0, 0.5, 0)
	ResizeVisual.Size = UDim2.new(0, 16, 0, 16)
	ResizeVisual.BackgroundTransparency = 1
	ResizeVisual.BorderSizePixel = 0
	ResizeVisual.ZIndex = 81
	ResizeVisual.Rotation = 45

	local chevronIconData = gl("chevron-right")
	if chevronIconData then
		ResizeVisual.Image = chevronIconData.Image
		ResizeVisual.ImageRectSize = chevronIconData.ImageRectSize
		ResizeVisual.ImageRectOffset = chevronIconData.ImageRectPosition
	else
		ResizeVisual.Image = "rbxassetid://10709791437"
	end
	ResizeVisual.ImageColor3 = Color3.fromRGB(255, 255, 255)
	ResizeVisual.ImageTransparency = 0.4

	local Sidebar = nil
	local currentActiveWallpaper = ""
	local WallDropdown = nil
	local WallAssetInput = nil
	local WallTransSlider = nil
	local WallDimSlider = nil
	local ConfigElements = {}

	local BackgroundMusic = Instance.new("Sound")
	BackgroundMusic.Name = "AetherBackgroundMusic"
	BackgroundMusic.Parent = game:GetService("SoundService")
	BackgroundMusic.Volume = 0.5
	BackgroundMusic.Looped = true

	local ClickSound = Instance.new("Sound")
	ClickSound.Name = "AetherClickSound"
	ClickSound.Parent = game:GetService("SoundService")
	ClickSound.Volume = 0.6
	ClickSound.SoundId = "rbxassetid://6895079853"

	local currentSoundConfig = {
		BGMEnabled = false,
		BGMTrack = "None",
		BGMAsset = "",
		BGMVolume = 50,
		BGMLooped = true,
		ClickEnabled = true,
		ClickSound = "Modern Click",
		ClickAsset = "rbxassetid://6895079853",
		ClickVolume = 60
	}

	playClickSound = function()
		if currentSoundConfig and currentSoundConfig.ClickEnabled and ClickSound and ClickSound.SoundId ~= "" then
			pcall(function()
				SoundService:PlayLocalSound(ClickSound)
			end)
		end
	end

	local BGMToggle = nil
	local BGMTrackDropdown = nil
	local BGMAssetInput = nil
	local BGMVolumeSlider = nil
	local BGMLoopToggle = nil

	local function applySoundConfig(cfg)
		if not cfg or type(cfg) ~= "table" then return end
		if cfg.BGMEnabled ~= nil then
			currentSoundConfig.BGMEnabled = cfg.BGMEnabled
			if BGMToggle then BGMToggle:SetValue(cfg.BGMEnabled, true) end
		end

		local curAsset = cfg.BGMAsset or ""
		local curTrack = cfg.BGMTrack or "None"
		local isAssetId = tostring(curAsset):match("^%d+$") or tostring(curAsset):match("^rbxassetid://") or tostring(curAsset):match("^rbxasset://") or tostring(curAsset):match("^https?://")

		if curAsset ~= "" and isAssetId then
			if not curAsset:find("rbxassetid://") and tonumber(curAsset) then
				curAsset = "rbxassetid://" .. curAsset
			end
			currentSoundConfig.BGMAsset = curAsset
			currentSoundConfig.BGMTrack = "None"
			if BGMTrackDropdown then BGMTrackDropdown:SetValue("None", true) end
			if BGMAssetInput then BGMAssetInput:SetValue(curAsset, true) end
		elseif curTrack and curTrack ~= "" and curTrack ~= "None" then
			currentSoundConfig.BGMTrack = curTrack
			currentSoundConfig.BGMAsset = ""
			if BGMTrackDropdown then BGMTrackDropdown:SetValue(curTrack, true) end
			if BGMAssetInput then BGMAssetInput:SetValue("", true) end
		else
			currentSoundConfig.BGMTrack = curTrack
			currentSoundConfig.BGMAsset = curAsset
			if BGMTrackDropdown then BGMTrackDropdown:SetValue(curTrack, true) end
			if BGMAssetInput then BGMAssetInput:SetValue(curAsset, true) end
		end

		if cfg.BGMVolume ~= nil then
			currentSoundConfig.BGMVolume = math.clamp(tonumber(cfg.BGMVolume) or 50, 0, 100)
			BackgroundMusic.Volume = currentSoundConfig.BGMVolume / 100
			if BGMVolumeSlider then BGMVolumeSlider:SetValue(currentSoundConfig.BGMVolume, true) end
		end
		if cfg.BGMLooped ~= nil then
			currentSoundConfig.BGMLooped = cfg.BGMLooped
			BackgroundMusic.Looped = cfg.BGMLooped
			if BGMLoopToggle then BGMLoopToggle:SetValue(cfg.BGMLooped, true) end
		end

		local bgmAssetToPlay = ""
		if currentSoundConfig.BGMAsset and currentSoundConfig.BGMAsset ~= "" then
			bgmAssetToPlay = FileSystem:GetAudioAsset(currentSoundConfig.BGMAsset)
		elseif currentSoundConfig.BGMTrack and currentSoundConfig.BGMTrack ~= "None" then
			local presetMap = {
				["Lofi Chill"] = "rbxassetid://9043887091",
				["Ambient Dream"] = "rbxassetid://1843404009"
			}
			bgmAssetToPlay = presetMap[currentSoundConfig.BGMTrack] or FileSystem:GetAudioAsset(currentSoundConfig.BGMTrack)
		end

		if bgmAssetToPlay and bgmAssetToPlay ~= "" then
			if BackgroundMusic.SoundId ~= bgmAssetToPlay then
				BackgroundMusic.SoundId = bgmAssetToPlay
			end
			if currentSoundConfig.BGMEnabled then
				pcall(function() BackgroundMusic:Play() end)
			else
				pcall(function() BackgroundMusic:Stop() end)
			end
		else
			pcall(function() BackgroundMusic:Stop() end)
		end
	end

	local currentCursorConfig = {
		Enabled = false,
		Preset = "Default (Roblox)",
		CustomAsset = ""
	}

	local CursorToggle = nil
	local CursorPresetDropdown = nil
	local CursorAssetInput = nil

	local isUpdatingCursorUI = false
	local function applyCursorConfig(cfg)
		if not cfg or type(cfg) ~= "table" then return end
		if isUpdatingCursorUI then return end
		isUpdatingCursorUI = true

		if cfg.Enabled ~= nil then
			currentCursorConfig.Enabled = cfg.Enabled
			if CursorToggle then CursorToggle:SetValue(cfg.Enabled, true) end
		end

		local curAsset = cfg.CustomAsset or ""
		local curPreset = cfg.Preset or "Default (Roblox)"
		local isAssetId = tostring(curAsset):match("^%d+$") or tostring(curAsset):match("^rbxassetid://") or tostring(curAsset):match("^rbxasset://") or tostring(curAsset):match("^https?://")

		if curAsset ~= "" and isAssetId then
			if not curAsset:find("rbxassetid://") and tonumber(curAsset) then
				curAsset = "rbxassetid://" .. curAsset
			end
			currentCursorConfig.CustomAsset = curAsset
			currentCursorConfig.Preset = "None"
			if CursorPresetDropdown then CursorPresetDropdown:SetValue("None", true) end
			if CursorAssetInput then CursorAssetInput:SetValue(curAsset, true) end
		elseif curPreset and curPreset ~= "" and curPreset ~= "None" and curPreset ~= "Default (Roblox)" then
			currentCursorConfig.Preset = curPreset
			currentCursorConfig.CustomAsset = ""
			if CursorPresetDropdown then CursorPresetDropdown:SetValue(curPreset, true) end
			if CursorAssetInput then CursorAssetInput:SetValue("", true) end
		else
			currentCursorConfig.Preset = curPreset
			currentCursorConfig.CustomAsset = curAsset
			if CursorPresetDropdown then CursorPresetDropdown:SetValue(curPreset, true) end
			if CursorAssetInput then CursorAssetInput:SetValue(curAsset, true) end
		end

		local mouse = LocalPlayer and LocalPlayer:GetMouse()
		if mouse then
			if not currentCursorConfig.Enabled then
				pcall(function() mouse.Icon = "" end)
			else
				local assetToApply = ""
				if currentCursorConfig.CustomAsset and currentCursorConfig.CustomAsset ~= "" then
					assetToApply = FileSystem:GetCursorAsset(currentCursorConfig.CustomAsset)
				elseif currentCursorConfig.Preset and currentCursorConfig.Preset ~= "Default (Roblox)" and currentCursorConfig.Preset ~= "None" then
					local presetMap = {
						["Crosshair (Clean)"] = "rbxasset://textures/Cursors/CrossKeyboard.png",
						["Arrow (Far)"] = "rbxasset://textures/ArrowFarCursor.png",
						["Pointer (Sleek)"] = "rbxasset://textures/ArrowCursor.png",
					}
					assetToApply = presetMap[currentCursorConfig.Preset] or FileSystem:GetCursorAsset(currentCursorConfig.Preset)
				end

				pcall(function()
					mouse.Icon = assetToApply or ""
				end)
			end
		end

		isUpdatingCursorUI = false
	end

	local TransPresetDropdown = nil
	local SidebarTransSlider = nil
	local SectionTransSlider = nil
	local CardTransSlider = nil
	local WindowTransSlider = nil
	local skipPresetReset = false

	local function getPresetNameForValues(w, s, sec, c)
		if w == 0 and s == 0 and sec == 0 and c == 0 then
			return "Solid (Default)"
		elseif w == 15 and s == 20 and sec == 25 and c == 20 then
			return "Frosted Glass"
		elseif w == 35 and s == 40 and sec == 45 and c == 35 then
			return "Translucent"
		elseif w == 60 and s == 65 and sec == 70 and c == 55 then
			return "Ultra Clear"
		else
			return "Custom"
		end
	end

	local function setSidebarTransparency(val)
		val = math.clamp(tonumber(val) or 0, 0, 100)
		currentWindowTheme.transparency.Sidebar = val
		if Sidebar and Sidebar.Parent then
			Sidebar.BackgroundTransparency = val / 100
		end
	end

	local function setSectionTransparency(val)
		val = math.clamp(tonumber(val) or 0, 0, 100)
		currentWindowTheme.transparency.Section = val
		for _, sec in ipairs(currentWindowTheme.sections or {}) do
			if sec.Frame and sec.Frame.Parent then
				sec.Frame.BackgroundTransparency = val / 100
			end
		end
	end

	local function setCardTransparency(val)
		val = math.clamp(tonumber(val) or 0, 0, 100)
		currentWindowTheme.transparency.Card = val
		for _, card in ipairs(currentWindowTheme.cards or {}) do
			if card.Background and card.Background.Parent then
				card.Background.BackgroundTransparency = val / 100
			end
		end
	end

	local function setWindowTransparency(val)
		val = math.clamp(tonumber(val) or 0, 0, 100)
		currentWindowTheme.transparency.Window = val
		if Background_1 and Background_1.Parent then
			Background_1.BackgroundTransparency = val / 100
		end
	end

	local function applyAllTransparencies(windowT, sidebarT, sectionT, cardT)
		local w, s, sec, c
		if type(windowT) == "table" then
			w = windowT.Window or 0
			s = windowT.Sidebar or 0
			sec = windowT.Section or 0
			c = windowT.Card or 0
		else
			w = windowT or 0
			s = sidebarT or 0
			sec = sectionT or 0
			c = cardT or 0
		end

		skipPresetReset = true
		setWindowTransparency(w)
		setSidebarTransparency(s)
		setSectionTransparency(sec)
		setCardTransparency(c)

		if WindowTransSlider then WindowTransSlider:SetValue(w, true) end
		if SidebarTransSlider then SidebarTransSlider:SetValue(s, true) end
		if SectionTransSlider then SectionTransSlider:SetValue(sec, true) end
		if CardTransSlider then CardTransSlider:SetValue(c, true) end

		if TransPresetDropdown then
			local matchedPreset = getPresetNameForValues(w, s, sec, c)
			TransPresetDropdown:SetValue(matchedPreset, true)
		end
		skipPresetReset = false
	end

	local ReopenBtn = Instance.new("CanvasGroup")
	local ReopenCorner = Instance.new("UICorner")
	local ReopenStroke = Instance.new("UIStroke")
	local ReopenContent = Instance.new("Frame")
	local ReopenLayout = Instance.new("UIListLayout")
	local ReopenPadding = Instance.new("UIPadding")

	local ReopenOnlineDot = Instance.new("Frame")
	local ReopenOnlineDotCorner = Instance.new("UICorner")

	local ReopenPlayerLabel = Instance.new("TextLabel")
	local ReopenDiv1 = Instance.new("Frame")
	local ReopenStats = Instance.new("TextLabel")
	local ReopenDiv2 = Instance.new("Frame")

	local ReopenKeyBadge = Instance.new("Frame")
	local ReopenKeyCorner = Instance.new("UICorner")
	local ReopenKeyStroke = Instance.new("UIStroke")
	local ReopenKeybind = Instance.new("TextLabel")

	local ReopenClick = Instance.new("TextButton")

	ReopenBtn.Name = "ReopenButton"
	ReopenBtn.Parent = ScreenGui
	ReopenBtn.AnchorPoint = Vector2.new(0.5, 0)
	ReopenBtn.Position = UDim2.new(0.5, 0, 0, 14)
	ReopenBtn.Size = UDim2.new(0, 0, 0, 32)
	ReopenBtn.AutomaticSize = Enum.AutomaticSize.X
	ReopenBtn.BackgroundColor3 = curColors['Card']
	ReopenBtn.BorderSizePixel = 0
	ReopenBtn.GroupTransparency = 1
	ReopenBtn.Visible = false
	ReopenBtn.ZIndex = 100
	ReopenBtn.ClipsDescendants = true

	ReopenCorner.CornerRadius = UDim.new(0, 8)
	ReopenCorner.Parent = ReopenBtn

	ReopenStroke.Color = curColors['CardBorder']
	ReopenStroke.Thickness = 1.2
	ReopenStroke.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
	ReopenStroke.Transparency = 1
	ReopenStroke.Parent = ReopenBtn

	ReopenContent.Name = "Content"
	ReopenContent.Parent = ReopenBtn
	ReopenContent.BackgroundTransparency = 1
	ReopenContent.Size = UDim2.new(0, 0, 1, 0)
	ReopenContent.AutomaticSize = Enum.AutomaticSize.X

	ReopenLayout.Parent = ReopenContent
	ReopenLayout.FillDirection = Enum.FillDirection.Horizontal
	ReopenLayout.HorizontalAlignment = Enum.HorizontalAlignment.Center
	ReopenLayout.VerticalAlignment = Enum.VerticalAlignment.Center
	ReopenLayout.SortOrder = Enum.SortOrder.LayoutOrder
	ReopenLayout.Padding = UDim.new(0, 9)

	ReopenPadding.Parent = ReopenContent
	ReopenPadding.PaddingLeft = UDim.new(0, 12)
	ReopenPadding.PaddingRight = UDim.new(0, 12)

	ReopenOnlineDot.Name = "StatusDot"
	ReopenOnlineDot.Parent = ReopenContent
	ReopenOnlineDot.BackgroundColor3 = Color3.fromRGB(34, 197, 94)
	ReopenOnlineDot.BorderSizePixel = 0
	ReopenOnlineDot.Size = UDim2.new(0, 7, 0, 7)
	ReopenOnlineDot.LayoutOrder = 1

	ReopenOnlineDotCorner.CornerRadius = UDim.new(1, 0)
	ReopenOnlineDotCorner.Parent = ReopenOnlineDot

	local initialPlayerName = (LocalPlayer and (LocalPlayer.DisplayName or LocalPlayer.Name)) or "Player"
	ReopenPlayerLabel.Name = "PlayerName"
	ReopenPlayerLabel.Parent = ReopenContent
	ReopenPlayerLabel.BackgroundTransparency = 1
	ReopenPlayerLabel.Size = UDim2.new(0, 0, 0, 18)
	ReopenPlayerLabel.AutomaticSize = Enum.AutomaticSize.X
	ReopenPlayerLabel.Font = Enum.Font.BuilderSansBold
	ReopenPlayerLabel.Text = initialPlayerName
	ReopenPlayerLabel.TextColor3 = curColors['Text & Icon']
	ReopenPlayerLabel.TextSize = 13
	ReopenPlayerLabel.TextXAlignment = Enum.TextXAlignment.Left
	ReopenPlayerLabel.TextTruncate = Enum.TextTruncate.None
	ReopenPlayerLabel.LayoutOrder = 2

	ReopenDiv1.Name = "Div1"
	ReopenDiv1.Parent = ReopenContent
	ReopenDiv1.BackgroundColor3 = curColors['CardBorder']
	ReopenDiv1.BorderSizePixel = 0
	ReopenDiv1.Size = UDim2.new(0, 1, 0, 13)
	ReopenDiv1.LayoutOrder = 3

	ReopenStats.Name = "Stats"
	ReopenStats.Parent = ReopenContent
	ReopenStats.BackgroundTransparency = 1
	ReopenStats.Size = UDim2.new(0, 0, 0, 18)
	ReopenStats.AutomaticSize = Enum.AutomaticSize.X
	ReopenStats.Font = Enum.Font.BuilderSansMedium
	ReopenStats.RichText = true
	ReopenStats.Text = '<font color="rgb(34,197,94)"><b>60</b></font> <font color="rgb(150,150,165)">FPS</font> <font color="rgb(70,70,85)">•</font> <font color="rgb(255,255,255)"><b>0</b></font> <font color="rgb(150,150,165)">ms</font>'
	ReopenStats.TextSize = 12
	ReopenStats.TextXAlignment = Enum.TextXAlignment.Center
	ReopenStats.LayoutOrder = 4

	ReopenDiv2.Name = "Div2"
	ReopenDiv2.Parent = ReopenContent
	ReopenDiv2.BackgroundColor3 = curColors['CardBorder']
	ReopenDiv2.BorderSizePixel = 0
	ReopenDiv2.Size = UDim2.new(0, 1, 0, 13)
	ReopenDiv2.LayoutOrder = 5

	local initKeyName = (Keybind and Keybind.Name) or "LeftCtrl"
	if initKeyName == "LeftControl" then initKeyName = "LeftCtrl" end
	if initKeyName == "RightControl" then initKeyName = "RightCtrl" end

	ReopenKeyBadge.Name = "KeyBadge"
	ReopenKeyBadge.Parent = ReopenContent
	ReopenKeyBadge.BackgroundColor3 = curColors['Background']:Lerp(curColors['Card'], 0.5)
	ReopenKeyBadge.BorderSizePixel = 0
	ReopenKeyBadge.Size = UDim2.new(0, 0, 0, 20)
	ReopenKeyBadge.AutomaticSize = Enum.AutomaticSize.X
	ReopenKeyBadge.LayoutOrder = 6

	ReopenKeyCorner.CornerRadius = UDim.new(0, 5)
	ReopenKeyCorner.Parent = ReopenKeyBadge

	ReopenKeyStroke.Color = curColors['CardBorder']
	ReopenKeyStroke.Thickness = 1
	ReopenKeyStroke.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
	ReopenKeyStroke.Parent = ReopenKeyBadge

	local ReopenKeyPadding = Instance.new("UIPadding")
	ReopenKeyPadding.Parent = ReopenKeyBadge
	ReopenKeyPadding.PaddingLeft = UDim.new(0, 6)
	ReopenKeyPadding.PaddingRight = UDim.new(0, 6)

	ReopenKeybind.Name = "KeyHint"
	ReopenKeybind.Parent = ReopenKeyBadge
	ReopenKeybind.BackgroundTransparency = 1
	ReopenKeybind.Size = UDim2.new(0, 0, 1, 0)
	ReopenKeybind.AutomaticSize = Enum.AutomaticSize.X
	ReopenKeybind.Font = Enum.Font.BuilderSansBold
	ReopenKeybind.Text = "[" .. initKeyName .. "]"
	ReopenKeybind.TextColor3 = curColors['Main']
	ReopenKeybind.TextSize = 11
	ReopenKeybind.TextXAlignment = Enum.TextXAlignment.Center

	ReopenClick.Name = "Click"
	ReopenClick.Parent = ReopenBtn
	ReopenClick.BackgroundTransparency = 1
	ReopenClick.Size = UDim2.new(1, 0, 1, 0)
	ReopenClick.Text = ""
	ReopenClick.ZIndex = 25

	local isHidden = false
	local function setUIVisibility(visible)
		isHidden = not visible
		if isHidden then
			tw({v = UIStroke_Win, t = 0.12, g = {Transparency = 1}}):Play()
			tw({v = Shadow_1, t = 0.12, g = {ImageTransparency = 1}}):Play()
			if ResizeVisual then
				tw({v = ResizeVisual, t = 0.12, g = {ImageTransparency = 1}}):Play()
			end
			tw({v = Background_1, t = 0.12, s = Enum.EasingStyle.Quad, d = "In", g = {Size = UDim2.new(1, -12, 1, -12)}}):Play()
			task.delay(0.12, function()
				if isHidden then
					Shadow_1.Visible = false
					Background_1.Visible = false
					Background_1.Size = UDim2.new(1, 0, 1, 0)
				end
			end)
			ReopenBtn.Visible = true
			ReopenBtn.GroupTransparency = 1
			ReopenStroke.Transparency = 1
			tw({v = ReopenBtn, t = 0.22, s = Enum.EasingStyle.Back, d = "Out", g = {GroupTransparency = 0}}):Play()
			tw({v = ReopenStroke, t = 0.22, s = Enum.EasingStyle.Back, d = "Out", g = {Transparency = 0}}):Play()
		else
			tw({v = ReopenBtn, t = 0.12, g = {GroupTransparency = 1}}):Play()
			tw({v = ReopenStroke, t = 0.12, g = {Transparency = 1}}):Play()
			task.delay(0.12, function()
				if not isHidden then
					ReopenBtn.Visible = false
					ReopenStroke.Transparency = 1
				end
			end)
			Shadow_1.Visible = true
			Background_1.Visible = true
			Background_1.Size = UDim2.new(1, -12, 1, -12)
			UIStroke_Win.Transparency = 1
			Shadow_1.ImageTransparency = 1
			tw({v = Background_1, t = 0.16, s = Enum.EasingStyle.Back, d = "Out", g = {Size = UDim2.new(1, 0, 1, 0)}}):Play()
			tw({v = UIStroke_Win, t = 0.16, g = {Transparency = 0}}):Play()
			tw({v = Shadow_1, t = 0.16, g = {ImageTransparency = 0.5}}):Play()
			if ResizeVisual then
				tw({v = ResizeVisual, t = 0.16, g = {ImageTransparency = 0.4}}):Play()
			end
		end
	end

	ReopenClick.MouseEnter:Connect(function()
		local c = currentWindowTheme.colors or curColors
		tw({v = ReopenStroke, t = 0.15, g = {Color = c['Main']}}):Play()
		tw({v = ReopenBtn, t = 0.15, g = {BackgroundColor3 = c['Card']:Lerp(c['Main'], 0.08)}}):Play()
		tw({v = ReopenKeyStroke, t = 0.15, g = {Color = c['Main']}}):Play()
	end)
	ReopenClick.MouseLeave:Connect(function()
		local c = currentWindowTheme.colors or curColors
		tw({v = ReopenStroke, t = 0.15, g = {Color = c['CardBorder']}}):Play()
		tw({v = ReopenBtn, t = 0.15, g = {BackgroundColor3 = c['Card']}}):Play()
		tw({v = ReopenKeyStroke, t = 0.15, g = {Color = c['CardBorder']}}):Play()
	end)

	local fpsVal = 60
	local pingVal = 0
	local fpsFrames = 0
	local fpsLastTime = tick()

	local RunService = Session:Service("RunService")
	local StatsService = game:GetService("Stats")

	RunService.RenderStepped:Connect(function()
		fpsFrames = fpsFrames + 1
		local curTime = tick()
		if curTime - fpsLastTime >= 0.5 then
			fpsVal = math.round(fpsFrames / (curTime - fpsLastTime))
			fpsFrames = 0
			fpsLastTime = curTime
		end
	end)

	task.spawn(function()
		while ScreenGui and ScreenGui.Parent do
			pcall(function()
				local netItem = StatsService and StatsService.Network and StatsService.Network.ServerStatsItem and StatsService.Network.ServerStatsItem["Data Ping"]
				if netItem then
					pingVal = math.round(netItem:GetValue())
				end
			end)
			if ReopenStats and ReopenStats.Parent then
				local fpsColor = (fpsVal >= 50 and "rgb(34,197,94)") or (fpsVal >= 30 and "rgb(234,179,8)") or "rgb(239,68,68)"
				ReopenStats.Text = string.format(
					'<font color="%s"><b>%d</b></font> <font color="rgb(150,150,165)">FPS</font> <font color="rgb(70,70,85)">•</font> <font color="rgb(255,255,255)"><b>%d</b></font> <font color="rgb(150,150,165)">ms</font>',
					fpsColor,
					fpsVal,
					pingVal
				)
			end
			task.wait(0.5)
		end
	end)

	do
		local isDragging = false
		local dragStart = nil
		local startPos = nil
		local hasMoved = false

		local function startDrag(input)
			if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
				isDragging = true
				hasMoved = false
				dragStart = input.Position
				startPos = ReopenBtn.Position
			end
		end

		ReopenBtn.InputBegan:Connect(startDrag)
		ReopenClick.InputBegan:Connect(startDrag)

		U.InputChanged:Connect(function(input)
			if isDragging and (input.UserInputType == Enum.UserInputType.MouseMovement or input.UserInputType == Enum.UserInputType.Touch) then
				local delta = input.Position - dragStart
				if math.abs(delta.X) > 3 or math.abs(delta.Y) > 3 then
					hasMoved = true
				end
				ReopenBtn.Position = UDim2.new(
					startPos.X.Scale,
					startPos.X.Offset + delta.X,
					startPos.Y.Scale,
					startPos.Y.Offset + delta.Y
				)
			end
		end)

		U.InputEnded:Connect(function(input)
			if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
				isDragging = false
			end
		end)

		ReopenClick.MouseButton1Click:Connect(function()
			if not hasMoved then
				playClickSound()
				setUIVisibility(true)
			end
		end)
	end

	local org = Background_1.Size
	Background_1.Size = org - UDim2.fromOffset(6, 6)
	tw({
		v = Background_1,
		t = 0.22,
		s = Enum.EasingStyle.Quad,
		d = "Out",
		g = { Size = org }
	}):Play()
	tw({
		v = UIStroke_Win,
		t = 0.22,
		s = Enum.EasingStyle.Quad,
		d = "Out",
		g = { Transparency = 0 }
	}):Play()
	tw({
		v = Shadow_1,
		t = 0.22,
		s = Enum.EasingStyle.Quad,
		d = "Out",
		g = { ImageTransparency = 0.5 }
	}):Play()

	Sidebar = Instance.new("Frame")
	local SidebarBorder = Instance.new("Frame")
	local SidebarCorner = Instance.new("UICorner")
	local SidebarSquareFiller = Instance.new("Frame")

	Sidebar.Name = "Sidebar"
	Sidebar.Parent = Background_1
	Sidebar.BackgroundColor3 = curColors['Sidebar']
	Sidebar.BackgroundTransparency = (currentWindowTheme.transparency and currentWindowTheme.transparency.Sidebar or 0) / 100
	Sidebar.BorderSizePixel = 0
	Sidebar.Size = UDim2.new(0, 165, 1, 0)
	Sidebar.ZIndex = 5

	SidebarCorner.CornerRadius = UDim.new(0, 10)
	SidebarCorner.Parent = Sidebar

	SidebarSquareFiller.Name = "SquareFiller"
	SidebarSquareFiller.Parent = Sidebar
	SidebarSquareFiller.BackgroundColor3 = curColors['Sidebar']
	SidebarSquareFiller.BackgroundTransparency = (currentWindowTheme.transparency and currentWindowTheme.transparency.Sidebar or 0) / 100
	SidebarSquareFiller.BorderSizePixel = 0
	SidebarSquareFiller.Position = UDim2.new(1, -12, 0, 0)
	SidebarSquareFiller.Size = UDim2.new(0, 12, 1, 0)
	SidebarSquareFiller.ZIndex = 5

	SidebarBorder.Name = "SidebarBorder"
	SidebarBorder.Parent = Background_1
	SidebarBorder.BackgroundColor3 = curColors['CardBorder']
	SidebarBorder.BorderSizePixel = 0
	SidebarBorder.Position = UDim2.new(0, 165, 0, 0)
	SidebarBorder.Size = UDim2.new(0, 1, 1, 0)
	SidebarBorder.ZIndex = 6

	local LogoFrame = Instance.new("Frame")
	local LogoLayout = Instance.new("UIListLayout")
	local LogoTitle = Instance.new("TextLabel")
	local LogoSub = Instance.new("TextLabel")

	LogoFrame.Name = "LogoFrame"
	LogoFrame.Parent = Sidebar
	LogoFrame.BackgroundTransparency = 1
	LogoFrame.Position = UDim2.new(0, 16, 0, 14)
	LogoFrame.Size = UDim2.new(1, -24, 0, 24)

	LogoLayout.Parent = LogoFrame
	LogoLayout.FillDirection = Enum.FillDirection.Horizontal
	LogoLayout.SortOrder = Enum.SortOrder.LayoutOrder
	LogoLayout.VerticalAlignment = Enum.VerticalAlignment.Center
	LogoLayout.Padding = UDim.new(0, 5)

	LogoTitle.Name = "LogoTitle"
	LogoTitle.Parent = LogoFrame
	LogoTitle.BackgroundTransparency = 1
	LogoTitle.Size = UDim2.new(0, 0, 1, 0)
	LogoTitle.AutomaticSize = Enum.AutomaticSize.X
	LogoTitle.Font = Enum.Font.BuilderSansBold
	LogoTitle.Text = tostring(Title)
	LogoTitle.TextColor3 = Color3.fromRGB(255, 255, 255)
	LogoTitle.TextSize = 17
	LogoTitle.TextXAlignment = Enum.TextXAlignment.Left
	LogoTitle.LayoutOrder = 1

	LogoSub.Name = "LogoSub"
	LogoSub.Parent = LogoFrame
	LogoSub.BackgroundTransparency = 1
	LogoSub.Size = UDim2.new(0, 0, 1, 0)
	LogoSub.AutomaticSize = Enum.AutomaticSize.X
	LogoSub.Font = Enum.Font.BuilderSansBold
	LogoSub.Text = tostring(Desc)
	LogoSub.TextColor3 = (currentWindowTheme.name == "Default") and Color3.fromRGB(255, 255, 255) or curColors['Main']
	LogoSub.TextSize = 17
	LogoSub.TextXAlignment = Enum.TextXAlignment.Left
	LogoSub.LayoutOrder = 2

	local TabListFrame = Instance.new("ScrollingFrame")
	local TabListLayout = Instance.new("UIListLayout")
	local TabListPadding = Instance.new("UIPadding")

	TabListFrame.Name = "TabList"
	TabListFrame.Parent = Sidebar
	TabListFrame.BackgroundTransparency = 1
	TabListFrame.BorderSizePixel = 0
	TabListFrame.Position = UDim2.new(0, 0, 0, 48)
	TabListFrame.Size = UDim2.new(1, 0, 1, -96)
	TabListFrame.CanvasSize = UDim2.new(0, 0, 0, 0)
	TabListFrame.ScrollBarThickness = 0
	TabListFrame.ClipsDescendants = true

	TabListLayout.Parent = TabListFrame
	TabListLayout.SortOrder = Enum.SortOrder.LayoutOrder
	TabListLayout.Padding = UDim.new(0, 5)

	TabListPadding.Parent = TabListFrame
	TabListPadding.PaddingLeft = UDim.new(0, 10)
	TabListPadding.PaddingRight = UDim.new(0, 10)
	TabListPadding.PaddingTop = UDim.new(0, 4)

	changecanvas(TabListFrame, TabListLayout, 5)

	local SubInfoFrame = Instance.new("Frame")
	local SubInfoLayout = Instance.new("UIListLayout")
	local SubTierLabelSidebar = Instance.new("TextLabel")
	local SubExpiresLabelSidebar = Instance.new("TextLabel")

	SubInfoFrame.Name = "SubscriptionInfo"
	SubInfoFrame.Parent = Sidebar
	SubInfoFrame.BackgroundTransparency = 1
	SubInfoFrame.Position = UDim2.new(0, 16, 1, -56)
	SubInfoFrame.Size = UDim2.new(1, -24, 0, 48)

	SubInfoLayout.Parent = SubInfoFrame
	SubInfoLayout.SortOrder = Enum.SortOrder.LayoutOrder
	SubInfoLayout.Padding = UDim.new(0, 4)
	SubInfoLayout.VerticalAlignment = Enum.VerticalAlignment.Bottom

	SubTierLabelSidebar.Name = "SubscriptionTier"
	SubTierLabelSidebar.Parent = SubInfoFrame
	SubTierLabelSidebar.BackgroundTransparency = 1
	SubTierLabelSidebar.Size = UDim2.new(1, 0, 0, 18)
	SubTierLabelSidebar.Font = Enum.Font.BuilderSansMedium
	SubTierLabelSidebar.RichText = true
	SubTierLabelSidebar.Text = '<font color="rgb(180,180,195)">Subscription: </font><font color="rgb(255,255,255)"><b>Free</b></font>'
	SubTierLabelSidebar.TextSize = 13
	SubTierLabelSidebar.TextXAlignment = Enum.TextXAlignment.Left
	SubTierLabelSidebar.LayoutOrder = 1

	SubExpiresLabelSidebar.Name = "SubscriptionExpires"
	SubExpiresLabelSidebar.Parent = SubInfoFrame
	SubExpiresLabelSidebar.BackgroundTransparency = 1
	SubExpiresLabelSidebar.Size = UDim2.new(1, 0, 0, 18)
	SubExpiresLabelSidebar.Font = Enum.Font.BuilderSansMedium
	SubExpiresLabelSidebar.RichText = true
	SubExpiresLabelSidebar.Text = '<font color="rgb(180,180,195)">Expires at: </font><font color="rgb(255,255,255)">0 seconds</font>'
	SubExpiresLabelSidebar.TextSize = 13
	SubExpiresLabelSidebar.TextXAlignment = Enum.TextXAlignment.Left
	SubExpiresLabelSidebar.LayoutOrder = 2

	local Topbar = Instance.new("Frame")
	Topbar.Name = "Topbar"
	Topbar.Parent = Background_1
	Topbar.BackgroundTransparency = 1
	Topbar.Position = UDim2.new(0, 165, 0, 0)
	Topbar.Size = UDim2.new(1, -165, 0, 48)
	Topbar.ZIndex = 50

	lak(Topbar, Shadow_1)

	local isResizingWindow = false
	local resizeDragStart = nil
	local resizeStartSize = nil
	local resizeStartPos = nil
	local resizeActiveInput = nil
	local autoSaveSize = true

	ResizeHandle.MouseEnter:Connect(function()
		if not isResizingWindow then
			tw({v = ResizeVisual, t = 0.15, g = {ImageTransparency = 0.15}}):Play()
		end
	end)

	ResizeHandle.MouseLeave:Connect(function()
		if not isResizingWindow then
			tw({v = ResizeVisual, t = 0.15, g = {ImageTransparency = 0.4}}):Play()
		end
	end)

	local function startResizeWindow(input)
		isResizingWindow = true
		resizeActiveInput = input
		resizeDragStart = input.Position
		resizeStartSize = Vector2.new(Shadow_1.Size.X.Offset, Shadow_1.Size.Y.Offset)
		resizeStartPos = Shadow_1.Position
		tw({v = ResizeVisual, t = 0.12, g = {ImageTransparency = 0, Size = UDim2.new(0, 19, 0, 19)}}):Play()
	end

	ResizeHandle.InputBegan:Connect(function(input)
		if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
			startResizeWindow(input)
		end
	end)

	local resizeInputChangedConn = U.InputChanged:Connect(function(input)
		if isResizingWindow then
			if input.UserInputType == Enum.UserInputType.MouseMovement or (input.UserInputType == Enum.UserInputType.Touch and (resizeActiveInput == nil or input == resizeActiveInput)) then
				local k = WindowScale.Scale
				local delta = (input.Position - resizeDragStart) / k
				local camera = workspace.CurrentCamera
				local vp = (camera and camera.ViewportSize or Vector2.new(1920, 1080)) / k
				local minW, minH = 460, 280
				local maxW = math.max(minW, vp.X - 16)
				local maxH = math.max(minH, vp.Y - 16)

				local newW = math.clamp(math.round(resizeStartSize.X + delta.X), minW, maxW)
				local newH = math.clamp(math.round(resizeStartSize.Y + delta.Y), minH, maxH)

				local deltaW = newW - resizeStartSize.X
				local deltaH = newH - resizeStartSize.Y

				Shadow_1.Size = UDim2.new(0, newW, 0, newH)
				Shadow_1.Position = UDim2.new(
					resizeStartPos.X.Scale,
					resizeStartPos.X.Offset + (deltaW * k / 2),
					resizeStartPos.Y.Scale,
					resizeStartPos.Y.Offset + (deltaH * k / 2)
				)
			end
		end
	end)

	local resizeInputEndedConn = U.InputEnded:Connect(function(input)
		if isResizingWindow then
			if input.UserInputType == Enum.UserInputType.MouseButton1 or (input.UserInputType == Enum.UserInputType.Touch and (resizeActiveInput == nil or input == resizeActiveInput)) then
				isResizingWindow = false
				resizeActiveInput = nil
				tw({v = ResizeVisual, t = 0.18, g = {ImageTransparency = 0.4, Size = UDim2.new(0, 16, 0, 16)}}):Play()
				if autoSaveSize and Shadow_1 and Shadow_1.Parent then
					local curW = Shadow_1.Size.X.Offset
					local curH = Shadow_1.Size.Y.Offset
					FileSystem:SaveWindowSize(curW, curH)
				end
			end
		end
	end)

	Shadow_1.Destroying:Connect(function()
		pcall(function() resizeInputChangedConn:Disconnect() end)
		pcall(function() resizeInputEndedConn:Disconnect() end)
	end)

	local WindowControls = Instance.new("Frame")
	local WinLayout = Instance.new("UIListLayout")
	local Minisize_1 = Instance.new("ImageButton")
	local Close_1 = Instance.new("ImageButton")

	WindowControls.Name = "WindowControls"
	WindowControls.Parent = Topbar
	WindowControls.AnchorPoint = Vector2.new(1, 0.5)
	WindowControls.BackgroundTransparency = 1
	WindowControls.Position = UDim2.new(1, -12, 0.5, 0)
	WindowControls.Size = UDim2.new(0, 56, 0, 26)

	WinLayout.Parent = WindowControls
	WinLayout.FillDirection = Enum.FillDirection.Horizontal
	WinLayout.SortOrder = Enum.SortOrder.LayoutOrder
	WinLayout.HorizontalAlignment = Enum.HorizontalAlignment.Right
	WinLayout.VerticalAlignment = Enum.VerticalAlignment.Center
	WinLayout.Padding = UDim.new(0, 12)

	Minisize_1.Name = "Minimize"
	Minisize_1.Parent = WindowControls
	Minisize_1.BackgroundTransparency = 1
	Minisize_1.Size = UDim2.new(0, 18, 0, 18)
	Minisize_1.Image = "rbxassetid://10734896206"
	Minisize_1.ImageColor3 = curColors['SubText']

	Close_1.Name = "Close"
	Close_1.Parent = WindowControls
	Close_1.BackgroundTransparency = 1
	Close_1.Size = UDim2.new(0, 18, 0, 18)
	Close_1.Image = "rbxassetid://15082305656"
	Close_1.ImageColor3 = curColors['SubText']

	Minisize_1.MouseEnter:Connect(function() tw({v = Minisize_1, t = 0.15, g = {ImageColor3 = Color3.fromRGB(255, 255, 255)}}):Play() end)
	Minisize_1.MouseLeave:Connect(function()
		local c = currentWindowTheme.colors or curColors
		tw({v = Minisize_1, t = 0.15, g = {ImageColor3 = c['SubText']}}):Play()
	end)

	Close_1.MouseEnter:Connect(function() tw({v = Close_1, t = 0.15, g = {ImageColor3 = Color3.fromRGB(255, 70, 70)}}):Play() end)
	Close_1.MouseLeave:Connect(function()
		local c = currentWindowTheme.colors or curColors
		tw({v = Close_1, t = 0.15, g = {ImageColor3 = c['SubText']}}):Play()
	end)

	local FeaturesRegistry = {}

	local function registerFeature(title, cType, frame, meta)
		if not title or title == "" then return end
		table.insert(FeaturesRegistry, {
			Title = tostring(title),
			Type = cType or "Feature",
			Frame = frame,
			TabTitle = (meta and meta.TabTitle) or "Home",
			SubTabTitle = meta and meta.SubTabTitle,
			SelectTab = meta and meta.SelectTab,
			SelectSubTab = meta and meta.SelectSubTab,
			Scroll = meta and meta.Scroll
		})
	end

	local SearchFrame = Instance.new("Frame")
	local SearchCorner = Instance.new("UICorner")
	local SearchStroke = Instance.new("UIStroke")
	local SearchIcon = Instance.new("ImageLabel")
	local SearchBox = Instance.new("TextBox")
	local ClearSearchBtn = Instance.new("ImageButton")

	SearchFrame.Name = "SearchFrame"
	SearchFrame.Parent = Topbar
	SearchFrame.AnchorPoint = Vector2.new(1, 0.5)
	SearchFrame.Position = UDim2.new(1, -78, 0.5, 0)
	SearchFrame.Size = UDim2.new(0, 195, 0, 28)
	SearchFrame.BackgroundColor3 = curColors['Card']
	SearchFrame.BorderSizePixel = 0

	SearchCorner.CornerRadius = UDim.new(0, 6)
	SearchCorner.Parent = SearchFrame

	SearchStroke.Color = curColors['CardBorder']
	SearchStroke.Thickness = 1
	SearchStroke.Parent = SearchFrame

	SearchIcon.Name = "Icon"
	SearchIcon.Parent = SearchFrame
	SearchIcon.AnchorPoint = Vector2.new(0, 0.5)
	SearchIcon.BackgroundTransparency = 1
	SearchIcon.Position = UDim2.new(0, 8, 0.5, 0)
	SearchIcon.Size = UDim2.new(0, 14, 0, 14)
	SearchIcon.Image = "rbxassetid://10734883986"
	SearchIcon.ImageColor3 = curColors['SubText']

	SearchBox.Name = "Box"
	SearchBox.Parent = SearchFrame
	SearchBox.BackgroundTransparency = 1
	SearchBox.Position = UDim2.new(0, 28, 0, 0)
	SearchBox.Size = UDim2.new(1, -50, 1, 0)
	SearchBox.Font = Enum.Font.BuilderSansMedium
	SearchBox.PlaceholderText = "Search features..."
	SearchBox.PlaceholderColor3 = curColors['SubText']
	SearchBox.Text = ""
	SearchBox.TextColor3 = curColors['Text & Icon']
	SearchBox.TextSize = 13
	SearchBox.TextXAlignment = Enum.TextXAlignment.Left
	SearchBox.ClearTextOnFocus = false

	ClearSearchBtn.Name = "Clear"
	ClearSearchBtn.Parent = SearchFrame
	ClearSearchBtn.AnchorPoint = Vector2.new(1, 0.5)
	ClearSearchBtn.BackgroundTransparency = 1
	ClearSearchBtn.Position = UDim2.new(1, -7, 0.5, 0)
	ClearSearchBtn.Size = UDim2.new(0, 12, 0, 12)
	ClearSearchBtn.Image = "rbxassetid://15082305656"
	ClearSearchBtn.ImageColor3 = curColors['SubText']
	ClearSearchBtn.Visible = false

	local SearchDropdown = Instance.new("Frame")
	local DropCorner = Instance.new("UICorner")
	local DropStroke = Instance.new("UIStroke")
	local DropScroll = Instance.new("ScrollingFrame")
	local DropLayout = Instance.new("UIListLayout")
	local DropPadding = Instance.new("UIPadding")

	SearchDropdown.Name = "SearchDropdown"
	SearchDropdown.Parent = Topbar
	SearchDropdown.AnchorPoint = Vector2.new(1, 0)
	SearchDropdown.Position = UDim2.new(1, -66, 0.5, 18)
	SearchDropdown.Size = UDim2.new(0, 245, 0, 0)
	SearchDropdown.BackgroundColor3 = curColors['Card']
	SearchDropdown.BorderSizePixel = 0
	SearchDropdown.Visible = false
	SearchDropdown.ZIndex = 150
	SearchDropdown.ClipsDescendants = true

	DropCorner.CornerRadius = UDim.new(0, 8)
	DropCorner.Parent = SearchDropdown

	DropStroke.Color = curColors['CardBorder']
	DropStroke.Thickness = 1
	DropStroke.Parent = SearchDropdown

	DropScroll.Name = "Scroll"
	DropScroll.Parent = SearchDropdown
	DropScroll.BackgroundTransparency = 1
	DropScroll.BorderSizePixel = 0
	DropScroll.Size = UDim2.new(1, 0, 1, 0)
	DropScroll.ScrollBarThickness = 2
	DropScroll.ScrollBarImageColor3 = Color3.fromRGB(150, 150, 160)
	DropScroll.CanvasSize = UDim2.new(0, 0, 0, 0)
	DropScroll.ZIndex = 151

	DropLayout.Parent = DropScroll
	DropLayout.SortOrder = Enum.SortOrder.LayoutOrder
	DropLayout.Padding = UDim.new(0, 3)

	DropPadding.Parent = DropScroll
	DropPadding.PaddingTop = UDim.new(0, 4)
	DropPadding.PaddingBottom = UDim.new(0, 4)
	DropPadding.PaddingLeft = UDim.new(0, 4)
	DropPadding.PaddingRight = UDim.new(0, 4)

	local function updateSearch(queryText)
		local q = queryText:lower():match("^%s*(.-)%s*$")
		for _, child in ipairs(DropScroll:GetChildren()) do
			if child:IsA("Frame") or child:IsA("TextLabel") or child:IsA("TextButton") then
				child:Destroy()
			end
		end

		if not q or q == "" then
			SearchDropdown.Visible = false
			ClearSearchBtn.Visible = false
			return
		end

		ClearSearchBtn.Visible = true

		local qClean = q:gsub("[%s%-%_]", "")
		local matches = {}
		for _, feat in ipairs(FeaturesRegistry) do
			local tLower = feat.Title:lower()
			local tClean = tLower:gsub("[%s%-%_]", "")
			local typeLower = feat.Type:lower()
			local tabLower = feat.TabTitle:lower()
			local subLower = feat.SubTabTitle and feat.SubTabTitle:lower() or ""

			if tLower:find(q, 1, true) or (qClean ~= "" and tClean:find(qClean, 1, true)) then
				local isPrefix = (tLower:sub(1, #q) == q) or (qClean ~= "" and tClean:sub(1, #qClean) == qClean)
				table.insert(matches, { feat = feat, score = (isPrefix and 1 or 2) })
			elseif subLower:find(q, 1, true) or tabLower:find(q, 1, true) or typeLower:find(q, 1, true) then
				table.insert(matches, { feat = feat, score = 3 })
			end
		end

		table.sort(matches, function(a, b) return a.score < b.score end)

		if #matches == 0 then
			local emptyLabel = Instance.new("TextLabel")
			emptyLabel.Name = "Empty"
			emptyLabel.Parent = DropScroll
			emptyLabel.BackgroundTransparency = 1
			emptyLabel.Size = UDim2.new(1, 0, 0, 36)
			emptyLabel.Font = Enum.Font.BuilderSansMedium
			emptyLabel.Text = "No features found for '" .. queryText .. "'"
			emptyLabel.TextColor3 = curColors['SubText']
			emptyLabel.TextSize = 13
			emptyLabel.TextXAlignment = Enum.TextXAlignment.Center
			emptyLabel.ZIndex = 152

			SearchDropdown.Size = UDim2.new(0, 245, 0, 44)
			SearchDropdown.Visible = true
			DropScroll.CanvasSize = UDim2.new(0, 0, 0, 44)
			return
		end

		local maxVisible = math.min(#matches, 6)
		SearchDropdown.Size = UDim2.new(0, 245, 0, maxVisible * 42 + 8)
		SearchDropdown.Visible = true
		DropScroll.CanvasSize = UDim2.new(0, 0, 0, #matches * 42 + 8)

		for i, m in ipairs(matches) do
			local feat = m.feat
			local itemBtn = Instance.new("TextButton")
			local itemCorner = Instance.new("UICorner")
			local itemPadding = Instance.new("UIPadding")
			local itemTitle = Instance.new("TextLabel")
			local itemLoc = Instance.new("TextLabel")
			local itemBadge = Instance.new("Frame")
			local badgeCorner = Instance.new("UICorner")
			local badgeStroke = Instance.new("UIStroke")
			local badgeLabel = Instance.new("TextLabel")

			itemBtn.Name = "Result_" .. i
			itemBtn.Parent = DropScroll
			itemBtn.BackgroundColor3 = Color3.fromRGB(24, 24, 30)
			itemBtn.BackgroundTransparency = 1
			itemBtn.BorderSizePixel = 0
			itemBtn.Size = UDim2.new(1, 0, 0, 38)
			itemBtn.AutoButtonColor = false
			itemBtn.Text = ""
			itemBtn.ZIndex = 152

			itemCorner.CornerRadius = UDim.new(0, 6)
			itemCorner.Parent = itemBtn

			itemPadding.Parent = itemBtn
			itemPadding.PaddingLeft = UDim.new(0, 8)
			itemPadding.PaddingRight = UDim.new(0, 8)

			itemTitle.Name = "Title"
			itemTitle.Parent = itemBtn
			itemTitle.BackgroundTransparency = 1
			itemTitle.Position = UDim2.new(0, 0, 0, 4)
			itemTitle.Size = UDim2.new(1, -65, 0, 16)
			itemTitle.Font = Enum.Font.BuilderSansBold
			itemTitle.Text = feat.Title
			itemTitle.TextColor3 = curColors['Text & Icon']
			itemTitle.TextSize = 13
			itemTitle.TextXAlignment = Enum.TextXAlignment.Left
			itemTitle.TextTruncate = Enum.TextTruncate.AtEnd
			itemTitle.ZIndex = 153

			local locStr = feat.TabTitle .. (feat.SubTabTitle and (" › " .. feat.SubTabTitle) or "")
			itemLoc.Name = "Loc"
			itemLoc.Parent = itemBtn
			itemLoc.BackgroundTransparency = 1
			itemLoc.Position = UDim2.new(0, 0, 0, 20)
			itemLoc.Size = UDim2.new(1, -65, 0, 14)
			itemLoc.Font = Enum.Font.BuilderSansMedium
			itemLoc.Text = locStr
			itemLoc.TextColor3 = curColors['SubText']
			itemLoc.TextSize = 12
			itemLoc.TextXAlignment = Enum.TextXAlignment.Left
			itemLoc.ZIndex = 153

			itemBadge.Name = "Badge"
			itemBadge.Parent = itemBtn
			itemBadge.AnchorPoint = Vector2.new(1, 0.5)
			itemBadge.BackgroundColor3 = Color3.fromRGB(18, 18, 22)
			itemBadge.BorderSizePixel = 0
			itemBadge.Position = UDim2.new(1, 0, 0.5, 0)
			itemBadge.Size = UDim2.new(0, 58, 0, 18)
			itemBadge.ZIndex = 153

			badgeCorner.CornerRadius = UDim.new(0, 4)
			badgeCorner.Parent = itemBadge

			badgeStroke.Color = curColors['CardBorder']
			badgeStroke.Thickness = 1
			badgeStroke.Parent = itemBadge

			badgeLabel.Name = "Label"
			badgeLabel.Parent = itemBadge
			badgeLabel.BackgroundTransparency = 1
			badgeLabel.Size = UDim2.new(1, 0, 1, 0)
			badgeLabel.Font = Enum.Font.BuilderSansBold
			badgeLabel.Text = feat.Type:upper()
			badgeLabel.TextColor3 = curColors['SubText']
			badgeLabel.TextSize = 11
			badgeLabel.ZIndex = 154

			itemBtn.MouseEnter:Connect(function()
				local c = currentWindowTheme.colors or curColors
				tw({v = itemBtn, t = 0.12, g = {BackgroundTransparency = 0, BackgroundColor3 = c['Card']:Lerp(c['Main'], 0.08)}}):Play()
				tw({v = badgeLabel, t = 0.12, g = {TextColor3 = c['Main']}}):Play()
			end)
			itemBtn.MouseLeave:Connect(function()
				local c = currentWindowTheme.colors or curColors
				tw({v = itemBtn, t = 0.12, g = {BackgroundTransparency = 1}}):Play()
				tw({v = badgeLabel, t = 0.12, g = {TextColor3 = c['SubText']}}):Play()
			end)

			itemBtn.MouseButton1Click:Connect(function()
				SearchDropdown.Visible = false
				SearchBox.Text = ""
				ClearSearchBtn.Visible = false
				SearchBox:ReleaseFocus()

				if feat.SelectTab then feat.SelectTab() end
				if feat.SelectSubTab then feat.SelectSubTab() end

				if feat.Frame then
					local p = feat.Frame
					while p and p ~= feat.Scroll and p ~= game do
						if p:IsA("Frame") and p.Name:sub(1, 8) == "Section_" then
							for _, sec in ipairs(currentWindowTheme.sections or {}) do
								if sec.Frame == p and sec.IsMinimized and sec.IsMinimized() then
									sec.SetMinimized(false)
								end
							end
							break
						end
						p = p.Parent
					end
				end

				if feat.Scroll and feat.Frame then
					task.defer(function()
						pcall(function()
							local targetY = feat.Frame.AbsolutePosition.Y - feat.Scroll.AbsolutePosition.Y + feat.Scroll.CanvasPosition.Y - 20
							feat.Scroll.CanvasPosition = Vector2.new(0, math.clamp(targetY, 0, feat.Scroll.AbsoluteCanvasSize.Y))
							local stroke = feat.Frame:FindFirstChildOfClass("UIStroke")
							if stroke then
								local prevCol = stroke.Color
								tw({v = stroke, t = 0.2, g = {Color = Color3.fromRGB(255, 255, 255)}}):Play()
								task.delay(0.7, function()
									tw({v = stroke, t = 0.4, g = {Color = prevCol}}):Play()
								end)
							end
						end)
					end)
				end
			end)
		end
	end

	SearchBox:GetPropertyChangedSignal("Text"):Connect(function()
		updateSearch(SearchBox.Text)
	end)

	ClearSearchBtn.MouseButton1Click:Connect(function()
		SearchBox.Text = ""
		updateSearch("")
	end)

	U.InputBegan:Connect(function(input)
		if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
			if SearchDropdown.Visible then
				local mPos = input.Position
				local sfPos = SearchFrame.AbsolutePosition
				local sfSize = SearchFrame.AbsoluteSize
				local sdPos = SearchDropdown.AbsolutePosition
				local sdSize = SearchDropdown.AbsoluteSize

				local inSF = (mPos.X >= sfPos.X and mPos.X <= sfPos.X + sfSize.X and mPos.Y >= sfPos.Y and mPos.Y <= sfPos.Y + sfSize.Y)
				local inSD = (mPos.X >= sdPos.X and mPos.X <= sdPos.X + sdSize.X and mPos.Y >= sdPos.Y and mPos.Y <= sdPos.Y + sdSize.Y)

				if not inSF and not inSD then
					SearchDropdown.Visible = false
				end
			end
		end
	end)

	local Page_1 = Instance.new("Frame")
	Page_1.Name = "Page"
	Page_1.Parent = Background_1
	Page_1.BackgroundColor3 = curColors['Page']
	Page_1.BackgroundTransparency = 1
	Page_1.BorderSizePixel = 0
	Page_1.Position = UDim2.new(0, 165, 0, 48)
	Page_1.Size = UDim2.new(1, -165, 1, -48)
	Page_1.ZIndex = 5

	local PagePadding = Instance.new("UIPadding")
	PagePadding.Parent = Page_1
	PagePadding.PaddingBottom = UDim.new(0, 8)
	PagePadding.PaddingLeft = UDim.new(0, 14)
	PagePadding.PaddingRight = UDim.new(0, 14)
	PagePadding.PaddingTop = UDim.new(0, 2)

	local Tabs = {
		List = {},
		DefaultIndex = 1,
		UIToggleKeybind = Keybind,
		ShowReopenPill = true,
		ActiveTabName = "Home",
		StreamerMode = false,
		SubscriptionTier = "Free",
		SubscriptionExpires = "0 seconds",
	}

	function Tabs:SaveWindowSize()
		local curW = Shadow_1.Size.X.Offset
		local curH = Shadow_1.Size.Y.Offset
		FileSystem:SaveWindowSize(curW, curH)
		Tabs:Notify({
			Title = "Window Size Saved",
			Desc = string.format("Default size locked to %d x %d", curW, curH),
			Time = 2.5
		})
	end

	function Tabs:ResetWindowSize()
		FileSystem:ResetWindowSize()
		local defW = defaultSize.X.Offset
		local defH = defaultSize.Y.Offset
		local curW = Shadow_1.Size.X.Offset
		local curH = Shadow_1.Size.Y.Offset
		local deltaW = defW - curW
		local deltaH = defH - curH
		local curPos = Shadow_1.Position

		tw({
			v = Shadow_1,
			t = 0.25,
			s = Enum.EasingStyle.Quad,
			d = "Out",
			g = {
				Size = defaultSize,
				Position = UDim2.new(
					curPos.X.Scale,
					curPos.X.Offset + (deltaW / 2),
					curPos.Y.Scale,
					curPos.Y.Offset + (deltaH / 2)
				)
			}
		}):Play()

		Tabs:Notify({
			Title = "Window Size Reset",
			Desc = string.format("Restored default size (%d x %d)", defW, defH),
			Time = 2.5
		})
	end

	function Tabs:SetWindowSize(w, h, save)
		local curW = Shadow_1.Size.X.Offset
		local curH = Shadow_1.Size.Y.Offset
		local deltaW = w - curW
		local deltaH = h - curH
		local curPos = Shadow_1.Position

		Shadow_1.Size = UDim2.new(0, w, 0, h)
		Shadow_1.Position = UDim2.new(
			curPos.X.Scale,
			curPos.X.Offset + (deltaW / 2),
			curPos.Y.Scale,
			curPos.Y.Offset + (deltaH / 2)
		)

		if save then
			FileSystem:SaveWindowSize(w, h)
		end
	end

	function Tabs:GetWindowSize()
		return Shadow_1.Size.X.Offset, Shadow_1.Size.Y.Offset
	end

	function Tabs:ApplyTheme(themeNameOrColors)
		local tCols
		if type(themeNameOrColors) == "string" and themes[themeNameOrColors] then
			currentWindowTheme.name = themeNameOrColors
			currentWindowTheme.colors = deepCopy(themes[themeNameOrColors])
			tCols = currentWindowTheme.colors
		elseif type(themeNameOrColors) == "table" then
			currentWindowTheme.name = "Custom"
			local newCols = deepCopy(currentWindowTheme.colors or themes.Default)
			for k, v in pairs(themeNameOrColors) do
				newCols[k] = v
			end
			if themeNameOrColors.Background then
				newCols.Page = themeNameOrColors.Background
				newCols.Sidebar = themeNameOrColors.Background
			end
			if themeNameOrColors.Main then
				local m = themeNameOrColors.Main
				newCols.MainDark = Color3.fromRGB(math.floor(m.R * 40), math.floor(m.G * 40), math.floor(m.B * 40))
				newCols.MainLight = Color3.fromRGB(math.min(255, math.floor(m.R * 255 + 25)), math.min(255, math.floor(m.G * 255 + 25)), math.min(255, math.floor(m.G * 255 + 25)))
			end
			currentWindowTheme.colors = newCols
			tCols = currentWindowTheme.colors
		else
			tCols = currentWindowTheme.colors
		end

		curColors = tCols

		tw({v = Background_1, t = 0.2, g = {BackgroundColor3 = tCols['Background']}}):Play()
		tw({v = UIStroke_Win, t = 0.2, g = {Color = tCols['CardBorder']}}):Play()
		tw({v = Sidebar, t = 0.2, g = {BackgroundColor3 = tCols['Sidebar']}}):Play()
		tw({v = SidebarBorder, t = 0.2, g = {BackgroundColor3 = tCols['CardBorder']}}):Play()
		tw({v = ReopenBtn, t = 0.2, g = {BackgroundColor3 = tCols['Card']}}):Play()
		tw({v = ReopenStroke, t = 0.2, g = {Color = tCols['CardBorder']}}):Play()
		if ReopenPlayerLabel and ReopenPlayerLabel.Parent then tw({v = ReopenPlayerLabel, t = 0.2, g = {TextColor3 = tCols['Text & Icon']}}):Play() end
		if ReopenDiv1 and ReopenDiv1.Parent then tw({v = ReopenDiv1, t = 0.2, g = {BackgroundColor3 = tCols['CardBorder']}}):Play() end
		if ReopenDiv2 and ReopenDiv2.Parent then tw({v = ReopenDiv2, t = 0.2, g = {BackgroundColor3 = tCols['CardBorder']}}):Play() end
		if ReopenKeyBadge and ReopenKeyBadge.Parent then tw({v = ReopenKeyBadge, t = 0.2, g = {BackgroundColor3 = tCols['Background']:Lerp(tCols['Card'], 0.5)}}):Play() end
		if ReopenKeyStroke and ReopenKeyStroke.Parent then tw({v = ReopenKeyStroke, t = 0.2, g = {Color = tCols['CardBorder']}}):Play() end
		if ReopenKeybind and ReopenKeybind.Parent then tw({v = ReopenKeybind, t = 0.2, g = {TextColor3 = tCols['Main']}}):Play() end

		if LogoSub and LogoSub.Parent then
			local subColor = (currentWindowTheme.name == "Default") and Color3.fromRGB(255, 255, 255) or tCols['Main']
			tw({v = LogoSub, t = 0.2, g = {TextColor3 = subColor}}):Play()
		end

		for _, item in ipairs(self.List) do
			local isActive = (item.Page and item.Page.Visible)
			if isActive then
				tw({v = item.Button, t = 0.18, g = {BackgroundColor3 = tCols['MainDark'], BackgroundTransparency = 0}}):Play()
				tw({v = item.Icon, t = 0.18, g = {ImageColor3 = tCols['Main']}}):Play()
				tw({v = item.TitleLabel, t = 0.18, g = {TextColor3 = tCols['Text & Icon']}}):Play()
			else
				tw({v = item.Button, t = 0.18, g = {BackgroundTransparency = 1}}):Play()
				tw({v = item.Icon, t = 0.18, g = {ImageColor3 = tCols['SubText']}}):Play()
				tw({v = item.TitleLabel, t = 0.18, g = {TextColor3 = tCols['SubText']}}):Play()
			end
		end

		for _, card in ipairs(currentWindowTheme.cards) do
			if card.Background and card.Background.Parent then
				tw({v = card.Background, t = 0.2, g = {BackgroundColor3 = tCols['Card']}}):Play()
				tw({v = card.UIStroke, t = 0.2, g = {Color = tCols['CardBorder']}}):Play()
				if card.Title then tw({v = card.Title, t = 0.2, g = {TextColor3 = tCols['Text & Icon']}}):Play() end
				if card.Desc then tw({v = card.Desc, t = 0.2, g = {TextColor3 = tCols['SubText']}}):Play() end
				if card.Icon then tw({v = card.Icon, t = 0.2, g = {ImageColor3 = tCols['Main']}}):Play() end
			end
		end

		for _, sec in ipairs(currentWindowTheme.sections or {}) do
			if sec.Frame and sec.Frame.Parent then
				tw({v = sec.Frame, t = 0.2, g = {BackgroundColor3 = tCols['Background']:Lerp(tCols['Card'], 0.35)}}):Play()
				if sec.Stroke then tw({v = sec.Stroke, t = 0.2, g = {Color = tCols['CardBorder']}}):Play() end
				if sec.Title then tw({v = sec.Title, t = 0.2, g = {TextColor3 = tCols['SubText']}}):Play() end
				if sec.Indicator then tw({v = sec.Indicator, t = 0.2, g = {BackgroundColor3 = tCols['Main']}}):Play() end
				if sec.MinBtn then tw({v = sec.MinBtn, t = 0.2, g = {ImageColor3 = tCols['SubText']}}):Play() end
			end
		end

		for _, st in ipairs(currentWindowTheme.subTabs or {}) do
			if st.Button and st.Button.Parent then
				if st.IsActive then
					tw({v = st.Button, t = 0.2, g = {BackgroundColor3 = tCols['Main']}}):Play()
					tw({v = st.Label, t = 0.2, g = {TextColor3 = tCols['Background']}}):Play()
					tw({v = st.Stroke, t = 0.2, g = {Color = tCols['Main']}}):Play()
				else
					tw({v = st.Button, t = 0.2, g = {BackgroundColor3 = tCols['Card']}}):Play()
					tw({v = st.Label, t = 0.2, g = {TextColor3 = tCols['SubText']}}):Play()
					tw({v = st.Stroke, t = 0.2, g = {Color = tCols['CardBorder']}}):Play()
				end
			end
		end

		for _, ctrl in ipairs(currentWindowTheme.controls or {}) do
			if ctrl.type == "dropdown" then
				if ctrl.ValueBox and ctrl.ValueBox.Parent then
					local bgCol = tCols['Function'] and tCols['Function']['Dropdown'] and tCols['Function']['Dropdown']['Value Background'] or tCols['Card']
					tw({v = ctrl.ValueBox, t = 0.2, g = {BackgroundColor3 = bgCol}}):Play()
				end
				if ctrl.Stroke and ctrl.Stroke.Parent then
					local strokeCol = tCols['Function'] and tCols['Function']['Dropdown'] and tCols['Function']['Dropdown']['Value Stroke'] or tCols['CardBorder']
					tw({v = ctrl.Stroke, t = 0.2, g = {Color = strokeCol}}):Play()
				end
				if ctrl.Popup and ctrl.Popup.Parent then
					tw({v = ctrl.Popup, t = 0.2, g = {BackgroundColor3 = tCols['Card']}}):Play()
				end
				if ctrl.Search and ctrl.Search.Parent then
					tw({v = ctrl.Search, t = 0.2, g = {BackgroundColor3 = tCols['Background']}}):Play()
				end
			elseif ctrl.type == "toggle" then
				if ctrl.GetValue and ctrl.GetValue() then
					if ctrl.Frame_1 and ctrl.Frame_1.Parent then
						tw({v = ctrl.Frame_1, t = 0.2, g = {BackgroundColor3 = tCols['Main']}}):Play()
					end
				end
			elseif ctrl.type == "textbox" or ctrl.type == "keybind" or ctrl.type == "slider_value" then
				if ctrl.Box and ctrl.Box.Parent then
					local bgCol = tCols['Function'] and tCols['Function']['Dropdown'] and tCols['Function']['Dropdown']['Value Background'] or tCols['Card']
					tw({v = ctrl.Box, t = 0.2, g = {BackgroundColor3 = bgCol}}):Play()
				end
				if ctrl.Stroke and ctrl.Stroke.Parent then
					local strokeCol = tCols['Function'] and tCols['Function']['Dropdown'] and tCols['Function']['Dropdown']['Value Stroke'] or tCols['CardBorder']
					tw({v = ctrl.Stroke, t = 0.2, g = {Color = strokeCol}}):Play()
				end
			end
		end

		if SearchFrame and SearchFrame.Parent then
			tw({v = SearchFrame, t = 0.2, g = {BackgroundColor3 = tCols['Card']}}):Play()
			tw({v = SearchStroke, t = 0.2, g = {Color = tCols['CardBorder']}}):Play()
			tw({v = SearchIcon, t = 0.2, g = {ImageColor3 = tCols['SubText']}}):Play()
			tw({v = ClearSearchBtn, t = 0.2, g = {ImageColor3 = tCols['SubText']}}):Play()
			tw({v = SearchBox, t = 0.2, g = {TextColor3 = tCols['Text & Icon'], PlaceholderColor3 = tCols['SubText']}}):Play()
			tw({v = SearchDropdown, t = 0.2, g = {BackgroundColor3 = tCols['Card']}}):Play()
			tw({v = DropStroke, t = 0.2, g = {Color = tCols['CardBorder']}}):Play()
		end
	end

	function Tabs:SetSubscription(tier, expires)
		tier = tier or Tabs.SubscriptionTier or "Free"
		expires = expires or Tabs.SubscriptionExpires or "0 seconds"
		Tabs.SubscriptionTier = tier
		Tabs.SubscriptionExpires = expires

		local tierText = string.format('<font color="rgb(180,180,195)">Subscription: </font><font color="rgb(255,255,255)"><b>%s</b></font>', tostring(tier))
		local expText = string.format('<font color="rgb(180,180,195)">Expires at: </font><font color="rgb(255,255,255)">%s</font>', tostring(expires))

		if SubTierLabelSidebar and SubTierLabelSidebar.Parent then SubTierLabelSidebar.Text = tierText end
		if SubExpiresLabelSidebar and SubExpiresLabelSidebar.Parent then SubExpiresLabelSidebar.Text = expText end
		if Tabs._subTierHero and Tabs._subTierHero.Parent then Tabs._subTierHero.Text = tierText end
		if Tabs._subExpiresHero and Tabs._subExpiresHero.Parent then Tabs._subExpiresHero.Text = expText end
	end

	Tabs._reopenPlayerLabel = ReopenPlayerLabel
	Tabs._reopenKeybind = ReopenKeybind
	Tabs._streamerToggles = {}

	function Tabs:SetStreamerMode(enabled)
		Tabs.StreamerMode = enabled
		local displayName = (LocalPlayer and LocalPlayer.DisplayName) or "Player"
		local realName = (LocalPlayer and LocalPlayer.Name) or "Player"
		local uid = (LocalPlayer and tostring(LocalPlayer.UserId)) or "78439201"

		if enabled then
			if Tabs._playerTitle and Tabs._playerTitle.Parent then Tabs._playerTitle.Text = "Welcome 👋" end
			if Tabs._profileName and Tabs._profileName.Parent then Tabs._profileName.Text = "Protected" end
			if Tabs._reopenPlayerLabel and Tabs._reopenPlayerLabel.Parent then Tabs._reopenPlayerLabel.Text = "Protected" end
			if Tabs._avatarImg and Tabs._avatarImg.Parent then Tabs._avatarImg.Image = "rbxassetid://10747373176" end
		else
			if Tabs._playerTitle and Tabs._playerTitle.Parent then Tabs._playerTitle.Text = displayName .. " 👋" end
			if Tabs._profileName and Tabs._profileName.Parent then Tabs._profileName.Text = realName end
			if Tabs._reopenPlayerLabel and Tabs._reopenPlayerLabel.Parent then Tabs._reopenPlayerLabel.Text = displayName end
			if Tabs._avatarImg and Tabs._avatarImg.Parent then
				Tabs._avatarImg.Image = (LocalPlayer and "rbxthumb://type=AvatarHeadShot&id=" .. uid .. "&w=150&h=150") or "rbxassetid://10747373176"
			end
		end

		for _, tog in ipairs(Tabs._streamerToggles) do
			if tog and tog.GetValue and tog:GetValue() ~= enabled then
				pcall(function() tog:SetValue(enabled) end)
			end
		end
	end

	local function createControlBuilder(targetScroll, meta)
		local currentSection = nil
		local sectionOrder = 1
		local B = {}

		function B:Section(secOptions)
			secOptions = (type(secOptions) == "table" and secOptions) or { Title = tostring(secOptions or "SECTION") }
			local SectionTitle = secOptions.Title or 'SECTION'
			local isMinimized = (secOptions.Minimized == true or secOptions.Minized == true or secOptions.DefaultMinimized == true) or false
			local SectionFrame = Instance.new("Frame")
			local SectionCorner = Instance.new("UICorner")
			local SectionStroke = Instance.new("UIStroke")
			local SectionHeader = Instance.new("Frame")
			local SectionTitleLabel = Instance.new("TextLabel")
			local SectionContent = Instance.new("Frame")
			local ContentList = Instance.new("UIListLayout")
			local ContentPadding = Instance.new("UIPadding")

			SectionFrame.Name = "Section_" .. SectionTitle
			SectionFrame.Parent = secOptions.Parent or targetScroll
			sectionOrder = sectionOrder + 1
			local curTrans = (currentWindowTheme and currentWindowTheme.transparency) or {}
			SectionFrame.LayoutOrder = secOptions.LayoutOrder or sectionOrder
			SectionFrame.BackgroundColor3 = curColors['Background']:Lerp(curColors['Card'], 0.35)
			SectionFrame.BackgroundTransparency = (curTrans.Section or 0) / 100
			SectionFrame.BorderSizePixel = 0
			SectionFrame.Size = isMinimized and UDim2.new(1, 0, 0, 32) or UDim2.new(1, 0, 0, 74)
			SectionFrame.ClipsDescendants = isMinimized
			if not isMinimized then
				pcall(function() SectionFrame.AutomaticSize = Enum.AutomaticSize.Y end)
			end
			registerFeature(SectionTitle, "Section", SectionFrame, meta)

			SectionCorner.CornerRadius = UDim.new(0, 8)
			SectionCorner.Parent = SectionFrame

			SectionStroke.Color = curColors['CardBorder']
			SectionStroke.Thickness = 1
			SectionStroke.Parent = SectionFrame

			SectionHeader.Name = "Header"
			SectionHeader.Parent = SectionFrame
			SectionHeader.BackgroundTransparency = 1
			SectionHeader.Size = UDim2.new(1, 0, 0, 32)

			local SectionTitleHolder = Instance.new("Frame")
			SectionTitleHolder.Name = "TitleHolder"
			SectionTitleHolder.Parent = SectionHeader
			SectionTitleHolder.BackgroundTransparency = 1
			SectionTitleHolder.BorderSizePixel = 0
			SectionTitleHolder.AnchorPoint = Vector2.new(0, 0.5)
			SectionTitleHolder.Position = UDim2.new(0, 10, 0.5, 0)
			SectionTitleHolder.Size = UDim2.new(1, -50, 0, 14)

			local SectionIndicator = Instance.new("Frame")
			local SectionIndicatorCorner = Instance.new("UICorner")

			SectionIndicator.Name = "Indicator"
			SectionIndicator.Parent = SectionTitleHolder
			SectionIndicator.AnchorPoint = Vector2.new(0, 0.5)
			SectionIndicator.BackgroundColor3 = curColors['Main']
			SectionIndicator.BorderSizePixel = 0
			SectionIndicator.Position = UDim2.new(0, 0, 0.5, 0)
			SectionIndicator.Size = UDim2.new(0, 3, 0, 10)

			SectionIndicatorCorner.CornerRadius = UDim.new(0, 2)
			SectionIndicatorCorner.Parent = SectionIndicator

			SectionTitleLabel.Name = "Title"
			SectionTitleLabel.Parent = SectionTitleHolder
			SectionTitleLabel.AnchorPoint = Vector2.new(0, 0.5)
			SectionTitleLabel.BackgroundTransparency = 1
			SectionTitleLabel.Position = UDim2.new(0, 12, 0.5, -1)
			SectionTitleLabel.Size = UDim2.new(1, -12, 1, 0)
			SectionTitleLabel.Font = Enum.Font.BuilderSansBold
			SectionTitleLabel.Text = SectionTitle:upper()
			SectionTitleLabel.TextColor3 = curColors['SubText']
			SectionTitleLabel.TextSize = 13.5
			SectionTitleLabel.TextXAlignment = Enum.TextXAlignment.Left
			SectionTitleLabel.TextYAlignment = Enum.TextYAlignment.Center

			local MinBtn = Instance.new("ImageLabel")
			MinBtn.Name = "Minimize"
			MinBtn.Parent = SectionHeader
			MinBtn.AnchorPoint = Vector2.new(1, 0.5)
			MinBtn.BackgroundTransparency = 1
			MinBtn.Position = UDim2.new(1, -12, 0.5, 0)
			MinBtn.Size = UDim2.new(0, 14, 0, 14)
			MinBtn.Image = "rbxassetid://10709791437"
			MinBtn.ImageColor3 = curColors['SubText']
			MinBtn.Rotation = isMinimized and 0 or 90
			MinBtn.ZIndex = 6
			if secOptions.HideMinimize then
				MinBtn.Visible = false
			end

			SectionContent.Name = "Content"
			SectionContent.Parent = SectionFrame
			SectionContent.BackgroundTransparency = 1
			SectionContent.Position = UDim2.new(0, 0, 0, 32)
			SectionContent.Size = isMinimized and UDim2.new(1, 0, 0, 0) or UDim2.new(1, 0, 0, 42)
			SectionContent.ClipsDescendants = isMinimized
			SectionContent.Visible = not isMinimized
			if not isMinimized then
				pcall(function() SectionContent.AutomaticSize = Enum.AutomaticSize.Y end)
			end

			ContentList.Parent = SectionContent
			ContentList.SortOrder = Enum.SortOrder.LayoutOrder
			ContentList.Padding = UDim.new(0, 5)

			ContentPadding.Parent = SectionContent
			ContentPadding.PaddingBottom = UDim.new(0, 8)
			ContentPadding.PaddingLeft = UDim.new(0, 8)
			ContentPadding.PaddingRight = UDim.new(0, 8)
			ContentPadding.PaddingTop = UDim.new(0, 2)

			local function getContentHeight()
				local acs = ContentList.AbsoluteContentSize.Y
				if acs and acs > 0 then
					return acs + 14
				end
				local total = 0
				local count = 0
				for _, child in ipairs(SectionContent:GetChildren()) do
					if child:IsA("GuiObject") and child.Visible ~= false then
						local h = child.Size.Y.Offset
						if h <= 0 then
							local bg = child:FindFirstChild("Background")
							if bg and bg:IsA("GuiObject") and bg.Size.Y.Offset > 0 then
								h = bg.Size.Y.Offset
							else
								h = 40
							end
						end
						total = total + h
						count = count + 1
					end
				end
				if count > 0 then
					total = total + (count - 1) * 5 + 14
				end
				return math.max(42, total)
			end

			local function updateSectionHeight()
				if isMinimized then
					pcall(function() SectionFrame.AutomaticSize = Enum.AutomaticSize.None end)
					pcall(function() SectionContent.AutomaticSize = Enum.AutomaticSize.None end)
					SectionFrame.ClipsDescendants = true
					SectionContent.Visible = false
					SectionFrame.Size = UDim2.new(1, 0, 0, 32)
				else
					SectionContent.Visible = true
					SectionFrame.ClipsDescendants = false
					SectionContent.ClipsDescendants = false
					local ch = getContentHeight()
					SectionContent.Size = UDim2.new(1, 0, 0, ch)
					SectionFrame.Size = UDim2.new(1, 0, 0, 32 + ch)
					pcall(function() SectionContent.AutomaticSize = Enum.AutomaticSize.Y end)
					pcall(function() SectionFrame.AutomaticSize = Enum.AutomaticSize.Y end)
				end
			end

			local function toggleMinimize()
				isMinimized = not isMinimized
				tw({v = MinBtn, t = 0.2, s = Enum.EasingStyle.Cubic, d = "Out", g = {Rotation = isMinimized and 0 or 90}}):Play()
				if isMinimized then
					pcall(function() SectionFrame.AutomaticSize = Enum.AutomaticSize.None end)
					pcall(function() SectionContent.AutomaticSize = Enum.AutomaticSize.None end)
					SectionFrame.ClipsDescendants = true
					local twClose = tw({v = SectionFrame, t = 0.22, s = Enum.EasingStyle.Cubic, d = "Out", g = {Size = UDim2.new(1, 0, 0, 32)}})
					twClose.Completed:Connect(function()
						if isMinimized then
							SectionContent.Visible = false
						end
					end)
					twClose:Play()
				else
					SectionContent.Visible = true
					SectionFrame.ClipsDescendants = false
					SectionContent.ClipsDescendants = false
					local ch = getContentHeight()
					SectionContent.Size = UDim2.new(1, 0, 0, ch)
					local twOpen = tw({v = SectionFrame, t = 0.22, s = Enum.EasingStyle.Cubic, d = "Out", g = {Size = UDim2.new(1, 0, 0, 32 + ch)}})
					twOpen.Completed:Connect(function()
						if not isMinimized then
							pcall(function() SectionContent.AutomaticSize = Enum.AutomaticSize.Y end)
							pcall(function() SectionFrame.AutomaticSize = Enum.AutomaticSize.Y end)
						end
					end)
					twOpen:Play()
				end
			end

			local function getSharedMinBtn()
				if type(secOptions.SharedMinBtn) == "function" then
					return secOptions.SharedMinBtn()
				end
				return secOptions.SharedMinBtn
			end

			local HeaderClick = click(SectionHeader)
			HeaderClick.MouseButton1Click:Connect(function()
				if secOptions.CustomToggle then
					secOptions.CustomToggle()
				else
					toggleMinimize()
				end
			end)
			HeaderClick.MouseEnter:Connect(function()
				local c = currentWindowTheme.colors or curColors
				if MinBtn.Visible then
					tw({v = MinBtn, t = 0.15, g = {ImageColor3 = c['Main']}}):Play()
				end
				local sharedBtn = getSharedMinBtn()
				if sharedBtn and sharedBtn.Parent and sharedBtn.Visible then
					tw({v = sharedBtn, t = 0.15, g = {ImageColor3 = c['Main']}}):Play()
				end
				tw({v = SectionTitleLabel, t = 0.15, g = {TextColor3 = c['Text & Icon']}}):Play()
			end)
			HeaderClick.MouseLeave:Connect(function()
				local c = currentWindowTheme.colors or curColors
				if MinBtn.Visible then
					tw({v = MinBtn, t = 0.15, g = {ImageColor3 = isMinimized and c['SubText'] or c['Text & Icon']}}):Play()
				end
				local sharedBtn = getSharedMinBtn()
				if sharedBtn and sharedBtn.Parent and sharedBtn.Visible then
					tw({v = sharedBtn, t = 0.15, g = {ImageColor3 = isMinimized and c['SubText'] or c['Text & Icon']}}):Play()
				end
				tw({v = SectionTitleLabel, t = 0.15, g = {TextColor3 = c['SubText']}}):Play()
			end)

			ContentList:GetPropertyChangedSignal("AbsoluteContentSize"):Connect(function()
				if not isMinimized then updateSectionHeight() end
			end)

			SectionContent.ChildAdded:Connect(function(child)
				if child:IsA("GuiObject") then
					child:GetPropertyChangedSignal("Size"):Connect(function()
						if not isMinimized then updateSectionHeight() end
					end)
					child:GetPropertyChangedSignal("Visible"):Connect(function()
						if not isMinimized then updateSectionHeight() end
					end)
				end
				if not isMinimized then updateSectionHeight() end
			end)

			SectionContent.ChildRemoved:Connect(function()
				if not isMinimized then updateSectionHeight() end
			end)

			targetScroll:GetPropertyChangedSignal("Visible"):Connect(function()
				if targetScroll.Visible and not isMinimized then
					updateSectionHeight()
					task.defer(updateSectionHeight)
					task.delay(0.08, updateSectionHeight)
				end
			end)

			if targetScroll.Parent and targetScroll.Parent:IsA("GuiObject") then
				targetScroll.Parent:GetPropertyChangedSignal("Visible"):Connect(function()
					if targetScroll.Parent.Visible and not isMinimized then
						updateSectionHeight()
						task.defer(updateSectionHeight)
						task.delay(0.08, updateSectionHeight)
					end
				end)
			end

			updateSectionHeight()
			task.delay(0.05, updateSectionHeight)
			task.delay(0.15, updateSectionHeight)
			task.delay(0.35, updateSectionHeight)

			currentSection = SectionContent

			local secData = {
				Frame = SectionFrame,
				Stroke = SectionStroke,
				Title = SectionTitleLabel,
				Indicator = SectionIndicator,
				MinBtn = MinBtn,
				TitleText = SectionTitle,
				Scroll = targetScroll,
				UpdateHeight = updateSectionHeight,
				IsMinimized = function() return isMinimized end,
				SetMinimized = function(state)
					if state and not isMinimized then
						toggleMinimize()
					elseif not state and isMinimized then
						toggleMinimize()
					end
				end
			}
			table.insert(currentWindowTheme.sections, secData)

			local SecObj = setmetatable({}, {
				__index = function(_, k)
					if B[k] then
						return function(self, ...)
							currentSection = SectionContent
							return B[k](B, ...)
						end
					end
				end
			})
			SecObj.Frame = SectionFrame
			SecObj.MinBtn = MinBtn
			SecObj.ToggleMinimize = toggleMinimize
			function SecObj:SetTitle(t) SectionTitleLabel.Text = tostring(t):upper() end
			function SecObj:Minimize() if not isMinimized then toggleMinimize() end end
			function SecObj:Expand() if isMinimized then toggleMinimize() end end
			function SecObj:IsMinimized() return isMinimized end
			return SecObj
		end

		function B:DualSection(leftOptions, rightOptions)
			leftOptions = (type(leftOptions) == "table" and leftOptions) or { Title = tostring(leftOptions or "Left") }
			rightOptions = (type(rightOptions) == "table" and rightOptions) or { Title = tostring(rightOptions or "Right") }

			local isDualMinimized = (leftOptions.Minimized == true or leftOptions.Minized == true or leftOptions.DefaultMinimized == true
				or rightOptions.Minimized == true or rightOptions.Minized == true or rightOptions.DefaultMinimized == true) or false

			local RowFrame = Instance.new("Frame")
			RowFrame.Name = "DualSectionRow_" .. (leftOptions.Title or "Left") .. "_" .. (rightOptions.Title or "Right")
			RowFrame.Parent = targetScroll
			sectionOrder = sectionOrder + 1
			RowFrame.LayoutOrder = leftOptions.LayoutOrder or rightOptions.LayoutOrder or sectionOrder
			RowFrame.BackgroundTransparency = 1
			RowFrame.BorderSizePixel = 0
			RowFrame.Size = UDim2.new(1, 0, 0, 0)
			RowFrame.ClipsDescendants = false
			pcall(function() RowFrame.AutomaticSize = Enum.AutomaticSize.Y end)

			local RowList = Instance.new("UIListLayout")
			RowList.Parent = RowFrame
			RowList.FillDirection = Enum.FillDirection.Horizontal
			RowList.HorizontalAlignment = Enum.HorizontalAlignment.Left
			RowList.VerticalAlignment = Enum.VerticalAlignment.Top
			RowList.SortOrder = Enum.SortOrder.LayoutOrder
			RowList.Padding = UDim.new(0, 8)

			local LeftCol = Instance.new("Frame")
			LeftCol.Name = "LeftColumn"
			LeftCol.Parent = RowFrame
			LeftCol.LayoutOrder = 1
			LeftCol.BackgroundTransparency = 1
			LeftCol.BorderSizePixel = 0
			LeftCol.Size = UDim2.new(0.5, -4, 0, 0)
			LeftCol.ClipsDescendants = false
			pcall(function() LeftCol.AutomaticSize = Enum.AutomaticSize.Y end)

			local RightCol = Instance.new("Frame")
			RightCol.Name = "RightColumn"
			RightCol.Parent = RowFrame
			RightCol.LayoutOrder = 2
			RightCol.BackgroundTransparency = 1
			RightCol.BorderSizePixel = 0
			RightCol.Size = UDim2.new(0.5, -4, 0, 0)
			RightCol.ClipsDescendants = false
			pcall(function() RightCol.AutomaticSize = Enum.AutomaticSize.Y end)

			local LeftSec = nil
			local RightSec = nil

			local function toggleDual()
				isDualMinimized = not isDualMinimized
				if LeftSec then
					if isDualMinimized then LeftSec:Minimize() else LeftSec:Expand() end
				end
				if RightSec then
					if isDualMinimized then RightSec:Minimize() else RightSec:Expand() end
				end
			end

			local leftOpts = {}
			for k, v in pairs(leftOptions) do leftOpts[k] = v end
			leftOpts.Parent = LeftCol
			leftOpts.Minimized = isDualMinimized
			leftOpts.HideMinimize = true
			leftOpts.CustomToggle = toggleDual
			leftOpts.SharedMinBtn = function() return RightSec and RightSec.MinBtn end

			local rightOpts = {}
			for k, v in pairs(rightOptions) do rightOpts[k] = v end
			rightOpts.Parent = RightCol
			rightOpts.Minimized = isDualMinimized
			rightOpts.HideMinimize = false
			rightOpts.CustomToggle = toggleDual

			LeftSec = B:Section(leftOpts)
			RightSec = B:Section(rightOpts)

			return LeftSec, RightSec
		end

		function B:Toggle(p)
			local Value = p.Value or false
			local Image = p.Image or ''
			local Callback = p.Callback or function() end
			local ToggleTitle = p.Title or 'Toggle'
			local Desc = p.Desc or ''

			local parent = currentSection or targetScroll
			local Toggle, Config = background(parent, ToggleTitle, Desc, Image, 'Toggle')
			registerFeature(ToggleTitle, "Toggle", Toggle, meta)

			local F_1 = Instance.new("Frame")
			local Frame_1 = Instance.new("Frame")
			local UICorner_2 = Instance.new("UICorner")
			local Frame_2 = Instance.new("Frame")
			local UICorner_3 = Instance.new("UICorner")

			F_1.Name = "F"
			F_1.Parent = Toggle
			F_1.AnchorPoint = Vector2.new(1, 0.5)
			F_1.BackgroundTransparency = 1
			F_1.Position = UDim2.new(1, -12, 0.5, 0)
			F_1.Size = UDim2.new(0, 40, 0, 22)

			Frame_1.Name = "SwitchPill"
			Frame_1.Parent = F_1
			Frame_1.AnchorPoint = Vector2.new(0.5, 0.5)
			Frame_1.Position = UDim2.new(0.5, 0, 0.5, 0)
			Frame_1.BackgroundColor3 = Value and curColors['Main'] or Color3.fromRGB(36, 36, 46)
			Frame_1.BorderSizePixel = 0
			Frame_1.Size = UDim2.new(1, 0, 0, 22)

			UICorner_2.CornerRadius = UDim.new(1, 0)
			UICorner_2.Parent = Frame_1

			Frame_2.Name = "Knob"
			Frame_2.Parent = Frame_1
			Frame_2.AnchorPoint = Vector2.new(0, 0.5)
			Frame_2.BackgroundColor3 = Value and Color3.fromRGB(15, 15, 18) or Color3.fromRGB(160, 160, 170)
			Frame_2.BorderSizePixel = 0
			Frame_2.Position = Value and UDim2.new(1, -19, 0.5, 0) or UDim2.new(0, 3, 0.5, 0)
			Frame_2.Size = UDim2.new(0, 16, 0, 16)
			Frame_2.ZIndex = 3

			UICorner_3.CornerRadius = UDim.new(1, 0)
			UICorner_3.Parent = Frame_2

			table.insert(currentWindowTheme.controls, {
				type = "toggle",
				Frame_1 = Frame_1,
				Frame_2 = Frame_2,
				GetValue = function() return Value end
			})

			local Click = click(Toggle)

			local function setToggleState(state, skipCallback)
				Value = state
				if Value then
					Config:SetTextTransparencyTitle(0)
					local c = currentWindowTheme.colors or curColors
					tw({v = Frame_1, t = 0.18, g = {BackgroundColor3 = c['Main']}}):Play()
					tw({v = Frame_2, t = 0.18, g = {BackgroundColor3 = Color3.fromRGB(15, 15, 18), Position = UDim2.new(1, -19, 0.5, 0)}}):Play()
				else
					Config:SetTextTransparencyTitle(0.3)
					tw({v = Frame_1, t = 0.18, g = {BackgroundColor3 = Color3.fromRGB(36, 36, 46)}}):Play()
					tw({v = Frame_2, t = 0.18, g = {BackgroundColor3 = Color3.fromRGB(160, 160, 170), Position = UDim2.new(0, 3, 0.5, 0)}}):Play()
				end
				if not skipCallback then
					pcall(Callback, Value)
				end
			end

			Click.MouseButton1Click:Connect(function() setToggleState(not Value) end)
			task.delay(0.05, function() setToggleState(Value, true) end)

			local New = {}
			function New:SetTitle(t) Config:SetTitle(t) end
			function New:SetDesc(t) Config:SetDesc(t) end
			function New:SetVisible(t) Toggle.Visible = t end
			function New:SetValue(t, skip) setToggleState(t, skip) end
			function New:GetValue() return Value end

			if not p.IgnoreConfig then
				local flagKey = p.Flag or ToggleTitle
				table.insert(ConfigElements, {
					Type = "Toggle",
					Title = ToggleTitle,
					Flag = flagKey,
					Default = (p.Value ~= nil and p.Value or false),
					Get = function() return New:GetValue() end,
					Set = function(v) New:SetValue(v) end
				})
			end

			return New
		end

		function B:Button(p)
			local BtnTitle = p.Title or 'Button'
			local Desc = p.Desc or ''
			local Image = p.Image or ''
			local Callback = p.Callback or function() end

			local parent = currentSection or targetScroll
			local Button, Config = background(parent, BtnTitle, Desc, Image, 'Button')
			registerFeature(BtnTitle, "Button", Button, meta)
			Config:SetTextTransparencyTitle(0)
			Config:SetSizeT(42)

			local ArrowIcon = Instance.new("ImageLabel")
			ArrowIcon.Name = "Arrow"
			ArrowIcon.Parent = Button
			ArrowIcon.AnchorPoint = Vector2.new(1, 0.5)
			ArrowIcon.BackgroundTransparency = 1
			ArrowIcon.Position = UDim2.new(1, -12, 0.5, 0)
			ArrowIcon.Size = UDim2.new(0, 18, 0, 18)
			ArrowIcon.Image = "rbxassetid://10709791437"
			ArrowIcon.ImageColor3 = curColors['SubText']

			local Click = click(Button)
			Click.MouseEnter:Connect(function()
				local c = currentWindowTheme.colors or curColors
				tw({v = ArrowIcon, t = 0.15, g = {Position = UDim2.new(1, -8, 0.5, 0), ImageColor3 = c['Main']}}):Play()
				tw({v = Button, t = 0.15, g = {BackgroundColor3 = c['Card']:Lerp(c['Main'], 0.08)}}):Play()
			end)
			Click.MouseLeave:Connect(function()
				local c = currentWindowTheme.colors or curColors
				tw({v = ArrowIcon, t = 0.15, g = {Position = UDim2.new(1, -12, 0.5, 0), ImageColor3 = c['SubText']}}):Play()
				tw({v = Button, t = 0.15, g = {BackgroundColor3 = c['Card']}}):Play()
			end)
			Click.MouseButton1Click:Connect(function()
				jc(Click, Button)
				local c = currentWindowTheme.colors or curColors
				tw({v = Button, t = 0.08, g = {BackgroundColor3 = c['Card']:Lerp(c['Main'], 0.18)}}):Play()
				task.delay(0.08, function()
					local cNow = currentWindowTheme.colors or curColors
					tw({v = Button, t = 0.15, g = {BackgroundColor3 = cNow['Card']}}):Play()
				end)
				pcall(Callback)
			end)

			local New = {}
			function New:SetTitle(t) Config:SetTitle(t) end
			function New:SetDesc(t) Config:SetDesc(t) end
			function New:SetVisible(t) Button.Visible = t end
			return New
		end

		function B:Slider(p)
			local SliderTitle = p.Title or 'Slider'
			local Desc = p.Desc or ''
			local Image = p.Image or ''
			local Min = p.Min or 0
			local Max = p.Max or 100
			local Value = p.Value or Min
			local Rounding = p.Rounding or 0
			local Callback = p.Callback or function() end

			local parent = currentSection or targetScroll
			local Slider, Config = background(parent, SliderTitle, Desc, Image, 'Slider')
			registerFeature(SliderTitle, "Slider", Slider, meta)
			Config:SetTextTransparencyTitle(0)
			Config:SetSizeT(185)

			local F = Instance.new("Frame")
			local SliderTrack = Instance.new("Frame")
			local TrackCorner = Instance.new("UICorner")
			local SliderFill = Instance.new("Frame")
			local FillCorner = Instance.new("UICorner")
			local Knob = Instance.new("Frame")
			local KnobCorner = Instance.new("UICorner")
			local ValueBox = Instance.new("TextBox")
			local ValueBoxCorner = Instance.new("UICorner")
			local ValueBoxStroke = Instance.new("UIStroke")

			F.Name = "SliderControls"
			F.Parent = Slider
			F.AnchorPoint = Vector2.new(1, 0.5)
			F.BackgroundTransparency = 1
			F.Position = UDim2.new(1, -12, 0.5, 0)
			F.Size = UDim2.new(0, 168, 0, 24)

			SliderTrack.Name = "Track"
			SliderTrack.Parent = F
			SliderTrack.AnchorPoint = Vector2.new(0, 0.5)
			SliderTrack.BackgroundColor3 = Color3.fromRGB(54, 54, 68)
			SliderTrack.BorderSizePixel = 0
			SliderTrack.Position = UDim2.new(0, 0, 0.5, 0)
			SliderTrack.Size = UDim2.new(0, 114, 0, 5)

			TrackCorner.CornerRadius = UDim.new(1, 0)
			TrackCorner.Parent = SliderTrack

			local TrackStroke = Instance.new("UIStroke")
			TrackStroke.Color = Color3.fromRGB(68, 68, 86)
			TrackStroke.Thickness = 1
			TrackStroke.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
			TrackStroke.Parent = SliderTrack

			Slider.MouseEnter:Connect(function()
				tw({v = SliderTrack, t = 0.15, g = {BackgroundColor3 = Color3.fromRGB(66, 66, 82)}}):Play()
				tw({v = TrackStroke, t = 0.15, g = {Color = Color3.fromRGB(82, 82, 102)}}):Play()
			end)
			Slider.MouseLeave:Connect(function()
				tw({v = SliderTrack, t = 0.2, g = {BackgroundColor3 = Color3.fromRGB(54, 54, 68)}}):Play()
				tw({v = TrackStroke, t = 0.2, g = {Color = Color3.fromRGB(68, 68, 86)}}):Play()
			end)

			SliderFill.Name = "Fill"
			SliderFill.Parent = SliderTrack
			SliderFill.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
			SliderFill.BorderSizePixel = 0
			SliderFill.Size = UDim2.new(0, 0, 1, 0)
			SliderFill.Active = false

			FillCorner.CornerRadius = UDim.new(1, 0)
			FillCorner.Parent = SliderFill

			Knob.Name = "Knob"
			Knob.Parent = SliderFill
			Knob.AnchorPoint = Vector2.new(0.5, 0.5)
			Knob.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
			Knob.BorderSizePixel = 0
			Knob.Position = UDim2.new(1, 0, 0.5, 0)
			Knob.Size = UDim2.new(0, 15, 0, 15)
			Knob.ZIndex = 4
			Knob.Active = false

			KnobCorner.CornerRadius = UDim.new(1, 0)
			KnobCorner.Parent = Knob

			ValueBox.Name = "ValueBox"
			ValueBox.Parent = F
			ValueBox.AnchorPoint = Vector2.new(1, 0.5)
			ValueBox.BackgroundColor3 = curColors['Function']['Dropdown']['Value Background'] or Color3.fromRGB(15, 15, 18)
			ValueBox.BorderSizePixel = 0
			ValueBox.Position = UDim2.new(1, 0, 0.5, 0)
			ValueBox.Size = UDim2.new(0, 42, 0, 22)
			ValueBox.Font = Enum.Font.BuilderSansMedium
			ValueBox.Text = tostring(Value)
			ValueBox.TextColor3 = curColors['Text & Icon']
			ValueBox.TextSize = 13

			ValueBoxCorner.CornerRadius = UDim.new(0, 4)
			ValueBoxCorner.Parent = ValueBox

			ValueBoxStroke.Color = curColors['Function']['Dropdown']['Value Stroke'] or Color3.fromRGB(44, 44, 54)
			ValueBoxStroke.Thickness = 1
			ValueBoxStroke.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
			ValueBoxStroke.Parent = ValueBox

			table.insert(currentWindowTheme.controls, {
				type = "slider_value",
				Box = ValueBox,
				Stroke = ValueBoxStroke,
			})

			local function roundVal(v, r)
				local factor = 10 ^ r
				return math.floor(v * factor + 0.5) / factor
			end

			local function updateSlider(v, skipCallback)
				v = math.clamp(v, Min, Max)
				v = roundVal(v, Rounding)
				Value = v
				local progress = (Max > Min) and ((v - Min) / (Max - Min)) or 0
				tw({v = SliderFill, t = 0.08, g = {Size = UDim2.new(math.clamp(progress, 0, 1), 0, 1, 0)}}):Play()
				ValueBox.Text = tostring(v)
				if not skipCallback then
					pcall(Callback, v)
				end
			end

			updateSlider(Value, true)

			ValueBox.FocusLost:Connect(function()
				local n = tonumber(ValueBox.Text) or Min
				updateSlider(n)
			end)

			local dragging = false
			local SliderHitbox = Instance.new("TextButton")
			SliderHitbox.Name = "SliderHitbox"
			SliderHitbox.Parent = F
			SliderHitbox.AnchorPoint = Vector2.new(0, 0.5)
			SliderHitbox.BackgroundTransparency = 1
			SliderHitbox.BorderSizePixel = 0
			SliderHitbox.Position = UDim2.new(0, -10, 0.5, 0)
			SliderHitbox.Size = UDim2.new(0, 134, 0, 24)
			SliderHitbox.ZIndex = 15
			SliderHitbox.Text = ""
			SliderHitbox.AutoButtonColor = false

			local function move(input)
				local rel = math.clamp((input.Position.X - SliderTrack.AbsolutePosition.X) / SliderTrack.AbsoluteSize.X, 0, 1)
				local n = rel * (Max - Min) + Min
				updateSlider(n)
			end

			SliderHitbox.InputBegan:Connect(function(input)
				if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
					dragging = true
					move(input)
				end
			end)

			U.InputEnded:Connect(function(input)
				if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
					dragging = false
				end
			end)

			U.InputChanged:Connect(function(input)
				if dragging and (input.UserInputType == Enum.UserInputType.MouseMovement or input.UserInputType == Enum.UserInputType.Touch) then
					move(input)
				end
			end)

			local New = {}
			function New:SetTitle(t) Config:SetTitle(t) end
			function New:SetDesc(t) Config:SetDesc(t) end
			function New:SetVisible(t) Slider.Visible = t end
			function New:SetValue(t, skip) updateSlider(t, skip) end
			function New:SetMin(t)
				Min = tonumber(t) or Min
				if Value < Min then updateSlider(Min) else updateSlider(Value, true) end
			end
			function New:SetMax(t)
				Max = tonumber(t) or Max
				if Value > Max then updateSlider(Max) else updateSlider(Value, true) end
			end
			function New:GetMin() return Min end
			function New:GetMax() return Max end
			function New:GetValue() return Value end

			if not p.IgnoreConfig then
				local flagKey = p.Flag or SliderTitle
				table.insert(ConfigElements, {
					Type = "Slider",
					Title = SliderTitle,
					Flag = flagKey,
					Default = (p.Value ~= nil and p.Value or Min),
					Get = function() return New:GetValue() end,
					Set = function(v) New:SetValue(tonumber(v) or v) end
				})
			end

			return New
		end

		function B:Dropdown(p)
			local DropdownTitle = p.Title or 'Dropdown'
			local Desc = p.Desc or ''
			local Image = p.Image or ''
			local List = p.List or {}
			local Value = p.Value or (p.Multi and {} or List[1])
			local Multi = p.Multi or false
			local Callback = p.Callback or function() end

			local parent = currentSection or targetScroll
			local Dropdown, Config = background(parent, DropdownTitle, Desc, Image, 'Dropdown')
			registerFeature(DropdownTitle, "Dropdown", Dropdown, meta)
			Config:SetTextTransparencyTitle(0)
			Config:SetSizeT(140)

			local DropdownSelect = addDropdownSelect(Dropdown, Dropdown, Multi, Callback, Value, List)

			local New = {}
			function New:SetTitle(t) Config:SetTitle(t) end
			function New:SetDesc(t) Config:SetDesc(t) end
			function New:SetVisible(t) Dropdown.Visible = t end
			function New:SetValue(t, skip) DropdownSelect:SetValue(t, skip) end
			function New:GetValue() return DropdownSelect:GetValue() end
			function New:Add(t) DropdownSelect:Add(t) end
			function New:Clear(t) DropdownSelect:Clear(t) end
			function New:Refresh(newList, defaultVal)
				DropdownSelect:Clear()
				if type(newList) == "table" then
					for _, item in ipairs(newList) do
						DropdownSelect:Add(item)
					end
					if defaultVal ~= nil then
						DropdownSelect:SetValue(defaultVal, true)
					end
				end
			end

			if not p.IgnoreConfig then
				local flagKey = p.Flag or DropdownTitle
				table.insert(ConfigElements, {
					Type = "Dropdown",
					Title = DropdownTitle,
					Flag = flagKey,
					Default = (p.Value ~= nil and p.Value or (List and List[1]) or ""),
					Get = function() return New:GetValue() end,
					Set = function(v) New:SetValue(v) end
				})
			end

			return New
		end

		function B:Textbox(p)
			local TextboxTitle = p.Title or 'Textbox'
			local Desc = p.Desc or ''
			local Image = p.Image or ''
			local Value = p.Value or ''
			local Placeholder = p.Placeholder or 'Input text...'
			local ClearText = (p.ClearText ~= nil and p.ClearText) or (p.ClearTextOnFocus ~= nil and p.ClearTextOnFocus) or false
			local Callback = p.Callback or function() end

			local parent = currentSection or targetScroll
			local Textbox, Config = background(parent, TextboxTitle, Desc, Image, 'Textbox')
			registerFeature(TextboxTitle, "Input", Textbox, meta)
			Config:SetTextTransparencyTitle(0)
			Config:SetSizeT(140)

			local F = Instance.new("Frame")
			local InputBox = Instance.new("TextBox")
			local BoxCorner = Instance.new("UICorner")
			local BoxStroke = Instance.new("UIStroke")

			F.Name = "F"
			F.Parent = Textbox
			F.AnchorPoint = Vector2.new(1, 0.5)
			F.BackgroundTransparency = 1
			F.Position = UDim2.new(1, -12, 0.5, 0)
			F.Size = UDim2.new(0, 120, 0, 22)
			F.ClipsDescendants = false

			InputBox.Name = "Input"
			InputBox.Parent = F
			InputBox.BackgroundColor3 = curColors['Function']['Dropdown']['Value Background'] or Color3.fromRGB(15, 15, 18)
			InputBox.BorderSizePixel = 0
			InputBox.Size = UDim2.new(1, 0, 1, 0)
			InputBox.Font = Enum.Font.BuilderSansMedium
			InputBox.PlaceholderColor3 = curColors['SubText']
			InputBox.PlaceholderText = Placeholder
			InputBox.Text = Value
			InputBox.TextColor3 = curColors['Text & Icon']
			InputBox.TextSize = 13
			InputBox.ClearTextOnFocus = ClearText
			InputBox.TextXAlignment = Enum.TextXAlignment.Left
			InputBox.TextYAlignment = Enum.TextYAlignment.Center

			local BoxPadding = Instance.new("UIPadding")
			BoxPadding.Parent = InputBox
			BoxPadding.PaddingLeft = UDim.new(0, 8)
			BoxPadding.PaddingRight = UDim.new(0, 8)

			BoxCorner.CornerRadius = UDim.new(0, 4)
			BoxCorner.Parent = InputBox

			BoxStroke.Color = curColors['Function']['Dropdown']['Value Stroke'] or Color3.fromRGB(44, 44, 54)
			BoxStroke.Thickness = 1
			BoxStroke.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
			BoxStroke.Parent = InputBox

			table.insert(currentWindowTheme.controls, {
				type = "textbox",
				Box = InputBox,
				Stroke = BoxStroke,
			})

			InputBox.FocusLost:Connect(function() pcall(Callback, InputBox.Text) end)

			local New = {}
			function New:SetTitle(t) Config:SetTitle(t) end
			function New:SetDesc(t) Config:SetDesc(t) end
			function New:SetVisible(t) Textbox.Visible = t end
			function New:SetValue(t, skip)
				InputBox.Text = tostring(t or "")
				if not skip then
					pcall(Callback, InputBox.Text)
				end
			end
			function New:SetClearTextOnFocus(t) InputBox.ClearTextOnFocus = not not t end
			function New:SetPlaceholderText(t) InputBox.PlaceholderText = tostring(t) end
			function New:GetValue() return InputBox.Text end

			if not p.IgnoreConfig then
				local flagKey = p.Flag or TextboxTitle
				table.insert(ConfigElements, {
					Type = "Textbox",
					Title = TextboxTitle,
					Flag = flagKey,
					Default = (p.Value ~= nil and tostring(p.Value) or ""),
					Get = function() return New:GetValue() end,
					Set = function(v) New:SetValue(v) end
				})
			end

			return New
		end

		function B:Keybind(p)
			local KeyTitle = p.Title or 'Keybind'
			local Desc = p.Desc or ''
			local Image = p.Image or ''
			local Key = p.Key or Enum.KeyCode.E
			local Callback = p.Callback or function() end

			local parent = currentSection or targetScroll
			local Keybind, Config = background(parent, KeyTitle, Desc, Image, 'Keybind')
			registerFeature(KeyTitle, "Keybind", Keybind, meta)
			Config:SetSizeT(115)

			local KeyButton = Instance.new("TextButton")
			local KeyCorner = Instance.new("UICorner")
			local KeyStroke = Instance.new("UIStroke")
			local KeyPadding = Instance.new("UIPadding")
			local KeyConstraint = Instance.new("UISizeConstraint")

			KeyButton.Name = "KeyBtn"
			KeyButton.Parent = Keybind
			KeyButton.AnchorPoint = Vector2.new(1, 0.5)
			KeyButton.BackgroundColor3 = curColors['Function']['Dropdown']['Value Background'] or Color3.fromRGB(15, 15, 18)
			KeyButton.BorderSizePixel = 0
			KeyButton.Position = UDim2.new(1, -12, 0.5, 0)
			KeyButton.AutomaticSize = Enum.AutomaticSize.X
			KeyButton.Size = UDim2.new(0, 0, 0, 22)
			KeyButton.Font = Enum.Font.BuilderSansMedium
			KeyButton.Text = tostring(Key):gsub("Enum.KeyCode.", "")
			KeyButton.TextColor3 = curColors['Text & Icon']
			KeyButton.TextSize = 13

			KeyConstraint.Parent = KeyButton
			KeyConstraint.MinSize = Vector2.new(38, 22)

			KeyPadding.Parent = KeyButton
			KeyPadding.PaddingLeft = UDim.new(0, 8)
			KeyPadding.PaddingRight = UDim.new(0, 8)

			KeyCorner.CornerRadius = UDim.new(0, 4)
			KeyCorner.Parent = KeyButton

			KeyStroke.Color = curColors['Function']['Dropdown']['Value Stroke'] or Color3.fromRGB(44, 44, 54)
			KeyStroke.Thickness = 1
			KeyStroke.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
			KeyStroke.Parent = KeyButton

			table.insert(currentWindowTheme.controls, {
				type = "keybind",
				Box = KeyButton,
				Stroke = KeyStroke,
			})

			local listening = false
			KeyButton.MouseButton1Click:Connect(function()
				listening = true
				KeyButton.Text = "..."
				tw({v = KeyStroke, t = 0.15, g = {Color = Color3.fromRGB(255, 255, 255)}}):Play()
				local conn
				conn = U.InputBegan:Connect(function(input)
					if input.UserInputType == Enum.UserInputType.Keyboard then
						Key = input.KeyCode
						KeyButton.Text = tostring(Key):gsub("Enum.KeyCode.", "")
						tw({v = KeyStroke, t = 0.15, g = {Color = curColors['Function']['Dropdown']['Value Stroke'] or Color3.fromRGB(44, 44, 54)}}):Play()
						conn:Disconnect()
						listening = false
						pcall(Callback, Key)
					end
				end)
			end)

			local New = {}
			function New:SetTitle(t) Config:SetTitle(t) end
			function New:SetDesc(t) Config:SetDesc(t) end
			function New:SetVisible(t) Keybind.Visible = t end
			function New:SetKey(t)
				if typeof(t) == "EnumItem" then
					Key = t
				elseif type(t) == "string" then
					local kname = t:gsub("Enum.KeyCode.", "")
					if Enum.KeyCode[kname] then
						Key = Enum.KeyCode[kname]
					end
				end
				KeyButton.Text = tostring(Key):gsub("Enum.KeyCode.", "")
				pcall(Callback, Key)
			end
			function New:GetKey() return Key end
			function New:SetValue(t) New:SetKey(t) end
			function New:GetValue() return tostring(Key):gsub("Enum.KeyCode.", "") end

			if not p.IgnoreConfig then
				local flagKey = p.Flag or KeyTitle
				table.insert(ConfigElements, {
					Type = "Keybind",
					Title = KeyTitle,
					Flag = flagKey,
					Default = (p.Key ~= nil and tostring(p.Key):gsub("Enum.KeyCode.", "") or "E"),
					Get = function() return New:GetValue() end,
					Set = function(v) New:SetValue(v) end
				})
			end

			return New
		end

		function B:ColorPicker(p)
			local CPTitle = p.Title or 'Color'
			local Desc = p.Desc or ''
			local Image = p.Image or ''
			local Value = p.Value or Color3.fromRGB(255, 255, 255)
			local Callback = p.Callback or function() end

			local parent = currentSection or targetScroll
			local ColorPicker, Config = background(parent, CPTitle, Desc, Image, 'Color Picker')
			registerFeature(CPTitle, "Color", ColorPicker, meta)
			Config:SetTextTransparencyTitle(0)
			Config:SetSizeT(50)

			local Preview = Instance.new("Frame")
			local PrevCorner = Instance.new("UICorner")
			local PrevStroke = Instance.new("UIStroke")

			Preview.Name = "ColorPreview"
			Preview.Parent = ColorPicker
			Preview.AnchorPoint = Vector2.new(1, 0.5)
			Preview.BackgroundColor3 = Value
			Preview.BorderSizePixel = 0
			Preview.Position = UDim2.new(1, -12, 0.5, 0)
			Preview.Size = UDim2.new(0, 24, 0, 20)

			PrevCorner.CornerRadius = UDim.new(0, 4)
			PrevCorner.Parent = Preview

			PrevStroke.Color = curColors['CardBorder']
			PrevStroke.Thickness = 1
			PrevStroke.Parent = Preview

			local ColorpickBar = Instance.new("Frame")
			local CPBarCorner = Instance.new("UICorner")
			local CPBarStroke = Instance.new("UIStroke")
			local Color_1 = Instance.new("ImageLabel")
			local ColorSelection_1 = Instance.new("ImageLabel")
			local Hue_1 = Instance.new("ImageLabel")
			local HueSelection_1 = Instance.new("ImageLabel")

			ColorpickBar.Name = "ColorpickBar"
			ColorpickBar.Parent = ScreenGui
			ColorpickBar.BackgroundColor3 = curColors['Card']
			ColorpickBar.BorderSizePixel = 0
			ColorpickBar.Size = UDim2.new(0, 185, 0, 110)
			ColorpickBar.Visible = false
			ColorpickBar.ZIndex = 90

			CPBarCorner.CornerRadius = UDim.new(0, 6)
			CPBarCorner.Parent = ColorpickBar

			CPBarStroke.Color = Color3.fromRGB(70, 70, 80)
			CPBarStroke.Thickness = 1
			CPBarStroke.Parent = ColorpickBar

			Color_1.Name = "Color"
			Color_1.Parent = ColorpickBar
			Color_1.Position = UDim2.new(0, 8, 0, 8)
			Color_1.Size = UDim2.new(0, 115, 0, 94)
			Color_1.Image = "rbxassetid://4155801252"
			Color_1.ZIndex = 91

			local ColorCorner = Instance.new("UICorner")
			ColorCorner.CornerRadius = UDim.new(0, 4)
			ColorCorner.Parent = Color_1

			ColorSelection_1.Name = "ColorSelection"
			ColorSelection_1.Parent = Color_1
			ColorSelection_1.AnchorPoint = Vector2.new(0.5, 0.5)
			ColorSelection_1.BackgroundTransparency = 1
			ColorSelection_1.Size = UDim2.new(0, 10, 0, 10)
			ColorSelection_1.Image = "rbxassetid://4805639000"
			ColorSelection_1.ZIndex = 92

			Hue_1.Name = "Hue"
			Hue_1.Parent = ColorpickBar
			Hue_1.Position = UDim2.new(0, 130, 0, 8)
			Hue_1.Size = UDim2.new(0, 16, 0, 94)
			Hue_1.ZIndex = 91

			local HueCorner = Instance.new("UICorner")
			HueCorner.CornerRadius = UDim.new(0, 4)
			HueCorner.Parent = Hue_1

			local HueGrad = Instance.new("UIGradient")
			HueGrad.Color = ColorSequence.new{
				ColorSequenceKeypoint.new(0, Color3.fromRGB(255, 0, 0)),
				ColorSequenceKeypoint.new(0.17, Color3.fromRGB(255, 255, 0)),
				ColorSequenceKeypoint.new(0.33, Color3.fromRGB(0, 255, 0)),
				ColorSequenceKeypoint.new(0.5, Color3.fromRGB(0, 255, 255)),
				ColorSequenceKeypoint.new(0.67, Color3.fromRGB(0, 0, 255)),
				ColorSequenceKeypoint.new(0.83, Color3.fromRGB(255, 0, 255)),
				ColorSequenceKeypoint.new(1, Color3.fromRGB(255, 0, 0))
			}
			HueGrad.Rotation = 90
			HueGrad.Parent = Hue_1

			HueSelection_1.Name = "HueSelection"
			HueSelection_1.Parent = Hue_1
			HueSelection_1.AnchorPoint = Vector2.new(0.5, 0.5)
			HueSelection_1.BackgroundTransparency = 1
			HueSelection_1.Position = UDim2.new(0.5, 0, 0, 0)
			HueSelection_1.Size = UDim2.new(1, 4, 0, 4)
			HueSelection_1.Image = "rbxassetid://4805639000"
			HueSelection_1.ZIndex = 92

			local ColorH, ColorS, ColorV = Color3.toHSV(Value)
			Color_1.BackgroundColor3 = Color3.fromHSV(ColorH, 1, 1)
			ColorSelection_1.Position = UDim2.new(ColorS, 0, 1 - ColorV, 0)
			HueSelection_1.Position = UDim2.new(0.5, 0, ColorH, 0)

			local isOpen = false
			local draggingColor = false
			local draggingHue = false

			local function updateColor()
				local c = Color3.fromHSV(ColorH, ColorS, ColorV)
				Value = c
				Preview.BackgroundColor3 = c
				Color_1.BackgroundColor3 = Color3.fromHSV(ColorH, 1, 1)
				pcall(Callback, c)
			end

			local ClickCP = click(ColorPicker)
			ClickCP.MouseButton1Click:Connect(function()
				isOpen = not isOpen
				if isOpen then
					local targetX = Preview.AbsolutePosition.X - ColorpickBar.Parent.AbsolutePosition.X - 160
					local targetY = Preview.AbsolutePosition.Y - ColorpickBar.Parent.AbsolutePosition.Y + 26
					ColorpickBar.Position = UDim2.new(0, math.max(10, targetX), 0, math.max(10, targetY))
					ColorpickBar.Visible = true
				else
					ColorpickBar.Visible = false
				end
			end)

			Color_1.InputBegan:Connect(function(input)
				if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
					draggingColor = true
					local relX = math.clamp((input.Position.X - Color_1.AbsolutePosition.X) / Color_1.AbsoluteSize.X, 0, 1)
					local relY = math.clamp((input.Position.Y - Color_1.AbsolutePosition.Y) / Color_1.AbsoluteSize.Y, 0, 1)
					ColorSelection_1.Position = UDim2.new(relX, 0, relY, 0)
					ColorS = relX
					ColorV = 1 - relY
					updateColor()
				end
			end)

			Hue_1.InputBegan:Connect(function(input)
				if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
					draggingHue = true
					local relY = math.clamp((input.Position.Y - Hue_1.AbsolutePosition.Y) / Hue_1.AbsoluteSize.Y, 0, 1)
					HueSelection_1.Position = UDim2.new(0.5, 0, relY, 0)
					ColorH = relY
					updateColor()
				end
			end)

			U.InputEnded:Connect(function(input)
				if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
					draggingColor = false
					draggingHue = false
				end
			end)

			U.InputChanged:Connect(function(input)
				if draggingColor and (input.UserInputType == Enum.UserInputType.MouseMovement or input.UserInputType == Enum.UserInputType.Touch) then
					local relX = math.clamp((input.Position.X - Color_1.AbsolutePosition.X) / Color_1.AbsoluteSize.X, 0, 1)
					local relY = math.clamp((input.Position.Y - Color_1.AbsolutePosition.Y) / Color_1.AbsoluteSize.Y, 0, 1)
					ColorSelection_1.Position = UDim2.new(relX, 0, relY, 0)
					ColorS = relX
					ColorV = 1 - relY
					updateColor()
				elseif draggingHue and (input.UserInputType == Enum.UserInputType.MouseMovement or input.UserInputType == Enum.UserInputType.Touch) then
					local relY = math.clamp((input.Position.Y - Hue_1.AbsolutePosition.Y) / Hue_1.AbsoluteSize.Y, 0, 1)
					HueSelection_1.Position = UDim2.new(0.5, 0, relY, 0)
					ColorH = relY
					updateColor()
				end
			end)

			U.InputBegan:Connect(function(input)
				if (input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch) and isOpen then
					local bPos = ColorpickBar.AbsolutePosition
					local bSize = ColorpickBar.AbsoluteSize
					local mX, mY = input.Position.X, input.Position.Y
					if mX < bPos.X or mX > bPos.X + bSize.X or mY < bPos.Y - 24 or mY > bPos.Y + bSize.Y then
						ColorpickBar.Visible = false
						isOpen = false
					end
				end
			end)

			local New = {}
			function New:SetTitle(t) Config:SetTitle(t) end
			function New:SetDesc(t) Config:SetDesc(t) end
			function New:SetVisible(t) ColorPicker.Visible = t end
			function New:SetValue(c)
				if typeof(c) == "Color3" then
					Value = c
					ColorH, ColorS, ColorV = Color3.toHSV(Value)
					ColorSelection_1.Position = UDim2.new(ColorS, 0, 1 - ColorV, 0)
					HueSelection_1.Position = UDim2.new(0.5, 0, ColorH, 0)
					updateColor()
				elseif type(c) == "table" then
					local r = c.R or c[1]
					local g = c.G or c[2]
					local b = c.B or c[3]
					if r and g and b then
						if r > 1 or g > 1 or b > 1 then
							Value = Color3.fromRGB(math.clamp(r, 0, 255), math.clamp(g, 0, 255), math.clamp(b, 0, 255))
						else
							Value = Color3.new(math.clamp(r, 0, 1), math.clamp(g, 0, 1), math.clamp(b, 0, 1))
						end
						ColorH, ColorS, ColorV = Color3.toHSV(Value)
						ColorSelection_1.Position = UDim2.new(ColorS, 0, 1 - ColorV, 0)
						HueSelection_1.Position = UDim2.new(0.5, 0, ColorH, 0)
						updateColor()
					end
				end
			end
			function New:GetValue()
				return { R = math.floor(Value.R * 255), G = math.floor(Value.G * 255), B = math.floor(Value.B * 255) }
			end

			if not p.IgnoreConfig then
				local flagKey = p.Flag or CPTitle
				table.insert(ConfigElements, {
					Type = "ColorPicker",
					Title = CPTitle,
					Flag = flagKey,
					Default = (p.Value and (p.Value.R and Color3.new(p.Value.R, p.Value.G, p.Value.B) or p.Value) or Color3.fromRGB(255, 255, 255)),
					Get = function() return New:GetValue() end,
					Set = function(v) New:SetValue(v) end
				})
			end

			return New
		end

		function B:Label(p)
			local LabelTitle = p.Title or 'Label'
			local Desc = p.Desc or ''
			local Image = p.Image or ''

			local parent = currentSection or targetScroll
			local Label, Config = background(parent, LabelTitle, Desc, Image, 'Label')
			registerFeature(LabelTitle, "Label", Label, meta)
			Config:SetTextTransparencyTitle(0)

			local New = {}
			function New:SetTitle(t) Config:SetTitle(t) end
			function New:SetDesc(t) Config:SetDesc(t) end
			function New:SetVisible(t) Label.Visible = t end
			return New
		end

		function B:Code(p)
			local Title = p.Title or 'Script / Code'
			local CodeText = p.Code or '-- print("Aether Studios")'
			local parent = currentSection or targetScroll

			local CodeFrame = Instance.new("Frame")
			local UICorner_1 = Instance.new("UICorner")
			local Stroke = Instance.new("UIStroke")
			local Top_1 = Instance.new("Frame")
			local Title_L = Instance.new("TextLabel")
			local CopyBtn = Instance.new("TextButton")
			local CodeScroll = Instance.new("ScrollingFrame")
			local CodeLabel = Instance.new("TextLabel")

			CodeFrame.Name = "CodeBox"
			CodeFrame.Parent = parent
			CodeFrame.BackgroundColor3 = Color3.fromRGB(15, 15, 18)
			CodeFrame.Size = UDim2.new(1, 0, 0, 110)
			CodeFrame.ClipsDescendants = true
			registerFeature(Title or "Code", "Code", CodeFrame, meta)

			UICorner_1.CornerRadius = UDim.new(0, 6)
			UICorner_1.Parent = CodeFrame

			Stroke.Color = curColors['CardBorder']
			Stroke.Thickness = 1
			Stroke.Parent = CodeFrame

			Top_1.Parent = CodeFrame
			Top_1.BackgroundColor3 = curColors['Card']
			Top_1.Size = UDim2.new(1, 0, 0, 24)

			Title_L.Parent = Top_1
			Title_L.BackgroundTransparency = 1
			Title_L.Position = UDim2.new(0, 8, 0, 0)
			Title_L.Size = UDim2.new(0.5, 0, 1, 0)
			Title_L.Font = Enum.Font.BuilderSansBold
			Title_L.Text = tostring(Title)
			Title_L.TextColor3 = curColors['Text & Icon']
			Title_L.TextSize = 12
			Title_L.TextXAlignment = Enum.TextXAlignment.Left

			CopyBtn.Parent = Top_1
			CopyBtn.AnchorPoint = Vector2.new(1, 0.5)
			CopyBtn.BackgroundTransparency = 1
			CopyBtn.Position = UDim2.new(1, -8, 0.5, 0)
			CopyBtn.Size = UDim2.new(0, 42, 1, 0)
			CopyBtn.Font = Enum.Font.BuilderSansBold
			CopyBtn.Text = "COPY"
			CopyBtn.TextColor3 = Color3.fromRGB(255, 255, 255)
			CopyBtn.TextSize = 11

			CodeScroll.Parent = CodeFrame
			CodeScroll.BackgroundTransparency = 1
			CodeScroll.Position = UDim2.new(0, 8, 0, 28)
			CodeScroll.Size = UDim2.new(1, -16, 1, -32)
			CodeScroll.ScrollBarThickness = 2
			CodeScroll.ScrollBarImageColor3 = Color3.fromRGB(255, 255, 255)

			CodeLabel.Parent = CodeScroll
			CodeLabel.BackgroundTransparency = 1
			CodeLabel.Font = Enum.Font.RobotoMono
			CodeLabel.Text = CodeText
			CodeLabel.TextColor3 = curColors['SubText']
			CodeLabel.TextSize = 10.5
			CodeLabel.TextXAlignment = Enum.TextXAlignment.Left
			CodeLabel.TextYAlignment = Enum.TextYAlignment.Top
			CodeLabel.AutomaticSize = Enum.AutomaticSize.XY

			CopyBtn.MouseButton1Click:Connect(function()
				if setclipboard then setclipboard(CodeText) end
				CopyBtn.Text = "COPIED"
				task.wait(1)
				CopyBtn.Text = "COPY"
			end)

			local New = {}
			function New:SetTitle(t) Title_L.Text = tostring(t) end
			function New:SetCode(t)
				CodeText = t
				CodeLabel.Text = t
			end
			return New
		end

		function B:CreateLogger(p)
			local TitleText = p.Title or "SYSTEM LOGS"
			local BoxSize = p.Size or UDim2.new(1, 0, 0, 160)
			local parent = currentSection or targetScroll

			local Background = Instance.new("Frame")
			local LogCorner = Instance.new("UICorner")
			local LogStroke = Instance.new("UIStroke")
			local TopBar = Instance.new("Frame")
			local TitleLabel = Instance.new("TextLabel")
			local ActionButtons = Instance.new("Frame")
			local UIListLayout_Actions = Instance.new("UIListLayout")
			local ClearBtn = Instance.new("TextButton")
			local CopyBtn = Instance.new("TextButton")
			local LogContainer = Instance.new("ScrollingFrame")
			local UIListLayout_Logs = Instance.new("UIListLayout")
			local UIPadding_Logs = Instance.new("UIPadding")

			Background.Name = "LoggerBox"
			Background.Parent = parent
			Background.BackgroundColor3 = Color3.fromRGB(15, 15, 18)
			Background.Size = BoxSize
			Background.ClipsDescendants = true
			registerFeature(TitleText, "Logger", Background, meta)

			LogCorner.CornerRadius = UDim.new(0, 6)
			LogCorner.Parent = Background

			LogStroke.Color = curColors['CardBorder']
			LogStroke.Thickness = 1
			LogStroke.Parent = Background

			TopBar.Name = "TopBar"
			TopBar.Parent = Background
			TopBar.BackgroundColor3 = curColors['Card']
			TopBar.Size = UDim2.new(1, 0, 0, 24)

			TitleLabel.Parent = TopBar
			TitleLabel.BackgroundTransparency = 1
			TitleLabel.Position = UDim2.new(0, 8, 0, 0)
			TitleLabel.Size = UDim2.new(0.5, 0, 1, 0)
			TitleLabel.Font = Enum.Font.BuilderSansBold
			TitleLabel.Text = TitleText:upper()
			TitleLabel.TextColor3 = curColors['Text & Icon']
			TitleLabel.TextSize = 12
			TitleLabel.TextXAlignment = Enum.TextXAlignment.Left

			ActionButtons.Parent = TopBar
			ActionButtons.BackgroundTransparency = 1
			ActionButtons.Size = UDim2.new(0.5, -8, 1, 0)
			ActionButtons.Position = UDim2.new(0.5, 0, 0, 0)

			UIListLayout_Actions.Parent = ActionButtons
			UIListLayout_Actions.FillDirection = Enum.FillDirection.Horizontal
			UIListLayout_Actions.HorizontalAlignment = Enum.HorizontalAlignment.Right
			UIListLayout_Actions.VerticalAlignment = Enum.VerticalAlignment.Center
			UIListLayout_Actions.Padding = UDim.new(0, 8)

			local function makeActionBtn(btn, text)
				btn.Parent = ActionButtons
				btn.BackgroundTransparency = 1
				btn.Size = UDim2.new(0, 36, 1, 0)
				btn.Font = Enum.Font.BuilderSansBold
				btn.Text = text
				btn.TextColor3 = curColors['SubText']
				btn.TextSize = 11
				return btn
			end

			makeActionBtn(ClearBtn, "CLEAR")
			makeActionBtn(CopyBtn, "COPY")

			LogContainer.Name = "LogContainer"
			LogContainer.Parent = Background
			LogContainer.BackgroundTransparency = 1
			LogContainer.Position = UDim2.new(0, 0, 0, 25)
			LogContainer.Size = UDim2.new(1, 0, 1, -25)
			LogContainer.ScrollBarThickness = 2
			LogContainer.ScrollBarImageColor3 = Color3.fromRGB(255, 255, 255)
			LogContainer.AutomaticCanvasSize = Enum.AutomaticSize.Y

			UIListLayout_Logs.Parent = LogContainer
			UIListLayout_Logs.SortOrder = Enum.SortOrder.LayoutOrder
			UIListLayout_Logs.Padding = UDim.new(0, 2)

			UIPadding_Logs.Parent = LogContainer
			UIPadding_Logs.PaddingLeft = UDim.new(0, 8)
			UIPadding_Logs.PaddingTop = UDim.new(0, 4)

			local Logger = {}
			function Logger:Log(message, color)
				local LogLabel = Instance.new("TextLabel")
				LogLabel.Parent = LogContainer
				LogLabel.BackgroundTransparency = 1
				LogLabel.Size = UDim2.new(1, -10, 0, 14)
				LogLabel.Font = Enum.Font.RobotoMono
				LogLabel.Text = string.format("[%s] %s", os.date("%X"), tostring(message))
				LogLabel.TextColor3 = color or curColors['SubText']
				LogLabel.TextSize = 9.5
				LogLabel.TextXAlignment = Enum.TextXAlignment.Left
				LogLabel.RichText = true

				task.defer(function()
					local contentHeight = UIListLayout_Logs.AbsoluteContentSize.Y + 8
					local windowHeight = LogContainer.AbsoluteWindowSize.Y
					if contentHeight > windowHeight then
						LogContainer.CanvasPosition = Vector2.new(0, contentHeight - windowHeight)
					else
						LogContainer.CanvasPosition = Vector2.new(0, 0)
					end
				end)
			end

			function Logger:Clear()
				for _, item in pairs(LogContainer:GetChildren()) do
					if item:IsA("TextLabel") then item:Destroy() end
				end
				LogContainer.CanvasPosition = Vector2.new(0, 0)
			end

			ClearBtn.MouseButton1Click:Connect(function() Logger:Clear() end)
			CopyBtn.MouseButton1Click:Connect(function()
				local fullLog = ""
				for _, item in pairs(LogContainer:GetChildren()) do
					if item:IsA("TextLabel") then fullLog = fullLog .. item.Text .. "\n" end
				end
				if setclipboard then setclipboard(fullLog) end
				CopyBtn.Text = "DONE"
				task.wait(1)
				CopyBtn.Text = "COPY"
			end)

			return Logger
		end

		function B:Image(p)
			local ImageId = (type(p) == "table" and (p.Image or p.Id)) or (type(p) == "string" and p) or 'rbxassetid://111362591084511'
			local ImageHeight = (type(p) == "table" and p.Height) or 130
			local parent = currentSection or targetScroll

			local ImageLogo = Instance.new("ImageLabel")
			local UICorner_1 = Instance.new("UICorner")
			ImageLogo.Name = "ImageBanner"
			ImageLogo.Parent = parent
			ImageLogo.BackgroundTransparency = 1
			ImageLogo.Size = UDim2.new(1, 0, 0, ImageHeight)
			ImageLogo.Image = ImageId
			ImageLogo.ScaleType = Enum.ScaleType.Crop
			UICorner_1.CornerRadius = UDim.new(0, 6)
			UICorner_1.Parent = ImageLogo
			return ImageLogo
		end

		function B:PricingCards(cardsData)
			local parent = currentSection or targetScroll
			local Container = Instance.new("Frame")
			local RowLayout = Instance.new("UIListLayout")
			local RowPadding = Instance.new("UIPadding")

			Container.Name = "PricingCardsRow"
			Container.Parent = parent
			Container.BackgroundTransparency = 1
			Container.BorderSizePixel = 0
			Container.Size = UDim2.new(1, 0, 0, 235)
			Container.ClipsDescendants = false

			RowLayout.Parent = Container
			RowLayout.FillDirection = Enum.FillDirection.Horizontal
			RowLayout.HorizontalAlignment = Enum.HorizontalAlignment.Center
			RowLayout.SortOrder = Enum.SortOrder.LayoutOrder
			RowLayout.Padding = UDim.new(0, 8)

			RowPadding.Parent = Container
			RowPadding.PaddingLeft = UDim.new(0, 2)
			RowPadding.PaddingRight = UDim.new(0, 2)
			RowPadding.PaddingTop = UDim.new(0, 2)
			RowPadding.PaddingBottom = UDim.new(0, 4)

			local totalCards = #cardsData
			for idx, cData in ipairs(cardsData) do
				local CardWrapper = Instance.new("Frame")
				CardWrapper.Name = "CardWrapper_" .. (cData.Title or idx)
				CardWrapper.Parent = Container
				CardWrapper.LayoutOrder = idx
				CardWrapper.BackgroundTransparency = 1
				CardWrapper.BorderSizePixel = 0
				CardWrapper.Size = UDim2.new(1 / totalCards, - math.floor((totalCards - 1) * 8 / totalCards), 1, 0)
				CardWrapper.ClipsDescendants = false

				local Card = Instance.new("Frame")
				local CardCorner = Instance.new("UICorner")
				local CardStroke = Instance.new("UIStroke")
				local CardPadding = Instance.new("UIPadding")
				local CardLayout = Instance.new("UIListLayout")

				Card.Name = "PricingCard_" .. (cData.Title or idx)
				Card.Parent = CardWrapper
				Card.BackgroundColor3 = Color3.fromRGB(18, 18, 22)
				Card.BorderSizePixel = 0
				Card.Size = UDim2.new(1, 0, 1, 0)
				Card.ZIndex = 2
				Card.ClipsDescendants = true

				CardCorner.CornerRadius = UDim.new(0, 8)
				CardCorner.Parent = Card

				CardStroke.Thickness = 1
				CardStroke.Color = cData.BorderColor or curColors['CardBorder']
				CardStroke.Parent = Card

				if cData.Glow then
					CardStroke.Thickness = 1.5
					CardStroke.Color = Color3.fromRGB(255, 215, 0)

					local StrokeGrad = Instance.new("UIGradient")
					StrokeGrad.Color = ColorSequence.new({
						ColorSequenceKeypoint.new(0, Color3.fromRGB(255, 255, 235)),
						ColorSequenceKeypoint.new(0.2, Color3.fromRGB(250, 204, 21)),
						ColorSequenceKeypoint.new(0.5, Color3.fromRGB(160, 115, 10)),
						ColorSequenceKeypoint.new(0.8, Color3.fromRGB(250, 204, 21)),
						ColorSequenceKeypoint.new(1, Color3.fromRGB(255, 255, 235)),
					})
					StrokeGrad.Transparency = NumberSequence.new({
						NumberSequenceKeypoint.new(0, 0),
						NumberSequenceKeypoint.new(0.22, 0.35),
						NumberSequenceKeypoint.new(0.5, 0.7),
						NumberSequenceKeypoint.new(0.78, 0.35),
						NumberSequenceKeypoint.new(1, 0),
					})
					StrokeGrad.Parent = CardStroke

					task.spawn(function()
						while Card and Card.Parent do
							StrokeGrad.Rotation = (StrokeGrad.Rotation + 2.5) % 360
							task.wait(0.03)
						end
					end)
				end

				CardPadding.Parent = Card
				CardPadding.PaddingTop = UDim.new(0, 10)
				CardPadding.PaddingBottom = UDim.new(0, 10)
				CardPadding.PaddingLeft = UDim.new(0, 10)
				CardPadding.PaddingRight = UDim.new(0, 10)

				CardLayout.Parent = Card
				CardLayout.FillDirection = Enum.FillDirection.Vertical
				CardLayout.SortOrder = Enum.SortOrder.LayoutOrder
				CardLayout.Padding = UDim.new(0, 6)

				local HeaderFrame = Instance.new("Frame")
				local HeaderLayout = Instance.new("UIListLayout")
				local TitleLabel = Instance.new("TextLabel")

				HeaderFrame.Name = "Header"
				HeaderFrame.Parent = Card
				HeaderFrame.LayoutOrder = 1
				HeaderFrame.BackgroundTransparency = 1
				HeaderFrame.Size = UDim2.new(1, 0, 0, 22)

				HeaderLayout.Parent = HeaderFrame
				HeaderLayout.FillDirection = Enum.FillDirection.Horizontal
				HeaderLayout.HorizontalAlignment = Enum.HorizontalAlignment.Center
				HeaderLayout.VerticalAlignment = Enum.VerticalAlignment.Center
				HeaderLayout.SortOrder = Enum.SortOrder.LayoutOrder
				HeaderLayout.Padding = UDim.new(0, 8)

				TitleLabel.Name = "Title"
				TitleLabel.Parent = HeaderFrame
				TitleLabel.LayoutOrder = 1
				TitleLabel.BackgroundTransparency = 1
				TitleLabel.Size = UDim2.new(0, 0, 0, 20)
				TitleLabel.AutomaticSize = Enum.AutomaticSize.X
				TitleLabel.Font = Enum.Font.BuilderSansBold
				TitleLabel.Text = (cData.Title or "TIER"):upper()
				TitleLabel.TextColor3 = cData.TextColor or curColors['Text & Icon']
				TitleLabel.TextSize = 16
				TitleLabel.TextXAlignment = Enum.TextXAlignment.Left
				TitleLabel.TextYAlignment = Enum.TextYAlignment.Center

				if cData.Badge then
					local BadgeFrame = Instance.new("Frame")
					local BadgeCorner = Instance.new("UICorner")
					local BadgePad = Instance.new("UIPadding")
					local BadgeText = Instance.new("TextLabel")

					BadgeFrame.Name = "Badge"
					BadgeFrame.Parent = HeaderFrame
					BadgeFrame.LayoutOrder = 2
					BadgeFrame.BackgroundColor3 = (cData.AccentColor or curColors['Main']):Lerp(Color3.fromRGB(0, 0, 0), 0.65)
					BadgeFrame.BorderSizePixel = 0
					BadgeFrame.Size = UDim2.new(0, 0, 0, 20)
					BadgeFrame.AutomaticSize = Enum.AutomaticSize.X

					BadgeCorner.CornerRadius = UDim.new(0, 5)
					BadgeCorner.Parent = BadgeFrame

					BadgePad.Parent = BadgeFrame
					BadgePad.PaddingLeft = UDim.new(0, 7)
					BadgePad.PaddingRight = UDim.new(0, 7)

					BadgeText.Name = "BadgeText"
					BadgeText.Parent = BadgeFrame
					BadgeText.BackgroundTransparency = 1
					BadgeText.Size = UDim2.new(1, 0, 1, 0)
					BadgeText.Font = Enum.Font.BuilderSansBold
					BadgeText.Text = tostring(cData.Badge):upper()
					BadgeText.TextColor3 = cData.AccentColor or curColors['Main']
					BadgeText.TextSize = 11.5
					BadgeText.TextYAlignment = Enum.TextYAlignment.Center
				end

				if type(cData.Subtitle) == "table" or (type(cData.Subtitle) == "string" and string.find(cData.Subtitle, "/")) then
					local SubFrame = Instance.new("Frame")
					local SubLayout = Instance.new("UIListLayout")

					SubFrame.Name = "SubtitleRow"
					SubFrame.Parent = Card
					SubFrame.LayoutOrder = 2
					SubFrame.BackgroundTransparency = 1
					SubFrame.Size = UDim2.new(1, 0, 0, 18)

					SubLayout.Parent = SubFrame
					SubLayout.FillDirection = Enum.FillDirection.Horizontal
					SubLayout.HorizontalAlignment = Enum.HorizontalAlignment.Center
					SubLayout.VerticalAlignment = Enum.VerticalAlignment.Center
					SubLayout.SortOrder = Enum.SortOrder.LayoutOrder
					SubLayout.Padding = UDim.new(0, 7)

					local items = (type(cData.Subtitle) == "table" and cData.Subtitle) or {}
					if type(cData.Subtitle) == "string" then
						for part in string.gmatch(cData.Subtitle, "[^/]+") do
							table.insert(items, (part:gsub("^%s*(.-)%s*$", "%1")))
						end
					end

					for sIdx, sText in ipairs(items) do
						local itemLabel = Instance.new("TextLabel")
						itemLabel.Name = "Item_" .. sIdx
						itemLabel.Parent = SubFrame
						itemLabel.LayoutOrder = sIdx * 2 - 1
						itemLabel.BackgroundTransparency = 1
						itemLabel.Size = UDim2.new(0, 0, 1, 0)
						itemLabel.AutomaticSize = Enum.AutomaticSize.X
						itemLabel.Font = Enum.Font.BuilderSansBold
						itemLabel.Text = sText
						itemLabel.TextColor3 = cData.SubtitleColor or cData.TextColor or curColors['SubText']
						itemLabel.TextSize = 13
						itemLabel.TextYAlignment = Enum.TextYAlignment.Center

						if sIdx < #items then
							local vDiv = Instance.new("Frame")
							vDiv.Name = "VDivider_" .. sIdx
							vDiv.Parent = SubFrame
							vDiv.LayoutOrder = sIdx * 2
							vDiv.BackgroundColor3 = Color3.fromRGB(55, 55, 68)
							vDiv.BorderSizePixel = 0
							vDiv.Size = UDim2.new(0, 1, 0, 10)
						end
					end
				else
					local SubLabel = Instance.new("TextLabel")
					SubLabel.Name = "Subtitle"
					SubLabel.Parent = Card
					SubLabel.LayoutOrder = 2
					SubLabel.BackgroundTransparency = 1
					SubLabel.Size = UDim2.new(1, 0, 0, 18)
					SubLabel.Font = Enum.Font.BuilderSansMedium
					SubLabel.Text = cData.Subtitle or ""
					SubLabel.TextColor3 = cData.SubtitleColor or cData.TextColor or curColors['SubText']
					SubLabel.TextSize = 13
					SubLabel.TextXAlignment = Enum.TextXAlignment.Center
					SubLabel.TextYAlignment = Enum.TextYAlignment.Center
				end

				local Divider = Instance.new("Frame")
				Divider.Name = "Divider"
				Divider.Parent = Card
				Divider.LayoutOrder = 3
				Divider.BackgroundColor3 = Color3.fromRGB(36, 36, 44)
				Divider.BorderSizePixel = 0
				Divider.Size = UDim2.new(1, 0, 0, 1)

				local FeaturesFrame = Instance.new("Frame")
				local FeaturesLayout = Instance.new("UIListLayout")

				FeaturesFrame.Name = "Features"
				FeaturesFrame.Parent = Card
				FeaturesFrame.LayoutOrder = 4
				FeaturesFrame.BackgroundTransparency = 1
				FeaturesFrame.Size = UDim2.new(1, 0, 0, cData.ButtonText and 115 or 145)

				FeaturesLayout.Parent = FeaturesFrame
				FeaturesLayout.SortOrder = Enum.SortOrder.LayoutOrder
				FeaturesLayout.Padding = UDim.new(0, 4)

				if type(cData.Features) == "table" then
					for fIdx, fText in ipairs(cData.Features) do
						local featRow = Instance.new("Frame")
						local featCheck = Instance.new("ImageLabel")
						local featLabel = Instance.new("TextLabel")

						featRow.Name = "Feat_" .. fIdx
						featRow.Parent = FeaturesFrame
						featRow.BackgroundTransparency = 1
						featRow.Size = UDim2.new(1, 0, 0, 20)
						featRow.LayoutOrder = fIdx

						local checkData = gl("check")
						featCheck.Name = "Check"
						featCheck.Parent = featRow
						featCheck.BackgroundTransparency = 1
						featCheck.AnchorPoint = Vector2.new(0, 0.5)
						featCheck.Position = UDim2.new(0, 0, 0.5, 0)
						featCheck.Size = UDim2.new(0, 16, 0, 16)
						featCheck.Image = checkData.Image
						featCheck.ImageRectSize = checkData.ImageRectSize
						featCheck.ImageRectOffset = checkData.ImageRectPosition
						featCheck.ImageColor3 = Color3.fromRGB(255, 255, 255)

						local cleanText = tostring(fText):gsub("^✔️%s*", "")
						featLabel.Name = "Label"
						featLabel.Parent = featRow
						featLabel.BackgroundTransparency = 1
						featLabel.Position = UDim2.new(0, 22, 0, 0)
						featLabel.Size = UDim2.new(1, -22, 1, 0)
						featLabel.Font = Enum.Font.BuilderSansMedium
						featLabel.Text = cleanText
						featLabel.TextColor3 = cData.TextColor or curColors['Text & Icon']
						featLabel.TextSize = 13.5
						featLabel.TextTruncate = Enum.TextTruncate.AtEnd
						featLabel.TextXAlignment = Enum.TextXAlignment.Left
					end
				else
					local DescLabel = Instance.new("TextLabel")
					DescLabel.Name = "Description"
					DescLabel.Parent = FeaturesFrame
					DescLabel.BackgroundTransparency = 1
					DescLabel.Size = UDim2.new(1, 0, 1, 0)
					DescLabel.Font = Enum.Font.BuilderSansMedium
					DescLabel.Text = cData.Desc or ""
					DescLabel.TextColor3 = cData.TextColor or curColors['Text & Icon']
					DescLabel.TextSize = 13
					DescLabel.TextWrapped = true
					DescLabel.TextXAlignment = Enum.TextXAlignment.Left
					DescLabel.TextYAlignment = Enum.TextYAlignment.Top
				end

				if cData.ButtonText then
					local ActionBtn = Instance.new("Frame")
					local ActionBtnCorner = Instance.new("UICorner")
					local ActionBtnStroke = Instance.new("UIStroke")
					local ActionBtnLabel = Instance.new("TextLabel")
					local ActionBtnClick = click(ActionBtn)

					ActionBtn.Name = "ActionButton"
					ActionBtn.Parent = Card
					ActionBtn.LayoutOrder = 5
					ActionBtn.BackgroundColor3 = cData.ButtonBg or Color3.fromRGB(24, 24, 30)
					ActionBtn.BorderSizePixel = 0
					ActionBtn.Size = UDim2.new(1, 0, 0, 30)

					ActionBtnCorner.CornerRadius = UDim.new(0, 6)
					ActionBtnCorner.Parent = ActionBtn

					ActionBtnStroke.Thickness = 1
					ActionBtnStroke.Color = cData.ButtonStroke or cData.AccentColor or curColors['CardBorder']
					ActionBtnStroke.Parent = ActionBtn

					ActionBtnLabel.Name = "Label"
					ActionBtnLabel.Parent = ActionBtn
					ActionBtnLabel.BackgroundTransparency = 1
					ActionBtnLabel.Size = UDim2.new(1, 0, 1, 0)
					ActionBtnLabel.Font = Enum.Font.BuilderSansBold
					ActionBtnLabel.Text = cData.ButtonText
					ActionBtnLabel.TextColor3 = cData.ButtonTextColor or cData.TextColor or curColors['Text & Icon']
					ActionBtnLabel.TextSize = 13

					ActionBtnClick.MouseEnter:Connect(function()
						tw({v = ActionBtn, t = 0.15, g = {BackgroundColor3 = (cData.ButtonBg or Color3.fromRGB(24, 24, 30)):Lerp(Color3.fromRGB(255, 255, 255), 0.15)}}):Play()
					end)
					ActionBtnClick.MouseLeave:Connect(function()
						tw({v = ActionBtn, t = 0.15, g = {BackgroundColor3 = cData.ButtonBg or Color3.fromRGB(24, 24, 30)}}):Play()
					end)
					ActionBtnClick.MouseButton1Click:Connect(function()
						jc(ActionBtn, ActionBtn)
						if cData.Callback then cData.Callback() end
					end)
				end

				table.insert(currentWindowTheme.cards, {
					Background = Card,
					UIStroke = CardStroke,
					Title = nil,
					Desc = nil,
				})
			end

			return Container
		end

		return B
	end

	function Tabs:Line()
		local Line = Instance.new("Frame")
		Line.Name = "TabDivider"
		Line.Parent = TabListFrame
		Line.BackgroundColor3 = curColors['CardBorder']
		Line.BackgroundTransparency = 0.5
		Line.BorderSizePixel = 0
		Line.Size = UDim2.new(1, -12, 0, 1)
		Line.Position = UDim2.new(0, 6, 0, 0)
		return Line
	end

	function Tabs:SelectTab(nameOrIndex)
		if type(nameOrIndex) == "number" then
			if currentWindowTheme.tabs[nameOrIndex] and currentWindowTheme.tabs[nameOrIndex].Select then
				currentWindowTheme.tabs[nameOrIndex].Select()
			end
		else
			for _, t in ipairs(currentWindowTheme.tabs or {}) do
				if t.TabTitle and string.lower(t.TabTitle) == string.lower(tostring(nameOrIndex)) and t.Select then
					t.Select()
					break
				end
			end
		end
	end

	function Tabs:Tab(tabOptions)
		local TabTitle = tabOptions.Title or 'Tab'
		local Icon = tabOptions.Icon or 'house'

		local TabBtn = Instance.new("Frame")
		local TabCorner = Instance.new("UICorner")
		local TabIcon = Instance.new("ImageLabel")
		local TabTitleLabel = Instance.new("TextLabel")

		TabBtn.Name = "Tab_" .. TabTitle
		TabBtn.Parent = TabListFrame
		TabBtn.BackgroundColor3 = curColors['MainDark']
		TabBtn.BackgroundTransparency = 1
		TabBtn.BorderSizePixel = 0
		TabBtn.Size = UDim2.new(1, 0, 0, 40)

		TabCorner.CornerRadius = UDim.new(0, 6)
		TabCorner.Parent = TabBtn

		TabIcon.Name = "TabIcon"
		TabIcon.Parent = TabBtn
		TabIcon.AnchorPoint = Vector2.new(0, 0.5)
		TabIcon.BackgroundTransparency = 1
		TabIcon.Position = UDim2.new(0, 12, 0.5, 0)
		TabIcon.Size = UDim2.new(0, 20, 0, 20)
		local ic = gl(Icon)
		TabIcon.Image = ic.Image
		TabIcon.ImageRectSize = ic.ImageRectSize
		TabIcon.ImageRectOffset = ic.ImageRectPosition
		TabIcon.ImageColor3 = curColors['SubText']

		TabTitleLabel.Name = "Title"
		TabTitleLabel.Parent = TabBtn
		TabTitleLabel.AnchorPoint = Vector2.new(0, 0.5)
		TabTitleLabel.BackgroundTransparency = 1
		TabTitleLabel.Position = UDim2.new(0, 42, 0.5, 0)
		TabTitleLabel.Size = UDim2.new(1, -48, 0, 20)
		TabTitleLabel.Font = Enum.Font.BuilderSansBold
		TabTitleLabel.Text = TabTitle
		TabTitleLabel.TextColor3 = curColors['SubText']
		TabTitleLabel.TextSize = 15
		TabTitleLabel.TextXAlignment = Enum.TextXAlignment.Left

		local InPage_1 = Instance.new("Frame")
		InPage_1.Name = "InPage_" .. TabTitle
		InPage_1.Parent = Page_1
		InPage_1.BackgroundTransparency = 1
		InPage_1.Size = UDim2.new(1, 0, 1, 0)
		InPage_1.Visible = false

		local SubTabsBar = Instance.new("Frame")
		local SubTabsLayout = Instance.new("UIListLayout")
		SubTabsBar.Name = "SubTabsBar"
		SubTabsBar.Parent = InPage_1
		SubTabsBar.BackgroundTransparency = 1
		SubTabsBar.Position = UDim2.new(0, 6, 0, 0)
		SubTabsBar.Size = UDim2.new(1, -12, 0, 34)
		SubTabsBar.Visible = false

		SubTabsLayout.Parent = SubTabsBar
		SubTabsLayout.FillDirection = Enum.FillDirection.Horizontal
		SubTabsLayout.SortOrder = Enum.SortOrder.LayoutOrder
		SubTabsLayout.VerticalAlignment = Enum.VerticalAlignment.Center
		SubTabsLayout.Padding = UDim.new(0, 8)

		local PageScroll = Instance.new("ScrollingFrame")
		local PageLayout = Instance.new("UIListLayout")
		local PagePadding_1 = Instance.new("UIPadding")

		PageScroll.Name = "ScrollingFrame"
		PageScroll.Parent = InPage_1
		PageScroll.Active = true
		PageScroll.BackgroundTransparency = 1
		PageScroll.BorderSizePixel = 0
		PageScroll.Position = UDim2.new(0, 0, 0, 0)
		PageScroll.Size = UDim2.new(1, 0, 1, 0)
		PageScroll.ScrollBarThickness = 3
		PageScroll.ScrollBarImageColor3 = Color3.fromRGB(200, 200, 200)
		PageScroll.ClipsDescendants = true
		pcall(function() PageScroll.AutomaticCanvasSize = Enum.AutomaticSize.Y end)

		PageLayout.Parent = PageScroll
		PageLayout.Padding = UDim.new(0, 8)
		PageLayout.SortOrder = Enum.SortOrder.LayoutOrder

		PagePadding_1.Parent = PageScroll
		PagePadding_1.PaddingTop = UDim.new(0, 2)
		PagePadding_1.PaddingBottom = UDim.new(0, 12)
		PagePadding_1.PaddingLeft = UDim.new(0, 6)
		PagePadding_1.PaddingRight = UDim.new(0, 6)

		changecanvas(PageScroll, PageLayout, 10)

		local ClickTab = click(TabBtn)

		local tabData = {
			Page = InPage_1,
			Scroll = PageScroll,
			Button = TabBtn,
			Title = TabTitle,
			Icon = TabIcon,
			TitleLabel = TabTitleLabel
		}

		table.insert(self.List, tabData)
		local MyIndex = #self.List

		local function selectThisTab()
			Tabs.ActiveTabName = TabTitle
			for _, item in ipairs(self.List) do
				if item.Page then item.Page.Visible = false end
				tw({v = item.Button, t = 0.16, g = {BackgroundTransparency = 1}}):Play()
				tw({v = item.Icon, t = 0.16, g = {ImageColor3 = curColors['SubText']}}):Play()
				tw({v = item.TitleLabel, t = 0.16, g = {TextColor3 = curColors['SubText']}}):Play()
			end

			InPage_1.Position = UDim2.new(0, 4, 0, 0)
			InPage_1.Visible = true
			tw({v = InPage_1, t = 0.18, s = Enum.EasingStyle.Cubic, d = "Out", g = {Position = UDim2.new(0, 0, 0, 0)}}):Play()

			tw({v = TabBtn, t = 0.18, g = {BackgroundTransparency = 0, BackgroundColor3 = curColors['MainDark']}}):Play()
			tw({v = TabIcon, t = 0.18, g = {ImageColor3 = Color3.fromRGB(255, 255, 255)}}):Play()
			tw({v = TabTitleLabel, t = 0.18, g = {TextColor3 = Color3.fromRGB(255, 255, 255)}}):Play()
		end

		tabData.Select = selectThisTab
		ClickTab.MouseButton1Click:Connect(selectThisTab)

		TabBtn.MouseEnter:Connect(function()
			if not InPage_1.Visible then
				local c = currentWindowTheme.colors or curColors
				tw({v = TabBtn, t = 0.15, g = {BackgroundTransparency = 0.7, BackgroundColor3 = c['MainDark']}}):Play()
				tw({v = TabTitleLabel, t = 0.15, g = {TextColor3 = Color3.fromRGB(255, 255, 255)}}):Play()
			end
		end)
		TabBtn.MouseLeave:Connect(function()
			if not InPage_1.Visible then
				local c = currentWindowTheme.colors or curColors
				tw({v = TabBtn, t = 0.15, g = {BackgroundTransparency = 1}}):Play()
				tw({v = TabTitleLabel, t = 0.15, g = {TextColor3 = c['SubText']}}):Play()
			end
		end)

		if MyIndex == 1 then
			task.spawn(function()
				task.wait(0.05)
				selectThisTab()
			end)
		end

		local tabMeta = {
			TabTitle = TabTitle,
			SelectTab = selectThisTab,
			Scroll = PageScroll
		}
		local BaseBuilder = createControlBuilder(PageScroll, tabMeta)

		local Func = {
			Scroll = PageScroll,
			Page = InPage_1,
			Select = selectThisTab,
			SubTabsList = {}
		}

		for k, v in pairs(BaseBuilder) do
			Func[k] = function(self, ...) return v(BaseBuilder, ...) end
		end

		function Func:SubTab(subTitleOrOptions)
			local sTitle = (type(subTitleOrOptions) == "table" and subTitleOrOptions.Title) or tostring(subTitleOrOptions)
			SubTabsBar.Visible = true
			PageScroll.Position = UDim2.new(0, 0, 0, 38)
			PageScroll.Size = UDim2.new(1, 0, 1, -38)

			local subBtn = Instance.new("Frame")
			local subBtnCorner = Instance.new("UICorner")
			local subBtnStroke = Instance.new("UIStroke")
			local subBtnLabel = Instance.new("TextLabel")
			local subBtnClick = click(subBtn)

			subBtn.Name = "SubTab_" .. sTitle
			subBtn.Parent = SubTabsBar
			subBtn.BackgroundColor3 = Color3.fromRGB(24, 24, 30)
			subBtn.BorderSizePixel = 0
			subBtn.Size = UDim2.new(0, 85, 0, 32)
			subBtn.AutomaticSize = Enum.AutomaticSize.X

			subBtnCorner.CornerRadius = UDim.new(0, 6)
			subBtnCorner.Parent = subBtn

			subBtnStroke.Color = curColors['CardBorder']
			subBtnStroke.Thickness = 1
			subBtnStroke.Parent = subBtn

			subBtnLabel.Name = "Label"
			subBtnLabel.Parent = subBtn
			subBtnLabel.BackgroundTransparency = 1
			subBtnLabel.Size = UDim2.new(1, 0, 1, 0)
			subBtnLabel.Font = Enum.Font.BuilderSansBold
			subBtnLabel.Text = sTitle
			subBtnLabel.TextColor3 = curColors['SubText']
			subBtnLabel.TextSize = 15

			local subPad = Instance.new("UIPadding")
			subPad.Parent = subBtn
			subPad.PaddingLeft = UDim.new(0, 16)
			subPad.PaddingRight = UDim.new(0, 16)

			local SubScroll = Instance.new("ScrollingFrame")
			local SubLayout = Instance.new("UIListLayout")
			local SubPadding = Instance.new("UIPadding")

			SubScroll.Name = "SubScroll_" .. sTitle
			SubScroll.Parent = InPage_1
			SubScroll.Active = true
			SubScroll.BackgroundTransparency = 1
			SubScroll.BorderSizePixel = 0
			SubScroll.Position = UDim2.new(0, 0, 0, 38)
			SubScroll.Size = UDim2.new(1, 0, 1, -38)
			SubScroll.ScrollBarThickness = 3
			SubScroll.ScrollBarImageColor3 = Color3.fromRGB(200, 200, 200)
			SubScroll.Visible = false
			SubScroll.ClipsDescendants = true
		pcall(function() SubScroll.AutomaticCanvasSize = Enum.AutomaticSize.Y end)

			SubLayout.Parent = SubScroll
			SubLayout.Padding = UDim.new(0, 8)
			SubLayout.SortOrder = Enum.SortOrder.LayoutOrder

			SubPadding.Parent = SubScroll
			SubPadding.PaddingTop = UDim.new(0, 2)
			SubPadding.PaddingBottom = UDim.new(0, 12)
			SubPadding.PaddingLeft = UDim.new(0, 6)
			SubPadding.PaddingRight = UDim.new(0, 6)

			changecanvas(SubScroll, SubLayout, 10)

			local subObj
			local function selectSub()
				PageScroll.Visible = false
				local c = currentWindowTheme.colors or curColors
				for _, st in ipairs(Func.SubTabsList) do
					st.Scroll.Visible = false
					st.IsActive = false
					tw({v = st.Button, t = 0.15, g = {BackgroundColor3 = c['Card']}}):Play()
					tw({v = st.Label, t = 0.15, g = {TextColor3 = c['SubText']}}):Play()
					tw({v = st.Stroke, t = 0.15, g = {Color = c['CardBorder']}}):Play()
				end
				SubScroll.Visible = true
				if subObj then subObj.IsActive = true end
				tw({v = subBtn, t = 0.15, g = {BackgroundColor3 = c['Main']}}):Play()
				tw({v = subBtnLabel, t = 0.15, g = {TextColor3 = c['Background']}}):Play()
				tw({v = subBtnStroke, t = 0.15, g = {Color = c['Main']}}):Play()

				task.defer(function()
					for _, sec in ipairs(currentWindowTheme.sections or {}) do
						if sec.Scroll == SubScroll and sec.UpdateHeight then
							sec.UpdateHeight()
						end
					end
				end)
				task.delay(0.06, function()
					for _, sec in ipairs(currentWindowTheme.sections or {}) do
						if sec.Scroll == SubScroll and sec.UpdateHeight then
							sec.UpdateHeight()
						end
					end
				end)
				task.delay(0.18, function()
					for _, sec in ipairs(currentWindowTheme.sections or {}) do
						if sec.Scroll == SubScroll and sec.UpdateHeight then
							sec.UpdateHeight()
						end
					end
				end)
			end

			local subMeta = {
				TabTitle = TabTitle,
				SubTabTitle = sTitle,
				SelectTab = selectThisTab,
				SelectSubTab = selectSub,
				Scroll = SubScroll
			}
			local subBuilder = createControlBuilder(SubScroll, subMeta)

			subObj = {
				Scroll = SubScroll,
				Button = subBtn,
				Label = subBtnLabel,
				Stroke = subBtnStroke,
				IsActive = false,
				Select = selectSub
			}
			table.insert(Func.SubTabsList, subObj)
			table.insert(currentWindowTheme.subTabs, subObj)

			subBtnClick.MouseButton1Click:Connect(selectSub)

			if #Func.SubTabsList == 1 then
				task.defer(selectSub)
			end

			local ReturnSub = {
				Scroll = SubScroll,
				Select = selectSub
			}
			for k, v in pairs(subBuilder) do
				ReturnSub[k] = function(self, ...) return v(subBuilder, ...) end
			end
			return ReturnSub
		end

		return Func
	end

	local Notification = Instance.new("Frame")
	local UIPaddingNotif = Instance.new("UIPadding")
	local UIListLayoutNotif = Instance.new("UIListLayout")

	Notification.Name = "Notification"
	Notification.Parent = ScreenGui
	Notification.AnchorPoint = Vector2.new(1, 1)
	Notification.BackgroundTransparency = 1
	Notification.BorderSizePixel = 0
	Notification.Position = UDim2.new(1, -14, 1, -14)
	Notification.Size = UDim2.new(0, 310, 0, 360)
	Notification.ZIndex = 120

	UIPaddingNotif.Parent = Notification
	UIPaddingNotif.PaddingBottom = UDim.new(0, 8)

	UIListLayoutNotif.Parent = Notification
	UIListLayoutNotif.HorizontalAlignment = Enum.HorizontalAlignment.Right
	UIListLayoutNotif.SortOrder = Enum.SortOrder.LayoutOrder
	UIListLayoutNotif.VerticalAlignment = Enum.VerticalAlignment.Bottom
	UIListLayoutNotif.Padding = UDim.new(0, 6)

	local uiInitTime = tick()
	Tabs._suppressNotifications = false

	function Tabs:ClearNotifications()
		pcall(function()
			for _, child in ipairs(Notification:GetChildren()) do
				if child:IsA("GuiObject") then
					child:Destroy()
				end
			end
		end)
	end

	function Tabs:Notify(p)
		if Tabs._suppressNotifications then
			return
		end
		if (tick() - uiInitTime) < 1.0 and not (p and p.Force) then
			return
		end
		local NotifTitle = p.Title or 'Aether'
		local Desc = p.Desc or ''
		local Time = p.Time or 3.5
		local Type = p.Type or 'normal'

		local colorMap = {
			error = Color3.fromRGB(255, 60, 70),
			warning = Color3.fromRGB(255, 175, 40),
			normal = Color3.fromRGB(255, 255, 255)
		}
		local accent = colorMap[Type] or Color3.fromRGB(255, 255, 255)

		local NotifCard = Instance.new("CanvasGroup")
		local CardCorner = Instance.new("UICorner")
		local CardStroke = Instance.new("UIStroke")
		local Indicator = Instance.new("Frame")
		local IndicatorCorner = Instance.new("UICorner")
		local TextContainer = Instance.new("Frame")
		local TextPadding = Instance.new("UIPadding")
		local TextLayout = Instance.new("UIListLayout")
		local TitleLabel = Instance.new("TextLabel")
		local DescLabel = Instance.new("TextLabel")

		NotifCard.Name = "NotifCard"
		NotifCard.Parent = Notification
		NotifCard.BackgroundColor3 = curColors['Card']
		NotifCard.BorderSizePixel = 0
		NotifCard.Size = UDim2.new(1, 0, 0, 0)
		NotifCard.AutomaticSize = Enum.AutomaticSize.Y
		NotifCard.ClipsDescendants = true
		NotifCard.GroupTransparency = 1

		CardCorner.CornerRadius = UDim.new(0, 7)
		CardCorner.Parent = NotifCard

		CardStroke.Color = curColors['CardBorder']
		CardStroke.Thickness = 1.2
		CardStroke.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
		CardStroke.Transparency = 1
		CardStroke.Parent = NotifCard

		Indicator.Name = "Indicator"
		Indicator.Parent = NotifCard
		Indicator.BackgroundColor3 = accent
		Indicator.BorderSizePixel = 0
		Indicator.Position = UDim2.new(0, 0, 0, 0)
		Indicator.Size = UDim2.new(0, 3.5, 1, 0)

		IndicatorCorner.CornerRadius = UDim.new(0, 2)
		IndicatorCorner.Parent = Indicator

		TextContainer.Name = "TextContainer"
		TextContainer.Parent = NotifCard
		TextContainer.BackgroundTransparency = 1
		TextContainer.Position = UDim2.new(0, 14, 0, 0)
		TextContainer.Size = UDim2.new(1, -26, 0, 0)
		TextContainer.AutomaticSize = Enum.AutomaticSize.Y

		TextPadding.Parent = TextContainer
		TextPadding.PaddingTop = UDim.new(0, 9)
		TextPadding.PaddingBottom = UDim.new(0, 11)

		TextLayout.Parent = TextContainer
		TextLayout.SortOrder = Enum.SortOrder.LayoutOrder
		TextLayout.Padding = UDim.new(0, 3)

		TitleLabel.Name = "Title"
		TitleLabel.Parent = TextContainer
		TitleLabel.BackgroundTransparency = 1
		TitleLabel.Size = UDim2.new(1, 0, 0, 0)
		TitleLabel.AutomaticSize = Enum.AutomaticSize.Y
		TitleLabel.Font = Enum.Font.BuilderSansBold
		TitleLabel.Text = tostring(NotifTitle)
		TitleLabel.TextColor3 = curColors['Text & Icon']
		TitleLabel.TextSize = 14
		TitleLabel.TextXAlignment = Enum.TextXAlignment.Left
		TitleLabel.TextWrapped = true
		TitleLabel.LayoutOrder = 1

		if Desc ~= '' then
			DescLabel.Name = "Desc"
			DescLabel.Parent = TextContainer
			DescLabel.BackgroundTransparency = 1
			DescLabel.Size = UDim2.new(1, 0, 0, 0)
			DescLabel.AutomaticSize = Enum.AutomaticSize.Y
			DescLabel.Font = Enum.Font.BuilderSansMedium
			DescLabel.Text = tostring(Desc)
			DescLabel.TextColor3 = curColors['SubText']
			DescLabel.TextSize = 12.5
			DescLabel.TextXAlignment = Enum.TextXAlignment.Left
			DescLabel.TextWrapped = true
			DescLabel.LayoutOrder = 2
		end

		local ProgressBar = Instance.new("Frame")
		ProgressBar.Name = "ProgressBar"
		ProgressBar.Parent = NotifCard
		ProgressBar.BackgroundColor3 = accent
		ProgressBar.BorderSizePixel = 0
		ProgressBar.Position = UDim2.new(0, 0, 1, -2)
		ProgressBar.Size = UDim2.new(1, 0, 0, 2)
		tw({v = ProgressBar, t = Time, s = Enum.EasingStyle.Linear, d = "Out", g = {Size = UDim2.new(0, 0, 0, 2)}}):Play()

		NotifCard.Position = UDim2.new(1, 30, 0, 0)
		tw({v = NotifCard, t = 0.25, s = Enum.EasingStyle.Cubic, d = "Out", g = {GroupTransparency = 0, Position = UDim2.new(0, 0, 0, 0)}}):Play()
		tw({v = CardStroke, t = 0.25, s = Enum.EasingStyle.Cubic, d = "Out", g = {Transparency = 0}}):Play()

		task.delay(Time, function()
			local close = tw({v = NotifCard, t = 0.22, s = Enum.EasingStyle.Quad, d = "In", g = {GroupTransparency = 1, Position = UDim2.new(1, 30, 0, 0)}})
			local closeStroke = tw({v = CardStroke, t = 0.22, s = Enum.EasingStyle.Quad, d = "In", g = {Transparency = 1}})
			close:Play()
			closeStroke:Play()
			close.Completed:Connect(function() NotifCard:Destroy() end)
		end)
	end

	function Tabs:Dialog(p)
		local Button1 = (p.Button1 and p.Button1.Callback) or function() end
		local Button2 = (p.Button2 and p.Button2.Callback) or function() end
		local DialogTitle = p.Title or 'Notice'
		local TitleButton1 = (p.Button1 and p.Button1.Title) or 'Confirm'
		local TitleButton2 = (p.Button2 and p.Button2.Title) or 'Cancel'

		local Dialog = Instance.new("Frame")
		local DialogCorner = Instance.new("UICorner")
		local DialogCard = Instance.new("Frame")
		local CardCorner = Instance.new("UICorner")
		local CardStroke = Instance.new("UIStroke")
		local TitleLabel = Instance.new("TextLabel")
		local BtnFrame = Instance.new("Frame")
		local BtnLayout = Instance.new("UIListLayout")
		local B1 = Instance.new("TextButton")
		local B1Corner = Instance.new("UICorner")
		local B2 = Instance.new("TextButton")
		local B2Corner = Instance.new("UICorner")

		Dialog.Name = "Dialog"
		Dialog.Parent = Background_1
		Dialog.BackgroundColor3 = Color3.fromRGB(0, 0, 0)
		Dialog.BackgroundTransparency = 0.5
		Dialog.BorderSizePixel = 0
		Dialog.Size = UDim2.new(1, 0, 1, 0)
		Dialog.ZIndex = 80

		DialogCorner.CornerRadius = UDim.new(0, 10)
		DialogCorner.Parent = Dialog

		DialogCard.Name = "Card"
		DialogCard.Parent = Dialog
		DialogCard.AnchorPoint = Vector2.new(0.5, 0.5)
		DialogCard.BackgroundColor3 = curColors['Card']
		DialogCard.BorderSizePixel = 0
		DialogCard.Position = UDim2.new(0.5, 0, 0.5, 0)
		DialogCard.Size = UDim2.new(0, 280, 0, 115)

		CardCorner.CornerRadius = UDim.new(0, 8)
		CardCorner.Parent = DialogCard

		CardStroke.Color = curColors['CardBorder']
		CardStroke.Thickness = 1
		CardStroke.Parent = DialogCard

		TitleLabel.Parent = DialogCard
		TitleLabel.BackgroundTransparency = 1
		TitleLabel.Position = UDim2.new(0, 16, 0, 14)
		TitleLabel.Size = UDim2.new(1, -32, 0, 36)
		TitleLabel.Font = Enum.Font.BuilderSansBold
		TitleLabel.RichText = true
		TitleLabel.Text = DialogTitle
		TitleLabel.TextColor3 = curColors['Text & Icon']
		TitleLabel.TextSize = 14
		TitleLabel.TextWrapped = true

		BtnFrame.Parent = DialogCard
		BtnFrame.AnchorPoint = Vector2.new(0.5, 1)
		BtnFrame.BackgroundTransparency = 1
		BtnFrame.Position = UDim2.new(0.5, 0, 1, -12)
		BtnFrame.Size = UDim2.new(1, -32, 0, 30)

		BtnLayout.Parent = BtnFrame
		BtnLayout.FillDirection = Enum.FillDirection.Horizontal
		BtnLayout.HorizontalAlignment = Enum.HorizontalAlignment.Center
		BtnLayout.Padding = UDim.new(0, 12)

		B1.Parent = BtnFrame
		B1.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
		B1.BorderSizePixel = 0
		B1.Size = UDim2.new(0.48, 0, 1, 0)
		B1.Font = Enum.Font.BuilderSansBold
		B1.Text = TitleButton1
		B1.TextColor3 = Color3.fromRGB(15, 15, 18)
		B1.TextSize = 13

		B1Corner.CornerRadius = UDim.new(0, 4)
		B1Corner.Parent = B1

		B2.Parent = BtnFrame
		B2.BackgroundColor3 = Color3.fromRGB(36, 36, 46)
		B2.BorderSizePixel = 0
		B2.Size = UDim2.new(0.48, 0, 1, 0)
		B2.Font = Enum.Font.BuilderSansBold
		B2.Text = TitleButton2
		B2.TextColor3 = curColors['SubText']
		B2.TextSize = 13

		B2Corner.CornerRadius = UDim.new(0, 4)
		B2Corner.Parent = B2

		B1.MouseButton1Click:Connect(function()
			pcall(Button1)
			Dialog:Destroy()
		end)

		B2.MouseButton1Click:Connect(function()
			pcall(Button2)
			Dialog:Destroy()
		end)
	end

	do
		Close_1.MouseButton1Click:Connect(function()
			Tabs:Dialog({
				Title = "Close Aether interface?",
				Button1 = {
					Title = 'Close',
					Callback = function() ScreenGui:Destroy() end,
				},
				Button2 = { Title = 'Cancel' }
			})
		end)

		Minisize_1.MouseButton1Click:Connect(function()
			playClickSound()
			setUIVisibility(false)
		end)

		local toggleConn
		toggleConn = U.InputBegan:Connect(function(input)
			if (input.KeyCode == Enum.KeyCode.LeftControl or (Tabs.UIToggleKeybind and input.KeyCode == Tabs.UIToggleKeybind)) then
				if not U:GetFocusedTextBox() then setUIVisibility(isHidden) end
			end
		end)

		function Tabs:Toggle() setUIVisibility(isHidden) end
		function Tabs:SetVisible(vis) setUIVisibility(vis) end

		ScreenGui.Destroying:Connect(function()
			pcall(function()
				if toggleConn and toggleConn.Connected then
					toggleConn:Disconnect()
				end
			end)
			pcall(function()
				if resizeInputChangedConn and resizeInputChangedConn.Connected then
					resizeInputChangedConn:Disconnect()
				end
			end)
			pcall(function()
				if resizeInputEndedConn and resizeInputEndedConn.Connected then
					resizeInputEndedConn:Disconnect()
				end
			end)
			pcall(function()
				if LocalPlayer then
					LocalPlayer:GetMouse().Icon = ""
				end
			end)
			pcall(function()
				if BackgroundMusic then
					BackgroundMusic:Stop()
					BackgroundMusic:Destroy()
				end
				if ClickSound then
					ClickSound:Destroy()
				end
			end)
		end)
	end

	local HomeTab = Tabs:Tab({
		Title = "Home",
		Icon = "house"
	})

	local MainSub = HomeTab:SubTab("Main")
	local SettingsSub = HomeTab:SubTab("Settings")
	local ThemesSub = HomeTab:SubTab("Themes")

	Tabs.HomeTab = HomeTab
	Tabs.MainSub = MainSub
	Tabs.PricingSub = MainSub
	Tabs.SettingsSub = SettingsSub
	Tabs.ThemesSub = ThemesSub

	local mainScroll = MainSub.Scroll
	mainScroll.ScrollBarThickness = 2
	pcall(function() mainScroll.AutomaticCanvasSize = Enum.AutomaticSize.Y end)

	local HeroCard = Instance.new("Frame")
	local HeroCorner = Instance.new("UICorner")
	local HeroStroke = Instance.new("UIStroke")
	local HeroPadding = Instance.new("UIPadding")

	HeroCard.Name = "HeroCard"
	HeroCard.Parent = mainScroll
	HeroCard.BackgroundColor3 = curColors['Card']
	HeroCard.BorderSizePixel = 0
	HeroCard.Size = UDim2.new(1, 0, 0, 115)
	HeroCard.ClipsDescendants = true
	HeroCard.LayoutOrder = 1

	HeroCorner.CornerRadius = UDim.new(0, 10)
	HeroCorner.Parent = HeroCard

	HeroStroke.Color = curColors['CardBorder']
	HeroStroke.Thickness = 1
	HeroStroke.Parent = HeroCard

	HeroPadding.Parent = HeroCard
	HeroPadding.PaddingBottom = UDim.new(0, 14)
	HeroPadding.PaddingLeft = UDim.new(0, 16)
	HeroPadding.PaddingRight = UDim.new(0, 16)
	HeroPadding.PaddingTop = UDim.new(0, 14)

	table.insert(currentWindowTheme.cards, {
		Background = HeroCard,
		UIStroke = HeroStroke,
		Title = nil,
		Desc = nil,
	})

	local HeroLeft = Instance.new("Frame")
	HeroLeft.Name = "HeroLeft"
	HeroLeft.Parent = HeroCard
	HeroLeft.AnchorPoint = Vector2.new(0, 0.5)
	HeroLeft.BackgroundTransparency = 1
	HeroLeft.Position = UDim2.new(0, 0, 0.5, 0)
	HeroLeft.Size = UDim2.new(0.6, 0, 0, 84)

	local WelcomeBack = Instance.new("TextLabel")
	WelcomeBack.Name = "WelcomeBack"
	WelcomeBack.Parent = HeroLeft
	WelcomeBack.BackgroundTransparency = 1
	WelcomeBack.Position = UDim2.new(0, 0, 0, 0)
	WelcomeBack.Size = UDim2.new(1, 0, 0, 16)
	WelcomeBack.Font = Enum.Font.BuilderSansMedium
	WelcomeBack.Text = "Welcome back,"
	WelcomeBack.TextColor3 = curColors['SubText']
	WelcomeBack.TextSize = 13
	WelcomeBack.TextXAlignment = Enum.TextXAlignment.Left

	local PlayerTitle = Instance.new("TextLabel")
	PlayerTitle.Name = "PlayerTitle"
	PlayerTitle.Parent = HeroLeft
	PlayerTitle.BackgroundTransparency = 1
	PlayerTitle.Position = UDim2.new(0, 0, 0, 16)
	PlayerTitle.Size = UDim2.new(1, 0, 0, 26)
	PlayerTitle.Font = Enum.Font.BuilderSansBold
	PlayerTitle.RichText = true
	local displayName = (LocalPlayer and LocalPlayer.DisplayName) or "Player"
	PlayerTitle.Text = displayName .. ' 👋'
	PlayerTitle.TextColor3 = curColors['Text & Icon']
	PlayerTitle.TextSize = 22
	PlayerTitle.TextXAlignment = Enum.TextXAlignment.Left

	Tabs._playerTitle = PlayerTitle

	local ReadyLabel = Instance.new("TextLabel")
	ReadyLabel.Name = "ReadyLabel"
	ReadyLabel.Parent = HeroLeft
	ReadyLabel.BackgroundTransparency = 1
	ReadyLabel.Position = UDim2.new(0, 0, 0, 48)
	ReadyLabel.Size = UDim2.new(1, 0, 0, 18)
	ReadyLabel.Font = Enum.Font.BuilderSansBold
	ReadyLabel.Text = "Aether Studios is ready."
	ReadyLabel.TextColor3 = curColors['Text & Icon']
	ReadyLabel.TextSize = 14
	ReadyLabel.TextXAlignment = Enum.TextXAlignment.Left

	local ReadyDesc = Instance.new("TextLabel")
	ReadyDesc.Name = "ReadyDesc"
	ReadyDesc.Parent = HeroLeft
	ReadyDesc.BackgroundTransparency = 1
	ReadyDesc.Position = UDim2.new(0, 0, 0, 68)
	ReadyDesc.Size = UDim2.new(1, 0, 0, 16)
	ReadyDesc.Font = Enum.Font.BuilderSansMedium
	ReadyDesc.Text = "Thank you for using Aether!"
	ReadyDesc.TextColor3 = curColors['SubText']
	ReadyDesc.TextSize = 13
	ReadyDesc.TextXAlignment = Enum.TextXAlignment.Left

	local HeroRight = Instance.new("Frame")
	HeroRight.Name = "HeroRight"
	HeroRight.Parent = HeroCard
	HeroRight.AnchorPoint = Vector2.new(1, 0.5)
	HeroRight.BackgroundTransparency = 1
	HeroRight.Position = UDim2.new(1, 0, 0.5, 0)
	HeroRight.Size = UDim2.new(0.4, 0, 0, 56)

	local AvatarCircle = Instance.new("Frame")
	local AvatarCorner = Instance.new("UICorner")
	local AvatarStroke = Instance.new("UIStroke")
	local AvatarImg = Instance.new("ImageLabel")
	local AvatarImgCorner = Instance.new("UICorner")
	local OnlineDot = Instance.new("Frame")
	local OnlineDotCorner = Instance.new("UICorner")

	AvatarCircle.Name = "AvatarCircle"
	AvatarCircle.Parent = HeroRight
	AvatarCircle.AnchorPoint = Vector2.new(0, 0.5)
	AvatarCircle.BackgroundColor3 = Color3.fromRGB(24, 24, 30)
	AvatarCircle.BorderSizePixel = 0
	AvatarCircle.Position = UDim2.new(0, 0, 0.5, 0)
	AvatarCircle.Size = UDim2.new(0, 56, 0, 56)

	AvatarCorner.CornerRadius = UDim.new(1, 0)
	AvatarCorner.Parent = AvatarCircle

	AvatarStroke.Color = Color3.fromRGB(60, 60, 70)
	AvatarStroke.Thickness = 1.5
	AvatarStroke.Parent = AvatarCircle

	AvatarImg.Name = "AvatarImg"
	AvatarImg.Parent = AvatarCircle
	AvatarImg.BackgroundTransparency = 1
	AvatarImg.Size = UDim2.new(1, 0, 1, 0)
	AvatarImg.Image = (LocalPlayer and "rbxthumb://type=AvatarHeadShot&id="..LocalPlayer.UserId.."&w=150&h=150") or "rbxassetid://10747373176"

	AvatarImgCorner.CornerRadius = UDim.new(1, 0)
	AvatarImgCorner.Parent = AvatarImg

	Tabs._avatarImg = AvatarImg

	OnlineDot.Name = "OnlineDot"
	OnlineDot.Parent = AvatarCircle
	OnlineDot.AnchorPoint = Vector2.new(1, 1)
	OnlineDot.BackgroundColor3 = Color3.fromRGB(34, 197, 94)
	OnlineDot.BorderSizePixel = 0
	OnlineDot.Position = UDim2.new(1, 2, 1, 2)
	OnlineDot.Size = UDim2.new(0, 12, 0, 12)
	OnlineDot.ZIndex = 5

	OnlineDotCorner.CornerRadius = UDim.new(1, 0)
	OnlineDotCorner.Parent = OnlineDot

	local ProfileInfo = Instance.new("Frame")
	ProfileInfo.Name = "ProfileInfo"
	ProfileInfo.Parent = HeroRight
	ProfileInfo.AnchorPoint = Vector2.new(0, 0.5)
	ProfileInfo.BackgroundTransparency = 1
	ProfileInfo.Position = UDim2.new(0, 68, 0.5, 0)
	ProfileInfo.Size = UDim2.new(1, -68, 0, 56)

	local ProfileName = Instance.new("TextLabel")
	ProfileName.Name = "Name"
	ProfileName.Parent = ProfileInfo
	ProfileName.BackgroundTransparency = 1
	ProfileName.Position = UDim2.new(0, 0, 0, 0)
	ProfileName.Size = UDim2.new(1, 0, 0, 18)
	ProfileName.Font = Enum.Font.BuilderSansBold
	ProfileName.Text = (LocalPlayer and LocalPlayer.Name) or "Player"
	ProfileName.TextColor3 = curColors['Text & Icon']
	ProfileName.TextSize = 15
	ProfileName.TextXAlignment = Enum.TextXAlignment.Left

	Tabs._profileName = ProfileName

	local SubRowHero = Instance.new("Frame")
	local SubTierLabelHero = Instance.new("TextLabel")
	local SubExpiresLabelHero = Instance.new("TextLabel")

	SubRowHero.Name = "SubscriptionRow"
	SubRowHero.Parent = ProfileInfo
	SubRowHero.BackgroundTransparency = 1
	SubRowHero.Position = UDim2.new(0, 0, 0, 20)
	SubRowHero.Size = UDim2.new(1, 0, 0, 36)

	SubTierLabelHero.Name = "SubTierLabel"
	SubTierLabelHero.Parent = SubRowHero
	SubTierLabelHero.BackgroundTransparency = 1
	SubTierLabelHero.Position = UDim2.new(0, 0, 0, 0)
	SubTierLabelHero.Size = UDim2.new(1, 0, 0, 18)
	SubTierLabelHero.Font = Enum.Font.BuilderSansMedium
	SubTierLabelHero.RichText = true
	SubTierLabelHero.Text = '<font color="rgb(180,180,195)">Subscription: </font><font color="rgb(255,255,255)"><b>Free</b></font>'
	SubTierLabelHero.TextSize = 13
	SubTierLabelHero.TextXAlignment = Enum.TextXAlignment.Left

	SubExpiresLabelHero.Name = "SubExpiresLabel"
	SubExpiresLabelHero.Parent = SubRowHero
	SubExpiresLabelHero.BackgroundTransparency = 1
	SubExpiresLabelHero.Position = UDim2.new(0, 0, 0, 18)
	SubExpiresLabelHero.Size = UDim2.new(1, 0, 0, 18)
	SubExpiresLabelHero.Font = Enum.Font.BuilderSansMedium
	SubExpiresLabelHero.RichText = true
	SubExpiresLabelHero.Text = '<font color="rgb(180,180,195)">Expires at: </font><font color="rgb(255,255,255)">0 seconds</font>'
	SubExpiresLabelHero.TextSize = 13
	SubExpiresLabelHero.TextXAlignment = Enum.TextXAlignment.Left

	Tabs._subTierHero = SubTierLabelHero
	Tabs._subExpiresHero = SubExpiresLabelHero



	local PricingSec = MainSub:Section({ Title = "Available Key Tiers & Subscriptions", LayoutOrder = 2 })

	PricingSec:PricingCards({
		{
			Title = "Free",
			Badge = "12-24H",
			Subtitle = "Free Key Checkpoint",
			TextColor = Color3.fromRGB(255, 255, 255),
			AccentColor = Color3.fromRGB(255, 255, 255),
			BorderColor = Color3.fromRGB(65, 65, 75),
			Features = {
				"Free features",
				"12-24h Key checkpoint",
				"Minimal support",
				"Renew anytime"
			}
		},
		{
			Title = "Premium",
			Badge = "EXCLUSIVE",
			Subtitle = { "Weekly", "Monthly", "Lifetime" },
			TextColor = Color3.fromRGB(250, 204, 21),
			SubtitleColor = Color3.fromRGB(253, 224, 71),
			AccentColor = Color3.fromRGB(250, 204, 21),
			BorderColor = Color3.fromRGB(250, 204, 21),
			Glow = true,
			ButtonBg = Color3.fromRGB(42, 34, 12),
			ButtonStroke = Color3.fromRGB(190, 150, 25),
			ButtonTextColor = Color3.fromRGB(250, 204, 21),
			ButtonText = "Purchase Key",
			Features = {
				"All Free features",
				"Priority support",
				"Helps the developers",
				"Keyless & instant"
			},
			Callback = function()
				if setclipboard then setclipboard("https://getaether.mysellauth.com/") end
				Tabs:Notify({ Title = "Store Link Copied", Desc = "Aether store link copied! Visit store to purchase your license.", Time = 3.5 })
			end
		},
		{
			Title = "Booster",
			Badge = "DISCORD",
			Subtitle = "Server Boosts",
			TextColor = Color3.fromRGB(192, 132, 252),
			SubtitleColor = Color3.fromRGB(216, 180, 254),
			AccentColor = Color3.fromRGB(192, 132, 252),
			BorderColor = Color3.fromRGB(115, 65, 160),
			ButtonBg = Color3.fromRGB(38, 20, 52),
			ButtonStroke = Color3.fromRGB(160, 85, 220),
			ButtonTextColor = Color3.fromRGB(192, 132, 252),
			ButtonText = "Boost Discord",
			Features = {
				"All Premium features",
				"Cool Discord role",
				"Priority support",
				"Free with 2x Boosts"
			},
			Callback = function()
				if setclipboard then setclipboard(DiscordLink) end
				Tabs:Notify({ Title = "Discord Link Copied", Desc = "Discord invite copied to clipboard! Boost our server to unlock perks.", Time = 3.5 })
			end
		}
	})

	local PrivSec = SettingsSub:Section({ Title = "Streamer Mode", LayoutOrder = 1 })

	local StreamerToggleObj = PrivSec:Toggle({
		Title = "Streamer Mode",
		Desc = "Hides your username, display name, user ID, and avatar across the entire interface",
		Image = "shield",
		Value = Tabs.StreamerMode,
		IgnoreConfig = true,
		Callback = function(enabled)
			Tabs:SetStreamerMode(enabled)
			Tabs:Notify({
				Title = "Streamer Mode",
				Desc = enabled and "Identity hidden for streaming/recording" or "Identity visible",
				Time = 2.5
			})
		end
	})
	table.insert(Tabs._streamerToggles, StreamerToggleObj)

	local FPSToggleObj = PrivSec:Toggle({
		Title = "FPS Unlocker",
		Desc = "Remove standard 60 FPS frame rate limit (unlocks up to 240+ FPS)",
		Image = "zap",
		Value = false,
		Callback = function(state)
			local setcap = (type(setfpscap) == "function" and setfpscap)
				or (type(set_fps_cap) == "function" and set_fps_cap)
			if setcap then
				pcall(function() setcap(state and 240 or 60) end)
			end
			Tabs:Notify({
				Title = "FPS Unlocker",
				Desc = state and "Frame rate unlocked (240 FPS cap)" or "Frame rate capped at 60 FPS",
				Time = 2
			})
		end
	})

	local ControlsSec = SettingsSub:Section({ Title = "Interface Controls", LayoutOrder = 3 })

	ControlsSec:Keybind({
		Title = "UI Toggle Keybind",
		Desc = "Key used to open and minimize the interface",
		Key = Keybind,
		IgnoreConfig = true,
		Callback = function(newKey)
			Tabs.UIToggleKeybind = newKey
			local kname = newKey.Name
			if kname == "LeftControl" then kname = "LeftCtrl" end
			if kname == "RightControl" then kname = "RightCtrl" end
			ReopenKeybind.Text = "[" .. kname .. "]"
			Tabs:Notify({ Title = "Keybind Set", Desc = "Rebound to " .. kname, Time = 2 })
		end
	})

	ControlsSec:Toggle({
		Title = "Auto-Save Window Size",
		Desc = "Automatically remember your window size whenever you finish resizing",
		Value = true,
		IgnoreConfig = true,
		Callback = function(val)
			autoSaveSize = val
		end
	})

	ControlsSec:Button({
		Title = "Save Current Window Size",
		Desc = "Lock in the current window dimensions so it always opens at this size",
		Image = "save",
		Callback = function()
			Tabs:SaveWindowSize()
		end
	})

	ControlsSec:Button({
		Title = "Reset Window Size",
		Desc = "Restore default window dimensions",
		Image = "refresh-cw",
		Callback = function()
			Tabs:ResetWindowSize()
		end
	})

	local ConfigSec = SettingsSub:Section({ Title = "Configuration Manager", LayoutOrder = 4 })
	local currentConfigName = "Default"

	ConfigSec:Textbox({
		Title = "Config File Name",
		Desc = "Name used when saving to Aether/Configs",
		Placeholder = "Enter config name...",
		Value = "Default",
		IgnoreConfig = true,
		Callback = function(val)
			if val and val ~= "" then currentConfigName = val end
		end
	})

	local ConfigDropdown = ConfigSec:Dropdown({
		Title = "Select Config",
		Desc = "Saved configs found in Aether/Configs",
		List = FileSystem:ListConfigs(),
		Value = currentConfigName,
		IgnoreConfig = true,
		Callback = function(val) currentConfigName = val end
	})

	ConfigSec:Button({
		Title = "Save Config",
		Desc = "Save all settings, toggles and appearance to Aether/Configs",
		Image = "save",
		Callback = function()
			Tabs._suppressNotifications = true

			local sectionsState = {}
			for _, sec in ipairs(currentWindowTheme.sections or {}) do
				if sec.TitleText and sec.IsMinimized then
					sectionsState[sec.TitleText] = sec.IsMinimized()
				end
			end

			local elementsState = {}
			for _, elem in ipairs(ConfigElements) do
				local ok, val = pcall(elem.Get)
				if ok and val ~= nil then
					elementsState[elem.Flag] = val
				end
			end

			local curTrans = (WallTransSlider and WallTransSlider:GetValue()) or math.floor(WindowBackgroundImg.ImageTransparency * 100)
			local curDim = (WallDimSlider and WallDimSlider:GetValue()) or math.floor((1 - WindowBackgroundDim.BackgroundTransparency) * 100)

			local cfgData = {
				ThemeName = currentWindowTheme.name,
				AccentColor = { R = math.floor(curColors['Main'].R * 255), G = math.floor(curColors['Main'].G * 255), B = math.floor(curColors['Main'].B * 255) },
				BackgroundColor = { R = math.floor(curColors['Background'].R * 255), G = math.floor(curColors['Background'].G * 255), B = math.floor(curColors['Background'].B * 255) },
				CardColor = { R = math.floor(curColors['Card'].R * 255), G = math.floor(curColors['Card'].G * 255), B = math.floor(curColors['Card'].B * 255) },
				TextColor = { R = math.floor(curColors['Text & Icon'].R * 255), G = math.floor(curColors['Text & Icon'].R * 255), B = math.floor(curColors['Text & Icon'].B * 255) },
				SubTextColor = { R = math.floor(curColors['SubText'].R * 255), G = math.floor(curColors['SubText'].G * 255), B = math.floor(curColors['SubText'].B * 255) },
				BorderColor = { R = math.floor(curColors['CardBorder'].R * 255), G = math.floor(curColors['CardBorder'].G * 255), B = math.floor(curColors['CardBorder'].B * 255) },
				Transparency = deepCopy(currentWindowTheme.transparency),
				Wallpaper = currentActiveWallpaper,
				BackgroundImage = WindowBackgroundImg.Image,
				BackgroundTransparency = curTrans,
				BackgroundDim = curDim,
				Sounds = deepCopy(currentSoundConfig),
				Cursor = deepCopy(currentCursorConfig),
				ToggleKeybind = tostring(Tabs.UIToggleKeybind),
				StreamerMode = Tabs.StreamerMode,
				Sections = sectionsState,
				Elements = elementsState,
				WindowSize = { Width = Shadow_1.Size.X.Offset, Height = Shadow_1.Size.Y.Offset }
			}
			FileSystem:SaveConfig(currentConfigName, cfgData)
			ConfigDropdown:Clear()
			for _, c in ipairs(FileSystem:ListConfigs()) do ConfigDropdown:Add(c) end
			ConfigDropdown:SetValue(currentConfigName, true)

			task.delay(0.2, function()
				Tabs._suppressNotifications = false
			end)
		end
	})

	ConfigSec:Button({
		Title = "Load Config",
		Desc = "Apply selected config file from Aether/Configs",
		Image = "folder",
		Callback = function()
			local cfgData = FileSystem:LoadConfig(currentConfigName)
			if cfgData then
				Tabs._suppressNotifications = true
				if Tabs.ClearNotifications then Tabs:ClearNotifications() end

				if cfgData.ThemeName and themes[cfgData.ThemeName] then Tabs:ApplyTheme(cfgData.ThemeName) end
				if cfgData.AccentColor then
					Tabs:ApplyTheme({
						Main = Color3.fromRGB(cfgData.AccentColor.R, cfgData.AccentColor.G, cfgData.AccentColor.B),
						Background = cfgData.BackgroundColor and Color3.fromRGB(cfgData.BackgroundColor.R, cfgData.BackgroundColor.G, cfgData.BackgroundColor.B) or curColors['Background'],
						Card = cfgData.CardColor and Color3.fromRGB(cfgData.CardColor.R, cfgData.CardColor.G, cfgData.CardColor.B) or curColors['Card'],
						['Text & Icon'] = cfgData.TextColor and Color3.fromRGB(cfgData.TextColor.R, cfgData.TextColor.G, cfgData.TextColor.B) or curColors['Text & Icon'],
						SubText = cfgData.SubTextColor and Color3.fromRGB(cfgData.SubTextColor.R, cfgData.SubTextColor.G, cfgData.SubTextColor.B) or curColors['SubText'],
						CardBorder = cfgData.BorderColor and Color3.fromRGB(cfgData.BorderColor.R, cfgData.BorderColor.G, cfgData.BorderColor.B) or curColors['CardBorder']
					})
				end

				if cfgData.Transparency and type(cfgData.Transparency) == "table" then
					applyAllTransparencies(cfgData.Transparency)
				end

				if cfgData.WindowSize and type(cfgData.WindowSize) == "table" and cfgData.WindowSize.Width and cfgData.WindowSize.Height then
					Tabs:SetWindowSize(cfgData.WindowSize.Width, cfgData.WindowSize.Height, true)
				end

				if cfgData.Sounds and type(cfgData.Sounds) == "table" then
					applySoundConfig(cfgData.Sounds)
				end

				if cfgData.Cursor and type(cfgData.Cursor) == "table" then
					applyCursorConfig(cfgData.Cursor)
				end

				local wp = cfgData.Wallpaper or cfgData.BackgroundImage
				if wp and wp ~= "" then
					currentActiveWallpaper = wp
					local asset = FileSystem:GetWallpaperAsset(wp)
					WindowBackgroundImg.Image = asset

					local isAssetId = tostring(wp):match("^%d+$") or tostring(wp):match("^rbxassetid://") or tostring(wp):match("^rbxasset://") or tostring(wp):match("^https?://")

					if isAssetId then
						if WallDropdown then WallDropdown:SetValue("None", true) end
						if WallAssetInput then WallAssetInput:SetValue(wp, true) end
					else
						if WallDropdown then WallDropdown:SetValue(wp, true) end
						if WallAssetInput then WallAssetInput:SetValue("", true) end
					end

					local trans = cfgData.BackgroundTransparency
					if trans == nil then trans = 30 end
					if trans > 0 and trans <= 1 then trans = math.floor(trans * 100) end
					if WallTransSlider then
						WallTransSlider:SetValue(trans, true)
					else
						WindowBackgroundImg.ImageTransparency = trans / 100
					end

					local dim = cfgData.BackgroundDim
					if dim == nil then dim = 50 end
					if dim > 0 and dim <= 1 then dim = math.floor((1 - dim) * 100) end
					if WallDimSlider then
						WallDimSlider:SetValue(dim, true)
					else
						WindowBackgroundDim.BackgroundTransparency = 1 - (dim / 100)
					end
				else
					currentActiveWallpaper = ""
					WindowBackgroundImg.Image = ""
					WindowBackgroundImg.ImageTransparency = 1
					WindowBackgroundDim.BackgroundTransparency = 1
					if WallDropdown then WallDropdown:SetValue("None", true) end
					if WallAssetInput then WallAssetInput:SetValue("", true) end
					if WallTransSlider then WallTransSlider:SetValue(30, true) end
					if WallDimSlider then WallDimSlider:SetValue(50, true) end
				end

				if cfgData.StreamerMode ~= nil then
					Tabs:SetStreamerMode(cfgData.StreamerMode)
				end
				if cfgData.Sections and type(cfgData.Sections) == "table" then
					for _, sec in ipairs(currentWindowTheme.sections or {}) do
						if sec.TitleText and cfgData.Sections[sec.TitleText] ~= nil then
							sec.SetMinimized(cfgData.Sections[sec.TitleText])
						else
							sec.SetMinimized(false)
						end
					end
				else
					for _, sec in ipairs(currentWindowTheme.sections or {}) do
						if sec.SetMinimized then
							sec.SetMinimized(false)
						end
					end
				end

				if cfgData.Elements and type(cfgData.Elements) == "table" then
					for _, elem in ipairs(ConfigElements) do
						local savedVal = cfgData.Elements[elem.Flag]
						if savedVal == nil then savedVal = cfgData.Elements[elem.Title] end
						if savedVal == nil and cfgData[elem.Flag] ~= nil then savedVal = cfgData[elem.Flag] end
						if savedVal ~= nil then
							pcall(function() elem.Set(savedVal) end)
						end
					end
				end

				task.delay(0.25, function()
					Tabs._suppressNotifications = false
				end)
			end
		end
	})

	ConfigSec:Button({
		Title = "Delete Config",
		Desc = "Remove selected config from Aether/Configs",
		Image = "trash-2",
		Callback = function()
			FileSystem:DeleteConfig(currentConfigName)
			ConfigDropdown:Clear()
			for _, c in ipairs(FileSystem:ListConfigs()) do ConfigDropdown:Add(c) end
			Tabs:Notify({ Title = "Config Deleted", Desc = "Deleted '" .. currentConfigName .. "'", Time = 2 })
		end
	})

	ConfigSec:Button({
		Title = "Reset to Default Config",
		Desc = "Reset all controls, toggles, appearance, and audio to defaults",
		Image = "rotate-ccw",
		Callback = function()
			Tabs._suppressNotifications = true
			if Tabs.ClearNotifications then Tabs:ClearNotifications() end

			for _, elem in ipairs(ConfigElements) do
				if elem.Default ~= nil and elem.Set then
					pcall(function() elem.Set(elem.Default) end)
				end
			end

			Tabs:ApplyTheme("Default")
			if ThemeDropdown then ThemeDropdown:SetValue("Default", true) end
			applyAllTransparencies(0, 0, 0, 0, 100)
			if TransPresetDropdown then TransPresetDropdown:SetValue("Solid (Default)", true) end

			currentActiveWallpaper = ""
			WindowBackgroundImg.Image = ""
			WindowBackgroundImg.ImageTransparency = 1
			WindowBackgroundDim.BackgroundTransparency = 1
			if WallDropdown then WallDropdown:SetValue("None", true) end
			if WallAssetInput then WallAssetInput:SetValue("", true) end
			if WallTransSlider then WallTransSlider:SetValue(30, true) end
			if WallDimSlider then WallDimSlider:SetValue(50, true) end

			for _, sec in ipairs(currentWindowTheme.sections or {}) do
				if sec.SetMinimized then sec.SetMinimized(false) end
			end

			Tabs:SetStreamerMode(false)

			applySoundConfig({
				BGMEnabled = false,
				BGMTrack = "None",
				BGMAsset = "",
				BGMVolume = 50,
				BGMLooped = true
			})

			applyCursorConfig({
				Enabled = false,
				Preset = "Default (Roblox)",
				CustomAsset = ""
			})

			task.delay(0.25, function()
				Tabs._suppressNotifications = false
			end)
		end
	})

	local ThemePresetSec = ThemesSub:Section({ Title = "Theme Presets" })

	local ThemeDropdown = ThemePresetSec:Dropdown({
		Title = "Theme Preset",
		Desc = "Select Default or Halloween",
		List = { "Default", "Halloween" },
		Value = currentWindowTheme.name,
		IgnoreConfig = true,
		Callback = function(themeName)
			if themeName and themeName ~= "Custom" then
				Tabs:ApplyTheme(themeName)
				Tabs:Notify({ Title = "Theme Changed", Desc = "Applied " .. themeName .. " theme", Time = 2 })
			end
		end
	})

	local ThemeColorSec = ThemesSub:Section({ Title = "Theme Colors Customizer" })

	ThemeColorSec:ColorPicker({
		Title = "Accent / Highlights Color",
		Desc = "Changes active toggles, highlights, and borders",
		Value = curColors['Main'],
		IgnoreConfig = true,
		Callback = function(col)
			Tabs:ApplyTheme({ Main = col })
			ThemeDropdown:SetValue("Custom", true)
		end
	})

	ThemeColorSec:ColorPicker({
		Title = "Window Background Color",
		Desc = "Base background color of Aether",
		Value = curColors['Background'],
		IgnoreConfig = true,
		Callback = function(col)
			Tabs:ApplyTheme({ Background = col, Page = col, Sidebar = col })
			ThemeDropdown:SetValue("Custom", true)
		end
	})

	ThemeColorSec:ColorPicker({
		Title = "Card Background Color",
		Desc = "Background of content boxes and sections",
		Value = curColors['Card'],
		IgnoreConfig = true,
		Callback = function(col)
			Tabs:ApplyTheme({ Card = col })
			ThemeDropdown:SetValue("Custom", true)
		end
	})

	ThemeColorSec:ColorPicker({
		Title = "Text & Icon Color",
		Desc = "Primary font and icon color",
		Value = curColors['Text & Icon'],
		IgnoreConfig = true,
		Callback = function(col)
			Tabs:ApplyTheme({ ['Text & Icon'] = col })
			ThemeDropdown:SetValue("Custom", true)
		end
	})

	ThemeColorSec:ColorPicker({
		Title = "SubText Color",
		Desc = "Descriptions, hints, and muted texts",
		Value = curColors['SubText'],
		IgnoreConfig = true,
		Callback = function(col)
			Tabs:ApplyTheme({ SubText = col })
			ThemeDropdown:SetValue("Custom", true)
		end
	})

	ThemeColorSec:ColorPicker({
		Title = "Card Border Color",
		Desc = "Border outline color",
		Value = curColors['CardBorder'],
		IgnoreConfig = true,
		Callback = function(col)
			Tabs:ApplyTheme({ CardBorder = col })
			ThemeDropdown:SetValue("Custom", true)
		end
	})

	local TransSec, WallSec = ThemesSub:DualSection(
		{ Title = "Interface Transparency & Glass" },
		{ Title = "Wallpaper Customization" }
	)

	local transPresets = {
		["Solid (Default)"] = { Window = 0, Sidebar = 0, Section = 0, Card = 0 },
		["Frosted Glass"] = { Window = 15, Sidebar = 20, Section = 25, Card = 20 },
		["Translucent"] = { Window = 35, Sidebar = 40, Section = 45, Card = 35 },
		["Ultra Clear"] = { Window = 60, Sidebar = 65, Section = 70, Card = 55 },
	}

	local initialPreset = getPresetNameForValues(
		currentWindowTheme.transparency.Window or 0,
		currentWindowTheme.transparency.Sidebar or 0,
		currentWindowTheme.transparency.Section or 0,
		currentWindowTheme.transparency.Card or 0
	)

	TransPresetDropdown = TransSec:Dropdown({
		Title = "Transparency Mode",
		Desc = "Quick presets for glass & transparency styling",
		List = { "Solid (Default)", "Frosted Glass", "Translucent", "Ultra Clear", "Custom" },
		Value = initialPreset,
		IgnoreConfig = true,
		Callback = function(presetName)
			if transPresets[presetName] then
				applyAllTransparencies(transPresets[presetName])
			end
		end
	})

	WindowTransSlider = TransSec:Slider({
		Title = "Window Base Transparency",
		Desc = "Main window canvas background (0% = solid, 100% = clear)",
		Min = 0,
		Max = 100,
		Value = currentWindowTheme.transparency.Window or 0,
		Rounding = 0,
		IgnoreConfig = true,
		Callback = function(val)
			setWindowTransparency(val)
			if not skipPresetReset and TransPresetDropdown then
				TransPresetDropdown:SetValue("Custom", true)
			end
		end
	})

	SidebarTransSlider = TransSec:Slider({
		Title = "Sidebar Transparency",
		Desc = "Left navigation tab bar (0% = solid, 100% = clear)",
		Min = 0,
		Max = 100,
		Value = currentWindowTheme.transparency.Sidebar or 0,
		Rounding = 0,
		IgnoreConfig = true,
		Callback = function(val)
			setSidebarTransparency(val)
			if not skipPresetReset and TransPresetDropdown then
				TransPresetDropdown:SetValue("Custom", true)
			end
		end
	})

	SectionTransSlider = TransSec:Slider({
		Title = "Section Frames Transparency",
		Desc = "Container frames around card groups (0% = solid, 100% = clear)",
		Min = 0,
		Max = 100,
		Value = currentWindowTheme.transparency.Section or 0,
		Rounding = 0,
		IgnoreConfig = true,
		Callback = function(val)
			setSectionTransparency(val)
			if not skipPresetReset and TransPresetDropdown then
				TransPresetDropdown:SetValue("Custom", true)
			end
		end
	})

	CardTransSlider = TransSec:Slider({
		Title = "Card Background Transparency",
		Desc = "Element cards background (0% = solid, 100% = clear)",
		Min = 0,
		Max = 100,
		Value = currentWindowTheme.transparency.Card or 0,
		Rounding = 0,
		IgnoreConfig = true,
		Callback = function(val)
			setCardTransparency(val)
			if not skipPresetReset and TransPresetDropdown then
				TransPresetDropdown:SetValue("Custom", true)
			end
		end
	})


	local initialWallpapers = FileSystem:ListWallpapers()
	local wallList = { "None" }
	for _, w in ipairs(initialWallpapers) do table.insert(wallList, w) end

	local initialWallVal = "None"
	if currentActiveWallpaper ~= "" and not currentActiveWallpaper:match("^%d+$") and not currentActiveWallpaper:match("^rbxassetid://") and not currentActiveWallpaper:match("^rbxasset://") and not currentActiveWallpaper:match("^https?://") then
		initialWallVal = currentActiveWallpaper
	end

	WallDropdown = WallSec:Dropdown({
		Title = "Custom Wallpaper (Themes/Wallpaper)",
		Desc = "Select image from Aether/Themes/Wallpaper folder",
		List = wallList,
		Value = initialWallVal,
		IgnoreConfig = true,
		Callback = function(val)
			if val and val ~= "None" and val ~= "--" and val ~= "" then
				currentActiveWallpaper = val
				local asset = FileSystem:GetWallpaperAsset(val)
				WindowBackgroundImg.Image = asset
				if WallAssetInput then WallAssetInput:SetValue("") end

				local curTrans = (WallTransSlider and WallTransSlider:GetValue()) or 30
				if curTrans >= 100 then
					curTrans = 30
					if WallTransSlider then WallTransSlider:SetValue(30) end
				end
				WindowBackgroundImg.ImageTransparency = curTrans / 100

				local curDim = (WallDimSlider and WallDimSlider:GetValue()) or 50
				WindowBackgroundDim.BackgroundTransparency = 1 - (curDim / 100)

				Tabs:Notify({ Title = "Wallpaper Applied", Desc = "Loaded '" .. tostring(val) .. "'", Time = 2 })
			elseif val == "None" then
				currentActiveWallpaper = ""
				WindowBackgroundImg.Image = ""
				WindowBackgroundImg.ImageTransparency = 1
				WindowBackgroundDim.BackgroundTransparency = 1
				Tabs:Notify({ Title = "Wallpaper Cleared", Desc = "Solid background restored", Time = 2 })
			end
		end
	})

	WallSec:Button({
		Title = "Refresh Wallpaper Folder",
		Desc = "Re-scan Aether/Themes/Wallpaper for newly placed images",
		Image = "refresh-cw",
		Callback = function()
			local refreshed = FileSystem:ListWallpapers()
			WallDropdown:Clear()
			WallDropdown:Add("None")
			for _, w in ipairs(refreshed) do
				WallDropdown:Add(w)
			end
			if currentActiveWallpaper ~= "" and table.find(refreshed, currentActiveWallpaper) then
				WallDropdown:SetValue(currentActiveWallpaper, true)
			else
				WallDropdown:SetValue("None", true)
			end
			Tabs:Notify({ Title = "Wallpapers Scanned", Desc = "Found " .. #refreshed .. " file(s) in folder", Time = 2.5 })
		end
	})

	WallAssetInput = WallSec:Textbox({
		Title = "Or Roblox Image Asset ID",
		Desc = "Roblox Image Asset ID (e.g. 12345678 or rbxassetid://...)",
		Placeholder = "rbxassetid://...",
		IgnoreConfig = true,
		Callback = function(val)
			if val and val ~= "" then
				local id = val
				if not id:find("rbxassetid://") and tonumber(id) then id = "rbxassetid://" .. id end
				currentActiveWallpaper = id
				WindowBackgroundImg.Image = id
				if WallDropdown then WallDropdown:SetValue("None", true) end

				local curTrans = (WallTransSlider and WallTransSlider:GetValue()) or 30
				if curTrans >= 100 then
					curTrans = 30
					if WallTransSlider then WallTransSlider:SetValue(30) end
				end
				WindowBackgroundImg.ImageTransparency = curTrans / 100

				local curDim = (WallDimSlider and WallDimSlider:GetValue()) or 50
				WindowBackgroundDim.BackgroundTransparency = 1 - (curDim / 100)

				Tabs:Notify({ Title = "Wallpaper Applied", Desc = "Custom asset ID loaded", Time = 2 })
			end
		end
	})

	WallTransSlider = WallSec:Slider({
		Title = "Wallpaper Transparency",
		Desc = "Adjust visibility of the custom background image",
		Min = 0,
		Max = 100,
		Value = 30,
		Rounding = 0,
		IgnoreConfig = true,
		Callback = function(val)
			WindowBackgroundImg.ImageTransparency = val / 100
		end
	})

	WallDimSlider = WallSec:Slider({
		Title = "Wallpaper Dim Level",
		Desc = "Darken the wallpaper so UI text stays clear and readable",
		Min = 0,
		Max = 100,
		Value = 50,
		Rounding = 0,
		IgnoreConfig = true,
		Callback = function(val)
			WindowBackgroundDim.BackgroundTransparency = 1 - (val / 100)
		end
	})

	WallSec:Button({
		Title = "Clear Wallpaper",
		Desc = "Removes custom wallpaper and resets to solid theme",
		Image = "trash-2",
		Callback = function()
			currentActiveWallpaper = ""
			WindowBackgroundImg.Image = ""
			WindowBackgroundImg.ImageTransparency = 1
			WindowBackgroundDim.BackgroundTransparency = 1
			if WallDropdown then WallDropdown:SetValue("None", true) end
			if WallAssetInput then WallAssetInput:SetValue("") end
			if WallTransSlider then WallTransSlider:SetValue(30) end
			if WallDimSlider then WallDimSlider:SetValue(50) end
			Tabs:Notify({ Title = "Wallpaper Cleared", Desc = "Solid background restored", Time = 2 })
		end
	})

	local SoundSec, CursorSec = ThemesSub:DualSection(
		{ Title = "Interface Background Music", Minimized = true },
		{ Title = "Custom Cursor Customization", Minimized = true }
	)

	local initialSounds = FileSystem:ListSounds()
	local bgmList = { "None", "Lofi Chill", "Ambient Dream" }
	for _, s in ipairs(initialSounds) do if not table.find(bgmList, s) then table.insert(bgmList, s) end end

	BGMToggle = SoundSec:Toggle({
		Title = "Enable Background Music",
		Desc = "Play continuous audio track or song in the background",
		Value = currentSoundConfig.BGMEnabled,
		IgnoreConfig = true,
		Callback = function(state)
			currentSoundConfig.BGMEnabled = state
			if state then
				local assetToPlay = ""
				if currentSoundConfig.BGMAsset and currentSoundConfig.BGMAsset ~= "" then
					assetToPlay = FileSystem:GetAudioAsset(currentSoundConfig.BGMAsset)
				elseif currentSoundConfig.BGMTrack and currentSoundConfig.BGMTrack ~= "None" then
					local presets = {
						["Lofi Chill"] = "rbxassetid://9043887091",
						["Ambient Dream"] = "rbxassetid://1843404009"
					}
					assetToPlay = presets[currentSoundConfig.BGMTrack] or FileSystem:GetAudioAsset(currentSoundConfig.BGMTrack)
				end
				if assetToPlay and assetToPlay ~= "" then
					BackgroundMusic.SoundId = assetToPlay
					pcall(function() BackgroundMusic:Play() end)
				end
			else
				pcall(function() BackgroundMusic:Stop() end)
			end
		end
	})

	BGMTrackDropdown = SoundSec:Dropdown({
		Title = "BGM Track Preset",
		Desc = "Choose from curated music loops or local files in Themes/Sounds",
		List = bgmList,
		Value = currentSoundConfig.BGMTrack or "None",
		IgnoreConfig = true,
		Callback = function(track)
			currentSoundConfig.BGMTrack = track
			if track and track ~= "None" and track ~= "" then
				currentSoundConfig.BGMAsset = ""
				if BGMAssetInput then BGMAssetInput:SetValue("", true) end
				local presets = {
					["Lofi Chill"] = "rbxassetid://9043887091",
					["Ambient Dream"] = "rbxassetid://1843404009"
				}
				local assetToPlay = presets[track] or FileSystem:GetAudioAsset(track)
				BackgroundMusic.SoundId = assetToPlay
				if currentSoundConfig.BGMEnabled then
					pcall(function() BackgroundMusic:Play() end)
				end
			else
				pcall(function() BackgroundMusic:Stop() end)
			end
		end
	})

	SoundSec:Button({
		Title = "Refresh Sounds List",
		Desc = "Re-scan Aether/Themes/Sounds for newly placed audio files",
		Image = "refresh-cw",
		Callback = function()
			local refreshed = FileSystem:ListSounds()
			local defaultBGM = { "None", "Lofi Chill", "Ambient Dream" }
			BGMTrackDropdown:Clear()
			for _, b in ipairs(defaultBGM) do
				BGMTrackDropdown:Add(b)
			end
			for _, s in ipairs(refreshed) do
				if not table.find(defaultBGM, s) then
					BGMTrackDropdown:Add(s)
				end
			end
			if currentSoundConfig.BGMTrack and currentSoundConfig.BGMTrack ~= "" then
				BGMTrackDropdown:SetValue(currentSoundConfig.BGMTrack, true)
			else
				BGMTrackDropdown:SetValue("None", true)
			end
			Tabs:Notify({ Title = "Sounds Scanned", Desc = "Found " .. #refreshed .. " file(s) in folder", Time = 2.5 })
		end
	})

	BGMAssetInput = SoundSec:Textbox({
		Title = "Or Custom BGM Sound ID / File",
		Desc = "Enter Roblox Sound ID (e.g. 9043887091) or audio file name",
		Placeholder = "rbxassetid://...",
		Value = currentSoundConfig.BGMAsset,
		IgnoreConfig = true,
		Callback = function(val)
			if val and val ~= "" then
				local id = val
				if not id:find("rbxassetid://") and tonumber(id) then id = "rbxassetid://" .. id end
				currentSoundConfig.BGMAsset = id
				currentSoundConfig.BGMTrack = "None"
				local asset = FileSystem:GetAudioAsset(id)
				BackgroundMusic.SoundId = asset
				if BGMTrackDropdown then BGMTrackDropdown:SetValue("None", true) end
				if currentSoundConfig.BGMEnabled then
					pcall(function() BackgroundMusic:Play() end)
				end
				Tabs:Notify({ Title = "BGM Updated", Desc = "Custom audio track loaded", Time = 2 })
			else
				currentSoundConfig.BGMAsset = ""
			end
		end
	})

	BGMVolumeSlider = SoundSec:Slider({
		Title = "BGM Volume",
		Desc = "Adjust background music volume level",
		Min = 0,
		Max = 100,
		Value = currentSoundConfig.BGMVolume,
		Suffix = "%",
		Rounding = 0,
		IgnoreConfig = true,
		Callback = function(val)
			currentSoundConfig.BGMVolume = val
			BackgroundMusic.Volume = val / 100
		end
	})

	BGMLoopToggle = SoundSec:Toggle({
		Title = "Loop Background Music",
		Desc = "Keep background music running continuously in a loop",
		Value = currentSoundConfig.BGMLooped,
		IgnoreConfig = true,
		Callback = function(state)
			currentSoundConfig.BGMLooped = state
			BackgroundMusic.Looped = state
		end
	})

	local initialCursors = FileSystem:ListCursors()
	local cursorList = { "Default (Roblox)", "Crosshair (Clean)", "Arrow (Far)", "Pointer (Sleek)" }
	for _, c in ipairs(initialCursors) do
		if not table.find(cursorList, c) then
			table.insert(cursorList, c)
		end
	end

	CursorToggle = CursorSec:Toggle({
		Title = "Enable Custom Cursor",
		Desc = "Replace default Roblox cursor with custom crosshair or image",
		Value = currentCursorConfig.Enabled,
		IgnoreConfig = true,
		Callback = function(state)
			if isUpdatingCursorUI then return end
			currentCursorConfig.Enabled = state
			if state and (currentCursorConfig.Preset == "Default (Roblox)" or currentCursorConfig.Preset == "None" or currentCursorConfig.Preset == "") and (currentCursorConfig.CustomAsset == "") then
				currentCursorConfig.Preset = "Crosshair (Clean)"
				if CursorPresetDropdown then CursorPresetDropdown:SetValue("Crosshair (Clean)", true) end
			end
			applyCursorConfig(currentCursorConfig)
		end
	})

	CursorPresetDropdown = CursorSec:Dropdown({
		Title = "Cursor Style Preset",
		Desc = "Select built-in cursor preset or local file from Themes/Cursors",
		List = cursorList,
		Value = currentCursorConfig.Preset or "Default (Roblox)",
		IgnoreConfig = true,
		Callback = function(val)
			if isUpdatingCursorUI then return end
			currentCursorConfig.Preset = val
			if val and val ~= "None" and val ~= "Default (Roblox)" and val ~= "" then
				currentCursorConfig.CustomAsset = ""
				currentCursorConfig.Enabled = true
				if CursorToggle then CursorToggle:SetValue(true, true) end
				if CursorAssetInput then CursorAssetInput:SetValue("", true) end
				applyCursorConfig(currentCursorConfig)
				Tabs:Notify({ Title = "Cursor Applied", Desc = "Loaded '" .. tostring(val) .. "'", Time = 2 })
			elseif val == "Default (Roblox)" or val == "None" then
				currentCursorConfig.Enabled = false
				currentCursorConfig.CustomAsset = ""
				if CursorToggle then CursorToggle:SetValue(false, true) end
				if CursorAssetInput then CursorAssetInput:SetValue("", true) end
				applyCursorConfig(currentCursorConfig)
			end
		end
	})

	CursorSec:Button({
		Title = "Refresh Cursors List",
		Desc = "Re-scan Aether/Themes/Cursors for newly placed cursor images",
		Image = "refresh-cw",
		Callback = function()
			local refreshed = FileSystem:ListCursors()
			local defaultPresets = { "Default (Roblox)", "Crosshair (Clean)", "Arrow (Far)", "Pointer (Sleek)" }
			CursorPresetDropdown:Clear()
			for _, p in ipairs(defaultPresets) do
				CursorPresetDropdown:Add(p)
			end
			for _, c in ipairs(refreshed) do
				if not table.find(defaultPresets, c) then
					CursorPresetDropdown:Add(c)
				end
			end
			if currentCursorConfig.Preset and currentCursorConfig.Preset ~= "" then
				CursorPresetDropdown:SetValue(currentCursorConfig.Preset, true)
			else
				CursorPresetDropdown:SetValue("Default (Roblox)", true)
			end
			Tabs:Notify({ Title = "Cursors Scanned", Desc = "Found " .. #refreshed .. " file(s) in folder", Time = 2.5 })
		end
	})

	CursorAssetInput = CursorSec:Textbox({
		Title = "Or Roblox Cursor Asset ID",
		Desc = "Roblox Image Asset ID (e.g. 12345678 or rbxassetid://...)",
		Placeholder = "rbxassetid://...",
		Value = currentCursorConfig.CustomAsset,
		IgnoreConfig = true,
		Callback = function(val)
			if isUpdatingCursorUI then return end
			if val and val ~= "" then
				local id = val
				if not id:find("rbxassetid://") and tonumber(id) then id = "rbxassetid://" .. id end
				currentCursorConfig.CustomAsset = id
				currentCursorConfig.Preset = "None"
				currentCursorConfig.Enabled = true
				if CursorToggle then CursorToggle:SetValue(true, true) end
				if CursorPresetDropdown then CursorPresetDropdown:SetValue("None", true) end
				applyCursorConfig(currentCursorConfig)
				Tabs:Notify({ Title = "Cursor Applied", Desc = "Custom cursor asset loaded", Time = 2 })
			else
				currentCursorConfig.CustomAsset = ""
				applyCursorConfig(currentCursorConfig)
			end
		end
	})

	CursorSec:Button({
		Title = "Reset Cursor to Default",
		Desc = "Restore original Roblox mouse pointer",
		Image = "rotate-ccw",
		Callback = function()
			applyCursorConfig({
				Enabled = false,
				Preset = "Default (Roblox)",
				CustomAsset = ""
			})
			Tabs:Notify({ Title = "Cursor Reset", Desc = "Default Roblox mouse cursor restored", Time = 2 })
		end
	})

	local ThemeFileSec = ThemesSub:Section({ Title = "Save / Load Custom Themes" })
	local currentCustomThemeName = "MyTheme"

	ThemeFileSec:Textbox({
		Title = "Custom Theme Name",
		Desc = "Name to save/load in Aether/Themes",
		Placeholder = "Theme name...",
		Value = "MyTheme",
		IgnoreConfig = true,
		Callback = function(val)
			if val and val ~= "" then currentCustomThemeName = val end
		end
	})

	local ThemeFileDropdown = ThemeFileSec:Dropdown({
		Title = "Saved Custom Themes",
		Desc = "Saved themes found in Aether/Themes",
		List = FileSystem:ListThemes(),
		Value = "Select Theme",
		IgnoreConfig = true,
		Callback = function(val) currentCustomThemeName = val end
	})

	ThemeFileSec:Button({
		Title = "Save Current Theme",
		Desc = "Saves active colors and wallpaper to Aether/Themes/",
		Image = "save",
		Callback = function()
			local curTrans = (WallTransSlider and WallTransSlider:GetValue()) or math.floor(WindowBackgroundImg.ImageTransparency * 100)
			local curDim = (WallDimSlider and WallDimSlider:GetValue()) or math.floor((1 - WindowBackgroundDim.BackgroundTransparency) * 100)

			local thmData = {
				Main = { math.floor(curColors['Main'].R * 255), math.floor(curColors['Main'].G * 255), math.floor(curColors['Main'].B * 255) },
				Background = { math.floor(curColors['Background'].R * 255), math.floor(curColors['Background'].G * 255), math.floor(curColors['Background'].B * 255) },
				Card = { math.floor(curColors['Card'].R * 255), math.floor(curColors['Card'].G * 255), math.floor(curColors['Card'].B * 255) },
				['Text & Icon'] = { math.floor(curColors['Text & Icon'].R * 255), math.floor(curColors['Text & Icon'].G * 255), math.floor(curColors['Text & Icon'].B * 255) },
				SubText = { math.floor(curColors['SubText'].R * 255), math.floor(curColors['SubText'].G * 255), math.floor(curColors['SubText'].B * 255) },
				CardBorder = { math.floor(curColors['CardBorder'].R * 255), math.floor(curColors['CardBorder'].G * 255), math.floor(curColors['CardBorder'].B * 255) },
				Transparency = deepCopy(currentWindowTheme.transparency),
				Wallpaper = currentActiveWallpaper,
				WallpaperImage = WindowBackgroundImg.Image,
				WallpaperTransparency = curTrans,
				WallpaperDim = curDim,
				Sounds = deepCopy(currentSoundConfig),
				Cursor = deepCopy(currentCursorConfig)
			}
			FileSystem:SaveTheme(currentCustomThemeName, thmData)
			ThemeFileDropdown:Clear()
			for _, t in ipairs(FileSystem:ListThemes()) do ThemeFileDropdown:Add(t) end
			ThemeFileDropdown:SetValue(currentCustomThemeName, true)
			Tabs:Notify({ Title = "Theme Saved", Desc = "Saved '" .. currentCustomThemeName .. "' with wallpaper & audio to Aether/Themes/", Time = 2.5 })
		end
	})

	ThemeFileSec:Button({
		Title = "Load Saved Theme",
		Desc = "Applies chosen theme from Aether/Themes/",
		Image = "folder",
		Callback = function()
			local thmData = FileSystem:LoadTheme(currentCustomThemeName)
			if thmData then
				Tabs._suppressNotifications = true
				if Tabs.ClearNotifications then Tabs:ClearNotifications() end

				local parsed = {}
				for k, v in pairs(thmData) do
					if type(v) == "table" and #v >= 3 then
						local r, g, b = v[1], v[2], v[3]
						if r > 1 or g > 1 or b > 1 then
							parsed[k] = Color3.fromRGB(math.clamp(r, 0, 255), math.clamp(g, 0, 255), math.clamp(b, 0, 255))
						else
							parsed[k] = Color3.new(math.clamp(r, 0, 1), math.clamp(g, 0, 1), math.clamp(b, 0, 1))
						end
					end
				end
				Tabs:ApplyTheme(parsed)
				ThemeDropdown:SetValue("Custom", true)

				if thmData.Transparency and type(thmData.Transparency) == "table" then
					applyAllTransparencies(thmData.Transparency)
				else
					applyAllTransparencies(0, 0, 0, 0)
				end

				if thmData.Sounds and type(thmData.Sounds) == "table" then
					applySoundConfig(thmData.Sounds)
				else
					applySoundConfig({
						BGMEnabled = false,
						BGMTrack = "None",
						BGMAsset = "",
						BGMVolume = 50,
						BGMLooped = true
					})
				end

				if thmData.Cursor and type(thmData.Cursor) == "table" then
					applyCursorConfig(thmData.Cursor)
				else
					applyCursorConfig({
						Enabled = false,
						Preset = "Default (Roblox)",
						CustomAsset = ""
					})
				end

				local wp = thmData.Wallpaper or thmData.WallpaperImage
				if wp and wp ~= "" then
					currentActiveWallpaper = wp
					local asset = FileSystem:GetWallpaperAsset(wp)
					WindowBackgroundImg.Image = asset

					local isAssetId = tostring(wp):match("^%d+$") or tostring(wp):match("^rbxassetid://") or tostring(wp):match("^rbxasset://") or tostring(wp):match("^https?://")

					if isAssetId then
						if WallDropdown then WallDropdown:SetValue("None", true) end
						if WallAssetInput then WallAssetInput:SetValue(wp, true) end
					else
						if WallDropdown then WallDropdown:SetValue(wp, true) end
						if WallAssetInput then WallAssetInput:SetValue("", true) end
					end

					local trans = thmData.WallpaperTransparency
					if trans == nil then trans = 30 end
					if trans > 0 and trans <= 1 then trans = math.floor(trans * 100) end
					if WallTransSlider then
						WallTransSlider:SetValue(trans, true)
					else
						WindowBackgroundImg.ImageTransparency = trans / 100
					end

					local dim = thmData.WallpaperDim
					if dim == nil then dim = 50 end
					if dim > 0 and dim <= 1 then
						dim = math.floor((1 - dim) * 100)
					end
					if WallDimSlider then
						WallDimSlider:SetValue(dim, true)
					else
						WindowBackgroundDim.BackgroundTransparency = 1 - (dim / 100)
					end
				else
					currentActiveWallpaper = ""
					WindowBackgroundImg.Image = ""
					WindowBackgroundImg.ImageTransparency = 1
					WindowBackgroundDim.BackgroundTransparency = 1
					if WallDropdown then WallDropdown:SetValue("None", true) end
					if WallAssetInput then WallAssetInput:SetValue("", true) end
					if WallTransSlider then WallTransSlider:SetValue(30, true) end
					if WallDimSlider then WallDimSlider:SetValue(50, true) end
				end

				Tabs._suppressNotifications = false
				Tabs:Notify({ Title = "Theme Loaded", Desc = "Loaded '" .. currentCustomThemeName .. "'", Time = 2.5 })
			else
				Tabs:Notify({ Title = "Theme Error", Desc = "Could not find theme file", Time = 2.5, Type = "error" })
			end
		end
	})

	ThemeFileSec:Button({
		Title = "Reset to Default Theme",
		Desc = "Restores pure black & white colors, solid glass, clears wallpapers & audio",
		Image = "rotate-ccw",
		Callback = function()
			Tabs._suppressNotifications = true
			if Tabs.ClearNotifications then Tabs:ClearNotifications() end

			Tabs:ApplyTheme("Default")
			if ThemeDropdown then ThemeDropdown:SetValue("Default", true) end

			applyAllTransparencies(0, 0, 0, 0)
			if TransPresetDropdown then TransPresetDropdown:SetValue("Solid (Default)", true) end

			currentActiveWallpaper = ""
			WindowBackgroundImg.Image = ""
			WindowBackgroundImg.ImageTransparency = 1
			WindowBackgroundDim.BackgroundTransparency = 1
			if WallDropdown then WallDropdown:SetValue("None", true) end
			if WallAssetInput then WallAssetInput:SetValue("", true) end
			if WallTransSlider then WallTransSlider:SetValue(30, true) end
			if WallDimSlider then WallDimSlider:SetValue(50, true) end

			applySoundConfig({
				BGMEnabled = false,
				BGMTrack = "None",
				BGMAsset = "",
				BGMVolume = 50,
				BGMLooped = true
			})

			applyCursorConfig({
				Enabled = false,
				Preset = "Default (Roblox)",
				CustomAsset = ""
			})

			Tabs._suppressNotifications = false
			Tabs:Notify({ Title = "Theme Reset", Desc = "Restored default theme settings", Time = 2.5 })
		end
	})



	task.spawn(function()
		task.wait(0.2)
		local autoName = FileSystem:GetAutoload()
		if autoName and autoName ~= "" then
			local cfgData = FileSystem:LoadConfig(autoName)
			if cfgData then
				Tabs._suppressNotifications = true
				if cfgData.ThemeName and themes[cfgData.ThemeName] then
					Tabs:ApplyTheme(cfgData.ThemeName)
				end
				if cfgData.Transparency and type(cfgData.Transparency) == "table" then
					applyAllTransparencies(cfgData.Transparency)
				end
				if cfgData.WindowSize and type(cfgData.WindowSize) == "table" and cfgData.WindowSize.Width and cfgData.WindowSize.Height then
					Tabs:SetWindowSize(cfgData.WindowSize.Width, cfgData.WindowSize.Height, false)
				end
				if cfgData.Sounds and type(cfgData.Sounds) == "table" then
					applySoundConfig(cfgData.Sounds)
				end
				if cfgData.Cursor and type(cfgData.Cursor) == "table" then
					applyCursorConfig(cfgData.Cursor)
				end
				if cfgData.StreamerMode ~= nil then
					Tabs:SetStreamerMode(cfgData.StreamerMode)
				end
				local wp = cfgData.Wallpaper or cfgData.BackgroundImage
				if wp and wp ~= "" then
					currentActiveWallpaper = wp
					local asset = FileSystem:GetWallpaperAsset(wp)
					WindowBackgroundImg.Image = asset

					local isAssetId = tostring(wp):match("^%d+$") or tostring(wp):match("^rbxassetid://") or tostring(wp):match("^rbxasset://") or tostring(wp):match("^https?://")

					if isAssetId then
						if WallDropdown then WallDropdown:SetValue("None", true) end
						if WallAssetInput then WallAssetInput:SetValue(wp, true) end
					else
						if WallDropdown then WallDropdown:SetValue(wp, true) end
						if WallAssetInput then WallAssetInput:SetValue("", true) end
					end

					local trans = cfgData.BackgroundTransparency
					if trans == nil then trans = 30 end
					if trans > 0 and trans <= 1 then trans = math.floor(trans * 100) end
					if WallTransSlider then
						WallTransSlider:SetValue(trans, true)
					else
						WindowBackgroundImg.ImageTransparency = trans / 100
					end

					local dim = cfgData.BackgroundDim
					if dim == nil then dim = 50 end
					if dim > 0 and dim <= 1 then dim = math.floor((1 - dim) * 100) end
					if WallDimSlider then
						WallDimSlider:SetValue(dim, true)
					else
						WindowBackgroundDim.BackgroundTransparency = 1 - (dim / 100)
					end
				end

				if cfgData.Elements and type(cfgData.Elements) == "table" then
					for _, elem in ipairs(ConfigElements) do
						local savedVal = cfgData.Elements[elem.Flag]
						if savedVal == nil then savedVal = cfgData.Elements[elem.Title] end
						if savedVal == nil and cfgData[elem.Flag] ~= nil then savedVal = cfgData[elem.Flag] end
						if savedVal ~= nil then
							pcall(function() elem.Set(savedVal) end)
						end
					end
				end
				task.delay(0.3, function()
					Tabs._suppressNotifications = false
				end)
			end
		end
	end)

	Library.CurrentWindow = Tabs
	if typeof(getgenv) == "function" then
		getgenv().CurrentWindow = Tabs
	end
	if typeof(_G) == "table" then
		_G.CurrentWindow = Tabs
	end
	if typeof(shared) == "table" then
		shared.CurrentWindow = Tabs
	end

	return Tabs
end

function Library:Notify(p)
	if Library.CurrentWindow and Library.CurrentWindow.Notify then
		return Library.CurrentWindow:Notify(p)
	end
end

exportGlobals(Library)

return Library
