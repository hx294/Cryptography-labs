#include<iostream>
#include<gmpxx.h>
#include<fstream>
#include<string>

mpz_class qmi(mpz_class a,mpz_class b,mpz_class m); 

// 初始化一个随机数生成器
gmp_randclass randGen(gmp_randinit_default);

void init(){
    // 设置种子，种子可以是当前时间或其他随机值
    randGen.seed(time(nullptr));
}

std::pair<mpz_class,mpz_class> generator(){
	std::pair<mpz_class,mpz_class> res;
	while(true){
		mpz_class q = randGen.get_z_bits(512);// 生成512 位随机数
		
		mpz_nextprime(q.get_mpz_t(),q.get_mpz_t());

		mpz_class p = 2 * q + 1;
		int result = mpz_probab_prime_p(p.get_mpz_t(), 25); // Miller-Rabin 素性检测
		if(result <= 0) continue;
		res.first = p;

		// 有很大概率p是素数
		
		// 生成原根，由于q是素数，则素数p的$Z_n^*$中的元素的阶为2，1，p-1,q。
		// 由于p-1较大，故计算另外两个。不满足另外三个的就是原根。
		while(true){
			// 随机生成一个位于 p-1 到 2 的群元素
			mpz_class g = randGen.get_z_range(p-2) + 2;	
			// 检测
			mpz_class check1 = qmi(g,2,p);
			mpz_class check2 = qmi(g,q,p);

			if(check1 == 1 && check2 == 1) continue;
			res.second = g;
			break;
		}

		break;
	}
	

	return res;
}
mpz_class qmi(mpz_class a,mpz_class b,mpz_class m) {
	a %= m;
	mpz_class res = 1;
	while(b){
		if((b & 1) == 1) res = res*a % m;
		a = a*a % m;
		b >>= 1;
	}

	return res;
}

std::pair<mpz_class,mpz_class> encrypt(const mpz_class m,const mpz_class p,const mpz_class g,const mpz_class ga) {
	// 随机选取整数k
	mpz_class k = randGen.get_z_range(p-2) + 1; //  1 <= k <= p-2;
	k = 853;
	std::cout << "k: " << k << std::endl;
	// 计算c1,c2
	mpz_class c1 = qmi(g,k,p);
	mpz_class c2= m % p * qmi(ga,k,p) % p;
	
	return {c1,c2};
}

mpz_class decrypt(mpz_class c1,mpz_class c2,mpz_class a,mpz_class p){
	// 计算 v = c_1^a (mod p);
	mpz_class V = qmi(c1,a,p);
	std::cout <<"V: " <<  V << std::endl;

	// 计算出明文  m = c2 v-1 (mod p) 因为V模p，且p是质数，所以p和V互质,且p是质数,逆元可以通过快速幂求解,费马小定理
	// 如果 p 和V仅仅是互质，只能用拓展欧几里得算法
	mpz_class m = c2 * qmi(V,p-2,p) % p;

	return m;
}

int main(){
	init();
	auto [p,g] = generator();

	std::cout << "p: " << p << std::endl;
	std::cout << "g: " << g << std::endl;

	mpz_class a = randGen.get_z_range(p);
	mpz_class ga = qmi(g,a,p);
	std::cout << "g^a: " << ga << std::endl;
	mpz_class m;
	std::string path{"./data/secret1.txt"};
	std::ifstream in{path};
	in >> m;
	// p = 2579,a = 765, g = 2,ga = 949;

	// 加密过程
	auto [c1,c2] = encrypt(m,p,g,ga);
	std::cout << "c1: " << c1 << std::endl;
	std::cout << "c2: " << c2 << std::endl;

	// 解密
	auto  dec_m = decrypt(c1,c2,a,p);

	std::cout << "dec_m: " << dec_m << std::endl; 

	// 验证
	if(dec_m == m) std::cout << "算法正确" << std::endl;
	else std::cout << "算法错误" << std::endl;

	return 0;
}
