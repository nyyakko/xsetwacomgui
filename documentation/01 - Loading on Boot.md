# Loading on Boot

Since the `xsetwacom` utility itself does not have a way to persist \
configuration, `xsetwacomgui` gives a way to do that. You just need to invoke \
the following command on boot:

```bash
xsetwacomgui --no-gui
```

which will load the saved configuration.

> [!IMPORTANT]
> Beware that this command only works if you already have a configuration saved
> to begin with.
