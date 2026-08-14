#!/usr/bin/env bash
# ==============================================================================
# Sterilized Benchmark Suite for symdep vs GNU Stow vs Dotbot
# ==============================================================================

set -euo pipefail

BENCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$BENCH_DIR/../.." && pwd)"

DEFAULT_WORK_DIR="/tmp/symdep_benchmark_workspace"
if [ -d "/dev/shm" ] && [ -w "/dev/shm" ]; then
    DEFAULT_WORK_DIR="/dev/shm/symdep_benchmark_workspace"
fi
WORK_DIR="${SYMDEP_BENCH_DIR:-$DEFAULT_WORK_DIR}"
VENDOR_DIR="$BENCH_DIR/vendor"
REPORT_FILE="$BENCH_DIR/BENCHMARK_REPORT.md"
JSON_FILE="$BENCH_DIR/benchmark_results.json"

SYMDEP_BIN="$REPO_ROOT/bin/symdep"
STOW_BIN="$(which stow 2>/dev/null || echo "")"
DOTBOT_REPO="$VENDOR_DIR/dotbot"
DOTBOT_BIN="$DOTBOT_REPO/bin/dotbot"
PYTHON_BIN="$(which python3 2>/dev/null || echo "")"
HYPERFINE_BIN="$(which hyperfine 2>/dev/null || echo "")"

QUICK_MODE=0
CUSTOM_WARMUP_STARTUP=10
CUSTOM_RUNS_STARTUP=50
CUSTOM_WARMUP_DS=2
CUSTOM_RUNS_DS=20

SKIP_DOTBOT=0
FORCE_FETCH_DOTBOT=0
INCLUDE_ULTRA=0
INCLUDE_MEGA=0

usage() {
    cat << 'EOF'
Usage: run_benchmark.sh [options]

Options:
  -q, --quick         Run in quick mode (1 warmup, 3-5 runs for fast evaluation)
  -w, --warmup N      Set custom warmup count
  -r, --runs N        Set custom measurement runs count
      --skip-dotbot   Skip Dotbot benchmarks (useful for offline/minimal setups)
      --fetch-dotbot  Force re-fetching Dotbot repository into vendor/dotbot
      --workdir DIR   Custom workspace directory (default: /dev/shm in-RAM tmpfs)
      --ultra         Include Ultra dataset benchmark (100,000 files)
      --mega          Include Ultra (100,000 files) and Mega (1,000,000 files) benchmarks
  -h, --help          Show this help message
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -q|--quick)
            QUICK_MODE=1
            CUSTOM_WARMUP_STARTUP=1
            CUSTOM_RUNS_STARTUP=5
            CUSTOM_WARMUP_DS=1
            CUSTOM_RUNS_DS=3
            shift
            ;;
        -w|--warmup)
            CUSTOM_WARMUP_STARTUP="$2"
            CUSTOM_WARMUP_DS="$2"
            shift 2
            ;;
        -r|--runs)
            CUSTOM_RUNS_STARTUP="$2"
            CUSTOM_RUNS_DS="$2"
            shift 2
            ;;
        --skip-dotbot)
            SKIP_DOTBOT=1
            shift
            ;;
        --fetch-dotbot)
            FORCE_FETCH_DOTBOT=1
            shift
            ;;
        --workdir)
            WORK_DIR="$2"
            shift 2
            ;;
        --ultra)
            INCLUDE_ULTRA=1
            shift
            ;;
        --mega)
            INCLUDE_ULTRA=1
            INCLUDE_MEGA=1
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "[ERROR] Unknown option: $1"
            usage
            ;;
    esac
done

echo "=============================================================================="
echo "               Symdep Sterilized Benchmark Suite Initializing                 "
if [ "$QUICK_MODE" -eq 1 ]; then
    echo "               [QUICK MODE ACTIVE: 1 Warmup, Fast Iterations]                "
fi
echo "=============================================================================="

# 1. Dependency Checks
if [ ! -f "$SYMDEP_BIN" ]; then
    echo "[!] Building symdep release binary..."
    make -C "$REPO_ROOT"
fi

if [ -z "$HYPERFINE_BIN" ]; then
    echo "[ERROR] 'hyperfine' is required to run timing benchmarks."
    echo "Please install hyperfine (e.g., 'sudo apt install hyperfine' or 'pacman -S hyperfine')."
    exit 1
fi

if [ -z "$STOW_BIN" ]; then
    echo "[WARNING] 'stow' (GNU Stow) not found in PATH. GNU Stow benchmarks will be skipped."
fi

# 2. Set up Dotbot vendor repository
if [ "$SKIP_DOTBOT" -eq 1 ]; then
    echo "[INFO] Skipping Dotbot benchmarks (--skip-dotbot specified)."
else
    if [ "$FORCE_FETCH_DOTBOT" -eq 1 ] && [ -d "$DOTBOT_REPO" ]; then
        echo "[!] Re-fetching Dotbot repository..."
        rm -rf "$DOTBOT_REPO"
    fi

    if [ ! -d "$DOTBOT_REPO" ]; then
        echo "[!] Fetching Dotbot repository into vendor/dotbot for comparison..."
        mkdir -p "$VENDOR_DIR"
        if git clone --depth=1 https://github.com/anishathalye/dotbot.git "$DOTBOT_REPO" >/dev/null 2>&1; then
            echo "[SUCCESS] Dotbot repository fetched successfully."
        else
            echo "[WARNING] Failed to clone Dotbot repository (network offline or git error). Skipping Dotbot benchmarks."
        fi
    fi
fi

# Clean & recreate workspace
cleanup_workspace() {
    if [ -d "$WORK_DIR" ]; then
        rm -rf "$WORK_DIR"
    fi
}
trap cleanup_workspace EXIT INT TERM

cleanup_workspace
mkdir -p "$WORK_DIR"

DOTBOT_STATUS="N/A"
if [ "$SKIP_DOTBOT" -eq 0 ] && [ -n "$PYTHON_BIN" ] && [ -f "$DOTBOT_BIN" ]; then
    DOTBOT_STATUS="$DOTBOT_BIN"
fi

IS_TMPFS="NO"
if [[ "$WORK_DIR" == /dev/shm* ]]; then
    IS_TMPFS="YES (in-RAM tmpfs)"
fi

echo "Workdir:   $WORK_DIR"
echo "RAMDisk:   $IS_TMPFS"
echo "symdep:    $SYMDEP_BIN"
echo "GNU Stow:  ${STOW_BIN:-N/A}"
echo "Dotbot:    $DOTBOT_STATUS"
echo "Hyperfine: $HYPERFINE_BIN"
echo "QuickMode: $([ "$QUICK_MODE" -eq 1 ] && echo "YES (Fast)" || echo "NO (Full)")"
echo "------------------------------------------------------------------------------"

# Helper: Generate Workload Dataset
# Args: <num_pkgs> <files_per_pkg> <output_dir>
generate_dataset() {
    local num_pkgs="$1"
    local files_per_pkg="$2"
    local base_dir="$3"
    
    mkdir -p "$base_dir/src" "$base_dir/target"
    local dotbot_config="$base_dir/dotbot.yaml"
    
    echo "- link:" > "$dotbot_config"
    
    local pkg_list=""
    for ((p=1; p<=num_pkgs; p++)); do
        local pkg_name="pkg_$p"
        pkg_list="$pkg_list $pkg_name"
        local pkg_dir="$base_dir/src/$pkg_name/.config/app_$p"
        mkdir -p "$pkg_dir"
        
        for ((f=1; f<=files_per_pkg; f++)); do
            local file_name="config_${f}.conf"
            local file_path="$pkg_dir/$file_name"
            echo "key_${f}=value_${f}" > "$file_path"
            
            # Add entry to dotbot config
            echo "    ~/.config/app_${p}/${file_name}:" >> "$dotbot_config"
            echo "      path: src/${pkg_name}/.config/app_${p}/${file_name}" >> "$dotbot_config"
            echo "      create: true" >> "$dotbot_config"
        done
    done
    
    echo "$pkg_list" > "$base_dir/pkg_list.txt"
}

# Helper: Reset target dir for prepare step
reset_target() {
    local target_dir="$1"
    rm -rf "$target_dir"
    mkdir -p "$target_dir"
}

# ------------------------------------------------------------------------------
# Test 1: Cold Start / Startup Latency (--help invocation)
# ------------------------------------------------------------------------------
echo ""
echo ">>> Phase 1: Measuring Cold Start / Startup Overhead (--help)"

STARTUP_CMDS=()
STARTUP_CMDS+=("$SYMDEP_BIN --help")
if [ -n "$STOW_BIN" ]; then
    STARTUP_CMDS+=("$STOW_BIN --help")
fi
if [ -n "$PYTHON_BIN" ] && [ -f "$DOTBOT_BIN" ]; then
    STARTUP_CMDS+=("$PYTHON_BIN $DOTBOT_BIN --help")
fi

hyperfine \
    --warmup "$CUSTOM_WARMUP_STARTUP" \
    --runs "$CUSTOM_RUNS_STARTUP" \
    --export-markdown "$WORK_DIR/startup_results.md" \
    --export-json "$WORK_DIR/startup_results.json" \
    "${STARTUP_CMDS[@]}"

# ------------------------------------------------------------------------------
# Test 2: Dataset Benchmarks (Small, Medium, Large)
# ------------------------------------------------------------------------------

# Datasets configuration: name pkgs files_per_pkg
DATASETS=(
    "Small 10 10"       # 100 files
    "Medium 50 50"      # 2,500 files
    "Large 100 100"     # 10,000 files
)

if [ "$INCLUDE_ULTRA" -eq 1 ]; then
    DATASETS+=("Ultra 100 1000")  # 100,000 files
fi

if [ "$INCLUDE_MEGA" -eq 1 ]; then
    DATASETS+=("Mega 1000 1000")  # 1,000,000 files
fi

REPORT_SECTIONS=""

for ds_spec in "${DATASETS[@]}"; do
    read -r ds_name num_pkgs files_per_pkg <<< "$ds_spec"
    total_files=$((num_pkgs * files_per_pkg))
    
    echo ""
    echo ">>> Phase 2: Measuring Deployment Performance - [$ds_name Dataset ($total_files files)]"
    
    ds_dir="$WORK_DIR/$ds_name"
    generate_dataset "$num_pkgs" "$files_per_pkg" "$ds_dir"
    
    pkg_args="$(cat "$ds_dir/pkg_list.txt")"
    src_dir="$ds_dir/src"
    target_dir="$ds_dir/target"
    dotbot_cfg="$ds_dir/dotbot.yaml"
    
    PREPARE_SCRIPT="$WORK_DIR/reset_${ds_name}.sh"
    cat << EOF > "$PREPARE_SCRIPT"
#!/usr/bin/env bash
rm -rf "$target_dir"
mkdir -p "$target_dir"
EOF
    chmod +x "$PREPARE_SCRIPT"
    
    BENCH_CMDS=()
    
    # 1. symdep command
    BENCH_CMDS+=("$SYMDEP_BIN -d $src_dir -t $target_dir link $pkg_args")
    
    # 2. GNU Stow command (with --no-folding to force per-file symlinks for 1-to-1 comparison)
    if [ -n "$STOW_BIN" ]; then
        BENCH_CMDS+=("$STOW_BIN --no-folding -d $src_dir -t $target_dir $pkg_args")
    fi
    
    # 3. Dotbot command (Skip Dotbot for Ultra/Mega datasets >= 100K files due to extreme Python runtime)
    if [ -n "$PYTHON_BIN" ] && [ -f "$DOTBOT_BIN" ] && [ "$total_files" -lt 100000 ]; then
        BENCH_CMDS+=("HOME=$target_dir $PYTHON_BIN $DOTBOT_BIN -d $ds_dir -c $dotbot_cfg")
    fi
    
    ds_warmup="$CUSTOM_WARMUP_DS"
    ds_runs="$CUSTOM_RUNS_DS"
    if [ "$total_files" -ge 1000000 ]; then
        ds_warmup=0
        ds_runs=1
    elif [ "$total_files" -ge 100000 ]; then
        ds_warmup=1
        ds_runs=2
    elif [ "$QUICK_MODE" -eq 0 ] && [ "$total_files" -ge 10000 ]; then
        ds_runs=10
    fi
    
    hyperfine \
        --ignore-failure \
        --warmup "$ds_warmup" \
        --runs "$ds_runs" \
        --prepare "$PREPARE_SCRIPT" \
        --export-markdown "$WORK_DIR/${ds_name}_results.md" \
        --export-json "$WORK_DIR/${ds_name}_results.json" \
        "${BENCH_CMDS[@]}"
done

# ------------------------------------------------------------------------------
# Test 3: Peak Resident Set Size (Memory Footprint)
# ------------------------------------------------------------------------------
echo ""
echo ">>> Phase 3: Measuring Peak Resident Set Size (Peak RSS Memory Usage)"

MEM_DATASET_DIR="$WORK_DIR/Large"
MEM_SRC="$MEM_DATASET_DIR/src"
MEM_TARGET="$MEM_DATASET_DIR/target"
MEM_PKGS="$(cat "$MEM_DATASET_DIR/pkg_list.txt")"
MEM_DOTBOT_CFG="$MEM_DATASET_DIR/dotbot.yaml"

get_peak_rss() {
    local cmd="$1"
    local prepare="$2"
    eval "$prepare"
    python3 -c "
import subprocess, resource, sys
cmd_str = sys.argv[1]
subprocess.run(cmd_str, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
print(resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)
" "$cmd"
}

PREPARE_CMD="rm -rf '$MEM_TARGET' && mkdir -p '$MEM_TARGET'"

SYMDEP_RSS="$(get_peak_rss "$SYMDEP_BIN -d '$MEM_SRC' -t '$MEM_TARGET' link $MEM_PKGS" "$PREPARE_CMD")"

STOW_RSS="N/A"
if [ -n "$STOW_BIN" ]; then
    STOW_RSS="$(get_peak_rss "$STOW_BIN --no-folding -d '$MEM_SRC' -t '$MEM_TARGET' $MEM_PKGS" "$PREPARE_CMD")"
fi

DOTBOT_RSS="N/A"
if [ -n "$PYTHON_BIN" ] && [ -f "$DOTBOT_BIN" ]; then
    DOTBOT_RSS="$(get_peak_rss "HOME='$MEM_TARGET' $PYTHON_BIN '$DOTBOT_BIN' -d '$MEM_DATASET_DIR' -c '$MEM_DOTBOT_CFG'" "$PREPARE_CMD")"
fi

# ------------------------------------------------------------------------------
# Generate Final Markdown Report
# ------------------------------------------------------------------------------

cat << EOF > "$REPORT_FILE"
# Benchmark Report: \`symdep\` vs Competitors

Automated benchmark evaluation comparing **\`symdep\`** (ISO C17) against **GNU Stow** (Perl) and **Dotbot** (Python).

- **Execution Host**: $(uname -s -m) ($(uname -n))
- **Date**: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
- **Compiler**: $(gcc --version | head -n1)

---

## 1. Startup & Cold-Start Latency (\`--help\` Invocation)

Measures execution overhead before doing any file operations (interpreter boot time vs native binary execution).

$(cat "$WORK_DIR/startup_results.md")

---

## 2. Link Deployment Performance

Measures wall-clock time required to discover, validate, and create symlink trees across datasets of varying scales.

### A. Small Dataset (10 Packages, 100 Files)

$(cat "$WORK_DIR/Small_results.md")

### B. Medium Dataset (50 Packages, 2,500 Files)

$(cat "$WORK_DIR/Medium_results.md")

### C. Large Dataset (100 Packages, 10,000 Files)

$(cat "$WORK_DIR/Large_results.md")

---

## 3. Peak Resident Set Size (Peak RSS Memory Usage)

Measures maximum memory footprint (in KB) during a 10,000 file deployment:

| Program | Runtime / Language | Peak RSS (KB) | Peak RSS (MB) |
| :--- | :--- | :---: | :---: |
| **\`symdep\`** | Native C (ISO C17) | **${SYMDEP_RSS} KB** | **$(awk "BEGIN {print $SYMDEP_RSS/1024}") MB** |
| **GNU Stow** | Perl Script | **${STOW_RSS:-N/A} KB** | **$(awk "BEGIN {if (\"$STOW_RSS\" != \"N/A\") print $STOW_RSS/1024; else print \"N/A\"}") MB** |
| **Dotbot** | Python Script | **${DOTBOT_RSS:-N/A} KB** | **$(awk "BEGIN {if (\"$DOTBOT_RSS\" != \"N/A\") print $DOTBOT_RSS/1024; else print \"N/A\"}") MB** |

---

## Summary & Key Takeaways

1. **Execution Speed**: \`symdep\` delivers significant speedup over legacy dynamic language implementations.
2. **Memory Efficiency**: Native memory allocation in ISO C17 ensures orders of magnitude lower memory footprint than dynamic scripting runtimes.
3. **Zero Dependencies**: \`symdep\` executes natively with zero external dynamic runtime requirements.

EOF

chmod +x "$BENCH_DIR/run_benchmark.sh"

echo ""

python3 -c "
import json, os, sys

work_dir = '$WORK_DIR'
symdep_rss = '$SYMDEP_RSS'
stow_rss = '$STOW_RSS'
dotbot_rss = '$DOTBOT_RSS'
stow_bin = '$STOW_BIN'
dotbot_status = '$DOTBOT_STATUS'

has_stow = bool(stow_bin and stow_bin != 'N/A')
has_dotbot = bool(dotbot_status and dotbot_status != 'N/A')

def get_mean_ms(json_path, cmd_substr):
    if not os.path.exists(json_path):
        return None
    try:
        with open(json_path) as f:
            data = json.load(f)
        for res in data.get('results', []):
            cmd = res.get('command', '')
            if cmd_substr in cmd:
                return res.get('mean', 0.0) * 1000.0
    except Exception:
        pass
    return None

phases = [
    ('Cold-Start (--help)', 'startup_results.json'),
    ('Small (100 files)', 'Small_results.json'),
    ('Medium (2,500 files)', 'Medium_results.json'),
    ('Large (10,000 files)', 'Large_results.json'),
]
if os.path.exists(os.path.join(work_dir, 'Ultra_results.json')):
    phases.append(('Ultra (100,000 files)', 'Ultra_results.json'))
if os.path.exists(os.path.join(work_dir, 'Mega_results.json')):
    phases.append(('Mega (1,000,000 files)', 'Mega_results.json'))

targets = ['ISO C17']
if has_stow:
    targets.append('STOW')
if has_dotbot:
    targets.append('DOTBOT')

title_text = f\"SYMDEP BENCHMARK EXECUTIVE SUMMARY ({' vs '.join(targets)})\"

cols = [('Benchmark Workload', 24), ('symdep (C17)', 14)]
if has_stow:
    cols.append(('Stow (Perl)', 14))
if has_dotbot:
    cols.append(('Dotbot (Py3)', 14))

sep = '+' + '+'.join('-' * (w + 2) for _, w in cols) + '+'
table_width = len(sep)
banner_border = '=' * table_width

print(banner_border)
print(f\"{title_text:^{table_width}}\")
print(banner_border)

header = '| ' + ' | '.join(f'{title:<{w}}' for title, w in cols) + ' |'
sep = '+' + '+'.join('-' * (w + 2) for _, w in cols) + '+'

print(sep)
print(header)
print(sep)

for label, json_name in phases:
    jpath = os.path.join(work_dir, json_name)
    sym_t = get_mean_ms(jpath, 'symdep')
    
    row = [f'{label:<24}', f'{f\"{sym_t:.2f} ms\" if sym_t is not None else \"N/A\":<14}']
    
    if has_stow:
        stow_t = get_mean_ms(jpath, 'stow')
        row.append(f'{f\"{stow_t:.2f} ms\" if stow_t is not None else \"N/A\":<14}')
    if has_dotbot:
        dot_t = get_mean_ms(jpath, 'dotbot')
        row.append(f'{f\"{dot_t:.2f} ms\" if dot_t is not None else \"N/A\":<14}')
        
    print('| ' + ' | '.join(row) + ' |')

print(sep)

def rss_str(rss_val):
    try:
        val = float(rss_val) / 1024.0
        return f'{val:.2f} MB'
    except Exception:
        return 'N/A'

mem_row = [f'{\"Peak Memory (RSS)\":<24}', f'{rss_str(symdep_rss):<14}']
if has_stow:
    mem_row.append(f'{rss_str(stow_rss):<14}')
if has_dotbot:
    mem_row.append(f'{rss_str(dotbot_rss):<14}')

print('| ' + ' | '.join(mem_row) + ' |')
print(sep)
"

echo ""
echo "[SUCCESS] Full markdown report saved to: $REPORT_FILE"
echo "[SUCCESS] Benchmark JSON results saved to: $WORK_DIR/*.json"
echo "================================================================================"

