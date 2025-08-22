# Listening Device Connections

Because its annoying to have to reopen xsetwacomgui to have the settings
applied every time you reconnect your device, you might wanna start the
application with the `--no-gui` flag.

```bash
xsetwacomgui --no-gui
```

passing this flag, the program will start a daemon which will listen for new
device connections and apply the settings accordingly without you having to
open xsetwacomgui manually.

> [!NOTE]
> The daemon will write its logs to /dev/log, so you can stat it there if you
> wish to do so.
