/* BinDiff by Alexander Thomas (doctor.lex@gmail.com)
 * Released under the New BSD License (see LICENSE file for details).
 *
 * Version 1.0: 2004/10: unsightly first version
 *         1.1: 2010/03: code cleanup, more options
 *         1.2: 2013/05: file dumping, capital hex, concise output for our robotic overlords
 *         1.3: 2015/12: ensure binary I/O, allow comparing with fixed byte value, fix bug if final block differs, proper tests
 *
 * TODO: - allow insertion/deletion detection
 *       - refactor for more sanity and better testability.
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstring>
using namespace std;

#define BUFF_SIZE 16384
#define min(x,y) x < y ? x : y


void printUsage(const char *pName, bool bFull=false)
{
	if(bFull)
		cout << "Binary diff 1.3 by Alexander Thomas, 2004-2015.\n"
		     << "Compares two similar binary files.\n"
		     << "Released under the New BSD License.\n" << endl;
	cout << "Usage:  " << pName << " [-hcdstxC] [-b B] [-n N] [-f F] [-g G] [-y Y] file1 file2" << endl;
	if(bFull) {
		cout << "  -h: show usage information and exit.\n"
		     << "  -c: concise output format: \"startByte length\".\n"
		     << "  -d: output byte positions in decimal instead of hex.\n"
		     << "  -s: stop after first block of differing bytes.\n"
		     << "  -t: print total number of bytes in differing blocks.\n"
		     << "  -x: output lengths in hex instead of decimal.\n"
		     << "  -C: use capitals for hex values.\n"
		     << "  -b B: compare per block of B bytes.\n"
		     << "  -n N: treat N identical bytes or blocks in between two differing bytes/blocks\n"
		     << "      as different, i.e. treat the whole as the same chunk.\n"
		     << "  -f F: dump each differing block to a file with as name byte position prefixed\n"
		     << "      with F.\n"
		     << "  -g G: dump each original block to a file with as name byte position prefixed\n"
		     << "      with G.\n"
		     << "  -y Y: compare file1 with a virtual file of same length, filled with bytes Y.\n"
		     << "      Y must be an integer between 0 and 255 (hex allowed if prefixed with 0x).\n"
		     << endl;
	}
	else
		cout << "  Invoke with -h to see full usage information." << endl;
}

void exitArg(const char *pName, const char *msg = NULL)
{
	if(msg != NULL)
		cerr << msg << endl;
	printUsage(pName);
	exit(2);
}

unsigned long countRemaining(ifstream &file)
{
	char buffer[BUFF_SIZE];
	unsigned long remaining = 0;

	while(! file.eof()) {
		file.read(buffer, BUFF_SIZE);
		remaining += file.gcount();
	}
	return remaining;
}

/* Parses a string in decimal or hex format (prefixed with 0x) to an integer.
 * Returns -1 if value cannot be parsed as a positive integer. */
long parseUnsignedIntegerString(const char *input)
{
	bool bHex = false;
	string inputString(input);
	if(inputString.substr(0, 2) == "0x") {
		inputString.erase(0, 2);
		bHex = true;
		for(unsigned int i = 0; i < inputString.length(); i++) {
			if(! ((inputString[i] >= '0' && inputString[i] <= '9') ||
			      (inputString[i] >= 'A' && inputString[i] <= 'F') ||
			      (inputString[i] >= 'a' && inputString[i] <= 'f')))
				return -1;
		}
	}
	else {
		for(unsigned int i = 0; i < inputString.length(); i++) {
			if(! (inputString[i] >= '0' && inputString[i] <= '9'))
				return -1;
		}
	}
	if(inputString.length() == 0)
		return -1;

	std::istringstream ss(inputString);
	if(bHex)
		ss >> std::hex;
	unsigned long parsedValue;
	ss >> parsedValue;
	return parsedValue;
}


int main(int argc, char * argv[])
{
	long wInterDiff = 0;
	unsigned long wBlockSize = 1;
	bool bStop=false, bTotal=false, bConcise=false, bCapital=false, bDumpDiff=false, bDumpOrig=false, bVirtual=false;
	char chVirtual = 0;
	// I dislike the fact that ios_base::showbase also capitalizes 0x.
	string sPosBase("0x"), sLenBase("");
	char *file1=NULL, *file2=NULL;
	string sPrefixDiff, sPrefixOrig;
	ios_base::fmtflags posFmt = ios_base::hex;
	ios_base::fmtflags lenFmt = ios_base::dec;

	int a=0;
	while(++a < argc) {
		if(argv[a][0] == '-') {
			char *argum = argv[a];
			int len = strlen(argum);
			for(int j = 1; j < len; j++) {
				long scanValue = -1;

				switch(argum[j]) {
				case 'h':
					printUsage(argv[0], true);
					exit(0);
					break;
				case 'd':
					posFmt = ios_base::dec;
					sPosBase = "";
					break;
				case 's':
					bStop = true;
					break;
				case 't':
					bTotal = true;
					break;
				case 'c':
					bConcise = true;
					break;
				case 'C':
					bCapital = true;
					break;
				case 'x':
					lenFmt = ios_base::hex;
					sLenBase = "0x";
					break;
				case 'n':
					if(argc <= a+1)
						exitArg(argv[0], "Error: missing value after -n");
					scanValue = parseUnsignedIntegerString(argv[++a]);
					if(scanValue < 0)
						exitArg(argv[0], "Error: number of bytes must be a positive integer.");
					wInterDiff = (unsigned long)scanValue;
					break;
				case 'b':
					if(argc <= a+1)
						exitArg(argv[0], "Error: missing value after -b");
					scanValue = parseUnsignedIntegerString(argv[++a]);
					if(scanValue < 1)
						exitArg(argv[0], "Error: block size must be a strictly positive integer.");
					wBlockSize = (unsigned long)scanValue;
					break;
				case 'f':
					if(argc <= a+1)
						exitArg(argv[0], "Error: missing string after -f");
					bDumpDiff = true;
					sPrefixDiff = argv[++a];
					break;
				case 'g':
					if(argc <= a+1)
						exitArg(argv[0], "Error: missing string after -g");
					bDumpOrig = true;
					sPrefixOrig = argv[++a];
					break;
				case 'y':
					if(argc <= a+1)
						exitArg(argv[0], "Error: missing value after -y");
					scanValue = parseUnsignedIntegerString(argv[++a]);
					if(scanValue < 0 || scanValue > 255)
						exitArg(argv[0], "Error: value after -y must be an integer between 0 and 255.");
					bVirtual = true;
					chVirtual = (char)scanValue;
					break;
				default:
					cerr << "Warning: ignoring unknown switch -" << argum[j] << endl;
					break;
				}
			}
		}
		else if(!file1)
			file1 = argv[a];
		else if(!file2)
			file2 = argv[a];
		else
			cerr << "Ignoring redundant argument `" << argv[a] << "'" << endl;
	}
	if(!file1 || (!file2 && !bVirtual))
		exitArg(argv[0], "Error: Not enough arguments.");
	if(bVirtual && file2)
		cerr << "Warning: ignoring second file argument because -y was specified." << endl;

	if(bDumpDiff && bDumpOrig && sPrefixDiff == sPrefixOrig) {
		cerr << "Warning: prefixes for original and different dump files are the same,\n"
		     << " prepending 'in_' to original prefix" << endl;
		sPrefixOrig = "in_"+sPrefixOrig;
	}

	ifstream input1(file1, ios::in | ios::binary);
	if(! input1.good()) {
		cerr << "Error: Could not open input file `" << file1 << "'.\n";
		exit(1);
	}
	ifstream input2;
	if(!bVirtual) {
		input2.open(file2, ios::in | ios::binary);
		if(! input2.good()) {
			cerr << "Error: Could not open input file `" << file2 << "'.\n";
			exit(1);
		}
	}

	if(bCapital) {
		posFmt = posFmt | ios_base::uppercase;
		lenFmt = lenFmt | ios_base::uppercase;
	}

	bool bEof = false;
	long numDiff = -1, // number of differing units up to now. -1 means nothing found yet.
	     lengthRemain = 0;  // length difference of last block before EOF
	unsigned long long offset = 0, // where we are in the file
	                   startDif = 0, // where the last difference started
	                   wTotalDiff = 0;
	ofstream ofDumpFile, ofOrigFile;

	// In case of -n, we need to buffer intermediate matching data because we can't know
	// beforehand if it needs to be written.
	char *bufferDump=NULL, *bufferOrig=NULL;
	if(bDumpDiff)
		bufferDump = new char[wInterDiff*wBlockSize];
	if(bDumpOrig)
		bufferOrig = new char[wInterDiff*wBlockSize];

	// Strictly spoken, a single routine could handle both single-byte and block mode,
	// but single-byte mode can be optimized.
	if(wBlockSize == 1) { // Single-byte mode
		char buffer1[BUFF_SIZE+1],
		     buffer2[BUFF_SIZE+1];
		// Virtual file: our second input buffer is fixed
		if(bVirtual)
			memset(buffer2, chVirtual, BUFF_SIZE);

		while(!bEof) {
			unsigned int bufLen;
			input1.read(buffer1, BUFF_SIZE);
			bEof = input1.eof();
			if(bVirtual) {
				lengthRemain = 0;
				bufLen = input1.gcount();
			}
			else {
				input2.read(buffer2, BUFF_SIZE);
				bEof |= input2.eof();
				lengthRemain = input1.gcount() - input2.gcount();
				bufLen = min(input1.gcount(), input2.gcount());
			}

			buffer1[bufLen] = buffer2[bufLen]; // slight hack for easier termination, see below

			for(unsigned int i = 0; i <= bufLen; i++) {
				if(i == bufLen && !bEof)
					break;
				if(buffer1[i] == buffer2[i]) {
					if(numDiff < 0)
						continue;
					numDiff++;

					if(numDiff > wInterDiff || i == bufLen) {
						// This is where we stop registering a differing string
						unsigned long long difLen = offset+i-startDif-numDiff+1;
						cout.flags(posFmt);
						if(difLen == 1) {
							if(bConcise)
								cout << sPosBase << startDif << " " << sLenBase << "1\n";
							else
								cout << "byte  " << sPosBase << startDif << " differs\n";
							wTotalDiff++;
						}
						else {
							if(bConcise) {
								cout << sPosBase << startDif << " ";
								cout.flags(lenFmt);
								cout << sLenBase << difLen << "\n";
							}
							else {
								cout << "bytes " << sPosBase << startDif << " to "
								     << sPosBase << startDif+difLen-1 << " differ (length = ";
								cout.flags(lenFmt);
								cout << sLenBase << difLen << ")\n";
							}
							wTotalDiff += difLen;
						}
						numDiff = -1;
						if(bDumpDiff)
							ofDumpFile.close();
						if(bDumpOrig)
							ofOrigFile.close();
						if(bStop) {
							bEof = true;
							break;
						}
					}
					else {
						if(bDumpDiff)
							bufferDump[numDiff-1] = buffer2[i];
						if(bDumpOrig)
							bufferOrig[numDiff-1] = buffer1[i];
					}
				}
				else if(buffer1[i] != buffer2[i]) { // slight hack: cannot happen at i == bufLen
					if(numDiff < 0) {
						// This is where we start registering a differing string
						startDif = offset+i;
						if(bDumpDiff) {
							ostringstream osFileName;
							osFileName.flags(posFmt);
							osFileName << sPrefixDiff << sPosBase << startDif;
							ofDumpFile.open(osFileName.str().c_str());
						}
						if(bDumpOrig) {
							ostringstream osFileName;
							osFileName.flags(posFmt);
							osFileName << sPrefixOrig << sPosBase << startDif;
							ofOrigFile.open(osFileName.str().c_str());
						}
					}
					if(bDumpDiff) {
						if(numDiff > 0)
							ofDumpFile.write(bufferDump, numDiff);
						ofDumpFile.put(buffer2[i]);
					}
					if(bDumpOrig) {
						if(numDiff > 0)
							ofOrigFile.write(bufferOrig, numDiff);
						ofOrigFile.put(buffer1[i]);
					}
					numDiff = 0;
				}
			}
			offset += bufLen;
		}
	}
	else { // Block mode
		char *buffer1 = new char[wBlockSize],
		     *buffer2 = new char[wBlockSize];
		unsigned long wBlockNum = 0;

		// Virtual file: our second input buffer is fixed
		if(bVirtual)
			memset(buffer2, chVirtual, wBlockSize);

		while(!bEof) {
			unsigned int bufLen;
			input1.read(buffer1, wBlockSize);
			bEof = input1.eof();
			if(bVirtual) {
				lengthRemain = 0;
				bufLen = input1.gcount();
			}
			else {
				input2.read(buffer2, wBlockSize);
				bEof |= input2.eof();
				lengthRemain = input1.gcount() - input2.gcount();
				bufLen = min(input1.gcount(), input2.gcount());
			}

			bool bBlocksMatch = (memcmp(buffer1, buffer2, bufLen) == 0);

			// Always test for this, instead of using an if/else on bBlocksMatch. Otherwise we would miss a differing final block.
			if(!bBlocksMatch) {
				if(numDiff < 0) {
					// This is where we start registering a differing block
					startDif = wBlockNum;
					if(bDumpDiff) {
						ostringstream osFileName;
						osFileName.flags(posFmt);
						osFileName << sPrefixDiff << sPosBase << startDif*wBlockSize;
						ofDumpFile.open(osFileName.str().c_str());
					}
					if(bDumpOrig) {
						ostringstream osFileName;
						osFileName.flags(posFmt);
						osFileName << sPrefixOrig << sPosBase << startDif*wBlockSize;
						ofOrigFile.open(osFileName.str().c_str());
					}
				}
				if(bDumpDiff) {
					if(numDiff > 0)
						ofDumpFile.write(bufferDump, numDiff*wBlockSize);
					ofDumpFile.write(buffer2, bufLen);
				}
				if(bDumpOrig) {
					if(numDiff > 0)
						ofDumpFile.write(bufferOrig, numDiff*wBlockSize);
					ofOrigFile.write(buffer1, bufLen);
				}
				numDiff = 0;
			}
			
			if(bBlocksMatch || bEof) {
				if(numDiff < 0) {
					wBlockNum++;
					if(!bEof || bBlocksMatch)
						continue;
				}
				numDiff++;
				if(numDiff > wInterDiff || bEof) {
					// This is where we stop registering a differing block
					unsigned long long difLen = wBlockNum - startDif - numDiff + 1;
					unsigned int wLastBlockLength = wBlockSize;
					// If we're at the EOF and blocks differ, they might be truncated as well, so ensure correct reporting of their length
					if(!bBlocksMatch)
						wLastBlockLength = bufLen;

					cout.flags(posFmt);
					// difLen can be 0 if the final block differs
					if(difLen <= 1) {
						if(bConcise) {
							cout << sPosBase << startDif*wBlockSize << " ";
							cout.flags(lenFmt);
							cout << sLenBase << wLastBlockLength << "\n";
						}
						else
							cout << "block  " << sPosBase << startDif << " (starting at "
							     << sPosBase << startDif*wBlockSize << ") differs\n";
					}
					else {
						if(bConcise) {
							cout << sPosBase << startDif*wBlockSize << " ";
							cout.flags(lenFmt);
							cout << sLenBase << ((difLen - 1)*wBlockSize + wLastBlockLength) << "\n";
						}
						else
							cout << "blocks " << sPosBase << startDif << " to "
							     << sPosBase << startDif+difLen-1 << " (starting at "
							     << sPosBase << startDif*wBlockSize << ") differ\n";
					}

					wTotalDiff += difLen*wBlockSize;
					if(bEof && !bBlocksMatch)
						wTotalDiff += bufLen;
					numDiff = -1;
					if(bDumpDiff)
						ofDumpFile.close();
					if(bDumpOrig)
						ofOrigFile.close();
					if(bStop)
						bEof = true;
				}
				else {
					if(bDumpDiff)
						memcpy(bufferDump+(numDiff-1)*wBlockSize, buffer2, bufLen);
					if(bDumpDiff)
						memcpy(bufferOrig+(numDiff-1)*wBlockSize, buffer1, bufLen);
				}
			}

			wBlockNum++;
		}

		delete [] buffer1;
		delete [] buffer2;
	}

	if(bDumpDiff)
		delete [] bufferDump;
	if(bDumpOrig)
		delete [] bufferOrig;

	bool difFound = (wTotalDiff > 0);

	// Don't check length difference if bVirtual because the virtual file is by definition always the same length.
	// Also don't report length difference if bStop and we already found a difference, because that would be a second difference.
	if(!bVirtual && !(bStop && difFound)) {
		if(lengthRemain < 0 || (input1.eof() && ! input2.eof())) {
			difFound = true;
			cout.flags(lenFmt);
			cout << "File 2 is " << sLenBase << countRemaining(input2) - lengthRemain << " bytes longer than file 1.\n";
		}
		else if(lengthRemain > 0 || (input2.eof() && ! input1.eof())) {
			difFound = true;
			cout.flags(lenFmt);
			cout << "File 1 is " << sLenBase << countRemaining(input1) + lengthRemain << " bytes longer than file 2.\n";
		}
	}

	if(!difFound)
		cout << "No differences found!\n";
	else if(bTotal) {
		cout.flags(lenFmt);
		cout << "Total size of differing blocks: " << sLenBase << wTotalDiff << endl;
	}

	return 0;
}
