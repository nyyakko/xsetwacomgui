# Loading settings on boot

Since the xsetwacom command does not persist across sessions, you may want to\
configure it on system boot. To do so you just need to invoke the following\
command on boot:

```bash
xsetwacomgui --no-gui
```

Beware that this command only works if you already have a configuration saved\
to begin with.
