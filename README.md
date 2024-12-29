# HellWM Wayland Compositor

## Why?
I got inspired by Vaxry, I thoguht that if someone from my country could create 'base' for Hyprland in a month, I should try writing my own WaylandCompositor too - it can't be that hard... right? (Oh god, how mistaken I was).

That being said, it kinda works after rewriting it 3 times ;P

## The hell is HellWM?

HellWM is meant to be very light and modular Wayland Compositor with some unique features, like:
- **binary workspaces**
- **config hot reload** (not really unique rn)
- **lua**

Still under heavy development, open for contributions and ideas that will make this project better :)

## Features

<details>
  <summary>Binary Workspaces (Click to expand)</summary>

## You can use less amount of keys to switch between workspaces!

Concept:

You can switch between **15 workspaces** just by using combinations of only **4 keys**!

Each key represents a binary value, and by combining them, you can achieve more funtionality with less clicks!

Keys and Binary Values:

    Key 1:
      1 = 0001
    
    Key 2:
      2 = 0010
    
    Key 3
      4 = 0100
    
    Key 4
      8 = 1000

By adding the binary values of the pressed keys, you determine the workspace number.
Examples:

    Pressing Key 1 and Key 3 together:
        
        Key 1 + Key 3 = 0001 + 0100 = 0101
        Result: 1 0 1 0
  
        Binary Values: 1 (Key 1) + 4 (Key 3) = 5
        Workspace: 5

    Pressing only Key 2:
        Key 2 = 0 1 0 0
        Result = 0 1 0 0
        
        Binary Values: 2 (Key 2) = 2
        Workspace: 2

Configuration:

Here’s how the configuration works:

#### Normal Workspaces: You can assign individual workspaces to single keys. For example:

```lua
bind(
    "Super_L, 1",   -- keys
    "workspace",    -- specify that it is a workspace
    1,              -- workspace number
    false,          -- binary workspaces enabled
    1,              -- binary workspaces value
    true            -- also move active window to this workspace (not working rn)
    )
```

```lua
bind("Super_L, 2", "workspace", 2, false, 0) -- Switch to workspace 2
```

#### Binary Workspaces: If you enable binary mode, keys combine to generate workspace numbers:

```lua
bind("Super_L, u", "workspace", 1, true, 1, true)  -- Key 'u' represents Binary 1
```

```lua
bind("Super_L, i", "workspace", 2, true, 2, false) -- Key 'i' represents Binary 2
```

```lua
bind("Super_L, o", "workspace", 3, true, 4, false) -- Key 'o' represents Binary 4
```

```lua
bind("Super_L, p", "workspace", 4, true, 8, false) -- Key 'p' represents Binary 8
```

  Binary mode enabled (true): Combines key presses to calculate the workspace number.
  
  Binary values (1, 2, 4, 8) determine which binary workspace is triggered.
    
</details>

## TODO:
- [ ] xwayland

## DONE:
- [x] config
- [x] tiling
- [x] borders
- [x] workspaces
- [x] wlr-layer-shell
- [x] binary workspaces
- [x] optimization in output_frame

# Special thanks

Thank you all for showing how 2 do stuff <3

- https://codeberg.org/dwl/dwl/
- https://github.com/swaywm/sway/
- https://github.com/buffet/kiwmi/
- https://github.com/dqrk0jeste/owl/
- https://github.com/hyprwm/Hyprland/
- https://github.com/inclement/vivarium/
