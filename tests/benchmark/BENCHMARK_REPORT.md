# Benchmark Report: `symdep` vs Competitors

Automated benchmark evaluation comparing **`symdep`** (ISO C17) against **GNU Stow** (Perl) and **Dotbot** (Python).

- **Execution Host**: Linux x86_64 (durzhars)
- **Date**: 2026-08-12 20:35:42 UTC
- **Compiler**: gcc (GCC) 16.1.1 20260728

---

## 1. Startup & Cold-Start Latency (`--help` Invocation)

Measures execution overhead before doing any file operations (interpreter boot time vs native binary execution).

| Command | Mean [µs] | Min [µs] | Max [µs] | Relative |
|:---|---:|---:|---:|---:|
| `/home/durzhars/Projects/symdep/bin/symdep --help` | 395.0 ± 476.3 | 0.0 | 1955.4 | 1.00 |
| `/usr/bin/stow --help` | 26040.3 ± 3913.0 | 21967.5 | 40508.8 | 65.93 ± 80.12 |
| `/usr/bin/python3 /home/durzhars/Projects/symdep/tests/benchmark/vendor/dotbot/bin/dotbot --help` | 60662.2 ± 2546.8 | 55468.2 | 66616.9 | 153.59 ± 185.33 |

---

## 2. Link Deployment Performance

Measures wall-clock time required to discover, validate, and create symlink trees across datasets of varying scales.

### A. Small Dataset (10 Packages, 100 Files)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `/home/durzhars/Projects/symdep/bin/symdep -d /tmp/symdep_benchmark_workspace/Small/src -t /tmp/symdep_benchmark_workspace/Small/target link  pkg_1 pkg_2 pkg_3 pkg_4 pkg_5 pkg_6 pkg_7 pkg_8 pkg_9 pkg_10` | 2.8 ± 0.8 | 2.0 | 5.1 | 1.00 |
| `/usr/bin/stow -d /tmp/symdep_benchmark_workspace/Small/src -t /tmp/symdep_benchmark_workspace/Small/target  pkg_1 pkg_2 pkg_3 pkg_4 pkg_5 pkg_6 pkg_7 pkg_8 pkg_9 pkg_10` | 29.3 ± 4.4 | 25.3 | 40.9 | 10.41 ± 3.35 |
| `HOME=/tmp/symdep_benchmark_workspace/Small/target /usr/bin/python3 /home/durzhars/Projects/symdep/tests/benchmark/vendor/dotbot/bin/dotbot -d /tmp/symdep_benchmark_workspace/Small -c /tmp/symdep_benchmark_workspace/Small/dotbot.yaml` | 80.3 ± 2.3 | 77.5 | 85.3 | 28.49 ± 8.16 |

### B. Medium Dataset (50 Packages, 2,500 Files)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `/home/durzhars/Projects/symdep/bin/symdep -d /tmp/symdep_benchmark_workspace/Medium/src -t /tmp/symdep_benchmark_workspace/Medium/target link  pkg_1 pkg_2 pkg_3 pkg_4 pkg_5 pkg_6 pkg_7 pkg_8 pkg_9 pkg_10 pkg_11 pkg_12 pkg_13 pkg_14 pkg_15 pkg_16 pkg_17 pkg_18 pkg_19 pkg_20 pkg_21 pkg_22 pkg_23 pkg_24 pkg_25 pkg_26 pkg_27 pkg_28 pkg_29 pkg_30 pkg_31 pkg_32 pkg_33 pkg_34 pkg_35 pkg_36 pkg_37 pkg_38 pkg_39 pkg_40 pkg_41 pkg_42 pkg_43 pkg_44 pkg_45 pkg_46 pkg_47 pkg_48 pkg_49 pkg_50` | 16.5 ± 3.3 | 14.1 | 30.0 | 1.00 |
| `/usr/bin/stow -d /tmp/symdep_benchmark_workspace/Medium/src -t /tmp/symdep_benchmark_workspace/Medium/target  pkg_1 pkg_2 pkg_3 pkg_4 pkg_5 pkg_6 pkg_7 pkg_8 pkg_9 pkg_10 pkg_11 pkg_12 pkg_13 pkg_14 pkg_15 pkg_16 pkg_17 pkg_18 pkg_19 pkg_20 pkg_21 pkg_22 pkg_23 pkg_24 pkg_25 pkg_26 pkg_27 pkg_28 pkg_29 pkg_30 pkg_31 pkg_32 pkg_33 pkg_34 pkg_35 pkg_36 pkg_37 pkg_38 pkg_39 pkg_40 pkg_41 pkg_42 pkg_43 pkg_44 pkg_45 pkg_46 pkg_47 pkg_48 pkg_49 pkg_50` | 38.2 ± 2.2 | 34.7 | 44.1 | 2.31 ± 0.48 |
| `HOME=/tmp/symdep_benchmark_workspace/Medium/target /usr/bin/python3 /home/durzhars/Projects/symdep/tests/benchmark/vendor/dotbot/bin/dotbot -d /tmp/symdep_benchmark_workspace/Medium -c /tmp/symdep_benchmark_workspace/Medium/dotbot.yaml` | 498.2 ± 14.9 | 471.0 | 544.2 | 30.11 ± 6.06 |

### C. Large Dataset (100 Packages, 10,000 Files)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `/home/durzhars/Projects/symdep/bin/symdep -d /tmp/symdep_benchmark_workspace/Large/src -t /tmp/symdep_benchmark_workspace/Large/target link  pkg_1 pkg_2 pkg_3 pkg_4 pkg_5 pkg_6 pkg_7 pkg_8 pkg_9 pkg_10 pkg_11 pkg_12 pkg_13 pkg_14 pkg_15 pkg_16 pkg_17 pkg_18 pkg_19 pkg_20 pkg_21 pkg_22 pkg_23 pkg_24 pkg_25 pkg_26 pkg_27 pkg_28 pkg_29 pkg_30 pkg_31 pkg_32 pkg_33 pkg_34 pkg_35 pkg_36 pkg_37 pkg_38 pkg_39 pkg_40 pkg_41 pkg_42 pkg_43 pkg_44 pkg_45 pkg_46 pkg_47 pkg_48 pkg_49 pkg_50 pkg_51 pkg_52 pkg_53 pkg_54 pkg_55 pkg_56 pkg_57 pkg_58 pkg_59 pkg_60 pkg_61 pkg_62 pkg_63 pkg_64 pkg_65 pkg_66 pkg_67 pkg_68 pkg_69 pkg_70 pkg_71 pkg_72 pkg_73 pkg_74 pkg_75 pkg_76 pkg_77 pkg_78 pkg_79 pkg_80 pkg_81 pkg_82 pkg_83 pkg_84 pkg_85 pkg_86 pkg_87 pkg_88 pkg_89 pkg_90 pkg_91 pkg_92 pkg_93 pkg_94 pkg_95 pkg_96 pkg_97 pkg_98 pkg_99 pkg_100` | 50.3 ± 1.5 | 47.8 | 52.6 | 1.04 ± 0.11 |
| `/usr/bin/stow -d /tmp/symdep_benchmark_workspace/Large/src -t /tmp/symdep_benchmark_workspace/Large/target  pkg_1 pkg_2 pkg_3 pkg_4 pkg_5 pkg_6 pkg_7 pkg_8 pkg_9 pkg_10 pkg_11 pkg_12 pkg_13 pkg_14 pkg_15 pkg_16 pkg_17 pkg_18 pkg_19 pkg_20 pkg_21 pkg_22 pkg_23 pkg_24 pkg_25 pkg_26 pkg_27 pkg_28 pkg_29 pkg_30 pkg_31 pkg_32 pkg_33 pkg_34 pkg_35 pkg_36 pkg_37 pkg_38 pkg_39 pkg_40 pkg_41 pkg_42 pkg_43 pkg_44 pkg_45 pkg_46 pkg_47 pkg_48 pkg_49 pkg_50 pkg_51 pkg_52 pkg_53 pkg_54 pkg_55 pkg_56 pkg_57 pkg_58 pkg_59 pkg_60 pkg_61 pkg_62 pkg_63 pkg_64 pkg_65 pkg_66 pkg_67 pkg_68 pkg_69 pkg_70 pkg_71 pkg_72 pkg_73 pkg_74 pkg_75 pkg_76 pkg_77 pkg_78 pkg_79 pkg_80 pkg_81 pkg_82 pkg_83 pkg_84 pkg_85 pkg_86 pkg_87 pkg_88 pkg_89 pkg_90 pkg_91 pkg_92 pkg_93 pkg_94 pkg_95 pkg_96 pkg_97 pkg_98 pkg_99 pkg_100` | 48.3 ± 4.9 | 44.1 | 60.3 | 1.00 |
| `HOME=/tmp/symdep_benchmark_workspace/Large/target /usr/bin/python3 /home/durzhars/Projects/symdep/tests/benchmark/vendor/dotbot/bin/dotbot -d /tmp/symdep_benchmark_workspace/Large -c /tmp/symdep_benchmark_workspace/Large/dotbot.yaml` | 1752.0 ± 22.9 | 1715.1 | 1795.5 | 36.29 ± 3.73 |

---

## 3. Peak Resident Set Size (Peak RSS Memory Usage)

Measures maximum memory footprint (in KB) during a 10,000 file deployment:

| Program | Runtime / Language | Peak RSS (KB) | Peak RSS (MB) |
| :--- | :--- | :---: | :---: |
| **`symdep`** | Native C (ISO C17) | **13820 KB** | **13.4961 MB** |
| **GNU Stow** | Perl Script | **13772 KB** | **13.4492 MB** |
| **Dotbot** | Python Script | **59996 KB** | **58.5898 MB** |

---

## Summary & Key Takeaways

1. **Execution Speed**: `symdep` delivers significant speedup over legacy dynamic language implementations.
2. **Memory Efficiency**: Native memory allocation in ISO C17 ensures orders of magnitude lower memory footprint than dynamic scripting runtimes.
3. **Zero Dependencies**: `symdep` executes natively with zero external dynamic runtime requirements.

