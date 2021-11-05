#!/usr/bin/env python3

import jinja2
import configparser
import sys
from pathlib import Path
from collections import namedtuple
from enum import Enum, IntEnum

class DataType(IntEnum):
    INTEGER8 = 0x0002
    INTEGER16 = 0x0003
    INTEGER32 = 0x0004
    UNSIGNED8 = 0x0005
    UNSIGNED16 = 0x0006
    UNSIGNED32 = 0x0007

class ObjectType(IntEnum):
    NULL = 0x00
    DOMAIN = 0x02
    DEFTYPE = 0x05
    DEFSTRUCT = 0x06
    VAR = 0x07
    RECORD = 0x08
    ARRAY = 0x09

class AccessType(Enum):
    ReadOnly = "ro"
    WriteOnly = "wo"
    ReadWrite = "rw"
    ReadWriteReadPdo = "rwr"
    ReadWriteWritePdo = "rww"
    Const = "const"

Entry = namedtuple("Entry", "name address data_type access_type pdo_mapping")
Address = namedtuple("Address", "index subindex")


def main():
    argc = len(sys.argv)
    if argc not in (2, 3):
        print("Usage: od_generator [eds filename] [output file]", file=sys.stderr)
        sys.exit(1)

    header_data = generate_data_header(sys.argv[1])
    if argc == 2 or sys.argv[2] == "-":
        sys.stdout.write(header_data)
    else:
        with open(sys.argv[2], "wt") as out:
            out.write(header_data)


def generate_data_header(eds_filename):
    env = create_jinja2_env()
    env.template = env.get_template("od_data.hpp.j2")
    eds = load_eds_file(eds_filename)
    entries = read_all_objects(eds)
    return env.template.render({"entries" : entries, "entry_count" : len(entries)})


def key_to_address(key):
    parts = key.split("sub")
    index = int(parts[0], 16)
    if len(parts) == 2:
        return Address(index, int(parts[1]))
    elif len(parts) > 2:
        raise ValueError("Invalid key '{}'".format(key))
    return Address(index, 0)


def load_eds_file(filename):
    with open(filename, "r") as eds_file:
        data = eds_file.read()
    eds = configparser.ConfigParser()
    eds.read_string(data)
    return eds


def parse_eds_number(string):
    string = string.strip()
    if len(string) == 1:
        return int(string)
    if string[0] == "0":
        if string[1].lower() == "x":
            return int(string, 16)
        else:
            return int(string, 8)
    return int(string)


def read_objects_from_section(eds, section):
    size = parse_eds_number(eds[section]["SupportedObjects"])
    objects = []
    for i in range(1, size + 1):
        index = eds[section][str(i)]
        # normalize key format ("1A00" for 0x1a00)
        key = hex(int(index, 16)).upper()[2:]
        objects += read_object(eds, key)
    return objects


def read_object(eds, key, recursive=True):
    obj = eds[key]
    object_type = ObjectType(parse_eds_number(obj["ObjectType"]))
    if object_type == ObjectType.VAR:
        type_number = parse_eds_number(obj["DataType"])
        try:
            data_type = DataType(type_number)
        except ValueError:
            print("Skipping object {} with unsupported type {}".format(key, type_number), file=sys.stderr)
            return []
        access_type = AccessType(obj["AccessType"])
        mapping = bool(parse_eds_number(obj["PDOMapping"]))
        name = obj["ParameterName"].strip()
        return [Entry(name, key_to_address(key), data_type, access_type, mapping)]
    elif object_type in (ObjectType.RECORD, ObjectType.ARRAY):
        if not recursive:
            raise ValueError("Key {} is not a value".format(key))
        subobject_count = parse_eds_number(obj["SubNumber"]) + 1
        subobjects = []
        for subindex in range(0, subobject_count):
            subkey = key + "sub" + str(subindex)
            if subkey in eds:
                subobjects.append(read_object(eds, subkey, False)[0])
        return subobjects
    else:
        raise ValueError("Unsupported object type '{}'".format(str(object_type)))


def read_all_objects(eds):
    mandatory_objects = read_objects_from_section(eds, "MandatoryObjects")
    optional_objects = read_objects_from_section(eds, "OptionalObjects")
    manufacturer_objects = read_objects_from_section(eds, "ManufacturerObjects")
    entries = mandatory_objects + optional_objects + manufacturer_objects
    check_entries(entries)
    return entries


data_type_map = {
    DataType.INTEGER8 : "Int8",
    DataType.INTEGER16 : "Int16",
    DataType.INTEGER32 : "Int32",
    DataType.UNSIGNED8 : "UInt8",
    DataType.UNSIGNED16 : "UInt16",
    DataType.UNSIGNED32 : "UInt32"
}

def convert_data_type(eds_type):
    return "DataType::" + data_type_map[eds_type]


access_type_map = {
    AccessType.ReadOnly : "ReadOnly",
    AccessType.WriteOnly : "WriteOnly",
    AccessType.ReadWrite : "ReadWrite",
    AccessType.ReadWriteReadPdo : "ReadWriteReadPdo",
    AccessType.ReadWriteWritePdo : "ReadWriteWritePdo",
    # TODO: 'Const' is not supported yet
    AccessType.Const : "ReadOnly"  # "Const"
}

def convert_access_type(eds_type):
    return "AccessType::" + access_type_map[eds_type]


def check_entries(entries):
    address_set = set()
    for entry in entries:
        if entry.access_type == AccessType.ReadWrite and entry.pdo_mapping:
            raise ValueError("Value 0x{:x}:{} of access type 'rw' can't be mappable, use 'rwr' or 'rwr'"
                             .format(entry.address.index, entry.address.subindex))
        if entry.address in address_set:
            raise ValueError("Duplicate address 0x{:x}:{}".format(entry.address.index, entry.address.subindex))
        address_set.add(entry.address)


def create_jinja2_env():
    loader = jinja2.FileSystemLoader(Path(__file__).resolve().parent)
    env = jinja2.Environment(loader=loader)
    env.line_statement_prefix = "%%"
    env.filters["hex"] = lambda number: hex(int(number))
    env.filters["data_type"] = convert_data_type
    env.filters["access_type"] = convert_access_type
    return env


if __name__ == "__main__":
    main()
