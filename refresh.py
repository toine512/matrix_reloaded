
lsbMsbTransitionBit = 0
while True:
#	psPerClock = 1000000000000 / 22857142
#	psPerClock = 1000000000000 / 18677760
	psPerClock = 1000000000000 / (240000000/(7+9/50)/2)
	nsPerLatch = 256 * psPerClock / 1000
	nsPerRow = 8 * nsPerLatch

	for i in range(lsbMsbTransitionBit+1, 8):
		nsPerRow = nsPerRow + ((1 << (i - lsbMsbTransitionBit - 1)) * (8 - i) * nsPerLatch)

	nsPerFrame = nsPerRow * 32
	actualRefreshRate = 1000000000 / nsPerFrame

	print(f"lsbMsbTransitionBit of {lsbMsbTransitionBit} gives {actualRefreshRate} Hz refresh rate.")

	if (lsbMsbTransitionBit < 7):
		lsbMsbTransitionBit = lsbMsbTransitionBit + 1
	else:
		break

# 9+ 28/49
#print(240000000/(9+31/54)) #25067520
#print(240000000/(14+13/36)) #16711680
#print(240000000/(12+45/53)) #18677760

#60 Hz 14+13/21
#90 Hz 9+31/41

#90 Hz N=4 b=48 a=62 lvl=100 (3.377 mA)
#60 Hz N=7 b=9 a=50 lvl=101