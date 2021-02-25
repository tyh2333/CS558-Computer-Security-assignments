/*
g++ -isystem /Users/macbook/Desktop/openssl-1.0.2-beta3/compiled/ right.cpp -lcrypto -L/usr/local/lib -std=c++11 -o ./exec

*/
#include <bitset>
#include <iostream>
#include <iomanip>
#include <openssl/aes.h>
#include "openssl/aes.h"
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
#define u8 unsigned char
typedef unsigned int ui;
using namespace std;
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
/* For CMAC Calculation */
  u8 const_Rb[16] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
  };
  u8 const_Zero[16] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  /* Basic Functions */
void xor_128(u8 *a, u8 *b, u8 *out)
{
      int i;
      for (i=0;i<16; i++)
      {
          out[i] = a[i] ^ b[i];
      }
}
void print_hex(char *str, u8 *buf, int len)
{
    int i;
    for ( i=0; i<len; i++ ) {
          if ( (i % 16) == 0 && i != 0 ) cout<<str;
          printf("%02x", buf[i]);
          if ( (i % 4) == 3 ) printf(" ");
          if ( (i % 16) == 15 ) printf("\n");
    }
    if ( (i % 16) != 0 ) printf("\n");
}
void print128(u8 *bytes)
{
      int j;
      for (j=0; j<16;j++) {
          printf("%02x",bytes[j]);
          if ( (j%4) == 3 ) printf(" ");
      }
}
void print96(u8 *bytes)
{
    int j;
    for (j=0; j<12;j++) {
        printf("%02x",bytes[j]);
        if ( (j%4) == 3 ) printf(" ");
    }
}
/* AES-CMAC Generation Function */

void leftshift_onebit(u8 *input,u8 *output){
      int i;
      u8 overflow = 0;
      for ( i=15; i>=0; i-- ) {
          output[i] = input[i] << 1;
          output[i] |= overflow;
          overflow = (input[i] & 0x80)?1:0;
      }
      return;
}
void padding ( u8 *lastb, u8 *pad, int length )
{
      int j;
	  int i;
	  bool flag1 = false;
	  u8 bit = 0x80;
	  int pb;
	  int pbp;
	  
      /* original last block */
	  pb = length/8;
	  pb = pb%16;
	  // printf("end byte:%d\n",pb);
	  pbp = length%8;
      for ( j=0; j<16; j++ ) {
          if ( j < pb) {
              pad[j] = lastb[j];
          } else if ( j == pb) {
			  if (pbp == 0){
				    flag1 = true;		  	
				 pad[j] = lastb[j];
			  }
			  else {
				 for (i = 0; i < pbp ; i++){
					bit = bit >> 1;
				 }
				 pad[j] = lastb[j] | bit;
			  }
          } else {
			  if (flag1 == true){
				  pad[j] = 0x80;
				  flag1 = false;
			  }
			  else
				  pad[j] = 0x00;
          }
      }
}
void AES_128(u8* key, u8* input, u8* output){
    AES_KEY AESkey;
    AES_set_encrypt_key((const u8 *) key, 128, &AESkey);
    AES_encrypt((const u8 *) input, output, (const AES_KEY *) &AESkey);
}
void generate_subkey(u8 *key, u8 *K1, u8 *K2)
{
      u8 L[16]; // for output of AES-128 applied to 0^128 
      u8 Z[16];
      u8 tmp[16];
      int i;
      for ( i=0; i<16; i++ ) Z[i] = 0;
      AES_128(key,Z,L); 

      if ( (L[0] & 0x80) == 0 ) { /* If MSB(L) = 0, then K1 = L << 1 */
          leftshift_onebit(L,K1);
      } else {    /* Else K1 = ( L << 1 ) (+) Rb */
          leftshift_onebit(L,tmp);
          xor_128(tmp,const_Rb,K1);
      }

      if ( (K1[0] & 0x80) == 0 ) {
          leftshift_onebit(K1,K2);
      } else {
          leftshift_onebit(K1,tmp);
          xor_128(tmp,const_Rb,K2);
      }
      return;
  }
void AES_CMAC (u8 *key, u8 *input, int length, u8 *mac )
{
	// printf("length is %d",length);
    u8 X[16],Y[16], M_last[16], padded[16];
    u8 K1[16], K2[16]; //存储生成的k1,k2
    int n, i, flag;
    generate_subkey(key,K1,K2); // 用key得到subkey: K1 和 K2
    n = (length+ 127) / 128;       /* n is number of rounds */ 
    // cout<<endl<< "rounds"<<n<<endl;
    if ( n == 0 ) {
        n = 1; flag = 0;
    }else{
        if ( (length% 128) == 0 ) { flag = 1; } /* last block is a complete block */
        else {  flag = 0; }  /* last block is not complete block */
    }
    if ( flag ) { /* last block is complete block */
        xor_128(&input[16*(n-1)],K1, M_last);
    } else {
        padding(&input[16*(n-1)],padded,length);
        // printf("padding  :"); print128(padded); printf("\n");
        xor_128(padded,K2,M_last);
    }
    // printf("M_last : "); print128(M_last); printf("\n");
    AES_KEY AESkey;
    for ( i=0; i<16; i++ ) X[i] = 0;
    for ( i=0; i<n-1; i++ ) {
        xor_128(X,&input[16*i],Y); /* Y := Mi (+) X  */
        AES_128(key,Y,X);      /* X := AES-128(KEY, Y); */
    }
    xor_128(X,M_last,Y);
    AES_128(key,Y,X);
    for ( i=0; i<16; i++ ) { mac[i] = X[i];}
}
int main(int argc, char const *argv[]) {
    if(argc != 2){
    cout <<"=============================================================="<<endl
         <<"Program usage: ./exec <input.txt> "<<endl
         <<"==============================================================" << endl;
         return 1;}
    ifstream input_s(argv[1]); /// input.txt
    string tmp_data, Direction, Length,Bearer;
    vector<string> Key,Count,Plaintext;
    int flag;
    while(input_s >> tmp_data){
        // 检查是否要切换读取的数据类型
        if(tmp_data == "IK")           { flag = 1; continue;}
        else if(tmp_data == "Count-I")    { flag = 2; continue;}
        else if(tmp_data == "Bearer")   { flag = 3; continue;}
        else if(tmp_data == "Direction"){ flag = 4; continue;}
        else if(tmp_data == "Length")   { flag = 5; continue;}
        else if(tmp_data == "Message"){ flag = 6; continue;}
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
    vector<ui> key_uc, cou_uc, pla_uc;
    int dir_uc = stoi(Direction);// direction用整数存, 0 or 1
    int len_uc = stoi(Length); // len用整数存
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
    ui tmp = stoul(ss.str(), 0, 16); 
    s_to_ui(Key, key_uc);
    s_to_ui(Count, cou_uc);
    s_to_ui(Plaintext, pla_uc);
    cou_uc.push_back(tmp); // 添加到T1中
    for(int i = 0; i < 3; i++){ cou_uc.push_back(0x00);}
    // 在Count+bearer+direction+0的后面加上Message信息，也就是plaintext中
    for(auto x : pla_uc){ cou_uc.push_back(x); }
    int M_size;
	M_size = (len_uc + 7)/8;
    u8 M[M_size + 8], enc_key[16];
    copy(begin(key_uc), end(key_uc), enc_key); // 得到key
    copy(begin(cou_uc), end(cou_uc), M); // 得到Message = Counter+bearer+dir+ Message
    u8 iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, AES_BLOCK_SIZE);
    u8 L[16], K1[16], K2[16], T[16];
    char* empty_str = NULL;
    //============================================================================//
                              // integrity check 
    //============================================================================//
      printf("--------------------------------------------------\n");
      printf("Subkey Generation\n");
      printf("K              "); print128(enc_key); printf("\n");
      AES_128(enc_key,const_Zero,L);
      printf("AES_128(key,0) "); print128(L); printf("\n");
      generate_subkey(enc_key,K1,K2);
      printf("K1             "); print128(K1); printf("\n");
      printf("K2             "); print128(K2); printf("\n");
      printf("M              "); 
      // print_hex(empty_str,M, 16);
      for(int i = 0; i < M_size + 8; i++){ // print M
          printf("%02x", M[i]);
          if(i%4 == 3) printf(" ");
      }
      // int M_len = (64 + len_uc)/8;
      int M_len = M_size + 8;
      AES_CMAC(enc_key,M,len_uc+ 64,T);
      printf("\n--------------------------------------------------\n");
      cout<<"    so first 32 bits of AES_CMAC is the MACT: "<<endl;
      printf("--------------------------------------------------\n");
      printf("AES_CMAC=      "); print128(T); printf("\n");
      printf("MACT    =      ");
      for(int i = 0; i < 4; i++){
        printf("%02x", T[i]);
      }
      printf("\n--------------------------------------------------\n");
    return 0;
}
