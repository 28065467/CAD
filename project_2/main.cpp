#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
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

static void read_limit(std::ifstream &in, InputData &data)
{
    std::istringstream ss;
    while(next_line(in, ss)){
        std::string key;
        ss >> key;
        if(key == "fanout")     ss >> data.src_fanout;
        else if(key == "length") ss >> data.src_length;
        else if(key == ".dimx")  ss >> data.dim_x;
        else if(key == ".dimy")  ss >> data.dim_y;
    }
}

static void read_buflib(std::ifstream &in, InputData &data)
{
    std::istringstream ss;
    while(next_line(in, ss)){
        Buffer b;
        ss >> b.type >> b.fanout >> b.length >> b.cost;
        data.buflib.push_back(b);
    }
}

static void read_objective(std::ifstream &in, InputData &data)
{
    std::istringstream ss;
    while(next_line(in, ss)){
        std::string key;
        ss >> key;
        if(key == "w_skew") ss >> data.w_skew;
    }
}

static void read_pin(std::ifstream &in, InputData &data)
{
    std::istringstream ss;
    std::vector<Pin> pins;
    while(next_line(in, ss)){
        Pin p;
        ss >> p.x >> p.y;
        pins.push_back(p);
    }
    // 第一個是 SRC，其餘為 sinks S1..Sn
    if(!pins.empty()){
        data.src = pins[0];
        data.sinks.assign(pins.begin() + 1, pins.end());
    }
}

bool read_input(const std::string &filename, InputData &data)
{
    std::ifstream in(filename);
    if(!in.is_open()){
        std::cerr << "No such input file\n";
        return false;
    }
    std::string line;
    while(getline(in, line)){
        std::istringstream ss(line.substr(0, line.find('#')));
        std::string key;
        ss >> key;
        if(key == ".limit")          read_limit(in, data);
        else if(key == ".buflib")    read_buflib(in, data);
        else if(key == ".objective") read_objective(in, data);
        else if(key == ".pin")       read_pin(in, data);
    }
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

int main(int argc, char* argv[])
{
    std::string input_file,output_file;
    // for(int i = 1 ; i < argc ; i++){
    //     std::string arg = argv[i];
    //     if(arg == "-if" && i+1 < argc)
    //         input_file = argv[++i];
    //     else if (arg == "-of" && i+1 < argc)
    //         output_file = argv[++i];
    //     else{
    //         std::cerr << "Unknow argument" << arg << '\n';
    //         return 1;
    //     }
    // }
    input_file = "/home/lin/CAD/project_2/Example/input.cbi";

    InputData data;
    if(!read_input(input_file, data))
        return 1;
    print_input(data);

    return 0;
}
