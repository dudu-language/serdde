# Direct-Wire Benchmarks

This suite compares equivalent typed schemas and operations. Serdde direct, Serdde's optional `Value`
backend, Rust Serde, yyjson, nlohmann/json, and Glaze are measured in separate processes.

## Environment

- Machine: AMD Ryzen 9 9950X 16-Core Processor, 32 logical CPUs, 62.4 GiB RAM
- OS: Ubuntu 24.04.4 LTS
- C++: c++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Rust: rustc 1.94.0 (4a4ef493e 2026-03-02)
- Dudu: dudu 0.1.0-alpha.13
- Optimization: release; C/C++ -O3 -DNDEBUG; Rust release without LTO
- Glaze: v7.9.1 (`cadcadea26554cc4214769e358f981426e40a02a`)
- serde / serde_json / ciborium: 1.0.228 / 1.0.150 / 0.2.2
- nlohmann/json / yyjson: 3.11.3 / 0.12.0

Runtime values are medians of independent process runs after one calibration run. Each case is calibrated
independently toward the requested measurement duration and capped by the checked-in iteration limits.
Allocation counters record gross
allocation requests, not retained bytes. Peak RSS includes process and linked-library startup. Throughput
uses encoded wire bytes. Malformed cases measure rejection and appear separately.

## JSON

| implementation | workload | operation | ns/op | MiB/s | alloc/op | allocated B/op | RSS MiB | wire B |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Serdde direct | small | encode | 336.4 | 345.9 | 6.00 | 646.0 | 3.8 | 122 |
| Serdde direct | small | decode | 747.4 | 155.7 | 29.00 | 2330.0 | 4.3 | 122 |
| Serdde direct | nested | encode | 273.1 | 279.3 | 7.00 | 1173.0 | 3.8 | 80 |
| Serdde direct | nested | decode | 520.0 | 146.7 | 19.00 | 1052.0 | 4.0 | 80 |
| Serdde direct | large_string | encode | 98604.7 | 659.5 | 14.00 | 245807.0 | 4.3 | 68186 |
| Serdde direct | large_string | decode | 35997.6 | 1806.4 | 11.00 | 523146.0 | 4.6 | 68186 |
| Serdde direct | large_array | encode | 29476.5 | 772.2 | 13.00 | 61613.0 | 4.1 | 23868 |
| Serdde direct | large_array | decode | 51866.4 | 438.9 | 24.00 | 347456.0 | 4.5 | 23868 |
| Serdde direct | map | encode | 20364.9 | 362.2 | 12.00 | 30892.0 | 4.2 | 7734 |
| Serdde direct | map | decode | 67294.2 | 109.6 | 2067.00 | 181106.0 | 4.3 | 7734 |
| Serdde direct | enum | encode | 204.0 | 261.8 | 5.00 | 540.0 | 4.0 | 56 |
| Serdde direct | enum | decode | 339.1 | 157.5 | 13.00 | 852.0 | 4.0 | 56 |
| Serdde Value | small | encode | 5549.2 | 21.0 | 162.00 | 61828.0 | 4.0 | 122 |
| Serdde Value | small | decode | 4058.9 | 28.7 | 98.00 | 28141.0 | 4.0 | 122 |
| Serdde Value | nested | encode | 5979.4 | 12.8 | 285.00 | 41335.0 | 3.8 | 80 |
| Serdde Value | nested | decode | 4391.2 | 17.4 | 204.00 | 28275.0 | 4.0 | 80 |
| Serdde Value | large_string | encode | 122455.6 | 531.0 | 47.00 | 1134508.0 | 4.6 | 68186 |
| Serdde Value | large_string | decode | 128319.5 | 506.8 | 41.00 | 1338057.0 | 4.6 | 68186 |
| Serdde Value | large_array | encode | 208760226.0 | 0.1 | 12332.00 | 6448854687.0 | 8.7 | 23868 |
| Serdde Value | large_array | decode | 562978.7 | 40.4 | 52.00 | 8101071.0 | 7.8 | 23868 |
| Serdde Value | map | encode | 6488755.6 | 1.1 | 138559.00 | 51215218.0 | 4.8 | 7734 |
| Serdde Value | map | decode | 268727.2 | 27.4 | 6198.00 | 1329557.0 | 4.8 | 7734 |
| Serdde Value | enum | encode | 2796.1 | 19.1 | 142.00 | 21054.0 | 4.1 | 56 |
| Serdde Value | enum | decode | 2252.3 | 23.7 | 93.00 | 13611.0 | 4.0 | 56 |
| Rust Serde | small | encode | 88.0 | 1321.9 | 1.00 | 128.0 | 2.2 | 122 |
| Rust Serde | small | decode | 306.5 | 379.6 | 8.00 | 626.0 | 2.1 | 122 |
| Rust Serde | nested | encode | 54.9 | 1388.5 | 1.00 | 128.0 | 2.2 | 80 |
| Rust Serde | nested | decode | 140.0 | 544.9 | 1.00 | 9.0 | 2.2 | 80 |
| Rust Serde | large_string | encode | 28556.3 | 2277.2 | 3.00 | 204680.0 | 2.4 | 68186 |
| Rust Serde | large_string | decode | 6650.0 | 9778.5 | 1.00 | 68172.0 | 2.4 | 68186 |
| Rust Serde | large_array | encode | 15441.7 | 1474.1 | 9.00 | 65408.0 | 2.1 | 23868 |
| Rust Serde | large_array | decode | 21087.5 | 1079.4 | 11.00 | 65504.0 | 2.1 | 23868 |
| Rust Serde | map | encode | 4865.9 | 1515.8 | 7.00 | 16256.0 | 2.1 | 7734 |
| Rust Serde | map | decode | 36718.0 | 200.9 | 596.00 | 35442.0 | 2.2 | 7734 |
| Rust Serde | enum | encode | 44.9 | 1188.8 | 1.00 | 128.0 | 2.1 | 56 |
| Rust Serde | enum | decode | 78.9 | 676.6 | 0.00 | 0.0 | 2.1 | 56 |
| yyjson | small | encode | 133.0 | 874.6 | 6.00 | 2227.0 | 3.7 | 122 |
| yyjson | small | decode | 151.9 | 766.1 | 5.00 | 798.0 | 3.7 | 122 |
| yyjson | nested | encode | 110.2 | 692.4 | 6.00 | 2025.0 | 3.5 | 80 |
| yyjson | nested | decode | 62.1 | 1227.8 | 2.00 | 420.0 | 3.7 | 80 |
| yyjson | large_string | encode | 11363.2 | 5722.6 | 6.00 | 546224.0 | 4.0 | 68186 |
| yyjson | large_string | decode | 9395.8 | 6920.9 | 3.00 | 318315.0 | 4.0 | 68186 |
| yyjson | large_array | encode | 12556.3 | 1812.8 | 12.00 | 294085.0 | 3.7 | 23868 |
| yyjson | large_array | decode | 14204.9 | 1602.4 | 4.00 | 216080.0 | 3.7 | 23868 |
| yyjson | map | encode | 4557.0 | 1618.5 | 15.00 | 83135.0 | 3.7 | 7734 |
| yyjson | map | decode | 31367.0 | 235.1 | 514.00 | 65354.0 | 4.0 | 7734 |
| yyjson | enum | encode | 88.8 | 601.7 | 5.00 | 1713.0 | 3.8 | 56 |
| yyjson | enum | decode | 54.9 | 973.3 | 2.00 | 332.0 | 3.8 | 56 |
| nlohmann/json | small | encode | 686.1 | 169.6 | 23.00 | 2463.0 | 3.5 | 122 |
| nlohmann/json | small | decode | 1109.1 | 104.9 | 31.00 | 1863.0 | 3.5 | 122 |
| nlohmann/json | nested | encode | 454.4 | 167.9 | 19.00 | 1558.0 | 3.5 | 80 |
| nlohmann/json | nested | decode | 628.0 | 121.5 | 24.00 | 959.0 | 3.5 | 80 |
| nlohmann/json | large_string | encode | 105804.6 | 614.6 | 16.00 | 330535.0 | 4.0 | 68186 |
| nlohmann/json | large_string | decode | 255121.8 | 254.9 | 39.00 | 644424.0 | 4.0 | 68186 |
| nlohmann/json | large_array | encode | 55739.3 | 408.4 | 30.00 | 258710.0 | 3.8 | 23868 |
| nlohmann/json | large_array | decode | 160619.4 | 141.7 | 38.00 | 295095.0 | 4.0 | 23868 |
| nlohmann/json | map | encode | 47286.6 | 156.0 | 537.00 | 88749.0 | 3.8 | 7734 |
| nlohmann/json | map | decode | 76209.0 | 96.8 | 1045.00 | 94431.0 | 3.8 | 7734 |
| nlohmann/json | enum | encode | 635.5 | 84.0 | 33.00 | 1813.0 | 3.5 | 56 |
| nlohmann/json | enum | decode | 518.7 | 103.0 | 18.00 | 671.0 | 3.6 | 56 |
| Glaze | small | encode | 39.2 | 2967.8 | 1.00 | 513.0 | 3.5 | 122 |
| Glaze | small | decode | 158.7 | 733.2 | 8.00 | 314.0 | 3.5 | 122 |
| Glaze | nested | encode | 27.2 | 2808.7 | 1.00 | 513.0 | 3.8 | 80 |
| Glaze | nested | decode | 32.0 | 2385.3 | 0.00 | 0.0 | 3.5 | 80 |
| Glaze | large_string | encode | 2850.8 | 22809.8 | 2.00 | 273244.0 | 4.2 | 68186 |
| Glaze | large_string | decode | 25085.0 | 2592.3 | 1.00 | 68173.0 | 3.7 | 68186 |
| Glaze | large_array | encode | 6908.7 | 3294.7 | 2.00 | 205846.0 | 4.0 | 23868 |
| Glaze | large_array | decode | 13217.9 | 1722.1 | 13.00 | 65528.0 | 3.7 | 23868 |
| Glaze | map | encode | 3003.9 | 2455.4 | 5.00 | 16019.0 | 4.0 | 7734 |
| Glaze | map | decode | 28899.9 | 255.2 | 512.00 | 36864.0 | 3.7 | 7734 |
| Glaze | enum | encode | 25.4 | 2102.7 | 1.00 | 513.0 | 3.5 | 56 |
| Glaze | enum | decode | 26.8 | 1989.3 | 0.00 | 0.0 | 3.8 | 56 |

Malformed-input rejection:

| implementation | ns/rejection | alloc/rejection | allocated B/rejection |
| --- | ---: | ---: | ---: |
| Serdde direct | 126.7 | 7.00 | 514.0 |
| Serdde Value | 486.2 | 15.00 | 945.0 |
| Rust Serde | 138.9 | 5.00 | 143.0 |
| yyjson | 31.9 | 2.00 | 202.0 |
| nlohmann/json | 3787.0 | 20.00 | 1017.0 |
| Glaze | 14.2 | 0.00 | 0.0 |

## CBOR

| implementation | workload | operation | ns/op | MiB/s | alloc/op | allocated B/op | RSS MiB | wire B |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Serdde direct | small | encode | 304.4 | 260.1 | 9.00 | 607.0 | 3.8 | 83 |
| Serdde direct | small | decode | 1157.5 | 68.4 | 32.00 | 4404.0 | 3.8 | 83 |
| Serdde direct | nested | encode | 244.5 | 218.4 | 6.00 | 932.0 | 3.8 | 56 |
| Serdde direct | nested | decode | 766.7 | 69.7 | 17.00 | 1936.0 | 3.8 | 56 |
| Serdde direct | large_string | encode | 22218.6 | 2926.7 | 3.00 | 68274.0 | 4.0 | 68186 |
| Serdde direct | large_string | decode | 57839.0 | 1124.3 | 10.00 | 273180.0 | 4.3 | 68186 |
| Serdde direct | large_array | encode | 26553.6 | 449.2 | 12.00 | 30868.0 | 4.1 | 12507 |
| Serdde direct | large_array | decode | 123209.9 | 96.8 | 35.00 | 819552.0 | 4.3 | 12507 |
| Serdde direct | map | encode | 43673.2 | 120.6 | 417.00 | 56754.0 | 4.3 | 5523 |
| Serdde direct | map | decode | 88060.5 | 59.8 | 2075.00 | 320560.0 | 4.0 | 5523 |
| Serdde direct | enum | encode | 190.2 | 195.6 | 7.00 | 546.0 | 4.0 | 39 |
| Serdde direct | enum | decode | 492.7 | 75.5 | 13.00 | 1832.0 | 3.8 | 39 |
| Serdde Value | small | encode | 5840.0 | 13.6 | 166.00 | 62049.0 | 3.8 | 83 |
| Serdde Value | small | decode | 3498.6 | 22.6 | 79.00 | 21484.0 | 3.8 | 83 |
| Serdde Value | nested | encode | 6147.4 | 8.7 | 299.00 | 42656.0 | 3.8 | 56 |
| Serdde Value | nested | decode | 3559.5 | 15.0 | 152.00 | 20816.0 | 3.8 | 56 |
| Serdde Value | large_string | encode | 39317.9 | 1653.9 | 37.00 | 888929.0 | 4.5 | 68186 |
| Serdde Value | large_string | decode | 47756.7 | 1361.6 | 22.00 | 682970.0 | 4.3 | 68186 |
| Serdde Value | large_array | encode | 202982710.0 | 0.1 | 12334.00 | 6448801484.0 | 7.9 | 12507 |
| Serdde Value | large_array | decode | 451590.1 | 26.4 | 59.00 | 6325136.0 | 7.8 | 12507 |
| Serdde Value | map | encode | 6752900.2 | 0.8 | 138967.00 | 51236914.0 | 4.5 | 5523 |
| Serdde Value | map | decode | 225026.2 | 23.4 | 4665.00 | 1059504.0 | 4.6 | 5523 |
| Serdde Value | enum | encode | 3043.7 | 12.2 | 151.00 | 21803.0 | 3.8 | 39 |
| Serdde Value | enum | decode | 1923.7 | 19.3 | 70.00 | 10256.0 | 4.0 | 39 |
| Rust Serde | small | encode | 141.0 | 561.4 | 5.00 | 248.0 | 2.2 | 83 |
| Rust Serde | small | decode | 651.4 | 121.5 | 7.00 | 620.0 | 2.1 | 83 |
| Rust Serde | nested | encode | 84.7 | 630.8 | 4.00 | 120.0 | 2.5 | 56 |
| Rust Serde | nested | decode | 272.8 | 195.8 | 1.00 | 9.0 | 2.2 | 56 |
| Rust Serde | large_string | encode | 609.0 | 106772.5 | 3.00 | 68210.0 | 2.4 | 68186 |
| Rust Serde | large_string | decode | 2443.9 | 26607.7 | 6.00 | 258048.0 | 2.4 | 68186 |
| Rust Serde | large_array | encode | 20751.9 | 574.8 | 12.00 | 32760.0 | 2.1 | 12507 |
| Rust Serde | large_array | decode | 56193.7 | 212.3 | 1.00 | 32768.0 | 2.2 | 12507 |
| Rust Serde | map | encode | 6754.7 | 779.8 | 11.00 | 16376.0 | 2.1 | 5523 |
| Rust Serde | map | decode | 65229.9 | 80.7 | 596.00 | 36064.0 | 2.1 | 5523 |
| Rust Serde | enum | encode | 93.2 | 399.1 | 4.00 | 120.0 | 2.4 | 39 |
| Rust Serde | enum | decode | 189.2 | 196.6 | 0.00 | 0.0 | 2.1 | 39 |
| nlohmann/json | small | encode | 676.1 | 117.1 | 33.00 | 1997.0 | 3.5 | 83 |
| nlohmann/json | small | decode | 1167.4 | 67.8 | 42.00 | 3288.0 | 3.6 | 83 |
| nlohmann/json | nested | encode | 472.9 | 112.9 | 28.00 | 1189.0 | 3.5 | 56 |
| nlohmann/json | nested | decode | 689.4 | 77.5 | 32.00 | 1720.0 | 3.5 | 56 |
| nlohmann/json | large_string | encode | 1798.9 | 36147.9 | 13.00 | 204816.0 | 4.0 | 68186 |
| nlohmann/json | large_string | decode | 53483.8 | 1215.8 | 25.00 | 450622.0 | 4.0 | 68186 |
| nlohmann/json | large_array | encode | 51659.3 | 230.9 | 34.00 | 240033.0 | 3.8 | 12507 |
| nlohmann/json | large_array | decode | 103393.8 | 115.4 | 49.00 | 491800.0 | 4.0 | 12507 |
| nlohmann/json | map | encode | 45110.6 | 116.8 | 1053.00 | 90763.0 | 3.8 | 5523 |
| nlohmann/json | map | decode | 75064.8 | 70.2 | 1564.00 | 151896.0 | 3.8 | 5523 |
| nlohmann/json | enum | encode | 701.0 | 53.1 | 43.00 | 1544.0 | 3.5 | 39 |
| nlohmann/json | enum | decode | 533.8 | 69.7 | 21.00 | 1208.0 | 3.5 | 39 |
| Glaze | small | encode | 42.8 | 2450.9 | 1.00 | 513.0 | 3.8 | 110 |
| Glaze | small | decode | 80.6 | 1301.1 | 3.00 | 224.0 | 3.7 | 110 |
| Glaze | nested | encode | 32.4 | 1647.5 | 1.00 | 513.0 | 3.5 | 56 |
| Glaze | nested | decode | 14.1 | 3787.3 | 0.00 | 0.0 | 3.8 | 56 |
| Glaze | large_string | encode | 1188.8 | 54702.0 | 2.00 | 137398.0 | 4.1 | 68186 |
| Glaze | large_string | decode | 579.6 | 112191.6 | 1.00 | 68173.0 | 4.0 | 68186 |
| Glaze | large_array | encode | 542.6 | 57612.8 | 2.00 | 66588.0 | 4.0 | 32781 |
| Glaze | large_array | decode | 358.3 | 87262.7 | 1.00 | 32768.0 | 4.0 | 32781 |
| Glaze | map | encode | 2476.9 | 2126.5 | 5.00 | 16163.0 | 3.7 | 5523 |
| Glaze | map | decode | 25350.3 | 207.8 | 512.00 | 36864.0 | 3.7 | 5523 |
| Glaze | enum | encode | 39.4 | 992.6 | 1.00 | 513.0 | 3.5 | 41 |
| Glaze | enum | decode | 15.3 | 2560.4 | 0.00 | 0.0 | 3.8 | 41 |

Malformed-input rejection:

| implementation | ns/rejection | alloc/rejection | allocated B/rejection |
| --- | ---: | ---: | ---: |
| Serdde direct | 93.2 | 5.00 | 280.0 |
| Serdde Value | 126.5 | 6.00 | 302.0 |
| Rust Serde | 141.8 | 5.00 | 224.0 |
| nlohmann/json | 2659.7 | 7.00 | 469.0 |
| Glaze | 5.9 | 0.00 | 0.0 |

## Compilation

| implementation | cold seconds | warm seconds | cold RSS MiB |
| --- | ---: | ---: | ---: |
| Serdde direct/Value | 138.30 | 3.81 | 586.7 |
| yyjson | 5.23 | 0.04 | 247.4 |
| nlohmann/json | 3.94 | 0.03 | 411.6 |
| Glaze | 5.01 | 0.03 | 517.1 |
| Rust Serde | 4.30 | 0.02 | 377.1 |

## Artifact Size

| artifact | bytes | generated lines |
| --- | ---: | ---: |
| Serdde direct | 1053240 |  |
| Serdde Value | 1053240 |  |
| Rust Serde | 829840 |  |
| yyjson | 333664 |  |
| nlohmann/json | 277488 |  |
| Glaze | 260792 |  |
| Dudu generated C++ | 532526 | 7099 |

## Reproduce

```bash
./scripts/bench_wire.sh --all --publish
```

The runner fetches the pinned Glaze commit into ignored build storage. Rust dependencies are fixed by
`benchmarks/rust/Cargo.lock`; yyjson is vendored; system nlohmann/json is version-checked by CMake.
