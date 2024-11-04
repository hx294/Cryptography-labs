#include <fstream>
#include<map>
#include<set>
#include<iostream>
#include<vector>
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

pair<bool,mpz_class> ch(vector<mpz_class> &as,vector<mpz_class> &ms){
    // 判断互质
    int flag = 1;
    auto len = as.size();
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
    if(!flag) return {false,0};

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

    return {true,x};
}
// 获取d
void get_d(vector<mpz_class> &d,const int n,const int tt,const mpz_class & k,const int lenth){

    // 1. 设置随机数生成器
    gmp_randclass randGen(gmp_randinit_default);
    randGen.seed(time(nullptr)); // 使用当前时间作为种子

    mpz_class base;
    mpz_class max;
    mpz_ui_pow_ui(base.get_mpz_t(),10,lenth/tt );
    mpz_ui_pow_ui(max.get_mpz_t(),10,lenth/(tt-1));
    // 从2开始向d集合中添加符合要求的数，直到满足要求
    while(true){
        d.clear();

        // 生成n个互质的数 从小到大，
        for(;d.size() != n;){
            mpz_class  i = randGen.get_z_range(max-base) + base ;
            bool flag = false;
            for(auto j: d) 
                if(gcd(j,i) != 1) {flag = true; break;}
            if(flag) continue;
            d.push_back(i);
        }

        // sort
        sort(d.begin(),d.end());
        
        //  N > M;
        mpz_class N = 1;
        auto jj = d.begin();
        // t 个
        for(int j = 0; j < tt; j ++){ 
            N *= *jj;
            jj++;
        }
        // t-1 个
        mpz_class M = 1;
        auto jjj = d.rbegin();
        for(int j = n- tt + 1; j < n; j++){
             M *= *jjj;
             jjj++;
        }
        if( N <= M) continue;

        // N > k > M
        if( N > k && k > M) {
            for(auto j = d.begin(); j != d.end(); j++) cout << "d" << j-d.begin()+1 << '\t' << *j << endl; 
            cout << "N:" << N << endl;
            cout << "M:" << M << endl;
            cout << "成功生成d" << endl;
            break;
        }
    }
}

int main(){
    int k = 2; // 测试文件个数
    for(int i = 1; i<=k; i++){
        string path{"./data/secret"};
        // 输入秘密k、t、n
        path += to_string(i);
        path += ".txt";
        cout << "开始测试" << path  << endl;
        ifstream in{path};

        mpz_class k;
        int t,n;
        in >> k;
        int lenth = k.get_str().size();
        puts("请输入t 和 n:");
        cin >> t >>n;
        // 获取符合要求1、2的n个d，不知怎么求，就暴力把
        vector<mpz_class> d;
        get_d(d,n,t,k,lenth);

        vector<mpz_class> son_k;
        // 求出n个子秘密
        for(auto& i: d) son_k.push_back( k % i);

        // 使用t-1个子秘密进行恢复的到faker,并与k对比
        vector<mpz_class> test1;
        test1.assign(son_k.begin(),son_k.begin() + t - 1);
        auto [flag,faker] = ch(test1,d);
        cout << "使用t-1个子秘密恢复得到的值为" << faker << endl;
        if(faker != k) cout << "恢复失败" << endl;
        else cout << "恢复成功" << endl;

        // 使用t个子密钥进行恢复并与k对比
        vector<mpz_class> test2;
        test2.assign(son_k.begin(),son_k.begin() + t);
        auto [flag1,key] = ch(test2,d);
        cout << "使用t个子秘密恢复得到的值为" << key << endl;
        if(key != k) cout << "恢复失败" << endl;
        else cout << "恢复成功" << endl;
    }
	return 0;
}
