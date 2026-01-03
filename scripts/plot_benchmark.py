#!/usr/bin/env python3
"""
Benchmark Visualization Script for libdynemit

This script reads CSV benchmark data and generates publication-quality charts
comparing performance across different SIMD levels and array sizes.

Usage:
    python plot_benchmark.py --input results1.csv:Label1 results2.csv:Label2 \\
                             --output docs/img/benchmark.png \\
                             --title "Performance Comparison"
"""

import argparse
import csv
import sys
from pathlib import Path
from typing import Dict, List, Tuple

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np


def read_csv_data(filepath: str) -> Tuple[List[int], List[float], List[float], List[float], List[float], List[float], List[float], List[float], str]:
    """
    Read benchmark CSV data from file.
    
    Returns:
        Tuple of (array_sizes, median_ms, mean_ms, stddev_ms, min_ms, max_ms, p99_ms, gflops, simd_level)
    """
    array_sizes = []
    median_ms = []
    mean_ms = []
    stddev_ms = []
    min_ms = []
    max_ms = []
    p99_ms = []
    gflops = []
    simd_level = None
    
    try:
        with open(filepath, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                array_sizes.append(int(row['array_size']))
                median_ms.append(float(row['median_ms']))
                mean_ms.append(float(row['mean_ms']))
                stddev_ms.append(float(row['stddev_ms']))
                min_ms.append(float(row['min_ms']))
                max_ms.append(float(row['max_ms']))
                p99_ms.append(float(row['p99_ms']))
                gflops.append(float(row['gflops']))
                if simd_level is None:
                    simd_level = row['simd_level']
        
        return array_sizes, median_ms, mean_ms, stddev_ms, min_ms, max_ms, p99_ms, gflops, simd_level
    
    except FileNotFoundError:
        print(f"Error: File '{filepath}' not found.", file=sys.stderr)
        sys.exit(1)
    except KeyError as e:
        print(f"Error: Missing column {e} in CSV file '{filepath}'.", file=sys.stderr)
        print("Expected columns: array_size,median_ms,mean_ms,stddev_ms,min_ms,max_ms,p99_ms,gflops,simd_level", file=sys.stderr)
        sys.exit(1)
    except ValueError as e:
        print(f"Error: Invalid data format in '{filepath}': {e}", file=sys.stderr)
        sys.exit(1)


def plot_benchmark(datasets: Dict[str, Tuple], output_path: str, title: str, 
                  metric: str = 'time', auto_title: bool = False):
    """
    Generate benchmark comparison chart.
    
    Args:
        datasets: Dict mapping label -> (array_sizes, median_ms, mean_ms, stddev_ms, min_ms, max_ms, p99_ms, gflops, simd_level)
        output_path: Path to save the output image
        title: Chart title
        metric: 'time' or 'gflops' - which metric to plot
        auto_title: Whether to auto-generate title from CPU names
    """
    plt.figure(figsize=(12, 7))
    
    # Sort datasets by performance (best to worst)
    # For time metric: lower median = better, higher = worse
    # Use the median time at the largest array size for sorting
    if metric == 'time':
        sorted_items = sorted(datasets.items(), 
                            key=lambda x: x[1][1][-1],  # x[1][1] = median_ms list, [-1] = last element
                            reverse=False)  # Reverse=False for best-to-worst (lowest time first)
    else:
        # For gflops: higher = better
        sorted_items = sorted(datasets.items(), 
                            key=lambda x: x[1][7][-1],  # x[1][7] = gflops list, [-1] = last element
                            reverse=True)  # Higher gflops first (best to worst)
    
    # Convert back to dict maintaining order
    datasets = dict(sorted_items)
    
    # Color scheme - expanded palette for many datasets
    colors = [
        '#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b',
        '#e377c2', '#7f7f7f', '#bcbd22', '#17becf', '#aec7e8', '#ffbb78',
        '#98df8a', '#ff9896', '#c5b0d5', '#c49c94', '#f7b6d2', '#c7c7c7',
        '#dbdb8d', '#9edae5', '#393b79', '#637939', '#8c6d31', '#843c39'
    ]
    markers = ['o', 's', '^', 'D', 'v', 'p', '*', 'X', 'P', 'h', '<', '>', 
               'd', '8', 'H', '1', '2', '3', '4', '+', 'x']
    
    for idx, (label, data) in enumerate(datasets.items()):
        array_sizes, median_ms, mean_ms, stddev_ms, min_ms, max_ms, p99_ms, gflops, simd_level = data
        
        color = colors[idx % len(colors)]
        marker = markers[idx % len(markers)]
        
        if metric == 'time':
            y_data = median_ms
            y_std = stddev_ms
            y_p99 = p99_ms
        else:
            y_data = gflops
            # For gflops, we don't have stddev, so approximate from min/max
            y_std = [(max_val - min_val) / 4.0 for max_val, min_val in zip(max_ms, min_ms)]
            y_p99 = gflops  # p99 doesn't make sense for gflops derived from median
        
        # Convert to numpy arrays for easier manipulation
        x = np.array(array_sizes)
        y = np.array(y_data)
        yerr = np.array(y_std)
        y_p99_arr = np.array(y_p99)
        
        # Plot main line (median)
        line = plt.plot(x, y, 
                       marker=marker, 
                       markersize=3,
                       linewidth=1.5,
                       label=label,
                       color=color,
                       zorder=3)
        
        # Add shaded error region (±1 std dev)
        plt.fill_between(x, y - yerr, y + yerr, 
                        alpha=0.2, 
                        color=color,
                        zorder=1)
        
        # Add error bars
        plt.errorbar(x, y, yerr=yerr,
                    fmt='none',
                    ecolor=color,
                    alpha=0.4,
                    capsize=2,
                    capthick=0.5,
                    linewidth=0.8,
                    zorder=2)
    
    # Generate auto title if requested
    if auto_title:
        cpu_names = list(datasets.keys())
        if len(cpu_names) == 1:
            title = f"Vector Multiply Performance - {cpu_names[0]}"
        elif len(cpu_names) == 2:
            title = f"Vector Multiply Performance - {cpu_names[0]} vs {cpu_names[1]}"
        else:
            title = f"Vector Multiply Performance - {len(cpu_names)} CPU Comparison"
    
    # Labels and title
    xlabel = 'Number of elements (32-bit floats, 4 bytes each)'
    plt.xlabel(xlabel, fontsize=11)
    
    if metric == 'time':
        ylabel = 'Time [s] - lower is better'
    else:
        ylabel = 'GFLOP/s - higher is better'
    
    plt.ylabel(ylabel, fontsize=11)
    plt.title(title, fontsize=13, fontweight='bold')
    
    # Create custom legend
    if metric == 'time':
        # Add explanation for the visual elements
        legend_elements = []
        for idx, label in enumerate(datasets.keys()):
            color = colors[idx % len(colors)]
            marker = markers[idx % len(markers)]
            legend_elements.append(plt.Line2D([0], [0], marker=marker, color=color, 
                                             linewidth=1.5, markersize=5, label=label))
        
        # Add legend entries for error visualization
        from matplotlib.patches import Patch
        legend_elements.append(Patch(facecolor='gray', alpha=0.2, label='±1σ region'))
        
        plt.legend(handles=legend_elements, loc='upper left', fontsize=9)
    else:
        plt.legend(loc='upper left', fontsize=10)
    
    plt.grid(True, alpha=0.3, linestyle='--', linewidth=0.5)
    
    # Format x-axis to show clean numbers without scientific notation
    ax = plt.gca()
    ax.set_xlim(left=0, right=max([max(data[0]) for data in datasets.values()]) * 1.05)
    
    # Add note about trials below x-axis
    ax.text(0.5, -0.1, '10 trials per data point, same build, GCC 15.2', 
            transform=ax.transAxes, 
            ha='center', 
            fontsize=9, 
            style='italic',
            color='gray')
    
    if metric == 'time':
        ax.set_ylim(bottom=0)
        
        # Add secondary y-axis for milliseconds
        ax2 = ax.twinx()
        # Get the y-limits from the primary axis (in seconds)
        y_min, y_max = ax.get_ylim()
        # Set the secondary axis to show the same range but in milliseconds
        ax2.set_ylim(y_min * 1000, y_max * 1000)
        ax2.set_ylabel('Time [ms] - lower is better', fontsize=10)
        
        # Increase the number of ticks on the secondary axis
        ax2.yaxis.set_major_locator(ticker.MaxNLocator(nbins=25, integer=False))
        # Add minor ticks for even finer granularity (without labels)
        ax2.yaxis.set_minor_locator(ticker.AutoMinorLocator(2))
        ax2.tick_params(which='minor', length=3, color='gray')
        ax2.grid(False)  # Don't show grid for secondary axis
    
    # Custom formatter for x-axis: show values as K, M
    def format_func(value, tick_number):
        if value == 0:
            return '0'
        elif value >= 1_000_000:
            # Format as millions
            if value % 1_000_000 == 0:
                return f'{int(value / 1_000_000)}M'
            else:
                return f'{value / 1_000_000:.1f}M'
        elif value >= 1_000:
            # Format as thousands
            if value % 1_000 == 0:
                return f'{int(value / 1_000)}K'
            else:
                return f'{value / 1_000:.1f}K'
        else:
            return f'{int(value)}'
    
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(format_func))
    
    # Tight layout to prevent label cutoff
    plt.tight_layout()
    
    # Save the figure
    output_file = Path(output_path)
    output_file.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Chart saved to: {output_path}")


def infer_label_from_filename(filepath: str) -> str:
    """
    Infer a human-readable label from the filename.
    
    Examples:
        results_amd_ryzen_9_9950x3d_avx_512f.csv -> AMD Ryzen 9 9950X3D (AVX-512F)
        results_intel_xeon_e5_2680_v4_avx2.csv -> Intel Xeon E5-2680 v4 (AVX2)
    """
    filename = Path(filepath).stem  # Get filename without extension
    
    # Remove 'results_' prefix if present
    if filename.startswith('results_'):
        filename = filename[8:]
    
    # Split by underscores
    parts = filename.split('_')
    
    # Try to identify SIMD level (usually at the end)
    simd_patterns = ['avx_512f', 'avx512f', 'avx2', 'avx', 'sse4_2', 'sse42', 'sse2', 'scalar']
    simd_level = None
    cpu_parts = []
    
    # Find where SIMD level starts
    for i, part in enumerate(parts):
        # Check if this and following parts form a SIMD level
        remaining = '_'.join(parts[i:])
        if any(remaining.startswith(pattern) for pattern in simd_patterns):
            cpu_parts = parts[:i]
            simd_level = remaining
            break
    
    if not simd_level:
        # Couldn't parse, just use the filename
        return filename.replace('_', ' ').title()
    
    # Format CPU name
    cpu_name = ' '.join(cpu_parts).upper()
    
    # Format SIMD level
    simd_map = {
        'avx_512f': 'AVX-512F',
        'avx512f': 'AVX-512F',
        'avx2': 'AVX2',
        'avx': 'AVX',
        'sse4_2': 'SSE4.2',
        'sse42': 'SSE4.2',
        'sse2': 'SSE2',
        'scalar': 'Scalar'
    }
    simd_display = simd_map.get(simd_level, simd_level.upper())
    
    return f"{cpu_name} ({simd_display})"


def main():
    parser = argparse.ArgumentParser(
        description='Generate benchmark visualization charts for libdynemit',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Auto-infer labels from filenames (recommended)
  python plot_benchmark.py bench/data/results_*.csv
  
  # Single dataset with custom label
  python plot_benchmark.py --input results.csv:AVX2
  
  # Multiple datasets with auto-inferred labels
  python plot_benchmark.py bench/data/results_amd_*.csv bench/data/results_intel_*.csv
  
  # Custom output and title
  python plot_benchmark.py bench/data/*.csv \\
    --output my_chart.png \\
    --title "My Custom Title"
  
  # Plot GFLOP/s instead of time
  python plot_benchmark.py bench/data/*.csv --metric gflops
        """
    )
    
    parser.add_argument(
        'files',
        nargs='*',
        metavar='FILE',
        help='Input CSV files (labels auto-inferred from filenames)'
    )
    
    parser.add_argument(
        '--input', '-i',
        nargs='+',
        metavar='FILE[:LABEL]',
        help='Input CSV files with optional labels, format: file.csv or file.csv:Label'
    )
    
    parser.add_argument(
        '--output', '-o',
        default='docs/img/benchmark_vector_mul.png',
        help='Output image path (default: docs/img/benchmark_vector_mul.png)'
    )
    
    parser.add_argument(
        '--title', '-t',
        default=None,
        help='Chart title (auto-generated from CPU names if not specified)'
    )
    
    parser.add_argument(
        '--metric', '-m',
        choices=['time', 'gflops'],
        default='time',
        help='Metric to plot: time (ms) or gflops (default: time)'
    )
    
    args = parser.parse_args()
    
    # Collect all input files
    input_specs = []
    
    # Add positional file arguments
    if args.files:
        input_specs.extend(args.files)
    
    # Add --input arguments
    if args.input:
        input_specs.extend(args.input)
    
    if not input_specs:
        parser.error("No input files specified. Provide files as positional arguments or use --input")
    
    # Parse input files and labels
    datasets = {}
    for input_spec in input_specs:
        if ':' in input_spec:
            # Explicit label provided
            filepath, label = input_spec.split(':', 1)
        else:
            # Infer label from filename
            filepath = input_spec
            label = infer_label_from_filename(filepath)
        
        print(f"Reading {filepath} (label: {label})...")
        data = read_csv_data(filepath)
        datasets[label] = data
        
        array_sizes, median_ms, mean_ms, stddev_ms, min_ms, max_ms, p99_ms, gflops, simd_level = data
        print(f"  - SIMD level: {simd_level}")
        print(f"  - Data points: {len(array_sizes)}")
        print(f"  - Statistical trials per size (10 trials with median, stddev, p99)")
    
    # Determine if we should auto-generate title
    auto_title = args.title is None
    title = args.title if args.title else "Vector Multiply Performance Comparison"
    
    # Generate the plot
    print(f"\nGenerating chart...")
    plot_benchmark(datasets, args.output, title, args.metric, auto_title)
    print("Done!")


if __name__ == '__main__':
    main()

