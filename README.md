<div align="center">
<h1>nes-tools</h1>

<img src="https://github.com/user-attachments/assets/225ad035-acc0-4ba5-a257-18f559e1dddb" width="50%" height="50%" />

NES emulator toolchain based on [ObaraEmmanuel/NES](https://github.com/ObaraEmmanuel/NES).

⚠️⚠️BEWARE OF BUGS⚠️⚠️
</div>

## Dependencies
This project requires the [SDL2](https://www.libsdl.org/) library to be installed in a searchable path (i.e. within your OS `PATH` variable). Additionally, you'll need a C compiler and [Make](https://www.gnu.org/software/make/).

## Install

Clone the repository
```
https://github.com/zeim839/nes-tools.git
```

Navigate to the project directory and run the configuration script
```
cd nes-tools
./configure
```

Build and install
```
make install
```

Run the emulator
```
nes-tools help
```

### Uninstall
```
make uninstall
```

## Usage

The emulator is currently designed to support mapper #0 game ROMs (see [NES Directory](https://nesdir.github.io/mapper0.html) for a complete list of supported games), which you'll need to install independently. To run a ROM, call the `nes-tools` executable as follows:
```
nes-tools run [path/to/rom/]
```

nes-tools supports [iNES](https://www.nesdev.org/wiki/INES) format cartridge ROMs.

## Contributing
All contributions are welcome and appreciated. Open an issue before proposing significant changes.

## License
[MIT](LICENSE)
