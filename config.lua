-- Monitor Settings
monitor("DP-1",2560,1440,120,1,0)
monitor("DP-2",2560,1440,120,1,1)
monitor("LVDS-1",1920,1080,60,1,0)

-- Keyboard Settings
keyboard("default", "us",51,205)
keyboard("Power Button", "jp",59,221)
keyboard("Sleep Button", "de",61,184)
keyboard("ckb1: CORSAIR K68 RGB Mechanical Gaming Keyboard vKB", "pl",50,200)

-- Keybindings
bind("Super_L, Escape", "kill_server")
bind("Super_L, q", "kill_active")
bind("Super_L, c", "focus_next")
bind("Super_L, t", "alacritty")
bind("Super_L, b", "firefox")
bind("Super_L, Return", "foot")
bind("Super_L, r", "reload_config")

bind("Super_L, 1", "ch1")
bind("Super_L, 2", "ch2")
bind("Super_L, 3", "ch3")
