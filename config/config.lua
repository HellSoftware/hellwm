-- My HellWM Config File 
-- Example config to see how to get things working :)

-- Keybinds
bind("Escape", "killall hellwm")
bind("p", "pavucontrol")
bind("Return", "alacritty")
bind("b", "firefox")
bind("e", "nemo")	
bind("o", "fuzzel")
bind("c", focus_next)
bind("q", kill_active)
bind("f", toggle_fullscreen)
bind("l", "libreoffice")
bind("0", "0ad")

decoration = {
  window_decoration_mode = 1
}

-- Keyboard Configuration
keyboard = {
  delay = 200,
  rate = 50,
  layout = "pl"
}

-- Monitors Configuration
monitor_DP_1 = {
  x = 0,
  y = 0
}

monitor_DP_2 = {
  x = 2560,
  y = -300,
  scale = 1,
  transfrom = 1,
  vrr = true
}

monitor_WL_1 = {
  width = 1000,
  height = 1000,
  vrr = true
}

-- Environment
env("GTK_THEME", "Adwaita:dark")

-- Hello Message :)
print("Welcome to HellWM")

loop = function()
  print ("Hello World!")
  sleep(1)
end
