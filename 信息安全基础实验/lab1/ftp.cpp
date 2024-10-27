#include <iostream>
#include <gmp.h>
#include <memory>
#include <string>
#include<ctime>
#include <fstream>

using std::cout,std::cin,std::endl,std::ifstream,std::string;

void gcd (const mpz_t x,const mpz_t y,mpz_t res){
    mpz_t a,b;
    mpz_init_set(a,x),mpz_init_set(b,y);
    while ( mpz_cmp_ui(b,0) > 0){
        mpz_set(res,a);
        mpz_set(a,b);
        mpz_mod(b,res,b);
    }
    mpz_set(res,a);
    mpz_clear(a),mpz_clear(b);
}


void qmi(const mpz_t x,const mpz_t y,mpz_t res){
    mpz_t a,b,m;
    mpz_init_set(a,x),mpz_init_set(b,y),mpz_init_set(m,y);
    mpz_sub_ui(b,b,1);

    mpz_mod(a,a,m);
    mpz_set_ui(res,1);
    mpz_t c;
    mpz_init_set_ui(c,1);

    while(mpz_cmp_ui(b,0) > 0 ){
        mpz_and(c,b,c);
        if( mpz_cmp_ui(c,1) == 0){
            mpz_mul(res,res,a);
            mpz_mod(res,res,m);
        }
        mpz_mul(a,a,a);
        mpz_mod(a,a,m);
        mpz_div_2exp(b,b,1);
        mpz_set_ui(c,1);
    }

    mpz_clear(a),mpz_clear(b),mpz_clear(m);

}

gmp_randstate_t state;  // 定义随机数生成器状态

void randinit(){
    // 初始化随机数生成器状态
    gmp_randinit_default(state);
    
    // 设置随机数生成器种子
    unsigned long seed = static_cast<unsigned long>(time(NULL));  // 使用当前时间作为种子
    gmp_randseed_ui(state, seed);
}




std::pair<bool,double> fpt (mpz_t m, mpz_t k){

    double rate = 1;
    mpz_t a,res;
    mpz_init(a),mpz_init(res);
    mpz_mod_ui(a,m,2);
    if( mpz_cmp_ui(m,3) < 0 || mpz_cmp_ui(a,0) == 0) return {false,1};

    for(int i=0; mpz_cmp_ui(k,i) > 0 ;i++){
        // （1）随机数 a ; 2 <= a <= m-2
        mpz_urandomm(a,state,m);
        mpz_sub_ui(m,m,3);
        mpz_mod(a,a,m);
        mpz_add_ui(a,a,2);
        mpz_add_ui(m,m,3);
        
        // (2) 最大公约数
        gcd(a,m,res);
        if(mpz_cmp_ui(res,1) != 0){
            mpz_clear(res);
            mpz_clear(a);
             return {false,0};
        }
        // (3) 幂运算
        qmi(a,m,res);
        if(mpz_cmp_ui(res,1) != 0) {
            mpz_clear(res);
            mpz_clear(a);
            return {false,0};
        }
        else rate *= 0.5;
        
    }
    mpz_clear(a),mpz_clear(res);
    return {true,1-rate};
}

int main(int argc,char* argv[]){
    if( argc != 2) {
        cout << "Usage: " << argv[0] << " <k>\n" << "k : the times to test" << endl; 
        exit(-1);
    }
    randinit();
    int times = 0;
    while(times ++ < 4){
        ifstream in;
        string path = "./data1/";
        path.append(std::to_string(times));
        path.append(".txt");
        in.open(path);
        mpz_t m,k;
        mpz_init(m),mpz_init(k);
        string s;
        in >> s;
        in.close();
        mpz_set_str(m,s.c_str(),10);
        mpz_set_str(k,argv[1],10);

        std::pair<bool,double> res = fpt(m,k);

        mpz_clear(m),mpz_clear(k);

        if(res.first) cout << " The probability that m = " <<s<< " is a prime is " << res.second << endl;
        else
            if(res.second == 1) cout << "illeagel number m = "<< s <<" input !" << endl;
            else cout << "m = "<< s << " is a composite number" << endl;
    }
    gmp_randclear(state); 
}