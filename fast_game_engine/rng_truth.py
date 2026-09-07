"""Emit mixed CPython Random operations for rng_validate.cpp."""
import random
import struct


keys = [0, 1, 2, 2**32 - 1, 2**32, 2**63 - 1, 2**64 - 1]
keys.extend((seed * 1_000_003) ^ day
            for seed in [11, 101, 70117, 2**32 - 1]
            for day in range(30))
for key in keys:
    rng = random.Random(key)
    print("KEY", key)
    for _ in range(64):
        bits = struct.unpack("<Q", struct.pack("<d", rng.random()))[0]
        print("R", bits)
    for bound in [1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 33, 255, 256, 257] * 4:
        print("B", bound, rng.randrange(bound))
