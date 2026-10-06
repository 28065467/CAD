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
#include <random>
#include <chrono>
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

enum Grouping{
    GREEDY,         // 從離 SRC 最遠的點開始，貪婪地抓最近的鄰居
    KMEANS          // 有容量限制的 k-means
};

enum Placement{
    NAIVE,          // 最便宜的 type，放在中位數再往 SRC 推
    BALANCED_CHEAP, // 最便宜的 type，放在讓子樹 skew 最小的位置
    BALANCED_TYPE   // 每種 type 都試，選 cost + w_skew * 子樹 skew 最小的
};

// 貪婪分群：從離 SRC 最遠的點開始，把離它最近的點一個一個加進來，直到 buffer 帶不動
static std::vector<std::vector<int>> group_greedy(std::vector<int> remaining, const InputData &data,
                                                  const Tree &tree, int max_fanout)
{
    auto dist_src = [&](int v){ return calculate_distance(data.src, tree.nodes[v].pos); };
    std::vector<std::vector<int>> groups;
    while(!remaining.empty()){
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
        groups.push_back(group);
    }
    return groups;
}

// 有容量限制的 k-means：每群最多 max_fanout 個點，中心用中位數（Manhattan 距離下的最佳中心）。
// k 用 greedy 分群的平均群大小來估，因為 length 限制通常讓一群塞不滿 max_fanout。
// 分完後帶不動的群，把最遠的點踢出去，再試著塞進附近還有空間的群
static std::vector<std::vector<int>> group_kmeans(const std::vector<int> &active, const InputData &data,
                                                  const Tree &tree, int max_fanout)
{
    int n = active.size();
    int pts = 0, cnt = 0;
    for(const std::vector<int> &g : group_greedy(active, data, tree, max_fanout))
        if(g.size() > 1){
            pts += g.size();
            cnt++;
        }
    double avg = cnt ? (double)pts / cnt : max_fanout;
    int k = std::max((n + max_fanout - 1) / max_fanout, (int)std::ceil(n / avg));
    k = std::min(k, n);
    auto pos = [&](int i){ return tree.nodes[active[i]].pos; };

    // 初始中心：farthest-first，從離 SRC 最遠的點開始
    std::vector<Pin> centers;
    std::vector<int> near_d(n, INT_MAX);
    int first = 0;
    for(int i = 1 ; i < n ; i++)
        if(calculate_distance(data.src, pos(i)) > calculate_distance(data.src, pos(first)))
            first = i;
    centers.push_back(pos(first));
    while((int)centers.size() < k){
        int far = 0;
        for(int i = 0 ; i < n ; i++){
            near_d[i] = std::min(near_d[i], calculate_distance(pos(i), centers.back()));
            if(near_d[i] > near_d[far])
                far = i;
        }
        centers.push_back(pos(far));
    }

    std::vector<int> assign(n, -1);
    const int CAND = 16;        // 每個點只看最近的幾個中心，太多會很慢
    for(int iter = 0 ; iter < 20 ; iter++){
        // 每個點的候選中心（由近到遠）
        std::vector<std::vector<int>> cand(n);
        std::vector<int> order(n), regret(n);
        for(int i = 0 ; i < n ; i++){
            std::vector<int> ids(k);
            for(int c = 0 ; c < k ; c++)
                ids[c] = c;
            int m = std::min(k, CAND);
            auto by_dist = [&](int a, int b){
                return calculate_distance(pos(i), centers[a]) < calculate_distance(pos(i), centers[b]);
            };
            std::partial_sort(ids.begin(), ids.begin() + m, ids.end(), by_dist);
            ids.resize(m);
            cand[i] = ids;
            // regret：第一和第二近的差距越大，越該先分，免得被搶走
            regret[i] = m > 1 ? calculate_distance(pos(i), centers[ids[1]]) - calculate_distance(pos(i), centers[ids[0]]) : 0;
            order[i] = i;
        }
        std::sort(order.begin(), order.end(), [&](int a, int b){ return regret[a] > regret[b]; });

        std::vector<int> size(k, 0), new_assign(n, -1);
        for(int i : order){
            for(int c : cand[i])
                if(size[c] < max_fanout){
                    new_assign[i] = c;
                    break;
                }
            if(new_assign[i] < 0){
                // 候選都滿了：找最近的還有空位的中心（總容量 k*F >= n，一定找得到）
                int best = -1;
                for(int c = 0 ; c < k ; c++)
                    if(size[c] < max_fanout &&
                       (best < 0 || calculate_distance(pos(i), centers[c]) < calculate_distance(pos(i), centers[best])))
                        best = c;
                new_assign[i] = best;
            }
            size[new_assign[i]]++;
        }

        bool changed = new_assign != assign;
        assign = new_assign;
        if(!changed)
            break;

        // 更新中心為群內的中位數
        std::vector<std::vector<int>> members(k);
        for(int i = 0 ; i < n ; i++)
            members[assign[i]].push_back(active[i]);
        for(int c = 0 ; c < k ; c++)
            if(!members[c].empty())
                centers[c] = median_pos(members[c], tree);
    }

    std::vector<std::vector<int>> groups(k), result;
    std::vector<int> kicked;
    for(int i = 0 ; i < n ; i++)
        groups[assign[i]].push_back(active[i]);
    for(std::vector<int> &g : groups){
        // length 超過：把離中位數最遠的點踢出去
        while(g.size() > 1 && cheapest_type(g, tree, data) < 0){
            Pin m = median_pos(g, tree);
            auto far = std::max_element(g.begin(), g.end(), [&](int a, int b){
                return calculate_distance(m, tree.nodes[a].pos) < calculate_distance(m, tree.nodes[b].pos);
            });
            kicked.push_back(*far);
            g.erase(far);
        }
    }

    // 被踢出去的點：由近到遠試其他群，加進去還帶得動就放，都不行才自成一群
    for(int v : kicked){
        const Pin p = tree.nodes[v].pos;
        std::vector<std::pair<int,int>> near;     // (到群中位數的距離, 群 index)
        for(int c = 0 ; c < k ; c++)
            if(!groups[c].empty() && (int)groups[c].size() < max_fanout)
                near.push_back({calculate_distance(p, median_pos(groups[c], tree)), c});
        std::sort(near.begin(), near.end());
        bool placed = false;
        for(size_t j = 0 ; j < near.size() && j < 16 && !placed ; j++){
            std::vector<int> &g = groups[near[j].second];
            g.push_back(v);
            if(cheapest_type(g, tree, data) >= 0)
                placed = true;
            else
                g.pop_back();
        }
        if(!placed)
            groups.push_back({v});
    }

    for(std::vector<int> &g : groups)
        if(!g.empty())
            result.push_back(g);
    return result;
}

double g_t0_ratio = 0.00002;    // 初始溫度 = max(2, 這個比例 * 初始 Score)，可用 --t0 調整
bool g_stats = false;          // 執行時加 --stats 才印分群統計

// 印出第一層分群的統計：每群 pin 數的分布、Σd（中位數到群內各點的距離和）的平均與最大值、單點群數量
static void print_group_stats(const char *name, const std::vector<std::vector<int>> &groups, const Tree &tree)
{
    std::vector<int> hist;
    long long sd_total = 0;
    int sd_max = 0, multi = 0, single = 0;
    for(const std::vector<int> &g : groups){
        if(g.size() >= hist.size())
            hist.resize(g.size() + 1, 0);
        hist[g.size()]++;
        if(g.size() == 1){
            single++;
            continue;
        }
        int sd = sum_dist(median_pos(g, tree), g, tree);
        sd_total += sd;
        sd_max = std::max(sd_max, sd);
        multi++;
    }
    std::cerr << "[" << name << "] groups " << groups.size() << ", size distribution:";
    for(size_t s = 1 ; s < hist.size() ; s++)
        if(hist[s])
            std::cerr << " " << s << "x" << hist[s];
    std::cerr << "\n  sum_d (groups with >1 pin): avg " << std::fixed << std::setprecision(1)
              << (multi ? (double)sd_total / multi : 0.0) << ", max " << sd_max
              << "\n  single-pin groups: " << single << '\n';
}

// 由下往上建樹：每一層把節點分群，每群放一個 buffer，直到 SRC 能直接帶剩下的所有節點
bool build_tree(const InputData &data, Tree &tree, Grouping grouping, Placement placement)
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

        std::vector<std::vector<int>> groups = grouping == KMEANS
            ? group_kmeans(active, data, tree, max_fanout)
            : group_greedy(active, data, tree, max_fanout);
        // 分群不受 placement 影響，只在 NAIVE 那次印，避免重複
        if(g_stats && iter == 0 && placement == NAIVE)
            print_group_stats(grouping == KMEANS ? "kmeans" : "greedy", groups, tree);

        std::vector<int> next;
        bool merged = false;
        for(const std::vector<int> &group : groups){
            // 只有一個點就不放 buffer，直接留到上一層
            int t = -1;
            Pin pos;
            if(group.size() > 1 && placement == BALANCED_TYPE){
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
                bool placed = t >= 0 && (placement == NAIVE
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

// ---------------- Local search（simulated annealing） ----------------
// 在建好的合法樹上反覆做小修改，每次只檢查受影響節點的 F/L，不合法或沒被接受就還原。
// 修改種類：移動 buffer、換 type、換 parent（sink 或整棵子樹）、刪掉只有 1 個 child 的 buffer
class LocalSearch{
public:
    LocalSearch(const InputData &d, const Tree &tree, unsigned seed) : data(d), rng(seed)
    {
        num_sinks = tree.num_sinks;
        int n = tree.nodes.size();
        src = n;                                // SRC 放在最後一個 index
        pos.resize(n + 1);
        type.resize(n + 1);
        parent.assign(n + 1, -1);
        ch.resize(n + 1);
        dead.assign(n + 1, 0);
        saved.assign(n + 1, 0);
        for(int v = 0 ; v < n ; v++){
            pos[v] = tree.nodes[v].pos;
            type[v] = tree.nodes[v].type;
            ch[v] = tree.nodes[v].children;
            occ.insert(key(pos[v]));
            if(type[v] >= 0)
                buffers.push_back(v);
        }
        pos[src] = data.src;
        type[src] = -2;
        ch[src] = tree.src_children;
        occ.insert(key(pos[src]));
        for(int u = 0 ; u <= n ; u++)
            for(int v : ch[u])
                parent[v] = u;
    }

    // 跑到時間用完（max_iters > 0 時改成跑固定次數，結果可重現），回傳看過的最佳解
    Tree run(double seconds, long long max_iters)
    {
        auto start = std::chrono::steady_clock::now();
        long long cur = eval();
        long long best = cur;
        Snapshot best_state = snapshot();
        double t0 = std::max(2.0, g_t0_ratio * cur), t_end = 0.01, temp = t0;
        int rmax = std::max(2, std::max(data.dim_x, data.dim_y) / 10);
        std::uniform_real_distribution<double> uni(0.0, 1.0);

        long long iter = 0, accepted = 0;
        while(true){
            if((iter & 255) == 0){
                double progress;            // 0 → 1，用來指數降溫
                if(max_iters > 0)
                    progress = (double)iter / max_iters;
                else
                    progress = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() / seconds;
                if(progress >= 1.0)
                    break;
                temp = t0 * std::pow(t_end / t0, progress);
            }
            iter++;

            std::vector<int> affected;
            bool done = false;
            int r = std::max(1, (int)(rmax * temp / t0));
            double pick = uni(rng);
            if(pick < 0.35)      done = move_buffer(r, affected);
            else if(pick < 0.45) done = change_type(affected);
            else if(pick < 0.90) done = reassign(affected);
            else                 done = splice(affected);
            if(!done){
                rollback();
                continue;
            }
            bool legal = true;
            for(int u : affected)
                if(!dead[u] && !legal_node(u)){
                    legal = false;
                    break;
                }
            if(!legal){
                rollback();
                continue;
            }
            long long nxt = eval();
            long long delta = nxt - cur;
            if(delta <= 0 || uni(rng) < std::exp(-delta / temp)){
                commit();
                cur = nxt;
                accepted++;
                if(cur < best){
                    best = cur;
                    best_state = snapshot();
                }
            }
            else
                rollback();
        }
        std::cerr << "local search: " << iter << " iterations, " << accepted << " accepted\n";
        restore(best_state);
        return to_tree();
    }

private:
    typedef struct saved_node{
        int v;
        Pin pos;
        int type;
        int parent;
        std::vector<int> ch;
        char dead;
    }SavedNode;

    typedef struct snapshot_t{
        std::vector<Pin> pos;
        std::vector<int> type, parent;
        std::vector<std::vector<int>> ch;
        std::vector<char> dead;
    }Snapshot;

    const InputData &data;
    std::mt19937 rng;
    int num_sinks, src;
    std::vector<Pin> pos;
    std::vector<int> type, parent;      // type: -1 sink, -2 SRC, >=0 buffer
    std::vector<std::vector<int>> ch;
    std::vector<char> dead, saved;
    std::vector<int> buffers;           // 所有 buffer 的 index（包含已刪除的）
    Occupied occ;
    std::vector<SavedNode> undo;
    int tmin_sink = 0, tmax_sink = 0;

    static std::pair<int,int> key(const Pin &p){ return {p.x, p.y}; }

    int rand_int(int lo, int hi){ return std::uniform_int_distribution<int>(lo, hi)(rng); }

    int fanout_limit(int u) const { return u == src ? data.src_fanout : data.buflib[type[u]].fanout; }
    int length_limit(int u) const { return u == src ? data.src_length : data.buflib[type[u]].length; }

    bool legal_node(int u) const
    {
        if(type[u] == -1)
            return true;
        if(u != src && ch[u].empty())
            return false;
        if((int)ch[u].size() > fanout_limit(u))
            return false;
        int len = 0;
        for(int v : ch[u])
            len += calculate_distance(pos[u], pos[v]);
        return len <= length_limit(u);
    }

    // 修改前先存下節點原本的狀態，一次修改裡每個節點只存一次
    void save(int v)
    {
        if(saved[v])
            return;
        saved[v] = 1;
        undo.push_back({v, pos[v], type[v], parent[v], ch[v], dead[v]});
    }

    void commit()
    {
        for(const SavedNode &s : undo)
            saved[s.v] = 0;
        undo.clear();
    }

    void rollback()
    {
        for(const SavedNode &s : undo)
            if(!dead[s.v])
                occ.erase(key(pos[s.v]));
        for(const SavedNode &s : undo){
            pos[s.v] = s.pos;
            type[s.v] = s.type;
            parent[s.v] = s.parent;
            ch[s.v] = s.ch;
            dead[s.v] = s.dead;
        }
        for(const SavedNode &s : undo)
            if(!dead[s.v])
                occ.insert(key(pos[s.v]));
        commit();
    }

    Snapshot snapshot() const { return {pos, type, parent, ch, dead}; }

    void restore(const Snapshot &s)
    {
        pos = s.pos;
        type = s.type;
        parent = s.parent;
        ch = s.ch;
        dead = s.dead;
    }

    // 算整棵樹的 Score，順便記下 arrival time 最小 / 最大的 sink
    long long eval()
    {
        int t_max = INT_MIN, t_min = INT_MAX;
        long long cost = 0;
        std::vector<std::pair<int,int>> stack = {{src, 0}};
        while(!stack.empty()){
            auto [u, t] = stack.back();
            stack.pop_back();
            if(type[u] == -1){
                if(t > t_max){ t_max = t; tmax_sink = u; }
                if(t < t_min){ t_min = t; tmin_sink = u; }
                continue;
            }
            if(u != src)
                cost += data.buflib[type[u]].cost;
            for(int v : ch[u])
                stack.push_back({v, t + calculate_distance(pos[u], pos[v])});
        }
        return cost + (long long)data.w_skew * (t_max - t_min);
    }

    int random_buffer()
    {
        for(int tries = 0 ; tries < 20 && !buffers.empty() ; tries++){
            int b = buffers[rand_int(0, buffers.size() - 1)];
            if(!dead[b])
                return b;
        }
        return -1;
    }

    // 把空掉的 buffer 刪掉，parent 也空掉的話一路往上刪
    void remove_if_empty(int u)
    {
        while(u != src && ch[u].empty()){
            int p = parent[u];
            save(u);
            save(p);
            dead[u] = 1;
            occ.erase(key(pos[u]));
            ch[p].erase(std::find(ch[p].begin(), ch[p].end(), u));
            parent[u] = -1;
            u = p;
        }
    }

    bool move_buffer(int r, std::vector<int> &affected)
    {
        int b = random_buffer();
        if(b < 0)
            return false;
        Pin p = {pos[b].x + rand_int(-r, r), pos[b].y + rand_int(-r, r)};
        if(p.x < 0 || p.y < 0 || p.x > data.dim_x || p.y > data.dim_y || occ.count(key(p)))
            return false;
        save(b);
        occ.erase(key(pos[b]));
        pos[b] = p;
        occ.insert(key(p));
        affected = {b, parent[b]};
        return true;
    }

    bool change_type(std::vector<int> &affected)
    {
        int b = random_buffer();
        if(b < 0 || data.buflib.size() < 2)
            return false;
        int t = rand_int(0, data.buflib.size() - 2);
        if(t >= type[b])
            t++;
        save(b);
        type[b] = t;
        affected = {b};
        return true;
    }

    // 把 v（sink 或 buffer 連同它的子樹）換到另一個 parent 底下
    bool reassign(std::vector<int> &affected)
    {
        int v;
        double pick = std::uniform_real_distribution<double>(0.0, 1.0)(rng);
        if(pick < 0.3)
            v = tmin_sink;
        else if(pick < 0.5)
            v = tmax_sink;
        else if(pick < 0.8)
            v = rand_int(0, num_sinks - 1);
        else
            v = random_buffer();
        if(v < 0)
            return false;

        // 隨機抽一些 parent 候選，從最近的 3 個裡挑一個
        std::vector<std::pair<int,int>> cand;
        for(int k = 0 ; k < 24 ; k++){
            int p = k == 0 ? src : random_buffer();
            if(p >= 0 && p != parent[v] && p != v)
                cand.push_back({calculate_distance(pos[v], pos[p]), p});
        }
        if(cand.empty())
            return false;
        std::sort(cand.begin(), cand.end());
        cand.erase(std::unique(cand.begin(), cand.end()), cand.end());
        int p = cand[rand_int(0, std::min<int>(3, cand.size()) - 1)].second;

        // p 不能在 v 的子樹裡，不然會變成 cycle
        for(int a = p ; a != -1 ; a = parent[a])
            if(a == v)
                return false;

        int old = parent[v];
        save(v);
        save(old);
        save(p);
        ch[old].erase(std::find(ch[old].begin(), ch[old].end(), v));
        ch[p].push_back(v);
        parent[v] = p;
        remove_if_empty(old);
        affected = {p};
        return true;
    }

    // 只有 1 個 child 的 buffer：刪掉，child 直接接到它的 parent
    bool splice(std::vector<int> &affected)
    {
        int b = random_buffer();
        if(b < 0 || ch[b].size() != 1)
            return false;
        int c = ch[b][0], p = parent[b];
        save(b);
        save(c);
        save(p);
        *std::find(ch[p].begin(), ch[p].end(), b) = c;
        parent[c] = p;
        ch[b].clear();
        parent[b] = -1;
        dead[b] = 1;
        occ.erase(key(pos[b]));
        affected = {p};
        return true;
    }

    // 轉回 Tree：刪掉的 buffer 拿掉，剩下的重新編 index
    Tree to_tree() const
    {
        Tree t;
        t.num_sinks = num_sinks;
        std::vector<int> remap(src, -1);
        for(int v = 0 ; v < src ; v++)
            if(!dead[v]){
                remap[v] = t.nodes.size();
                Node n;
                n.pos = pos[v];
                n.type = type[v];
                t.nodes.push_back(n);
            }
        for(int v = 0 ; v < src ; v++)
            if(!dead[v])
                for(int c : ch[v])
                    t.nodes[remap[v]].children.push_back(remap[c]);
        for(int c : ch[src])
            t.src_children.push_back(remap[c]);
        return t;
    }
};

int main(int argc, char* argv[])
{
    std::string input_file,output_file;
    double ls_time = 3.0;          // local search 秒數，--time 0 可關掉
    long long ls_iters = 0;        // > 0 時改用固定迭代次數，不看時間
    unsigned ls_seed = 12345;
    for(int i = 3 ; i < argc ; i++){
        std::string arg = argv[i];
        if(arg == "--stats")
            g_stats = true;
        else if(arg == "--time" && i + 1 < argc)
            ls_time = std::atof(argv[++i]);
        else if(arg == "--t0" && i + 1 < argc)
            g_t0_ratio = std::atof(argv[++i]);
        else if(arg == "--iters" && i + 1 < argc)
            ls_iters = std::atoll(argv[++i]);
        else if(arg == "--seed" && i + 1 < argc)
            ls_seed = std::strtoul(argv[++i], nullptr, 10);
    }
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
    const char *grouping_name[] = {"greedy", "kmeans"};
    const char *placement_name[] = {"naive", "balanced_cheap", "balanced_type"};
    for(Grouping g : {GREEDY, KMEANS})
    for(Placement p : {NAIVE, BALANCED_CHEAP, BALANCED_TYPE}){
        Tree t;
        if(!build_tree(data, t, g, p))
            continue;
        TreeInfo ti = analyze_tree(data, t);
        Result r = evaluate(data, t, ti);
        std::cerr << grouping_name[g] << " + " << placement_name[p] << ": cost " << r.cost
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

    if(ls_time > 0 || ls_iters > 0){
        LocalSearch ls(data, tree, ls_seed);
        tree = ls.run(ls_time, ls_iters);
        info = analyze_tree(data, tree);
        res = evaluate(data, tree, info);
        std::cerr << "after local search: cost " << res.cost << ", skew " << res.t_max - res.t_min
                  << ", Score " << res.score << '\n';
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
