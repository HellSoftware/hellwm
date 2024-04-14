# Config Usage 

```GROUP = KEYWORD, VALUE, ...```

Groups {
    
    source {
        Value=config path
        
        Example:
            source=~/.config/hellwm/keyboard.conf
    }

    bind {
        Keyword = KEY
        Value = shell command
        
        Example:
            bind=SUPER+Return,kitty
    }
    monitor {
        Keyword = DISPLAY NAME (example DP-1)
        Value = width, height, refresh_rate, scale, transfrom 
        
        Example:
            monitor=1920,1080,165,1,0
    }
    keyboard {
        Keyword:
            - rules [char*]
            - model [char*]
            - layout [char*]
            - variant [char*]
            - options [char*]
            - repeat [int]
            - delay [int]
        Value = Datatype value 
        
        Example:
            keyboard=layout,us
    }

}
