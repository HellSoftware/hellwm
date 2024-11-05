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
monitor("DP-2", 1, 2560, 1440, 120, 0, 1, 1)
monitor("LVDS-1", 1, 1920, 1080, 60, 0, 1, 0)


-- Keyboard Settings
keyboard(
    "Power Button", -- name
    "jp",           -- layout
    59,             -- rate
    221             -- delay
    )
keyboard("default", "us",51,205) -- you can specify 'default' keyboard for all unconfigured keyboards
keyboard("Sleep Button", "de",61,184)
keyboard("ckb1: CORSAIR K68 RGB Mechanical Gaming Keyboard vKB", "pl",50,200)


-- Keybindings
    -- you can bind any program simply just by providing it's name
bind("Super_L, b", "firefox")
bind("Super_L, t", "alacritty")
bind("Super_L, Return", "foot")

    -- there are mapped function that you can use
bind("Super_L, q", "kill_active")
bind("Super_L, c", "focus_next")
bind("Super_L, r", "reload_config")
bind("Super_L, Escape", "kill_server")

    -- you can bind workspace like this
bind("Super_L, 1", "workspace", 1)
bind("Super_L, 2", "workspace", 2)
bind("Super_L, 3", "workspace", 3)
bind("Super_L, 4", "workspace", 4)
bind("Super_L, 5", "workspace", 5)
bind("Super_L, 6", "workspace", 6)
