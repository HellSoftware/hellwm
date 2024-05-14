-- HellWM Config File

-- Keybinds
bind("Escape", "killall hellwm")
bind("p", "pavucontrol")
bind("Return", "kitty")
bind("b", "firefox")
bind("e", "nemo")	
bind("c", focus_next)
bind("q", kill_active)
bind("f", toggle_fullscreen)

-- Keyboard Configuration
keyboard = {
  delay = 200,
  rate = 50,
  layout = "pl"
}

-- Monitors Configuration
monitor_DP_2 = {
  transfrom = 1,
  vrr = true
}

monitor_WL_1 = {
  width = 1000,
  height = 1000,
  vrr = true
}

-- Hello Message :)
print("Welcome to HellWM")
