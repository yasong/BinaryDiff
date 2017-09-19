#!/usr/bin/python
# BinDiff by Alexander Thomas (doctor.lex@gmail.com)
# Released under the New BSD License (see LICENSE file for details).

import re
import subprocess
from difflib import unified_diff

binary = "../bindiff"

def run_test(test_num, test):
	test_name = test[0]
	file1 = test[1]
	file2 = test[2]
	args = test[3]
	expected = test[4]
	expected_failure = False
	if len(test) > 5:
		expected_failure = test[5]

	command_line = [binary]
	if args:
		command_line += args
	if file2:
		command_line += [file1, file2]
	else:
		command_line += [file1]

	result = "OK"
	failure = False
	try:
		output = subprocess.check_output(command_line, stderr=subprocess.STDOUT)
	except subprocess.CalledProcessError as err:
		failure = True
		if not expected_failure:
			result = "FAIL\nProgram exited abnormally: {}".format(err)
		output = err.output

	if expected_failure and not failure:
		result = "FAIL\nProgram was supposed to fail, but did not"
	elif not failure or (failure and expected_failure):
		if failure:
			# Strip the "Usage" at the end of error output
			output = re.sub(r"\nUsage: .*$", "", output, flags=re.DOTALL)
		if expected.strip() != output.strip():
			result = "FAIL\n====================\n"
			result += " ".join(command_line) + "\nOutput differs:\n"
			diff_report = unified_diff(expected.strip().splitlines(), output.strip().splitlines(), "expected", "actual", lineterm='')
			result += "\n".join(diff_report)
			result += "\n===================="

	print "{} - {}: {}".format(test_num, test_name, result)


#########################################
# Not every possible code path is tested. That is pretty hopeless anyway unless the program would be refactored to have fewer chunks of similar code.
tests = []

# Expected failures
tests.append(["Bad arguments", "b0rk", None, None, "Error: Not enough arguments.", True])
tests.append(["Bad blocksize", "b0rk", "b0rk", ["-b", "0"], "Error: block size must be a strictly positive integer.", True])
tests.append(["Bad chunksize", "b0rk", "b0rk", ["-n", "-32"], "Error: number of bytes must be a positive integer.", True])
tests.append(["Bad byte", "b0rk", "b0rk", ["-y", "0xbadc0ffe"], "Error: value after -y must be an integer between 0 and 255.", True])

# Normal tests
tests.append(["Identical", "block2-16a.dat", "block2-16a.dat", None, "No differences found!"])
tests.append(["Plain test", "simple1.txt", "simple2.txt", None, """byte  0xb differs
bytes 0x16 to 0x19 differ (length = 4)
byte  0x29 differs
byte  0x2b differs
File 2 is 10 bytes longer than file 1."""])
tests.append(["Plain concise", "simple1.txt", "simple2.txt", ["-c"], """0xb 1
0x16 4
0x29 1
0x2b 1
File 2 is 10 bytes longer than file 1."""])
tests.append(["Plain concise capital hexLength", "simple1.txt", "simple2.txt", ["-cC", "-x"], """0xB 0x1
0x16 0x4
0x29 0x1
0x2B 0x1
File 2 is 0xA bytes longer than file 1."""])
tests.append(["Plain stop decimal", "simple1.txt", "simple2.txt", ["-sd"], "byte  11 differs"])

tests.append(["Difference at end", "testEndDiff1.txt", "testEndDiff2.txt", None, "byte  0xf differs"])

tests.append(["Longer 1", "block2-17a.dat", "block2-16a.dat", None, "File 1 is 1 bytes longer than file 2."])
tests.append(["Longer 2", "block2-16a.dat", "block2-17a.dat", None, "File 2 is 1 bytes longer than file 1."])

# Test chunks (skip bytes)
tests.append(["Skip 1 byte concise", "simple1.txt", "simple2.txt", ["-cn", "1"], """0xb 1
0x16 4
0x29 3
File 2 is 10 bytes longer than file 1."""])
tests.append(["Skip 2 bytes concise", "simple1.txt", "simple2.txt", ["-cn", "2"], """0xb 1
0x16 4
0x29 3
File 2 is 10 bytes longer than file 1."""])
# Test border case: only from 10 bytes on the test must produce a different result
tests.append(["Skip 9 bytes concise", "simple1.txt", "simple2.txt", ["-cn", "9"], """0xb 1
0x16 4
0x29 3
File 2 is 10 bytes longer than file 1."""])
tests.append(["Skip 10 bytes concise", "simple1.txt", "simple2.txt", ["-cn", "10"], """0xb 15
0x29 3
File 2 is 10 bytes longer than file 1."""])

# Test virtual files
tests.append(["Virtual zeros", "block2-17a.dat", None, ["-cy", "0x0"], """0x2 2
0x8 2
0xc 4"""])
tests.append(["Virtual 0x11", "block2-17a.dat", None, ["-cy", "0x11"], """0x0 2
0x4 4
0xa 2
0x10 1"""])

# Test differences at the edge of buffer length
tests.append(["Buffer edge longer", "16KiBzeroes.dat", "16KiBzeroesAndOne1.dat", None, "File 2 is 1 bytes longer than file 1."])
tests.append(["Buffer diff + edge longer", "16KiBzeroes.dat", "16KiBzeroesAndOne1AndAnotherDifference.dat", None, """byte  0x1f27 differs
File 2 is 1 bytes longer than file 1."""])

# Test block comparison mode
tests.append(["Block size 2", "block2-16a.dat", "block2-16b.dat", ["-b", "2"], """block  0x1 (starting at 0x2) differs
block  0x4 (starting at 0x8) differs
blocks 0x6 to 0x7 (starting at 0xc) differ"""])
tests.append(["Block size 2 stop decimal", "block2-16a.dat", "block2-16b.dat", ["-sdb", "2"], "block  1 (starting at 2) differs"])
tests.append(["Block size 2, longer concise", "block2-16a.dat", "block2-17b.dat", ["-cb", "2"], """0x2 2
0x8 2
0xc 4
File 2 is 1 bytes longer than file 1."""])
tests.append(["Block size 2, skip 1, longer", "block2-16a.dat", "block2-17b.dat", ["-bn", "2", "1"], """block  0x1 (starting at 0x2) differs
blocks 0x4 to 0x7 (starting at 0x8) differ
File 2 is 1 bytes longer than file 1."""])
tests.append(["Block size 2, skip 2", "block2-16a.dat", "block2-16b.dat", ["-bn", "2", "2"], "blocks 0x1 to 0x7 (starting at 0x2) differ"])
tests.append(["Block size 512, longer", "16KiBzeroes.dat", "16KiBzeroesAndOne1AndAnotherDifference.dat", ["-b", "512"], """block  0xf (starting at 0x1e00) differs
File 2 is 1 bytes longer than file 1."""])
tests.append(["Block size 512, concise virtual", "16KiBzeroesAndOne1AndAnotherDifference.dat", None, ["-cby", "512", "0x0"], """0x1e00 512
0x4000 1"""])


test_num = 1
for test in tests:
	run_test(test_num, test)
	test_num += 1
