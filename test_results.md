# Test Results

All test files were compressed and decompressed with the program. `fc.exe /b` reported no differences for every original/restored pair. Compressed sizes include the 2,060-byte header. Compression ratio = original bytes / compressed bytes. Empty-input ratio is undefined.

## Size series: same byte distribution, varied file size

The pattern `ABACABA` was repeated 1, 1,000, and 10,000 times.

| Dataset | Original bytes | Compressed bytes | Ratio | Round trip |
|---|---:|---:|---:|---|
| ABACABA pattern x 1 | 7 | 2,062 | 0.0034 | Pass |
| ABACABA pattern x 1,000 | 7,000 | 3,310 | 2.1148 | Pass |
| ABACABA pattern x 10,000 | 70,000 | 14,560 | 4.8077 | Pass |

## Distribution series: same file size, varied byte distribution

Each file is 65,536 bytes. Uniform data contains each byte value 0-255 exactly 256 times. Skewed data contains 65,281 `A` bytes and each other byte value once. Deterministic random data uses Python `random.Random(3358).randbytes(65536)`. Text data repeats a short English-like string and truncates it to 65,536 bytes.

| Dataset | Original bytes | Compressed bytes | Ratio | Round trip |
|---|---:|---:|---:|---|
| Uniform byte values | 65,536 | 67,596 | 0.9695 | Pass |
| Skewed byte frequencies | 65,536 | 10,507 | 6.2374 | Pass |
| Deterministic random bytes | 65,536 | 67,596 | 0.9695 | Pass |
| Repeated text | 65,536 | 35,283 | 1.8574 | Pass |

## Edge cases

| Dataset | Original bytes | Compressed bytes | Round trip |
|---|---:|---:|---|
| Empty file | 0 | 2,060 | Pass |
| One symbol (`A` repeated) | 70,000 | 10,810 | Pass |

The ABACABA demo produced a 10-bit encoded payload. No runtime measurements are included; comparative timing is coordinated by the 5D teammate.
