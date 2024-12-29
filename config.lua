--[[
    _   _      _ ___        ____  __     ____             __ _       
   | | | | ___| | \ \      / /  \/  |   / ___|___  _ __  / _(_) __ _ 
   | |_| |/ _ \ | |\ \ /\ / /| |\/| |  | |   / _ \| '_ \| |_| |/ _` |
   |  _  |  __/ | | \ V  V / | |  | |  | |__| (_) | | | |  _| | (_| |
   |_| |_|\___|_|_|  \_/\_/  |_|  |_|   \____\___/|_| |_|_| |_|\__, |
                                                               |___/ 
]]

-- Monitor Settings
monitor(
    "DP-1", -- name
    true,   -- enabled
    2560,   -- width
    1440,   -- height
    120,    -- hz
    1,      -- adaptive sync enabled
    1,      -- scale
    0       -- transfrom
    )
monitor("DP-2", false)--, 2560, 1440, 120, 0, 1, 1)
monitor("LVDS-1", true, 1920, 1080, 60, 0, 1, 0)


-- Keyboard Settings
keyboard(
    "Power Button", -- name
    "jp",           -- layout
    59,             -- rate
    221             -- delay
    )
keyboard("default", "us",51,205)
keyboard("Sleep Button", "de",61,184)
keyboard("ckb1: CORSAIR K68 RGB Mechanical Gaming Keyboard vKB", "pl",50,200)


-- Keybindings
    
bind(
    "Super_L, b",     -- keys (for now you can only use combination of max 2
    "firefox"         -- program name
    )
bind("Super_L, Return", "alacritty")
bind("Super_L, F12", "pavucontrol")
bind("Super_L, F11", "grim")
bind("Super_L, w", "setwall")
bind("Super_L, v", "vesktop")
bind("Super_L, e", "wofi --show drun")

    -- Screen Brightness
bind("Alt_L, XF86AudioRaiseVolume", "brightnessctl s +1%")
bind("Alt_L, XF86AudioLowerVolume", "brightnessctl s 1%-")

    -- Volume Controls
bind("XF86AudioRaiseVolume", "wpctl set-volume -l 1.5 @DEFAULT_AUDIO_SINK@ 5%+")
bind("XF86AudioLowerVolume", "wpctl set-volume -l 1.5 @DEFAULT_AUDIO_SINK@ 5%-")
bind("XF86AudioMute", "wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle")
bind("XF86AudioPlay", "playerctl play-pause")
bind("XF86AudioNext", "playerctl next")
bind("XF86AudioPrev", "playerctl previous")
bind("XF86AudioStop", "playerctl stop")

    -- Screenshots
bind("Super_L, Shift_L, S", "grim -g \"$(slurp -d)\" - | tee >(swappy -f - -o - | wl-copy) | wl-copy")


bind(
    "Super_L, 1",   -- keys
    "workspace",    -- specify that it is a workspace
    1,              -- workspace number
    false,          -- binary workspaces enabled
    1,              -- binary workspaces value
    true            -- also move active window to this workspace
    )
bind("Super_L, 2", "workspace", 2, false, 0)
bind("Super_L, 3", "workspace", 3, false, 0)
bind("Super_L, 4", "workspace", 4, false, 0)
bind("Super_L, 5", "workspace", 5, false, 0)
bind("Super_L, 6", "workspace", 6, false, 0)

-- Binary workspaces
bind("Super_L, u", "workspace", 1, true, 1)
bind("Super_L, i", "workspace", 2, true, 2)
bind("Super_L, o", "workspace", 3, true, 4)
bind("Super_L, p", "workspace", 4, true, 8)


bind("Super_L, r", "reload_config")     -- Reload config

bind("Super_L, f", "set_fullscreen")     -- Fullscreens active toplevel

bind("Super_L, q", "kill_active")       -- Kill Active Window
bind("Super_L, Escape", "kill_server")  -- Kill HellWM

bind("Super_L, n", "focus_next")        -- Focus next Window

bind("Super_L, h", "focus_left")
bind("Super_L, l", "focus_right")
bind("Super_L, k", "focus_up")
bind("Super_L, j", "focus_down")
bind("Super_L, m", "toggle_split") --

-- Set environment variables
env("EDITOR", "nvim")
env("GTK_THEME", "Nordic")

-- Exec daemons etc.
exec("swww init")
exec("waybar")
exec("alacritty")

-- Tiling layout -- no matter what only 'dwindle' for now on
layout(3)

-- Decoration
border_width(0)
inner_gap(0)
outer_gap(0)

fade_duration(0.3)

animation_duration(.4)
animation_bezier(0.0, 1.12, 1.28, 1)

--animation_duration(1.4)
--animation_bezier(0.1, 1.12, -0.5, 1) -- Thats funny af, try it :D

    -- Types of animation directions
--animation_direction("left")
--animation_direction("right")
--animation_direction("up")
--animation_direction("down")
--animation_direction("shrink")
animation_direction("grow")


    -- Border colors
border_inactive_color("#87554c")
border_active_color("233, 23, 52, 128")

    -- Border colors imported from Hellwal
--dofile(os.getenv("HOME") .. "/.cache/hellwal/hellwm.lua")
--border_inactive_color(background)
--border_active_color(foreground)
