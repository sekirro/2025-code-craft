#include<bits/stdc++.h>
using namespace std;

#define MAX_DISK_NUM (20 + 5)
#define MAX_DISK_SIZE (16384 + 5)
#define MAX_REQUEST_NUM (30000000 + 5)
#define MAX_OBJECT_NUM (100000 + 5)
#define MAX_TAG_NUM (16 + 5)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define MAX_TIME (90000)
#define MAX_TIME_SLICE (MAX_TIME / FRE_PER_SLICING + 2)
#define EXTRA_TIME (105)
#define mkp make_pair
#define ALLOCATE_SIZE (5)
#define GROUP_NUM (16)

#define mul1 1.0
#define mul2 1.0
#define mul3 1.0
#define mul4 1.0
#define mul5 1.0
#define mul6 1.0
#define mul7 1.0
#define mul8 1.0
#define mul9 1.0
#define mul10 1.0
#define mul11 1.0
#define mul12 1.0
#define mul13 1.0
#define mul14 1.0
#define mul15 1.0
#define mul16 1.0
#define cluster_stage_length 1800
#define final_sword 50
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
    int temp_tag = 0;
    int size;
    int saved_disk[3];
    int saving_state;//0.正常存 1.互插 2.group 3.allocate
    vector<int> saved_position[3];
    double read_embedding[MAX_TIME/cluster_stage_length] = {0};
    int store_timestamp = 0;
    int delete_timestamp = 0;
    int in_tag_area = 0;
    int read_num = 0;
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

typedef struct DeleteRecord{
    int n_delete;
    vector<int> object_ids;
} DeleteRecord;

typedef struct WriteRecord{
    int n_write;
    vector<int> object_ids;
    vector<int> object_sizes;
    vector<int> object_tags;
} WriteRecord;

typedef struct ReadRecord{
    int n_read;
    vector<int> request_ids;
    vector<int> object_ids;
} ReadRecord;

ReadHead read_head[MAX_DISK_NUM];
vector<Disk> disks;
Object objects[MAX_OBJECT_NUM];
vector<read_Request> time2request[90000];
read_Request id2request[MAX_REQUEST_NUM];
DeleteRecord delete_record[MAX_TIME];
WriteRecord write_record[MAX_TIME];
ReadRecord read_record[MAX_TIME];
// ----------------------------structure--------------------------------
int T, M, N, V, G, K, K1, K2;
int origin_N;
int timestamp = 0;
vector<read_Request> success_read_list;
vector<read_Request> abort_read_list;
FILE *file;

int del_size[MAX_TAG_NUM][MAX_TIME_SLICE];//第j个1800帧内tag为i的删除大小
int write_size[MAX_TAG_NUM][MAX_TIME_SLICE];//第j个1800帧内tag为i的写入大
int accumulated_write_size[MAX_TAG_NUM][MAX_TIME_SLICE] = {0};//第j个1800帧前tag为i的累计写入大小
int accumulated_write_size_without_delete[MAX_TAG_NUM][MAX_TIME_SLICE] = {0};//第j个1800帧前tag为i的累计写入大小（不考虑删除）
int accumulated_read_size[MAX_TAG_NUM] = {0};//第j个1800帧内为i的累计读取大小
int round1_accumulated_write_size[MAX_TAG_NUM] = {0};//第j个1800帧内为i的累计读取大小
int read_size[MAX_TAG_NUM][MAX_TIME_SLICE];//第j个1800帧内tag为i的读取大小
int tot_del_size[MAX_TAG_NUM] = { 0 };//每个tag的总删除大小
int tot_write_size[MAX_TAG_NUM] = { 0 };//每个tag的总写入大小
int tot_read_size[MAX_TAG_NUM] = { 0 };//每个tag的总读取大小
int tag0_object_write_size[MAX_TAG_NUM] = {0};
int tag0_object_read_size[MAX_TIME_SLICE] = {0};
double standard[MAX_TIME_SLICE] = {0};

set<int> good_tags[2][MAX_TIME_SLICE];
int current_stage;
bool disable_bad_tag = false;
int count_disable_time = 0;
int bad_tags_can_read_cut = 0;
int adding_G[MAX_TIME_SLICE] = {0};

double cluster_core[MAX_TAG_NUM][MAX_TIME/cluster_stage_length] = {0};
vector<int> cluster_core_obj_id[MAX_TAG_NUM];
bool read_tag0[MAX_TIME_SLICE] = {false};
int read_tag0_line[MAX_TIME_SLICE] = {0};
int round1_tags_read_num[MAX_TIME][MAX_TAG_NUM] = {0};
//--------------------------------global variable--------------------------------
int order2tag[MAX_TAG_NUM+1] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,0};
int tag2order[MAX_TAG_NUM+1] = {0};
set<int> tag_group[GROUP_NUM] ={{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}};
int read_as_pass_cut = 10;
int time_cut = 105;
double min_read_times_of_all_tags = 0;

int bad_tag_request_count[MAX_TIME+105] = {0};
int good_tag_request_count[MAX_TIME+105] = {0};
int bad_tag_size[MAX_TIME+105] = {0};
int good_tag_size[MAX_TIME+105] = {0};

int border_tag[MAX_TIME_SLICE] = {0};
int border_line[MAX_TIME_SLICE] = {0};

double normalized_read_size[MAX_TAG_NUM][MAX_TIME_SLICE];
double normalized_delete_size[MAX_TAG_NUM][MAX_TIME_SLICE];

int obj_num = 0;

double ozd_weight[MAX_TAG_NUM] = {0.86,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};


// ----------------------------changeable variable--------------------------------
void init1(){
    for(int i = 1; i <= N; i++){
        read_head[i].leftg = G;
    }
    success_read_list.clear();
    for(int i = 1; i <= N; i++){
        read_head[i].action = "";
    }
}

void init2(){
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
    // // fprintf(file,"TIMESTAMP %d\n", timestamp);
    fflush(stdout);
}

void cal_tag2order(){
    for(int i = 1; i <= M + 1; i++){
        tag2order[order2tag[i]] = i;
    }
}

pair<double,pair<int,int>> cal_border_line(vector<int> temp_good_tags,int stage){
    double best_score = 0.0;
    int best_border_tag = 0;
    int best_border_line = 0;
    int temp_border_tag = 0;
    int temp_border_line = 0;
    double best_avg_cycle_score1 = 0.0;
    double best_avg_cycle_score2 = 0.0;
    int best_read_size1 = 0;
    int best_read_size2 = 0;
    for(int i = 0;i < temp_good_tags.size();i++){
        // // fprintf(file,"temp_good_tags[%d]:%d\n",i,temp_good_tags[i]);
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
            current_left = max(disks[1].divided_area[temp_good_tags[j] ].second.first,min_saved_pos==100000?1:min_saved_pos);
            current_right = min(disks[1].divided_area[temp_good_tags[j] ].second.second,max_saved_pos==0?V:max_saved_pos);
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
            current_left = max(disks[1].divided_area[temp_good_tags[j] ].second.first,min_saved_pos==100000?1:min_saved_pos);
            current_right = min(disks[1].divided_area[temp_good_tags[j] ].second.second,max_saved_pos==0?V:max_saved_pos);
            cycle_G2 += min(current_right - last_tag_left,G);
            if(j != i){
                // cycle_G2 += (current_right - current_left) * 16 * ozd_weight[temp_good_tags[j]];
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
        // // fprintf(file,"min_saved_pos:%d max_saved_pos:%d divided_area_left:%d divided_area_right:%d\n",min_saved_pos,max_saved_pos,disks[1].divided_area[temp_border_tag - 1].second.first,disks[1].divided_area[temp_border_tag - 1].second.second);
        current_left = max(disks[1].divided_area[temp_border_tag ].second.first,min_saved_pos==100000?1:min_saved_pos);
        current_right = min(disks[1].divided_area[temp_border_tag ].second.second,max_saved_pos==0?V:max_saved_pos);
        // fprintf(file,"border_tag:%d cycle_G1:%d cycle_G2:%d\n",temp_border_tag,cycle_G1,cycle_G2);
        // // fprintf(file,"temp_border_tag:%d current_left:%d current_right:%d\n",temp_border_tag,current_left,current_right);
        //遍历寻找最优的border_line
        for(int j = current_left;j <= current_right;j++){
            int temp_border_line = j;
            int temp_cycle_G1 = cycle_G1;
            int temp_cycle_G2 = cycle_G2;
            int temp_good_read_size1 = good_read_size1;
            int temp_good_read_size2 = good_read_size2;
            // temp_cycle_G1 += (j - current_left) * 16 * ozd_weight[temp_border_tag];
            // temp_cycle_G2 += (current_right - j) * 16 * ozd_weight[temp_border_tag];
            temp_cycle_G1 += (j - current_left) * 16;
            temp_cycle_G2 += (current_right - j) * 16;
            // // fprintf(file,"temp_cycle_G1:%d temp_cycle_G2:%d\n",temp_cycle_G1,temp_cycle_G2);
            temp_good_read_size1 += read_size[temp_border_tag][stage] * (j-current_left)/(current_right-current_left);
            temp_good_read_size2 += read_size[temp_border_tag][stage] * (current_right-j)/(current_right-current_left);
            // fprintf(file,"temp_good_read_size1:%d temp_good_read_size2:%d\n",temp_good_read_size1,temp_good_read_size2);
            double avg_temp_cycle_score1 = (ceil(temp_cycle_G1 / G) * 0.5) * (-0.01) + 1;
            // if(ceil(temp_cycle_G1 / G) >105){
            //     avg_temp_cycle_score1 = -1;
            // }
            double avg_temp_cycle_score2 = (ceil(temp_cycle_G2 / G) * 0.5) * (-0.01) + 1;
            // if(ceil(temp_cycle_G2 / G) >105){
            //     avg_temp_cycle_score2 = -1;
            // }
            // // fprintf(file,"avg_temp_cycle_score1:%f avg_temp_cycle_score2:%f\n",avg_temp_cycle_score1,avg_temp_cycle_score2);
            double temp_score = avg_temp_cycle_score1 * temp_good_read_size1 + avg_temp_cycle_score2 * temp_good_read_size2;
            // // fprintf(file,"temp_score:%f\n",temp_score);
            if(temp_score > best_score){
                // // fprintf(file,"cycle_G1:%d cycle_G2:%d\n",cycle_G1,cycle_G2);
                // // fprintf(file,"avg_temp_cycle_score1:%f avg_temp_cycle_score2:%f\n",avg_temp_cycle_score1,avg_temp_cycle_score2);
                // // fprintf(file,"temp_good_read_size1:%d temp_good_read_size2:%d\n",temp_good_read_size1,temp_good_read_size2);
                // // fprintf(file,"temp_score:%f\n",temp_score);
                best_avg_cycle_score1 = avg_temp_cycle_score1;
                best_avg_cycle_score2 = avg_temp_cycle_score2;
                best_score = temp_score;
                best_border_tag = temp_border_tag;
                best_border_line = temp_border_line;
                best_read_size1 = temp_good_read_size1;
                best_read_size2 = temp_good_read_size2;
            }
        }
    }
    // // fprintf(file,"\n");
    // // fprintf(file,"best_score:%f,best_border_tag:%d,best_border_line:%d\n",best_score,best_border_tag,best_border_line);
    // // fprintf(file,"best_score:%f\n",best_score);
    // // fprintf(file,"best_border_tag:%d best_border_line:%d\n",best_border_tag,best_border_line);
    // fprintf(file,"best_score:%f best_border_tag:%d best_border_line:%d best_avg_cycle_score1:%f best_avg_cycle_score2:%f read_size1:%d read_size2:%d\n",best_score,best_border_tag,best_border_line,best_avg_cycle_score1,best_avg_cycle_score2,best_read_size1,best_read_size2);
    return mkp(best_score,mkp(best_border_tag,best_border_line));
}

pair<double,pair<int,int>> cal_border_line_with_tag0(vector<int> temp_good_tags,int stage,int tag0_line){
    double best_score = 0.0;
    int best_border_tag = 0;
    int best_border_line = 0;
    int temp_border_tag = 0;
    int temp_border_line = 0;

    //构造新的tag取代tag0
    pair<int,pair<int,int>> temp_divided_area[MAX_TAG_NUM];
    int temp_read_size[MAX_TAG_NUM];
    for(int i = 0;i <= M;i++){
        temp_divided_area[i] = disks[1].divided_area[i];
        temp_read_size[i] = read_size[i][stage];
    }
    int temp_temp_left = 100000;
    int temp_temp_right = 0;
    for(int i=0;i<disks[1].saved_obj_position[0].size();i++){
        if(disks[1].saved_obj_position[0][i] < temp_temp_left){
            temp_temp_left = disks[1].saved_obj_position[0][i];
        }
        if(disks[1].saved_obj_position[0][i] > temp_temp_right){
            temp_temp_right = disks[1].saved_obj_position[0][i];
        }
    }
    temp_temp_left = max(temp_divided_area[0].second.first,temp_temp_left==100000?1:temp_temp_left);
    temp_temp_right = min(temp_divided_area[0].second.second,temp_temp_right==0?V:temp_temp_right);
    // temp_good_tags.push_back(mid_border_tag);
    temp_divided_area[0] = mkp(0,mkp(temp_temp_left,tag0_line));
    temp_read_size[0] = read_size[0][stage] * (tag0_line - temp_temp_left) / (temp_temp_right - temp_temp_left);
    
    for(int i = 0;i < temp_good_tags.size();i++){
        // // fprintf(file,"temp_good_tags[%d]:%d\n",i,temp_good_tags[i]);
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
            current_left = max(temp_divided_area[temp_good_tags[j]].second.first,min_saved_pos==100000?1:min_saved_pos);
            current_right = min(temp_divided_area[temp_good_tags[j] ].second.second,max_saved_pos==0?V:max_saved_pos);
            cycle_G1 += (current_left - last_tag_right) > G ? G : (current_left - last_tag_right);
            if(j != i){
                // cycle_G1 += (current_right - current_left) * 16 * ozd_weight[temp_good_tags[j]];
                cycle_G1 += (current_right - current_left) * 16;
                good_read_size1 += temp_read_size[temp_good_tags[j]];
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
            current_left = max(temp_divided_area[temp_good_tags[j]].second.first,min_saved_pos==100000?1:min_saved_pos);
            current_right = min(temp_divided_area[temp_good_tags[j]].second.second,max_saved_pos==0?V:max_saved_pos);
            cycle_G2 += min(current_right - last_tag_left,G);
            if(j != i){
                // cycle_G2 += (current_right - current_left) * 16 * ozd_weight[temp_good_tags[j]];
                cycle_G2 += (current_right - current_left) * 16;
            }
            good_read_size2 += temp_read_size[temp_good_tags[j]];
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
        // // fprintf(file,"min_saved_pos:%d max_saved_pos:%d divided_area_left:%d divided_area_right:%d\n",min_saved_pos,max_saved_pos,disks[1].divided_area[temp_border_tag - 1].second.first,disks[1].divided_area[temp_border_tag - 1].second.second);
        current_left = max(temp_divided_area[temp_border_tag].second.first,min_saved_pos==100000?1:min_saved_pos);
        current_right = min(temp_divided_area[temp_border_tag].second.second,max_saved_pos==0?V:max_saved_pos);
        // // fprintf(file,"current_left:%d current_right:%d\n",current_left,current_right);
        //遍历寻找最优的border_line
        for(int j = current_left;j <= current_right;j++){
            int temp_border_line = j;
            int temp_cycle_G1 = cycle_G1;
            int temp_cycle_G2 = cycle_G2;
            int temp_good_read_size1 = good_read_size1;
            int temp_good_read_size2 = good_read_size2;
            // temp_cycle_G1 += (j - current_left) * 16 * ozd_weight[temp_border_tag];
            // temp_cycle_G2 += (current_right - j) * 16 * ozd_weight[temp_border_tag];
            temp_cycle_G1 += (j - current_left) * 16;
            temp_cycle_G2 += (current_right - j) * 16;
            // // fprintf(file,"temp_cycle_G1:%d temp_cycle_G2:%d\n",temp_cycle_G1,temp_cycle_G2);
            temp_good_read_size1 += temp_read_size[temp_border_tag] * (j-current_left)/(current_right-current_left == 0?1:(current_right-current_left));
            temp_good_read_size2 += temp_read_size[temp_border_tag] * (current_right-j)/(current_right-current_left == 0?1:(current_right-current_left));
            // // fprintf(file,"temp_good_read_size1:%d temp_good_read_size2:%d\n",temp_good_read_size1,temp_good_read_size2);
            double avg_temp_cycle_score1 = (ceil(temp_cycle_G1 / G) * 0.5) * (-0.01) + 1;
            // if(ceil(temp_cycle_G1 / G) >105){
            //     avg_temp_cycle_score1 = -1;
            // }
            double avg_temp_cycle_score2 = (ceil(temp_cycle_G2 / G) * 0.5) * (-0.01) + 1;
            // if(ceil(temp_cycle_G2 / G) >105){
            //     avg_temp_cycle_score2 = -1;
            // }
            // // fprintf(file,"avg_temp_cycle_score1:%f avg_temp_cycle_score2:%f\n",avg_temp_cycle_score1,avg_temp_cycle_score2);
            double temp_score = avg_temp_cycle_score1 * temp_good_read_size1 + avg_temp_cycle_score2 * temp_good_read_size2;
            // // fprintf(file,"temp_score:%f\n",temp_score);
            if(temp_score > best_score){
                best_score = temp_score;
                best_border_tag = temp_border_tag;
                best_border_line = temp_border_line;
            }
        }
    }
    
    return mkp(best_score,mkp(best_border_tag,best_border_line));
}

int cal_tag0_line(vector<int> temp_good_tags,int stage){
    int cycle_G = 0;
    int current_left = 1;
    int current_right = 1;
    int last_tag_left = 1;
    int last_tag_right = 1;
    int good_read_size = 0;
    //计算从第一个tag到tag0左端花费的tokens
    for(int j = 0;j < temp_good_tags.size();j++){
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
        current_left = max(disks[1].divided_area[temp_good_tags[j] ].second.first,min_saved_pos==100000?1:min_saved_pos);
        current_right = min(disks[1].divided_area[temp_good_tags[j] ].second.second,max_saved_pos==0?V:max_saved_pos);
        cycle_G += (current_left - last_tag_right) > G ? G : (current_left - last_tag_right);
        last_tag_left = current_left;
        last_tag_right = current_right; 
        if(j != temp_good_tags.size() - 1){
            cycle_G += (current_right - current_left) * 16;
            // cycle_G += (current_right - current_left) * 16 * ozd_weight[temp_good_tags[j]];
            good_read_size += read_size[temp_good_tags[j]][stage];
        }
        // // fprintf(file,"temp_good_tags[%d]:%d,read_size:%d\n",j,temp_good_tags[j],read_size[temp_good_tags[j]][stage]);
    }
    // // fprintf(file,"good_read_size:%d\n",good_read_size);
    //遍历tag0的区域，计算每个位置的score
    int min_saved_pos = 100000;
    int max_saved_pos = 0;
    for(int k = 0;k < disks[1].saved_obj_position[0].size();k++){
        if(disks[1].saved_obj_position[0][k] < min_saved_pos){
            min_saved_pos = disks[1].saved_obj_position[0][k];
        }
        if(disks[1].saved_obj_position[0][k] > max_saved_pos){
            max_saved_pos = disks[1].saved_obj_position[0][k];
        }
    }
    current_left = max(disks[1].divided_area[0].second.first,min_saved_pos==100000?1:min_saved_pos);
    current_right = min(disks[1].divided_area[0].second.second,max_saved_pos==0?V:max_saved_pos);
    double best_score = 0;
    int best_tag0_line = 0;
    // // fprintf(file,"current_left:%d current_right:%d\n",current_left,current_right);
    for(int j = current_left;j <= current_right;j++){
        int temp_cycle_G = cycle_G;
        int temp_good_read_size = good_read_size;
        temp_cycle_G += (j - current_left) * 16 * ozd_weight[0];
        temp_good_read_size += read_size[0][stage] * (j-current_left)/(current_right-current_left);
        double avg_temp_cycle_score = (ceil(temp_cycle_G / (2*G)) * 0.5) * (-0.01) + 1;
        double temp_score = avg_temp_cycle_score * temp_good_read_size;
        // if(stage>5){
        //     // fprintf(file,"avg_temp_cycle_score:%f temp_good_read_size:%d temp_score:%f\n",avg_temp_cycle_score,temp_good_read_size,temp_score);
        // }
        if(temp_score > best_score){
            best_score = temp_score;
            best_tag0_line = j;
        }
    }
    return best_tag0_line;
}



void cal_good_tags1(int i){
    // if(i == 1){
    //     for(int j = 1;j<=M;j++){
    //         good_tags[0][i].insert(j);
    //     }
    //     good_tags[1][i].insert(0);
    //     return;
    // }
    double future_read_num[MAX_TAG_NUM] = {0};
    vector<pair<int,int>> temp_good_tags[MAX_TIME_SLICE];
    for(int j = 0; j <= M; j++){
        if(i == 1){
            if(j == 0){
                read_size[j][i] = 1800;
            }
            else{
                read_size[j][i] = 1;
            }
        }
        else{
        int sum_read_num = 0;
        for(int k = timestamp - final_sword;k<=timestamp;k++){
            sum_read_num += round1_tags_read_num[k][j];
        }
        double temp_read_size = static_cast<double>(sum_read_num) / (final_sword+1) * 1800;
        read_size[j][i] = temp_read_size;
        }
        // // fprintf(file,"core1:%f core2:%f\n",cluster_core[j][i-2],cluster_core[j][i-3]);
    }
    // // fprintf(file,"predict:\n");
    // for(int j = 1; j <= M; j++){
    //     read_size[0][i] += future_read_num[j] * tag0_object_write_size[j];
    //     // fprintf(file,"tag0_object_write_size[%d]:%d \n",j,tag0_object_write_size[j]);
    // }
    // // fprintf(file,"\n");
    // for(int j = 0; j <= M; j++){
    //     read_size[j][i] = future_read_num[j] * round1_accumulated_write_size[j] ;
    //     // // fprintf(file,"future_read_num[%d]:%f,accumulated_write_size[%d][%d]:%d,read_size[%d][%d]:%d\n",j,future_read_num[j],j,i,round1_accumulated_write_size[j],j,i,read_size[j][i]);
    // }
    // for(int j = 0; j <= M; j++){
        // fprintf(file,"read_size[0][%d]:%d\n",i,read_size[0][i]);
    // }
    // if(i>47){
    //     fclose(file);
    // }
    vector<pair<double,pair<int,int>>> temp_good_tags_with_score[MAX_TIME_SLICE];
    for(int j = 0; j <= M; j++){
        temp_good_tags_with_score[i].push_back(mkp(static_cast<double>(read_size[j][i]) / static_cast<double>(round1_accumulated_write_size[j] + 1),mkp(j,tag2order[j])));
    }
    sort(temp_good_tags_with_score[i].begin(),temp_good_tags_with_score[i].end(),[](pair<double,pair<int,int>> a,pair<double,pair<int,int>> b){
        return a.first > b.first;
    });
    // for(int j = 0;j<temp_good_tags_with_score[i].size();j++){
    //     fprintf(file,"tag:%d,temp_good_tags_with_score:%f\n",temp_good_tags_with_score[i][j].second.first,temp_good_tags_with_score[i][j].first);
    // }
    //计算tag0排第几名
    int tag0_idx = 0;
    for(int j = 0;j<temp_good_tags_with_score[i].size();j++){
        if(temp_good_tags_with_score[i][j].second.first == 0){
            tag0_idx = j;
            break;
        }
    }
    // // fprintf(file,"current_stage:%d\n",i);
    // // fprintf(file,"tag0_idx:%d\n",tag0_idx);
    //计算不算tag0最优组合及score
    double best_score = 0;
    int best_temp_cut = 0;
    int best_border_tag = 0;
    int best_border_line = 0;
    for(int j = 0; j < tag0_idx; j++){
        vector<pair<int,int>> temp_good_tags[MAX_TIME_SLICE];
        for(int k = 0;k<=j;k++){
            temp_good_tags[i].push_back(temp_good_tags_with_score[i][k].second);
        }
        sort(temp_good_tags[i].begin(),temp_good_tags[i].end(),[](pair<int,int> a,pair<int,int> b){
            return a.second < b.second;
        });
        vector<int> temp_vector;
        for(int j = 0; j < temp_good_tags[i].size(); j++){
            temp_vector.push_back(temp_good_tags[i][j].first);
        }
        pair<double,pair<int,int>> border_result = cal_border_line(temp_vector,i);
        if(border_result.first > best_score){
            best_score = border_result.first;
            best_temp_cut = j;
            best_border_tag = border_result.second.first;
            best_border_line = border_result.second.second;
        }
    }

    // fprintf(file,"best_score1:%f\n",best_score);


    //计算带上tag0最优组合及score
    vector<pair<int,int>> temp_good_tags_with_tag0[MAX_TIME_SLICE];
    for(int j = 0;j<=tag0_idx;j++){
        temp_good_tags_with_tag0[i].push_back(temp_good_tags_with_score[i][j].second);
    }
    sort(temp_good_tags_with_tag0[i].begin(),temp_good_tags_with_tag0[i].end(),[](pair<int,int> a,pair<int,int> b){
        return a.second < b.second;
    });
    vector<int> temp_vector_with_tag0;
    for(int j = 0; j < temp_good_tags_with_tag0[i].size(); j++){
        temp_vector_with_tag0.push_back(temp_good_tags_with_tag0[i][j].first);
    }
    int tag0_line = cal_tag0_line(temp_vector_with_tag0,i);
    // // fprintf(file,"tag0_line:%d\n",tag0_line);
    // // fprintf(file,"left:%d right:%d tag0_line:%d\n",disks[1].divided_area[0].second.first,disks[1].divided_area[0].second.second,tag0_line);
    pair<double,pair<int,int>> tag0_result = cal_border_line_with_tag0(temp_vector_with_tag0,i,tag0_line);
    if(tag0_result.first > best_score){
        best_score = tag0_result.first;
        best_temp_cut = tag0_idx;
        best_border_tag = 0;
        best_border_line = tag0_result.second.second;
        read_tag0[i] = true;
        read_tag0_line[i] = tag0_line;
    }

    // fprintf(file,"best_score2:%f\n",best_score);
    // // fprintf(file,"111best_border_tag:%d best_border_line:%d\n",best_border_tag,best_border_line);
    //计算带上tag0之后的tag的最优组合及score
    for(int j = tag0_idx+1; j < temp_good_tags_with_score[i].size(); j++){
        vector<pair<int,int>> temp_good_tags[MAX_TIME_SLICE];
        for(int k = 0;k<=j;k++){
            temp_good_tags[i].push_back(temp_good_tags_with_score[i][k].second);
        }
        sort(temp_good_tags[i].begin(),temp_good_tags[i].end(),[](pair<int,int> a,pair<int,int> b){
            return a.second < b.second;
        });
        vector<int> temp_vector;
        // fprintf(file,"temp_good_tags[i].size():%d\n",temp_good_tags[i].size());
        for(int k = 0; k < temp_good_tags[i].size(); k++){
            temp_vector.push_back(temp_good_tags[i][k].first);
            // fprintf(file,"temp_vector[%d]:%d ",k,temp_vector[k]);
        }
        pair<double,pair<int,int>> border_result = cal_border_line(temp_vector,i);
        if(border_result.first > best_score){
            best_score = border_result.first;
            best_temp_cut = j;
            best_border_tag = border_result.second.first;
            best_border_line = border_result.second.second;
            read_tag0[i] = false;
        }
    }
    // // fprintf(file,"222best_border_tag:%d best_border_line:%d\n",best_border_tag,best_border_line);
    // fprintf(file,"best_score3:%f\n",best_score);
    for(int j = 0;j<=best_temp_cut;j++){
        temp_good_tags[i].push_back(temp_good_tags_with_score[i][j].second);
        // // fprintf(file,"%d ",temp_good_tags[i][j].first);
    }
    sort(temp_good_tags[i].begin(),temp_good_tags[i].end(),[](pair<int,int> a,pair<int,int> b){
        return a.second < b.second;
    });
    vector<int> temp_vector;
    for(int j = 0; j < temp_good_tags[i].size(); j++){
        temp_vector.push_back(temp_good_tags[i][j].first);
    }
    pair<double,pair<int,int>> border_result = cal_border_line(temp_vector,i);
    bool is_filling_good_tags1 = true;
    // // fprintf(file,"border_result.first:%d border_result.second:%d\n",border_result.first,border_result.second);
    for(int j = 0; j < temp_vector.size(); j++){
        // // fprintf(file,"temp_good_tags[%d]:%d\n",j,temp_good_tags[i][j].first);
        if(temp_vector[j] == border_result.second.first){
            border_line[i] = border_result.second.second;
            border_tag[i] = border_result.second.first;
            is_filling_good_tags1 = false;
            continue;
        }
        if(is_filling_good_tags1){
            good_tags[0][i].insert(temp_vector[j]);
        }
        else{
            good_tags[1][i].insert(temp_vector[j]);
        }
    }
    // // fprintf(file,"current_stage:%d\n",i);
    // for(auto it = good_tags[0][i].begin(); it != good_tags[0][i].end(); it++){
    //     fprintf(file,"%d ",*it);
    // }
    // fprintf(file,"\n");
    // fprintf(file,"border_tag:%d border_line:%d\n",border_tag[i],border_line[i]);
    // for(auto it = good_tags[1][i].begin(); it != good_tags[1][i].end(); it++){
    //     fprintf(file,"%d ",*it);
    // }
    // fprintf(file,"read_tag0:%d read_tag0_line:%d\n",read_tag0[i],read_tag0_line[i]);
    // fprintf(file,"\n");
}

void cal_good_tags2(int i){
    double total_read_times[MAX_TIME_SLICE] = {0};
    vector<pair<double,pair<int,int>>> temp_good_tags_with_score[MAX_TIME_SLICE];
    for(int j = 1; j <= M; j++){
        temp_good_tags_with_score[i].push_back(mkp((read_size[j][i]) / static_cast<double>(accumulated_write_size_without_delete[j][i]),mkp(j,tag2order[j])));
    }
    sort(temp_good_tags_with_score[i].begin(),temp_good_tags_with_score[i].end(),[](pair<double,pair<int,int>> a,pair<double,pair<int,int>> b){
        return a.first > b.first;
    });
    // // fprintf(file,"temp_good_tags.size:%d\n",temp_good_tags[i].size());
    double best_score = 0;
    int best_temp_cut = 0;
    int best_border_tag = 0;
    int best_border_line = 0;
    for(int j = 0; j < temp_good_tags_with_score[i].size(); j++){
        vector<pair<int,int>> temp_good_tags[MAX_TIME_SLICE];
        for(int k = 0;k<=j;k++){
            temp_good_tags[i].push_back(temp_good_tags_with_score[i][k].second);
        }
        sort(temp_good_tags[i].begin(),temp_good_tags[i].end(),[](pair<int,int> a,pair<int,int> b){
            return a.second < b.second;
        });
        vector<int> temp_vector;
        for(int j = 0; j < temp_good_tags[i].size(); j++){
            temp_vector.push_back(temp_good_tags[i][j].first);
        }
        pair<double,pair<int,int>> border_result = cal_border_line(temp_vector,i);
        if(border_result.first > best_score){
            best_score = border_result.first;
            best_temp_cut = j;
            best_border_tag = border_result.second.first;
            best_border_line = border_result.second.second;
        }
    }
    vector<pair<int,int>> temp_good_tags[MAX_TIME_SLICE];
    // // fprintf(file,"best_good_tags:\n");
    for(int j = 0;j<=best_temp_cut;j++){
        temp_good_tags[i].push_back(temp_good_tags_with_score[i][j].second);
        // // fprintf(file,"%d ",temp_good_tags[i][j].first);
    }
    bool is_filling_good_tags1 = true;
    sort(temp_good_tags[i].begin(),temp_good_tags[i].end(),[](pair<int,int> a,pair<int,int> b){
        return a.second < b.second;
    });
        // // fprintf(file,"border_result.first:%d border_result.second:%d\n",border_result.first,border_result.second);
    for(int j = 0; j < temp_good_tags[i].size(); j++){
        // // fprintf(file,"temp_good_tags[%d]:%d\n",j,temp_good_tags[i][j].first);
        if(temp_good_tags[i][j].first == best_border_tag){
            border_line[i] = best_border_line;
            border_tag[i] = best_border_tag;
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
    // // fprintf(file,"current_stage:%d\n",i);
    // for(auto it = good_tags[0][i].begin(); it != good_tags[0][i].end(); it++){
    //     // fprintf(file,"%d ",*it);
    // }
    // // fprintf(file,"\n");
    // // fprintf(file,"border_tag:%d border_line:%d\n",border_tag[i],border_line[i]);
    // for(auto it = good_tags[1][i].begin(); it != good_tags[1][i].end(); it++){
    //     // fprintf(file,"%d ",*it);
    // }
}

void divide_disks_1() {//将所有磁盘按照tag的总写入大小加权划分
    int size[MAX_TAG_NUM];

    for(int i = 1; i <= M; i++){//计算每个tag分多少
        // size[i] = (V / 3 * 10/9) * tot_write_size[i] / tot_size;
        size[i] = (V / 3 * 35/100 + min(K * 48, V / 10)) / M;
        if(i == 15){
            size[i] *= 1.05;
        }
    }
    size[0] = (V / 3) * 65/100;

    int left = 1;
    int right;
    pair<int,pair<int,int>> temp_area[MAX_TAG_NUM];
    for(int i = 1; i <= M+1; i++){//计算每个tag的区域
        right = left + size[order2tag[i]];
        temp_area[i] = mkp(order2tag[i],mkp(left,right));
        left = right + 1;
    }

    for(int i = 1; i <= origin_N; i++){
        for(int j = 0; j <= M; j++){
            disks[i].divided_area.push_back(temp_area[tag2order[j]]);
            // fprintf(file,"disk[%d].divided_area[%d]:%d %d\n",i,j,disks[i].divided_area[j].second.first,disks[i].divided_area[disks[i].divided_area.size()-1].second.second);
        }
    }
}

void divide_disks_2() {//将所有磁盘按照tag的总写入大小加权划分
    int size[MAX_TAG_NUM];
    int tot_size = 0;
    vector<pair<int,pair<int,int>>> area;
    int group_size[MAX_TAG_NUM][MAX_TIME_SLICE];
    for(int i = 1;i<=M;i+=2){//计算所有tag的总写入大小
        for(int j = 1;j<=(T - 1) / FRE_PER_SLICING + 1;j++){
            group_size[i / 2 + 1][j] = (accumulated_write_size[order2tag[i]][j] + accumulated_write_size[order2tag[i+1]][j]) / 10 + 33;
        }
    }
    // for(int i = 1;i<=M/2;i++){
    //     for(int j = 1;j<=(T - 1) / FRE_PER_SLICING + 1;j++){
    //         // fprintf(file,"group_size[%d][%d]:%d\n",i,j,group_size[i][j]);
    //     }
    // }
    for(int i = 1;i<=M;i+=2){
        sort(group_size[i / 2 + 1],group_size[i / 2 + 1] + (T - 1) / FRE_PER_SLICING + 1,greater<int>());
        size[order2tag[i]] = group_size[i / 2 + 1][0] / 2;
        size[order2tag[i+1]] = group_size[i / 2 + 1][0] / 2;
    }
    // for(int i = 1;i<=(T - 1) / FRE_PER_SLICING + 1;i++){
    //     for(int j = 1;j<=M;j++){
    //         tot_write_size[j] += write_size[j][i];
    //     }
    // }

    // for(int i = 1; i <= M; i++){//计算每个tag分多少
    //     size[i] = (V / 3 * 10/9) * tot_write_size[i] / tot_size;
    // }

    int left = 1;
    int right;
    pair<int,pair<int,int>> temp_area[MAX_TAG_NUM];
    for(int i = 1; i <= M; i++){//计算每个tag的区域
        right = left + size[order2tag[i]] - 1;
        temp_area[i] = mkp(order2tag[i],mkp(left,right));
        left = right + 1;
    }

    for(int i = 1; i <= origin_N; i++){
        for(int j = 0; j <= M; j++){
            disks[i].divided_area.push_back(temp_area[tag2order[j]]);
        }
    }
    // for(int i = 1;i<=origin_N;i++){
    //     for(int j = 0;j<disks[i].divided_area.size();j++){
    //         // fprintf(file,"tag:%d left:%d right:%d\n",disks[i].divided_area[j].first,disks[i].divided_area[j].second.first,disks[i].divided_area[j].second.second);
    //     }
    // }
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
    // //// fprintf(file,"temp_position_size:%d ,find_obj_id:%d ,obj_size:%d\n",temp_position.size(),find_obj_id,objects[find_obj_id-1].size);

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
    // //// fprintf(file, "from_positon:%d to_position:%d\n", from_position, to_position);

    
}
void cal_stage_info(){
    //计算每个stage的写入大小
    for(int i = 1; i <= T; i++){
        for(int j = 1; j<=write_record[i].n_write;j++){
            write_size[objects[write_record[i].object_ids[j-1]-1].tag][(i-1)/FRE_PER_SLICING+1] += objects[write_record[i].object_ids[j-1]-1].size;
        }
    }
    //计算每个stage的读取大小
    for(int i = 1; i <= T; i++){
        for(int j = 1; j<=read_record[i].n_read;j++){
            read_size[objects[read_record[i].object_ids[j-1]-1].tag][(i-1)/FRE_PER_SLICING+1] += objects[read_record[i].object_ids[j-1]-1].size;
        }
    }
    //计算每个stage的删除大小
    for(int i = 1; i <= T; i++){
        for(int j = 1; j<=delete_record[i].n_delete;j++){
            del_size[objects[delete_record[i].object_ids[j-1]-1].tag][(i-1)/FRE_PER_SLICING+1] += objects[delete_record[i].object_ids[j-1]-1].size;
        }
    }
    // // fprintf(file, "%d %d %d %d %d %d\n", T, M, N, V, G,K);
    // for (int i = 1; i <= M; i++) {//计算每个tag总共删除的大小
    //     for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
    //         // fprintf(file, "%d ", del_size[i][j]);
    //     }
    //     // fprintf(file, "\n");
    // }
    // for (int i = 1; i <= M; i++) {//计算每个tag总共删除的大小
    //     for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
    //         // fprintf(file, "%d ", write_size[i][j]);
    //     }
    //     // fprintf(file, "\n");
    // }
    // for (int i = 1; i <= M; i++) {//计算每个tag总共删除的大小
    //     for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
    //         // fprintf(file, "%d ", read_size[i][j]);
    //     }
    //     // fprintf(file, "\n");
    // }
}

void write_print(int id, int size, int tag, int save_disk_id[], vector<int> areas[]){
    printf("%d\n",id);
    ////// fprintf(file,"id:%d\n",id);
    for(int i = 0; i < 3; i++){
        printf("%d ",(save_disk_id[i]-1)%10 + 1);
        ////// fprintf(file,"disk_id:%d ",save_disk_id[i]);
        for(int j = 0; j < areas[i].size(); j++){

            // if(save_disk_id[i]>10){
            //     printf("%d ",areas[i][j]+V);
            // }
            // else{
            printf("%d ",areas[i][j]);
            // }
            ////// fprintf(file,"area:%d ",areas[i][j]);
        }
        ////// fprintf(file,"\n");
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
    ////////////// fprintf(file,"obj_id:%d,obj_size:%d\n",obj_id,obj_size);
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
        && ((good_tags[0][current_stage].count(objects[disks[(id-1)%origin_N+1].read_list[i][size-1].object_id-1].tag) != 0 || good_tags[1][current_stage].count(objects[disks[(id-1)%origin_N+1].read_list[i][size-1].object_id-1].tag) != 0||objects[disks[(id-1)%origin_N+1].read_list[i][size-1].object_id-1].tag == border_tag[current_stage])
            || (good_tags[0][current_stage].count(objects[disks[(id-1)%origin_N+1].read_list[i][size-1].object_id-1].in_tag_area) != 0 || good_tags[1][current_stage].count(objects[disks[(id-1)%origin_N+1].read_list[i][size-1].object_id-1].in_tag_area) != 0||objects[disks[(id-1)%origin_N+1].read_list[i][size-1].object_id-1].in_tag_area == border_tag[current_stage]));
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

void normalize_data(){
    for(int i = 1; i <= M; i++){
        int max_read_size = 0;
        int max_delete_size = 0;
        for(int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++){
            max_read_size = max(max_read_size,read_size[i][j]);
            max_delete_size = max(max_delete_size,del_size[i][j]);
        }
        for(int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++){
            normalized_read_size[i][j] = double(read_size[i][j]) / double(max_read_size);
            normalized_delete_size[i][j] = double(del_size[i][j]) / double(max_delete_size);
        }
    }
}
double cal_read_distance(int tag1,int tag2){
    double distance = 0;
    for(int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; i++){
        distance += abs(normalized_read_size[tag1][i] - normalized_read_size[tag2][i]);
    }
    return distance;
}

double cal_delete_distance(int tag1,int tag2){
    double distance = 0;
    for(int i = 1; i <= (T - 1) / FRE_PER_SLICING + 1; i++){
        distance += abs(normalized_delete_size[tag1][i] - normalized_delete_size[tag2][i]);
    }
    return distance;
}

double cal_SA_score(){
    double score = 0;
    double read_weight = 0.7;
    double delete_weight = 0.3;
    double read_distance = 0;
    double delete_distance = 0;
    for(int i = 2; i <= M; i++){
        read_distance += cal_read_distance(order2tag[i],order2tag[i-1]);
    }
    for(int i = 1;i<=M/2;i++){
        delete_distance += cal_delete_distance(order2tag[2*i],order2tag[2*i-1]);
    }
    score = delete_distance * delete_weight - read_distance * read_weight;
    return score;
}

void greedy(){
    normalize_data();
    int first_tag = rand() % M + 1;
    bool used_tag[MAX_TAG_NUM];
    for(int i = 1; i <= M; i++){
        used_tag[i] = false;
    }
    used_tag[first_tag] = true;


    double read_weight = 1.0;
    double delete_weight = 0.0;
    double read_distance = 0;
    double delete_distance = 0;
    order2tag[1] = first_tag;
    for (int i = 2; i <= M; i++) {
        double max_score = -1e9;
        int max_tag = 0;
        double score = 0.0;
        for(int j = 1; j <= M; j++){
            if(used_tag[j] == false){
                delete_distance = cal_delete_distance(order2tag[i-1],j);
                read_distance = cal_read_distance(order2tag[i-1],j);
                score = delete_distance * delete_weight - read_distance * read_weight;
                if(score > max_score){
                    max_score = score;
                    max_tag = j;
                }
            }
        }
        used_tag[max_tag] = true;
        order2tag[i] = max_tag;
    }
}

void Simulated_Annealing(){
    // 初始化温度
    double T = 1000;
    double T_min = 1e-8;
    double alpha = 0.999;
    int max_iter = 100000;

    normalize_data();
    // 初始化order2tag
    double score = cal_SA_score();
    int old_order2tag[MAX_TAG_NUM];
    for(int iter = 0; iter < max_iter; iter++){
        for(int i = 1; i <= M; i++){
            old_order2tag[i] = order2tag[i];
        }
        // 随机交换两个tag
        int tag1 = rand() % M + 1;
        int tag2 = rand() % M + 1;
        // 交换order2tag
        int temp = order2tag[tag1];
        order2tag[tag1] = order2tag[tag2];
        order2tag[tag2] = temp;

        double new_score = cal_SA_score();
        double delta_score = new_score - score;
        if(delta_score > 0){//接受新解
            score = new_score;
        }
        else{
            double p = exp(-delta_score / T);
            if(rand() < p * RAND_MAX){//接受新解
                score = new_score;
            }
            else{//拒绝新解
                for(int i = 1; i <= M; i++){
                    order2tag[i] = old_order2tag[i];
                }
            }
        }
        T *= alpha;
    }

}

// ----------------------------basic function--------------------------------

void delete_action_1()//删除对象
{
    int n_delete;
    int abort_num = 0;
    vector<int> abort_request_id;
    int obj_id[MAX_OBJECT_NUM];
    n_delete = fast_read();
    delete_record[timestamp].n_delete = n_delete;
    for (int i = 1; i <= n_delete; i++) {
        obj_id[i] = fast_read();
        objects[obj_id[i]-1].delete_timestamp = timestamp;
        delete_record[timestamp].object_ids.push_back(obj_id[i]);
        if(objects[obj_id[i]-1].in_tag_area != 0){
            round1_accumulated_write_size[objects[obj_id[i]-1].in_tag_area] -= objects[obj_id[i]-1].size;
        }
        else{
            round1_accumulated_write_size[objects[obj_id[i]-1].tag] -= objects[obj_id[i]-1].size;
        }
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
    // fclose(file);
    printf("%d\n",abort_num);
    for (int i = 0; i < abort_request_id.size(); i++) {
        printf("%d\n", abort_request_id[i]);
        /////// fprintf(file,"%d\n", abort_request_id[i]);
    }
    fflush(stdout);
}

void delete_action_2()//删除对象
{
    int n_delete;
    int abort_num = 0;
    vector<int> abort_request_id;
    int obj_id[MAX_OBJECT_NUM];
    // n_delete = fast_read();
    n_delete = delete_record[timestamp].n_delete;
    
    for (int i = 1; i <= n_delete; i++) {
        // obj_id[i] = fast_read();
        obj_id[i] = delete_record[timestamp].object_ids[i-1];
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
    ////// fprintf(file,"abort_num:%d\n",abort_num);
    for (int i = 0; i < abort_request_id.size(); i++) {
        printf("%d\n", abort_request_id[i]);
        /////// fprintf(file,"%d\n", abort_request_id[i]);
    }

    fflush(stdout);
}
void incre_action(){
    int incre_num = fast_read();
    for(int i = 1; i <= incre_num; i++){
        int obj_id = fast_read();
        int obj_tag = fast_read();
        cluster_core_obj_id[obj_tag].push_back(obj_id);
        // // fprintf(file,"obj_id:%d current_tag:%d new_tag:%d\n",obj_id,objects[obj_id-1].tag,obj_tag);
        objects[obj_id-1].tag = obj_tag;
    }
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

void write_action_1(){//写入对象
    int n_write;
    int id[MAX_OBJECT_NUM];
    int size[MAX_OBJECT_NUM];
    int tag[MAX_OBJECT_NUM];

    n_write = fast_read();
    obj_num += n_write;
    Object temp_obj[1000];
    //// fprintf(file,"n_write:%d\n",n_write);
    // fclose(file);
    for (int i = 1; i <= n_write; i++) {
        temp_obj[i].id = fast_read();
        temp_obj[i].size = fast_read();
        temp_obj[i].tag = fast_read();
        round1_accumulated_write_size[temp_obj[i].tag] += temp_obj[i].size;
        write_record[timestamp].n_write = n_write;
        write_record[timestamp].object_ids.push_back(temp_obj[i].id);
        write_record[timestamp].object_sizes.push_back(temp_obj[i].size);
        write_record[timestamp].object_tags.push_back(temp_obj[i].tag);
        //// fprintf(file,"%d %d %d\n",temp_obj[i].id,temp_obj[i].size,temp_obj[i].tag);
    }
    for(int i = 1; i <= n_write; i++){
        id[i] = temp_obj[i].id;
        size[i] = temp_obj[i].size;
        tag[i] = temp_obj[i].tag;
        //// fprintf(file,"%d %d %d\n",id[i],size[i],tag[i]);
    }
    // for(int i = 1; i <= n_write; i++){
    //     id[i] = temp_obj[i].id;
    //     size[i] = temp_obj[i].size;
    //     tag[i] = temp_obj[i].tag;

    //     //// fprintf(file,"%d %d %d\n",id[i],size[i],tag[i]);
    // }
    
    
    int chosen_disks[3];
    // auto now = std::chrono::system_clock::now();
    // auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    // // fprintf(file,"write_action6666666前当前时间戳（毫秒）: :%d\n",time);
    // // fprintf(file,"write_action_1 aaaaaafter\n");
    // fclose(file);
    for (int i = 1; i <= n_write; i++) {

        bool writed_successfully[3] = {false,false,false};//三个盘分别是否能够写入

        int chosen_disks[3];//选择磁盘
        chosen_disks[0] = find_big_clean_disk(temp_obj[i].tag);
        vector<pair<int, int>> disk_id_and_size;
        for(int j = 1; j <= origin_N; j++){
            if(disks[j].id % 10 != chosen_disks[0] % 10){
                // temp_disks.push_back(disks[j]);
                disk_id_and_size.push_back(mkp(disks[j].id,disks[j].trash_area_size));
            }
        }

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
        obj.id = temp_obj[i].id;
        obj.size = temp_obj[i].size;
        obj.tag = temp_obj[i].tag;
        obj.temp_tag = temp_obj[i].tag;
        obj.store_timestamp = timestamp;
        obj.in_tag_area = obj.tag;
        obj.saved_disk[0] = chosen_disks[0];
        obj.saved_disk[1] = chosen_disks[1];
        obj.saved_disk[2] = chosen_disks[2];

        int left = disks[chosen_disks[0]].divided_area[tag[i]].second.first;
        int right = disks[chosen_disks[0]].divided_area[tag[i]].second.second;
        int leftright = tag2order[tag[i]]>1?disks[chosen_disks[0]].divided_area[order2tag[tag2order[tag[i]]-1]].second.first:1;
        int rightleft = tag2order[tag[i]]<M?disks[chosen_disks[0]].divided_area[order2tag[tag2order[tag[i]]+1]].second.second:right;
        int cal = 0;

        if(tag2order[tag[i]] % 2 == 1){
            vector<pair<int,int>> temp_self_vector;
            vector<pair<int,int>> temp_other_vector;
            bool is_self = true;
            // //// fprintf(file,"left:%d,rightleft:%d\n",left,rightleft);
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
                    // //// fprintf(file,"obj_id:%d,st:%d\n",id[i],st);
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

        if(!writed_successfully[0]){
            int group_index = -1;
            for(int j = 0; j < GROUP_NUM; j++){//找当前obj对应的group
                if(tag_group[j].count(tag[i]) > 0){
                    group_index = j;
                    break;
                }
            }
            if(group_index != -1){
                for(int temp_tag : tag_group[group_index]){
                    if(writed_successfully[0]){//写入成功后就跳出循环
                        break;
                    }
                    int left = disks[chosen_disks[0]].divided_area[temp_tag].second.first;
                    int right = disks[chosen_disks[0]].divided_area[temp_tag].second.second;
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
                            disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                            break;
                        }
                    }
                }
            }
        }



        for(int j = 1; j <= 2; j++){//写入副本
            int cal = 0;
            for(int k = V; k >= 1; k--){
                if(disks[chosen_disks[j]].saved_object_id[k] == 0){
                    cal++;
                    obj.saved_position[j].push_back(k);
                    disks[chosen_disks[j]].saved_object_id[k] = temp_obj[i].id;
                    if(cal >= temp_obj[i].size){
                        writed_successfully[j] = true;//副本能够成功写入
                        if(j == 0){
                            disks[chosen_disks[j]].used_clean_area_size[temp_obj[i].tag] += temp_obj[i].size;
                        }
                        else{
                            disks[chosen_disks[j]].trash_area_size += temp_obj[i].size;
                        }
                        break;
                    }
                }
            }
        }

        for(int j = 0; j < temp_obj[i].size; j++){
            int position = obj.saved_position[0][j];
            if(find(disks[chosen_disks[0]].saved_obj_position[temp_obj[i].tag].begin(), disks[chosen_disks[0]].saved_obj_position[temp_obj[i].tag].end(), position) == disks[chosen_disks[0]].saved_obj_position[temp_obj[i].tag].end()){
                disks[chosen_disks[0]].saved_obj_position[temp_obj[i].tag].push_back(position);
            }
        }
        objects[temp_obj[i].id-1] = obj;
        if(objects[temp_obj[i].id-1].tag != 0){
            cluster_core_obj_id[objects[temp_obj[i].id-1].tag].push_back(temp_obj[i].id);
        }
        write_print(temp_obj[i].id,temp_obj[i].size,temp_obj[i].tag,chosen_disks,obj.saved_position);
    }
    
    // fclose(file);
    fflush(stdout);
}

void write_action_2(){//写入对象
    int n_write;
    int id[MAX_OBJECT_NUM];
    int size[MAX_OBJECT_NUM];
    int tag[MAX_OBJECT_NUM];
    // n_write = fast_read();
    n_write = write_record[timestamp].n_write;
    Object temp_obj[1000];
    //// fprintf(file,"n_write:%d\n",n_write);
    // fclose(file);
    for (int i = 1; i <= n_write; i++) {
        // temp_obj[i].id = fast_read();
        temp_obj[i].id = write_record[timestamp].object_ids[i-1];
        // temp_obj[i].size = fast_read();
        temp_obj[i].size = write_record[timestamp].object_sizes[i-1];
        // temp_obj[i].tag = fast_read();
        temp_obj[i].tag = objects[temp_obj[i].id-1].tag;
        //// fprintf(file,"%d %d %d\n",temp_obj[i].id,temp_obj[i].size,temp_obj[i].tag);
    }
    // fclose(file);
    // sort(temp_obj+1,temp_obj+n_write+1,compare_by_size);
    for(int i = 1; i <= n_write; i++){
        id[i] = temp_obj[i].id;
        size[i] = temp_obj[i].size;
        tag[i] = temp_obj[i].tag;
        //// fprintf(file,"%d %d %d\n",id[i],size[i],tag[i]);
    }
    // fclose(file);

    int chosen_disks[3];
    // auto now = std::chrono::system_clock::now();
    // auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    // // fprintf(file,"write_action6666666前当前时间戳（毫秒）: :%d\n",time);
    for (int i = 1; i <= n_write; i++) {
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // // fprintf(file,"write_action444444444前当前时间戳（毫秒）: :%d\n",time);
        bool writed_successfully[3] = {false,false,false};//三个盘分别是否能够写入

        int chosen_disks[3];//选择磁盘
        vector<pair<int, int>> disk_id_and_size;
        if(tag[i] != 0){
            chosen_disks[0] = find_big_clean_disk(tag[i]);
        }
        else{
            for(int j = 1; j <= origin_N; j++){
                if(disks[j].id != chosen_disks[0]){
                    // temp_disks.push_back(disks[j]);
                    disk_id_and_size.push_back(mkp(disks[j].id,disks[j].trash_area_size));
                }
            }
            int min_trash_area_size = 1000000000;
            for(int j = 0; j < disk_id_and_size.size(); j++){//从所有非主盘的磁盘中选择垃圾区最小的磁盘和区
                if(disk_id_and_size[j].second < min_trash_area_size){
                    min_trash_area_size = disk_id_and_size[j].second;
                    chosen_disks[0] = disk_id_and_size[j].first;
                }
            }
        }
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // // fprintf(file,"write_action555555555前当前时间戳（毫秒）: :%d\n",time);
        disk_id_and_size.clear();
        for(int j = 1; j <= origin_N; j++){
            if(disks[j].id % 10 != chosen_disks[0] % 10){
                // temp_disks.push_back(disks[j]);
                disk_id_and_size.push_back(mkp(disks[j].id,disks[j].trash_area_size));
            }
        }
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // // fprintf(file,"write_action77777写入本体前当前时间戳（毫秒）: :%d\n",time);
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
        int left = disks[chosen_disks[0]].divided_area[tag[i]].second.first;
        int right = disks[chosen_disks[0]].divided_area[tag[i]].second.second;
        int leftright = tag2order[tag[i]]>1?disks[chosen_disks[0]].divided_area[order2tag[tag2order[tag[i]]-1]].second.first:1;
        int rightleft = tag2order[tag[i]]<M?disks[chosen_disks[0]].divided_area[order2tag[tag2order[tag[i]]+1]].second.second:right;
        int cal = 0;
        
        if(tag[i] == 0){//tag为0，直接写入垃圾堆
            cal = 0;
            for(int k = V; k >= 1; k--){
                if(disks[chosen_disks[0]].saved_object_id[k] == 0){
                    cal++;
                    obj.saved_position[0].push_back(k);
                    disks[chosen_disks[0]].saved_object_id[k] = id[i];
                    if(cal >= size[i]){
                        writed_successfully[0] = true;//副本能够成功写入
                        disks[chosen_disks[0]].trash_area_size += size[i];
                        break;
                    }
                }
            }
        }
        if(writed_successfully[0]== false){
            if(tag2order[tag[i]] % 2 == 1){
                vector<pair<int,int>> temp_self_vector;
                vector<pair<int,int>> temp_other_vector;
                bool is_self = true;
                // //// fprintf(file,"left:%d,rightleft:%d\n",left,rightleft);
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
                        // //// fprintf(file,"obj_id:%d,st:%d\n",id[i],st);
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
        }
        
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // // fprintf(file,"write8888888后当前时间戳（毫秒）: :%d\n",time);
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
                int left = disks[chosen_disks[0]].divided_area[temp_tag].second.first;
                int right = disks[chosen_disks[0]].divided_area[temp_tag].second.second;
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
                        disks[chosen_disks[0]].used_clean_area_size[tag[i]] += size[i];
                        break;
                    }
                }
            }
        }
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // // fprintf(file,"write9999999后当前时间戳（毫秒）: :%d\n",time);
        if(!writed_successfully[0]&&disks[chosen_disks[0]].allocated_area.size() == 0){//如果写不下，且allocated_area为空，则创建第一个allocated_area
            int idx = disks[chosen_disks[0]].divided_area.size();
            left = disks[chosen_disks[0]].divided_area[order2tag[idx]].second.second + 1;//第一个allocated_area的起始位置为最后一个分区的结束位置+1
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
        // // fprintf(file,"write0000000后当前时间戳（毫秒）: :%d\n",time);
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
        // // fprintf(file,"write11111111后当前时间戳（毫秒）: :%d\n",time);
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
        // // fprintf(file,"write222222222222222后当前时间戳（毫秒）: :%d\n",time);
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
        // // fprintf(file,"write333333333333333后当前时间戳（毫秒）: :%d\n",time);
    }
    fflush(stdout);
}

void findTarget(int read_head_id){
    int position = read_head[read_head_id].position;

    for (int i = 1; i <= V; i++){
        int size = disks[(read_head_id-1)%origin_N+1].read_list[(i+position-2)%V+1].size();
        bool flag = false;
        if(size > 0){
            Object obj = objects[(disks[(read_head_id-1)%origin_N+1].read_list[(i+position-2)%V+1][size-1].object_id)-1];
            if(read_head_id <= 10){
                flag = (obj.tag !=0 && (good_tags[0][current_stage].count(obj.tag) != 0 || border_tag[current_stage] == obj.tag && (i+position-2)%V+1 <= border_line[current_stage]))
                || ((obj.tag == 0 && obj.in_tag_area != 0 || obj.tag == 0 && obj.in_tag_area == 0 && read_tag0[current_stage] == 0 || obj.tag == 0 && obj.in_tag_area == 0 && read_tag0[current_stage] == 1 && (i+position-2)%V+1 <= read_tag0_line[current_stage]) && (good_tags[0][current_stage].count(obj.in_tag_area) != 0 || border_tag[current_stage] == obj.in_tag_area && ((i+position-2)%V+1) <= border_line[current_stage]));
            }
            else{
                flag = (obj.tag !=0 && (good_tags[1][current_stage].count(obj.tag) != 0 || border_tag[current_stage] == obj.tag && (i+position-2)%V+1 > border_line[current_stage]))
                || ((obj.tag == 0 && obj.in_tag_area != 0 || obj.tag == 0 && obj.in_tag_area == 0 && read_tag0[current_stage] == 0 || obj.tag == 0 && obj.in_tag_area == 0 && read_tag0[current_stage] == 1 && (i+position-2)%V+1 <= read_tag0_line[current_stage]) && (good_tags[1][current_stage].count(obj.in_tag_area) != 0 || border_tag[current_stage] == obj.in_tag_area && ((i+position-2)%V+1) > border_line[current_stage]));
            }
            // if(obj_num>14881){
            //     fprintf(file,"14881:tag:%d in_tag_area:%d contains:%d\n", objects[14880].tag, objects[14880].in_tag_area,good_tags[0][current_stage].count(objects[14880].in_tag_area));
            // }
        }
        if(size > 0 
        && disks[(read_head_id-1)%origin_N+1].read_list[(i+position-2)%V+1][size-1].st_time > timestamp - 105
        && disks[(read_head_id-1)%origin_N+1].read_list[(i+position-2)%V+1][size-1].is_abort == false 
        && disks[(read_head_id-1)%origin_N+1].read_list[(i+position-2)%V+1][size-1].is_done == false
        && flag)
        {
            read_head[read_head_id].target_position = (i+position-2)%V+1;
            break;
        }
    }
}

bool cmp_temp_requests(pair<int,int> a,pair<int,int> b){
    return a.second < b.second;
}

int tag_appear_num[MAX_TAG_NUM] = {0};



int find_nearest_cluster_core(int object_id){
    if(objects[object_id-1].store_timestamp % cluster_stage_length != 0){
        double embedding = objects[object_id-1].read_embedding[objects[object_id-1].store_timestamp/cluster_stage_length];
        embedding = embedding * cluster_stage_length / (timestamp - objects[object_id-1].store_timestamp);
        objects[object_id-1].read_embedding[objects[object_id-1].store_timestamp/cluster_stage_length] = embedding;
        objects[object_id-1].store_timestamp = timestamp - cluster_stage_length;
    }
    if(objects[object_id-1].delete_timestamp % cluster_stage_length != 0){
        double embedding = objects[object_id-1].read_embedding[objects[object_id-1].delete_timestamp/cluster_stage_length];
        embedding = embedding * cluster_stage_length / (objects[object_id-1].delete_timestamp - timestamp + cluster_stage_length);
        objects[object_id-1].read_embedding[objects[object_id-1].delete_timestamp/cluster_stage_length] = embedding;
        objects[object_id-1].delete_timestamp = timestamp;
    }
    double min_distance = 1000000000;
    int nearest_cluster_core = 0;
    for (int i = 1; i <= M; i++){
        double distance = 0;
        for(int j = 0; j < timestamp/cluster_stage_length; j++){
            if(j * cluster_stage_length >= objects[object_id-1].store_timestamp && (objects[object_id-1].delete_timestamp == 0 || objects[object_id-1].delete_timestamp >= j * cluster_stage_length)){
                distance += abs(objects[object_id-1].read_embedding[j] - cluster_core[i][j]);
            }
        }
        if(distance < min_distance){
            min_distance = distance;
            nearest_cluster_core = i;
        }
    }
    if(objects[object_id-1].read_num < 8){
        nearest_cluster_core = 0;
    }
    return nearest_cluster_core;
}

void update_cluster_core(){
    for(int i = 1; i <= M; i++){
        for(int j = 0; j < cluster_core_obj_id[i].size(); j++){
            if(objects[cluster_core_obj_id[i][j]-1].store_timestamp % cluster_stage_length != 0){
                int embedding = objects[cluster_core_obj_id[i][j]-1].read_embedding[objects[cluster_core_obj_id[i][j]-1].store_timestamp/cluster_stage_length];
                embedding = embedding * cluster_stage_length / (timestamp - objects[cluster_core_obj_id[i][j]-1].store_timestamp);
                objects[cluster_core_obj_id[i][j]-1].read_embedding[objects[cluster_core_obj_id[i][j]-1].store_timestamp/cluster_stage_length] = embedding;
                objects[cluster_core_obj_id[i][j]-1].store_timestamp = timestamp - cluster_stage_length;
                // // fprintf(file,"object_id:%d store_timestamp:%d embedding:%d\n",cluster_core_obj_id[i][j],objects[cluster_core_obj_id[i][j]-1].store_timestamp,embedding);
            }
            if(objects[cluster_core_obj_id[i][j]-1].delete_timestamp % cluster_stage_length != 0){
                int embedding = objects[cluster_core_obj_id[i][j]-1].read_embedding[objects[cluster_core_obj_id[i][j]-1].delete_timestamp/cluster_stage_length];
                embedding = embedding * cluster_stage_length / (objects[cluster_core_obj_id[i][j]-1].delete_timestamp - timestamp + cluster_stage_length);
                objects[cluster_core_obj_id[i][j]-1].read_embedding[objects[cluster_core_obj_id[i][j]-1].delete_timestamp/cluster_stage_length] = embedding;
                objects[cluster_core_obj_id[i][j]-1].delete_timestamp = timestamp;
                // // fprintf(file,"object_id:%d delete_timestamp:%d embedding:%d\n",cluster_core_obj_id[i][j],objects[cluster_core_obj_id[i][j]-1].delete_timestamp,embedding);
            }
        }
        for(int j = 0; j < timestamp/cluster_stage_length; j++){
            double sum = 0;
            int weight = 0;
            for(int k = 0; k < cluster_core_obj_id[i].size(); k++){
                if(j * cluster_stage_length >= objects[cluster_core_obj_id[i][k]-1].store_timestamp && (objects[cluster_core_obj_id[i][k]-1].delete_timestamp == 0 || objects[cluster_core_obj_id[i][k]-1].delete_timestamp >= j * cluster_stage_length)){
                    sum += objects[cluster_core_obj_id[i][k]-1].read_embedding[j];
                    weight += 1;
                    // // fprintf(file,"tag:%d sum:%d weight:%d\n",i,sum,weight);
                }
            }
            if(weight > 0){
                cluster_core[i][j] = sum / weight;
            }
        }
    }
}

void update_cluster0_core(){
    for(int i = 1; i<=obj_num; i++){
        if(objects[i-1].tag == 0 && objects[i-1].in_tag_area == 0){
            if(objects[i-1].store_timestamp % cluster_stage_length != 0){
                int embedding = objects[i-1].read_embedding[objects[i-1].store_timestamp/cluster_stage_length];
                embedding = embedding * cluster_stage_length / (timestamp - objects[i-1].store_timestamp);
                objects[i-1].read_embedding[objects[i-1].store_timestamp/cluster_stage_length] = embedding;
                objects[i-1].store_timestamp = timestamp - cluster_stage_length;
                // // fprintf(file,"object_id:%d store_timestamp:%d embedding:%d\n",cluster_core_obj_id[i][j],objects[cluster_core_obj_id[i][j]-1].store_timestamp,embedding);
            }
            if(objects[i-1].delete_timestamp % cluster_stage_length != 0){
                int embedding = objects[i-1].read_embedding[objects[i-1].delete_timestamp/cluster_stage_length];
                embedding = embedding * cluster_stage_length / (objects[i-1].delete_timestamp - timestamp + cluster_stage_length);
                objects[i-1].read_embedding[objects[i-1].delete_timestamp/cluster_stage_length] = embedding;
                objects[i-1].delete_timestamp = timestamp;
                // // fprintf(file,"object_id:%d delete_timestamp:%d embedding:%d\n",cluster_core_obj_id[i][j],objects[cluster_core_obj_id[i][j]-1].delete_timestamp,embedding);
            }
        }
    }
    for(int j = 0; j < timestamp/cluster_stage_length; j++){
            double sum = 0;
            int weight = 0;
            for(int k = 1; k <= obj_num; k++){
                if(objects[k-1].tag == 0 && objects[k-1].in_tag_area == 0){
                if(j * cluster_stage_length >= objects[k-1].store_timestamp && (objects[k-1].delete_timestamp == 0 || objects[k-1].delete_timestamp >= j * cluster_stage_length)){
                    sum += objects[k-1].read_embedding[j];
                    weight += 1;
                    // // fprintf(file,"tag:%d sum:%d weight:%d\n",i,sum,weight);
                }
            }
            if(weight > 0){
                cluster_core[0][j] = sum / weight;
            }
        }
    }
}

void read_action_1(){//读取对象
    int n_read;
    int request_id, object_id;
    // n_read = fast_read();
    n_read = fast_read();
    read_record[timestamp].n_read = n_read;
    vector<pair<int,int>> temp_requests;
    ////// fprintf(file,"n_read:%d\n",n_read);
    // cal_bad_tag_timely();
    for (int i = 1; i <= n_read; i++) {
        // request_id = fast_read();
        request_id = fast_read();
        // object_id = fast_read();
        object_id = fast_read();
        read_Request temp_request ;
        temp_request.request_id = request_id;
        temp_request.object_id = object_id;
        temp_request.is_done = false;
        temp_request.is_abort = false;
        temp_request.st_time = timestamp;
        id2request[request_id] = temp_request;
        read_record[timestamp].request_ids.push_back(request_id);
        read_record[timestamp].object_ids.push_back(object_id);
        temp_requests.push_back(mkp(request_id,object_id));
        objects[object_id-1].read_embedding[timestamp/cluster_stage_length]++;
        Object obj = objects[temp_request.object_id-1];
        objects[object_id-1].read_num++;
        int disk_id = obj.saved_disk[0];
        // accumulated_read_size[obj.tag] += obj.size;
        bool flag = false;
        if(obj.tag == 0){
            round1_tags_read_num[timestamp][obj.in_tag_area] += obj.size;
        }
        else{
            round1_tags_read_num[timestamp][obj.tag]+=obj.size;
        }
        if(obj.tag == 0 && obj.in_tag_area == 0){
            tag0_object_read_size[current_stage] += obj.size;
        }
        // if(obj.tag == 0 && obj.in_tag_area == 0 && read_tag0[current_stage] == 1){
        //     flag = obj.saved_position[0][0] <= read_tag0_line[current_stage]
        //     && ((good_tags[0][current_stage].count(obj.tag) != 0 
        //         || good_tags[1][current_stage].count(obj.tag) != 0 
        //         || obj.tag == border_tag[current_stage])
        //     ||(good_tags[0][current_stage].count(obj.in_tag_area) != 0 
        //         || good_tags[1][current_stage].count(obj.in_tag_area) != 0 
        //         || obj.in_tag_area == border_tag[current_stage]));
        //     // fprintf(file,"current_stage:%d obj_id:%d flag:%d\n",current_stage,object_id,flag);
        //     // fclose(file);
        // }
        // else{
        flag = (read_tag0[current_stage] == 1 && obj.saved_position[0][0] <= read_tag0_line[current_stage] || read_tag0[current_stage] == 0) 
        &&(good_tags[0][current_stage].count(obj.in_tag_area) != 0 
            || good_tags[1][current_stage].count(obj.in_tag_area) != 0 
            || obj.in_tag_area == border_tag[current_stage]);
        // }
        if(flag){
            disks[(disk_id-1)%origin_N+1].read_list[obj.saved_position[0][0]].push_back(temp_request);
            // read_head[disk_id+origin_N].read_list[obj.saved_position[0][0]].push_back(temp_request);
            time2request[timestamp].push_back(temp_request);
        }
        else{
            abort_read_list.push_back(temp_request);
            // if(timestamp <= MAX_TIME){
            //    for(int j = 0; j < 105; j++){
            //         bad_tag_request_count[timestamp + j]++;
            //     }
            // }
        }
    }
    // fprintf(file,"finish1111\n");
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
    // fprintf(file,"finish2222\n");
    if(timestamp % 1800 == 1){
        for(int i = 1; i <= N; i++){
            for(int j = 1; j <= V; j++){
                int obj_id = disks[(i-1)%origin_N+1].saved_object_id[j];
                bool flag = false;
                if(obj_id != 0){
                    flag = (read_tag0[current_stage] == 1 
                        && objects[obj_id-1].saved_position[0][0] > read_tag0_line[current_stage]);
                }
                if(obj_id != 0 
                && ((good_tags[0][current_stage].count(objects[obj_id-1].tag) == 0 
                && good_tags[1][current_stage].count(objects[obj_id-1].tag) == 0 
                && objects[obj_id-1].tag != border_tag[current_stage]
                && good_tags[0][current_stage].count(objects[obj_id-1].in_tag_area) == 0
                && good_tags[1][current_stage].count(objects[obj_id-1].in_tag_area) == 0
                && objects[obj_id-1].in_tag_area != border_tag[current_stage])
                || flag)){
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
    // fprintf(file,"finish3333\n");
    if (timestamp > 105) {
        for (int i = 0; i < time2request[timestamp-105].size(); i++){
            int disk_id = objects[time2request[timestamp-105][i].object_id-1].saved_disk[0];
            int position = objects[time2request[timestamp-105][i].object_id-1].saved_position[0][0];
            if (disks[(disk_id-1)%origin_N+1].read_list[position].size() > 0){
                for (int j = 0; j < disks[(disk_id-1)%origin_N+1].read_list[position].size(); j++){
                    if(disks[(disk_id-1)%origin_N+1].read_list[position][j].st_time <= timestamp - 105 && disks[(disk_id-1)%origin_N+1].read_list[position][j].is_abort == false && disks[(disk_id-1)%origin_N+1].read_list[position][j].is_done == false){
                        abort_read_list.push_back(disks[(disk_id-1)%origin_N+1].read_list[position][j]);
                        disks[(disk_id-1)%origin_N+1].read_list[position][j].is_abort = true;
                        Object obj = objects[disks[(disk_id-1)%origin_N+1].saved_object_id[position]-1];
                        // fprintf(file,"abort!!!!object_id:%d tag:%d in_tag_area:%d saved_position:%d\n", obj.id, obj.tag, obj.in_tag_area, position);
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

void read_action_2(){//读取对象
    int n_read;
    int request_id, object_id;
    // n_read = fast_read();
    n_read = read_record[timestamp].n_read;
    ////// fprintf(file,"n_read:%d\n",n_read);
    // cal_bad_tag_timely();
    for (int i = 1; i <= n_read; i++) {
        // request_id = fast_read();
        request_id = read_record[timestamp].request_ids[i-1];
        // object_id = fast_read();
        object_id = read_record[timestamp].object_ids[i-1];
        read_Request temp_request ;
        temp_request.request_id = request_id;
        temp_request.object_id = object_id;
        temp_request.is_done = false;
        temp_request.is_abort = false;
        temp_request.st_time = timestamp;
        id2request[request_id] = temp_request;

        Object obj = objects[temp_request.object_id-1];
        int disk_id = obj.saved_disk[0];
        // accumulated_read_size[obj.tag] += obj.size;
        if(obj.tag != 0 
        && ((good_tags[0][current_stage].count(obj.tag) != 0 || good_tags[1][current_stage].count(obj.tag) != 0 || obj.tag == border_tag[current_stage])
            ||(good_tags[0][current_stage].count(obj.in_tag_area) != 0 || good_tags[1][current_stage].count(obj.in_tag_area) != 0 || obj.in_tag_area == border_tag[current_stage]))){
            disks[(disk_id-1)%origin_N+1].read_list[obj.saved_position[0][0]].push_back(temp_request);
            // read_head[disk_id+origin_N].read_list[obj.saved_position[0][0]].push_back(temp_request);
            time2request[timestamp].push_back(temp_request);
        }
        else{
            abort_read_list.push_back(temp_request);
            // if(timestamp <= MAX_TIME){
            //    for(int j = 0; j < 105; j++){
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

void sort_this_tag_need_gc_obj(vector<int> &this_tag_need_gc_obj, int this_tag) {
    vector<pair<int,double>> sorted_order; // id distance
    for (int i = 0; i < this_tag_need_gc_obj.size(); i++) {
        int id = this_tag_need_gc_obj[i];
        double dis = 0;
        int embedding_length = timestamp/cluster_stage_length - objects[id-1].store_timestamp/cluster_stage_length + 1;
        int end_stage = timestamp/cluster_stage_length;
        if (objects[id-1].delete_timestamp != 0) {
            end_stage = objects[id-1].delete_timestamp/cluster_stage_length;
        }
        for(int j = objects[id-1].store_timestamp/cluster_stage_length; j <= end_stage; j++){
            dis += abs(objects[id-1].read_embedding[j] - cluster_core[this_tag][j]);
        }
        dis /= embedding_length;
        sorted_order.push_back(mkp(id, dis));
    }
    sort(sorted_order.begin(),sorted_order.end(),[](const pair<int,double>& a, const pair<int,double>& b){
        return a.second < b.second;
    });
    this_tag_need_gc_obj.clear();
    for(int i = 0; i < sorted_order.size(); i++){
        this_tag_need_gc_obj.push_back(sorted_order[i].first);
    }
}

void gc_action_1()
{
    scanf("%*s %*s");
    // tag0_object_write_size[0] = 0;
    vector<pair<int,int>> swap_position[MAX_DISK_NUM];
    for(int i = 1; i <= obj_num; i++){
        if(objects[i-1].delete_timestamp !=0){
            continue;
        }
    }
    int left_K[MAX_DISK_NUM];
    for(int i = 1; i <= origin_N; i++){
        left_K[i] = K;
    }
    vector<int> need_gc_obj;
    printf("GARBAGE COLLECTION\n");
    for(int i = 1 ; i <= obj_num; i++){//遍历所有对象，判断是否需要回收
        bool continue_flag = false;
        if(objects[i-1].delete_timestamp != 0){//如果对象已经被删除，则跳过
            continue_flag = true;
        }
        if (objects[i-1].tag != 0) {//如果对象有tag，则跳过
            continue_flag = true;
        }
        
        if(objects[i-1].in_tag_area > 0 && objects[i-1].in_tag_area == objects[i-1].temp_tag){//如果对象在tag区域，则跳过
            continue_flag = true;
        }
        
        if(read_head[objects[i-1].saved_disk[0]].position == objects[i-1].saved_position[0][0]){//如果读取头1在对象位置，则跳过
            continue_flag = true;
        }
        if(read_head[objects[i-1].saved_disk[0]+origin_N].position == objects[i-1].saved_position[0][0]){//如果读取头2在对象位置，则跳过
            continue_flag = true;
        }
        for (int cal = 0; cal < disks[objects[i-1].saved_disk[0]].read_list[objects[i-1].saved_position[0][0]].size(); cal++){
            if(disks[objects[i-1].saved_disk[0]].read_list[objects[i-1].saved_position[0][0]][cal].is_read == true){//如果正在读取，则跳过
                continue_flag = true;
                break;
            }
        }
        for (int cal = 0; cal < objects[i-1].size; cal++){
            if(read_head[objects[i-1].saved_disk[0]].target_position == objects[i-1].saved_position[0][cal]){//如果读取头1在对象位置，则跳过
                continue_flag = true;
                break;
            }
            if (read_head[objects[i-1].saved_disk[0] + origin_N].position == objects[i-1].saved_position[0][cal]){//如果读取头2在对象位置，则跳过
                continue_flag = true;
                break;
            }
            if(read_head[objects[i-1].saved_disk[0] + origin_N].target_position == objects[i-1].saved_position[0][cal]){//如果读取头2在对象位置，则跳过
                continue_flag = true;
                break;
            }
            if(read_head[objects[i-1].saved_disk[0]].position == objects[i-1].saved_position[0][cal]){//如果读取头1在对象位置，则跳过
                continue_flag = true;
                break;
            }
        }
        if(continue_flag == true){
            continue;
        }
        need_gc_obj.push_back(i);
    }

    vector<int> temp_need_gc_obj;
    int future_read_num[MAX_TAG_NUM] = {0};
    vector<pair<int,double>> future_read_tag;
    // for(int j = 1; j <= M; j++){
    //     if(current_stage > 2){
    //         future_read_num[j] = 2 * cluster_core[j][current_stage-1] - cluster_core[j][current_stage-2];
    //     }
    //     else{
    //         future_read_num[j] = 1;
    //     }
    //     future_read_tag.push_back(mkp(j,future_read_num[j]));
    //     // // fprintf(file,"core1:%f core2:%f\n",cluster_core[j][i-2],cluster_core[j][i-3]);
    // }
    for(int j = 0; j <= M; j++){
        if(timestamp < 1800){
            if(j == 0){
                future_read_num[j] = 1800;
            }
            else{
                future_read_num[j] = 1;
            }
        }
        else{
            int sum_read_num = 0;
            for(int k = timestamp - final_sword;k<=timestamp;k++){
                sum_read_num += round1_tags_read_num[k][j];
            }
            double temp_read_size = static_cast<double>(sum_read_num) / (final_sword + 1) * 1800;
            future_read_num[j] = temp_read_size;
            future_read_tag.push_back(mkp(j,future_read_num[j]));
        }
        // // fprintf(file,"core1:%f core2:%f\n",cluster_core[j][i-2],cluster_core[j][i-3]);
    }
    sort(future_read_tag.begin(),future_read_tag.end(),[](const pair<int,double>& a, const pair<int,double>& b){
        return static_cast<double>(a.second) /(static_cast<double>(round1_accumulated_write_size[a.first] + 1)) > static_cast<double>(b.second) /(static_cast<double>(round1_accumulated_write_size[b.first] + 1));
    });
    vector<int> scan_tag_order;
    for(int i = 0; i < future_read_tag.size(); i++){
        scan_tag_order.push_back(future_read_tag[i].first);
    }
    // // fprintf(file,"scan_tag_order:");
    // for(int i = 0; i < scan_tag_order.size(); i++){
    //     // fprintf(file,"%d ",scan_tag_order[i]);
    // }
    // // fprintf(file,"\n");
    // if(current_stage >10){
    //     fclose(file);
    // }
    // scan_tag_order = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    // 每个tag都扫所有need_gc_obj
    for (int i = 0; i < scan_tag_order.size(); i++){
        vector<int> this_tag_need_gc_obj;
        for (int j = 0; j < need_gc_obj.size(); j++){
            if (objects[need_gc_obj[j]-1].temp_tag == scan_tag_order[i]
            && objects[need_gc_obj[j]-1].tag == 0
            // && objects[need_gc_obj[j]-1].in_tag_area != scan_tag_order[i]
            ){
                this_tag_need_gc_obj.push_back(need_gc_obj[j]);
            }
        }
        sort_this_tag_need_gc_obj(this_tag_need_gc_obj, scan_tag_order[i]);
        for (int k = 0; k < this_tag_need_gc_obj.size(); k++) {
            temp_need_gc_obj.push_back(this_tag_need_gc_obj[k]);
        }
    }
    // for (int i = 0; i < need_gc_obj.size(); i++) {
    //     for (int j = 0; j < scan_tag_order.size(); j++) {
    //         vector<int> this_tag_need_gc_obj;
    //         if(objects[need_gc_obj[i]-1].temp_tag == scan_tag_order[j] && objects[need_gc_obj[i]-1].tag == 0) {
    //             this_tag_need_gc_obj.push_back(need_gc_obj[i]);
    //         }
    //         sort_this_tag_need_gc_obj(this_tag_need_gc_obj, scan_tag_order[j]);
    //         for (int k = 0; k < this_tag_need_gc_obj.size(); k++) {
    //             temp_need_gc_obj.push_back(this_tag_need_gc_obj[k]);
    //         }
    //     }
    // }
    for (int i = 0; i < temp_need_gc_obj.size(); i++){
        int obj_id = temp_need_gc_obj[i];
        int disk_id = objects[obj_id-1].saved_disk[0];
        if (left_K[disk_id] < objects[obj_id-1].size){
            continue;
        }

        int left = disks[disk_id].divided_area[objects[obj_id-1].temp_tag].second.first;
        int right = disks[disk_id].divided_area[objects[obj_id-1].temp_tag].second.second;
        vector<int> temp_position;
        if (tag2order[objects[obj_id-1].temp_tag] % 2 == 1){
            for(int j = left; j <= right; j++){ // 按照真实位置找一定连续的空位
                if (disks[disk_id].saved_object_id[j] == 0){
                    temp_position.push_back(j);
                }
                else{
                    temp_position.clear();
                }
                if (temp_position.size() >= objects[obj_id-1].size){
                    round1_accumulated_write_size[objects[obj_id-1].temp_tag] += objects[obj_id-1].size;
                    round1_accumulated_write_size[objects[obj_id-1].tag] -= objects[obj_id-1].size;
                    for(int k = 0; k < objects[obj_id-1].size; k++){
                        swap_position[disk_id].push_back(mkp(temp_position[k],objects[obj_id-1].saved_position[0][k]));
                    }
                    if(find(disks[disk_id].saved_obj_position[objects[obj_id-1].temp_tag].begin(),disks[disk_id].saved_obj_position[objects[obj_id-1].temp_tag].end(),temp_position[0]) == disks[disk_id].saved_obj_position[objects[obj_id-1].temp_tag].end()){
                        for(int k = 0;k<temp_position.size();k++){
                            disks[disk_id].saved_obj_position[objects[obj_id-1].temp_tag].push_back(temp_position[k]);
                        }
                    }
                    gc_update(disk_id,temp_position,obj_id);
                    objects[obj_id-1].in_tag_area = objects[obj_id-1].temp_tag;
                    left_K[disk_id] -= objects[obj_id-1].size;
                    break;
                }
            }
        }
        else{
            for(int j = right; j >= left; j--){
                if (disks[disk_id].saved_object_id[j] == 0){
                    temp_position.push_back(j);
                }
                else{
                    temp_position.clear();
                }
                if (temp_position.size() >= objects[obj_id-1].size){
                    sort(temp_position.begin(),temp_position.end()); // 按照位置排序
                    round1_accumulated_write_size[objects[obj_id-1].temp_tag] += objects[obj_id-1].size;
                    round1_accumulated_write_size[objects[obj_id-1].tag] -= objects[obj_id-1].size;
                    if(find(disks[disk_id].saved_obj_position[objects[obj_id-1].temp_tag].begin(),disks[disk_id].saved_obj_position[objects[obj_id-1].temp_tag].end(),temp_position[0]) == disks[disk_id].saved_obj_position[objects[obj_id-1].temp_tag].end()){
                        for(int k = 0;k<temp_position.size();k++){
                            disks[disk_id].saved_obj_position[objects[obj_id-1].temp_tag].push_back(temp_position[k]);
                        }
                    }
                    for(int k = 0; k < objects[obj_id-1].size; k++){
                        swap_position[disk_id].push_back(mkp(temp_position[k],objects[obj_id-1].saved_position[0][k]));
                    }
                    gc_update(disk_id,temp_position,obj_id);
                    objects[obj_id-1].in_tag_area = objects[obj_id-1].temp_tag;
                    left_K[disk_id] -= objects[obj_id-1].size;
                    break;
                }
            }
        }
    }
    // vector<int> need_gc_obj_sorted[MAX_DISK_NUM];
    // vector<int> scan_tag_order = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    // for(int i = 1; i <= origin_N; i++){//遍历所有磁盘，进行优先级排序
    //     for (int j = 0; j < scan_tag_order.size(); j++){
    //         for (int k = 0; k < need_gc_obj[i].size(); k++){
    //             if(objects[need_gc_obj[i][k]-1].temp_tag == scan_tag_order[j]){
    //                 need_gc_obj_sorted[i].push_back(need_gc_obj[i][k]);
    //             }
    //         }
    //     }
    // }
    for (int i = 1; i <= origin_N; i++) {
        printf("%d\n", swap_position[i].size());
        // //// fprintf(file, "size:%d\n", swap_position[i].size()+swap_position[i+10].size());
        for (int j = 0; j < swap_position[i].size(); j++){
            printf("%d %d\n",swap_position[i][j].first,swap_position[i][j].second);
            // //// fprintf(file, "position:%d %d\n",swap_position[i][j].first,swap_position[i][j].second);
        }
        swap_position[i].clear();
    }
    
    fflush(stdout);
}
void update_tag0_object_write_size(){
    memset(tag0_object_write_size,0,sizeof(tag0_object_write_size));
    for (int i = 1; i <= obj_num; i++) {
        if (objects[i-1].tag == 0 && objects[i-1].in_tag_area == 0 && objects[i-1].delete_timestamp == 0) {
            tag0_object_write_size[objects[i-1].temp_tag] += objects[i-1].size;
        }
    }
}
void gc_action_2()
{
    // scanf("%*s %*s");
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
                            //                 //// fprintf(file,"find_obj_id:%d found_id:%d\n",find_obj_id,found_id);
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
                            //                 //// fprintf(file,"find_obj_id:%d found_id:%d\n",find_obj_id,found_id);
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
        // //// fprintf(file, "size:%d\n", swap_position[i].size()+swap_position[i+10].size());
        for (int j = 0; j < swap_position[i].size(); j++){
            printf("%d %d\n",swap_position[i][j].first,swap_position[i][j].second);
            // //// fprintf(file, "position:%d %d\n",swap_position[i][j].first,swap_position[i][j].second);
        }
        swap_position[i].clear();
        for (int j = 0; j < swap_position[i+10].size(); j++){
            printf("%d %d\n",swap_position[i+10][j].first,swap_position[i+10][j].second);
            // //// fprintf(file, "position:%d %d\n",swap_position[i+10][j].first+V,swap_position[i+10][j].second+V);
        }
        swap_position[i+10].clear();
    }
    
    fflush(stdout);
}


//---------------------------------complex function---------------------------------
void preprocess_1(){
    T = fast_read();
    M = fast_read();
    // M += 1;
    origin_N = fast_read();
    N = origin_N*2;
    V = fast_read();
    G = fast_read();
    K1 = fast_read();
    K2 = fast_read();
    K = K1;

    // for (int i = 1; i <= M; i++) {//计算每个tag总共删除的大小
    //     for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
    //         del_size[i][j] = fast_read();
    //         accumulated_write_size[i][j] -= 0.3 * del_size[i][j];
    //         tot_del_size[i] += del_size[i][j];
    //     }
    // }

    // for (int i = 1; i <= M; i++) {//计算每个tag总共写入的大小
    //     int writed_num = 0;
    //     for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
    //         write_size[i][j] = fast_read();
    //         writed_num += write_size[i][j];
    //         accumulated_write_size[i][j] += writed_num;
    //         if(i == 1){
    //             write_size[i][j] *= mul1;
    //         }
    //         if(i == 2){
    //             write_size[i][j] *= mul2;
    //         }
    //         if(i == 3){
    //             write_size[i][j] *= mul3;
    //         }
    //         if(i == 4){
    //             write_size[i][j] *= mul4;
    //         }
    //         if(i == 5){
    //             write_size[i][j] *= mul5;
    //         }
    //         if(i == 6){
    //             write_size[i][j] *= mul6;
    //         }
    //         if(i == 7){
    //             write_size[i][j] *= mul7;
    //         }
    //         if(i == 8){
    //             write_size[i][j] *= mul8;
    //         }
    //         if(i == 9){
    //             write_size[i][j] *= mul9;
    //         }
    //         if(i == 10){
    //             write_size[i][j] *= mul10;
    //         }
    //         if(i == 11){
    //             write_size[i][j] *= mul11;
    //         }
    //         if(i == 12){
    //             write_size[i][j] *= mul12;
    //         }
    //         if(i == 13){
    //             write_size[i][j] *= mul13;
    //         }
    //         if(i == 14){
    //             write_size[i][j] *= mul14;
    //         }
    //         if(i == 15){
    //             write_size[i][j] *= mul15;
    //         }
    //         if(i == 16){
    //             write_size[i][j] *= mul16;
    //         }

    //         tot_write_size[i] += write_size[i][j];
    //     }
    // }
    // for (int i = 1; i <= M; i++) {//计算每个tag总共读取的大小
    //     for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
    //         read_size[i][j] = fast_read();
    //         tot_read_size[i] += read_size[i][j];
    //     }
    // }
    // for(int i = 1; i <= (T - 1) / FRE_PER_SLICING + 2 ; i++){
    //     adding_G[i] = fast_read();
    // }
    for(int i=0;i<=N;i++){
        Disk disk;
        disk.id = i;
        disk.size = V;
        disks.push_back(disk);
    }
    cal_tag2order();
    divide_disks_1();
    // cal_good_tags();
    for(int i = 1; i <= N; i++){
        read_head[i].disk_id = i;
        read_head[i].position = 1;
        read_head[i].leftg = G + adding_G[1];
        read_head[i].target_position = 0;
    } 
    ////// fprintf(file,"OK\n");
    printf("OK\n");
    fflush(stdout);
}
void preprocess_2(){
    K = K2;
    for(int i = 1; i <= T; i++){
        for(int j = 1; j<=read_record[i].n_read;j++){
            read_size[objects[read_record[i].object_ids[j-1]-1].tag][(i-1)/FRE_PER_SLICING+1] = 0;
        }
    }
    current_stage = 0;
    cal_stage_info();
    for(int i = 0;i<=T+EXTRA_TIME;i++){
        time2request[i].clear();
    }
    disks.clear();
    for(int i=1;i<=obj_num;i++){
        objects[i-1].saved_disk[0] = 0;
        objects[i-1].saved_disk[1] = 0;
        objects[i-1].saved_disk[2] = 0;
        objects[i-1].saved_position[0].clear();
        objects[i-1].saved_position[1].clear();
        objects[i-1].saved_position[2].clear();
    }
    for(int i = 1;i<=(T - 1) / FRE_PER_SLICING + 1;i++){
        good_tags[0][i].clear();
        good_tags[1][i].clear();
        border_tag[i] = 0;
        border_line[i] = 0;
        read_tag0[i] = 0;
        read_tag0_line[i] = 0;
    }
    for (int i = 1; i <= M; i++) {//计算每个tag总共删除的大小
        int deleted_num = 0;
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            // del_size[i][j] = fast_read();
            deleted_num += del_size[i][j];
            accumulated_write_size[i][j] -= deleted_num;
            tot_del_size[i] += del_size[i][j];
        }
    }
    

    for (int i = 1; i <= M; i++) {//计算每个tag总共写入的大小
        int writed_num = 0;
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            // write_size[i][j] = fast_read();
            writed_num += write_size[i][j];
            accumulated_write_size[i][j] += writed_num;
            accumulated_write_size_without_delete[i][j] += writed_num;
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

    // for(int i = 1; i <= M; i++){
    //     // fprintf(file,"tot_write_size[%d]:%d\n",i,tot_write_size[i]);
    //     // fprintf(file,"tot_del_size[%d]:%d\n",i,tot_del_size[i]);
    //     for(int j = 1; j <= (T-1)/FRE_PER_SLICING+1; j++){
    //         // fprintf(file,"write_size[%d][%d]:%d\n",i,j,write_size[i][j]);
    //         // fprintf(file,"accumulated_write_size[%d][%d]:%d\n",i,j,accumulated_write_size[i][j]);
    //         // fprintf(file,"del_size[%d][%d]:%d\n",i,j,del_size[i][j]);
    //     }
    // }

    for (int i = 1; i <= M; i++) {//计算每个tag总共读取的大小
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            // read_size[i][j] = fast_read();
            tot_read_size[i] += read_size[i][j];
        }
    }
    // for(int i = 1; i <= M; i++){
    //     // fprintf(file,"tot_read_size[%d]:%d\n",i,tot_read_size[i]);
    //     for(int j = 1; j <= (T-1)/FRE_PER_SLICING+1; j++){
    //         // fprintf(file,"read_size[%d][%d]:%d\n",i,j,read_size[i][j]);
    //     }
    // }
    // fclose(file);
    for(int i=0;i<=N;i++){
        Disk disk;
        disk.id = i;
        disk.size = V;
        disks.push_back(disk);
    }
    // Simulated_Annealing();
    greedy();
    cal_tag2order();
    divide_disks_2();
    // cal_good_tags();
    for(int i = 1; i <= N; i++){
        read_head[i].disk_id = i;
        read_head[i].position = 1;
        read_head[i].leftg = G + adding_G[1];
        read_head[i].target_position = 0;
        read_head[i].last_read = false;
        read_head[i].last_cost = 0;
    } 
    ////// fprintf(file,"OK\n");
    // printf("OK\n");
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
        // //// fprintf(file,"timestamp:%d bad_density:%f good_density:%f\n",i,bad_density,good_density);
    }
}
//---------------------------------preprocess---------------------------------
int main()//主函数
{
    
    // file = fopen("output.txt","w");
    

    preprocess_1();
    
    int origin_G = G;
    for (int t = 1; t <= EXTRA_TIME + T; t++) {
        current_stage = static_cast<int>(ceil(t / 1800.0));
        init1();
        timestamp_action();
        if(timestamp % 1800 == 1){
            cal_good_tags1(current_stage);
        }
        delete_action_1();
        write_action_1();
        read_action_1();
        if(t % cluster_stage_length == 0){
            update_cluster_core();
            for(int i = 1; i <= M; i++){
                // for(int j = 0; j < timestamp / cluster_stage_length; j++){
                    // // fprintf(file,"cluster_core[%d][%d]:%f\n",i,j,cluster_core[i][j]);
                // }
            }
            for (int i = 1; i <= obj_num; i++){
                if (objects[i-1].tag == 0 && objects[i-1].delete_timestamp == 0){
                    objects[i-1].temp_tag = find_nearest_cluster_core(i);
                    // // fprintf(file,"object_id:%d temp_tag:%d\n",i,objects[i-1].temp_tag);
                }
            }
        }
        if (t % FRE_PER_SLICING == 0) {
            gc_action_1();
            update_tag0_object_write_size();
            update_cluster0_core();
        }
        // if(current_stage>45){
        //     fclose(file);
        // }
    }
    
    int tag_num[MAX_TAG_NUM] = {0};
    // // fprintf(file,"obj_num:%d\n",obj_num);
    for(int i = 1; i <= obj_num; i++){
        // if(objects[i-1].need_voted == 0){
        //     tag_num[objects[i-1].tag]++;
        // }
        // double max_tickets = 0;
        // int max_tag = 0;
        // // // fprintf(file,"object_id:%d\n",i);
        // for(int j = 1; j <= M; j++){
        //     // // fprintf(file,"tag:%d tickets:%f\n",j,objects[i-1].voted_boxs[j]);
        //     if(objects[i-1].voted_boxs[j] > max_tickets){
        //         max_tickets = objects[i-1].voted_boxs[j];
        //         max_tag = j;
        //     }
        // }
        // if(max_tickets < 10 ){
        //     max_tag = 0;
        // }
        // objects[i-1].tag = objects[i-1].temp_tag;
        // // fprintf(file,"%d %d\n",i,objects[i-1].tag);
    }
    for(int i = 1;i<=(T - 1) / FRE_PER_SLICING + 1;i++){
        // fprintf(file,"tag0_object_read_size[%d]:%d\n",i,tag0_object_read_size[i]);
    }
    // fclose(file);
    int tag0_num = 0;
    incre_action();
    update_cluster_core();
    for (int i = 1; i <= obj_num; i++){
        if (objects[i-1].tag == 0 && objects[i-1].delete_timestamp == 0){
            objects[i-1].temp_tag = find_nearest_cluster_core(i);
            // // fprintf(file,"object_id:%d temp_tag:%d\n",i,objects[i-1].temp_tag);
        }
    }
    
    tag_num[MAX_TAG_NUM] = {0};
    // // fprintf(file,"obj_num:%d\n",obj_num);
    for(int i = 1; i <= obj_num; i++){
        if(objects[i-1].tag == 0){
            objects[i-1].tag = objects[i-1].temp_tag;
        }
        if(objects[i-1].read_num < 15){
            objects[i-1].tag = 0;
        }
        // // fprintf(file,"%d %d\n",i,objects[i-1].tag);
    }
    for(int i = 1; i <= M; i++){
        // // fprintf(file,"tag:%d num:%d\n",i,tag_num[i]);
    }
    preprocess_2();
    for (int t = 1; t <= EXTRA_TIME + T; t++){
        current_stage = static_cast<int>(ceil(t / 1800.0));
        init2();
        timestamp_action();
        if(timestamp % 1800 == 1){
            cal_good_tags2(current_stage);
        }
        delete_action_2();
        // auto now = std::chrono::system_clock::now();
        // auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // // fprintf(file,"delete_action后当前时间戳（毫秒）: :%d\n",time);
        
        write_action_2();
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // // fprintf(file,"write后当前时间戳（毫秒）: :%d\n",time);
        read_action_2();
        // now = std::chrono::system_clock::now();
        // time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // // fprintf(file,"read后当前时间戳（毫秒）: :%d\n",time);
        // count_tag_size();
        if (t % FRE_PER_SLICING == 0) {
            gc_action_2();
        }
    }
    // fclose(file);
    // print_density();
    return 0;
}
//---------------------------------main function---------------------------------