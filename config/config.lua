-- Config File

-- Keybinds
bind("Escape","killall hellwm")
bind("Return","kitty")
bind("b", "firefox")
bind("e", "nemo")	
bind("p", "pavucontrol");
bind("c", focus_next)
bind("q", kill_active)

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
  width = 500,
  height = 500,
  vrr = true
}

-- Hello Message :)
print("Welcome to HellWM")
