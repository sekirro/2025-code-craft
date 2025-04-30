#include<bits/stdc++.h>
using namespace std;

#define MAX_DISK_NUM (20 + 1)
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

#define mul1 0.9
#define mul2 1.2
#define mul3 0.9
#define mul4 1.0
#define mul5 0.7
#define mul6 0.8
#define mul7 1.0
#define mul8 0.7
#define mul9 1.0
#define mul10 0.65
#define mul11 1.05
#define mul12 1.0
#define mul13 0.9
#define mul14 0.65
#define mul15 1.1
#define mul16 1.0
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
    int saved_disk[3];
    int saving_state;//0.正常存 1.互插 2.group 3.allocate
    vector<int> saved_position[3];
} Object;

typedef struct Disk {
    int id;
    int size;
    int used_clean_area_size[MAX_TAG_NUM] = {0};
    int trash_area_size= 0;
    vector<pair<int,pair<int,int>>> divided_area;//初始分配的空间<tag,<left,right>>,这个area[0]是id为1的tag
    vector<pair<int,pair<int,int>>> allocated_area;//新分配的空间<tag,<left,right>>
    vector<int> saved_obj_position[MAX_TAG_NUM];
    int saved_object_id[MAX_DISK_SIZE] = {0};
    vector<read_Request> read_list[MAX_DISK_SIZE+5];
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
    int target_position;
    bool last_read = false;
    int last_cost = 0;
    string action;
    
} ReadHead;

ReadHead read_head[MAX_DISK_NUM];
vector<Disk> disks;
Object objects[MAX_OBJECT_NUM];
vector<read_Request> time2request[90000];
read_Request id2request[MAX_REQUEST_NUM];
// ----------------------------structure--------------------------------
int T, M, N, V, G, K;
int origin_N;
int timestamp = 0;
vector<read_Request> success_read_list;
vector<read_Request> abort_read_list;
FILE *file;

int del_size[MAX_TAG_NUM][MAX_TIME_SLICE];//第j个1800帧内tag为i的删除大小
int write_size[MAX_TAG_NUM][MAX_TIME_SLICE];//第j个1800帧内tag为i的写入大小
int accumulated_write_size[MAX_TAG_NUM][MAX_TIME_SLICE] = {0};//第j个1800帧前tag为i的累计写入大小
int accumulated_read_size[MAX_TAG_NUM] = {0};//第j个1800帧内为i的累计读取大小
int read_size[MAX_TAG_NUM][MAX_TIME_SLICE];//第j个1800帧内tag为i的读取大小
int tot_del_size[MAX_TAG_NUM] = { 0 };//每个tag的总删除大小
int tot_write_size[MAX_TAG_NUM] = { 0 };//每个tag的总写入大小
int tot_read_size[MAX_TAG_NUM] = { 0 };//每个tag的总读取大小
double standard[MAX_TIME_SLICE] = {0};

set<int> good_tags[2][MAX_TIME_SLICE];
int current_stage;
bool disable_bad_tag = false;
int count_disable_time = 0;
int bad_tags_can_read_cut = 0;
int adding_G[MAX_TIME_SLICE] = {0};
//--------------------------------global variable--------------------------------
int order2tag[MAX_TAG_NUM] = {0,14,9,6,12,1,11,8,5,3,7,15,13,10,2,16,4};
int tag2order[MAX_TAG_NUM] = {0};
set<int> tag_group[GROUP_NUM] ={{6,7},{1,3,4,5,6,7,8,9,10,11,12,13,14,15,16}};
int read_as_pass_cut = 10;
int time_cut = 105;
double min_read_times_of_all_tags = 0;

int bad_tag_request_count[MAX_TIME+105] = {0};
int good_tag_request_count[MAX_TIME+105] = {0};
int bad_tag_size[MAX_TIME+105] = {0};
int good_tag_size[MAX_TIME+105] = {0};

int border_tag[MAX_TIME_SLICE] = {0};
int border_line[MAX_TIME_SLICE] = {0};

// ----------------------------changeable variable--------------------------------
void init(){
    for(int i = 1; i <= N; i++){
        read_head[i].leftg = G;
    }
    success_read_list.clear();
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

void timestamp_action(){//时间戳{
    timestamp = fast_read();
    printf("TIMESTAMP %d\n", timestamp);
    // //fprintf(file,"TIMESTAMP %d\n", timestamp);
    fflush(stdout);
}

void cal_tag2order(){
    for(int i = 1; i <= M; i++){
        tag2order[order2tag[i]] = i;
    }
}

void dp_two_group(int current_stage,vector<int> temp_good_tags){
    int tot_write_size;
    for(int i = 0; i < temp_good_tags.size(); i++){
        tot_write_size += accumulated_write_size[temp_good_tags[i]][current_stage];
    }
    //fprintf(file,"tot_write_size:%d\n",tot_write_size);
    // fclose(file);
    bool dp[100000] = {false};
    int dp_last_size[100000] = {0};
    int dp_last_index[100000] = {0};
    dp[0] = true;
    for (int i = 0; i < temp_good_tags.size(); i++){
        for (int j = tot_write_size; j >= 0; j--){
            if(dp[j]){
                if(dp[j+accumulated_write_size[temp_good_tags[i]][current_stage]] == false){
                    dp[j+accumulated_write_size[temp_good_tags[i]][current_stage]] = true;
                    dp_last_size[j+accumulated_write_size[temp_good_tags[i]][current_stage]] = j;
                    dp_last_index[j+accumulated_write_size[temp_good_tags[i]][current_stage]] = temp_good_tags[i];
                }
            }
        }
    }
    // for(int i = 0; i <= tot_write_size; i++){
    //     //fprintf(file,"dp[%d]:%d ",i,dp[i]);
    // }
    // //fprintf(file,"\n");
    // fclose(file);
    int min_dis2half = 1e9;
    int min_dis = 1e9;
    // int min_dis2half_index = 0;
    for(int i = 0; i <= tot_write_size; i++){
        if(dp[i]){
            int temp = abs(tot_write_size - 2*i);
            if(temp < min_dis){
                min_dis2half = i;
                min_dis = temp;
                // min_dis2half_index = i;
            }
        }
    }
    //fprintf(file,"min_dis2half:%d\n",min_dis2half);
    while(min_dis2half != 0){
        good_tags[0][current_stage].insert(dp_last_index[min_dis2half]);
        min_dis2half = dp_last_size[min_dis2half];
    }
    for(int i = 1; i <= M; i++){
        if(find(temp_good_tags.begin(),temp_good_tags.end(),i) != temp_good_tags.end()){
            if(good_tags[0][current_stage].count(i) == 0){
                good_tags[1][current_stage].insert(i);
            }
        }
    }
}

pair<int,int> cal_border_line(vector<int> temp_good_tags,int stage){
    double best_score = 0.0;
    int best_border_tag = 0;
    int best_border_line = 0;
    int temp_border_tag = 0;
    int temp_border_line = 0;
    for(int i = 0;i < temp_good_tags.size();i++){
        // fprintf(file,"temp_good_tags[%d]:%d\n",i,temp_good_tags[i]);
        temp_border_tag = temp_good_tags[i];
        int last_tag_left = 1;
        int last_tag_right = 1;
        int cycle_G1 = 0;
        int cycle_G2 = 0;
        int current_left = 1;
        int current_right = 1;
        int good_read_size1 = 0;
        int good_read_size2 = 0;
        //计算cycle_G1：从第0个good_tag到border_tag左边界花费的tokens
        for(int j = 0;j <= i;j++){
            int min_saved_pos = 100000;
            int max_saved_pos = 0;
            for(int k = 0;k < disks[1].saved_obj_position[temp_good_tags[j]].size();k++){
                if(disks[1].saved_obj_position[temp_good_tags[j]][k] < min_saved_pos){
                    min_saved_pos = disks[1].saved_obj_position[temp_good_tags[j]][k];
                }
                if(disks[1].saved_obj_position[temp_good_tags[j]][k] > max_saved_pos){
                    max_saved_pos = disks[1].saved_obj_position[temp_good_tags[j]][k];
                }
            }
            current_left = max(disks[1].divided_area[temp_good_tags[j] - 1].second.first,min_saved_pos==100000?1:min_saved_pos);
            current_right = min(disks[1].divided_area[temp_good_tags[j] - 1].second.second,max_saved_pos==0?V:max_saved_pos);
            cycle_G1 += (current_left - last_tag_right) > G ? G : (current_left - last_tag_right);
            if(j != i){
                cycle_G1 += (current_right - current_left) * 16;
                good_read_size1 += read_size[temp_good_tags[j]][stage];
            }
            last_tag_left = current_left;
            last_tag_right = current_right; 
        }
        //计算cycle_G2：从border_tag右边界到最后一个good_tag花费的tokens
        for(int j = i + 1;j < temp_good_tags.size();j++){
            int min_saved_pos = 100000;
            int max_saved_pos = 0;
            for(int k = 0;k < disks[1].saved_obj_position[temp_good_tags[j]].size();k++){
                if(disks[1].saved_obj_position[temp_good_tags[j]][k] < min_saved_pos){
                    min_saved_pos = disks[1].saved_obj_position[temp_good_tags[j]][k];
                }
                if(disks[1].saved_obj_position[temp_good_tags[j]][k] > max_saved_pos){
                    max_saved_pos = disks[1].saved_obj_position[temp_good_tags[j]][k];
                }
            }
            current_left = max(disks[1].divided_area[temp_good_tags[j] - 1].second.first,min_saved_pos==100000?1:min_saved_pos);
            current_right = min(disks[1].divided_area[temp_good_tags[j] - 1].second.second,max_saved_pos==0?V:max_saved_pos);
            cycle_G2 += min(current_right - last_tag_left,G);
            if(j != i){
                cycle_G2 += (current_right - current_left) * 16;
            }
            good_read_size2 += read_size[temp_good_tags[j]][stage];
        }
        //重置回border_tag的左右边界
        int min_saved_pos = 100000;
        int max_saved_pos = 0;
        for(int k = 0;k < disks[1].saved_obj_position[temp_border_tag].size();k++){
            if(disks[1].saved_obj_position[temp_border_tag][k] < min_saved_pos){
                min_saved_pos = disks[1].saved_obj_position[temp_border_tag][k];
            }
            if(disks[1].saved_obj_position[temp_border_tag][k] > max_saved_pos){
                max_saved_pos = disks[1].saved_obj_position[temp_border_tag][k];
            }
        }
        // fprintf(file,"min_saved_pos:%d max_saved_pos:%d divided_area_left:%d divided_area_right:%d\n",min_saved_pos,max_saved_pos,disks[1].divided_area[temp_border_tag - 1].second.first,disks[1].divided_area[temp_border_tag - 1].second.second);
        current_left = max(disks[1].divided_area[temp_border_tag - 1].second.first,min_saved_pos==100000?1:min_saved_pos);
        current_right = min(disks[1].divided_area[temp_border_tag - 1].second.second,max_saved_pos==0?V:max_saved_pos);
        // fprintf(file,"current_left:%d current_right:%d\n",current_left,current_right);
        //遍历寻找最优的border_line
        for(int j = current_left;j <= current_right;j++){
            int temp_border_line = j;
            int temp_cycle_G1 = cycle_G1;
            int temp_cycle_G2 = cycle_G2;
            int temp_good_read_size1 = good_read_size1;
            int temp_good_read_size2 = good_read_size2;
            temp_cycle_G1 += (j - current_left) * 16;
            temp_cycle_G2 += (current_right - j) * 16;
            // fprintf(file,"temp_cycle_G1:%d temp_cycle_G2:%d\n",temp_cycle_G1,temp_cycle_G2);
            temp_good_read_size1 += read_size[temp_border_tag][stage] * (j-current_left)/(current_right-current_left);
            temp_good_read_size2 += read_size[temp_border_tag][stage] * (current_right-j)/(current_right-current_left);
            // fprintf(file,"temp_good_read_size1:%d temp_good_read_size2:%d\n",temp_good_read_size1,temp_good_read_size2);
            double avg_temp_cycle_score1 = (ceil(temp_cycle_G1 / G) * 0.5) * (-0.01) + 1;
            double avg_temp_cycle_score2 = (ceil(temp_cycle_G2 / G) * 0.5) * (-0.01) + 1;
            // fprintf(file,"avg_temp_cycle_score1:%f avg_temp_cycle_score2:%f\n",avg_temp_cycle_score1,avg_temp_cycle_score2);
            double temp_score = avg_temp_cycle_score1 * temp_good_read_size1 + avg_temp_cycle_score2 * temp_good_read_size2;
            // fprintf(file,"temp_score:%f\n",temp_score);
            if(temp_score > best_score){
                best_score = temp_score;
                best_border_tag = temp_border_tag;
                best_border_line = temp_border_line;
            }
        }
    }
    // fprintf(file,"best_score:%f,best_border_tag:%d,best_border_line:%d\n",best_score,best_border_tag,best_border_line);
    // fprintf(file,"best_score:%f\n",best_score);
    // fprintf(file,"best_border_tag:%d best_border_line:%d\n",best_border_tag,best_border_line);
    return mkp(best_border_tag,best_border_line);
}

void cal_good_tags(int i){
    double total_read_times[MAX_TIME_SLICE] = {0};
    vector<pair<int,int>> temp_good_tags[MAX_TIME_SLICE];

    for(int j = 1; j <= M; j++){
        total_read_times[i] += static_cast<double>(read_size[j][i])/ static_cast<double>(accumulated_write_size[j][i]);
    }
    
    if(i >= 7){
        min_read_times_of_all_tags = 0.06;
    }
    standard[i] = min_read_times_of_all_tags*total_read_times[i];
    for(int j = 1; j <= M; j++){
        if(static_cast<double>(read_size[j][i]) / static_cast<double>(accumulated_write_size[j][i]) >= standard[i]){
            temp_good_tags[i].push_back(mkp(j,tag2order[j]));
        }
    }
    // fprintf(file,"temp_good_tags.size:%d\n",temp_good_tags[i].size());
    sort(temp_good_tags[i].begin(),temp_good_tags[i].end(),[](pair<int,int> a,pair<int,int> b){
        return a.second < b.second;
    });
    vector<int> temp_vector;
    for(int j = 0; j < temp_good_tags[i].size(); j++){
        temp_vector.push_back(temp_good_tags[i][j].first);
    }
    pair<int,int> border_result = cal_border_line(temp_vector,i);
    bool is_filling_good_tags1 = true;
    // fprintf(file,"border_result.first:%d border_result.second:%d\n",border_result.first,border_result.second);
    for(int j = 0; j < temp_good_tags[i].size(); j++){
        // fprintf(file,"temp_good_tags[%d]:%d\n",j,temp_good_tags[i][j].first);
        if(temp_good_tags[i][j].first == border_result.first){
            border_line[i] = border_result.second;
            border_tag[i] = border_result.first;
            is_filling_good_tags1 = false;
            continue;
        }
        if(is_filling_good_tags1){
            good_tags[0][i].insert(temp_good_tags[i][j].first);
        }
        else{
            good_tags[1][i].insert(temp_good_tags[i][j].first);
        }
    }

    // fprintf(file,"current_stage:%d\n",i);
    // for(auto it = good_tags[0][i].begin(); it != good_tags[0][i].end(); it++){
    //     fprintf(file,"tag:%d\n",*it);
    // }
    // fprintf(file,"border_tag:%d border_line:%d\n",border_tag[i],border_line[i]);
    // for(auto it = good_tags[1][i].begin(); it != good_tags[1][i].end(); it++){
    //     fprintf(file,"tag:%d\n",*it);
    // }
}

void divide_disks() {//将所有磁盘按照tag的总写入大小加权划分
    int size[MAX_TAG_NUM];
    int tot_size = 0;
    vector<pair<int,pair<int,int>>> area;

    for(int i = 1;i<=M;i++){//计算所有tag的总写入大小
        tot_size += tot_write_size[i];
    }

    for(int i = 1; i <= M; i++){//计算每个tag分多少
        size[i] = (V / 3 * 10/9) * tot_write_size[i] / tot_size;
    }

    int left = 1;
    int right;
    pair<int,pair<int,int>> temp_area[MAX_TAG_NUM];
    for(int i = 1; i <= M; i++){//计算每个tag的区域
        right = left + size[order2tag[i]] - 1;
        temp_area[i] = mkp(order2tag[i],mkp(left,right));
        left = right + 1;
    }

    for(int i = 1; i <= origin_N; i++){
        for(int j = 1; j <= M; j++){
            disks[i].divided_area.push_back(temp_area[tag2order[j]]);
        }
    }
}

void do_object_delete(int id){//删除对象,目前扫盘，待修改，使用Object中的信息{
    for(int i=0;i<3;i++){
        for(int j=0;j<objects[id-1].saved_position[i].size();j++){
            disks[objects[id-1].saved_disk[i]].saved_object_id[objects[id-1].saved_position[i][j]] = 0;
        }
    }
}

void do_object_write(int id, int size, int tag, int save_disk_id[], vector<int> areas[]){//写入对象
    Object obj;
    obj.id = id;
    obj.size = size;
    obj.tag = tag;
    
    // 复制 saved_disk 数组
    for(int i = 0; i < 3; i++) {
        obj.saved_disk[i] = save_disk_id[i];
        // 复制该磁盘的所有区域信息
        obj.saved_position[i] = areas[i];
    }
    objects[id-1] = obj;

    for (int i = 0; i < 3; i++) {
        int disk_id = save_disk_id[i];
        for (int j = 0; j < areas[i].size(); j++) {
            for(int k = areas[i][j]; k <= areas[i][j] + size - 1; k++){
                disks[disk_id].saved_object_id[k] = id;
            }
        }
    }
}

void gc_update(int disk_id,vector<int> temp_position,int find_obj_id){
    // //fprintf(file,"temp_position_size:%d ,find_obj_id:%d ,obj_size:%d\n",temp_position.size(),find_obj_id,objects[find_obj_id-1].size);

    // for(int i = 0; i < objects[disks[disk_id].saved_object_id[to_position]-1].saved_position[0].size(); i++){
    //     if(objects[disks[disk_id].saved_object_id[from_position]-1].saved_position[0][i] == from_position){
    //         objects[disks[disk_id].saved_object_id[from_position]-1].saved_position[0][i] = to_position;
    //     }
    // }
    // if (read_head[disk_id].target_position == objects[find_obj_id-1].saved_position[0][0]){
    //     read_head[disk_id].target_position = 0;
    //     for (int i = 0; i < read_head[disk_id].read_list[objects[find_obj_id-1].saved_position[0][0]].size(); i++){
    //         abort_read_list.push_back(read_head[disk_id].read_list[objects[find_obj_id-1].saved_position[0][0]][i]);
    //     }
    // }

    //更新目标位置的object
    int found_id = disks[(disk_id-1)%origin_N+1].saved_object_id[temp_position[0]];
    if(found_id != 0){
        for(int i = 0; i < objects[find_obj_id-1].size; i++){
            objects[found_id-1].saved_position[0][i] = objects[find_obj_id-1].saved_position[0][i];
        }
        objects[found_id-1].saving_state = 0;
    }
    
    for(int i = 0; i < temp_position.size(); i++){
        int temp = disks[(disk_id-1)%origin_N+1].saved_object_id[temp_position[i]];
        disks[(disk_id-1)%origin_N+1].saved_object_id[temp_position[i]] = disks[(disk_id-1)%origin_N+1].saved_object_id[objects[find_obj_id-1].saved_position[0][i]];
        disks[(disk_id-1)%origin_N+1].saved_object_id[objects[find_obj_id-1].saved_position[0][i]] = temp;
    }

    vector<read_Request> temp_read_list;
    temp_read_list = disks[(disk_id-1)%origin_N+1].read_list[objects[find_obj_id-1].saved_position[0][0]];
    disks[(disk_id-1)%origin_N+1].read_list[objects[find_obj_id-1].saved_position[0][0]] = disks[(disk_id-1)%origin_N+1].read_list[temp_position[0]];
    disks[(disk_id-1)%origin_N+1].read_list[temp_position[0]] = temp_read_list;

    //更新被换的object
    for(int i = 0; i < temp_position.size(); i++){
        objects[find_obj_id-1].saved_position[0][i] = temp_position[i];
    }
    objects[find_obj_id-1].saving_state = 0;
    // //fprintf(file, "from_positon:%d to_position:%d\n", from_position, to_position);

    
}

void write_print(int id, int size, int tag, int save_disk_id[], vector<int> areas[]){
    printf("%d\n",id);
    ////fprintf(file,"id:%d\n",id);
    for(int i = 0; i < 3; i++){
        printf("%d ",(save_disk_id[i]-1)%10 + 1);
        ////fprintf(file,"disk_id:%d ",save_disk_id[i]);
        for(int j = 0; j < areas[i].size(); j++){

            // if(save_disk_id[i]>10){
            //     printf("%d ",areas[i][j]+V);
            // }
            // else{
            printf("%d ",areas[i][j]);
            // }
            ////fprintf(file,"area:%d ",areas[i][j]);
        }
        ////fprintf(file,"\n");
        printf("\n");
    }     
}

void jump(int id,int position ,MoveAction move){
    read_head[id].position = position;
    read_head[id].last_read = false;
    read_head[id].last_cost = 0;
    read_head[id].action += "j";
    read_head[id].action += " ";
    // if(id > 10){
    //     read_head[id].action += to_string(position+V);
    // }
    // else{
    read_head[id].action += to_string(position);
    // }
}

void pass(int id,MoveAction move){
    read_head[id].position = (read_head[id].position + 1) % V;
    read_head[id].last_read = false;
    read_head[id].last_cost = 0;
    read_head[id].action += "p";
}

void read_as_pass(int id,bool last_read,MoveAction move){
    read_head[id].position = (read_head[id].position + 1) % V;
    read_head[id].last_read = true;
    read_head[id].last_cost = move.gcost;
    read_head[id].action += "r";
}

void read(int id,bool last_read,int last_cost,MoveAction move){ 
    int now_position = read_head[id].position;
    int target_obj_position = objects[disks[(id-1)%origin_N+1].saved_object_id[now_position]-1].saved_position[0][0];
    if (now_position == target_obj_position){
        int size = disks[(id-1)%origin_N+1].read_list[target_obj_position].size();
        for (int i = 0; i < size; i++){
            if (disks[(id-1)%origin_N+1].read_list[target_obj_position][i].is_done == false && disks[(id-1)%origin_N+1].read_list[target_obj_position][i].is_abort == false){
                disks[(id-1)%origin_N+1].read_list[target_obj_position][i].is_read = true;
            }
        }
    }
    read_head[id].last_cost = move.gcost;
    read_head[id].last_read = true;
    int obj_id = disks[(id-1)%origin_N+1].saved_object_id[read_head[id].target_position];
    int obj_size = objects[obj_id-1].size;
    ////////////fprintf(file,"obj_id:%d,obj_size:%d\n",obj_id,obj_size);
    read_head[id].target_position+=1;

    if(read_head[id].position == objects[obj_id-1].saved_position[0][obj_size-1]){
        int position = objects[obj_id-1].saved_position[0][0];
        for (int i = 0; i < disks[(id-1)%origin_N+1].read_list[position].size(); i++) {
            if (disks[(id-1)%origin_N+1].read_list[position][i].is_read == true && disks[(id-1)%origin_N+1].read_list[position][i].is_abort == false && disks[(id-1)%origin_N+1].read_list[position][i].is_done == false){
                disks[(id-1)%origin_N+1].read_list[position][i].is_read = false;
                disks[(id-1)%origin_N+1].read_list[position][i].is_done = true;
                id2request[disks[(id-1)%origin_N+1].read_list[position][i].request_id].is_done = true;
                success_read_list.push_back(disks[(id-1)%origin_N+1].read_list[position][i]);
            }
        }
        if(disks[(id-1)%origin_N+1].read_list[position][disks[(id-1)%origin_N+1].read_list[position].size()-1].is_done == true){
            disks[(id-1)%origin_N+1].read_list[position].clear();
        }
        read_head[id].target_position = 0;
    }

    read_head[id].action += "r";
    read_head[id].position = (read_head[id].position + 1) % V;
}

void end(int id){
    read_head[id].action += "#";
}

bool read_as_pass_judge(int st,int ed,int id,bool last_read,int last_cost){
    if(!last_read){
        return false;
    }
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
        int size = disks[(id-1)%origin_N+1].read_list[i].size();
        
        bool flag1 = last_read_2 
        && size > 0 
        && disks[(id-1)%origin_N+1].read_list[i][size-1].st_time > timestamp - 105 
        && disks[(id-1)%origin_N+1].read_list[i][size-1].is_abort == false 
        && disks[(id-1)%origin_N+1].read_list[i][size-1].is_done == false;
        bool flag2 = !last_read_2
        && size > 0
        && disks[(id-1)%origin_N+1].read_list[i][size-1].st_time > timestamp - time_cut
        && disks[(id-1)%origin_N+1].read_list[i][size-1].is_abort == false
        && disks[(id-1)%origin_N+1].read_list[i][size-1].is_done == false 
        // && good_tags[(disk_id>10?1:0)][current_stage].count(objects[read_head[id].read_list[i][size-1].object_id-1].tag) == 1;
        && (good_tags[0][current_stage].count(objects[disks[(id-1)%origin_N+1].read_list[i][size-1].object_id-1].tag) != 0 || good_tags[1][current_stage].count(objects[disks[(id-1)%origin_N+1].read_list[i][size-1].object_id-1].tag) != 0||objects[disks[(id-1)%origin_N+1].read_list[i][size-1].object_id-1].tag == border_tag[current_stage]);
        if(flag1||flag2){
            is_read_id_2 = disks[(id-1)%origin_N+1].read_list[i][size-1].object_id;
        }
        if(disks[(id-1)%origin_N+1].saved_object_id[i] == is_read_id_2){
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
            move.gcost = cost_g;
            move.time = 0;
            return move;
        }
        else{
            MoveAction move;//not move
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
    else if(g<((ed-st)>0?(ed-st)%V:V+(ed-st)%V) && g>=G ){//如果jump优于pass，且剩余g大于G，则jump
        MoveAction move;//jump
        move.op = 2;
        move.gcost = G;
        move.time = 1;
        return move;
    }
    else{//剩下的所有情况都应该pass
        if(last_read){
            bool read_as_pass_or_not = read_as_pass_judge(st,ed,id,last_read,last_cost);
            if(read_as_pass_or_not && g >= cost_g){            
                MoveAction move;
                move.op = 4;
                move.gcost = cost_g;
                move.time = 0;
                return move;
            }
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
        move.time = 0;
        return move;
    }
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

// ----------------------------basic function--------------------------------
void delete_action()//删除对象
{
    int n_delete;
    int abort_num = 0;
    vector<int> abort_request_id;
    int obj_id[MAX_OBJECT_NUM];

    n_delete = fast_read();
    for (int i = 1; i <= n_delete; i++) {
        obj_id[i] = fast_read();
        do_object_delete(obj_id[i]); //同步obj和disk删除的状态
    }
    for (int i = 1; i <= n_delete; i++) {
        int disk_id = objects[obj_id[i]-1].saved_disk[0];
        for(int j = 0;j<objects[obj_id[i]-1].saved_position[0].size();j++){
            if(read_head[disk_id].target_position == objects[obj_id[i]-1].saved_position[0][j]){
                read_head[disk_id].target_position = 0;
                break;
            }
            if(read_head[disk_id+origin_N].target_position == objects[obj_id[i]-1].saved_position[0][j]){
                read_head[disk_id+origin_N].target_position = 0;
                break;
            }
        }
        int position = objects[obj_id[i]-1].saved_position[0][0];
        int size = disks[(disk_id-1)%origin_N+1].read_list[position].size();
        for (int j = 0; j < size; j++){
            if (disks[(disk_id-1)%origin_N+1].read_list[position][j].is_done == false && disks[(disk_id-1)%origin_N+1].read_list[position][j].is_abort == false){
                disks[(disk_id-1)%origin_N+1].read_list[position][j].is_abort = true;
                disks[(disk_id-1)%origin_N+1].read_list[position][j].is_read = false;
                id2request[disks[(disk_id-1)%origin_N+1].read_list[position][j].request_id].is_abort = true;
                abort_num++;
                abort_request_id.push_back(disks[(disk_id-1)%origin_N+1].read_list[position][j].request_id);
            }
        }
        disks[(disk_id-1)%origin_N+1].read_list[position].clear();

        
        disks[objects[obj_id[i]-1].saved_disk[0]].used_clean_area_size[objects[obj_id[i]-1].tag]-=objects[obj_id[i]-1].size;
        disks[objects[obj_id[i]-1].saved_disk[1]].trash_area_size-=objects[obj_id[i]-1].size;
        disks[objects[obj_id[i]-1].saved_disk[2]].trash_area_size-=objects[obj_id[i]-1].size;
    }

    printf("%d\n", abort_num);
    ////fprintf(file,"abort_num:%d\n",abort_num);
    for (int i = 0; i < abort_request_id.size(); i++) {
        printf("%d\n", abort_request_id[i]);
        /////fprintf(file,"%d\n", abort_request_id[i]);
    }

    fflush(stdout);
}

int find_big_clean_disk(int tag){//找used clean area最小的磁盘
    int min_clean_area_size = 1000000000;
    int min_clean_area_disk = 1;
    for(int i = 1; i <= origin_N; i++){
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

bool compare_by_blank_size1(pair<int,int> a, pair<int,int> b){
    return a.first == b.first ? a.second < b.second : a.first < b.first;
}

bool compare_by_position1(pair<int,int> a, pair<int,int> b){
    return a.second < b.second;
}

bool compare_by_blank_size2(pair<int,int> a, pair<int,int> b){
    return a.first == b.first ? a.second > b.second : a.first < b.first;
}

bool compare_by_position2(pair<int,int> a, pair<int,int> b){
    return a.second > b.second;
}

bool compare_by_size(Object a, Object b){
    return a.size > b.size;
}

void write_action(){//写入对象
    int n_write;
    int id[MAX_OBJECT_NUM];
    int size[MAX_OBJECT_NUM];
    int tag[MAX_OBJECT_NUM];
    n_write = fast_read();
    Object temp_obj[1000];
    //fprintf(file,"n_write:%d\n",n_write);
    // fclose(file);
    for (int i = 1; i <= n_write; i++) {
        temp_obj[i].id = fast_read();
        temp_obj[i].size = fast_read();
        temp_obj[i].tag = fast_read();
        //fprintf(file,"%d %d %d\n",temp_obj[i].id,temp_obj[i].size,temp_obj[i].tag);
    }
    // fclose(file);
    // sort(temp_obj+1,temp_obj+n_write+1,compare_by_size);
    for(int i = 1; i <= n_write; i++){
        id[i] = temp_obj[i].id;
        size[i] = temp_obj[i].size;
        tag[i] = temp_obj[i].tag;
        //fprintf(file,"%d %d %d\n",id[i],size[i],tag[i]);
    }
    // fclose(file);

    int chosen_disks[3];
    // auto now = std::chrono::system_clock::now();
    // auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    // fprintf(file,"write_action6666666前当前时间戳（毫秒）: :%d\n",time);
    for (int i = 1; i <= n_write; i++) {
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write_action444444444前当前时间戳（毫秒）: :%d\n",time);
        bool writed_successfully[3] = {false,false,false};//三个盘分别是否能够写入

        int chosen_disks[3];//选择磁盘
        chosen_disks[0] = find_big_clean_disk(tag[i]);
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write_action555555555前当前时间戳（毫秒）: :%d\n",time);
        vector<Disk> temp_disks;
        vector<pair<int, int>> disk_id_and_size;
        for(int j = 1; j <= origin_N; j++){
            if(disks[j].id % 10 != chosen_disks[0] % 10){
                // temp_disks.push_back(disks[j]);
                disk_id_and_size.push_back(mkp(disks[j].id,disks[j].trash_area_size));
            }
        }
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write_action77777写入本体前当前时间戳（毫秒）: :%d\n",time);
        int min_trash_area_size = 1000000000;
        for(int j = 0; j < disk_id_and_size.size(); j++){//从所有非主盘的磁盘中选择垃圾区最小的磁盘和区
            if(disk_id_and_size[j].second < min_trash_area_size){
                min_trash_area_size = disk_id_and_size[j].second;
                chosen_disks[1] = disk_id_and_size[j].first;
            }
        }
        
        disk_id_and_size.clear();
        for(int j = 1; j <= origin_N; j++){
            if(disks[j].id % 10 != chosen_disks[0] % 10 && disks[j].id % 10 != chosen_disks[1] % 10){
                disk_id_and_size.push_back(mkp(disks[j].id,disks[j].trash_area_size));
            }
        }
        min_trash_area_size = 1000000000;
        for(int j = 0; j < disk_id_and_size.size(); j++){
            if(disk_id_and_size[j].second < min_trash_area_size){
                min_trash_area_size = disk_id_and_size[j].second;
                chosen_disks[2] = disk_id_and_size[j].first;
            }
        }
       


        Object obj;
        obj.id = id[i];
        obj.size = size[i];
        obj.tag = tag[i];
        obj.saved_disk[0] = chosen_disks[0];
        obj.saved_disk[1] = chosen_disks[1];
        obj.saved_disk[2] = chosen_disks[2];

        //写入本体
        //写入本体到对应的tag分区
        //找到主盘中tag为tag[i]的分区
        int left = disks[chosen_disks[0]].divided_area[tag[i]-1].second.first;
        int right = disks[chosen_disks[0]].divided_area[tag[i]-1].second.second;
        int leftright = tag2order[tag[i]]>1?disks[chosen_disks[0]].divided_area[order2tag[tag2order[tag[i]]-1]-1].second.first:1;
        int rightleft = tag2order[tag[i]]<M?disks[chosen_disks[0]].divided_area[order2tag[tag2order[tag[i]]+1]-1].second.second:right;
        int cal = 0;
        

        if(tag2order[tag[i]] % 2 == 1){
            vector<pair<int,int>> temp_self_vector;
            vector<pair<int,int>> temp_other_vector;
            bool is_self = true;
            // //fprintf(file,"left:%d,rightleft:%d\n",left,rightleft);
            for(int j = left; j <= rightleft; j++){
                if(disks[chosen_disks[0]].saved_object_id[j] == 0){
                    cal++;
                }
                else{
                    if(is_self && cal >= size[i]){
                        temp_self_vector.push_back(mkp(cal,j));
                    }
                    else if(!is_self && cal >= size[i]){
                        temp_other_vector.push_back(mkp(cal,j));
                    }
                    if(objects[disks[chosen_disks[0]].saved_object_id[j]-1].tag != tag[i] && is_self){
                        is_self = false;
                    }
                    cal = 0;
                }
            }
            if(is_self && cal >= size[i]){
                temp_self_vector.push_back(mkp(cal,rightleft+1));
            }
            else if(!is_self && cal >= size[i]){
                temp_other_vector.push_back(mkp(cal,rightleft+1));
            }
            if(temp_self_vector.size() > 0 || temp_other_vector.size() > 0){
                sort(temp_self_vector.begin(),temp_self_vector.end(),compare_by_blank_size1);
                sort(temp_other_vector.begin(),temp_other_vector.end(),compare_by_position1);
                if(temp_self_vector.size() > 0){
                    int st = temp_self_vector[0].second - temp_self_vector[0].first;
                    // //fprintf(file,"obj_id:%d,st:%d\n",id[i],st);
                    writed_successfully[0] = true;//本体能够成功写入初始分区内
                    if(st+size[i]-1>right){
                        obj.saving_state = 1;
                    }
                    else{
                        obj.saving_state = 0;
                    }
                    for(int k = st; k <= st + size[i] - 1 ; k++){
                        disks[chosen_disks[0]].saved_object_id[k] = id[i];
                        obj.saved_position[0].push_back(k);
                    }
                    disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                }
                else{
                    int st = temp_other_vector[0].second - temp_other_vector[0].first;
                    writed_successfully[0] = true;//本体能够成功写入对面分区内
                    if(st+size[i]-1>right){
                        obj.saving_state = 1;
                    }
                    else{
                        obj.saving_state = 0;
                    }
                    for(int k = st; k <= st + size[i] - 1 ; k++){
                        disks[chosen_disks[0]].saved_object_id[k] = id[i];
                        obj.saved_position[0].push_back(k);
                    }
                    disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                }
            }
        }
        else{
            vector<pair<int,int>> temp_self_vector;
            vector<pair<int,int>> temp_other_vector;
            bool is_self = true;
            for(int j = right; j >= leftright; j--){
                if(disks[chosen_disks[0]].saved_object_id[j] == 0){
                    cal++;
                }
                else{
                    if(is_self && cal >= size[i]){
                        temp_self_vector.push_back(mkp(cal,j));
                    }
                    else if(!is_self && cal >= size[i]){
                        temp_other_vector.push_back(mkp(cal,j));
                    }
                    if(objects[disks[chosen_disks[0]].saved_object_id[j]-1].tag != tag[i] && is_self){
                        is_self = false;
                    }
                    cal = 0;
                }
            }
            if(is_self && cal >= size[i]){
                temp_self_vector.push_back(mkp(cal,leftright-1));
            }
            else if(!is_self && cal >= size[i]){
                temp_other_vector.push_back(mkp(cal,leftright-1));
            }
            if(temp_self_vector.size() > 0 || temp_other_vector.size() > 0){
                sort(temp_self_vector.begin(),temp_self_vector.end(),compare_by_blank_size2);
                sort(temp_other_vector.begin(),temp_other_vector.end(),compare_by_position2);
                if(temp_self_vector.size() > 0){
                    int st = temp_self_vector[0].second + temp_self_vector[0].first;
                    writed_successfully[0] = true;//本体能够成功写入初始分区内
                    if(st-size[i]+1<left){
                        obj.saving_state = 1;
                    }
                    else{
                        obj.saving_state = 0;
                    }
                    for(int k = st - size[i] + 1; k <= st ; k++){
                        disks[chosen_disks[0]].saved_object_id[k] = id[i];
                        obj.saved_position[0].push_back(k);
                    }
                    disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                }
                else{
                    int st = temp_other_vector[0].second + temp_other_vector[0].first;
                    writed_successfully[0] = true;//本体能够成功写入对面分区内
                    if(st-size[i]+1<left){
                        obj.saving_state = 1;
                    }
                    else{
                        obj.saving_state = 0;
                    }
                    for(int k = st - size[i] + 1; k <= st ; k++){
                        disks[chosen_disks[0]].saved_object_id[k] = id[i];
                        obj.saved_position[0].push_back(k);
                    }
                    disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                }
            }
        }
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write8888888后当前时间戳（毫秒）: :%d\n",time);
        if(!writed_successfully[0]){
            int group_index = 0;
            for(int j = 0; j < GROUP_NUM; j++){//找当前obj对应的group
                if(tag_group[j].count(tag[i]) > 0){
                    group_index = j;
                    break;
                }
            }

            for(int temp_tag : tag_group[group_index]){
                if(writed_successfully[0]){//写入成功后就跳出循环
                    break;
                }
                int left = disks[chosen_disks[0]].divided_area[temp_tag-1].second.first;
                int right = disks[chosen_disks[0]].divided_area[temp_tag-1].second.second;
                cal = 0;
                for(int j = left; j <= right; j++){
                    if(disks[chosen_disks[0]].saved_object_id[j] == 0){
                        cal++;
                    }
                    else{
                        cal = 0;
                    }
                    if(cal >= size[i]){
                        writed_successfully[0] = true;//本体能够成功写入初始分区内
                        obj.saving_state = 2;
                        for(int k = j-size[i]+1; k <= j ; k++){
                            disks[chosen_disks[0]].saved_object_id[k] = id[i];
                            obj.saved_position[0].push_back(k);
                        }
                        disks[chosen_disks[0]].used_clean_area_size[temp_tag] += size[i];
                        break;
                    }
                }
            }
        }
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write9999999后当前时间戳（毫秒）: :%d\n",time);
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
                                obj.saving_state = 3;
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
                                obj.saving_state = 3;
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
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write0000000后当前时间戳（毫秒）: :%d\n",time);
        if(!writed_successfully[0]&&disks[chosen_disks[0]].allocated_area.size()!=0){//如果依然写不下，新开一个allocated_area
            int idx = disks[chosen_disks[0]].allocated_area.size()-1;
            left = disks[chosen_disks[0]].allocated_area[idx].second.second + 1;
            right = left + ALLOCATE_SIZE - 1;
            if(right < V){//如果新的allocated_area的结束位置小于M，则创建新的allocated_area
                disks[chosen_disks[0]].allocated_area.push_back(mkp(tag[i],mkp(left,right)));
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
                                obj.saving_state = 3;
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
                                obj.saving_state = 3;
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
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write11111111后当前时间戳（毫秒）: :%d\n",time);
        for(int j = 1; j <= 2; j++){
            cal = 0;
            for(int k = V; k >= 1; k--){
                if(disks[chosen_disks[j]].saved_object_id[k] == 0){
                    cal++;
                    obj.saved_position[j].push_back(k);
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
            obj.saved_position[0].clear();
            cal = 0;
            for(int j = 1; j <= V; j++){
                if(disks[chosen_disks[0]].saved_object_id[j] == 0){
                    cal++;
                }
                else{
                    cal = 0;
                }
                if(cal >= size[i]){//能在allocated_area里写下
                    obj.saved_position[0].push_back(j);
                    disks[chosen_disks[0]].saved_object_id[j] = id[i];
                    disks[chosen_disks[0]].used_clean_area_size[tag[i]] += 1;
                    if(cal >= size[i]){
                        writed_successfully[0] = true;
                        obj.saving_state = 3;
                        for(int l = j - size[i] + 1; l <= j ; l++){//写入
                            disks[chosen_disks[0]].saved_object_id[l] = id[i];
                            obj.saved_position[0].push_back(l);
                        }
                        disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                        // area_found = true;
                        break;
                    }
                }
            }
        }
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write222222222222222后当前时间戳（毫秒）: :%d\n",time);
        for(int j = 0; j < size[i]; j++){
            int position = obj.saved_position[0][j];
            if(find(disks[chosen_disks[0]].saved_obj_position[tag[i]].begin(), disks[chosen_disks[0]].saved_obj_position[tag[i]].end(), position) == disks[chosen_disks[0]].saved_obj_position[tag[i]].end()){
                disks[chosen_disks[0]].saved_obj_position[tag[i]].push_back(position);
            }
        }
        objects[id[i]-1] = obj;
        write_print(id[i],size[i],tag[i],chosen_disks,obj.saved_position);
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write333333333333333后当前时间戳（毫秒）: :%d\n",time);
    }
    fflush(stdout);
}

void findTarget(int read_head_id){
    int position = read_head[read_head_id].position;

    for (int i = 1; i <= V; i++){
        int size = disks[(read_head_id-1)%origin_N+1].read_list[(i+position-2)%V+1].size();
        if(size > 0 
        && disks[(read_head_id-1)%origin_N+1].read_list[(i+position-2)%V+1][size-1].st_time > timestamp - 105
        && disks[(read_head_id-1)%origin_N+1].read_list[(i+position-2)%V+1][size-1].is_abort == false 
        && disks[(read_head_id-1)%origin_N+1].read_list[(i+position-2)%V+1][size-1].is_done == false
        && ((good_tags[(read_head_id>10)?1:0][current_stage].count(objects[disks[(read_head_id-1)%origin_N+1].saved_object_id[(i+position-2)%V+1] - 1].tag) != 0)
        || (objects[disks[(read_head_id-1)%origin_N+1].saved_object_id[(i+position-2)%V+1] - 1].tag == border_tag[current_stage] && (((read_head_id>10) && (i+position-2)%V+1 > border_line[current_stage])||((read_head_id<=10)&&((i+position-2)%V+1<=border_line[current_stage]))))))
        {
            read_head[read_head_id].target_position = objects[disks[(read_head_id-1)%origin_N+1].saved_object_id[(i+position-2)%V+1] - 1].saved_position[0][0];
            break;
        }
    }
}

void read_action(){//读取对象
    int n_read;
    int request_id, object_id;
    n_read = fast_read();
    ////fprintf(file,"n_read:%d\n",n_read);
    // cal_bad_tag_timely();
    for (int i = 1; i <= n_read; i++) {
        request_id = fast_read();
        object_id = fast_read();
        read_Request temp_request ;
        temp_request.request_id = request_id;
        temp_request.object_id = object_id;
        temp_request.is_done = false;
        temp_request.is_abort = false;
        temp_request.st_time = timestamp;
        id2request[request_id] = temp_request;
        Object obj = objects[temp_request.object_id-1];
        int disk_id = obj.saved_disk[0];
        accumulated_read_size[obj.tag] += obj.size;
        if(good_tags[0][current_stage].count(obj.tag) != 0 || good_tags[1][current_stage].count(obj.tag) != 0 || obj.tag == border_tag[current_stage]){
            disks[(disk_id-1)%origin_N+1].read_list[obj.saved_position[0][0]].push_back(temp_request);
            // read_head[disk_id+origin_N].read_list[obj.saved_position[0][0]].push_back(temp_request);
            time2request[timestamp].push_back(temp_request);
        }
        else{
            abort_read_list.push_back(temp_request);
            // if(timestamp <= MAX_TIME){
            //     for(int j = 0; j < 105; j++){
            //         bad_tag_request_count[timestamp + j]++;
            //     }
            // }
        }
    }
    
    for(int i = 1; i <= N; i++){
        int jump_flag = 0;
        int TIMER = 0;
        // if(timestamp == 1 && i > origin_N){
        //     jump_flag = 1;
        //     jump(i,1,MoveAction{2,G,1});
        //     read_head[i].leftg = 0;
        // }
        while(read_head[i].leftg > 0){
            if(read_head[i].target_position != 0){
                
                MoveAction move = cost_to_position(read_head[i].position,read_head[i].target_position,read_head[i].leftg,read_head[i].last_read,read_head[i].last_cost,i);
                read_head[i].leftg -= move.gcost;
                if(move.op == 0){
                    read(i,read_head[i].last_read,read_head[i].last_cost,move);
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
                
                if(read_head[i].target_position == 0){
                    read_head[i].leftg = 0;
                }
            }
        }
        if(jump_flag == 0){
            end(i);
        }
    }


    for(int i = 1; i <= origin_N; i++){
        printf("%s\n",read_head[i].action.c_str());
        printf("%s\n",read_head[i+origin_N].action.c_str());
    }
    printf("%d\n",success_read_list.size());
    for(int i = 0; i < success_read_list.size(); i++){
        printf("%d\n",success_read_list[i].request_id);
    }
    success_read_list.clear();
    if(timestamp % 1800 == 1){
        for(int i = 1; i <= N; i++){
            for(int j = 1; j <= V; j++){
                int obj_id = disks[(i-1)%origin_N+1].saved_object_id[j];
                if(obj_id != 0 && good_tags[0][current_stage].count(objects[obj_id-1].tag) == 0 && good_tags[1][current_stage].count(objects[obj_id-1].tag) == 0 && objects[obj_id-1].tag != border_tag[current_stage]){
                    for(int k = 0; k < disks[(i-1)%origin_N+1].read_list[j].size(); k++){
                        if(disks[(i-1)%origin_N+1].read_list[j][k].is_abort == false && disks[(i-1)%origin_N+1].read_list[j][k].is_done == false){
                            abort_read_list.push_back(disks[(i-1)%origin_N+1].read_list[j][k]);
                        }
                    }
                    disks[(i-1)%origin_N+1].read_list[j].clear();
                }
            }
        }
    }
    if (timestamp > 105) {
        for (int i = 0; i < time2request[timestamp-105].size(); i++){
            int disk_id = objects[time2request[timestamp-105][i].object_id-1].saved_disk[0];
            int position = objects[time2request[timestamp-105][i].object_id-1].saved_position[0][0];
            if (disks[(disk_id-1)%origin_N+1].read_list[position].size() > 0){
                for (int j = 0; j < disks[(disk_id-1)%origin_N+1].read_list[position].size(); j++){
                    if(disks[(disk_id-1)%origin_N+1].read_list[position][j].st_time <= timestamp - 105 && disks[(disk_id-1)%origin_N+1].read_list[position][j].is_abort == false && disks[(disk_id-1)%origin_N+1].read_list[position][j].is_done == false){
                        abort_read_list.push_back(disks[(disk_id-1)%origin_N+1].read_list[position][j]);
                        disks[(disk_id-1)%origin_N+1].read_list[position][j].is_abort = true;
                    }
                }
            }
            // if(id2request[time2request[timestamp-105][i].request_id].is_abort == false && id2request[time2request[timestamp-105][i].request_id].is_done == false){
            //     abort_read_list.push_back(time2request[timestamp-105][i]);
            // }
        }
    }
    printf("%d\n",abort_read_list.size());
    for(int i = 0; i < abort_read_list.size(); i++){
        printf("%d\n",abort_read_list[i].request_id);
    }
    abort_read_list.clear();
    fflush(stdout);
}

void gc_action()
{
    scanf("%*s %*s");
    vector<pair<int,int>> swap_position[MAX_DISK_NUM];
    int left_k[MAX_DISK_NUM];
    for(int i = 1; i <= origin_N; i++){
        left_k[i] = K ;
    }
    int time_slice = (timestamp - 1) / FRE_PER_SLICING + 1;
    vector<pair<int,int>> temp_order_to_tag;
    for(int i = 1; i <= M; i++){
        temp_order_to_tag.push_back(mkp(i,read_size[i][time_slice]));
    }
    sort(temp_order_to_tag.begin(),temp_order_to_tag.end(),[](const pair<int,int>& a, const pair<int,int>& b){
        return a.second > b.second;
    });
    for(int disk_i = 1; disk_i <= origin_N; disk_i++){
        for(int j = 3; j >= 0; j--){
            for(int temp_order = 0; temp_order < temp_order_to_tag.size(); temp_order++){
                for(int times = 1; times <= 1 ; times++){
                    int k = temp_order_to_tag[temp_order].first;
                    int i = disk_i;
                    if(times == 2){
                        i += origin_N;
                    }
                    for(int l = disks[(i-1)%origin_N+1].saved_obj_position[k].size()-1; l >= 0; l--){
                        int find_obj_id = 0;
                        if(disks[(i-1)%origin_N+1].saved_object_id[disks[(i-1)%origin_N+1].saved_obj_position[k][l]]-1 > 0
                        && objects[disks[(i-1)%origin_N+1].saved_object_id[disks[(i-1)%origin_N+1].saved_obj_position[k][l]]-1].saving_state == j 
                        && objects[disks[(i-1)%origin_N+1].saved_object_id[disks[(i-1)%origin_N+1].saved_obj_position[k][l]]-1].tag == k){//找符合条件的对象
                            find_obj_id = disks[(i-1)%origin_N+1].saved_object_id[disks[(i-1)%origin_N+1].saved_obj_position[k][l]];
                            if (find_obj_id != 0) {
                                for (int cal = 0; cal < disks[(i-1)%origin_N+1].read_list[objects[find_obj_id-1].saved_position[0][0]].size(); cal++){
                                    if(disks[(i-1)%origin_N+1].read_list[objects[find_obj_id-1].saved_position[0][0]][cal].is_read == true){
                                        find_obj_id = 0;
                                        break;
                                    }
                                }
                            }
                            if (find_obj_id != 0){
                                for (int cal = 0; cal < objects[find_obj_id-1].size; cal++){
                                    if(read_head[i].target_position == objects[find_obj_id-1].saved_position[0][cal]){
                                        find_obj_id = 0;
                                        break;
                                    }
                                    if (i <= 10) {
                                        if(read_head[i+origin_N].target_position == objects[find_obj_id-1].saved_position[0][cal]){
                                            find_obj_id = 0;
                                            break;
                                        }
                                    }
                                    if (i > 10) {
                                        if(read_head[i-origin_N].target_position == objects[find_obj_id-1].saved_position[0][cal]){
                                            find_obj_id = 0;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (find_obj_id != 0){
                                for (int cal = 0; cal < objects[find_obj_id-1].size; cal++){
                                    if (read_head[i].position == objects[find_obj_id-1].saved_position[0][cal]){
                                        find_obj_id = 0;
                                        break;
                                    }
                                    if (i <= 10) {
                                        if (read_head[i+origin_N].position == objects[find_obj_id-1].saved_position[0][cal]){
                                            find_obj_id = 0;
                                            break;
                                        }
                                    }
                                    if (i > 10) {
                                        if (read_head[i-origin_N].position == objects[find_obj_id-1].saved_position[0][cal]){
                                            find_obj_id = 0;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        if(find_obj_id != 0 && j != 1){//若不是互插，找空位
                            vector<int> temp_position;
                            for(int m = 0; m < l - objects[find_obj_id-1].size + 1; m++){
                                if(disks[(i-1)%origin_N+1].saved_object_id[disks[(i-1)%origin_N+1].saved_obj_position[k][m]] == 0){
                                    temp_position.push_back(disks[(i-1)%origin_N+1].saved_obj_position[k][m]);
                                }
                                else{
                                    temp_position.clear();
                                }

                                if(temp_position.size() >= objects[find_obj_id-1].size && left_k[(i-1)%origin_N+1] >= objects[find_obj_id-1].size){
                                    sort(temp_position.begin(),temp_position.end());

                                    bool flag = true;
                                    for(int idx = 1; idx < temp_position.size(); idx++){
                                        if(temp_position[idx] - temp_position[idx-1] > 1){
                                            flag = false;
                                            break;
                                        }
                                    }
                                    if(flag == false){
                                        temp_position.clear();
                                        continue;
                                    }
                                    

                                    left_k[(i-1)%origin_N+1]-=objects[find_obj_id-1].size;
                                    
                                    for(int n = 0; n < objects[find_obj_id-1].size; n++){
                                        swap_position[i].push_back(mkp(temp_position[n],objects[find_obj_id-1].saved_position[0][n]));
                                    }
                                    gc_update(i,temp_position,find_obj_id);
                                    break;
                                }
                            }
                        }
                        if(find_obj_id != 0 && j == 1){//若是互插，空位和对面的都可以找
                            // vector<int> pair_temp_position;
                            // if(tag2order[objects[find_obj_id-1].tag]%2==1){
                            //     int left = disks[i].divided_area[objects[find_obj_id-1].tag-1].second.first;
                            //     int right = objects[find_obj_id-1].saved_position[0][0];
                            //     for(int m = left; m <= right; m++){
                            //         int found_id = disks[i].saved_object_id[m];
                            //         if(found_id != 0){//不是空位
                            //             if(
                            //             abs(tag2order[objects[find_obj_id-1].tag] - tag2order[objects[found_id-1].tag]) == 1 
                            //             && max(tag2order[objects[found_id-1].tag], tag2order[objects[find_obj_id-1].tag]) % 2 == 0
                            //             && objects[found_id-1].size == objects[find_obj_id-1].size){//找到的obj和自己是一对，且大小相同
                            //                 //fprintf(file,"find_obj_id:%d found_id:%d\n",find_obj_id,found_id);
                            //                 for(int n = 0; n < objects[find_obj_id-1].size; n++){
                            //                     pair_temp_position.push_back(objects[found_id-1].saved_position[0][n]);
                            //                 }
                            //                 bool satisfy = true;
                            //                 for (int cal = 0; cal < read_head[i].read_list[objects[found_id-1].saved_position[0][0]].size(); cal++){
                            //                     if(read_head[i].read_list[objects[found_id-1].saved_position[0][0]][cal].is_read == true){
                            //                         satisfy = false;
                            //                         break;
                            //                     }
                            //                 }
                            //                 for (int cal = 0; cal < objects[found_id-1].size; cal++){
                            //                     if(read_head[i].target_position == objects[found_id-1].saved_position[0][cal]){
                            //                         satisfy = false;
                            //                         break;
                            //                     }
                            //                 }
                            //                 for (int cal = 0; cal < objects[found_id-1].size; cal++){
                            //                     if (read_head[i].position == objects[found_id-1].saved_position[0][cal]){
                            //                         satisfy = false;
                            //                         break;
                            //                     }
                            //                 }
                            //                 if(satisfy == true && left_k[(i-1)%origin_N+1] >= objects[find_obj_id-1].size){
                            //                     sort(pair_temp_position.begin(),pair_temp_position.end());
                            //                     left_k[(i-1)%origin_N+1]-=objects[find_obj_id-1].size;
                                                
                            //                     for(int n = 0; n < objects[find_obj_id-1].size; n++){
                            //                         swap_position[i].push_back(mkp(pair_temp_position[n],objects[find_obj_id-1].saved_position[0][n]));
                            //                     }
                            //                     gc_update(i,pair_temp_position,find_obj_id);
                            //                     break;
                            //                 }
                            //                 else{
                            //                     pair_temp_position.clear();
                            //                 }
                            //             }
                            //         }
                            //     }
                            // }
                            // else{
                            //     int left = objects[find_obj_id-1].saved_position[0][objects[find_obj_id-1].size-1];
                            //     int right = disks[i].divided_area[objects[find_obj_id-1].tag-1].second.second;
                            //     for(int m = right; m >= left; m--){
                            //         int found_id = disks[i].saved_object_id[m];
                            //         if(found_id != 0){//不是空位
                            //             if(
                            //             abs(tag2order[objects[find_obj_id-1].tag] - tag2order[objects[found_id-1].tag]) == 1 
                            //             && max(tag2order[objects[found_id-1].tag], tag2order[objects[find_obj_id-1].tag]) % 2 == 0
                            //             && objects[found_id-1].size == objects[find_obj_id-1].size){//找到的obj和自己是一对，且大小相同
                            //                 //fprintf(file,"find_obj_id:%d found_id:%d\n",find_obj_id,found_id);
                            //                 for(int n = 0; n < objects[find_obj_id-1].size; n++){
                            //                     pair_temp_position.push_back(objects[found_id-1].saved_position[0][n]);
                            //                 }
                            //                 bool satisfy = true;
                            //                 for (int cal = 0; cal < read_head[i].read_list[objects[found_id-1].saved_position[0][0]].size(); cal++){
                            //                     if(read_head[i].read_list[objects[found_id-1].saved_position[0][0]][cal].is_read == true){
                            //                         satisfy = false;
                            //                         break;
                            //                     }
                            //                 }
                            //                 for (int cal = 0; cal < objects[found_id-1].size; cal++){
                            //                     if(read_head[i].target_position == objects[found_id-1].saved_position[0][cal]){
                            //                         satisfy = false;
                            //                         break;
                            //                     }
                            //                 }
                            //                 for (int cal = 0; cal < objects[found_id-1].size; cal++){
                            //                     if (read_head[i].position == objects[found_id-1].saved_position[0][cal]){
                            //                         satisfy = false;
                            //                         break;
                            //                     }
                            //                 }
                            //                 if(satisfy == true && left_k[(i-1)%origin_N+1] >= objects[find_obj_id-1].size){
                            //                     sort(pair_temp_position.begin(),pair_temp_position.end());
                            //                     left_k[(i-1)%origin_N+1]-=objects[find_obj_id-1].size;
                                                
                            //                     for(int n = 0; n < objects[find_obj_id-1].size; n++){
                            //                         swap_position[i].push_back(mkp(pair_temp_position[n],objects[find_obj_id-1].saved_position[0][n]));
                            //                     }
                            //                     gc_update(i,pair_temp_position,find_obj_id);
                            //                     break;
                            //                 }
                            //                 else{
                            //                     pair_temp_position.clear();
                            //                 }
                            //             }
                            //         }
                            //     }
                            // }

                            vector<int> blank_temp_position;
                            // if(pair_temp_position.size() == 0)
                            // {
                                for(int m = 0; m < l - objects[find_obj_id-1].size + 1; m++){
                                    int found_id = disks[(i-1)%origin_N+1].saved_object_id[disks[(i-1)%origin_N+1].saved_obj_position[k][m]];
                                    
                                    if(found_id == 0){//空位
                                        blank_temp_position.push_back(disks[(i-1)%origin_N+1].saved_obj_position[k][m]);
                                    }
                                    else{
                                        blank_temp_position.clear();
                                    }

                                    if(blank_temp_position.size() >= objects[find_obj_id-1].size && left_k[(i-1)%origin_N+1] >= objects[find_obj_id-1].size){
                                        sort(blank_temp_position.begin(),blank_temp_position.end());

                                        bool flag = true;
                                        for(int idx = 1; idx < blank_temp_position.size(); idx++){//不连续就不行
                                            if(blank_temp_position[idx] - blank_temp_position[idx-1] > 1){
                                                flag = false;
                                                break;
                                            }
                                        }
                                        if(flag == false){
                                            blank_temp_position.clear();
                                            continue;
                                        }
                                        

                                        left_k[(i-1)%origin_N+1]-=objects[find_obj_id-1].size;
                                        
                                        for(int n = 0; n < objects[find_obj_id-1].size; n++){
                                            swap_position[i].push_back(mkp(blank_temp_position[n],objects[find_obj_id-1].saved_position[0][n]));
                                        }
                                        gc_update(i,blank_temp_position,find_obj_id);
                                        break;
                                    }
                                }
                            // }
                        }
                    }
                }
            }
        }
    }

    printf("GARBAGE COLLECTION\n");
    for (int i = 1; i <= origin_N; i++) {
        printf("%d\n", swap_position[i].size()+swap_position[i+10].size());
        // //fprintf(file, "size:%d\n", swap_position[i].size()+swap_position[i+10].size());
        for (int j = 0; j < swap_position[i].size(); j++){
            printf("%d %d\n",swap_position[i][j].first,swap_position[i][j].second);
            // //fprintf(file, "position:%d %d\n",swap_position[i][j].first,swap_position[i][j].second);
        }
        swap_position[i].clear();
        for (int j = 0; j < swap_position[i+10].size(); j++){
            printf("%d %d\n",swap_position[i+10][j].first,swap_position[i+10][j].second);
            // //fprintf(file, "position:%d %d\n",swap_position[i+10][j].first+V,swap_position[i+10][j].second+V);
        }
        swap_position[i+10].clear();
    }
    
    fflush(stdout);
}


//---------------------------------complex function---------------------------------
void preprocess(){
    T = fast_read();
    M = fast_read();
    origin_N = fast_read();
    N = origin_N*2;
    V = fast_read();
    G = fast_read();
    K = fast_read();

    for (int i = 1; i <= M; i++) {//计算每个tag总共删除的大小
        int deled_num = 0;
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            del_size[i][j] = fast_read();
            deled_num += del_size[i][j];
            accumulated_write_size[i][j] -= deled_num;
            tot_del_size[i] += del_size[i][j];
        }
    }

    for (int i = 1; i <= M; i++) {//计算每个tag总共写入的大小
        int writed_num = 0;
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            write_size[i][j] = fast_read();
            writed_num += write_size[i][j];
            accumulated_write_size[i][j] += writed_num;
            if(i == 1){
                write_size[i][j] *= mul1;
            }
            if(i == 2){
                write_size[i][j] *= mul2;
            }
            if(i == 3){
                write_size[i][j] *= mul3;
            }
            if(i == 4){
                write_size[i][j] *= mul4;
            }
            if(i == 5){
                write_size[i][j] *= mul5;
            }
            if(i == 6){
                write_size[i][j] *= mul6;
            }
            if(i == 7){
                write_size[i][j] *= mul7;
            }
            if(i == 8){
                write_size[i][j] *= mul8;
            }
            if(i == 9){
                write_size[i][j] *= mul9;
            }
            if(i == 10){
                write_size[i][j] *= mul10;
            }
            if(i == 11){
                write_size[i][j] *= mul11;
            }
            if(i == 12){
                write_size[i][j] *= mul12;
            }
            if(i == 13){
                write_size[i][j] *= mul13;
            }
            if(i == 14){
                write_size[i][j] *= mul14;
            }
            if(i == 15){
                write_size[i][j] *= mul15;
            }
            if(i == 16){
                write_size[i][j] *= mul16;
            }

            tot_write_size[i] += write_size[i][j];
        }
    }
    for (int i = 1; i <= M; i++) {//计算每个tag总共读取的大小
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            read_size[i][j] = fast_read();
            tot_read_size[i] += read_size[i][j];
        }
    }
    for(int i = 1; i <= (T - 1) / FRE_PER_SLICING + 2 ; i++){
        adding_G[i] = fast_read();
    }
    for(int i=0;i<=N;i++){
        Disk disk;
        disk.id = i;
        disk.size = V;
        disks.push_back(disk);
    }
    cal_tag2order();
    divide_disks();
    // cal_good_tags();
    for(int i = 1; i <= N; i++){
        read_head[i].disk_id = i;
        read_head[i].position = 1;
        read_head[i].leftg = G;
        read_head[i].target_position = 0;
    } 
    ////fprintf(file,"OK\n");
    printf("OK\n");
    fflush(stdout);
}

// void count_tag_size(){
//     for(int i = 1; i <= N; i++){
//         for(int j = 1;j <= V;j++){
//             if(disks[(i-1)%origin_N+1].saved_object_id[j] != 0){
//                 if(good_tags[0][current_stage].count(objects[disks[(i-1)%origin_N+1].saved_object_id[j]-1].tag) != 0 || good_tags[1][current_stage].count(objects[disks[(i-1)%origin_N+1].saved_object_id[j]-1].tag) != 0){
//                     good_tag_size[timestamp] += objects[disks[(i-1)%origin_N+1].saved_object_id[j]-1].size;
//                     for(int k = 0; k < read_head[i].read_list[j].size(); k++){
//                         if(read_head[i].read_list[j][k].is_done != true && read_head[i].read_list[j][k].is_abort != true){//每一帧goodtag的读取请求数
//                             good_tag_request_count[timestamp]++;
//                         }
//                     }
//                 }
//                 else{
//                     bad_tag_size[timestamp] += objects[disks[(i-1)%origin_N+1].saved_object_id[j]-1].size;
//                 }
//             }
//         }
//     }
// }
void print_density(){
    for(int i = 1; i <= MAX_TIME; i++){
        double bad_density = (double)bad_tag_request_count[i] / bad_tag_size[i] ;
        double good_density = (double)good_tag_request_count[i] / good_tag_size[i];
        // //fprintf(file,"timestamp:%d bad_density:%f good_density:%f\n",i,bad_density,good_density);
    }
}
//---------------------------------preprocess---------------------------------
int main()//主函数  
{
    
    // file = fopen("output.txt","w");
    preprocess();
    int origin_G = G;
    for (int t = 1; t <= EXTRA_TIME + T; t++) {
        current_stage = static_cast<int>(ceil(t / 1800.0));
        if(t % 1800 == 1){
            G = origin_G + adding_G[current_stage];
        }
        current_stage = static_cast<int>(ceil(t / 1800.0));
        if(t % 1800 == 1){
            cal_good_tags(current_stage);
        }
        ////fprintf(file,"timestamp:%d\n",timestamp);
        // fclose(file);
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

        
        timestamp_action();
        delete_action();
        // auto now = std::chrono::system_clock::now();
        // auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"delete_action后当前时间戳（毫秒）: :%d\n",time);
        
        write_action();
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"write后当前时间戳（毫秒）: :%d\n",time);
        read_action();
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // fprintf(file,"read后当前时间戳（毫秒）: :%d\n",time);

        // count_tag_size();

        if (t % FRE_PER_SLICING == 0) {
            gc_action();
        }
        
        // fclose(file);
    }
    // fclose(file);
    // print_density();
    return 0;
}
//---------------------------------main function---------------------------------