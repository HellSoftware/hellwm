# Config Guide 

To make your own config edit config/config.lua for your needs.
You don't have to manually set every value provided bellow,
set only those you need to, rest will be set by default.

# Keybinds

Name: bind(key, action)

Example if you want on Meta Key + Return execute kitty termulator:
```
bind("Return","kitty")
```

# Monitor 

Name: ***monitor_OUTPUT***

Keywords:
- width [int]
- height [int]
- hz [int]
- scale [int]
- transfrom [int]

Example for DP-1:
```
monitor_DP_1 = {
   width = 2560,
   height = 1440,
   hz = 165
}
```

# Keyboard

Name: ***keyboard***

Keywords:
- rules [char*]
- model [char*]
- layout [char*]
- variant [char*]
- options [char*]
- repeat [int]
- delay [int]

Example:
```
keyboard = {
   repeat = 200,
   delay = 50,
   layout = "us"
}
```
