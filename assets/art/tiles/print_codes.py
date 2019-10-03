#!/usr/bin/env python3

# generate lookup table of index=>tile codes
lookup = {}
index = 0
for i in range(0, 255):
	T = L = R = B = TL = TR = BL = BR = False
	code = ""
	if i & 1:
		TL = True
	if i & 4:
		TR = True
	if i & 32:
		BL = True
	if i & 128:
		BR = True

	if i & 2:
		code += " T"
		T = True
		TL = False
		TR = False
	if i & 8:
		code += " L"
		L = True
		TL = False
		BL = False
	if i & 16:
		code += " R"
		R = True
		TR = False
		BR = False
	if i & 64:
		code += " B"
		B = True
		BL = False
		BR = False

	if TL:
		code += " TL"
	if TR:
		code += " TR"
	if BL:
		code += " BL"
	if BR:
		code += " BR"

	bits = (
		TL * 1 +
		T * 2 +
		TR * 4 +
		L * 8 +
		R * 16 +
		BL * 32 +
		B * 64 +
		BR * 128
	)

	if bits not in lookup:
		lookup[bits] = code.strip()
		index += 1

# print codes
i = 0
for index in lookup:
	print(str(i) + ":" + lookup[index])
	i += 1

# print commands
i = 0
for index in lookup:
	command = "convert "
	code = lookup[index].lower()
	tokens = code.split(" ")
	if len(tokens) <= 1:
		if i == 0:
			command += "$out/t.png -fill transparent -draw 'color 0,0 reset' $out/00.png"
		else:
			command = "cp $out/" + tokens[0] + ".png $out/" + str(i).zfill(2) + ".png"
		print(command)
		i += 1
		continue

	for token in tokens:
		command += "$out/" + token + ".png "

	command += "-evaluate-sequence max $out/" + str(i).zfill(2) + ".png"
	print(command)
	i += 1
