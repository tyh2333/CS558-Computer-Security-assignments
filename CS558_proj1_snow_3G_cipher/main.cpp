/*
    input file: 128 bits key , 128 bits variable
    KEY 0xaaaaaaaa 0x1234bbbb 0xbbbbbbbb 0xcccccccc
    VAR 0xabcdabcd 0x11111111 0xabcdabcd 0x22222222
    NUM 5
*/
/*
    output file: produce 32 bits words
    c8b2a283
    f6a76785
    44457389
    61e19c8e
    08674b90
*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <string>
using namespace std;
typedef unsigned int ui;
ui R1 = 0x00; ui R2 = 0x00; ui R3 = 0x00;
vector<ui> S(16, 0x00); /// s0 - s15
vector<ui> KS; /// store output

/// ========================================
///             implementation
///=========================================

int get_leftmost_bit(ui number){
    return (number & 0xFF) >> 7;
}

vector<ui> trans_8_to_4x2(ui number){
    vector<ui> ans;
    ui part1 = (number >> 4) & 0xF;
    ui part2 =  (number & 0xF);
    ans.push_back(part1);      ans.push_back(part2);
    return ans;
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

ui MULx(ui V, ui c){
/// maps 16 bits to 8 bits.  Let V and c be 8-bit input values.
///   Then MULx is defined:
///  (1)If the leftmost bit of V equals 1, t = 1, n = 8
///       MULx(V,c) = (V<<8 1) XOR c  (t-bit left shift in an n-bit register.)
///  (2)else
///       MULx(V,c) = (V<<8 1)
///   e.g. (1) MULx(0x69,0x1B) =0xD2;      (2) MULx(0x96,0x1B) =0x37;
///    (1) for example 1:         l_most = 0, so v<< 1 directly, 1101 0100 =>0xD2
///       0x69 => 6 = 0 1 1 0
///                9 = 1 0 1 0
///    (2) for example 2:         l_most = 1, so v<< 1 XOR c, 0100 1101 XOR 0001 1011 => 0101 0111 =0x37
///       0x96 => 9 = 1 0 1 0
///                6 = 0 1 1 0
    if(get_leftmost_bit(V) == 1){
        return (((V << 1) & 0xFF) ^ c);
    }
    return ((V << 1) & 0xFF);
}

ui MULy(ui V, int i, ui c){
///     MULy maps 16 bits and a positive integer i to 8 bit.
///     Let V and c be 8-bit input values, then MULy(V, i, c) is recursively defined:
///      (1) If i equals 0:  MULy(V, i, c) = V,
///      (2) else MULy(V, i, c) = MULx(MULy(V, i - 1, c), c).
    if(i == 0) return (V & 0xFF);
    return (MULx(MULy(V, i - 1, c), c)&0xFF);
}

int trans_letter_to_int(ui letter){
    /// for check SR and SQ
    if(letter == 0xa) return 10;
    if(letter == 0xb) return 11;
    if(letter == 0xc) return 12;
    if(letter == 0xd) return 13;
    if(letter == 0xe) return 14;
    if(letter == 0xf) return 15;
    else return (int)letter;
}

ui SR(ui w){ /// just search the form x
//  SR(x0 2^4+x1) = y0 2^4 + y1
//  SR(42) = SR(0x2A) =0xE5 = 229;
    int S_box_R[16][16] = {
            {0x63,0x7C,0x77,0x7B,0xF2,0x6B,0x6F,0xC5,0x30,0x01,0x67,0x2B,0xFE,0xD7,0xAB,0x76},///(0)
            {0xCA,0x82,0xC9,0x7D,0xFA,0x59,0x47,0xF0,0xAD,0xD4,0xA2,0xAF,0x9C,0xA4,0x72,0xC0},///(1)
            {0xB7,0xFD,0x93,0x26,0x36,0x3F,0xF7,0xCC,0x34,0xA5,0xE5,0xF1,0x71,0xD8,0x31,0x15},///(2)
            {0x04,0xC7,0x23,0xC3,0x18,0x96,0x05,0x9A,0x07,0x12,0x80,0xE2,0xEB,0x27,0xB2,0x75},///(3)
            {0x09,0x83,0x2C,0x1A,0x1B,0x6E,0x5A,0xA0,0x52,0x3B,0xD6,0xB3,0x29,0xE3,0x2F,0x84},///(4)
            {0x53,0xD1,0x00,0xED,0x20,0xFC,0xB1,0x5B,0x6A,0xCB,0xBE,0x39,0x4A,0x4C,0x58,0xCF},///(5)
            {0xD0,0xEF,0xAA,0xFB,0x43,0x4D,0x33,0x85,0x45,0xF9,0x02,0x7F,0x50,0x3C,0x9F,0xA8},///(6)
            {0x51,0xA3,0x40,0x8F,0x92,0x9D,0x38,0xF5,0xBC,0xB6,0xDA,0x21,0x10,0xFF,0xF3,0xD2},///(7)
            {0xCD,0x0C,0x13,0xEC,0x5F,0x97,0x44,0x17,0xC4,0xA7,0x7E,0x3D,0x64,0x5D,0x19,0x73},///(8)
            {0x60,0x81,0x4F,0xDC,0x22,0x2A,0x90,0x88,0x46,0xEE,0xB8,0x14,0xDE,0x5E,0x0B,0xDB},///(9)
            {0xE0,0x32,0x3A,0x0A,0x49,0x06,0x24,0x5C,0xC2,0xD3,0xAC,0x62,0x91,0x95,0xE4,0x79},///(10 A)
            {0xE7,0xC8,0x37,0x6D,0x8D,0xD5,0x4E,0xA9,0x6C,0x56,0xF4,0xEA,0x65,0x7A,0xAE,0x08},///(11 B)
            {0xBA,0x78,0x25,0x2E,0x1C,0xA6,0xB4,0xC6,0xE8,0xDD,0x74,0x1F,0x4B,0xBD,0x8B,0x8A},///(12 C)
            {0x70,0x3E,0xB5,0x66,0x48,0x03,0xF6,0x0E,0x61,0x35,0x57,0xB9,0x86,0xC1,0x1D,0x9E},///(13 D)
            {0xE1,0xF8,0x98,0x11,0x69,0xD9,0x8E,0x94,0x9B,0x1E,0x87,0xE9,0xCE,0x55,0x28,0xDF},///(14 E)
            {0x8C,0xA1,0x89,0x0D,0xBF,0xE6,0x42,0x68,0x41,0x99,0x2D,0x0F,0xB0,0x54,0xBB,0x16}///(15 F)
    };
/// 把输入的8 bit拆成两个 4 bit +  直接查表
    vector<ui> four = trans_8_to_4x2(w);
    int i = 0, j = 0;
//    cout<< four[0]<<four[1]<<endl;
    i = trans_letter_to_int(four[0]); /// four[0] = 2
    j = trans_letter_to_int(four[1]); /// four[1] = a
    return S_box_R[i][j];

}

ui SQ(ui w) { /// just search the form x
///  SR(42) = SR(0x2A) =0xAC = 172;
    int S_box_Q[16][16] = {
            {0x25, 0x24, 0x73, 0x67, 0xD7, 0xAE, 0x5C, 0x30, 0xA4, 0xEE, 0x6E, 0xCB, 0x7D, 0xB5, 0x82, 0xDB},///(0)
            {0xE4, 0x8E, 0x48, 0x49, 0x4F, 0x5D, 0x6A, 0x78, 0x70, 0x88, 0xE8, 0x5F, 0x5E, 0x84, 0x65, 0xE2},///(1)
            {0xD8, 0xE9, 0xCC, 0xED, 0x40, 0x2F, 0x11, 0x28, 0x57, 0xD2, 0xAC, 0xE3, 0x4A, 0x15, 0x1B, 0xB9},///(2)
            {0xB2, 0x80, 0x85, 0xA6, 0x2E, 0x02, 0x47, 0x29, 0x07, 0x4B, 0x0E, 0xC1, 0x51, 0xAA, 0x89, 0xD4},///(3)
            {0xCA, 0x01, 0x46, 0xB3, 0xEF, 0xDD, 0x44, 0x7B, 0xC2, 0x7F, 0xBE, 0xC3, 0x9F, 0x20, 0x4C, 0x64},///(4)
            {0x83, 0xA2, 0x68, 0x42, 0x13, 0xB4, 0x41, 0xCD, 0xBA, 0xC6, 0xBB, 0x6D, 0x4D, 0x71, 0x21, 0xF4},///(5)
            {0x8D, 0xB0, 0xE5, 0x93, 0xFE, 0x8F, 0xE6, 0xCF, 0x43, 0x45, 0x31, 0x22, 0x37, 0x36, 0x96, 0xFA},///(6)
            {0xBC, 0x0F, 0x08, 0x52, 0x1D, 0x55, 0x1A, 0xC5, 0x4E, 0x23, 0x69, 0x7A, 0x92, 0xFF, 0x5B, 0x5A},///(7)
            {0xEB, 0x9A, 0x1C, 0xA9, 0xD1, 0x7E, 0x0D, 0xFC, 0x50, 0x8A, 0xB6, 0x62, 0xF5, 0x0A, 0xF8, 0xDC},///(8)
            {0x03, 0x3C, 0x0C, 0x39, 0xF1, 0xB8, 0xF3, 0x3D, 0xF2, 0xD5, 0x97, 0x66, 0x81, 0x32, 0xA0, 0x00},///(9)
            {0x06, 0xCE, 0xF6, 0xEA, 0xB7, 0x17, 0xF7, 0x8C, 0x79, 0xD6, 0xA7, 0xBF, 0x8B, 0x3F, 0x1F, 0x53},///(10 A)
            {0x63, 0x75, 0x35, 0x2C, 0x60, 0xFD, 0x27, 0xD3, 0x94, 0xA5, 0x7C, 0xA1, 0x05, 0x58, 0x2D, 0xBD},///(11 B)
            {0xD9, 0xC7, 0xAF, 0x6B, 0x54, 0x0B, 0xE0, 0x38, 0x04, 0xC8, 0x9D, 0xE7, 0x14, 0xB1, 0x87, 0x9C},///(12 C)
            {0xDF, 0x6F, 0xF9, 0xDA, 0x2A, 0xC4, 0x59, 0x16, 0x74, 0x91, 0xAB, 0x26, 0x61, 0x76, 0x34, 0x2B},///(13 D)
            {0xAD, 0x99, 0xFB, 0x72, 0xEC, 0x33, 0x12, 0xDE, 0x98, 0x3B, 0xC0, 0x9B, 0x3E, 0x18, 0x10, 0x3A},///(14 E)
            {0x56, 0xE1, 0x77, 0xC9, 0x1E, 0x9E, 0x95, 0xA3, 0x90, 0x19, 0xA8, 0x6C, 0x09, 0xD0, 0xF0, 0x86},///(15 F)
    };
    /// 把输入的8 bit拆成两个 4 bit +  直接查表
    vector<ui> four = trans_8_to_4x2(w);
    int i = 0, j = 0;
    //    cout<< four[0]<<four[1]<<endl;
    i = four[0]; /// four[0] = 2
    j = four[1]; /// four[1] = a
//    i = trans_letter_to_int(four[0]); /// four[0] = 2
//    j = trans_letter_to_int(four[1]); /// four[1] = a
    return S_box_Q[i][j];
}

ui s1(ui w){ /// used in FSM
    vector<ui> W = trans_32_to_8x4(w);
    ui w0 = W[0]; ui w1 = W[1]; ui w2 = W[2]; ui w3 = W[3];
    ui r0, r1, r2, r3;
    r0 = MULx(SR(w0),0x1B) ^ SR(w1) ^ SR(w2) ^ MULx(SR(w3),0x1B) ^ SR(w3);
    r1 = MULx(SR(w0),0x1B) ^ SR(w0) ^ MULx(SR(w1),0x1B) ^ SR(w2) ^ SR(w3);
    r2 = SR(w0) ^ MULx(SR(w1),0x1B) ^ SR(w1) ^ MULx(SR(w2),0x1B) ^ SR(w3);
    r3 = SR(w0) ^ SR(w1) ^ MULx(SR(w2),0x1B) ^ SR(w2) ^ MULx(SR(w3),0x1B);
    return (r0<<24) |(r1<<16)|(r2<<8)|r3;
}

ui s2(ui w){
    vector<ui > ret;
    vector<ui> W = trans_32_to_8x4(w);
    ui w0 = W[0]; ui w1 = W[1]; ui w2 = W[2]; ui w3 = W[3];
    ui r0, r1, r2, r3;
    r0 = (MULx(SQ(w0),0x69) ^ SQ(w1) ^ SQ(w2) ^ (MULx(SQ(w3),0x69) ^ SQ(w3)));
    r1 = ((MULx(SQ(w0),0x69) ^ SQ(w0)) ^ MULx(SQ(w1),0x69) ^ SQ(w2) ^ SQ(w3));
    r2 = (SQ(w0) ^ (MULx(SQ(w1),0x69) ^ SQ(w1)) ^ MULx(SQ(w2),0x69) ^ SQ(w3));
    r3 = (SQ(w0) ^ SQ(w1) ^ MULx(SQ(w2),0x69) ^ SQ(w2) ^ MULx(SQ(w3),0x69));
    return (r0<<24) |(r1<<16)|(r2<<8)|r3;
}

ui MUL_alpha(ui c){ /// c : 8-bit word
    /// The function MUL_alpha maps 8 bits to 32 bits. Let c be the 8-bit input, then MUL_alpha is defined as:
    /// MUL_alpha(c) = MULy(c, 23, 0xA9) || MULy(c, 245, 0xA9) || MULy(c, 48, 0xA9) || MULy(c, 239, 0xA9)
    /// Debug: 结合算法 right ✔
    ui p0 = MULy(c, 23,  0xA9);
    ui p1 = MULy(c, 245, 0xA9);
    ui p2 = MULy(c, 48,  0xA9);
    ui p3 = MULy(c, 239, 0xA9);
    return (p3 + (p2<<8) + (p1<<16) + (p0<<24));
}

ui DIV_alpha(ui c){
    /// The function DIV_alpha maps 8 bits to 32 bits. Let c be the 8-bit input, then DIV_alpha is defined as:
    /// DIV_alpha(c) = MULy(c, 16, 0xA9) || MULy(c, 39, 0xA9) || MULy(c, 6, 0xA9) || MULy(c, 64, 0xA9)).
    ui p0 = MULy(c, 16, 0xA9);
    ui p1 = MULy(c, 39, 0xA9);
    ui p2 = MULy(c, 6,  0xA9);
    ui p3 = MULy(c, 64, 0xA9);
    return (p3 + (p2<<8) + (p1<<16) + (p0<<24));
}

ui FSM(){/// arguments are all 32-bit
    ui F; /// return is a 32-bit word F
    ui r; /// intermediate value r
    F = ((S[15] + R1) & 0xFFFFFFFF) ^ R2;
    r = (R2 + (R3 ^ S[5])) & 0xFFFFFFFF;
    R3 = s2(R2);
    R2 = s1(R1);
    R1 = r;
    return F; /// 32-bit word
}

void LFSR_init_mode(ui F){
    /// have input: 32-bit word F(output from FSM)
    vector<ui> s_zero;
    vector<ui> s_eleven;
    s_zero = trans_32_to_8x4(S[0]);
    s_eleven = trans_32_to_8x4(S[11]);
    ui v;
    ui p1 = (s_zero[1]<<24)|(s_zero[2]<<16)|(s_zero[3]<<8|0x00);
    ui p2 = MUL_alpha(s_zero[0]);
    ui p3 = S[2];
    ui p4 = (0x00<<24)|(s_eleven[0]<<16)|(s_eleven[1]<<8|s_eleven[2]);
    ui p5 = DIV_alpha(s_eleven[3]);
    v = p1 ^ p2 ^ p3 ^ p4 ^ p5 ^ F;
    for(int i = 0; i < 15; ++i) S[i] = S[i+1];
    S[15] = v;
}

void LFSR_KeyStream_mode(){
    /// compared to LFSR_init_mode: don't have input F
    vector<ui> s_zero;
    vector<ui> s_eleven;
    s_zero = trans_32_to_8x4(S[0]);
    s_eleven = trans_32_to_8x4(S[11]);
    ui v;
    ui p1 = (s_zero[1]<<24)|(s_zero[2]<<16)|(s_zero[3]<<8)|0x00;
    ui p2 = MUL_alpha(s_zero[0]);
    ui p3 = S[2];
    ui p4 = (0x00<<24)+(s_eleven[0]<<16)+(s_eleven[1]<<8)+(s_eleven[2]);
    ui p5 = DIV_alpha(s_eleven[3]);
    v = p1 ^ p2 ^ p3 ^ p4 ^ p5;
    for(int i = 0; i < 15; ++i) S[i] = S[i+1];
    S[15] = v;
}

void initialization(ui k0, ui k1,ui k2, ui k3, ui iv0, ui iv1, ui iv2, ui iv3){
///  The FSM is initialized with R1 = R2 = R3 = 0.
///  Then the cipher runs in a special mode without producing output:
    /// K store k0,k1,k2,k3, IV store IV0,IV1,IV2,IV3, all 32 bits
    /// to get s0, s1, ... s15 , all 32 bits
    ui one = 0xffffffff;
    S[15] = k3 ^ iv0;
    S[14] = k2;
    S[13] = k1;
    S[12] = k0 ^ iv1;
    S[11] = k3 ^ one;
    S[10] = k2 ^ one ^ iv2;
    S[9] = k1 ^ one ^ iv3;
    S[8] = k0 ^ one;
    S[7] = k3;
    S[6] = k2;
    S[5] = k1;
    S[4] = k0;
    S[3] = k3 ^ one;
    S[2] = k2 ^ one;
    S[1] = k1 ^ one;
    S[0] = k0 ^ one;
    for(int i = 0; i < 32; i++) {
    /// STEP 1: The FSM is clocked producing the 32-bit word F.
        ui F = FSM();
    /// STEP 2: Then the LFSR is clocked in Initialization Mode consuming F.
        LFSR_init_mode(F);
    }
}

void generate_keystream(int n){
/// (1) n : the number of 32-bit words produced in the keystream
/// (2) KS :  array of n produced 32-bit words

/// First the FSM is clocked once. discarded output word
    FSM();
///  Then the LFSR is clocked once in Keystream Mode.
    LFSR_KeyStream_mode(); /// don't need input F
///  After loop n 32-bit words of keystream are produced:
    for(int i = 0; i < n; i++){
/// STEP 1:
///     The FSM is clocked and produces a 32-bit output word F.
        ui F = FSM();
/// STEP 2:
///     The next keystream word is computed as zt = F ^ s0.
        KS.push_back(F ^ S[0]);
//        cout<<hex<<tmp<<endl;
/// STEP 3:
///     Then the LFSR is clocked in Keystream Mode.
        LFSR_KeyStream_mode();
    }
}

int main(int argc, char const *argv[]) {
    if(argc != 5){
        cout <<"=============================================================="<<endl
        <<"Program usage: ./main key.txt iv.txt n output.txt"<<endl
        <<"==============================================================" << endl;
        return 1;
    }

    /// key and IV Read :
    ifstream key_s(argv[1]); /// key.txt
    ifstream iv_s(argv[2]); /// iv.txt
    int n = atoi(argv[3]);         /// string needs to be transform to int
    string output = argv[4];      /// output.txt
    string tmp_key = "",tmp_iv = "";
    vector<string> keys, ivs;
    while(key_s >> tmp_key) keys.push_back(tmp_key);
    while(iv_s >> tmp_iv) ivs.push_back(tmp_iv);
    key_s.close();
    iv_s.close();

    /// Key and IV split :
    /// usage : need to #include <cstdlib>
    ///         unsigned long stoul (const string&  str, size_t* idx = 0, int base = 10);
    ui k0 = stoul(keys[0], 0, 16);
    ui k1 = stoul(keys[1], 0, 16);
    ui k2 = stoul(keys[2], 0, 16);
    ui k3 = stoul(keys[3], 0, 16);
    ui iv0 = stoul(ivs[0], 0, 16);
    ui iv1 = stoul(ivs[1], 0, 16);
    ui iv2 = stoul(ivs[2], 0, 16);
    ui iv3 = stoul(ivs[3], 0, 16);

    /// initialization
    initialization( k0, k1, k2, k3, iv0, iv1, iv2, iv3);

    /// generate_keystream
    generate_keystream(n);

    /// Test for KS :
    cout<<"The "<< n << " keys we got are as following: "<< endl;
    /// fill front '0's. need to #include <iostream>  and #include <iomanip>
    ///     setfill: fill what kind of char
    ///     setw:    fill to how many chars
    for(ui x : KS) cout << hex<< setfill('0') << setw(8) << x << endl;
    /// Write to output.txt
    ofstream o(output);
    for(ui x : KS) o << hex << setfill('0') << setw(8) << x <<endl;

    ///=========================================
    ///              TEST PART
    ///=========================================
////    vector<ui> k = {0xaaaaaaaa, 0x1234bbbb, 0xbbbbbbbb, 0xcccccccc};
////    vector<ui> iv =  {0xabcdabcd, 0x11111111, 0xabcdabcd, 0x22222222};
////    int n = 5;
////    initialization( k[0], k[1], k[2],  k[3],  iv[0],  iv[1],  iv[2],  iv[3] );
////    generate_keystream(n);
//    cout<<hex << (0x01<<8|0x09)<<endl;
    //    cout<<"##########"<<endl;
//    cout<< trans_letter_to_int(0x2)<<endl;
//    cout<< trans_letter_to_int(0xa);
//    cout<< trans_32_to_8(0x12345678);
    /// (1) test trans bit functions : success
//   vector<ui> temp = trans_8_to_4x2(0x0bf);
//   for(auto x : temp) cout<<hex<<x<<endl;

    /// (2) Test SR and SQ : success
//    cout<<hex<< SR(0x2A)<<endl;
//    cout<<hex<< SQ(0x2A)<<endl;
//    cout<<hex<< SQ(0x1B)<<endl;
//    cout<<hex<< SQ(0x3A)<<endl;
//    cout<<hex<< SQ(0x42)<<endl;
//    cout<<hex<< SQ(0x53)<<endl;
//    cout<<hex<< SQ(0x64)<<endl;
//    cout<<hex<< SQ(0x75)<<endl;
//    cout<<hex<< SQ(0x86)<<endl;
//    cout<<hex<< SQ(0x97)<<endl;
    /// (3) Test | operations
//    ui p0 = 0x12;
//    ui p1 = 0x34;
//    ui p2 = 0x56;
//    ui p3 = 0x78;
//    ui p_final = (p3 + (p2<<8) + (p1<<16) + (p0<<24));
//    ui p_final1 = (p3 |(p2<<8) | (p1<<16) | (p0<<24));
//    cout<<hex<<p_final1<<p_final<<endl; /// p_final = 12345678

    /// (4) Test MULx, MULy : success
//    ui V = 0x23;
//    ui c = 0x45;
//    cout<< hex << MULy(V, 1, c)<<" "<<MULx(V, c);

    /// (5) TEST S1, S2 : success
//    ui w = 0x12345678;
//    vector<ui > W = trans_32_to_8x4(w);
//    for(auto x: W) cout<<hex<<x<<" ";
//    ui r0 = 0x12;
//    ui r1 = 0x34;
//    ui r2 = 0x56;
//    ui r3 = 0x78;
//    ui ret = S1(w);
//    ui ret_2 = S2(w);
//    cout<<hex<< ret <<endl<< ret_2 <<endl;
    return 0;
}
