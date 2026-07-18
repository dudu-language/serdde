use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use std::alloc::{GlobalAlloc, Layout, System};
use std::collections::BTreeMap;
use std::env;
use std::io::Cursor;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::Instant;

struct CountingAllocator;

static ALLOCATIONS: AtomicU64 = AtomicU64::new(0);
static ALLOCATED_BYTES: AtomicU64 = AtomicU64::new(0);

fn record(size: usize) {
    ALLOCATIONS.fetch_add(1, Ordering::Relaxed);
    ALLOCATED_BYTES.fetch_add(size as u64, Ordering::Relaxed);
}

unsafe impl GlobalAlloc for CountingAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        record(layout.size());
        unsafe { System.alloc(layout) }
    }

    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        record(layout.size());
        unsafe { System.alloc_zeroed(layout) }
    }

    unsafe fn realloc(&self, pointer: *mut u8, old: Layout, new_size: usize) -> *mut u8 {
        record(new_size);
        unsafe { System.realloc(pointer, old, new_size) }
    }

    unsafe fn dealloc(&self, pointer: *mut u8, layout: Layout) {
        unsafe { System.dealloc(pointer, layout) }
    }
}

#[global_allocator]
static GLOBAL: CountingAllocator = CountingAllocator;

#[derive(Default, Deserialize, Serialize)]
struct SmallRecord {
    id: u64,
    name: String,
    active: bool,
    scores: Vec<i32>,
    metadata: BTreeMap<String, String>,
}

#[derive(Default, Deserialize, Serialize)]
struct NestedLeaf {
    value: i64,
    label: String,
}

#[derive(Default, Deserialize, Serialize)]
struct NestedLevel4 {
    child: NestedLeaf,
}

#[derive(Default, Deserialize, Serialize)]
struct NestedLevel3 {
    child: NestedLevel4,
}

#[derive(Default, Deserialize, Serialize)]
struct NestedLevel2 {
    child: NestedLevel3,
}

#[derive(Default, Deserialize, Serialize)]
struct NestedRecord {
    child: NestedLevel2,
}

#[derive(Default, Deserialize, Serialize)]
struct StringRecord {
    payload: String,
}

#[derive(Default, Deserialize, Serialize)]
struct ArrayRecord {
    values: Vec<i64>,
}

#[derive(Default, Deserialize, Serialize)]
struct MapRecord {
    values: BTreeMap<String, i64>,
}

#[derive(Deserialize, Serialize)]
enum BenchEvent {
    Quit,
    KeyDown { key: i32 },
    MouseMove { x: i32, y: i32 },
}

impl Default for BenchEvent {
    fn default() -> Self {
        Self::Quit
    }
}

#[derive(Default, Deserialize, Serialize)]
struct EventRecord {
    sequence: u64,
    event: BenchEvent,
}

trait Checksum {
    fn checksum(&self) -> u64;
}

impl Checksum for SmallRecord {
    fn checksum(&self) -> u64 {
        self.id
            + self.name.len() as u64
            + self.scores.len() as u64
            + self.metadata.len() as u64
            + u64::from(self.active)
    }
}

impl Checksum for NestedRecord {
    fn checksum(&self) -> u64 {
        (self.child.child.child.child.value as u64)
            .wrapping_add(self.child.child.child.child.label.len() as u64)
    }
}

impl Checksum for StringRecord {
    fn checksum(&self) -> u64 {
        self.payload.len() as u64
    }
}

impl Checksum for ArrayRecord {
    fn checksum(&self) -> u64 {
        (self.values.len() as u64).wrapping_add(*self.values.last().unwrap_or(&0) as u64)
    }
}

impl Checksum for MapRecord {
    fn checksum(&self) -> u64 {
        (self.values.len() as u64).wrapping_add(*self.values.get("key_511").unwrap_or(&0) as u64)
    }
}

impl Checksum for EventRecord {
    fn checksum(&self) -> u64 {
        let event = match self.event {
            BenchEvent::Quit => 0,
            BenchEvent::KeyDown { key } => key as u64,
            BenchEvent::MouseMove { x, y } => x.wrapping_add(y) as u64,
        };
        self.sequence.wrapping_add(event)
    }
}

fn encode<T: Serialize>(value: &T, format: &str) -> Result<Vec<u8>, String> {
    if format == "cbor" {
        let mut output = Vec::new();
        ciborium::into_writer(value, &mut output).map_err(|error| error.to_string())?;
        Ok(output)
    } else {
        serde_json::to_vec(value).map_err(|error| error.to_string())
    }
}

fn decode<T: DeserializeOwned>(source: &[u8], format: &str) -> Result<T, String> {
    if format == "cbor" {
        ciborium::from_reader(Cursor::new(source)).map_err(|error| error.to_string())
    } else {
        serde_json::from_slice(source).map_err(|error| error.to_string())
    }
}

fn reset_allocations() {
    ALLOCATIONS.store(0, Ordering::Relaxed);
    ALLOCATED_BYTES.store(0, Ordering::Relaxed);
}

fn report(
    workload: &str,
    format: &str,
    operation: &str,
    iterations: i32,
    elapsed_ns: u64,
    wire_bytes: usize,
    checksum: u64,
) {
    let allocations = ALLOCATIONS.load(Ordering::Relaxed);
    let allocated_bytes = ALLOCATED_BYTES.load(Ordering::Relaxed);
    println!("implementation=rust-serde");
    println!("format={format}");
    println!("workload={workload}");
    println!("operation={operation}");
    println!("iterations={iterations}");
    println!("elapsed_ns={elapsed_ns}");
    println!("wire_bytes={wire_bytes}");
    println!("allocation_count={allocations}");
    println!("allocated_bytes={allocated_bytes}");
    println!("checksum={checksum}");
}

fn run_case<T>(value: &T, workload: &str, format: &str, operation: &str, iterations: i32) -> i32
where
    T: Checksum + DeserializeOwned + Serialize,
{
    let source = match encode(value, format) {
        Ok(source) => source,
        Err(error) => {
            eprintln!("initial encode failed: {error}");
            return 2;
        }
    };
    reset_allocations();
    let start = Instant::now();
    let mut checksum = 0_u64;
    if operation == "decode" {
        for _ in 0..iterations {
            match decode::<T>(&source, format) {
                Ok(value) => checksum = checksum.wrapping_add(value.checksum()),
                Err(error) => {
                    eprintln!("decode failed: {error}");
                    return 3;
                }
            }
        }
    } else {
        for _ in 0..iterations {
            match encode(value, format) {
                Ok(value) => checksum = checksum.wrapping_add(value.len() as u64),
                Err(error) => {
                    eprintln!("encode failed: {error}");
                    return 4;
                }
            }
        }
    }
    let elapsed_ns = start.elapsed().as_nanos() as u64;
    report(
        workload,
        format,
        operation,
        iterations,
        elapsed_ns,
        source.len(),
        checksum,
    );
    0
}

fn run_malformed(format: &str, iterations: i32) -> i32 {
    let source: &[u8] = if format == "cbor" {
        b"a"
    } else {
        br#"{"id":7,"name":"Ada",]"#
    };
    reset_allocations();
    let start = Instant::now();
    let mut rejected = 0_u64;
    for _ in 0..iterations {
        if decode::<SmallRecord>(source, format).is_err() {
            rejected += 1;
        }
    }
    let elapsed_ns = start.elapsed().as_nanos() as u64;
    report(
        "malformed",
        format,
        "decode",
        iterations,
        elapsed_ns,
        source.len(),
        rejected,
    );
    if rejected == iterations as u64 { 0 } else { 5 }
}

fn small_record() -> SmallRecord {
    SmallRecord {
        id: 7,
        name: "Ada Lovelace".into(),
        active: true,
        scores: vec![2, 3, 5, 7, 11, 13, 17, 19],
        metadata: BTreeMap::from([
            ("format".into(), "wire".into()),
            ("language".into(), "dudu".into()),
        ]),
    }
}

fn nested_record() -> NestedRecord {
    NestedRecord {
        child: NestedLevel2 {
            child: NestedLevel3 {
                child: NestedLevel4 {
                    child: NestedLeaf {
                        value: -123456789,
                        label: "deep leaf".into(),
                    },
                },
            },
        },
    }
}

fn main() {
    let format = env::var("SERDDE_BENCH_FORMAT").unwrap_or_else(|_| "json".into());
    let workload = env::var("SERDDE_BENCH_WORKLOAD").unwrap_or_else(|_| "small".into());
    let operation = env::var("SERDDE_BENCH_OPERATION").unwrap_or_else(|_| "encode".into());
    let iterations = env::var("SERDDE_BENCH_ITERATIONS")
        .ok()
        .and_then(|value| value.parse().ok())
        .filter(|value| *value > 0)
        .unwrap_or(1000);

    let status = match workload.as_str() {
        "small" => run_case(&small_record(), &workload, &format, &operation, iterations),
        "nested" => run_case(&nested_record(), &workload, &format, &operation, iterations),
        "large_string" => run_case(
            &StringRecord {
                payload: "Dudu direct wire payload. ".repeat(2622),
            },
            &workload,
            &format,
            &operation,
            iterations,
        ),
        "large_array" => run_case(
            &ArrayRecord {
                values: (0_i64..4096).map(|value| (value * 17) - 2048).collect(),
            },
            &workload,
            &format,
            &operation,
            iterations,
        ),
        "map" => run_case(
            &MapRecord {
                values: (0_i64..512)
                    .map(|value| (format!("key_{value}"), (value * 31) - 7))
                    .collect(),
            },
            &workload,
            &format,
            &operation,
            iterations,
        ),
        "enum" => run_case(
            &EventRecord {
                sequence: 99,
                event: BenchEvent::MouseMove { x: 320, y: -180 },
            },
            &workload,
            &format,
            &operation,
            iterations,
        ),
        "malformed" => run_malformed(&format, iterations),
        _ => {
            eprintln!("unknown workload: {workload}");
            7
        }
    };
    std::process::exit(status);
}
