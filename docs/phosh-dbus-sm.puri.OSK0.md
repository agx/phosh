Title: sm.puri.OSK0 DBus Interface
Slug: sm.puri.OSK0

# sm.puri.OSK0

## Description



This interface is exported by on screen keyboards (OSK) to
indicate and set state like visibility.



### Properties
### sm.puri.OSK0:Visible

```
    Visible readable b
```

Indicates whether the on screen keyboard is currently
unfolded. `TRUE` if the OSK is currently visible to the user,
`FALSE` otherwise.



### Methods
### sm.puri.OSK0.SetVisible

```
    SetVisible (
      IN visible b
    )
```

Toggle keyboard visibility.



* visible: `TRUE` if the OSK should be shown to the user, `FALSE` otherwise


