#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "gurobi_c++.h"	//將Gurobi載入

using namespace std;

#define H 53 //22 27 40 44 53
#define N 53 //0代表depot, 1~N代表顧客點
#define TRUCK_AMOUNT 12	//1~4代表非加班車
#define C 600 //車子材積
#define mu 0.95 //可允許容積的最大使用率
#define M 9999 //極大值
#define T 240 //非加班最大工時
#define O 180 //加班最大工時
#define w 3.67 //時薪率 (每分鐘的薪水)(omega)
#define f 5.62 //油耗
#define g 25.8 //油價
#define R 7	//分區
#define P 5	//順位

/*
報告書表11中的結果為調整此處之參數alpha
原測試值、測試值(iii)、測試值(iv)為100000
測試值(i)為1000000
測試值(ii)為10000
*/
#define A 1000000 //使用率懲罰值(alpha)


int route_begin[TRUCK_AMOUNT][H];
int route_end[TRUCK_AMOUNT][H];
int route_num[TRUCK_AMOUNT];

string itos(int i)
{
	stringstream s;
	s << i;
	return s.str();
}

int main(int argc, char* argv[])
{
	string s;


	//旅行時間
	fstream cij;
	cij.open("D:/graduated project/final data/virtual/pm/Duration.txt", ios::in);

	double c[H + 1][H + 1];
	for (int i = 0; i <= H; i++)
	{
		for (int j = 0; j <= H; j++)
		{
			cij >> c[i][j];
		}
	}

	//旅行距離
	fstream tij;
	tij.open("D:/graduated project/final data/virtual/pm/Distance.txt", ios::in);

	double t[H + 1][H + 1];
	for (int i = 0; i <= H; i++)
	{
		for (int j = 0; j <= H; j++)
		{
			tij >> t[i][j];
		}
	}

	//總材積
	fstream qi;
	qi.open("D:/graduated project/final data/virtual/pm/TotalVolume.txt", ios::in);

	double q[H + 1];
	for (int i = 0; i <= H; i++)
	{
		qi >> q[i];
	}

	//總服務時間
	fstream svi;
	svi.open("D:/graduated project/final data/virtual/pm/ServiceTime.txt", ios::in);

	double sv[H + 1];
	for (int i = 0; i <= H; i++)
	{
		svi >> sv[i];
	}

	//分區
	fstream Gri;
	Gri.open("D:/graduated project/final data/virtual/pm/District.txt", ios::in);
	int Gr[H + 1];
	for (int i = 0; i <= H; i++)
	{
		Gri >> Gr[i];
	}

	//順位
	fstream drp;
	drp.open("D:/graduated project/final data/virtual/pm/Priority.txt", ios::in);
	int d[R + 1][P + 1];
	for (int r = 1; r <= R; r++)
	{
		for (int p = 1; p <= P-1; p++)
		{
			drp >> d[r][p];
		}
	}
	
	//懲罰值
	/*
	報告書表11中的結果為調整此處讀取之檔案
	原測試值與測試值(i)所使用的檔案為"Penalty1.txt"
	測試值(ii)所使用的檔案為"Penalty2.txt"
	測試值(iii)所使用的檔案為"Penalty3.txt"
	測試值(iv)所使用的檔案為"Penalty4.txt"
	*/
	fstream Bp;	
	Bp.open("D:/graduated project/final data/virtual/pm/Penalty1.txt", ios::in);
	int B[P + 1];
	for (int p = 1; p <= P; p++)
	{
		Bp >> B[p];
	}


	try {
		GRBEnv env = GRBEnv();
		GRBModel model = GRBModel(env);

		//model.set(GRB_DoubleParam_MIPGap, 0.1);
		model.set(GRB_DoubleParam_TimeLimit, 3600);//最長1小時
		//決策變數v
		GRBVar v[N + 1][TRUCK_AMOUNT + 1];
		for (int i = 0; i <= N; i++)
		{
			for (int k = 1; k <= TRUCK_AMOUNT; k++)
			{
				s = "(v" + itos(i) + "_" + itos(k) + ")";
				v[i][k] = model.addVar(0, 1, 0.0, GRB_BINARY, s);
			}
		}

		//決策變數u
		GRBVar u[N + 1][TRUCK_AMOUNT + 1];
		for (int i = 0; i <= N; i++)
		{
			for (int k = 1; k <= TRUCK_AMOUNT; k++)
			{
				s = "(u" + itos(i) + "_" + itos(k) + ")";
				u[i][k] = model.addVar(0, 9999, 0.0, GRB_INTEGER, s);
			}
		}

		//決策變數x
		GRBVar x[N + 1][N + 1][TRUCK_AMOUNT + 1];
		for (int i = 0; i <= N; i++)
		{
			for (int j = 0; j <= N; j++)
			{
				for (int k = 1; k <= TRUCK_AMOUNT; k++)
				{
					s = "(x" + itos(i) + "_" + itos(j) + "_" + itos(k) + ")";
					x[i][j][k] = model.addVar(0, 1, 0.0, GRB_BINARY, s);
				}
			}
		}

		//決策變數z
		GRBVar z[R + 1][P + 1];
		for (int r = 1; r <= R; r++)
		{
			for (int p = 1; p <= P; p++)
			{
				s = "(z" + itos(r) + "_" + itos(p) + ")";
				z[r][p] = model.addVar(0, 1, 0.0, GRB_BINARY, s);
			}
		}


		GRBLinExpr sum = 0;
		GRBLinExpr sum1 = 0;
		GRBLinExpr beta = 0;
		GRBLinExpr distance = 0;
		GRBLinExpr time = 0;

		//Obj 式(16)
		//加班人力成本
		for (int k = R + 1; k <= TRUCK_AMOUNT; k++)
		{
			for (int i = 0; i <= N; i++)
			{
				sum += w * (v[i][k] * sv[i]);

				for (int j = 0; j <= N; j++)
				{
					sum += w * (x[i][j][k] * c[i][j]);
				}
			}
		}
		//所有車輛旅行成本
		for (int k = 1; k <= TRUCK_AMOUNT; k++)
		{
			for (int i = 0; i <= N; i++)
			{
				for (int j = 0; j <= N; j++)
				{
					sum += x[i][j][k] * (t[i][j] / f) * g;	//(t[i][j] / f) * g  -> 旅行成本
				}
			}
		}

		//優先順序之懲罰值
		for (int r = 1; r <= R; r++)
		{
			for (int p = 1; p <= P; p++)
			{
				sum += B[p] * z[r][p];
			}
		}

		//式(a)
		//使用率之懲罰值—客戶點數量
		for (int r = 1; r <= R; r++)
		{
			for (int i = 1; i <= N; i++)
			{
				if (r == Gr[i])
				{
					sum -= A * v[i][d[r][1]];
				}
			}
		}


		model.setObjective(sum, GRB_MINIMIZE);



		//Constr2
		//式(2)表示車輛k可以選擇是否離開場站。
		for (int k = 1; k <= TRUCK_AMOUNT; k++)
		{
			sum = 0;
			for (int j = 1; j <= N; j++)
			{
				sum += x[0][j][k];
			}
			s = "(C2_" + itos(k) +")";
			model.addConstr(sum <= 1, s);
		}

		//Constr3
		//式(3)表示車輛k有若有離開場站，最後一定要回到場站。
		for (int k = 1; k <= TRUCK_AMOUNT; k++)
		{
			sum = 0;
			sum1 = 0;
			for (int i = 1; i <= N; i++)
			{
				sum += x[0][i][k];
			}
			for (int i = 1; i <= N; i++)
			{
				sum1 += x[i][0][k];
			}
			s = "(C3_" + itos(k) + ")";
			model.addConstr(sum == sum1, s);
		}

		//Constr4
		//式(4)判斷車輛k是否有服務客戶點i。
		for (int i = 1; i <= N; i++)
		{
			for (int k = 1; k <= TRUCK_AMOUNT; k++)
			{
				sum = 0;
				for (int j = 0; j <= N; j++)
				{
					if (j != i)
					{
						sum += x[j][i][k];
					}
				}
				s = "(C4_" + itos(i) + "_" + itos(k) + ")";
				model.addConstr(sum == v[i][k], s);
			}
		}

		//Constr5
		//式(5)對所有客戶點而言，客戶點i一定要被車輛k服務一次。
		for (int i = 1; i <= N; i++)
		{
			sum = 0;
			for (int k = 1; k <= TRUCK_AMOUNT; k++)
			{
				sum += v[i][k];
			}
			s = "(C5_" + itos(i) + ")";
			model.addConstr(sum == 1, s);
		}

		//Constr6
		//式(6)若車輛k有從任一客戶點j到客戶點i，該車必定要從客戶點i離開，此式為流量守恆之概念。
		for (int i = 1; i <= N; i++)
		{
			for (int k = 1; k <= TRUCK_AMOUNT; k++)
			{
				sum = 0;
				sum1 = 0;
				for (int j = 0; j <= N; j++)
				{
					if (j != i)
					{
						sum += x[j][i][k];
					}
				}
				for (int j = 0; j <= N; j++)
				{
					if (j != i)
					{
						sum1 += x[i][j][k];
					}
				}
				s = "(C6_" + itos(i) + "_" + itos(k) + ")";
				model.addConstr(sum == sum1, s);
			}
		}

		//Constr7
		//式(7)限制車輛k從場站出發的服務次序必為0。
		for (int k = 1; k <= TRUCK_AMOUNT; k++)
		{
			s = "(C7_" + itos(k) + ")";
			model.addConstr(u[0][k] == 0, s);
		}

		//Constr8
		//式(8)表示若車輛k有從點i到點j，則點j的服務次序必為點i服務次序加一，此式同時可避免子迴圈產生。
		for (int i = 0; i <= N; i++)
		{
			for (int j = 1; j <= N; j++)
			{
				for (int k = 1; k <= TRUCK_AMOUNT; k++)
				{
					s = "(C8_"  + itos(i) + "_" + itos(j) + "_" + itos(k) + ")";
					model.addConstr(u[i][k] + 1 - M * (1 - x[i][j][k]) <= u[j][k], s);
				}
			}
		}

		//Constr9
		//式(9)表示車輛k裝載的量不會超過其可放置的容量，此式的容量限制有額外乘上每部貨車可允許容積的最大使用率(μ)。
		for (int k = 1; k <= TRUCK_AMOUNT; k++)
		{
			sum = 0;
			for (int i = 1; i <= N; i++)
			{
				sum += q[i] * v[i][k];
			}
			s = "(C9_" + itos(k) + ")";
			model.addConstr(sum <= mu * C, s);
		}

		//Constr10
		//式(10)限制非加班車輛k的最大工時。
		for (int k = 1; k <= R; k++)
		{
			sum = 0;
			for (int i = 0; i <= N; i++)
			{
				sum += v[i][k] * sv[i];

				for (int j = 0; j <= N; j++)
				{
					sum += x[i][j][k] * c[i][j];
				}
			}
			s = "(C10_" + itos(k) + ")";
			model.addConstr(sum <= T, s);
		}

		//Constr11
		//式(11)限制加班車輛k的最大工時。
		for (int k = R + 1; k <= TRUCK_AMOUNT; k++)
		{
			sum = 0;
			for (int i = 0; i <= N; i++)
			{
				sum += v[i][k] * sv[i];

				for (int j = 0; j <= N; j++)
				{
					sum += x[i][j][k] * c[i][j];
				}
			}
			s = "(C11_" + itos(k) + ")";
			model.addConstr(sum <= O, s);
		}




		//Constr18
		//式(18)表示第r區的客戶所使用的車輛中的最大順位只會是所有順位中的其中一個順位。
		for (int r = 1; r <= R; r++)
		{
			sum = 0;
			for (int p = 1; p <= P; p++)
			{
				sum += z[r][p];
			}
			s = "(C18_" + itos(r) + ")";
			model.addConstr(sum == 1, s);
		}

		//Constr17
		//式(17)將不會服務到區域r的車輛k的決策變數v_i^k設為0，此式r區客戶只給負責該區的三個順位及所有加班車輛服務。
		for (int k = 1; k <= R; k++)
		{
			for (int i = 1; i <= N; i++)
			{
				for (int r = 1; r <= R; r++)
				{
					if (Gr[i] == r)
					{
						int p = 1;
						int c = 0;
						while (p < P) {
							if (k != d[r][p]) {
								c += 1;
							}
							p += 1;
						}
						if (c == P-1)
						{
							s = "(C17_" + itos(k) + "_" + itos(i) + "_" + itos(r) + ")";
							model.addConstr(v[i][k] == 0, s);
						}
					}
				}
			}
		}

		//Constr19
		//式(19)表示若第r區所使用的車輛中的最大順位是前三順位其中一個順位的話，該順位以後的車輛絕對不能去服務該區的客戶。
		for (int i = 1; i <= N; i++)
		{
			for (int r = 1; r <= R; r++)
			{
				if (Gr[i] == r)
				{
					for (int p = 2; p < P; p++)
					{
						sum = 0;
						for (int pi = 1; pi <= p - 1; pi++)
						{
							sum += z[r][pi];
						}
						s = "(C19_" + itos(i) + "_" + itos(r) + "_" + itos(p) + ")";
						model.addConstr(v[i][d[r][p]] <= 1 - sum, s);
					}
				}
			}
		}

		//Constr20
		//式(20)表示若第r區的客戶所使用的車輛中的最大順位是前三順位中任一順位服務的話，加班車絕對不能去服務該區的客戶。
		for (int k = R + 1; k <= TRUCK_AMOUNT; k++)
		{
			for (int r = 1; r <= R; r++)
			{
				for (int i = 1; i <= N; i++)
				{
					if (Gr[i] == r)
					{
						sum = 0;
						for (int p = 1; p < P; p++)
						{
							sum += z[r][p];
						}
						s = "(C20_" + itos(k) + "_" + itos(r) + "_" + itos(i) + ")";
						model.addConstr(v[i][k] <= 1 - sum, s);
					}
				}
			}
		}




		model.update();
		model.write("debug.lp");
		model.optimize();

		//Output z_rp
		cout << "z[r][p]" << endl;
		for (int r = 1; r <= R; r++)
		{
			cout << r << "\t";
			for (int p = 1; p <= P; p++)
			{
				cout << z[r][p].get(GRB_DoubleAttr_X) << "\t";
			}
			cout << endl;
		}

		distance = 0;
		time = 0;

		//Output truck route
		cout << "truck route:" << endl;
		for (int k = 1; k <= TRUCK_AMOUNT; k++)
		{
			int begin = 0;
			for (int i = 0; i <= N; i++)
			{
				if (v[i][k].get(GRB_DoubleAttr_X) > 0.5 && k > R) {
					time += sv[i];
				}
				for (int j = 0; j <= N; j++)
				{
					if (x[i][j][k].get(GRB_DoubleAttr_X) > 0.5) //如果有值
					{
						route_begin[k-1][begin] = i ;
						route_end[k-1][begin] = j;
						distance += t[i][j];
						if (k > R) {
							time += c[i][j];
						}
						begin += 1;
					}
				}
			}
			route_num[k-1] = begin;
		}

		for (int k = 1;k <= TRUCK_AMOUNT; k++) {
			int y = 0;
			if (route_num[k-1] != 0) {
				cout << "第" << k << "車:" << "0";
				do
				{
					int i = 0;
					while (i < H) {
						if (route_begin[k - 1][i] == y) {
							y = route_end[k-1][i];
							cout << "->" << y;
							break;
						}
						i += 1;
					}
				} while (y != 0);
			}
			else {
				cout << "第" << k << "車:" << "No service";
			}
			cout << endl;
		}



		//Output Capacity
		cout << "Capacity:" << endl;
		for (int k = 1; k <= TRUCK_AMOUNT; k++)
		{
			sum = 0;
			for (int i = 0; i <= N; i++)
			{
				if (v[i][k].get(GRB_DoubleAttr_X) > 0.5) //如果有值
				{
					sum += q[i] * v[i][k].get(GRB_DoubleAttr_X);
				}
			}
			cout << "第" << k << "車: " << sum << "\tPercent: " << sum / (mu * C) << endl;
		}

		//output Cost
		cout << "traveling_cost : " << (distance / f) * g << endl;
		cout << "overtime_cost : " << time * w << endl;
		//Output Penalty
		
		sum = 0;
		for (int i = 0; i <= N; i++)
		{
			for (int r = 1; r <= R; r++)
			{
				if (r == Gr[i])
				{
					if (v[i][d[r][1]].get(GRB_DoubleAttr_X) > 0.5) //如果有值
					{
						sum += A;
					}
				}
			}
		}
		cout << "Bonus for usage rate : "<< sum << endl;

		//Output priority penalty
	
		beta = 0;
		for (int r = 1; r <= R; r++)
		{
			for (int p = 1; p <= P; p++)
			{
				if (z[r][p].get(GRB_DoubleAttr_X) > 0.5) //如果有值
				{
					beta += B[p] * z[r][p].get(GRB_DoubleAttr_X);
				}
			}
		}
		cout << "Penalty for priority: " <<beta << endl;
		//output Cost
		cout << "Cost of all route: "<< model.get(GRB_DoubleAttr_ObjVal)+sum-beta << endl;
		//Output Obj
		cout << "Objective_value: " << model.get(GRB_DoubleAttr_ObjVal) << endl;//列印最佳解的值
	}


	catch (GRBException e) {
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
	}
	catch (...) {
		cout << "Exception during optimization" << endl;
	}
	cout << "runtime: " << (double)clock() / CLOCKS_PER_SEC << "s" << endl;

	system("pause");
	return 0;
}