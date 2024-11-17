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
bind("Super_L, t", "alacritty")
bind("Super_L, Return", "foot")


bind(
    "Super_L, 1",   -- keys
    "workspace",    -- specify that it is a workspace
    1,              -- workspace number
    true,           -- binary workspaces enabled
    1               -- binary workspaces value
    )
bind("Super_L, 2", "workspace", 2, true, 2)
bind("Super_L, 3", "workspace", 3, true, 4)
bind("Super_L, 4", "workspace", 4, true, 8)
bind("Super_L, 5", "workspace", 5)
bind("Super_L, 6", "workspace", 6)


bind("Super_L, q", "kill_active")
bind("Super_L, c", "focus_next")
bind("Super_L, r", "reload_config")
bind("Super_L, Escape", "kill_server")

env("GTK", "NONE")
