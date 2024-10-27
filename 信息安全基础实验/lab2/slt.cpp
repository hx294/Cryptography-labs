#include <fstream>
#include<iostream>
#include<string>
#include<gmpxx.h>

using namespace std;

int gcd(mpz_class a,mpz_class b){
	
	mpz_class t;
	while(b != 0){
		t = b;
		b = a%b;
		a = t;	
	}

	return a.get_ui();
}

mpz_class extgcd(mpz_class a,mpz_class b,mpz_class &x ,mpz_class &y){
	if(b == 0){
		x = 1,y = 0;
		return a;
	}
	mpz_class d = extgcd(b,a%b,y,x);
	
	y -= a/b*x;
	return d;
}

mpz_class inverse(mpz_class a, mpz_class b){
	mpz_class x,y;
	extgcd(a,b,x,y);

	return x;
}


int main(){
	int choice[4] = {2,4,5,0};
	for(int i = 0; choice[i] ; i ++){
		vector<mpz_class> as,ms;
		mpz_class a,m;
		string path = "./实验2的报告模板和数据/";
		path.append(to_string(choice[i]));
		path.append(".txt");
		cout << path << endl;
		ifstream in(path);
		
		// input
		for(int i=0; i < 3 ; i++){
			in >> a;
			as.push_back(a);
		}
		for(int i=0; i< 3; i++){
			in >> m;
			ms.push_back(m);
		}

		// 判断互质
		int flag = 1;
		auto len = ms.size();
		for(int i=0; i< len; i++)
			for(int j = i+1; j < len; j++)
			{
				if(gcd(ms[i],ms[j]) != 1){
					cout << "不能直接利用中国剩余定理" << endl;
					flag = 0;
					break;
				}
				if(!flag)break; 
			}
		if(!flag) continue;

		// 计算m1*...mk
		mpz_class sum = 1;
		for(int i=0; i<len;i++) sum *= ms[i];

		mpz_class x = 0;
		for(int i = 0; i<len; i++){
			mpz_class M,M_;
			M = sum/ms[i]; // M 和 ms[i] 互质,可使用拓展欧几里得算法求m的逆
			M_ = inverse(M,ms[i]);
			x = (x + M_*M % sum*as[i] % sum ) % sum;
		}

		// 可能是负数
		x = (x + sum)%sum;


		cout << x << endl;
	}

	return 0;
}
