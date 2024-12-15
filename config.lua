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
    "Super_L, b",     -- keys
    "firefox"         -- program name
    )
bind("Super_L, Return", "foot")
bind("Super_L, t", "alacritty")
bind("Super_L, F12", "pavucontrol")
bind("Super_L, w", "setwall")


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

bind("Super_L, q", "kill_active")       -- Kill Active Window
bind("Super_L, Escape", "kill_server")  -- Kill HellWM

bind("Super_L, n", "focus_next")        -- Focus next Window
bind("Alt_L, n", "focus_next_center")   -- Focus next and center Window
--bind("Super_L, p", "focus_prev")
--bind("Alt_L, p", "focus_prev_center")


-- Set environment variables
env("EDITOR", "nvim")
env("GTK_THEME", "Adwaita")

-- Exec daemons etc.
exec("swww init ; swww img ~/wallpapers/anime-girls-balloon-women-sky-wallpaper-659852899b3ce30278d24f855a4395e8.jpg")
exec("waybar")
exec("alacritty")
