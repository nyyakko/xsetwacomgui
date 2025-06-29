import os
import sys
import configparser

def configure_desktop_metadata(prefix):
    meta = configparser.ConfigParser()
    meta.optionxform = str

    meta['Desktop Entry'] = {
        'Version': '1.0',
        'Type': 'Application',
        'Name': 'XSetWacomGUI',
        'Comment': 'A graphical xsetwacom wrapper for ease of use',
        'Exec': f'{prefix}/bin/xsetwacomgui',
        'Icon': 'xsetwacomgui.png',
        'Terminal': 'false',
        'Categories': 'Wacom;Drawing;Graphics;Utility;'
    }

    return meta

def install_desktop_metadata(prefix):
    meta = configure_desktop_metadata(prefix)
    file = open(f'{prefix}/share/applications/xsetwacomgui.desktop', 'w+')
    meta.write(file)
    file.close()

def main(arguments):
    prefix = "~/.local"

    if len(arguments):
        prefix = arguments[0]

    os.system(f'cmake --install build --prefix {prefix}')
    install_desktop_metadata(os.path.expanduser(prefix))

if __name__ == "__main__":
    sys.argv.pop(0)
    main(sys.argv)
