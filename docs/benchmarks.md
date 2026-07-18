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
| Serdde direct | small | encode | 329.3 | 353.3 | 6.00 | 646.0 | 3.8 | 122 |
| Serdde direct | small | decode | 689.5 | 168.7 | 24.00 | 1978.0 | 4.1 | 122 |
| Serdde direct | nested | encode | 273.0 | 279.5 | 7.00 | 1173.0 | 3.8 | 80 |
| Serdde direct | nested | decode | 356.4 | 214.1 | 8.00 | 596.0 | 4.0 | 80 |
| Serdde direct | large_string | encode | 102186.5 | 636.4 | 14.00 | 245807.0 | 4.1 | 68186 |
| Serdde direct | large_string | decode | 35334.6 | 1840.3 | 8.00 | 522930.0 | 4.6 | 68186 |
| Serdde direct | large_array | encode | 29727.6 | 765.7 | 13.00 | 61613.0 | 4.2 | 23868 |
| Serdde direct | large_array | decode | 51363.2 | 443.2 | 21.00 | 347240.0 | 4.3 | 23868 |
| Serdde direct | map | encode | 20296.6 | 363.4 | 12.00 | 30892.0 | 4.1 | 7734 |
| Serdde direct | map | decode | 293577.7 | 25.1 | 2062.00 | 172658.0 | 4.1 | 7734 |
| Serdde direct | enum | encode | 198.0 | 269.7 | 5.00 | 540.0 | 3.9 | 56 |
| Serdde direct | enum | decode | 224.2 | 238.2 | 6.00 | 492.0 | 4.1 | 56 |
| Serdde Value | small | encode | 5582.9 | 20.8 | 162.00 | 61828.0 | 3.9 | 122 |
| Serdde Value | small | decode | 4259.3 | 27.3 | 98.00 | 28141.0 | 4.1 | 122 |
| Serdde Value | nested | encode | 5930.1 | 12.9 | 285.00 | 41335.0 | 3.9 | 80 |
| Serdde Value | nested | decode | 4337.0 | 17.6 | 204.00 | 28275.0 | 3.9 | 80 |
| Serdde Value | large_string | encode | 113967.3 | 570.6 | 47.00 | 1134508.0 | 4.6 | 68186 |
| Serdde Value | large_string | decode | 133810.7 | 486.0 | 41.00 | 1338057.0 | 4.6 | 68186 |
| Serdde Value | large_array | encode | 213296260.0 | 0.1 | 12332.00 | 6448854687.0 | 8.8 | 23868 |
| Serdde Value | large_array | decode | 562528.3 | 40.5 | 52.00 | 8101071.0 | 7.9 | 23868 |
| Serdde Value | map | encode | 6413773.9 | 1.1 | 138559.00 | 51215218.0 | 4.5 | 7734 |
| Serdde Value | map | decode | 272884.8 | 27.0 | 6198.00 | 1329557.0 | 4.6 | 7734 |
| Serdde Value | enum | encode | 2805.2 | 19.0 | 142.00 | 21054.0 | 3.9 | 56 |
| Serdde Value | enum | decode | 2261.3 | 23.6 | 93.00 | 13611.0 | 4.1 | 56 |
| Rust Serde | small | encode | 88.6 | 1312.9 | 1.00 | 128.0 | 2.1 | 122 |
| Rust Serde | small | decode | 312.6 | 372.2 | 8.00 | 626.0 | 2.1 | 122 |
| Rust Serde | nested | encode | 55.4 | 1377.0 | 1.00 | 128.0 | 2.2 | 80 |
| Rust Serde | nested | decode | 141.5 | 539.0 | 1.00 | 9.0 | 2.2 | 80 |
| Rust Serde | large_string | encode | 28388.1 | 2290.6 | 3.00 | 204680.0 | 2.4 | 68186 |
| Rust Serde | large_string | decode | 6589.9 | 9867.8 | 1.00 | 68172.0 | 2.4 | 68186 |
| Rust Serde | large_array | encode | 15705.9 | 1449.3 | 9.00 | 65408.0 | 2.2 | 23868 |
| Rust Serde | large_array | decode | 22812.4 | 997.8 | 11.00 | 65504.0 | 2.2 | 23868 |
| Rust Serde | map | encode | 4971.3 | 1483.7 | 7.00 | 16256.0 | 2.2 | 7734 |
| Rust Serde | map | decode | 37616.7 | 196.1 | 596.00 | 35442.0 | 2.2 | 7734 |
| Rust Serde | enum | encode | 44.9 | 1189.2 | 1.00 | 128.0 | 2.2 | 56 |
| Rust Serde | enum | decode | 78.0 | 684.4 | 0.00 | 0.0 | 2.1 | 56 |
| yyjson | small | encode | 133.3 | 872.8 | 6.00 | 2227.0 | 3.5 | 122 |
| yyjson | small | decode | 157.0 | 740.9 | 5.00 | 798.0 | 3.8 | 122 |
| yyjson | nested | encode | 110.1 | 692.8 | 6.00 | 2025.0 | 3.5 | 80 |
| yyjson | nested | decode | 61.7 | 1237.0 | 2.00 | 420.0 | 3.8 | 80 |
| yyjson | large_string | encode | 11788.8 | 5516.0 | 6.00 | 546224.0 | 4.0 | 68186 |
| yyjson | large_string | decode | 9484.0 | 6856.5 | 3.00 | 318315.0 | 3.7 | 68186 |
| yyjson | large_array | encode | 12778.1 | 1781.4 | 12.00 | 294085.0 | 3.7 | 23868 |
| yyjson | large_array | decode | 13774.7 | 1652.5 | 4.00 | 216080.0 | 3.7 | 23868 |
| yyjson | map | encode | 4559.8 | 1617.5 | 15.00 | 83135.0 | 3.8 | 7734 |
| yyjson | map | decode | 31485.9 | 234.3 | 514.00 | 65354.0 | 4.0 | 7734 |
| yyjson | enum | encode | 88.8 | 601.4 | 5.00 | 1713.0 | 3.5 | 56 |
| yyjson | enum | decode | 54.7 | 975.9 | 2.00 | 332.0 | 3.7 | 56 |
| nlohmann/json | small | encode | 693.3 | 167.8 | 23.00 | 2463.0 | 3.6 | 122 |
| nlohmann/json | small | decode | 1134.3 | 102.6 | 31.00 | 1863.0 | 3.6 | 122 |
| nlohmann/json | nested | encode | 435.5 | 175.2 | 19.00 | 1558.0 | 3.6 | 80 |
| nlohmann/json | nested | decode | 635.7 | 120.0 | 24.00 | 959.0 | 3.6 | 80 |
| nlohmann/json | large_string | encode | 104753.8 | 620.8 | 16.00 | 330535.0 | 4.0 | 68186 |
| nlohmann/json | large_string | decode | 250080.3 | 260.0 | 39.00 | 644424.0 | 4.0 | 68186 |
| nlohmann/json | large_array | encode | 56688.4 | 401.5 | 30.00 | 258710.0 | 3.8 | 23868 |
| nlohmann/json | large_array | decode | 161546.5 | 140.9 | 38.00 | 295095.0 | 4.0 | 23868 |
| nlohmann/json | map | encode | 46230.6 | 159.5 | 537.00 | 88749.0 | 3.8 | 7734 |
| nlohmann/json | map | decode | 75428.0 | 97.8 | 1045.00 | 94431.0 | 3.8 | 7734 |
| nlohmann/json | enum | encode | 625.7 | 85.4 | 33.00 | 1813.0 | 3.6 | 56 |
| nlohmann/json | enum | decode | 515.6 | 103.6 | 18.00 | 671.0 | 3.6 | 56 |
| Glaze | small | encode | 40.0 | 2905.4 | 1.00 | 513.0 | 3.5 | 122 |
| Glaze | small | decode | 152.6 | 762.4 | 8.00 | 314.0 | 3.7 | 122 |
| Glaze | nested | encode | 27.0 | 2824.6 | 1.00 | 513.0 | 3.7 | 80 |
| Glaze | nested | decode | 33.2 | 2295.9 | 0.00 | 0.0 | 3.5 | 80 |
| Glaze | large_string | encode | 2835.3 | 22934.7 | 2.00 | 273244.0 | 4.0 | 68186 |
| Glaze | large_string | decode | 27128.7 | 2397.0 | 1.00 | 68173.0 | 3.8 | 68186 |
| Glaze | large_array | encode | 6993.9 | 3254.6 | 2.00 | 205846.0 | 4.0 | 23868 |
| Glaze | large_array | decode | 13607.3 | 1672.8 | 13.00 | 65528.0 | 3.7 | 23868 |
| Glaze | map | encode | 2984.3 | 2471.5 | 5.00 | 16019.0 | 3.7 | 7734 |
| Glaze | map | decode | 28539.2 | 258.4 | 512.00 | 36864.0 | 3.7 | 7734 |
| Glaze | enum | encode | 24.8 | 2153.2 | 1.00 | 513.0 | 3.5 | 56 |
| Glaze | enum | decode | 26.9 | 1988.3 | 0.00 | 0.0 | 3.7 | 56 |

Malformed-input rejection:

| implementation | ns/rejection | alloc/rejection | allocated B/rejection |
| --- | ---: | ---: | ---: |
| Serdde direct | 130.6 | 7.00 | 458.0 |
| Serdde Value | 510.1 | 15.00 | 945.0 |
| Rust Serde | 138.4 | 5.00 | 143.0 |
| yyjson | 30.8 | 2.00 | 202.0 |
| nlohmann/json | 3689.4 | 20.00 | 1017.0 |
| Glaze | 14.1 | 0.00 | 0.0 |

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
| Serdde direct | 1042016 |  |
| Serdde Value | 1042016 |  |
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
