#!/usr/bin/env python3
"""產生隨機的 CBI 測資。

用法：
    python3 gen_input.py SEED N DIM SRC_FANOUT SRC_LENGTH [-o FILE] [--w-skew W]

例如：
    python3 gen_input.py 1 200 500 3 300 -o in.cbi

同一組參數 + 同一個 SEED 一定產生一樣的檔案。
buffer library 固定用作業範例的 SMALL / MEDIUM / LARGE。
"""
import argparse
import random
import sys

BUFLIB = [("SMALL", 3, 90, 10), ("MEDIUM", 6, 120, 16), ("LARGE", 9, 190, 22)]


def generate(seed, n, dim, src_fanout, src_length, w_skew=1):
    rng = random.Random(seed)
    # SRC + n 個 sink，座標都不重複
    pts = set()
    while len(pts) < n + 1:
        pts.add((rng.randint(0, dim), rng.randint(0, dim)))
    pts = list(pts)
    rng.shuffle(pts)

    lines = [".limit",
             f"fanout {src_fanout}",
             f"length {src_length}",
             f".dimx {dim}",
             f".dimy {dim}",
             ".e", "",
             f".buflib {len(BUFLIB)}"]
    lines += [f"{t} {f} {l} {c}" for t, f, l, c in BUFLIB]
    lines += [".e", "", ".objective", f"w_skew {w_skew}", ".e", "", f".pin {n + 1}"]
    lines += [f"{x} {y}" for x, y in pts]
    lines += [".e"]
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser(description="產生隨機 CBI 測資")
    ap.add_argument("seed", type=int)
    ap.add_argument("n", type=int, help="sink 數量")
    ap.add_argument("dim", type=int, help="晶片邊長（dimx = dimy）")
    ap.add_argument("src_fanout", type=int)
    ap.add_argument("src_length", type=int)
    ap.add_argument("--w-skew", type=int, default=1)
    ap.add_argument("-o", "--output", help="輸出檔，不給就印到 stdout")
    args = ap.parse_args()

    if (args.dim + 1) ** 2 < args.n + 1:
        sys.exit("晶片太小，放不下這麼多 pin")
    text = generate(args.seed, args.n, args.dim, args.src_fanout, args.src_length, args.w_skew)
    if args.output:
        with open(args.output, "w") as f:
            f.write(text)
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
