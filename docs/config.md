# Config Guide 

To make your own config edit config/config.lua for your needs.
You don't have to manually set every value provided bellow,
for example you can just set only layout:
```
keyboard = {
    layout = "us"
}
```


# monitor 

Name: monitor_OUTPUT

Keywords:
- width [int]
- height [int]
- refresh_rate [int]
- scale [int]
- transfrom [int]

Example for DP-1:
```
monitor_DP_1 = {
   width = 2560,
   height = 1440,
   refresh_rate = 165
}
```

# keyboard

Name: keyboard

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
