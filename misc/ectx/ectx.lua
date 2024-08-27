--
-- ectx.lua: A Lua script for engage-cmd that simulates group transmit scenarios
--
-- Copyright (c) 2024 Rally Tactical Systems, Inc.
--

------------------------------------------------------------
-- Settings
--
-- The following settings can be overridden by setting the
-- corresponding environment variable before running the
-- script.
------------------------------------------------------------
-- The minimum number of milliseconds to run for
-- 0 means forever
local ECTX_RT = 0

-- The minimum number of milliseconds of transmit time
-- This must be at least 500 ms and must be less than ECTX_MAX_TX
local ECTX_MIN_TX = 500

-- The maximum number of milliseconds of transmit time
-- This must be at least 500 ms greater than ECTX_MIN_TX
local ECTX_MAX_TX = 2000

-- The probability that a tx will be turned on in any given cycle
-- This must be between 1 and 100
local ECTX_TX_PROB = 75

-- If true (the default), displays a (somewhat) pretty UI
local ECTX_UI = true


------------------------------------------------------------
-- ANSI color codes
------------------------------------------------------------
local CLR_ANSI_FG_BLACK = 30
local CLR_ANSI_FG_WHITE = 37
local CLR_ANSI_FG_GRAY = 90
local CLR_ANSI_FG_GREEN = 32
local CLR_ANSI_FG_RED = 31

local CLR_ANSI_BG_BLACK = 40
local CLR_ANSI_BG_RED = 41
local CLR_ANSI_BG_GREEN = 42

------------------------------------------------------------
-- Global variables
------------------------------------------------------------
local _voiceGroupControlArray = {}
local _numberOfGroups = 0
local _startTickMs = 0

------------------------------------------------------------
function round(num)
    if num >= 0 then
        return math.floor(num + 0.5)
    else
        return math.ceil(num - 0.5)
    end
end

------------------------------------------------------------
function intFromEnvOrDefault(envVar, defaultValue)
	local v = math.tointeger(os.getenv(envVar))
	if v == nil then
		return defaultValue
	else
		return v
	end
end

------------------------------------------------------------
function checkSettings()
	-- Get any overrides from the environment
	ECTX_RT = intFromEnvOrDefault("ECTX_RT", ECTX_RT)
	ECTX_MIN_TX = intFromEnvOrDefault("ECTX_MIN_TX", ECTX_MIN_TX)
	ECTX_MAX_TX = intFromEnvOrDefault("ECTX_MAX_TX", ECTX_MAX_TX)
	ECTX_TX_PROB = intFromEnvOrDefault("ECTX_TX_PROB", ECTX_TX_PROB)

	-- We can't run for less than 0 milliseconds
	if ECTX_RT < 0 then
		print("**** runtime of " .. ECTX_RT .. " is invalid. ****")
		return false
	end

	-- We can't tx less than 500 ms at a time
	if ECTX_MIN_TX < 500 then
		print("**** ECTX_MIN_TX of " .. ECTX_MIN_TX .. " cannot be less than 500. ****")
		return false
	end
	-- We can't tx more than 60000 ms at a time
	if ECTX_MAX_TX > 60000 then
		print("**** ECTX_MAX_TX of " .. ECTX_MAX_TX .. " cannot be greater than 60000. ****")
		return false
	end

	-- ECTX_MIN_TX must be less than ECTX_MAX_TX
	if ECTX_MIN_TX >= ECTX_MAX_TX then
		print("**** ECTX_MIN_TX of " .. ECTX_MIN_TX .. " is greater than ECTX_MAX_TX of " .. ECTX_MAX_TX .. ".****")
		return false
	end

	-- ECTX_MAX_TX must be at least 500 ms greater than ECTX_MIN_TX
	if ECTX_MAX_TX - ECTX_MIN_TX < 500 then
		print("**** difference between ECTX_MIN_TX of " .. ECTX_MIN_TX .. " and ECTX_MAX_TX of " .. ECTX_MAX_TX .. " must be 500 milliseconds or more.****")
		return false
	end

	-- ECTX_TX_PROB must be between 1 and 100
	if ECTX_TX_PROB < 1 or ECTX_TX_PROB > 100 then
		print("**** ECTX_TX_PROB of " .. ECTX_TX_PROB .. " is not between 1 and 100.****")
		return false
	end

	return true
end

------------------------------------------------------------
function clearScreen()
    io.write("\27[2J")
    io.write("\27[H")
end

------------------------------------------------------------
function moveCursor(x, y)
    io.write(string.format("\27[%d;%dH", y, x))
end

------------------------------------------------------------
function setTextColor(color)
    io.write("\27[" .. color .. "m")
end

------------------------------------------------------------
function resetFormatting()
    io.write("\27[0m")
end

------------------------------------------------------------
function randRange(base, max)
    return base + math.ceil(math.random() * max)
end


------------------------------------------------------------
function updateUi()
	if ECTX_UI then
		clearScreen()
		moveCursor(1, 1)
		setTextColor(CLR_ANSI_FG_WHITE)
		setTextColor(CLR_ANSI_BG_BLACK)
		io.write("----------------------------------------------------------------------------------------\n")
		io.write("ectx.lua: A Lua script for engage-cmd that simulates group transmit scenarios\n")
		io.write("Copyright (c) 2024 Rally Tactical Systems, Inc.\n")
		io.write("----------------------------------------------------------------------------------------\n")
		
		io.write(_numberOfGroups .. " voice group")
		if _numberOfGroups > 1 then
			io.write("s") 
		end	
		io.write(" | " .. ECTX_TX_PROB .. "% tx probability | tx range " .. ECTX_MIN_TX .. " to " .. ECTX_MAX_TX .. " ms | ")

		if ECTX_RT == 0 then
			io.write("forever")
		else
			now = ecGetTickMs()
			remaining = round((((_startTickMs + ECTX_RT) - now) / 1000))
			io.write(remaining .. " seconds remaining")
		end
		io.write("\n")
		io.write("\n")

		now = ecGetTickMs()	
		blocks = 0
		
		for x = 0, #_voiceGroupControlArray do
			-- If we have printed n blocks, then start a new line
			if blocks == 4 then
				io.write("\n")
				blocks = 0
			end

			-- Print the number of milliseconds left in the tx cycle (if any) for the group.  Use green to indicate
			-- that the tx is on, and red to indicate that tx is off
			if _voiceGroupControlArray[x].txOffAt > 0 then
				setTextColor(CLR_ANSI_FG_BLACK)
				setTextColor(CLR_ANSI_BG_GREEN)
				val = _voiceGroupControlArray[x].txOffAt - now
			else
				setTextColor(CLR_ANSI_FG_GRAY)
				setTextColor(CLR_ANSI_BG_BLACK)
				val = 0
			end

			io.write(string.format("%24s", _voiceGroupControlArray[x].groupName .. " : " .. string.format("%6d", val)))
			blocks = blocks + 1

			setTextColor(CLR_ANSI_FG_WHITE)
			setTextColor(CLR_ANSI_BG_BLACK)
			io.write("  ")
		end	

		io.write("\n")

		resetFormatting()
	end
end


------------------------------------------------------------
-- main
------------------------------------------------------------
-- We always want the same random sequence so that we can recreate the same scenario every time we test
math.randomseed(12345)

-- Check the settings and keep going if we're good
if checkSettings() then
	-- Initialize our control array
	arrayIndex = 0
	for g = 0, ecGetGroupCount()-1 do
		-- We only care about voice groups (type 1)
		groupType = ecGetGroupType(g)	
		if groupType == 1 then
			_voiceGroupControlArray[arrayIndex] = {groupIndex = g, groupName = ecGetGroupName(g), txOffAt = 0}
			arrayIndex = arrayIndex + 1
			_numberOfGroups = _numberOfGroups + 1
		end
	end

	if _numberOfGroups > 0 then
		-- Wait for a little while to let the system settle
		print("waiting for system to settle ...")
		ecSleep(1000)

		-- Create and join all groups; and wait for them to be ready
		print("creating " .. _numberOfGroups .. " groups ...")
		ecCreateGroup(-1)

		print("joining " .. _numberOfGroups .. " groups ...")
		ecJoinGroup(-1)

		print("waiting for " .. _numberOfGroups .. " groups to be ready ...")
		ecWaitForGroupReady(-1)


		-- Main loop
		_startTickMs = ecGetTickMs()
		lastUiUpdate = 0
		while true do
			-- Current Engage tick
			now = ecGetTickMs()

			-- If we have a runtime limit and we have exceeded it, then break out of the loop
			if (ECTX_RT > 0) and ((now - _startTickMs) >= ECTX_RT) then
				break
			end

			-- For each group ...
			uiUpdateNeeded = false
			for x = 0, #_voiceGroupControlArray do
				-- If tx is off ...
				if _voiceGroupControlArray[x].txOffAt == 0 then
					-- ... and the probability is right, then turn it on for a random amount of time
					v = randRange(0, 100)
					if v <= ECTX_TX_PROB then
						_voiceGroupControlArray[x].txOffAt = now + randRange(ECTX_MIN_TX, ECTX_MAX_TX)
						ecBeginGroupTx(_voiceGroupControlArray[x].groupIndex)
						uiUpdateNeeded = true
					end

				-- tx is on, and the time has come (or already gone) to turn it off, so turn it off
				elseif _voiceGroupControlArray[x].txOffAt <= now then			
					_voiceGroupControlArray[x].txOffAt = 0
					ecEndGroupTx(_voiceGroupControlArray[x].groupIndex)
					uiUpdateNeeded = true
				end
			end

			-- If we haven't updated the UI in a while, then force it
			if (not uiUpdateNeeded) and ((now - lastUiUpdate) > 1000) then
				uiUpdateNeeded = true					
			end
			
			if uiUpdateNeeded then
				lastUiUpdate = now
				updateUi()
			end
			
			ecSleep(500)
		end
	else
		print("**** No voice groups found.  Exiting ... ****")
	end
end

print("\n**** Done! ****\n")