/*
g++ -isystem /Users/macbook/Desktop/openssl-1.0.2-beta3/compiled/ cipher.cpp -lcrypto -L/Users/macbook/Desktop/openssl-1.0.2-beta3/compiled/lib -o ./exec
*/
/*
g++ -isystem /Users/macbook/Desktop/openssl-1.0.2-beta3/compiled/ cipher.cpp -lcrypto -L/usr/local/lib -std=c++11 -o ./exec

*/
#include <iostream>
#include <iomanip>
#include <openssl/aes.h>
#include <openssl/modes.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#define u8 unsigned char //存储0到255 , 2^8
typedef unsigned int ui;
using namespace std;
struct ctr_state{
    unsigned char ivec[AES_BLOCK_SIZE]; /// AES_BLOCK_SIZE = 16
    ui num;
    unsigned char ecount[AES_BLOCK_SIZE];
};
void init_ctr(struct ctr_state *state, const unsigned char t1[16])
{
    state->num = 0;
    memset(state->ecount, 0, 16);
    memcpy(state->ivec, t1, 16); 
}
void crypt_message(const u8* src, u8* dst, ui src_len, const AES_KEY* key, const u8* counter_block)
{
    struct ctr_state state;
    init_ctr(&state, counter_block);
/*
void CRYPTO_ctr128_encrypt 
    (const unsigned char *    in,
    unsigned char *           out,
    size_t                    len,
    const void *              key,
    unsigned char        ivec[16],
    unsigned char  ecount_buf[16],
    ui *            num,
    block128_f              block) 
*/
    CRYPTO_ctr128_encrypt(src, dst,  src_len,key, state.ivec, state.ecount, &state.num, (block128_f) AES_encrypt); 
}
vector<ui> trans_32_to_8x4(ui number){
    vector<ui> ans;
    ui part1 = (number >> 24) & 0xFF;
    ui part2 = (number >> 16) & 0xFF;
    ui part3 = (number >> 8) & 0xFF;
    ui part4 =  (number & 0xFF);
    ans.push_back(part1); ans.push_back(part2); ans.push_back(part3); ans.push_back(part4);
    return ans;
}
void s_to_ui(vector<string> &s, vector<ui> &u_i){
    for(auto x : s){ 
        vector<ui> v = trans_32_to_8x4(stoul(x, 0, 16 ));
        for(auto y : v) u_i.push_back(y); 
    }
}
int main(int argc, char const *argv[]) {
    if(argc != 2){
    cout <<"=============================================================="<<endl
         <<"Program usage: ./exec <input.txt> "<<endl
         <<"==============================================================" << endl;
         return 1;}
    ifstream input_s(argv[1]); /// input.txt
    // string output = argv[2];      /// output.txt
    string tmp_data, Direction, Length,Bearer;
    vector<string> Key,Count,Plaintext;
    int flag;
    while(input_s >> tmp_data){
        // 检查是否要切换读取的数据类型
        if(tmp_data == "Key")           { flag = 1; continue;}
        else if(tmp_data == "Count")    { flag = 2; continue;}
        else if(tmp_data == "Bearer")   { flag = 3; continue;}
        else if(tmp_data == "Direction"){ flag = 4; continue;}
        else if(tmp_data == "Length")   { flag = 5; continue;}
        else if(tmp_data == "Plaintext"){ flag = 6; continue;}
        else if(tmp_data == "=")        { continue;}
        // 根据flag进行读取
        if(flag == 1)      Key.push_back(tmp_data);
        else if(flag == 2) Count.push_back(tmp_data);
        else if(flag == 3) Bearer = tmp_data;
        else if(flag == 4) Direction = tmp_data;
        else if(flag == 5) Length = tmp_data;
        else if(flag == 6) Plaintext.push_back(tmp_data);
    } 
    input_s.close();
    // print all the vectors
/*
    // cout<<endl<<"Key: ";
    // for(auto x : Key) cout<<x<<" ";
    // cout<<endl<<"Count: ";
    // for(auto x : Count) cout<<x<<" ";
    // cout<<endl<<"Bearer: ";
    // for(auto x : Bearer) cout<<x<<" ";
    // cout<<endl<<"Direction: ";
    // for(auto x : Direction) cout<<x<<" ";
    // cout<<endl<<"Length: ";
    // for(auto x : Length) cout<<x<<" ";
    // cout<<endl<<"Plaintext: ";
    // for(auto x : Plaintext) cout<<x<<" ";
    // cout<<endl;
*/
    vector<ui> key_uc, cou_uc, pla_uc;
    int dir_uc = stoi(Direction);// direction用整数存, 0 or 1
    int len_uc = stoi(Length); // len用整数存
    // cout<<"len_uc"<<len_uc<<endl<<endl;

    int tmp_len = len_uc / 8;
    if(len_uc % 8 != 0) tmp_len += 1; // get argument: len 
    
    bitset<8> bit(stoi(Bearer, nullptr, 16)); // 15 : 0001 0101
    bitset<8> bitvec1(00000001);
    // dir是1，移位和1异或再移位,bit是得到的2进制 1010 1100
    if(dir_uc == 1){ bit = (((bit << 1) ^ bitvec1) << 2);} 
    // dir是0直接移3位,bit是得到的2进制 1010 1000
    else if(dir_uc == 0){ bit = (bit << 3);} 
    // combine count, bearer and direction to get T1:
    stringstream ss;
    ss << hex << uppercase << bit.to_ulong();
    ui tmp = stoul(ss.str(), 0, 16 ); 
    s_to_ui(Key, key_uc);
    s_to_ui(Count, cou_uc);
    s_to_ui(Plaintext, pla_uc);
    cou_uc.push_back(tmp); // 添加到T1中
    for(int i = 0; i < 11; i++){ cou_uc.push_back(0x00);}
    //============================================================================//
                              // 函数调用 
    //============================================================================//
    // int len; 
    int size = 32;
    size = 4* Plaintext.size(); // 需要根据明文的大小调整存储明文和解密明文的数组。
    // cout<<"size: "<<size<<endl;

    int bit_size = size * 8;
    u8 cipher[size],recovered[size], source[size],enc_key[16], t1[16];//t1[16] = {0x39,0x8a,0x59,0xb4,0xac};
    copy(begin(pla_uc), end(pla_uc), source);
    copy(begin(key_uc), end(key_uc), enc_key);
    copy(begin(cou_uc), end(cou_uc), t1);

    unsigned char iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, AES_BLOCK_SIZE);
    AES_KEY key; AES_set_encrypt_key(enc_key, 128, &key);/// len of key to 128bit，
    memset(recovered, 0, sizeof(recovered));
    crypt_message((const u8 *) source,(u8 *) cipher, tmp_len, &key,t1); 
    crypt_message((const u8 *) cipher, (u8 *) recovered,  tmp_len, &key, t1);
    // Print 
    int count = 0;
    // 在打印之前修改密文，最后的空余位要使用0来填充， 798差两位用0填充
    // cout<<"tmp_len"<<tmp_len<<endl<<endl;
    int padding_bit = (8*tmp_len) - len_uc; // 得到需要填几bit的0，比如800-798 = 2 bits
    if (padding_bit != 0){ //只有需要补0的时候才补零
        // 需要填充的位每满8bit就需要重组数组中的一项或者改成0x00：小于8是1round
        int round = (padding_bit/8) + 1;
        // cout<<"round: "<<round<<endl;
        int padding_size = padding_bit%8; // 需要填
        // cout<<"padding_size: "<<padding_size<<endl;

        for (int i = size-1; i > size - round-1; i--) // 从密文数组最后一位开始改，改round次
        {
            // 只要不是最后需要改的元素，直接变成0x00就行了
            if(i != size - round){
                cipher[i] = 0x00;
            }
            else{
            // printf("%02x", cipher[i]);
                cipher[i] = (( cipher[i] >> padding_bit) << padding_bit);
            }
        }
    }
    cout<<endl<<"source:    "<<" ";
    for(auto x : pla_uc){ printf("%02x", x); 
        count++;
        if (count % 4 == 0){printf(" ");}
        if (count % 16 == 0){printf("\n            ");}
    }
    cout<<endl<<endl<<"recovered: "<<" ";
    count = 0;
    for(auto x : recovered){ printf("%02x", x);
        count++;
        if (count % 4 == 0){printf(" ");}
        if (count % 16 == 0){printf("\n            ");}
    }
    count = 0;
    cout<<endl<<endl<<"cipher:    "<<" ";
    for(auto x : cipher){ printf("%02x", x);
        count++;
        if (count % 4 == 0){printf(" ");}  
        if (count % 16 == 0){printf("\n            ");}
    }
    cout<<endl;
    // cout<<hex<< ((0x53d3 >> 2)<<2)<<endl
    return 0;
}