#include<iostream>
#include<algorithm>
#include<vector>
#include<fstream>
#include<gmpxx.h>
#include<string>
#include<gmssl/sm3.h>

typedef std::pair<bool ,std::pair<mpz_class,mpz_class>> Pair;

// 声明
Pair ScalarMulti(mpz_class k ,Pair G,mpz_class p,mpz_class a);

// 初始化随机状态，
gmp_randclass rand_state(gmp_randinit_default);

void printbyte(std::vector<unsigned char>  bytes){
    for(auto i = bytes.rbegin(); i != bytes.rend(); i++)
    {
        printf("%02x",(unsigned char)*i);
    }
    puts("");
}

void printbit(std::vector<unsigned char>  bits){
    for(auto x = bits.rbegin(); x != bits.rend(); x++)
        printf("%01x",*x);
    puts("");
}

std::vector<unsigned char>  bittobyte(std::vector<unsigned char>  bits){
    // std::cout << "bitobyte" << std::endl;
    // printbit(bits);
    std::vector<unsigned char>  bytes;
    auto len = bits.size();
    // std::cout << "len: " << len << std::endl;
    for(long i=0; i<len; i+=8){
        unsigned char t = 0;
        long j = i+7; // 使用 long 否则后面将死循环
        if( j > len-1) j = len-1;

        for(; j >= i; j--)
            t = t*2 + bits[j];
        
        bytes.push_back(t);
    }
    // printbyte(bytes);

    return bytes;
}

std::vector<unsigned char>  inttobyte(mpz_class x){
    std::vector<unsigned char>  res; // 左边为低位。
    while(x){
        mpz_class t1 = x & 0xff;
        res.push_back((char)t1.get_ui());
        x /= 0x100;
    }

    return res;
}

std::vector<unsigned char>  pointtobyte(Pair P){
    auto x = P.second.first,y = P.second.second;
    // std::cout << std::hex << "x: " << x << "y: " << y << std::endl;
    auto x1 = inttobyte(x),y1 = inttobyte(y);
    // printbyte(x1);
    // printbyte(y1);
    // x1 || y1
    y1.insert(y1.end(),x1.begin(),x1.end());
    char PC = 04;
    // PC || X1 || y1
    y1.push_back(PC);
    return y1;
}

mpz_class bytetoint(std::vector<unsigned char> bytes)
{
    // bytes 左边是最低位
    mpz_class res = 0;

    for(auto i = bytes.rbegin(); i != bytes.rend(); i++)
        res = 256*res + *i;

    return res;
}

Pair bytetopoint(std::vector<unsigned char> bytes)
{
    Pair res;
    
    if(bytes.back() != (unsigned char)4){
        std::cout << "Your PC = "<< bytes.back() << "PC != 0x04 " << std::endl;
        exit(-1);
    }

    size_t l = (bytes.size()-1)/2;

    std::vector<unsigned char> y1(bytes.begin(),bytes.begin()+l);
    std::vector<unsigned char> x1(bytes.begin()+l,bytes.end()-1);

    auto x1_int = bytetoint(x1);
    auto y1_int = bytetoint(y1);

    // std::cout << std::hex << "x1=" << x1_int << "\ny1=" << y1_int << std::endl;

    res = {false,{x1_int,y1_int}};
    return res;
}

std::vector<unsigned char>  bytetobit(std::vector<unsigned char>  Bytes){
    std::vector<unsigned char>  bits;
    // printbyte(Bytes);
    for( unsigned char x : Bytes) // 使用unsigned char ，否则除法是有符号的。
    {
        int cnt = 8; // 一个字节8bit,可能有前置0所以使用这种方法。
        // printf("%02x ",(unsigned char)x);
        while(cnt --)
        {
            bits.push_back(x & 1);
            x /= 2;
        }
    }

    return bits;
}

std::pair<mpz_class,Pair> GenKey(mpz_class &n,Pair &G,mpz_class &p, mpz_class &a){
    // a) 产生随机整数d [1,n-2]
    mpz_class d = rand_state.get_z_range(n-2) + 1;
    // b) 计算P
    auto P = ScalarMulti(d,G,p,a);

    // c) (d,P)
    return {d,P};
}

mpz_class qmi(mpz_class a,mpz_class b,mpz_class p){
    a = a % p;
    mpz_class res = 1;

    while(b){
        if((b & 1) == 1) res = res * a % p;
        a = a*a %p;
        b >>= 1;
    }

    return res;
}

Pair EllipticCurveAdd(Pair p1,Pair p2,mpz_class a,mpz_class p){
    Pair res;

    auto [infinate_p1,temp0] = p1;
    auto [x1,y1] = temp0;

    auto [infinate_p2,temp1] = p2;
    auto [x2,y2] = temp1;

    auto&[infinate,temp2] = res;
    auto&[x3,y3] = temp2;

    if(infinate_p1  && infinate_p2 ) infinate = true;
    else if( infinate_p1 ){ infinate = false; x3 = x2 ; y3 = y2;}
    else if( infinate_p2 ){ infinate = false; x3 = x1 ; y3 = y1;}
    else if( x1 == x2 && y1 == -y2) infinate = true; // 互逆
    else if( x1 != x2 ){
        // 不同点
        mpz_class r = (y2-y1)%p*qmi((x2-x1)%p,p-2,p)%p;
        x3 =(( r * r % p - x1)%p - x2 )%p;
        y3 = ( r * (x1-x3) % p - y1) % p;
        infinate = false;
    }else if( x1 == x2 && y1 == y2){
        // 相同点
        mpz_class r = ( 3 * x1 * x1 % p + a) % p * qmi(2 * y1, p -2 , p) % p ;
        x3 = ( r*r%p - 2 * x1 )%p ;
        y3 = ( r * ( x1 - x3 ) % p - y1 ) % p;
        infinate = false;
    }

    x3 = (x3 + p) % p;
    y3 = (y3 + p) % p;

    return res;
}

Pair ScalarMulti(mpz_class k ,Pair G,mpz_class p,mpz_class a){ 
    // n==#E且n是#E的素因子，所以n和#E都是素数。
    // P_B=[d]G, 1 <= d<= n-2,故[k]P_B=[k][d]G,[k][d]不被n整除，就不会等于无穷远点。
    
    Pair res{true,{0,0}};
    // std::cout << std::hex << k << '\n' << p << '\n' << a  << '\n' << G.second.first << '\n' << G.second.second<< std::endl;

    // 二进制展开法
    while(k){
        if ((k & 1 )== 1)res = EllipticCurveAdd(G,res,a,p);
        G = EllipticCurveAdd(G,G,a,p);
        k >>= 1;
    }

    return res;
}

/* 检查 比特串是否全0 */
bool CheckAll0(std::vector<unsigned char>  bits){
    
    bool res = true;

    for(auto x: bits)
        if ( x ){ res = false; break; }

    return res;
}

/* 
输入 bit 串，输出32byte串
*/
std::vector<unsigned char> hash_sm3(std::vector<unsigned char>  bits)
{
    std::vector<unsigned char>  res;
    // std::cout << "hash_sm3 输入bytes: " << std::endl;
    auto bytes = bittobyte(bits);
    // printbyte(bytes);
    std::reverse(bytes.begin(),bytes.end());

    unsigned char digest[SM3_DIGEST_SIZE]; // 32 * 8 = 256bit
    SM3_CTX sm3;
    sm3_init(&sm3);

    sm3_update(&sm3, bytes.data() ,bytes.size());

    sm3_finish(&sm3,digest);
    res.insert(res.end(),digest,digest + SM3_DIGEST_SIZE);
    // printbyte(res);

    return res;
}

void ctPlus1(std::vector<unsigned char>  ct){
    for(int i=0; i<4; i++)
        if( ct [i] != 0xff){ ct[i] ++;break;}
        else{
            ct[i] = 0;
        }
}
/* KDF 
输入：比特串Z，整数klen(表示要获得的密钥数据的比特长度，要求该值小于(232-1)v)。
输出：长度为klen的密钥数据比特串K。
*/
std::vector<unsigned char>  KDF(std::vector<unsigned char>  Z,const std::size_t klen){
    std::vector<unsigned char>  res;
    // sm3 返回的是256bit
    std::size_t v = 256 ; 

    auto Z_byte = bittobyte(Z);

    if( v * (1 << 32 - 1)  <= klen)  {
        std::cout << "klen should be smaller than (2 ** 32 -1 ) * v" << std::endl;
        exit(-1);
    }

    
    // 1. 计数器 ct
    std::vector<unsigned char>  ct {0x01,0x00,0x00,0x00};// 左边是低位

    // 2. 
    std::size_t len = klen/v + ((klen % v)? 1:0);
    for(int i=1; i < len ; i++)
    {
        // 2.1 计算哈希值
        auto Zct = ct;
        Zct.insert(Zct.end(),Z_byte.begin(),Z_byte.end());
        auto temp = hash_sm3(Zct);
        res.insert(res.end(),temp.begin(),temp.end());
        ctPlus1(ct);
    }

    res = bytetobit(res);

    // 最后一个放在外部计算
    std::vector<unsigned char>  Zct = ct;
    Zct.insert(Zct.end(),Z_byte.begin(),Z_byte.end());
    auto temp = hash_sm3(bytetobit(Zct));
    temp = bytetobit(temp);
    if( klen % v)
        temp.erase(temp.begin()+klen- (v * (klen/v)), temp.end());

    res.insert(res.end(),temp.begin(),temp.end());
    res = bittobyte(res);
    std::reverse(res.begin(),res.end());
    res = bytetobit(res);

    return res;
}

bool OnCurve( Pair point, mpz_class a, mpz_class b, mpz_class p){
    // 曲线方程 ： y^2 = x^3 + a^x + b
    auto [infinate,temp] = point;
    auto [x,y] = temp;

    mpz_class left = y * y % p;
    mpz_class right = (x * x % p * x % p + a * x % p + b ) % p;

    if(left == right) return true;
    else return false;
}

std::vector<unsigned char>  encrypt(std::vector<unsigned char>  M,mpz_class h,mpz_class n,Pair G,Pair P_B,mpz_class p,mpz_class a,mpz_class b,const std::size_t klen){
    std::vector<unsigned char>  C;
    while (true){
        // a1 产生随机数   1 <= k <= n-1;
        mpz_class k = rand_state.get_z_range(n-1)+1; // 去除n并去除0
        // mpz_class k("0x4C62EEFD 6ECFC2B9 5B92FD6C 3D957514 8AFA1742 5546D490 18E5388D 49DD7B4F");
        // a2 
        // 生成C1
        auto C1_point = ScalarMulti(k,G,p,a);
        // 转为bit串
        auto C1 = pointtobyte(C1_point);
        std::cout << "C1 : ";
        printbyte(C1);
        C1 = bytetobit(C1);

        // 计算S
        // 判断S是否为无穷远点
        auto S = ScalarMulti(h,P_B,p,a);
        // std::cout << "S: " << S.second.first << "\n\t" << S.second.second << std::endl;
        if(S.first == true) { 
            std::cout << " S can't not be infinite point" << std::endl;
            exit(-1);
        }

        // 计算 x2,y2,并转为bit串
        auto [infinate,temp] = ScalarMulti(k,P_B,p,a);
        auto [x2,y2] = temp;
        auto x2_byte = inttobyte(x2),y2_byte = inttobyte(y2);
        std::cout << "x2,y2: ";
        printbyte(x2_byte),printbyte(y2_byte);
        auto x2_bit = bytetobit(x2_byte),y2_bit = bytetobit(y2_byte);
        // printbit(x2_bit),printbit(y2_bit);


        // 计算t
        auto x2_bit_y2_bit = y2_bit;
        x2_bit_y2_bit.insert(x2_bit_y2_bit.end(),x2_bit.begin(),x2_bit.end());
        auto t = KDF(x2_bit_y2_bit,klen);
        std::cout << "t = "; 
        printbyte(bittobyte(t));
        if(CheckAll0(t)) continue;

        // 计算 C2 = M ^ t;
        std::vector<unsigned char>  C2;
        for(size_t i = 0; i<klen; i++)
            C2.push_back(M[i] ^ t[i]);
        std::cout << "C2= ";
        printbyte(bittobyte(C2));

        // 计算 C3
        auto y2_bit_M_x2_bit  = y2_bit;
        y2_bit_M_x2_bit.insert(y2_bit_M_x2_bit.end(),M.begin(),M.end());
        y2_bit_M_x2_bit.insert(y2_bit_M_x2_bit.end(),x2_bit.begin(),x2_bit.end());
        std::vector<unsigned char>  C3 = hash_sm3(y2_bit_M_x2_bit);
        std::reverse(C3.begin(),C3.end());
        std::cout << "C3=";
        printbyte(C3);
        C3 = bytetobit(C3);

        C = C3;
        C.insert(C.end(),C2.begin(),C2.end());
        C.insert(C.end(),C1.begin(),C1.end());
        // std::cout << "C= ";
        // printbyte(bittobyte(C));

        break;
    }

    
    return C;
}

/* C 是bit 串 
输出bit串
*/
std::vector<unsigned char> decrypt(std::vector<unsigned char>  C,mpz_class h,mpz_class n,Pair G,mpz_class d_B,mpz_class p,mpz_class a,mpz_class b,const std::size_t klen){
    std::vector<unsigned char> C3(C.begin(),C.begin()+256); 
    std::vector<unsigned char> C2(C.begin()+256,C.begin()+256 +klen);
    std::vector<unsigned char> C1(C.begin()+256+klen,C.end());

    // std::cout << "C1=";
    // printbyte(bittobyte(C1));
    // std::cout << "C2=";
    // printbyte(bittobyte(C2));
    // std::cout << "C3=";
    // printbyte(bittobyte(C3));

    // 1.将 C1 转化为 字节，再转为 整数。最后检测是否在曲线上。
    C1 = bittobyte(C1);
    auto C1_point = bytetopoint(C1);

    if(!OnCurve(C1_point,a,b,p)){
        std::cout << "C1 is not on the curve" << std::endl;
        exit(-1);
    }

    // 2. 计算 S= [h]C1 ,判断是否无穷远点
    auto S = ScalarMulti(h,C1_point,p,a);
    if( S.first == true) {
        std::cout << "S is a infinate point" << std::endl;
        exit(-1);
    }

    // 3. 计算 [d_B]C1 = (x2,y2) ,并将 x2,y2 转化为 bit
    auto [infinate,temp] = ScalarMulti(d_B,C1_point,p,a);
    mpz_class x2_int = temp.first,y2_int = temp.second;

    auto x2 = inttobyte(x2_int),y2 = inttobyte(y2_int);
    // std::cout << "x2,y2:";
    // printbyte(x2),printbyte(y2);
    x2 = bytetobit(x2),y2 = bytetobit(y2);

    // 4. 计算 t ,若t为全0，则报错退出
    std::vector<unsigned char> x2_y2(y2);
    x2_y2.insert(x2_y2.end(),x2.begin(),x2.end());
    auto t = KDF(x2_y2,klen) ;
    // std::cout << "t:";
    // printbyte(bittobyte(t));
    if( CheckAll0(x2_y2)) {
        std::cout << "t == 0" << std::endl;
        exit(-1);
    }

    // 5. 计算 M` = C2 ^ t;
    size_t len= t.size();
    std::vector<unsigned char> M;
    for(int i=0; i < len; i++)
        M.push_back(C2[i] ^ t[i]);
    
    // std::cout << "M` :";
    // printbyte(bittobyte(M));

    // 6. 计算 u = hash(x2 || M || y2) ,判断是否与c3 相等。
    std::vector<unsigned char> x2_M_y2 = y2 ;
    x2_M_y2.insert(x2_M_y2.end(),M.begin(),M.end());
    x2_M_y2.insert(x2_M_y2.end(),x2.begin(),x2.end());
    auto u = hash_sm3(x2_M_y2);

    std::reverse(u.begin(),u.end());
    if( u != bittobyte(C3)) {
        std::cout << " 哈希值不一致" << std::endl;
        exit(-1);
    }

    return M;
}

int main(){
    // 使用时间作为种子
    rand_state.seed(static_cast<unsigned long>(time(nullptr)));

    mpz_class p("0xFFFFFFFE FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF 00000000 FFFFFFFF FFFFFFFF");
    mpz_class a("0xFFFFFFFE FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF 00000000 FFFFFFFF FFFFFFFC");
    mpz_class b("0x28E9FA9E 9D9F5E34 4D5A9E4B CF6509A7 F39789F5 15AB8F92 DDBCBD41 4D940E93");
    mpz_class n("0xFFFFFFFE FFFFFFFF FFFFFFFF FFFFFFFF 7203DF6B 21C6052B 53BBF409 39D54123");
    mpz_class Gx("0x32C4AE2C 1F198119 5F990446 6A39C994 8FE30BBF F2660BE1 715A4589 334C74C7");
    mpz_class Gy("0xBC3736A2 F4F6779C 59BDCEE3 6B692153 D0A9877C C62A4740 02DF32E5 2139F0A0");
    mpz_class h{1};
    std::vector<char> choice{'1'};
    Pair G{false,{Gx,Gy}};

    // 输出参数
    std::cout << "椭圆方程为：y^2 = x ^ 3 + a*x + b" <<std::endl;
    std::cout << "椭圆参数：\np = " << p << "\na = " << a << "\nb = " << b << std::endl;
    std::cout << "基元G:\nGx = " << Gx << "\nGy = " << Gy << std::endl; 
    std::cout << "阶 n = " << n << "\nh = " << h << std::endl;

    for(auto ind : choice){
        
        std::string  path {"./data/"},raw;
        std::vector<unsigned char> M;
        path.push_back(ind);
        path.append(".txt");

        std::ifstream in{path};
        std::getline(in,raw);
        for(unsigned char x: raw) M.push_back(x);
        std::reverse(M.begin(),M.end());
        std::size_t klen = raw.length() * 8;

        std::cout << "M = " ;
        printbyte(M);

        M = bytetobit(M);
        // std::cout <<  "转为bit: ";
        // printbit(M);
        auto [d_B,P_B] = GenKey(n,G,p,a);
        // mpz_class d_B("0x1649AB77 A00637BD 5E2EFE28 3FBF3535 34AA7F7C B89463F2 08DDBC29 20BB0DA0");
        // mpz_class x_B("0x435B39CC A8F3B508 C1488AFC 67BE491A 0F7BA07E 581A0E48 49A5CF70 628A7E0A");
        // mpz_class y_B("0x75DDBA78 F15FEECB 4C7895E2 C1CDF5FE 01DEBB2C DBADF453 99CCF77B BA076A42");
        // Pair P_B={false,{x_B,y_B}};

        // 加密
        std::cout << "开始加密。。。" << std::endl;
        std::vector<unsigned char>  C = encrypt(M,h,n,G,P_B,p,a,b,klen);
        std::cout << "加密结果为：";
        printbyte(bittobyte(C));

        // 解密
        std::cout << "开始解密...." << std::endl;
        auto de_M = decrypt(C,h,n,G,d_B,p,a,b,klen);
        std::cout << "解密结果为: ";
        printbyte(bittobyte(de_M));
        if(de_M == M) std::cout << "解密成功" << std::endl;
        else std::cout << "解密失败" << std::endl;
    }

    return 0;
}