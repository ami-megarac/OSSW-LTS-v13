/* Copyright (c) 2002 Johnny Shelley All rights reserved.

 * Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above 
 *    copyright notice, this list of conditions and the 
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the 
 *    following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of the author nor any contributors may
 *    be used to endorse or promote products derived from 
 *    this software without specific prior written permission.
 *    
 *    THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDERS AND
 *    CONTRIBUTORS ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 *    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *    
 *    This algorithm is licensed under the BSD software license.    
 */

#ifndef __BLOWFISH_H__
#define __BLOWFISH_H__
 
//To get proper user defined sizes.
#define REQ_ENCRYPT_BUFF_SIZE( X )  (X % 8)  != 0 ? X + ( 8 - ( X % 8 )): X
#define REQ_DECRYPT_BUFF_SIZE( X )  (X % 8)  != 0 ? X + ( 8 - ( X % 8 )): X
#define MAX_SIZE_KEY 56		/* For Blowfish Encryption Algorithm, the key can have maximum of 56 bytes. */

typedef void * BFHANDLE;

#define		ENCRYPTKEY_FILE		"/conf/pwdEncKey"
BFHANDLE blowfishInit(  unsigned char *key, int keyLen);
int blowfishEncryptPacket(char *inBuf, unsigned int  sizeInBuf, char *outBuf, unsigned int sizeOutBuf, BFHANDLE );
int blowfishDecryptPacket( char *packet, int packLen,BFHANDLE );
void blowfishClose(BFHANDLE ctx );
unsigned int rotatedWord(unsigned int dWord);
extern int EncryptPassword (char* PlainPswd, unsigned char sizeInBuf, char* EncryptPswd, unsigned char sizeOutBuf, unsigned char* EncryptKey);
extern int DecryptPassword (char* EncryptPswd, unsigned char sizeInBuf, char* DecryptPswd, unsigned char sizeOutBuf, unsigned char* EncryptKey);
extern int getEncryptKey(unsigned char* EncryptKey);
extern int setEncryptKey(unsigned char* EncryptKey);
#endif // __BLOWFISH_H__



