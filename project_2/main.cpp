#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <set>
#include <queue>
#include <climits>
#include <cstdlib>
#include <cmath>
#include <err.h>
typedef struct buffer
{
    std::string type;
    int fanout;
    int length;
    int cost;
}Buffer;

typedef struct pin{
    int x;
    int y;
    
}Pin;

typedef struct input_data{
    int src_fanout = 0;
    int src_length = 0;
    int dim_x = 0;
    int dim_y = 0;
    int w_skew = 0;
    std::vector<Buffer> buflib;
    Pin src = {0, 0};
    std::vector<Pin> sinks;
}InputData;

typedef struct node{
    Pin pos;
    int type;                   // -1: sink, >=0: buflib 的 index
    std::vector<int> children;
    int dmin = 0;               // 從這個節點到底下 sinks 的最短 / 最長 delay
    int dmax = 0;
}Node;

typedef struct tree{
    std::vector<Node> nodes;    // [0, num_sinks) 是 sinks，後面是 buffers
    int num_sinks = 0;
    std::vector<int> src_children;
}Tree;

// BFS 之後的結果：buffer 編號、深度、arrival time
typedef struct tree_info{
    std::vector<int> buf_order; // 依 BFS 順序排列的 buffer（B1, B2, ...）
    std::vector<int> buf_id;
    std::vector<int> depth;
    std::vector<int> arrival;
    int max_level = 0;
}TreeInfo;

int calculate_distance(const Pin &p1, const Pin &p2)
{
    return abs(p1.x - p2.x) + abs(p1.y - p2.y);
}
// 讀下一個有效行（去掉 # 註解、跳過空白行），讀到 .e 或 EOF 回傳 false
static bool next_line(std::ifstream &in, std::istringstream &ss)
{
    std::string line;
    while(getline(in, line)){
        line = line.substr(0, line.find('#'));
        if(line.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;
        ss.clear();
        ss.str(line);
        std::string first;
        ss >> first;
        if(first == ".e")
            return false;
        ss.clear();
        ss.str(line);
        return true;
    }
    return false;
}

bool read_input(const std::string &filename, InputData &data)
{
    std::ifstream in(filename);
    if(!in.is_open()){
        std::cerr << "No such input file\n";
        return false;
    }
    std::string line;
    std::istringstream ss;
    while(getline(in, line)){
        std::istringstream hs(line.substr(0, line.find('#')));
        std::string section;
        hs >> section;

        if(section == ".limit"){
            while(next_line(in, ss)){
                std::string key;
                ss >> key;
                if(key == "fanout")      ss >> data.src_fanout;
                else if(key == "length") ss >> data.src_length;
                else if(key == ".dimx")  ss >> data.dim_x;
                else if(key == ".dimy")  ss >> data.dim_y;
            }
        }
        else if(section == ".buflib"){
            while(next_line(in, ss)){
                Buffer b;
                ss >> b.type >> b.fanout >> b.length >> b.cost;
                data.buflib.push_back(b);
            }
        }
        else if(section == ".objective"){
            while(next_line(in, ss)){
                std::string key;
                ss >> key;
                if(key == "w_skew") ss >> data.w_skew;
            }
        }
        else if(section == ".pin"){
            // 第一個是 SRC，其餘為 sinks S1..Sn
            bool first = true;
            while(next_line(in, ss)){
                Pin p;
                ss >> p.x >> p.y;
                if(first){
                    data.src = p;
                    first = false;
                }
                else
                    data.sinks.push_back(p);
            }
        }
    }
    in.close();
    return true;
}

void print_input(const InputData &data)
{
    std::cout << "SRC limit: fanout " << data.src_fanout << ", length " << data.src_length << '\n';
    std::cout << "dim: " << data.dim_x << " x " << data.dim_y << '\n';
    std::cout << "buflib:\n";
    for(const Buffer &b : data.buflib)
        std::cout << "  " << b.type << " F=" << b.fanout << " L=" << b.length << " C=" << b.cost << '\n';
    std::cout << "w_skew: " << data.w_skew << '\n';
    std::cout << "SRC: (" << data.src.x << ", " << data.src.y << ")\n";
    for(size_t i = 0 ; i < data.sinks.size() ; i++)
        std::cout << "  S" << i+1 << ": (" << data.sinks[i].x << ", " << data.sinks[i].y << ")\n";
}

typedef std::set<std::pair<int,int>> Occupied;

static int sum_dist(const Pin &p, const std::vector<int> &group, const Tree &tree)
{
    int s = 0;
    for(int v : group)
        s += calculate_distance(p, tree.nodes[v].pos);
    return s;
}

// x、y 各取中位數，Manhattan 距離總和最小
static Pin median_pos(const std::vector<int> &group, const Tree &tree)
{
    std::vector<int> xs, ys;
    for(int v : group){
        xs.push_back(tree.nodes[v].pos.x);
        ys.push_back(tree.nodes[v].pos.y);
    }
    std::sort(xs.begin(), xs.end());
    std::sort(ys.begin(), ys.end());
    int m = (group.size() - 1) / 2;
    return {xs[m], ys[m]};
}

// 能帶得動這群的最便宜 buffer type，都不行回傳 -1
static int cheapest_type(const std::vector<int> &group, const Tree &tree, const InputData &data)
{
    int sd = sum_dist(median_pos(group, tree), group, tree);
    int best = -1;
    for(size_t i = 0 ; i < data.buflib.size() ; i++){
        const Buffer &b = data.buflib[i];
        if((int)group.size() <= b.fanout && sd <= b.length)
            if(best == -1 || b.cost < data.buflib[best].cost)
                best = i;
    }
    return best;
}

// 從中位數出發，在 length 預算內盡量往 SRC 移動，並避開已被佔用的座標
static bool place_buffer(const std::vector<int> &group, int limit, const InputData &data,
                         const Tree &tree, const Occupied &occupied, Pin &out)
{
    Pin m = median_pos(group, tree);
    if(sum_dist(m, group, tree) > limit)
        return false;

    bool found = false;
    Pin cur = m;
    if(!occupied.count({cur.x, cur.y})){
        out = cur;
        found = true;
    }
    const Pin &target = data.src;
    while(true){
        int dx = (target.x > cur.x) - (target.x < cur.x);
        int dy = (target.y > cur.y) - (target.y < cur.y);
        Pin cand[2] = {{cur.x + dx, cur.y}, {cur.x, cur.y + dy}};
        bool moved = false;
        int best_sum = INT_MAX;
        Pin next = cur;
        for(int k = 0 ; k < 2 ; k++){
            if((k == 0 && dx == 0) || (k == 1 && dy == 0))
                continue;
            int s = sum_dist(cand[k], group, tree);
            if(s <= limit && s < best_sum){
                best_sum = s;
                next = cand[k];
                moved = true;
            }
        }
        if(!moved)
            break;
        cur = next;
        if(!occupied.count({cur.x, cur.y})){
            out = cur;
            found = true;
        }
    }
    if(found)
        return true;

    // 整條路都被佔用：在中位數附近找空位
    for(int r = 1 ; r <= 20 ; r++){
        for(int dx = -r ; dx <= r ; dx++){
            int rest = r - abs(dx);
            for(int dy : {-rest, rest}){
                Pin p = {m.x + dx, m.y + dy};
                if(p.x < 0 || p.y < 0 || p.x > data.dim_x || p.y > data.dim_y)
                    continue;
                if(occupied.count({p.x, p.y}))
                    continue;
                if(sum_dist(p, group, tree) <= limit){
                    out = p;
                    return true;
                }
            }
        }
    }
    return false;
}

// buffer 放在 p 時，它底下 sinks 的 delay 範圍 [lo, hi]
static void delay_range(const Pin &p, const std::vector<int> &group, const Tree &tree, int &lo, int &hi)
{
    lo = INT_MAX;
    hi = INT_MIN;
    for(int v : group){
        int d = calculate_distance(p, tree.nodes[v].pos);
        lo = std::min(lo, d + tree.nodes[v].dmin);
        hi = std::max(hi, d + tree.nodes[v].dmax);
    }
}

// 在 children 的 bounding box 裡找讓子樹 skew 最小的位置（同分時選離 SRC 近的）。
// box 太大時先用粗格子掃，再在最佳點附近細掃
static bool place_balanced(const std::vector<int> &group, int limit, const InputData &data,
                           const Tree &tree, const Occupied &occupied, Pin &out, int &skew)
{
    int x0 = INT_MAX, x1 = INT_MIN, y0 = INT_MAX, y1 = INT_MIN;
    for(int v : group){
        x0 = std::min(x0, tree.nodes[v].pos.x);
        x1 = std::max(x1, tree.nodes[v].pos.x);
        y0 = std::min(y0, tree.nodes[v].pos.y);
        y1 = std::max(y1, tree.nodes[v].pos.y);
    }
    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);
    x1 = std::min(x1, data.dim_x);
    y1 = std::min(y1, data.dim_y);

    bool found = false;
    int best_skew = INT_MAX, best_src = INT_MAX;
    auto try_pos = [&](int x, int y){
        if(x < x0 || x > x1 || y < y0 || y > y1)
            return;
        if(occupied.count({x, y}))
            return;
        Pin p = {x, y};
        if(sum_dist(p, group, tree) > limit)
            return;
        int lo, hi;
        delay_range(p, group, tree, lo, hi);
        int s = hi - lo, ds = calculate_distance(p, data.src);
        if(s < best_skew || (s == best_skew && ds < best_src)){
            best_skew = s;
            best_src = ds;
            out = p;
            found = true;
        }
    };

    Pin m = median_pos(group, tree);
    try_pos(m.x, m.y);
    long long area = (long long)(x1 - x0 + 1) * (y1 - y0 + 1);
    int step = std::max(1, (int)std::ceil(std::sqrt(area / 2500.0)));
    for(int x = x0 ; x <= x1 ; x += step)
        for(int y = y0 ; y <= y1 ; y += step)
            try_pos(x, y);
    if(found && step > 1){
        Pin c = out;
        for(int x = c.x - step ; x <= c.x + step ; x++)
            for(int y = c.y - step ; y <= c.y + step ; y++)
                try_pos(x, y);
    }

    if(!found){
        // box 內沒有合法點：退回原本的放法
        if(!place_buffer(group, limit, data, tree, occupied, out))
            return false;
        int lo, hi;
        delay_range(out, group, tree, lo, hi);
        best_skew = hi - lo;
    }
    skew = best_skew;
    return true;
}

static int add_buffer(Tree &tree, int type, const std::vector<int> &group, const Pin &pos, Occupied &occupied)
{
    Node n;
    n.pos = pos;
    n.type = type;
    n.children = group;
    delay_range(pos, group, tree, n.dmin, n.dmax);
    tree.nodes.push_back(n);
    occupied.insert({pos.x, pos.y});
    return tree.nodes.size() - 1;
}

static bool src_can_drive(const std::vector<int> &active, const InputData &data, const Tree &tree)
{
    return (int)active.size() <= data.src_fanout &&
           sum_dist(data.src, active, tree) <= data.src_length;
}

enum BuildMode{
    NAIVE,          // 最便宜的 type，放在中位數再往 SRC 推
    BALANCED_CHEAP, // 最便宜的 type，放在讓子樹 skew 最小的位置
    BALANCED_TYPE   // 每種 type 都試，選 cost + w_skew * 子樹 skew 最小的
};

// 由下往上建樹：每一層把節點分群，每群放一個 buffer，直到 SRC 能直接帶剩下的所有節點
bool build_tree(const InputData &data, Tree &tree, BuildMode mode)
{
    tree.nodes.clear();
    tree.num_sinks = data.sinks.size();
    Occupied occupied;
    occupied.insert({data.src.x, data.src.y});
    std::vector<int> active;
    for(size_t i = 0 ; i < data.sinks.size() ; i++){
        Node n;
        n.pos = data.sinks[i];
        n.type = -1;
        tree.nodes.push_back(n);
        occupied.insert({n.pos.x, n.pos.y});
        active.push_back(i);
    }
    if(data.buflib.empty()){
        if(!src_can_drive(active, data, tree))
            return false;
        tree.src_children = active;
        return true;
    }

    int max_fanout = 0, long_type = 0;
    for(size_t i = 0 ; i < data.buflib.size() ; i++){
        max_fanout = std::max(max_fanout, data.buflib[i].fanout);
        if(data.buflib[i].length > data.buflib[long_type].length)
            long_type = i;
    }

    auto dist_src = [&](int v){ return calculate_distance(data.src, tree.nodes[v].pos); };

    for(int iter = 0 ; iter < 10000 ; iter++){
        if(src_can_drive(active, data, tree)){
            tree.src_children = active;
            return true;
        }

        std::vector<int> remaining = active, next;
        bool merged = false;
        while(!remaining.empty()){
            // 從離 SRC 最遠的點開始，把離它最近的點一個一個加進來
            auto far = std::max_element(remaining.begin(), remaining.end(),
                                        [&](int a, int b){ return dist_src(a) < dist_src(b); });
            int seed = *far;
            remaining.erase(far);
            const Pin sp = tree.nodes[seed].pos;
            std::sort(remaining.begin(), remaining.end(), [&](int a, int b){
                return calculate_distance(sp, tree.nodes[a].pos) < calculate_distance(sp, tree.nodes[b].pos);
            });

            std::vector<int> group = {seed};
            for(int v : remaining){
                if((int)group.size() >= max_fanout)
                    break;
                group.push_back(v);
                if(cheapest_type(group, tree, data) < 0)
                    group.pop_back();
            }
            for(size_t k = 1 ; k < group.size() ; k++)
                remaining.erase(std::find(remaining.begin(), remaining.end(), group[k]));

            // 只有一個點就不放 buffer，直接留到上一層
            int t = -1;
            Pin pos;
            if(group.size() > 1 && mode == BALANCED_TYPE){
                int sd = sum_dist(median_pos(group, tree), group, tree);
                long long best_val = LLONG_MAX;
                for(size_t i = 0 ; i < data.buflib.size() ; i++){
                    const Buffer &b = data.buflib[i];
                    Pin p;
                    int skew;
                    if((int)group.size() > b.fanout || sd > b.length)
                        continue;
                    if(!place_balanced(group, b.length, data, tree, occupied, p, skew))
                        continue;
                    long long val = b.cost + (long long)data.w_skew * skew;
                    if(val < best_val){
                        best_val = val;
                        t = i;
                        pos = p;
                    }
                }
            }
            else if(group.size() > 1){
                t = cheapest_type(group, tree, data);
                int skew;
                bool placed = t >= 0 && (mode == NAIVE
                    ? place_buffer(group, data.buflib[t].length, data, tree, occupied, pos)
                    : place_balanced(group, data.buflib[t].length, data, tree, occupied, pos, skew));
                if(!placed)
                    t = -1;
            }
            if(t >= 0){
                next.push_back(add_buffer(tree, t, group, pos, occupied));
                merged = true;
            }
            else
                next.insert(next.end(), group.begin(), group.end());
        }

        if(!merged){
            // 完全沒辦法合併：幫離 SRC 最遠的點加一個 repeater，把它往 SRC 拉近
            auto far = std::max_element(next.begin(), next.end(),
                                        [&](int a, int b){ return dist_src(a) < dist_src(b); });
            std::vector<int> group = {*far};
            Pin pos;
            if(place_buffer(group, data.buflib[long_type].length, data, tree, occupied, pos))
                *far = add_buffer(tree, long_type, group, pos, occupied);
        }
        active = next;
    }
    return false;
}

// BFS：依序編號 B1..Bm，並算出每個節點的深度與 arrival time
TreeInfo analyze_tree(const InputData &data, const Tree &tree)
{
    TreeInfo info;
    int n = tree.nodes.size();
    info.buf_id.assign(n, 0);
    info.depth.assign(n, -1);
    info.arrival.assign(n, 0);
    std::queue<int> q;
    for(int v : tree.src_children){
        info.depth[v] = 1;
        info.arrival[v] = calculate_distance(data.src, tree.nodes[v].pos);
        q.push(v);
    }
    while(!q.empty()){
        int u = q.front();
        q.pop();
        info.max_level = std::max(info.max_level, info.depth[u]);
        if(tree.nodes[u].type < 0)
            continue;
        info.buf_order.push_back(u);
        info.buf_id[u] = info.buf_order.size();
        for(int v : tree.nodes[u].children){
            info.depth[v] = info.depth[u] + 1;
            info.arrival[v] = info.arrival[u] + calculate_distance(tree.nodes[u].pos, tree.nodes[v].pos);
            q.push(v);
        }
    }
    return info;
}

static std::string node_name(int v, const Tree &tree, const TreeInfo &info)
{
    if(tree.nodes[v].type < 0)
        return "S" + std::to_string(v + 1);
    return "B" + std::to_string(info.buf_id[v]);
}

// 先列 buffers（依編號）再列 sinks
static std::string children_str(std::vector<int> children, const Tree &tree, const TreeInfo &info)
{
    std::sort(children.begin(), children.end(), [&](int a, int b){
        bool ba = tree.nodes[a].type >= 0, bb = tree.nodes[b].type >= 0;
        if(ba != bb)
            return ba;
        return ba ? info.buf_id[a] < info.buf_id[b] : a < b;
    });
    std::string s = "{";
    for(size_t i = 0 ; i < children.size() ; i++){
        if(i)
            s += " ";
        s += node_name(children[i], tree, info);
    }
    return s + "}";
}

bool write_output(const std::string &filename, const InputData &data, const Tree &tree, const TreeInfo &info)
{
    std::ofstream out(filename);
    if(!out.is_open()){
        std::cerr << "Cannot open output file\n";
        return false;
    }
    out << ".buffer " << info.buf_order.size() << '\n';
    for(int u : info.buf_order)
        out << "B" << info.buf_id[u] << " " << data.buflib[tree.nodes[u].type].type << " "
            << tree.nodes[u].pos.x << " " << tree.nodes[u].pos.y << '\n';
    out << ".e\n\n";

    out << ".level " << info.max_level << '\n';
    out << "1 SRC:" << children_str(tree.src_children, tree, info) << '\n';
    for(int k = 2 ; k <= info.max_level ; k++){
        out << k;
        for(int u : info.buf_order)
            if(info.depth[u] == k - 1)
                out << " " << node_name(u, tree, info) << ":" << children_str(tree.nodes[u].children, tree, info);
        out << '\n';
    }
    out << ".e\n";
    out.close();
    return true;
}

// 輸出 gnuplot 檔（格式同 Example/*.plt）：SRC、sinks，再依 buflib 順序每種 buffer 一組，
// buffer 越大的 type 點越大
bool write_plot(const std::string &filename, const InputData &data, const Tree &tree, const TreeInfo &info)
{
    std::ofstream out(filename);
    if(!out.is_open()){
        std::cerr << "Cannot open plot file\n";
        return false;
    }
    out << "set xrange [0:" << data.dim_x << "]\n";
    out << "set yrange [0:" << data.dim_y << "]\n\n";

    // 點太多時標籤會糊成一片，只在小測資加
    if(tree.nodes.size() <= 200){
        auto label = [&](const std::string &name, const Pin &p){
            out << "set label \"" << name << "\" at " << p.x << "," << p.y
                << " offset 0,-1.2 center tc rgb \"red\"\n";
        };
        label("SRC", data.src);
        for(int i = 0 ; i < tree.num_sinks ; i++)
            label("S" + std::to_string(i + 1), tree.nodes[i].pos);
        for(int u : info.buf_order)
            label(node_name(u, tree, info), tree.nodes[u].pos);
        out << '\n';
    }

    out << "plot '-' with points pt 9 ps 1.5 notitle, \\\n";
    out << "     '-' with points pt 7 ps 1.5 notitle";
    for(size_t t = 0 ; t < data.buflib.size() ; t++){
        out << ", \\\n";
        out << "     '-' with points pt 5 ps " << std::fixed << std::setprecision(1) << 1.5 + 0.5 * t << " notitle";
    }
    out << "\n\n";

    out << data.src.x << " " << data.src.y << " # SRC\ne\n";
    for(int i = 0 ; i < tree.num_sinks ; i++)
        out << tree.nodes[i].pos.x << " " << tree.nodes[i].pos.y << " # T" << i + 1 << '\n';
    out << "e\n";
    for(size_t t = 0 ; t < data.buflib.size() ; t++){
        for(int u : info.buf_order)
            if(tree.nodes[u].type == (int)t)
                out << tree.nodes[u].pos.x << " " << tree.nodes[u].pos.y << " # B" << info.buf_id[u] << '\n';
        out << "e\n";
    }
    out.close();
    return true;
}

// 檢查 fanout / length 限制、座標範圍與重複，有問題印到 stderr
bool check_legal(const InputData &data, const Tree &tree, const TreeInfo &info)
{
    bool ok = true;
    auto check_parent = [&](const std::string &name, const Pin &p, const std::vector<int> &ch, int F, int L){
        int len = 0;
        for(int v : ch)
            len += calculate_distance(p, tree.nodes[v].pos);
        if((int)ch.size() > F || len > L){
            std::cerr << "Illegal " << name << ": fanout " << ch.size() << "/" << F
                      << ", length " << len << "/" << L << '\n';
            ok = false;
        }
    };
    check_parent("SRC", data.src, tree.src_children, data.src_fanout, data.src_length);

    Occupied seen;
    seen.insert({data.src.x, data.src.y});
    for(size_t v = 0 ; v < tree.nodes.size() ; v++){
        const Node &n = tree.nodes[v];
        std::string name = node_name(v, tree, info);
        if(info.depth[v] < 0){
            std::cerr << name << " is not reachable from SRC\n";
            ok = false;
        }
        if(!seen.insert({n.pos.x, n.pos.y}).second){
            std::cerr << name << " overlaps another component\n";
            ok = false;
        }
        if(n.type < 0)
            continue;
        if(n.pos.x < 0 || n.pos.y < 0 || n.pos.x > data.dim_x || n.pos.y > data.dim_y){
            std::cerr << name << " is outside the chip\n";
            ok = false;
        }
        const Buffer &b = data.buflib[n.type];
        check_parent(name, n.pos, n.children, b.fanout, b.length);
    }
    return ok;
}

typedef struct result{
    int t_max = 0;
    int t_min = 0;
    int cost = 0;
    long long score = 0;
}Result;

Result evaluate(const InputData &data, const Tree &tree, const TreeInfo &info)
{
    Result r;
    r.t_max = INT_MIN;
    r.t_min = INT_MAX;
    for(int i = 0 ; i < tree.num_sinks ; i++){
        r.t_max = std::max(r.t_max, info.arrival[i]);
        r.t_min = std::min(r.t_min, info.arrival[i]);
    }
    for(int u : info.buf_order)
        r.cost += data.buflib[tree.nodes[u].type].cost;
    r.score = r.cost + (long long)data.w_skew * (r.t_max - r.t_min);
    return r;
}

int main(int argc, char* argv[])
{
    std::string input_file,output_file;
    if(argc >= 3){
        input_file = argv[1];
        output_file = argv[2];
    }
    else{
        std::cerr << "Usage: " << argv[0] << " INPUT_FILE OUTPUT_FILE (using default files)\n";
        input_file = "/home/lin/CAD/project_2/Example/input.cbi";
        output_file = "/home/lin/CAD/project_2/output.cbi";
    }

    InputData data;
    if(!read_input(input_file, data))
        return 1;

    // 每種建法都試，留 Score 最低的
    Tree tree;
    TreeInfo info;
    Result res;
    bool ok = false;
    const char *mode_name[] = {"naive", "balanced_cheap", "balanced_type"};
    for(BuildMode mode : {NAIVE, BALANCED_CHEAP, BALANCED_TYPE}){
        Tree t;
        if(!build_tree(data, t, mode))
            continue;
        TreeInfo ti = analyze_tree(data, t);
        Result r = evaluate(data, t, ti);
        std::cerr << mode_name[mode] << ": cost " << r.cost
                  << ", skew " << r.t_max - r.t_min << ", Score " << r.score << '\n';
        if(!ok || r.score < res.score){
            tree = t;
            info = ti;
            res = r;
            ok = true;
        }
    }
    if(!ok){
        std::cerr << "Failed to build a legal clock tree\n";
        return 1;
    }
    check_legal(data, tree, info);
    if(!write_output(output_file, data, tree, info))
        return 1;

    // output.cbi -> output.plt
    std::string plot_file = output_file;
    if(plot_file.size() >= 4 && plot_file.compare(plot_file.size() - 4, 4, ".cbi") == 0)
        plot_file.replace(plot_file.size() - 4, 4, ".plt");
    else
        plot_file += ".plt";
    write_plot(plot_file, data, tree, info);

    std::cout << "T_max: " << res.t_max << ", T_min: " << res.t_min << ", T_skew: " << (res.t_max - res.t_min) << ", Score: " << res.score << '\n';

    return 0;
}
