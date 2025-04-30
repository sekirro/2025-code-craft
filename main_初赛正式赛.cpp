#include<bits/stdc++.h>
#include<cmath>

using namespace std;

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define MAX_TAG_NUM (16 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define MAX_TIME (86400)
#define MAX_TIME_SLICE (MAX_TIME / FRE_PER_SLICING + 2)
#define EXTRA_TIME (105)
#define mkp make_pair
#define ALLOCATE_SIZE (5)
#define GROUP_NUM (16)
// ----------------------------parameter--------------------------------

typedef struct read_Request {
    int request_id;
    int st_time;
    int object_id;
    bool is_done = false;
    bool is_abort = false;
    bool is_read = false;
} read_Request;

typedef struct Object {
    int id;
    int tag;
    int size;
    bool is_delete = false;
    int can_read_request_num = 0;
    int saved_disk[10];
    vector<int> saved_position[10];
    // list<read_Request*> object_read_requests;
} Object;

typedef struct Disk {
    int id;
    int size;
    int used_clean_area_size[MAX_TAG_NUM] = {0};
    int trash_area_size= 0;
    vector<pair<int,pair<int,int>>> divided_area;//初始分配的空间<tag,<left,right>>,这个area[0]是id为1的tag
    vector<pair<int,pair<int,int>>> allocated_area;//新分配的空间<tag,<left,right>>
    int saved_object_id[MAX_DISK_SIZE] = {0};
    int left_request_num[MAX_DISK_SIZE] = {0};
} Disk;

typedef struct ReadAction {//读取请求，包含请求id，对象id，是否完成
    int request_id;
    int object_id;
    bool is_done = false;
    bool is_abort = false;
} ReadAction;

typedef struct MoveAction {
    int op;//0.不动 1.pass 2.jump
    int gcost;
    int time;
} MoveAction;

typedef struct ReadHead {
    int disk_id;
    int position;
    int leftg;
    read_Request target_request;
    int target_position;
    bool last_read = false;
    int last_cost = 0;
    string action;
    // list<read_Request*> read_list;
    vector<read_Request> read_list[MAX_DISK_SIZE+5];
} ReadHead;

ReadHead read_head[MAX_DISK_NUM];
vector<Disk> disks;
read_Request read_requests[MAX_REQUEST_NUM];
vector<Object> objects;
int read_request_timecut[MAX_DISK_NUM] = {0};
int read_request_time_num[MAX_DISK_NUM][110] = {0};

// ----------------------------structure--------------------------------
int jump_num = 0;
int T, M, N, V, G;
int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
int disk_point[MAX_DISK_NUM];
int timestamp = 0;
int current_read_tag[MAX_DISK_NUM] = {0};
int count_read_tag[MAX_TAG_NUM][110] = {0};

vector<read_Request> success_read_list;
FILE *file;
int cal_read_request_num_timely[MAX_DISK_NUM] = {0};
int del_size[MAX_TAG_NUM][MAX_TIME_SLICE];//第j个1800帧内tag为i的删除大小
int write_size[MAX_TAG_NUM][MAX_TIME_SLICE];//第j个1800帧内tag为i的写入大小
int accumulated_write_size[MAX_TAG_NUM][MAX_TIME_SLICE] = {0};//第j个1800帧前ag为i的累计写入大小
int read_size[MAX_TAG_NUM][MAX_TIME_SLICE];//第j个1800帧内tag为i的读取大小
int tot_del_size[MAX_TAG_NUM] = { 0 };//每个tag的总删除大小
int tot_write_size[MAX_TAG_NUM] = { 0 };//每个tag的总写入大小
int tot_read_size[MAX_TAG_NUM] = { 0 };//每个tag的总读取大小

int order2tag[MAX_TAG_NUM] = {0,8, 10, 12, 14, 15, 9, 4, 2, 11, 1, 5, 7, 6, 3, 16, 13};
int tag2order[MAX_TAG_NUM] = {0,4,16,15,1,14,13,12,11,6,8,7,10,9,2,5,3};
set<int> tag_group[GROUP_NUM] ={{14,8,12,15,9,4},{6,7},{5,1,14},{2,11}};

int ignore_size = 0;
set<int> bad_tags[MAX_TIME_SLICE];
int current_stage;
int totread = 0;
bool disable_bad_tag = false;
int count_disable_time = 0;
int bad_tags_can_read_cut = 3;
int size1 = 0;
int size2 = 0;  
int size3 = 0;
int size4 = 0;
int size5 = 0;
int success_read_num=0;

// 7 70 0.02 0.1 10330752.5600
// 8 80 0.025 0.1 10322076.5450
// 不pass 8 60 0.02 10592384.6000
// 不pass 9 60 0.02 10594559.1350
// 不pass 9 80 0.02 10595308.5200
// 不pass10 9 80 0.02 10597640.0250
// 不pass9 9 80 0.02 10598284.6050
// 不pass9 9 85 0.02 10598024.9725
int read_as_pass_cut = 11;
int time_cut = 70;

double min_read_times_of_all_tags = 0.014;
// double min_read_times_of_self_tags = 0.2;
// ----------------------------global variable--------------------------------
void init(){
    for(int i = 1; i <= N; i++){
        read_head[i].leftg = G;
    }
    success_read_list.clear();
    for(int i = 1; i <= N; i++){
        cal_read_request_num_timely[i] = 0;
    }
    for(int i = 1; i <= N; i++){
        read_head[i].action = "";
    }
}

int fast_read() {
    int x = 0;
    int c = getchar();
    while(c < '0' || c > '9') c = getchar();
    while(c >= '0' && c <= '9') {
        x = (x << 3) + (x << 1) + c - '0';
        c = getchar();
    }
    return x;
}

void timestamp_action()//时间戳
{
    // scanf("%*s%d", &timestamp);
    timestamp = fast_read();
    printf("TIMESTAMP %d\n", timestamp);

    ////fprintf(file,"TIMESTAMP %d\n", timestamp);

    fflush(stdout);
}

bool compare_by_second(pair<int,int> a, pair<int,int> b){
    return a.second < b.second;
}

void cal_tag2order(){
    for(int i = 1; i <= M; i++){
        tag2order[order2tag[i]] = i;
    }
}

void cal_bad_tags(){
    double total_read_times[MAX_TIME_SLICE] = {0};
    for(int i = 1; i <= static_cast<int>(ceil(T/1800.0)); i++){
        for(int j = 1; j <= M; j++){
            total_read_times[i] += static_cast<double>(read_size[j][i])/ static_cast<double>(accumulated_write_size[j][i]);
        }
    }

    for(int i = 1; i <= static_cast<int>(ceil(T/1800.0)); i++){
        for(int j = 1; j <= M; j++){
            if(static_cast<double>(read_size[j][i]) / static_cast<double>(accumulated_write_size[j][i]) < min_read_times_of_all_tags*total_read_times[i]){
                bad_tags[i].insert(j);
            }
        }
    }
}

void divide_disks() {//将所有磁盘按照tag的总写入大小加权划分
    int size[MAX_TAG_NUM];
    int tot_size = 0;
    vector<pair<int,pair<int,int>>> area;

    for(int i = 1;i<=M;i++){//计算所有tag的总写入大小
        tot_size += tot_write_size[i];
    }
    // ////////fprintf(file,"tot_size:%d\n",tot_size);

    for(int i = 1; i <= M; i++){//计算每个tag分多少
        size[i] = (V / 3 * 20/19) * tot_write_size[i] / tot_size;
        // ////////fprintf(file,"size[%d]:%d\n",i,size[i]);
    }

    int left = 1;
    int right;
    pair<int,pair<int,int>> temp_area[MAX_TAG_NUM];
    for(int i = 1; i <= M; i++){//计算每个tag的区域
        right = left + size[order2tag[i]] - 1;
        temp_area[i] = mkp(order2tag[i],mkp(left,right));
        // area.push_back(mkp(order2tag[i],mkp(left,right)));
        ////////fprintf(file,"area:%d left:%d right:%d, size:%d\n", i, left, right, size[i]);
        left = right + 1;
    }

    for(int i = 1; i <= N; i++){
        for(int j = 1; j <= M; j++){
            disks[i].divided_area.push_back(temp_area[tag2order[j]]);
        }
    }
}

// int divided_area_available (int disk_id, int tag, int size) {
//     int left = disks[disk_id].divided_area[tag].second.first;
//     int right = disks[disk_id].divided_area[tag].second.second;
//     int len = size;
//     int temp_len = 0;
//     for (int i = left; i <= right; i++) {
//         if (temp_len >= len) {
//             return i - len;// 是否需要+1-1
//         }
//         if (disks[disk_id].saved_object_id[i] == 0) {
//             temp_len++;
//         }
//         else {
//             temp_len = 0;
//         }
        
//     }
//     return -1;
// }

void do_object_delete(int id)//删除对象,目前扫盘，待修改，使用Object中的信息
{
    objects[id-1].is_delete = true;
    for(int i=0;i<3;i++){
        for(int j=0;j<objects[id-1].saved_position[i].size();j++){
            disks[objects[id-1].saved_disk[i]].saved_object_id[objects[id-1].saved_position[i][j]] = 0;
        }
    }
}

void do_object_write(int id, int size, int tag, int save_disk_id[], vector<int> areas[])//写入对象
{
    Object obj;
    obj.id = id;
    obj.size = size;
    obj.tag = tag;
    obj.is_delete = false;
    
    // 复制 saved_disk 数组
    for(int i = 0; i < 3; i++) {
        obj.saved_disk[i] = save_disk_id[i];
        // 复制该磁盘的所有区域信息
        obj.saved_position[i] = areas[i];
    }
    objects.push_back(obj);

    for (int i = 0; i < 3; i++) {
        int disk_id = save_disk_id[i];
        for (int j = 0; j < areas[i].size(); j++) {
            for(int k = areas[i][j]; k <= areas[i][j] + size - 1; k++){
                disks[disk_id].saved_object_id[k] = id;
            }
        }
    }
}

void clean()//释放内存
{
    // for (int i = 0; i < MAX_REQUEST_NUM-1; i++) {
    //     if (read_requests[i] != nullptr) {
    //         free(read_requests[i]);
    //     }
    //     read_requests[i] = nullptr;
    // }
    // for (int i = 0; i < success_read_list.size(); i++) {
    //     if (success_read_list[i] != nullptr) {
    //         free(success_read_list[i]);
    //     }
    //     success_read_list[i] = nullptr;
    // }
    // success_read_list.clear();
    // for (int i = 1; i <= N; i++) {
    //     if (read_head[i].target_request != nullptr) {
    //         free(read_head[i].target_request);
    //     }
    //     read_head[i].target_request = nullptr;
    // }
    // for (int i = 1; i <= N; i++) {
    //     for (int j = 1; j <= V; j++) {
    //         for (int k = 0; k < read_head[i].read_list[j].size(); k++) {
    //             if (read_head[i].read_list[j][k] != nullptr) {
    //                 free(read_head[i].read_list[j][k]);
    //             }
    //             read_head[i].read_list[j][k] = nullptr;
    //         }
    //     }
    // }
    
}

int update_tokens(int tokens,int op,int pre_op,int pre_used_tokens)//更新令牌
{
    //op:1:jump,2:pass,3:read
    if (op == 1) {
        return 0;
    }
    else if (op == 2) {
        return tokens - 1;
    }
    else if (op == 3) {
        if(pre_op == 3){
            return tokens - max(16,static_cast<int>(ceil(pre_used_tokens*0.8)));
        }
        else{
            return tokens - 64;
        }
    }
    return 0;
}

void write_print(int id, int size, int tag, int save_disk_id[], vector<int> areas[]){
    printf("%d\n",id);
    ////fprintf(file,"id:%d\n",id);
    for(int i = 0; i < 3; i++){
        printf("%d ",save_disk_id[i]);
        ////fprintf(file,"disk_id:%d ",save_disk_id[i]);
        for(int j = 0; j < areas[i].size(); j++){
            printf("%d ",areas[i][j]);
            ////fprintf(file,"area:%d ",areas[i][j]);
        }
        ////fprintf(file,"\n");
        printf("\n");
    }     
}

void jump(int id,int position ,MoveAction move){
    // read_head[id].leftg -= move.gcost;
    read_head[id].position = position;
    read_head[id].last_read = false;
    read_head[id].last_cost = 0;
    read_head[id].action += "j";
    read_head[id].action += " ";
    read_head[id].action += to_string(position);
    // printf("j %d\n",position);
    jump_num++;
    
    //////fprintf(file,"j %d\n",position);
}

void pass(int id,MoveAction move){
    
    read_head[id].position = (read_head[id].position + 1) % V;
    read_head[id].last_read = false;
    read_head[id].last_cost = 0;
    read_head[id].action += "p";
    // read_head[id].leftg -= move.gcost;
    // printf("p");
    ////fprintf(file,("p"));
}

void read_as_pass(int id,bool last_read,MoveAction move){
    read_head[id].position = (read_head[id].position + 1) % V;
    read_head[id].last_read = true;
    read_head[id].last_cost = move.gcost;
    read_head[id].action += "r";
    // read_head[id].leftg -= move.gcost;
    // printf("r");
    ////fprintf(file,"r");
}
void read(int id,bool last_read,int last_cost){
    read_head[id].target_position+=1;
    int now_position = read_head[id].position;
    int target_obj_position = objects[read_head[id].target_request.object_id-1].saved_position[0][0];
    if (now_position == target_obj_position){
        int size = read_head[id].read_list[target_obj_position].size();
        for (int i = 0; i < size; i++){
            if (read_head[id].read_list[target_obj_position][i].is_done == false && read_head[id].read_list[target_obj_position][i].is_abort == false){
                read_head[id].read_list[target_obj_position][i].is_read = true;
            }
        }
    }
    //////////fprintf(file,"id:%d\n",id);
    int cost_g;

    if (last_read){//如果上一次是读，则cost_g为上次读的cost的80%，否则为64
        cost_g = max(16,static_cast<int>(ceil(last_cost*0.8)));
    }
    else{
        cost_g = 64;
    }
    if(read_head[id].leftg >= cost_g){//如果磁头剩余g大于需要的g，则读取
        read_head[id].leftg -= cost_g;
        read_head[id].last_cost = cost_g;
        read_head[id].last_read = true;
    }
    int obj_id = read_head[id].target_request.object_id;
    int obj_size = objects[obj_id-1].size;
    //////////fprintf(file,"obj_id:%d,obj_size:%d\n",obj_id,obj_size);

    if(read_head[id].position == objects[obj_id-1].saved_position[0][obj_size-1]){
        //如果磁头读完了当前obj，则target_position为0
        // success_read_list.push_back(read_head[id].target_request);
        // //////////fprintf(file,"success_read_list:%d\n",read_head[id].target_request->request_id);
        // for(auto it = read_head[id].read_list.begin(); it != read_head[id].read_list.end(); ++it){
        //     if((*it) == read_head[id].target_request){
        //         read_head[id].read_list.erase(it);
        //         // delete *it;
        //         break;
        //     }
        // }
        int position = objects[obj_id-1].saved_position[0][0];
        for (int i = 0; i < read_head[id].read_list[position].size(); i++) {
            if (read_head[id].read_list[position][i].is_read == true && read_head[id].read_list[position][i].is_abort == false && read_head[id].read_list[position][i].is_done == false){
                read_head[id].read_list[position][i].is_read = false;
                read_head[id].read_list[position][i].is_done = true;
                disks[id].left_request_num[position]--;
                // objects[obj_id-1].can_read_request_num--;
                success_read_list.push_back(read_head[id].read_list[position][i]);
            }
        }
        if(read_head[id].read_list[position][read_head[id].read_list[position].size()-1].is_done == true){
            read_head[id].read_list[position].clear();
        }
        read_head[id].target_position = 0;
    }

    read_head[id].action += "r";
    // printf("r");
    ////fprintf(file,"r");
    read_head[id].position = (read_head[id].position + 1) % V;
}

void end(int id){
    read_head[id].action += "#";
    // printf("#\n");
    ////fprintf(file,"#\n");
}

bool read_as_pass_judge(int disk_id, int st,int ed,int id,bool last_read,int last_cost){
    if(!last_read){
        return false;
    }
    // if (bad_tags[current_stage].count(objects[disks[disk_id].saved_object_id[ed]-1].tag) == 1){
    //     return false;
    // }
    int cost_1 = 0;
    int cost_2 = ((ed-st)>0?ed-st:V+ed-st);
    int last_cost_2 = 64;//记录不read_as_pass的状态
    bool last_read_2 = false;//记录不read_as_pass的状态
    int is_read_id_2 = 0;//记录不read_as_pass正在读的object_id
    for(int i = st;i < ed;i++){
        cost_1 += max(16,static_cast<int>(ceil(last_cost*0.8)));
        last_cost = max(16,static_cast<int>(ceil(last_cost*0.8)));
    }
    for(int i = ed ; i < ed + read_as_pass_cut ; i++){
        cost_1 += max(16,static_cast<int>(ceil(last_cost*0.8)));
        last_cost = max(16,static_cast<int>(ceil(last_cost*0.8)));
        last_read = true;
        int size = read_head[id].read_list[i].size();
        
        bool flag1 = last_read_2 
        && size > 0 
        && read_head[id].read_list[objects[disks[id].saved_object_id[i]-1].saved_position[0][0]][size-1].st_time > timestamp - 105 
        && read_head[id].read_list[objects[disks[id].saved_object_id[i]-1].saved_position[0][0]][size-1].is_abort == false 
        && read_head[id].read_list[objects[disks[id].saved_object_id[i]-1].saved_position[0][0]][size-1].is_done == false;
        bool flag2 = !last_read_2
        && size > 0
        && read_head[id].read_list[objects[disks[id].saved_object_id[i]-1].saved_position[0][0]][size-1].st_time > timestamp - read_request_timecut[id] 
        && read_head[id].read_list[objects[disks[id].saved_object_id[i]-1].saved_position[0][0]][size-1].is_abort == false
        && read_head[id].read_list[objects[disks[id].saved_object_id[i]-1].saved_position[0][0]][size-1].is_done == false 
        && bad_tags[current_stage].count(objects[read_head[id].read_list[i][size-1].object_id-1].tag) == 0;
        if(flag1||flag2){
            is_read_id_2 = read_head[id].read_list[i][size-1].object_id;
        }
        if(disks[id].saved_object_id[i] == is_read_id_2){
            cost_2 += last_read_2 ? max(16,static_cast<int>(ceil(last_cost_2*0.8))) : 64;
            last_cost_2 = last_read_2 ? max(16,static_cast<int>(ceil(last_cost_2*0.8))) : 64;
            last_read_2 = true;
        }
        else{
            cost_2 += 1;
            last_read_2 = false;
            last_cost_2 = 64;
        }
        if(cost_1 < cost_2){
            return true;
        }
    }
    return false;
}
MoveAction cost_to_position(int st,int ed,int g,bool last_read,int last_cost,int id){
    int cost_g;
    int cost = 1;

    if (last_read){//如果上一次是读，则cost_g为上次读的cost的80%，否则为64
        cost_g = max(16,static_cast<int>(ceil(last_cost*0.8)));
    }
    else{
        cost_g = 64;
    }

    if(st == ed){//如果磁头位置和目标位置相同，且剩余g大于需要的g，则不移动
        if(g >= cost_g){
            MoveAction move;//read
            move.op = 0;
            move.gcost = 0;
            move.time = 0;
            return move;
        }
        else{
            MoveAction move;//not move
            move.op = 3;
            move.gcost = g;
            move.time = 1;
            if(cost_g - g == 1){
                // if(timestamp == 10569 && id ==8){
                //     fprintf(file,"%s\n",read_head[id].action.c_str());
                // }
                for(int i = read_head[id].action.size()-1; i >= 0; i--){
                    if(read_head[id].action[i] == 'p'){
                        read_head[id].last_read = true;
                        read_head[id].last_cost = read_head[id].action[read_head[id].action.size()-1] == 'r'?max(16,static_cast<int>(ceil(last_cost * 0.8))):64;
                        read_head[id].action[i] = 'r';
                        break;
                    }
                }
                // if(timestamp == 10569 && id == 8){
                //     fprintf(file,"%s\n",read_head[id].action.c_str());
                //     fclose(file);
                // }
            }
            return move;
        }
    }
    else if(g<((ed-st)>0?(ed-st)%V:V+(ed-st)%V) && g>=G ){//如果jump优于pass，且剩余g大于G，则jump
        MoveAction move;//jump
        move.op = 2;
        move.gcost = G;
        move.time = 1;
        return move;
    }
    else{//剩下的所有情况都应该pass
        if(last_read){
            bool read_as_pass_or_not = read_as_pass_judge(id, st,ed,id,last_read,last_cost);
            if(read_as_pass_or_not && g >= cost_g){            
                MoveAction move;
                move.op = 4;
                move.gcost = cost_g;
                move.time = 0;
                return move;
            }
            //10 10597640.0250
            if (read_as_pass_or_not && g < cost_g) {
                MoveAction move;
                move.op = 3;
                move.gcost = g;
                move.time = 1;
                if(cost_g - g == 1){
                    for(int i = read_head[id].action.size()-1; i >= 0; i--){
                        if(read_head[id].action[i] == 'p'){
                            read_head[id].last_read = true;
                            read_head[id].last_cost = read_head[id].action[read_head[id].action.size()-1] == 'r'?max(16,static_cast<int>(ceil(last_cost * 0.8))):64;
                            read_head[id].action[i] = 'r';
                            break;
                        }
                    }
                }
                return move;
            }
        }
        
        MoveAction move;
        move.op = 1;
        move.gcost = 1;
        // //////////fprintf(file,"move.gcost:%d\n",move.gcost);
        move.time = 0;
        return move;
    }
}

int calculate_readtime(int len){
    int g = G;
    int time = 0;
    int cost = 64;
    for(int i=1;i<=len;i++){
        if(g>=cost){
            g -= cost;
            cost = max(16,static_cast<int>(ceil(cost*0.8)));
        }
        else{
            time++;
            g = G;
        }
    }
    return time;
}

double calculate_profit(int time,int size){
    double mul = 0.5*(size+1);
    if(time <= 10){
        return mul*(-0.005*time+1);
    }
    else if(time <= 105){
        return mul*(-0.01*time+1.05);
    }
    else{
        return 0;
    }
}

// void adjust_read_request_timecut(){
//     for(int i=1;i<=N;i++){
//         for(int j = 1; j <= 105; j++){
//             read_request_time_num[i][j-1] = read_request_time_num[i][j];
//         }
//     }
//     for(int i = 1; i <= N; i++){
//         read_request_time_num[i][105] = cal_read_request_num_timely[i];
//     }
//     int sum[MAX_DISK_NUM] = {0};
//     int weight[MAX_DISK_NUM] = {0};
//     int avg[MAX_DISK_NUM] = {0};
//     for(int i = 1; i <= N; i++){
//         for(int j = 105; j >= 1; j--){
//             if(j <= 10){
//                 sum[i] += read_request_time_num[i][j] * j * 2;
//                 weight[i] += read_request_time_num[i][j];
//             }
//             else{
//                 sum[i] += read_request_time_num[i][j] * 10;
//                 weight[i] += read_request_time_num[i][j];
//             }
//         }
//         if(sum[i] <= 10000){
//             continue;
//         }
//         avg[i] = ceil(sum[i] / weight[i]);
//         if(sum[i] > 10000){
//             read_request_timecut[i] = 105-avg[i];
//         }
//     }
// }
// ----------------------------basic function--------------------------------

void delete_action()//删除对象
{
    int n_delete;
    int abort_num = 0;
    vector<int> abort_request_id;
    int obj_id[MAX_OBJECT_NUM];

    // scanf("%d", &n_delete);
    n_delete = fast_read();
    for (int i = 1; i <= n_delete; i++) {
        // scanf("%d", &obj_id[i]);
        obj_id[i] = fast_read();
        do_object_delete(obj_id[i]); //同步obj和disk删除的状态
    }
    // //////////fprintf(file,"below pppppppppp\n");
    for (int i = 1; i <= n_delete; i++) {
        int disk_id = objects[obj_id[i]-1].saved_disk[0];
        // read_head[disk_id].target_request = nullptr;
        // for (int j = 0; j <= objects[obj_id[i]-1].saved_position[0].size(); j++){
        //     if (read_head[disk_id].target_position == objects[obj_id[i]-1].saved_position[0][j]){
        //         read_head[disk_id].target_position = 0;
        //         break;
        //     }
        // }
        for(int j = 0;j<objects[obj_id[i]-1].saved_position[0].size();j++){
            // for (int k = 0; k < read_head[disk_id].read_list[objects[obj_id[i]-1].saved_position[0][j]].size(); k++){
            //     read_head[disk_id].read_list[objects[obj_id[i]-1].saved_position[0][j]][k].is_abort = true;
            // }
            if(read_head[disk_id].target_position == objects[obj_id[i]-1].saved_position[0][j]){
                read_head[disk_id].target_position = 0;
                break;
            }
        }
        int position = objects[obj_id[i]-1].saved_position[0][0];
        int size = read_head[disk_id].read_list[position].size();
        for (int j = 0; j < size; j++){
            if (read_head[disk_id].read_list[position][j].is_done == false && read_head[disk_id].read_list[position][j].is_abort == false){
                read_head[disk_id].read_list[position][j].is_abort = true;
                read_head[disk_id].read_list[position][j].is_read = false;
                read_head[disk_id].read_list[position].clear();
                disks[disk_id].left_request_num[position]--;
                abort_num++;
                abort_request_id.push_back(read_head[disk_id].read_list[position][j].request_id);
            }
        }

        
        disks[objects[obj_id[i]-1].saved_disk[0]].used_clean_area_size[objects[obj_id[i]-1].tag]-=objects[obj_id[i]-1].size;
        disks[objects[obj_id[i]-1].saved_disk[1]].trash_area_size-=objects[obj_id[i]-1].size;
        disks[objects[obj_id[i]-1].saved_disk[2]].trash_area_size-=objects[obj_id[i]-1].size;
        // for (auto it = objects[obj_id[i]-1].object_read_requests.begin(); it != objects[obj_id[i]-1].object_read_requests.end(); it++){
        //     if ((*it)->is_done == false && (*it)->is_abort == false){
        //         abort_num++;
        //         abort_request_id.push_back((*it)->request_id);
        //         (*it)->is_abort = true;
        //         read_requests[(*it)->request_id]->is_abort = true;
        //     }
        //     else if ((*it)->is_done == true){
        //         objects[obj_id[i]-1].object_read_requests.erase(it);
        //     }
        // }


        // for(auto it = objects[obj_id[i]-1].object_read_requests.begin(); it != objects[obj_id[i]-1].object_read_requests.end(); ){
        //     if((*it)->is_done == false && (*it)->is_abort == false){
        //         abort_num++;
        //         abort_request_id.push_back((*it)->request_id);
        //         (*it)->is_abort = true;
        //         read_requests[(*it)->request_id]->is_abort = true;
                // read_head[objects[obj_id[i]-1].saved_disk[0]].target_request = nullptr;
                // read_head[objects[obj_id[i]-1].saved_disk[0]].target_position = 0;
        //         for(auto it2 = read_head[objects[obj_id[i]-1].saved_disk[0]].read_list.begin(); it2 != read_head[objects[obj_id[i]-1].saved_disk[0]].read_list.end(); it2++){
        //             if((*it2) == (*it)){
        //                 read_head[objects[obj_id[i]-1].saved_disk[0]].read_list.erase(it2);
        //                 break;
        //             }
        //         }
        //         objects[obj_id[i]-1].object_read_requests.erase(it);
        //         // delete *it;
        //     }
        //     else{
        //         ++it;
        //     }
        // }
        // objects[obj_id[i]-1].object_read_requests.clear();
    }

    printf("%d\n", abort_num);
    ////fprintf(file,"abort_num:%d\n",abort_num);
    for (int i = 0; i < abort_request_id.size(); i++) {
        printf("%d\n", abort_request_id[i]);
        ////fprintf(file,"%d\n", abort_request_id[i]);
    }

    fflush(stdout);
}

// void clear_time_out() {
    // for (int i = 1; i <= N; i++) {
    //     for (auto it = read_head[i].read_list.begin(); it != read_head[i].read_list.end(); ) {
    //         if ((*it)->st_time <= timestamp-105) {
    //             it = read_head[i].read_list.erase(it);
    //         }
    //         else {
    //             ++it;
    //         }
    //     }
    // }
// }

int find_big_clean_disk(int tag){//找used clean area最小的磁盘
    int min_clean_area_size = 1000000000;
    int min_clean_area_disk = 1;
    for(int i = 1; i <= N; i++){
        if(disks[i].used_clean_area_size[tag] < min_clean_area_size){
            min_clean_area_size = disks[i].used_clean_area_size[tag];
            min_clean_area_disk = disks[i].id;
        }
    }
    return min_clean_area_disk;
}


bool compare_by_trash_area_size(Disk a, Disk b){
    return a.trash_area_size < b.trash_area_size;
}

bool compare_by_id(Disk a, Disk b){
    return a.id < b.id;
}

void write_action()//写入对象
{
    int n_write;
    int id[MAX_OBJECT_NUM];
    int size[MAX_OBJECT_NUM];
    int tag[MAX_OBJECT_NUM];
    // scanf("%d", &n_write);
    n_write = fast_read();
    for (int i = 1; i <= n_write; i++) {
        // scanf("%d%d%d", &id[i], &size[i], &tag[i]);
        id[i] = fast_read();
        size[i] = fast_read();
        tag[i] = fast_read();
        if(size[i] == 1){
            size1++;
        }
        else if(size[i] == 2){
            size2++;
        }
        else if(size[i] == 3){
            size3++;
        }
        else if(size[i] == 4){
            size4++;
        }
        else{
            size5++;
        }
    }

    int chosen_disks[3];

    for (int i = 1; i <= n_write; i++) {
        bool writed_successfully[3] = {false,false,false};//三个盘分别是否能够写入

        int chosen_disks[3];//选择磁盘
        chosen_disks[0] = find_big_clean_disk(tag[i]);
        ////////fprintf(file,"chosen_disk:%d\n",chosen_disks[0]);
        sort(disks.begin()+1,disks.end(),compare_by_trash_area_size);//按trash_area_size从小到大排序

        if(disks[1].id == chosen_disks[0]){
            chosen_disks[1] = disks[2].id;
            chosen_disks[2] = disks[3].id;
        }
        else if(disks[2].id == chosen_disks[0]){
            chosen_disks[1] = disks[1].id;
            chosen_disks[2] = disks[3].id;
        }
        else{
            chosen_disks[1] = disks[1].id;
            chosen_disks[2] = disks[2].id;
        }
        sort(disks.begin()+1, disks.end(), compare_by_id);

        Object obj;
        obj.id = id[i];
        obj.size = size[i];
        obj.tag = tag[i];
        obj.is_delete = false;
        obj.saved_disk[0] = chosen_disks[0];
        obj.saved_disk[1] = chosen_disks[1];
        obj.saved_disk[2] = chosen_disks[2];

        // ////////fprintf(file,"write_action2\n");
        // ////////fprintf(file,"chosen_disk:%d\n",chosen_disks[0]);

        //写入本体
        //写入本体到对应的tag分区
        
        
        //找到主盘中tag为tag[i]的分区
        int left = disks[chosen_disks[0]].divided_area[tag[i]-1].second.first;
        int right = disks[chosen_disks[0]].divided_area[tag[i]-1].second.second;
        // fprintf(file,"tag:%d,left:%d,right:%d\n",tag[i],left,right);
        int leftright = tag2order[tag[i]]>1?disks[chosen_disks[0]].divided_area[order2tag[tag2order[tag[i]]-1]-1].second.first:1;
        int rightleft = tag2order[tag[i]]<M?disks[chosen_disks[0]].divided_area[order2tag[tag2order[tag[i]]+1]-1].second.second:right;
        int cal = 0;

        if(tag2order[tag[i]] % 2 == 1){
            for(int j = left; j <= rightleft; j++){
                if(disks[chosen_disks[0]].saved_object_id[j] == 0){
                    cal++;
                }
                else{
                    cal = 0;
                }
                if(cal >= size[i]){
                    writed_successfully[0] = true;//本体能够成功写入初始分区内
                    for(int k = j-size[i]+1; k <= j ; k++){
                        ////////fprintf(file,"self_pushback_in_devided_area disk:%d position:%d\n",chosen_disks[0],k);
                        disks[chosen_disks[0]].saved_object_id[k] = id[i];
                        obj.saved_position[0].push_back(k);
                    }
                    disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                    break;
                }
            }
        }
        else{
            for(int j = right; j >= leftright; j--){
                if(disks[chosen_disks[0]].saved_object_id[j] == 0){
                    cal++;
                }
                else{
                    cal = 0;
                }
                if(cal >= size[i]){
                    writed_successfully[0] = true;//本体能够成功写入初始分区内
                    for(int k = j; k <= j + size[i] - 1 ; k++){
                        ////////fprintf(file,"self_pushback_in_devided_area disk:%d position:%d\n",chosen_disks[0],k);
                        disks[chosen_disks[0]].saved_object_id[k] = id[i];
                        obj.saved_position[0].push_back(k);
                    }
                    disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                    break;
                }
            }

        }
        if(!writed_successfully[0]){
                int group_index = 0;
                for(int j = 0; j < GROUP_NUM; j++){//找当前obj对应的group
                    if(tag_group[j].count(tag[i]) > 0){
                        group_index = j;
                        break;
                    }
                }
                // fprintf(file,"tag:%d,group:%d\n",tag[i],group_index);

                for(int temp_tag : tag_group[group_index]){
                    if(writed_successfully[0]){//写入成功后就跳出循环
                        break;
                    }
                    // fprintf(file,"tag_area:%d\n",temp_tag);
                    
                    int left = disks[chosen_disks[0]].divided_area[temp_tag-1].second.first;
                    int right = disks[chosen_disks[0]].divided_area[temp_tag-1].second.second;
                    cal = 0;
                    // fprintf(file,"left:%d,right:%d",left,right);
                    for(int j = left; j <= right; j++){
                        if(disks[chosen_disks[0]].saved_object_id[j] == 0){
                            cal++;
                        }
                        else{
                            cal = 0;
                        }
                        if(cal >= size[i]){
                            writed_successfully[0] = true;//本体能够成功写入初始分区内
                            for(int k = j-size[i]+1; k <= j ; k++){
                                // fprintf(file,"self_pushback_in_devided_area disk:%d position:%d\n",chosen_disks[0],k);
                                disks[chosen_disks[0]].saved_object_id[k] = id[i];
                                obj.saved_position[0].push_back(k);
                            }
                            disks[chosen_disks[0]].used_clean_area_size[temp_tag] += size[i];
                            break;
                        }
                    }
                }
            }

        if(!writed_successfully[0]&&disks[chosen_disks[0]].allocated_area.size() == 0){//如果写不下，且allocated_area为空，则创建第一个allocated_area
            int idx = disks[chosen_disks[0]].divided_area.size();
            left = disks[chosen_disks[0]].divided_area[order2tag[idx]-1].second.second + 1;//第一个allocated_area的起始位置为最后一个分区的结束位置+1
            right = left + ALLOCATE_SIZE - 1;
            disks[chosen_disks[0]].allocated_area.push_back(mkp(tag[i],mkp(left,right)));
        }

        if(!writed_successfully[0]){//在分区外的allocated_area中寻求整存
            bool area_found = false;
            for(int j = 0; j < disks[chosen_disks[0]].allocated_area.size(); j++){//遍历每个allocated_area
                if(disks[chosen_disks[0]].allocated_area[j].first == tag[i]){//找到了对应tag的allocated_area
                    cal = 0;
                    left = disks[chosen_disks[0]].allocated_area[j].second.first;
                    right = disks[chosen_disks[0]].allocated_area[j].second.second;
                    if(j%2==0){
                        for(int k = left; k <= right; k++){
                            if(disks[chosen_disks[0]].saved_object_id[k] == 0){
                                cal++;
                            }
                            else{
                                cal = 0;
                            }
                            if(cal >= size[i]){//能在allocated_area里写下
                                writed_successfully[0] = true;
                                for(int l = k - size[i] + 1; l <= k ; l++){//写入
                                    disks[chosen_disks[0]].saved_object_id[l] = id[i];
                                    obj.saved_position[0].push_back(l);
                                }
                                disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                                area_found = true;
                                break;
                            }
                        }
                    }
                    else{
                        for(int k = right; k >= left; k--){
                            if(disks[chosen_disks[0]].saved_object_id[k] == 0){
                                cal++;
                            }
                            else{
                                cal = 0;
                            }
                            if(cal >= size[i]){//能在allocated_area里写下
                                writed_successfully[0] = true;
                                for(int l = k; l <= k+size[i] - 1 ; l++){//写入
                                    disks[chosen_disks[0]].saved_object_id[l] = id[i];
                                    obj.saved_position[0].push_back(l);
                                }
                                disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                                area_found = true;
                                break;
                            }
                        }
                    }
                    
                }
                if(area_found) {
                    break;
                }
            }
        }

        if(!writed_successfully[0]&&disks[chosen_disks[0]].allocated_area.size()!=0){//如果依然写不下，新开一个allocated_area
            int idx = disks[chosen_disks[0]].allocated_area.size()-1;
            left = disks[chosen_disks[0]].allocated_area[idx].second.second + 1;
            right = left + ALLOCATE_SIZE - 1;
            if(right < V){//如果新的allocated_area的结束位置小于M，则创建新的allocated_area
                disks[chosen_disks[0]].allocated_area.push_back(mkp(tag[i],mkp(left,right)));
                // //////fprintf(file,"create more allocated_area disk: %d tag: %d left: %d right: %d\n",chosen_disks[0],tag[i],left,right);
            }
        }

        if(!writed_successfully[0]){//在分区外的allocated_area中寻求整存
            bool area_found = false;
            for(int j = 0; j < disks[chosen_disks[0]].allocated_area.size(); j++){//遍历每个allocated_area
                if(disks[chosen_disks[0]].allocated_area[j].first == tag[i]){//找到了对应tag的allocated_area
                    cal = 0;
                    left = disks[chosen_disks[0]].allocated_area[j].second.first;
                    right = disks[chosen_disks[0]].allocated_area[j].second.second;
                    if(j%2==0){
                        for(int k = left; k <= right; k++){
                            if(disks[chosen_disks[0]].saved_object_id[k] == 0){
                                cal++;
                            }
                            else{
                                cal = 0;
                            }
                            if(cal >= size[i]){//能在allocated_area里写下
                                writed_successfully[0] = true;
                                for(int l = k - size[i] + 1; l <= k ; l++){//写入
                                    disks[chosen_disks[0]].saved_object_id[l] = id[i];
                                    obj.saved_position[0].push_back(l);
                                }
                                disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                                area_found = true;
                                break;
                            }
                        }
                    }
                    else{
                        for(int k = right; k >= left; k--){
                            if(disks[chosen_disks[0]].saved_object_id[k] == 0){
                                cal++;
                            }
                            else{
                                cal = 0;
                            }
                            if(cal >= size[i]){//能在allocated_area里写下
                                writed_successfully[0] = true;
                                for(int l = k; l <= k+size[i] - 1 ; l++){//写入
                                    disks[chosen_disks[0]].saved_object_id[l] = id[i];
                                    obj.saved_position[0].push_back(l);
                                }
                                disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                                area_found = true;
                                break;
                            }
                        }
                    }
                    
                }
                if(area_found) {
                    break;
                }
            }
        }

        for(int j = 1; j <= 2; j++){
            // ////////fprintf(file,"j%d\n",j);
            cal = 0;
            // ////////fprintf(file,"chosen_disk:%d size:%d\n",chosen_disks[j],disks[chosen_disks[j]].size);
            for(int k = V; k >= 1; k--){
                if(disks[chosen_disks[j]].saved_object_id[k] == 0){
                    cal++;
                    obj.saved_position[j].push_back(k);
                    ////////fprintf(file,"pushback%d :%d\n",chosen_disks[j],k);
                    disks[chosen_disks[j]].saved_object_id[k] = id[i];
                    if(cal >= size[i]){
                        writed_successfully[j] = true;//副本能够成功写入
                        disks[chosen_disks[j]].trash_area_size += size[i];
                        break;
                    }
                }
            }
        }

        if(!writed_successfully[0]){//开了allocated_area都写不下
            //零碎写入  
            cal = 0;
            for(int j = 1; j <= V; j++){
                if(disks[chosen_disks[0]].saved_object_id[j] == 0){
                    cal++;
                    obj.saved_position[0].push_back(j);
                    disks[chosen_disks[0]].saved_object_id[j] = id[i];
                    disks[chosen_disks[0]].used_clean_area_size[tag[i]] += 1;
                    if(cal >= size[i]){
                        writed_successfully[0] = true;
                        break;
                    }
                }
            }
        }
        // for(int j= 1;j<= disks.size();j++){
        //     for(int k = 1; k <= disks[j].size; k++){
        //         ////////fprintf(file,"disk: %d;saved_disk_id %d;saved_position %d\n",disks[j].id,disks[j].saved_object_id[k],k);
        //     }
        // }
        objects.push_back(obj);
        write_print(id[i],size[i],tag[i],chosen_disks,obj.saved_position);
    }
    // fclose(file);
    fflush(stdout);
}

void findTarget(int disk_id){
    int position = read_head[disk_id].position;
    
    for (int i = 1; i <= V; i++){
        int size = read_head[disk_id].read_list[(i+position-2)%V+1].size();
        if(i==1 
        && size > 0 
        && read_head[disk_id].read_list[(i+position-2)%V+1][size-1].st_time > timestamp - 95
        && read_head[disk_id].read_list[(i+position-2)%V+1][size-1].is_abort == false 
        && read_head[disk_id].read_list[(i+position-2)%V+1][size-1].is_done == false){
            read_head[disk_id].target_request = read_head[disk_id].read_list[(i+position-2)%V+1][size-1];
            read_head[disk_id].target_position = objects[read_head[disk_id].target_request.object_id - 1].saved_position[0][0];
            
            break;
        }
        if (size > 0 
        && read_head[disk_id].read_list[(i+position-2)%V+1][size-1].st_time > timestamp- read_request_timecut[disk_id] 
        && read_head[disk_id].read_list[(i+position-2) % V + 1][size - 1].is_abort == false 
        && read_head[disk_id].read_list[(i+position-2) % V + 1][size - 1].is_done == false ){
            // if(timestamp < 25000){
                if(disable_bad_tag){
                    read_head[disk_id].target_request = read_head[disk_id].read_list[(i+position-2)%V+1][size-1];
                    read_head[disk_id].target_position = objects[read_head[disk_id].target_request.object_id - 1].saved_position[0][0];
                    break;
                }
                else if(bad_tags[current_stage].count(objects[read_head[disk_id].read_list[(i+position-2)%V+1][size-1].object_id-1].tag) == 0){
                    read_head[disk_id].target_request = read_head[disk_id].read_list[(i+position-2)%V+1][size-1];
                    read_head[disk_id].target_position = objects[read_head[disk_id].target_request.object_id - 1].saved_position[0][0];
                    break;
                }
                else{
                    int count = 0;
                    for(int j = 0;j<read_head[disk_id].read_list[(i+position-2)%V+1].size();j++){
                        if(read_head[disk_id].read_list[(i+position-2)%V+1][j].st_time > timestamp - 105 && read_head[disk_id].read_list[(i+position-2)%V+1][j].is_abort == false && read_head[disk_id].read_list[(i+position-2)%V+1][j].is_done == false){
                            count++;
                        }
                    }
                    if(count >= bad_tags_can_read_cut){
                        disable_bad_tag = true;
                        read_head[disk_id].target_request = read_head[disk_id].read_list[(i+position-2)%V+1][size-1];
                        read_head[disk_id].target_position = objects[read_head[disk_id].target_request.object_id - 1].saved_position[0][0];
                        break;
                    }
                }
        }
    }
            // }
}

void read_action()//读取对象        
{
    int n_read;
    int request_id, object_id;
    // scanf("%d", &n_read);
    n_read = fast_read();
    int bg = 0;
    //////////fprintf(file,"n_read:%d\n",n_read);
    for (int i = 1; i <= n_read; i++) {
        // scanf("%d%d", &request_id, &object_id);
        request_id = fast_read();
        object_id = fast_read();
        if(i==1){
            bg = request_id;
        }
        read_Request temp_request ;
        temp_request.request_id = request_id;
        temp_request.object_id = object_id;
        temp_request.is_done = false;
        temp_request.is_abort = false;
        temp_request.st_time = timestamp;
        read_requests[request_id] = temp_request;
        cal_read_request_num_timely[objects[object_id-1].saved_disk[0]]++;
        // objects[object_id-1].object_read_requests.push_back(read_requests[request_id]);
        // //////////fprintf(file,"request_id:%d,object_id:%d\n",request_id,object_id);
    }
    
    // //////////fprintf(file,"read1\n");
    for(int i = 1; i <= n_read; i++){
        Object obj = objects[read_requests[bg-1+i].object_id-1];
        int disk_id = obj.saved_disk[0];
        read_head[disk_id].read_list[obj.saved_position[0][0]].push_back(read_requests[bg-1+i]);
        disks[disk_id].left_request_num[obj.saved_position[0][0]]++;
    }

    for(int i = 1; i <= N; i++){
        int jump_flag = 0;
        int TIMER = 0;
        while(read_head[i].leftg > 0 && TIMER < G){
            TIMER++;
            // //////////fprintf(file,"leftg:%d\n",read_head[i].leftg);
            // //////////fprintf(file,"target_position:%d\n",read_head[i].target_position);
            if(read_head[i].target_position != 0){
                MoveAction move = cost_to_position(read_head[i].position,read_head[i].target_position,read_head[i].leftg,read_head[i].last_read,read_head[i].last_cost,i);
                // //////////fprintf(file,"id:%d,st:%d,ed:%d\n",i,read_head[i].position,read_head[i].target_position);
                read_head[i].leftg -= move.gcost;
                if(move.op == 0){
                    read(i,read_head[i].last_read,read_head[i].last_cost);
                }
                else if(move.op == 1){
                    pass(i,move);
                }
                else if(move.op == 2){
                    jump_flag = 1;
                    jump(i,read_head[i].target_position,move); 
                }
                else if(move.op == 3){}
                else if(move.op == 4){
                    read_as_pass(i,read_head[i].last_read,move);
                }
            }
            else{
                findTarget(i);
                // if(read_head[i].target_request != nullptr){
                //     //////////fprintf(file,"target_request:%d\n",read_head[i].target_request->request_id);
                // }
                if(read_head[i].target_position == 0){
                    read_head[i].leftg = 0;
                }
            }
        }
        if(jump_flag == 0){
            end(i);
        }
    }
    for(int i = 1; i <= N; i++){
        printf("%s\n",read_head[i].action.c_str());
    }
    // printf("0\n");
    printf("%d\n",success_read_list.size());
    // if(timestamp >= 10100&&timestamp<=11000){
    //     success_read_num+=success_read_list.size();
    // }
    // if(timestamp>11000){
    //     fprintf(file,"success_read_num:%d\n",success_read_num);
    //     fclose(file);
    // }
    ////fprintf(file,"success:%d\n",success_read_list.size());
    for(int i = 0; i < success_read_list.size(); i++){
        totread++;
        printf("%d\n",success_read_list[i].request_id);
        ////fprintf(file,"%d\n",success_read_list[i].request_id);
    }
    success_read_list.clear();
    // //////////fprintf(file,"read3\n");
    fflush(stdout);
}

//---------------------------------complex function---------------------------------
void preprocess(){
    // scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    T = fast_read();
    M = fast_read();
    N = fast_read();
    V = fast_read();
    G = fast_read();

    for (int i = 1; i <= M; i++) {//计算每个tag总共删除的大小
        int deled_num = 0;
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            // scanf("%d",&del_size[i][j]);
            del_size[i][j] = fast_read();
            deled_num += del_size[i][j];
            accumulated_write_size[i][j] -= deled_num;
            tot_del_size[i] += del_size[i][j];
        }
    }

    for (int i = 1; i <= M; i++) {//计算每个tag总共写入的大小
        int writed_num = 0;
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            // scanf("%d",&read_size[i][j]);
            write_size[i][j] = fast_read();
            writed_num += write_size[i][j];
            accumulated_write_size[i][j] += writed_num;
            if(i == 4){
                write_size[i][j] *= 1.48;
            }
            if(i == 15){
                write_size[i][j] *= 0.81;
            }
            if(i == 16){
                write_size[i][j] *= 0.78;
            }
            if(i == 10){
                write_size[i][j] *= 1.23;
            }
            if(i == 12){
                write_size[i][j] *= 1.12;
            }
            if(i == 5){
                write_size[i][j] *= 0.7;
            }
            if(i == 11){
                write_size[i][j] *= 0.98;
            }
            
            tot_write_size[i] += write_size[i][j];
        }
    }
    for (int i = 1; i <= M; i++) {//计算每个tag总共读取的大小
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            // scanf("%d",&read_size[i][j]);
            read_size[i][j] = fast_read();
            // if(i == 1){
            //     read_size[i][j] *= 1.3;
            // }
            // // if(i == 2){
            // //     read_size[i][j] *= 1.2;
            // // }
            // // if(i == 5){
            // //     read_size[i][j] *= 1.3;
            // // }
            // // if(i ==6){
            // //     read_size[i][j] *= 1.2;
            // // }
            // if(i ==11){
            //     read_size[i][j] *= 1.2;
            // }
            // if(i == 13){
            //     read_size[i][j] *= 1.1;
            // }
            // if(i == 15){
            //     read_size[i][j] *= 0.9;
            // }
            // if(i == 16){
            //     read_size[i][j] *= 1.1;
            // }
            tot_read_size[i] += read_size[i][j];
        }
    }

    for(int i=0;i<=N;i++){
        Disk disk;
        disk.id = i;
        disk.size = V;
        disks.push_back(disk);
    }
    cal_tag2order();
    // for(int i = 1; i <= M; i++){
    //     fprintf(file,"order:%d,tag:%d\n",i,order2tag[i]);
    // }
    // for(int i = 1; i <= M; i++){
    //     fprintf(file,"tag:%d,order:%d\n",i,tag2order[i]);
    // }
    // ////////fprintf(file,"disksize:%d\n",disks[3].size);
    divide_disks();
    // for(int i = 0; i <= M-1; i++){
    //     fprintf(file,"tag:%d,left:%d,right:%d\n",i+1,disks[1].divided_area[i].second.first,disks[1].divided_area[i].second.second);
    // }
    
    cal_bad_tags();
    for(int i = 1; i <= N; i++){
        read_head[i].disk_id = i;
        read_head[i].position = 1;
        read_head[i].leftg = G;
        read_head[i].target_position = 0;
    }
    
    for(int i = 1; i <= N; i++){
        read_request_timecut[i] = time_cut;
    }

    ////fprintf(file,"OK\n");
    printf("OK\n");
    fflush(stdout);
}


//---------------------------------preprocess---------------------------------
int main()//主函数  
{
    // file = fopen("output.txt","w");
    // //////////fprintf(file,"start\n");
    preprocess();
    
    for (int i = 1; i <= N; i++) {
        disk_point[i] = 1;
    }

    for (int t = 1; t <= EXTRA_TIME + T; t++) {
        
        
        current_stage = static_cast<int>(ceil(t / 1800.0));
        if(timestamp <= 75000){
            bad_tags_can_read_cut = 0.5 + current_stage * 0.35;
        }
        else{
            bad_tags_can_read_cut -= 0.2 / 1800;
        }
        if(disable_bad_tag){
            count_disable_time++;
        }
        if(count_disable_time >= (15 - 0.8 * current_stage)){
            disable_bad_tag = false;
            count_disable_time = 0;
        }
        init();
        
        // ////////fprintf(file,"init_leftg\n");
        timestamp_action();
        // ////////fprintf(file,"timestamp_action\n");
        delete_action();
        // ////////fprintf(file,"delete_action\n");
        write_action();
        
        // ////////fprintf(file,"write_action\n");
        // if(t==10953){
        //     fclose(file);
        // }
        read_action();
        // for(int i=1;i<N;i++){
        //     if(read_head[i].target_request != nullptr){
        //         //////////fprintf(file,"read_head[%d].position:%d\n",i,read_head[i].position);
        //     }
        // }
        // ////////fprintf(file,"read_action\n");
        // fclose(file);
    }
    // fprintf(file,"totread:%d\n",totread);
    // fprintf(file,"jump_num:%d\n",jump_num);
    // fprintf(file,"size1:%d,size2:%d,size3:%d,size4:%d,size5:%d\n",size1,size2,size3,size4,size5);
    // fclose(file);
    clean();

    return 0;
}
//---------------------------------main function---------------------------------