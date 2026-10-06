#!/usr/bin/env python3
"""批次測試 cbi：產生隨機測資 → 執行 → 獨立驗證輸出 → 輸出 CSV、Markdown 表格與比較圖。

用法（在 project_2 底下）：
    python3 test/bench.py                          # 預設 5 種設定 × seed 1-5
    python3 test/bench.py --seeds 1-10 --iters 300000
    python3 test/bench.py --config 500 800 3 300 --config 2000 1500 4 400 --seeds 3,7,11
    python3 test/bench.py --time 0                 # 關掉 local search，只比較建樹方法

--seeds 決定測資（同一個 seed 一定產生同一份測資）；--ls-seed 決定 local search 的亂數。
想要結果完全可重現，用 --iters（固定迭代次數）而不是 --time。

輸出到 --out 指定的資料夾（預設 test/results）：
    results.csv     每一組測資 × 每一種方法的 cost / skew / Score
    summary.md      各設定的平均值表格
    compare.png     best build / local search 相對於 naive 的 Score、cost、skew
    inputs/ outputs/  測資與 cbi 的輸出檔
"""
import argparse
import csv
import os
import re
import subprocess
import sys
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_input import generate  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_CONFIGS = [
    (20, 110, 2, 100),
    (200, 500, 3, 300),
    (1000, 1000, 2, 200),
    (50, 1000, 1, 50),
    (3000, 2000, 4, 400),
]
# 建樹方法不寫死：cbi 在 stderr 印出的每一行「名稱: cost X, skew Y, Score Z」都算一種方法，
# 新增方法時只要 main.cpp 照這個格式印，這裡不用改。BASELINE 是圖表 100% 的基準
BASELINE = "greedy + naive"

# 配色（dataviz 預設 palette，light mode）
SURFACE, TEXT, TEXT2, GRID, AXIS = "#fcfcfb", "#0b0b0b", "#52514e", "#e6e5e0", "#c3c2b7"
SERIES = ["#2a78d6", "#eb6834"]   # best build, local search


def parse_seeds(text):
    seeds = []
    for part in text.split(","):
        if "-" in part:
            a, b = part.split("-")
            seeds += range(int(a), int(b) + 1)
        else:
            seeds.append(int(part))
    return seeds


def config_name(cfg):
    n, dim, sf, sl = cfg
    return f"n{n}_d{dim}_f{sf}_l{sl}"


# ---------------- 獨立驗證：不信任 cbi 自己的檢查，從輸入 / 輸出檔重新算 ----------------

def read_input(path):
    data = {"buflib": {}, "pins": []}
    section = None
    with open(path) as f:
        for raw in f:
            line = raw.split("#")[0].strip()
            if not line:
                continue
            tok = line.split()
            if tok[0] in (".limit", ".buflib", ".objective", ".pin"):
                section = tok[0]
            elif tok[0] == ".e":
                section = None
            elif section == ".limit":
                key = {"fanout": "src_f", "length": "src_l", ".dimx": "dimx", ".dimy": "dimy"}[tok[0]]
                data[key] = int(tok[1])
            elif tok[0] in (".dimx", ".dimy"):
                data[tok[0][1:]] = int(tok[1])
            elif section == ".buflib":
                data["buflib"][tok[0]] = tuple(map(int, tok[1:4]))
            elif section == ".objective":
                data[tok[0]] = int(tok[1])
            elif section == ".pin":
                data["pins"].append((int(tok[0]), int(tok[1])))
    return data


def verify(inp_path, out_path):
    """回傳 (錯誤訊息 list, T_max, T_min, cost, Score)。"""
    d = read_input(inp_path)
    errors = []
    pos = {"SRC": d["pins"][0]}
    for i, p in enumerate(d["pins"][1:], 1):
        pos[f"S{i}"] = p
    btype = {}
    children = defaultdict(list)
    level_of = {"SRC": 0}
    section = None
    with open(out_path) as f:
        for raw in f:
            line = raw.split("#")[0].strip()
            if not line:
                continue
            tok = line.split()
            if tok[0] in (".buffer", ".level"):
                section = tok[0]
            elif tok[0] == ".e":
                section = None
            elif section == ".buffer":
                name, t, x, y = tok[0], tok[1], int(tok[2]), int(tok[3])
                btype[name] = t
                pos[name] = (x, y)
            elif section == ".level":
                k = int(tok[0])
                for parent, kids in re.findall(r"(\w+):\{([^}]*)\}", line):
                    if level_of.get(parent) != k - 1:
                        errors.append(f"{parent} 不在第 {k - 1} 層，卻在第 {k} 層當 parent")
                    for c in kids.split():
                        if c in level_of:
                            errors.append(f"{c} 出現不只一次")
                        level_of[c] = k
                        children[parent].append(c)

    m = len(btype)
    if sorted(btype) != sorted(f"B{i}" for i in range(1, m + 1)):
        errors.append("buffer 編號不是連續的 B1..Bm")
    for name in btype:
        if btype[name] not in d["buflib"]:
            errors.append(f"{name} 的 type {btype[name]} 不存在")
        x, y = pos[name]
        if not (0 <= x <= d["dimx"] and 0 <= y <= d["dimy"]):
            errors.append(f"{name} 超出晶片範圍")
        if not children[name]:
            errors.append(f"{name} 沒有 children")
    if len(set(pos.values())) != len(pos):
        errors.append("有元件座標重複")
    for name in list(pos):
        if name != "SRC" and name not in level_of:
            errors.append(f"{name} 沒接到 SRC")
        if name.startswith("S") and name != "SRC" and children[name]:
            errors.append(f"sink {name} 不能當 parent")

    def dist(a, b):
        return abs(pos[a][0] - pos[b][0]) + abs(pos[a][1] - pos[b][1])

    for parent, kids in children.items():
        if parent == "SRC":
            F, L = d["src_f"], d["src_l"]
        elif parent in btype and btype[parent] in d["buflib"]:
            F, L, _ = d["buflib"][btype[parent]]
        else:
            continue
        total = sum(dist(parent, c) for c in kids if c in pos)
        if len(kids) > F or total > L:
            errors.append(f"{parent} 超過限制：fanout {len(kids)}/{F}, length {total}/{L}")

    # arrival time
    arrival = {"SRC": 0}
    stack = ["SRC"]
    while stack:
        u = stack.pop()
        for c in children[u]:
            if c in pos:
                arrival[c] = arrival[u] + dist(u, c)
                stack.append(c)
    sinks = [arrival[f"S{i}"] for i in range(1, len(d["pins"])) if f"S{i}" in arrival]
    t_max, t_min = (max(sinks), min(sinks)) if sinks else (0, 0)
    cost = sum(d["buflib"][t][2] for t in btype.values() if t in d["buflib"])
    score = cost + d.get("w_skew", 1) * (t_max - t_min)
    return errors, t_max, t_min, cost, score


# ---------------- 執行 ----------------

LINE_RE = re.compile(r"^(.*): cost (\d+), skew (\d+), Score (\d+)$")
STDOUT_RE = re.compile(r"T_max: (\d+), T_min: (\d+).*Score: (\d+)")


def run_one(args, cfg, seed):
    name = f"{config_name(cfg)}_s{seed}"
    inp = os.path.join(args.out, "inputs", name + ".cbi")
    out = os.path.join(args.out, "outputs", name + ".cbi")
    with open(inp, "w") as f:
        f.write(generate(seed, *cfg, w_skew=args.w_skew))

    cmd = [args.bin, inp, out, "--time", str(args.time), "--seed", str(args.ls_seed)]
    if args.iters:
        cmd += ["--iters", str(args.iters)]
    p = subprocess.run(cmd, capture_output=True, text=True, timeout=args.timeout)
    if p.returncode != 0:
        return name, cfg, seed, None, [f"cbi 結束碼 {p.returncode}: {p.stderr.strip()[-200:]}"]

    rows = {}
    for line in p.stderr.splitlines():
        mt = LINE_RE.match(line.strip())
        if mt:
            method = "local search" if mt.group(1) == "after local search" else mt.group(1)
            rows[method] = tuple(map(int, mt.group(2, 3, 4)))
    errors, t_max, t_min, cost, score = verify(inp, out)
    ms = STDOUT_RE.search(p.stdout)
    if not ms:
        errors.append("stdout 沒有 T_max / T_min / Score")
    elif (int(ms.group(1)), int(ms.group(2)), int(ms.group(3))) != (t_max, t_min, score):
        errors.append(f"stdout 的數字和輸出檔算出來的不一致：{ms.group(0)} vs "
                      f"T_max {t_max}, T_min {t_min}, Score {score}")
    rows["final"] = (cost, t_max - t_min, score)
    return name, cfg, seed, rows, errors


# ---------------- 統計與畫圖 ----------------

def mean(xs):
    return sum(xs) / len(xs) if xs else float("nan")


def build_methods(results):
    """依照第一次出現的順序，列出所有建樹方法（不含 local search）。"""
    methods = []
    for r in results:
        for m in r["rows"] or {}:
            if m not in ("local search", "final") and m not in methods:
                methods.append(m)
    return methods


def summarize(results, configs, methods):
    """每個設定：baseline / best build / local search 的平均值，以及相對 baseline 的百分比。"""
    baseline = BASELINE if BASELINE in methods else methods[0]
    summary = []
    for cfg in configs:
        runs = [r for r in results if r["cfg"] == cfg and r["rows"]]
        if not runs:
            continue
        s = {"cfg": cfg, "n": len(runs), "methods": {}}
        for method in methods + ["local search"]:
            vals = [r["rows"][method] for r in runs if method in r["rows"]]
            if vals:
                s["methods"][method] = tuple(mean([v[i] for v in vals]) for i in range(3))
        stages = {"naive": [], "best build": [], "local search": []}
        for r in runs:
            builds = [r["rows"][m] for m in methods if m in r["rows"]]
            stages["naive"].append(r["rows"][baseline])
            stages["best build"].append(min(builds, key=lambda v: v[2]))
            stages["local search"].append(r["rows"]["final"])
        s["stages"] = {k: tuple(mean([v[i] for v in vals]) for i in range(3)) for k, vals in stages.items()}
        # 每組測資先算相對 naive 的百分比，再平均（大測資才不會蓋過小測資）
        pct = {}
        for k in ("best build", "local search"):
            pct[k] = tuple(mean([100.0 * b[i] / a[i] if a[i] else 100.0
                                 for a, b in zip(stages["naive"], stages[k])]) for i in range(3))
        s["pct"] = pct
        summary.append(s)
    return summary


def write_summary_md(path, summary, args, methods):
    with open(path, "w") as f:
        ls = f"--iters {args.iters}" if args.iters else f"--time {args.time}"
        f.write(f"# Benchmark\n\nseeds `{args.seeds}`，local search `{ls} --seed {args.ls_seed}`，w_skew {args.w_skew}\n\n")
        f.write("## 各階段（平均值）\n\n")
        f.write("| 設定 (n, dim, SRC F/L) | 組數 | 階段 | cost | skew | Score | Score 相對 naive |\n")
        f.write("|---|---|---|---|---|---|---|\n")
        for s in summary:
            n, dim, sf, sl = s["cfg"]
            for k in ("naive", "best build", "local search"):
                c, sk, sc = s["stages"][k]
                rel = "100%" if k == "naive" else f"{s['pct'][k][2]:.1f}%"
                f.write(f"| {n}, {dim}, {sf}/{sl} | {s['n']} | {k} | {c:.0f} | {sk:.0f} | {sc:.0f} | {rel} |\n")
        f.write("\n## 各建樹方法的平均 Score\n\n")
        f.write("| 設定 | " + " | ".join(methods + ["local search"]) + " |\n")
        f.write("|---" * (len(methods) + 2) + "|\n")
        for s in summary:
            n, dim, sf, sl = s["cfg"]
            vals = [s["methods"].get(m, (0, 0, float("nan")))[2] for m in methods + ["local search"]]
            best = min(vals[:-1])
            cells = [f"**{v:.0f}**" if v == best else f"{v:.0f}" for v in vals[:-1]] + [f"{vals[-1]:.0f}"]
            f.write(f"| {n}, {dim}, {sf}/{sl} | " + " | ".join(cells) + " |\n")


def plot(path_png, summary, out_dir):
    dat = os.path.join(out_dir, "compare.dat")
    with open(dat, "w") as f:
        for s in summary:
            n, dim, sf, sl = s["cfg"]
            label = f"n={n}\\n{dim}x{dim}, SRC F{sf} L{sl}"
            b, l = s["pct"]["best build"], s["pct"]["local search"]
            f.write(f"\"{label}\" {b[2]:.2f} {l[2]:.2f} {b[0]:.2f} {l[0]:.2f} {b[1]:.2f} {l[1]:.2f}\n")

    gp = os.path.join(out_dir, "compare.gp")
    panels = [("Score", 2), ("Buffer cost", 4), ("Skew", 6)]
    with open(gp, "w") as f:
        f.write(f"""set terminal pngcairo size 1200,1300 font "Sans,11" background "{SURFACE}"
set output "{path_png}"
set multiplot layout 3,1 title "Relative to naive greedy (= 100%, dashed line) — lower is better" font "Sans,13" textcolor rgb "{TEXT}"
set style data histograms
set style histogram clustered gap 1
set style fill solid 1.0 border lc rgb "{SURFACE}"
set boxwidth 0.92
set border 3 lc rgb "{AXIS}"
set tics textcolor rgb "{TEXT2}"
set xtics nomirror scale 0 font "Sans,10"
set ytics nomirror
set grid ytics lc rgb "{GRID}" lw 1
set format y "%g%%"
set yrange [0:*]
set offsets 0, 0, graph 0.12, 0
set bmargin 3.5
set lmargin 9
""")
        for i, (title, col) in enumerate(panels):
            if i == 0:
                f.write(f'set key top left horizontal textcolor rgb "{TEXT}" samplen 1.5\n')
            else:
                f.write("unset key\n")
            f.write(f'set title "{title}" textcolor rgb "{TEXT}"\n')
            f.write(
                f'plot "{dat}" using {col}:xtic(1) title "best build" lw 2 lc rgb "{SERIES[0]}", \\\n'
                f'     "" using {col + 1} title "+ local search" lw 2 lc rgb "{SERIES[1]}", \\\n'
                f'     "" using ($0 + 1.0/6):{col + 1}:(sprintf("%.0f%%", ${col + 1})) with labels offset 0,0.7 '
                f'font "Sans,8" textcolor rgb "{TEXT2}" notitle, \\\n'
                f'     100 with lines dt 2 lw 1.5 lc rgb "{TEXT2}" notitle\n')
        f.write("unset multiplot\n")
    p = subprocess.run(["gnuplot", gp], capture_output=True, text=True)
    if p.returncode != 0:
        print("gnuplot 失敗：", p.stderr, file=sys.stderr)


def main():
    ap = argparse.ArgumentParser(description="cbi 批次測試與比較")
    ap.add_argument("--bin", default=os.path.join(HERE, "..", "cbi"), help="cbi 執行檔（預設 project_2/cbi）")
    ap.add_argument("--seeds", default="1-5", help="測資 seed，例如 1-5 或 1,3,7")
    ap.add_argument("--config", nargs=4, type=int, action="append", metavar=("N", "DIM", "SRC_F", "SRC_L"),
                    help="測資設定，可重複；不給就用預設 5 種")
    ap.add_argument("--w-skew", type=int, default=1)
    ap.add_argument("--time", type=float, default=3.0, help="local search 秒數，0 = 關掉")
    ap.add_argument("--iters", type=int, default=0, help="local search 固定迭代次數（可重現）")
    ap.add_argument("--ls-seed", type=int, default=12345, help="local search 的亂數 seed")
    ap.add_argument("--jobs", type=int, default=1, help="同時跑幾個；用 --time 時建議 1，免得互搶 CPU")
    ap.add_argument("--timeout", type=float, default=300)
    ap.add_argument("--out", default=os.path.join(HERE, "results"))
    args = ap.parse_args()

    configs = [tuple(c) for c in args.config] if args.config else DEFAULT_CONFIGS
    seeds = parse_seeds(args.seeds)
    os.makedirs(os.path.join(args.out, "inputs"), exist_ok=True)
    os.makedirs(os.path.join(args.out, "outputs"), exist_ok=True)
    if not os.path.exists(args.bin):
        sys.exit(f"找不到 {args.bin}，先 make")

    jobs = [(cfg, s) for cfg in configs for s in seeds]
    results, n_bad = [], 0
    with ThreadPoolExecutor(max_workers=args.jobs) as ex:
        for name, cfg, seed, rows, errors in ex.map(lambda j: run_one(args, *j), jobs):
            status = "OK" if not errors else "ILLEGAL"
            final = rows["final"][2] if rows else "-"
            print(f"{name:32s} {status:8s} Score {final}")
            for e in errors[:5]:
                print("    " + e)
            n_bad += bool(errors)
            results.append({"name": name, "cfg": cfg, "seed": seed, "rows": rows, "legal": not errors})

    with open(os.path.join(args.out, "results.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["n", "dim", "src_fanout", "src_length", "seed", "method", "cost", "skew", "score", "legal"])
        for r in results:
            for method, (c, sk, sc) in (r["rows"] or {}).items():
                w.writerow([*r["cfg"], r["seed"], method, c, sk, sc, int(r["legal"])])

    methods = build_methods(results)
    if not methods:
        sys.exit("cbi 沒有印出任何方法的結果")
    summary = summarize(results, configs, methods)
    write_summary_md(os.path.join(args.out, "summary.md"), summary, args, methods)
    plot(os.path.join(args.out, "compare.png"), summary, args.out)

    print()
    for s in summary:
        n, dim, sf, sl = s["cfg"]
        st = s["stages"]
        print(f"n={n:<5d} dim={dim:<5d} SRC {sf}/{sl:<4d}  naive {st['naive'][2]:8.0f}  "
              f"best build {st['best build'][2]:8.0f}  local search {st['local search'][2]:8.0f}  "
              f"({s['pct']['local search'][2]:.1f}% of naive)")
    print(f"\n{len(results) - n_bad}/{len(results)} 合法，結果在 {args.out}/")
    sys.exit(1 if n_bad else 0)


if __name__ == "__main__":
    main()
