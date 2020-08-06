# Vagante Mod Loader

Simple mod loader for vagante. Mods need to be provided "as is" (not packed into a `data.vra`).
Can also be used to unpack and repack `data.vra` files.

## Usage

### Loading mods

* Place all of your mods in a `mods` directory
* Order them alphabetically (this is the order in which they will be loaded)
* Drag and drop a valid (preferably default) `data.vra` on `vagante_mod_loader.exe`
* Select the `Load mods` option
* A `_data` directory containing the unpacked data will be generated
* A `new_data.vra` will be generated
* Place the `new_data.vra` in your vagante install directory and rename it to `data.vra`

### Unpacking

* Drag and drop a valid `data.vra` on `vagante_mod_loader.exe`
* A `_data` directory containing the unpacked data will be generated

### Repacking

* Drag and drop an unpacked folder on `vagante_mod_loader.exe`, it MUST have been unpacked using this program
* A `new_data.vra` will be generated