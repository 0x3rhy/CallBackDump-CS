#include <stdio.h>



int main(int argc, char* argv[]) {


	if (argc < 4) {
		printf("[*] usage: dumpXorDecrypt <encryptfile> <decryptfile> <xor key>\n");
		return 0;
	}

	int keylen, index = 0;
	char* source, * dest, fBuffer[1], tBuffer[20], ckey;
	FILE* fSource, * fDest;

	source = argv[1];
	dest = argv[2];
	char* key = argv[3];

	keylen = sizeof(key);

	fSource = fopen(source, "rb");
	fDest = fopen(dest, "wb");

	while (!feof(fSource)) {

		fread(fBuffer, 1, 1, fSource);

		if (!feof(fSource)) {
			ckey = key[index % keylen];
			*fBuffer = *fBuffer ^ ckey;
			fwrite(fBuffer, 1, 1, fDest);
			index++;
		}

	}

	fclose(fSource);
	fclose(fDest);
	printf("decrypt end\n");

}
