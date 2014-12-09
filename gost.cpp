#include <iostream>
#include <fstream>
#include <memory.h>
#include "gost.h"

//from GOST 28147-89
unsigned char S[8][16] = { //S-blocks, testing parametrs
    { 4, 10, 9, 2, 13, 8, 0, 14, 6, 11, 1, 12, 7, 15, 5, 3 },
    { 14, 11, 4, 12, 6, 13, 15, 10, 2, 3, 8, 1, 0, 7, 5, 9 },
    { 5, 8, 1, 13, 10, 3, 4, 2, 14, 15, 12, 7, 6, 0, 9, 11 },
    { 7, 13, 10, 1, 0, 8, 9, 15, 14, 4, 6, 12, 11, 2, 5, 3 },
    { 6, 12, 7, 1, 5, 15, 13, 8, 4, 10, 9, 14, 0, 3, 11, 2 },
    { 4, 11, 10, 0, 7, 2, 1, 13, 3, 6, 8, 5, 9, 12, 15, 14 },
    { 13, 11, 4, 1, 3, 15, 5, 9, 0, 10, 14, 7, 6, 8, 2, 12 },
    { 1, 15, 13, 0, 5, 7, 10, 4, 9, 2, 3, 14, 6, 11, 8, 12 },
};

int fi(int arg)
{
    int i = arg & 0x03;
    int k = arg >> 2; 
    k++;
    return (i << 3) + k - 1;
}

void E_f(unsigned char A[], unsigned char K[], unsigned char R[]) //from function E();
{ 
    int c = 0;                                          //Summ mod 2^32 c - carry 
    for (int i = 0; i < 4; i++) 
    {
        c += A[i] + K[i];
        R[i] = c & 0xFF;
        c >>= 8;
    }
    //Replacement to S-blocks
    for (int i = 0; i < 8; i++) 
    {                  
        int x = R[i >> 1] & ((i & 1) ? 0xF0 : 0x0F);    //x - 4bit segment
        R[i >> 1] ^= x;                                
        x >>= (i & 1) ? 4 : 0;                          
        x = S[i][x];                                    //Replace to S-blocks
        R[i >> 1] |= x << ((i & 1) ? 4 : 0);            
    }
    int tmp = R[3];                                     
    R[3] = R[2];
    R[2] = R[1];
    R[1] = R[0];
    R[0] = tmp;
    tmp = R[0] >> 5;                                    
    for (int i = 1; i < 4; i++) 
    {
        int nTmp = R[i] >> 5;
        R[i] = (R[i] << 3) | tmp;
        tmp = nTmp;
    }
    R[0] = (R[0] << 3) | tmp;
} 

void E(unsigned char D[], unsigned char K[], unsigned char R[]) // GOST 28147-89
{ 
    unsigned char A[4], B[4];                               
    for (int i = 0; i < 4; i++) 
        A[i] = D[i];
    for (int i = 0; i < 4; i++) 
        B[i] = D[i + 4];
    for (int step = 0; step < 3; step++)                
    {
        for (int i = 0; i < 32; i += 4)
        {
            unsigned char tmp[4];
            E_f(A, K + i, tmp);                         
            for (int i = 0; i < 4; i++)
                tmp[i] ^= B[i];
            memcpy(B, A, sizeof A);
            memcpy(A, tmp, sizeof tmp);
        }
    }
    for (int i = 28; i >= 0; i -= 4) 
    { 
        unsigned char tmp[4]; 
        E_f(A, K + i, tmp);
        for (int i = 0; i < 4; i++) 
            tmp[i] ^= B[i];
        memcpy(B, A, sizeof A);
        memcpy(A, tmp, sizeof tmp);
    }
    for (int i = 0; i < 4; i++) 
        R[i] = B[i];      
    for (int i = 0; i < 4; i++) 
        R[i + 4] = A[i];
}

void A(unsigned char Y[], unsigned char R[]) 
{
    for (int i = 0; i < 24; i++) 
        R[i] = Y[i + 8];
    for (int i = 0; i < 8; i++) 
        R[i + 24] = Y[i] ^ Y[i + 8];
}

void P(unsigned char Y[], unsigned char R[]) 
{
    for (int i = 0; i < 32; i++) 
        R[i] = Y[fi(i)];
}

void psi(unsigned char arr[]) 
{
    unsigned char y16[] = { 0, 0 };
    y16[0] ^= arr[0]; y16[1] ^= arr[1];
    y16[0] ^= arr[2]; y16[1] ^= arr[3];
    y16[0] ^= arr[4]; y16[1] ^= arr[5];
    y16[0] ^= arr[6]; y16[1] ^= arr[7];
    y16[0] ^= arr[24]; y16[1] ^= arr[25];
    y16[0] ^= arr[30]; y16[1] ^= arr[31];
    for (int i = 0; i < 30; i++) 
        arr[i] = arr[i + 2];
    arr[30] = y16[0]; arr[31] = y16[1];
}

void psi(unsigned char arr[], int p) 
{
    while (p--) 
        psi(arr);
}

void f(unsigned char H[], unsigned char M[], unsigned char newH[]) 
{
	unsigned char U[32], V[32], W[32], S[32], C[4][32], K[4][32], tmp[32];
    memset(C, 0, sizeof C);
    C[2][0] = 0x00;	 C[2][1] = 0xFF;  C[2][2] = 0x00;  C[2][3] = 0xFF;  C[2][4] = 0x00;  C[2][5] = 0xFF;
    C[2][6] = 0x00;	 C[2][7] = 0xFF;  C[2][8] = 0xFF;  C[2][9] = 0x00;  C[2][10] = 0xFF; C[2][11] = 0x00;
    C[2][12] = 0xFF; C[2][13] = 0x00; C[2][14] = 0xFF; C[2][15] = 0x00; C[2][16] = 0x00; C[2][17] = 0xFF;
    C[2][18] = 0xFF; C[2][19] = 0x00; C[2][20] = 0xFF; C[2][21] = 0x00; C[2][22] = 0x00; C[2][23] = 0xFF;
    C[2][24] = 0xFF; C[2][25] = 0x00; C[2][26] = 0x00; C[2][27] = 0x00; C[2][28] = 0xFF; C[2][29] = 0xFF;
    C[2][30] = 0x00; C[2][31] = 0xFF; 
	memcpy(U, H, sizeof U);
    memcpy(V, M, sizeof V);
    for (int i = 0; i < 32; i++) 
        W[i] = U[i] ^ V[i];
    P(W, K[0]);
    for (int step = 1; step < 4; step++) 
    {
        A(U, tmp); 
        for (int i = 0; i < 32; i++) 
            U[i] = tmp[i] ^ C[step][i];
        A(V, tmp); 
        A(tmp, V);
        for (int i = 0; i < 32; i++) 
            W[i] = U[i] ^ V[i];
        P(W, K[step]);
    }
    for (int i = 0; i < 32; i += 8)
        E(H + i, K[i >> 3], S + i);
    psi(S, 12); 
    for (int i = 0; i < 32; i++) 
        S[i] ^= M[i];
    psi(S, 1); 
    for (int i = 0; i < 32; i++) 
        S[i] ^= H[i];
    psi(S, 61);
    memcpy(newH, S, sizeof S);
}

bool gost(char *inFile, char *outFile)
{                                                               
	unsigned char *buf, result[32], block[32], Sum[32], L[32], H[32], newH[32];
	int len;
	unsigned int pos = 0, posIB = 0;
	std::ifstream in(inFile, std::ios::binary);                                                     
	if (in.fail())                                                                          
		return false;                                                                       
	in.seekg(0, std::ios::end);                                                                  
	len = (unsigned int)in.tellg();                                      
	in.seekg(0, std::ios::beg);   
	buf = new unsigned char[len];
	in.read((char*)buf, len);                                                                
	in.close();                                                                             
    memset(Sum, 0, sizeof Sum);
    memset(H, 0, sizeof H);
    while ((posIB < len) || pos) 
    {
        if (posIB < len) 
            block[pos++] = buf[posIB++];
        else 
            block[pos++] = 0;
        if (pos == 32) 
        {
            pos = 0;
            int c = 0;
            for (int i = 0; i < 32; i++) 
            {
                c += block[i] + Sum[i];
                Sum[i] = c & 0xFF;
                c >>= 8;
            }
            f(H, block, newH); 
            memcpy(H, newH, sizeof newH);
        }
    }
    memset(L, 0, sizeof L);
    int c = len << 3;
    for (int i = 0; i < 32; i++) 
    {
        L[i] = c & 0xFF;
        c >>= 8;
    }
    f(H, L, newH);
    memcpy(H, newH, sizeof newH);
    f(H, Sum, newH);
	memcpy(result, newH, sizeof newH);
	std::ofstream out(outFile);                                                                 
	if(out.fail())                                                                         
		return false;                                                                       
	for (int i = 0; i < 32; i++)
		out<<std::hex<<(unsigned int)result[i];
    delete [] buf;
}
