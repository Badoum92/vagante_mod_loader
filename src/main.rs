use flate2::bufread::{ZlibDecoder, ZlibEncoder};
use flate2::Compression;
use std::convert::TryInto;
use std::fs::{self, File};
use std::io::prelude::*;
use std::io::{self, BufRead};
use std::path::{Path, PathBuf};

fn u32_from_bytes(bytes: &[u8]) -> u32 {
    let (value_bytes, _) = bytes.split_at(std::mem::size_of::<u32>());
    u32::from_le_bytes(value_bytes.try_into().unwrap())
}

fn u16_from_bytes(bytes: &[u8]) -> u16 {
    let (value_bytes, _) = bytes.split_at(std::mem::size_of::<u16>());
    u16::from_le_bytes(value_bytes.try_into().unwrap())
}

fn unpack(path: &Path) -> Result<(), io::Error> {
    println!("Loading data.vra");
    let mut data_vra = File::open(path)?;
    let mut raw_data = Vec::new();
    data_vra.read_to_end(&mut raw_data)?;

    println!("Decoding data.vra");
    let mut decoder = ZlibDecoder::new(&raw_data[..]);
    let mut data: Vec<u8> = Vec::new();
    match decoder.read_to_end(&mut data) {
        Ok(_) => {}
        Err(_) => {
            return Err(io::Error::new(
                io::ErrorKind::InvalidInput,
                "input is not a vaild data.vra",
            ))
        }
    };

    let nb_files = u16_from_bytes(&data[4..6]) + 3;

    let start_pos = 6;
    let mut nb_bytes = 0;
    let base_path = Path::new("_data");
    fs::create_dir_all(base_path)?;
    let mut repack_list = File::create(base_path.join(Path::new("repack_list.txt")))?;
    writeln!(repack_list, "{}", nb_files)?;

    println!("Unpacking data.vra");
    let mut percent;
    print!("0%");

    for i in 0..nb_files {
        let file_name_length = u32_from_bytes(&data[start_pos + nb_bytes..]) as usize;
        nb_bytes += 4;
        let name_range = start_pos + nb_bytes..start_pos + nb_bytes + file_name_length;
        let file_name = std::str::from_utf8(&data[name_range]).unwrap();
        nb_bytes += file_name_length;
        let file_length = u32_from_bytes(&data[start_pos + nb_bytes..]) as usize;
        nb_bytes += 4;
        let data_range = start_pos + nb_bytes..start_pos + nb_bytes + file_length;
        let file_data = &data[data_range];
        nb_bytes += file_length;

        let path = base_path.join(Path::new(file_name));
        fs::create_dir_all(path.parent().unwrap())?;
        let mut file = File::create(path)?;
        file.write_all(file_data)?;

        writeln!(repack_list, "{}", file_name)?;

        percent = ((i + 1) as f32) * 100.0 / (nb_files as f32);
        print!("\r{}%", percent as u32);
    }

    println!("\r100%");

    Ok(())
}

fn repack(path: &Path) -> Result<(), io::Error> {
    let repack_list = File::open(path.join("repack_list.txt"))?;
    let mut lines = io::BufReader::new(repack_list).lines();

    let mut data: Vec<u8> = Vec::new();
    data.extend_from_slice(&[0, 0, 0, 0]);

    let nb_files = (lines.nth(0).unwrap()?).parse::<u16>().unwrap();
    data.extend_from_slice(&(nb_files - 3).to_le_bytes());

    println!("Repacking data.vra");
    let mut percent;
    let mut i = 0;
    print!("0%");

    for line in lines {
        let file_name = line?;
        data.extend_from_slice(&(file_name.len() as u32).to_le_bytes());
        data.extend_from_slice(file_name.as_bytes());

        let file_path = path.join(file_name);
        let metadata = fs::metadata(&file_path)?;
        data.extend_from_slice(&(metadata.len() as u32).to_le_bytes());
        let mut file = File::open(&file_path)?;
        file.read_to_end(&mut data)?;

        percent = ((i + 1) as f32) * 100.0 / (nb_files as f32);
        print!("\r{}%", percent as u32);
        i += 1;
    }

    println!("\r100%");

    println!("Encoding data.vra");
    let mut encoder = ZlibEncoder::new(&data[..], Compression::fast());
    let mut encoded = Vec::new();
    encoder.read_to_end(&mut encoded)?;

    println!("Writing data.vra");
    let mut data_vra = File::create("new_data.vra")?;
    data_vra.write_all(&encoded[..])?;

    Ok(())
}

fn get_mods() -> Option<Vec<PathBuf>> {
    let mods_dir = Path::new("mods");
    if !mods_dir.is_dir() {
        return None;
    }

    let entries = fs::read_dir(mods_dir);

    if entries.is_err() {
        return None;
    }

    let mut mods = Vec::new();

    for entry in entries.unwrap() {
        let entry = entry.unwrap();
        let path = entry.path();

        if path.is_dir() {
            mods.push(path.to_path_buf());
        }
    }

    return Some(mods);
}

fn load_mod(dir: &Path) -> io::Result<()> {
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let path = entry.path();
        if path.is_dir() {
            load_mod(&path)?;
        } else {
            let mut ancestors = dir.ancestors();
            while ancestors.count() != 3 {
                ancestors.next();
            }
            let file_path = path.strip_prefix(ancestors.next().unwrap()).unwrap();
            let parent = file_path.parent();
            let data_path = Path::new("_data");
            if parent.is_some() {
                fs::create_dir_all(data_path.join(parent.unwrap()))?;
            }
            fs::copy(&path, data_path.join(file_path))?;
        }
    }
    Ok(())
}

fn load_mods(arg: &Path) -> Result<(), io::Error> {
    let mods = get_mods().unwrap_or_default();
    if mods.is_empty() {
        println!("No mods found, exiting.");
        return Err(io::Error::new(io::ErrorKind::InvalidInput, "no mods found"));
    }

    println!("Mods to load (will be loaded in alphabetical order, rename them if you want to change the order):\n");
    let mut i = 1;
    for m in &mods {
        println!("{}. {}", i, m.to_str().unwrap());
        i += 1;
    }
    print!("\nLoad mods? [Y,n] ");
    io::stdout().flush()?;

    loop {
        let mut input = String::new();
        io::stdin().read_line(&mut input).unwrap();
        input = input.to_lowercase();
        let to_remove: &[_] = &['\n', '\r'];
        let trimmed_input = input.trim_end_matches(to_remove);
        if trimmed_input == "" || trimmed_input == "y" {
            break;
        } else if trimmed_input == "n" {
            return Ok(());
        } else {
            print!("(Invalid input: {}) Load mods? [Y,n] ", trimmed_input);
            io::stdout().flush()?;
        }
    }

    println!("- - - - - - - - - - - - - - - - - - - -");
    unpack(&arg)?;

    println!("- - - - - - - - - - - - - - - - - - - -");
    for dir in mods {
        print!(
            "Loading mod: {}: ",
            dir.file_name().unwrap().to_str().unwrap()
        );
        io::stdout().flush().unwrap();

        match load_mod(dir.as_path()) {
            Ok(_) => println!("Ok"),
            Err(_) => println!("Error"),
        }
    }

    println!("- - - - - - - - - - - - - - - - - - - -");
    repack(Path::new("_data"))?;

    println!("- - - - - - - - - - - - - - - - - - - -");

    Ok(())
}

fn print_menu() {
    println!("1. Load mods (requires data.vra)");
    println!("2. Unpack (requires data.vra)");
    println!("3. Repack (requires unpacked directory)");
    println!("4. Help");
    println!("5. Exit");
}

fn main() -> Result<(), io::Error> {
    let arg = std::env::args().nth(1);

    print_menu();
    print!("Selection: ");
    io::stdout().flush()?;

    loop {
        let mut input = String::new();
        io::stdin().read_line(&mut input).unwrap();
        let n: u32 = input.trim().parse().unwrap_or(0);

        match n {
            1 => {
                if arg.is_none() {
                    println!("No data.vra file provided, exiting.");
                    std::process::exit(1);
                }
                println!("- - - - - - - - - - - - - - - - - - - -");
                match load_mods(Path::new(&arg.unwrap())) {
                    Ok(_) => {}
                    Err(e) => {
                        println!("Could not load mods: {}", e);
                    }
                }
                break;
            }
            2 => {
                if arg.is_none() {
                    println!("No data.vra file provided, exiting.");
                    std::process::exit(1);
                }
                println!("- - - - - - - - - - - - - - - - - - - -");
                match unpack(Path::new(&arg.unwrap())) {
                    Ok(_) => {}
                    Err(e) => {
                        println!("Could not unpack: {}", e);
                    }
                }
                break;
            }
            3 => {
                if arg.is_none() {
                    println!("No unpacked directory provided, exiting.");
                    std::process::exit(1);
                }
                println!("- - - - - - - - - - - - - - - - - - - -");
                match repack(Path::new(&arg.unwrap())) {
                    Ok(_) => {}
                    Err(e) => {
                        println!("Could not repack: {}", e);
                    }
                }
                break;
            }
            4 => {
                println!("\n");
                println!("To load mods place your mod directories into a directory called `mods`.");
                println!("Make sure to order them alphabetically in the order you want them to be loaded.");
                println!("Then drag and drop a valid `data.vra` file on this program.");
                println!("A new data.vra file will be generated. You can simply put it in your vagante install directory.");

                println!("\n");
                println!("To unpack a `data.vra` file drag and drop a valid `data.vra` file on this program");
                println!("A new `_data` directory containing the unpacked data will be generated.");

                println!("\n");
                println!("To repack a `data.vra` file drag and drop a directory (unpacked by this program) on this program");
                println!("A new `new_data.vra` file will be generated.");

                println!("\n");
                print_menu();
                print!("Selection: ");
                io::stdout().flush()?;
            }
            5 => {
                break;
            }
            _ => {
                print!("(Invalid input) Selection: ");
                io::stdout().flush()?;
            }
        }
    }

    println!("Press enter to exit.");
    let mut input = String::new();
    io::stdin().read_line(&mut input).unwrap();

    Ok(())
}
