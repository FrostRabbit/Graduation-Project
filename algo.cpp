#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <array>
#include <time.h> 
#include <algorithm>
using namespace std;
using index_t = std::vector<int>::size_type;

#define N 53 //0代表depot, 1~N代表顧客點
#define TRUCK_AMOUNT 12	//1~7代表非加班車
#define C 600 //車子材積
#define mu 0.95f //可允許容積的最大使用率
#define M 9999 //極大值
#define T 240 //非加班最大工時
#define O 180 //加班最大工時
#define w 3.67 //時薪率 (每分鐘的薪水)(omega)
#define f 5.62f //油耗
#define g 25.8f //油價
#define R 7	//分區
#define P 4	//順位
#define A 1000000//alpha



int nearest_centroid(double x[],int size);//找離質心最近的客戶點
int nearest_node(float x[],vector<int> r,int size);//從分區r中尚未加入到初始路徑的客戶中，離x最近的客戶點
int feasible(int vehicle,vector<int> route,int size,float c[N+1][N+1],double *q,float *sv);//判斷車輛路徑是否符合限制
void insert(int vehicle,vector<int> route[],int size,int size1,int priority,int f_index,int d[R][P],double h[N][R],float t[N+1][N+1]);
int largest_demand(vector<int> route,int size,int f_index,double *q);
void show_route(vector<int> route[],int size);
void show_load(vector<int> route[],int size,double *q);
float cal_obj(array<int,TRUCK_AMOUNT+N> route,float c[N+1][N+1],float t[N+1][N+1],float *sv,int *p,int d[R][P],int *G);


int main() {

	vector<int> route[TRUCK_AMOUNT];//實際車輛路徑
	vector<int> route_current[2];//測試用車輛路徑
	vector<int> remain_node;//剩餘非第一優先順序司機服務的客戶
	vector<int> remain_veh;//剩餘非第一優先順序司機服務的客戶之第一優先司機/車輛
	array<int,TRUCK_AMOUNT> isfeasible;//每條路徑是否滿足條件
	array<int,TRUCK_AMOUNT+N> join_route;//將所有車輛路徑vector轉換成一維陣列
	array<float,TRUCK_AMOUNT> v_demand;//每條路徑的總需求量
	array<float,TRUCK_AMOUNT> v_time;//每條路徑的總旅行時間和服務時間

	int len =0;
	int s = 0;
	int s1 = 0;
	int size = 0;


	int size1 =0;
	int f_index = 1;
	int support = 0;

	int flag = 0;
	int larger = 0;
	int v=0;
	int node;

	//旅行時間
	fstream cij;
	cij.open("D:/graduated project/final data/virtual/pm/Duration.txt", ios::in);

	float c[N + 1][N + 1];
	for (int i = 0; i <= N; i++)
	{
		for (int j = 0; j <= N; j++)
		{
			cij >> c[i][j];
		}
	}

	//旅行距離
	fstream tij;
	tij.open("D:/graduated project/final data/virtual/pm/Distance.txt", ios::in);

	float t[N + 1][N + 1];
	for (int i = 0; i <= N; i++)
	{
		for (int j = 0; j <= N; j++)
		{
			tij >> t[i][j];
		}
	}

	//總材積
	fstream qi;
	qi.open("D:/graduated project/final data/virtual/pm/TotalVolume.txt", ios::in);

	double q[N + 1];
	for (int i = 0; i <= N; i++)
	{
		qi >> q[i];
	}

	//總服務時間
	fstream svi;
	svi.open("D:/graduated project/final data/virtual/pm/ServiceTime.txt", ios::in);

	float sv[N + 1];
	for (int i = 0; i <= N; i++)
	{
		svi >> sv[i];
	}


	//分區
	fstream Gri;
	Gri.open("D:/graduated project/final data/virtual/pm/District.txt", ios::in);
	vector<int> Gr[R];
	int G[N+1];
	int dis;
	for (int i = 0; i <= N; i++)
	{
		Gri >> dis;
		G[i] = dis;
		switch (dis)
		{
		case 1:
			Gr[0].push_back(i);
			break;
		case 2:
			Gr[1].push_back(i);
			break;
		case 3:
			Gr[2].push_back(i);
			break;
		case 4:
			Gr[3].push_back(i);
			break;
		case 5:
			Gr[4].push_back(i);
			break;
		case 6:
			Gr[5].push_back(i);
			break;
		case 7:
			Gr[6].push_back(i);
			break;
		default:
			break;
		}
	}

	cout <<"各區包含的顧客點:"<<endl;
	cout <<"----------------------------------------------";
	for (int i =0; i<R;i++ ){
		cout <<endl<<"區"<<i+1<<": ";
		for (index_t j =0;j<Gr[i].size();j++){
			cout << Gr[i][j]<<" ";
		}
	}
	cout <<endl<<"----------------------------------------------"<<endl;


	//順位
	fstream drp;
	drp.open("D:/graduated project/final data/virtual/pm/Priority.txt", ios::in);
	int d[R][P];
	for (int r = 0; r < R; r++)
	{
		for (int p = 0; p < P; p++)
		{
			drp >> d[r][p];
		}
	}

	//懲罰值
	fstream p;
	p.open("D:/graduated project/final data/virtual/pm/Penalty1.txt", ios::in);
	int penalty[P+1];
	for (int i = 0; i <= P; i++)
	{
		p >> penalty[i];
	}


	//質心距離
	fstream hrj;
	hrj.open("D:/graduated project/final data/virtual/pm/Centriod.txt", ios::in);
	double h[N][R];
	for (int r = 0; r < N; r++)
	{
		for (int j = 0; j < R; j++)
		{
			hrj >> h[r][j];
		}
	}


	cout <<endl<< "將顧客點分配給第一優先順序車輛:"<<endl;
	cout <<"----------------------------------------------";

	for (int i = 0;i<TRUCK_AMOUNT;i++){
		route[i].push_back(0);
	}
	
    for (int i = 0;i < R; i++){
		if (Gr[i].empty()){
			continue;
		}
        len = (int)Gr[i].size();
        s = nearest_centroid(h[i],(int)Gr[i].size());
		vector<int>::iterator it = Gr[i].begin();
		route[i].push_back(Gr[i][(index_t)s]);
		s1 = Gr[i][(index_t)s];
		if (s == len-1){
			Gr[i].pop_back();
		}
		else{
			Gr[i].erase(it+(index_t)s);
		}
		
		size = (int)Gr[i].size();
        for (int j = 0;j < size;j++){
			s = nearest_node(t[s1],Gr[i],(int)Gr[i].size());
			s1 = Gr[i][(index_t)s];
			route[i].push_back(s1);

			if (s == (int)Gr[i].size()-1){
				Gr[i].pop_back();
			}
			else{
				Gr[i].erase(it+s);
			}
        }
    }

	for (int i =0;i<R;i++){
		route[i].push_back(0);
	}



	for (int i =0;i<TRUCK_AMOUNT;i++){
		cout <<endl<<"車"<<i+1<<": ";
		if(i<R) vector<int>().swap(Gr[i]); //釋放記憶體
		for (int j =0 ;j < (int)route[i].size();j++){
			cout << route[i][(index_t)j]<<" ";
		}
	}
	cout <<endl<<"----------------------------------------------"<<endl;

	cout <<endl<< "判斷目前哪些車不滿足限制:"<<endl;
	cout <<"----------------------------------------------"<<endl;
	for (size_t i = 0;i<(size_t)TRUCK_AMOUNT;i++){
		isfeasible[i] = feasible((int)i,route[i],(int)route[i].size(),c,q,sv);
		cout<<"車"<<i+1<<": "<<isfeasible[i]<<endl;
	}
	cout <<"----------------------------------------------"<<endl;



	//判斷每台車是否需要支援

	for (int i = 0;i<R;i++){
		support = 0;
		if (isfeasible[(size_t)i] == 0)
		{
			//使用非加班車支援
			cout <<endl<<"*車"<<i+1<<"需要支援*"<<endl;
			for (int priority = 2;priority<=P;priority++){
				support = d[i][priority-1]-1;
				if (isfeasible[(size_t)support] == 0){
					cout <<endl<<"*車"<<support+1<<"已滿*"<<endl;
					continue;
				}
				while (isfeasible[(size_t)i] != 1){
					size = (int)route[i].size();
					size1 = (int)route[support].size();
					f_index = 1;
					while (f_index < size-1){
						route_current[0].assign(route[i].begin(),route[i].end());
						route_current[1].assign(route[support].begin(),route[support].end());
						cout <<endl<<"車"<<i+1<<" -> 車"<<support+1<<"  第"<<f_index<<"近: ";
						insert(i,route_current,size,size1,priority,f_index,d,h,t);
						show_route(route_current,2);

						if(feasible(i,route_current[1],(int)route_current[1].size(),c,q,sv) != 0){
							cout <<"插入完成!"<<endl;
							route[i].assign(route_current[0].begin(),route_current[0].end());
							route[support].assign(route_current[1].begin(),route_current[1].end());
							if (feasible(i,route[i],(int)route[i].size(),c,q,sv) != 0) {
								isfeasible[(size_t)i] = 1;
								cout <<"[車"<<i+1<<"已滿足限制]"<<endl;
							}
							break;
						} 
						cout <<"插入失敗!"<<endl;
						f_index++;
					}

					if (f_index == size-1){
						break;
					}
						
				} 
			}


			//使用加班車支援
			while (isfeasible[(size_t)i] != 1){
				v = 0;
				while (v < TRUCK_AMOUNT-R){
					flag = 0;

					for (int k = 1; k < (int)route[i].size()-1;k++){

						cout <<endl<<"*區"<<i+1<<"使用加班車"<<v+1<<"*"<<endl;

						route_current[0].assign(route[i].begin(),route[i].end());
						route_current[1].assign(route[R+v].begin(),route[R+v].end());

						larger = largest_demand(route_current[0],(int)route_current[0].size(),k,q);

						cout <<"第"<<k<<"大需求點: "<<route_current[0][(size_t)larger]<<" "<<q[route_current[0][(size_t)larger]]<<endl;

						route_current[1].push_back(route_current[0][(size_t)larger]);
						route_current[0].erase(route_current[0].begin()+larger);

						show_route(route_current,2);

						if (feasible(v+R,route_current[1],(int)route_current[1].size(),c,q,sv) !=0){
							cout <<"插入完成!"<<endl;
							route[i].assign(route_current[0].begin(),route_current[0].end());
							route[v+R].assign(route_current[1].begin(),route_current[1].end());
							flag = 1;
							if (feasible(i,route[i],(int)route[i].size(),c,q,sv) != 0){
								isfeasible[(size_t)i] = 1;
								cout <<"[車"<<i+1<<"已滿足限制]"<<endl;
							}
							break;
						}

						cout <<"插入失敗!"<<endl;

					}

					if (flag == 0){
						v++;
					}
					else{
						break;
					}

				}
			}
		}
	}

	for (int i =R;i<TRUCK_AMOUNT;i++){
		route[i].push_back(0);
	}

	cout <<endl<<"Initial solution:"<<endl;
	show_route(route,TRUCK_AMOUNT);
	
	int count=0;
	for (int i =0;i<TRUCK_AMOUNT;i++){
		for (int j = 0;j<(int)route[i].size()-1;j++){
			join_route[(size_t)count] = route[i][(size_t)j];
			v_demand[i] += q[route[i][(size_t)j]];
			v_time[i] += sv[route[i][(size_t)j]] + c[route[i][(size_t)j]][route[i][(size_t)j+1]];

			if(!(G[join_route[(size_t)count]] == i+1 || join_route[(size_t)count] == 0)){
				remain_node.push_back(join_route[(size_t)count]);
				remain_veh.push_back(i);
			}
			count++;	
		}
		
	}

	cout <<endl;

	float result = cal_obj(join_route,c,t,sv,penalty,d,G);

	cout<<"objective_value: "<<result<<endl;
	cout<<"-----------------------------------"<<endl<<endl;

	cout <<"非第一順序服務的客戶:"<<endl;
	cout<<"-----------------------------------"<<endl;
	for (int i : remain_node){
		cout << i << " ";
	}
	cout<<endl<<"-----------------------------------"<<endl;


	//將非第一優先順序司機服務的客戶加回第一優先順序的車輛路徑
	float new_demand;
	float distance;
	float new_distance;
	float new_time;
	size = (int)remain_node.size();
	for(int i = 0; i<size;i++){
		count =0;
		s=0;
		s1=0;
		v=0;
		new_time =0;
		new_demand = M;
		distance = M;
		for (int j : remain_node){
			if (q[j] < new_demand){
				new_demand = q[j];
				node = j;
				s = count;
			}
			count++;
		}
		v = G[node];
		if (v_demand[v-1] + q[node] > C*mu){
			remain_node.erase(remain_node.begin()+s);
			remain_veh.erase(remain_veh.begin()+s);
			continue;
		}


		for (int j=0;j<(int)route[v-1].size()-1;j++){
			new_distance = t[route[v-1][j]][node] + t[node][route[v-1][j+1]] - t[route[v-1][j]][route[v-1][j+1]];
			if (new_distance < distance){
				distance = new_distance;
				s1 = j+1;
			}
		}

		new_time += v_time[v-1] + sv[node] + c[route[v-1][s1]][node] +\
		c[node][route[v-1][s1+1]] - c[route[v-1][s1]][route[v-1][s1+1]];

		if(new_time < T){
			v_time[v-1] = new_time;
			v_demand[v-1] += q[node];

			route[v-1].insert(route[v-1].begin()+s1,node);
			count =0;
			for(int j:route[remain_veh[s]]){
				if(j == node) route[remain_veh[s]].erase(route[remain_veh[s]].begin()+count);
				count++;
			}
		}
		remain_node.erase(remain_node.begin()+s);
		remain_veh.erase(remain_veh.begin()+s);
	}
	cout <<endl<<"New solution:"<<endl;
	show_route(route,TRUCK_AMOUNT);
	show_load(route,TRUCK_AMOUNT,q);


	count = 0;
	for (int i =0;i<TRUCK_AMOUNT;i++){
		for (int j = 0;j<(int)route[i].size()-1;j++){
			join_route[(size_t)count] = route[i][(size_t)j];
			count++;
		}
	}
	
	cout <<endl<<"將各路徑編碼成單一串列:"<<endl;
	for (int i : join_route){
		cout<<i<<" ";
	}
	cout <<endl;

	result = cal_obj(join_route,c,t,sv,penalty,d,G);

	cout<<"objective_value: "<<result<<endl;
	cout<<"-----------------------------------"<<endl;

	cout <<"runtime: " << (double)clock()/CLOCKS_PER_SEC << "s"<<endl;

	//釋放記憶體
	for (int i = 0 ; i<TRUCK_AMOUNT ; i++){
		vector<int>().swap(route[i]);
	}
	vector<int>().swap(route_current[0]);
	vector<int>().swap(route_current[1]);
	vector<int>().swap(remain_node);
	vector<int>().swap(remain_veh);

	system("pause");
    return 0;
}



int nearest_centroid(double x[],int size){

    int y=0;
    double distance = x[0];

    for (int i = 1; i < size; i++){
        if (x[i] < distance){
            y = i;
            distance = x[i];
        }
    }

    return y;
}

int nearest_node(float x[],vector<int> r,int size){

    int y=0;
    float distance = x[r[0]];
    for (int i = 1; i < size; i++){
        if (x[r[(index_t)i]] < distance){
            y = i;
            distance = x[r[(index_t)i]];
        }
    }
    return y;
}

int feasible(int vehicle,vector<int> route,int size,float c[N+1][N+1],double *q,float *sv){
	float time;
	float total_time = 0;
	double total_demand = 0;

	if (vehicle >= R){
		time = O;
	}
	else{
		time = T;
	}

	if (size > 2){
		for (index_t i = 1;i < (index_t)size;i++){
			total_time += c[route[i-1]][route[i]];
			total_time += sv[route[i]];
			total_demand += q[route[i]];
		}
	}

	if (total_time > time || total_demand > C*mu){
		return 0;
	}
	else{
		return 1;
	}
}


void insert(int vehicle,vector<int> route[],int size,int size1,int priority,int f_index,int d[R][P],double h[N][R],float t[N+1][N+1]){

	int new_vehicle = d[vehicle][priority-1]-1;
	int node_x = 0;
	int node_y = 0;
	int pos_x = 0;
	double distance = 0;
	double new_distance=0;
	vector<double> alt;//可用來插入支援車輛的客戶的需求量

	for (int i =1;i<size-1;i++){
		alt.push_back(h[route[0][(size_t)i]-1][new_vehicle]);
	}

	//將需求量排序
	sort(alt.begin(),alt.end());
	distance=alt[(size_t)f_index-1];//第f_index小的需求量

	//找出符合該需求量的客戶
	for (size_t i =1 ; i < (size_t) size-1;i++){
		if (h[route[0][i]-1][new_vehicle]==distance){
			node_x = route[0][i];
			pos_x = (int)i;
			cout<<node_x<<" ";
			cout<<distance<<endl;
		} 
	}

	distance = 99999;

	//找最適合插入的位置
	for (size_t i = 1; i < (size_t) size1 ; i++){
		new_distance = t[route[1][i-1]][node_x] + t[node_x][route[1][i]] - t[route[1][i-1]][route[1][i]];

		if (distance > new_distance){
			node_y = (int)i;
			distance = new_distance;
		}
	}

	
	route[1].emplace(route[1].begin()+node_y,node_x);
	route[0].erase(route[0].begin()+pos_x);
	
}

void show_route(vector<int> route[],int size){
	cout<<"-----------------------------------"<<endl;
	for (int i =0 ;i < size;i++){
		cout<<i+1<<". ";
		for (int j : route[i]){
			cout << j<<" ";
		}
		cout<<endl;
	}
	cout<<"-----------------------------------"<<endl;
}

void show_load(vector<int> route[],int size,double *q){
	cout<<endl<<"各車裝載量"<<endl;
	cout<<"-----------------------------------"<<endl;
	float sum;
	float percent;
	for (int i =0 ;i < size;i++){
		sum = 0;
		percent = 0;
		cout<<i+1<<". ";
		for (int j : route[i]){
			sum += q[j];
		}
		percent = sum/(C*mu)*100.0;
		cout<<sum<<" "<<percent<<"%"<<endl;
	}
	cout<<"-----------------------------------"<<endl;
}

int largest_demand(vector<int> route,int size,int f_index,double *q){
	int node = 0;
	double node_demand=0;
	vector<double> demand={0};

	for(int i =1;i<size-1;i++){
		demand.push_back(q[route[(size_t)i]]);
	}

	sort(demand.begin(),demand.end(),greater<double>());
	node_demand = demand[(size_t)f_index-1];

	for(int i =1;i<size-1;i++){
		if(q[route[(size_t)i]] == node_demand){
			node = i;
			break;
		}
	}
	return node;
}


float cal_obj(array<int,TRUCK_AMOUNT+N> route,float c[N+1][N+1],float t[N+1][N+1],float *sv,int *p,int d[R][P],int *G){

	int origin_node = 0;
	int overtime_node =0;
	float overtime_cost = 0;
	double sa = 3.67;
	float traveling_cost = 0;
	float penalty_cost = 0;
	float first_reward = 0;
	int penalty[R] = {0};
	int new_penalty = 0;
	int district=0;

	for (int i = 0; i<TRUCK_AMOUNT+N-1;i++){
		//計算總旅行距離
		if(i!=0&&!(route[(size_t)i-1]==0 && route[(size_t)i]==0)) traveling_cost += t[route[(size_t)i-1]][route[(size_t)i]];
		if (route[(size_t)i] == 0){
			origin_node++;
			if (origin_node > R) {
				overtime_node = i;
				break;
			}
			continue;
		}
		district = G[route[(size_t)i]]-1;

		//計算各區的懲罰值及獎勵值
		for (int j = 0 ;j < P;j++){
			if (d[district][j] == origin_node){
				if (j ==0){
					first_reward += A;
				}
				else{
					new_penalty = p[j];
					if (new_penalty > penalty[district]){
						penalty[district] = new_penalty;
					}
				}
			}
		}
		
	}

	//計算加班車的旅行時間
	for (int i = overtime_node+1;i<TRUCK_AMOUNT+N;i++){
		if (route[(size_t)i] == 0){
			if (origin_node >= TRUCK_AMOUNT) break;
			origin_node++;
			
		}
		if (route[(size_t)i]!=0){
			penalty[G[route[(size_t)i]]-1] = p[P];
		}
		if (route[(size_t)i-1]!=route[(size_t)i]){
			
			overtime_cost += c[route[(size_t)i-1]][route[(size_t)i]];
			overtime_cost += sv[route[(size_t)i]];
		}
	}

	cout<<endl<<"各區懲罰值: "<<endl;
	cout<<"-----------------------------------"<<endl;
	for (int i = 0; i<R;i++){
		penalty_cost += (float)penalty[(size_t)i];
		cout<<i+1<<": "<<penalty[i]<<endl;
	}
	cout<<"-----------------------------------"<<endl;

	traveling_cost = traveling_cost / f * g;
	overtime_cost = overtime_cost*sa;

	cout<<endl<<"目標值: "<<endl;
	cout<<"-----------------------------------"<<endl;
	cout<<"Traveling_cost: "<<traveling_cost<<endl;
	cout<<"Overtime_cost: "<<overtime_cost<<endl;
	cout<<"Penalty for priority: "<<penalty_cost<<endl;
	cout<<"Penalty for usage rate: "<<first_reward<<endl;
	return traveling_cost+overtime_cost+penalty_cost-first_reward;
}
