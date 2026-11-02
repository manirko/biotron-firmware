#!/usr/bin/env python3
"""Inspect on-disk RP2040 UF2 bytes; never opens a USB device."""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def require(ok, message):
    if not ok:
        raise ValueError(message)


def decode(data):
    require(data and len(data) % 512 == 0, 'UF2 must contain whole blocks')
    count = len(data) // 512
    chunks = {}
    for index in range(count):
        block = data[index * 512:(index + 1) * 512]
        magic0, magic1, flags, address, size, number, total, family = struct.unpack_from('<8I', block)
        require((magic0, magic1, struct.unpack_from('<I', block, 508)[0]) ==
                (0x0A324655, 0x9E5D5157, 0x0AB16F30), 'UF2 magic')
        require(flags == 0x2000 and family == 0xE48BFF56, 'RP2040 family/flags')
        require(size == 256 and number == index and total == count, 'UF2 block sequence/size')
        require(0x10000000 <= address and address + size <= 0x10080000,
                'UF2 reaches outside firmware area or touches settings')
        require(address % 256 == 0 and address not in chunks, 'UF2 address overlap/alignment')
        chunks[address] = block[32:32 + size]
    addresses = sorted(chunks)
    require(addresses == list(range(0x10000000, 0x10000000 + count * 256, 256)),
            'UF2 flash image is not contiguous from flash base')
    return b''.join(chunks[a] for a in addresses)


def inspect(data, layout):
    payload = decode(data)
    stack, reset = struct.unpack_from('<II', payload, 256)
    require(0x20000000 <= stack <= 0x20042000 and stack % 8 == 0, 'RP2040 stack vector')
    require(reset & 1 and 0x10000100 <= (reset & ~1) < 0x10000000 + len(payload), 'reset vector')
    triple = bytes([239, 2, 1] if layout == 'fixed' else [0, 0, 0])
    device = bytes([18, 1, 0, 2]) + triple + bytes([64, 254, 202, 17, 48, 0, 1, 1, 2, 3, 1])
    require(payload.count(device) == 1, 'unique Biotron CAFE/3011 device descriptor')
    signature = bytes([9, 2, 199, 0, 4, 1, 0, 128, 50])
    require(payload.count(signature) == 1, 'unique 199-byte/4-interface/100mA configuration')
    offset = payload.index(signature)
    config = payload[offset:offset + 199]
    descriptors = []
    pos = 0
    while pos < len(config):
        length = config[pos]
        require(length >= 2 and pos + length <= len(config), 'descriptor boundary')
        descriptors.append(config[pos:pos + length])
        pos += length
    interfaces = [list(d[2:8]) for d in descriptors if d[1] == 4]
    expected = [[0, 0, 0, 1, 1, 0], [1, 0, 2, 1, 3, 0],
                [2, 0, 1, 2, 2, 0], [3, 0, 2, 10, 0, 0]]
    if layout == 'official':
        expected = expected[2:] + expected[:2]
    require(interfaces == expected, 'interface order/classes/endpoints')
    iad = [list(d) for d in descriptors if d[1] == 11]
    require(iad == [[8, 11, 2, 2, 2, 2, 0, 0]], 'CDC association 2/3')
    endpoints = sorted((d[2], d[3], struct.unpack_from('<H', d, 4)[0])
                       for d in descriptors if d[1] == 5)
    require(endpoints == [(1, 2, 64), (2, 2, 64), (129, 2, 64), (130, 2, 64), (131, 3, 8)],
            'MIDI/CDC endpoint addresses, types and packet sizes')
    jack_ids = [d[4] for d in descriptors if d[1] == 36 and d[2] in (2, 3) and len(d) in (6, 9)]
    require(sorted(jack_ids) == list(range(1, 9)), 'two MIDI cables, eight unique jacks')
    associations = [list(d[3:]) for d in descriptors if d[1] == 37]
    require(associations == [[2, 1, 5], [2, 3, 7]], 'MIDI cable endpoint jack associations')
    require(b'Biotron\x00' in payload and b'Playtronica\x00' in payload, 'product/manufacturer strings')
    report = dict(sha256=hashlib.sha256(data).hexdigest(), size_bytes=len(data),
                  blocks=len(data) // 512, payload_bytes=len(payload), family='RP2040',
                  flash_start='0x10000000', flash_end_exclusive=hex(0x10000000 + len(payload)),
                  settings_start='0x10080000', settings_not_touched='PASS',
                  usb_device_address=hex(0x10000000 + payload.index(device)),
                  usb_configuration_address=hex(0x10000000 + offset),
                  vid='0xcafe', pid='0x3011', device_class_subclass_protocol=list(triple),
                  configuration_bytes=len(config), interface_order=[i[0] for i in interfaces],
                  endpoints=endpoints, midi_virtual_cables=2, cdc='retained', power_ma=100,
                  structure_and_descriptors='PASS')
    return report, device, config


def selftest():
    # Valid synthetic UF2 envelope, then corrupted family and settings address.
    block = bytearray(512)
    struct.pack_into('<8I', block, 0, 0x0A324655, 0x9E5D5157, 0x2000,
                     0x10000000, 256, 0, 1, 0xE48BFF56)
    struct.pack_into('<I', block, 508, 0x0AB16F30)
    require(len(decode(bytes(block))) == 256, 'selftest valid envelope')
    for offset, value in [(28, 0), (12, 0x10080000), (20, 1), (16, 512), (508, 0)]:
        invalid = bytearray(block)
        struct.pack_into('<I', invalid, offset, value)
        try:
            decode(bytes(invalid))
        except ValueError:
            continue
        raise ValueError('selftest failed to reject corrupt UF2')
    print('PASS: UF2 envelope negative checks')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('uf2', nargs='?', type=Path)
    parser.add_argument('--layout', choices=['fixed', 'official'], default='fixed')
    parser.add_argument('--reference', type=Path, help='Official 1.8.2 UF2; compare USB functions byte for byte')
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest:
        selftest()
        return
    require(args.uf2 is not None, 'UF2 path required')
    report, device, config = inspect(args.uf2.read_bytes(), args.layout)
    if args.reference:
        reference, old_device, old_config = inspect(args.reference.read_bytes(), 'official')
        require(device[:4] + device[7:] == old_device[:4] + old_device[7:], 'USB identity differs')
        require(config[:9] == old_config[:9] and config[9:133] == old_config[75:]
                and config[133:] == old_config[9:75], 'USB function descriptor bytes changed beyond order')
        report['reference_sha256'] = reference['sha256']
        report['usb_only_class_and_function_order'] = 'PASS'
    report['file'] = str(args.uf2.resolve())
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
