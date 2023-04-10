import io,struct,os,sys,zlib,math

##
# @package f2profile
# Fiesta 2 profile tools
#

#-------------------------------------------------------------------------------------------
##
# Save and rank file related constants like indexes
#

CONST_PROFILE_ENC_DEC_INIT_SEED = 		0xEBADA1
CONST_SAVE_FILE_MACHINE_TABLE_INDEX = 	0x2F0
CONST_SAVE_FILE_PERSONAL_TABLE_INDEX = 	0x25970

#-------------------------------------------------------------------------------------------
##
# Structure holding the profile header of a F2 save file
#
class F2SaveStructProfileHeader:
	def __init__(self):							# offset	length		datatype
		self.adler32 = 0						# 0x00		0x04		LE int
		self.adlerSeed = 0						# 0x04		0x04		LE int
		self.playerID = ""						# 0x08		0x08		string (null term)
		self.region = 0							# 0x14		0x04		LE int
		self.avatarID = 999						# 0x18		0x04		LE int
		self.level = 1 							# 0x1C		0x04		LE int
		self.totalCalories = 0.0 				# 0x20		0x04		LE float
		self.totalV02 = 0.0						# 0x24		0x04		LE float
		self.numberRunningSteps = 0				# 0x28   	0x08		LE int64
		self.numberGamesPlayed = 0				# 0x30		0x08		LE int64
		self.exp = 0							# 0x38		0x08		LE int64
		self.arcadeScore = 0					# 0x40		0x08		LE int64
		self.numberMissionsCompleted = 0		# 0x48		0x04		LE int
		self.numberCoopMissionsCompleted = 0	# 0x4C		0x04		LE int
		self.battleCountWins = 0				# 0x50		0x04		LE int
		self.battleCountLoses = 0				# 0x54		0x04		LE int
		self.battleCountDraws = 0				# 0x58		0x04		LE int

		# TODO: modifier stuff
		self.unlock = 0                         # 0x6C      0x08        LE int64
		self.unlock_seed = 0                    # 0x78      0x04        LE int

##
# Structure with machine info of a F2 save file
#
class F2SaveStructMachineInfo:
	def __init__(self):									# offset	length		datatype
		self.versionString = ""							# 0x120		0x08		string (null term)
		self.cpuString = ""								# 0x128		0x80		string (null term)
		self.motherboardString = ""						# 0x1A8		0x80		string (null term)
		self.gfxcardString = ""							# 0x228		0x80		string (null term)
		self.hddString = ""								# 0x2A8		0x20		string (null term)
		self.totalRamBytes = 0							# 0x2C8		0x04		LE int
		self.haspkeyID = 0								# 0x2CC		0x04		LE int
		self.machineStatsBeginnerModePlayCount = 0		# 0x2D0		0x04		LE int
		self.machineStatsManiacModePlayCount = 0		# 0x2D4		0x04		LE int
		self.machineStatsUnknown = 0					# 0x2D8		0x04		LE int
		self.machineStatsUnknown2 = 0					# 0x2DC		0x04		LE int
		self.machineStatsBattleModePlayCount = 0		# 0x2E0		0x04		LE int
		self.machineStatsMissionZonePlayCount = 0		# 0x2E4		0x04		LE int
		self.machineStatsSkillZonePlayCount = 0			# 0x2E8		0x04		LE int
		self.machineStatsCoopMissionPlayCount = 0		# 0x2EC		0x04		LE int


#-------------------------------------------------------------------------------------------
##
# Structure of a single machine highscore table entry of the F2 save file
#
class F2SaveStructMachineScoreTableEntry:
	def __init__(self):									# offset	length		datatype
		self.songID = 0x00								# 0x00		0x04		LE hex
		self.difficulty = 0								# 0x04		0x01		byte
		self.mode = 0									# 0x05		0x01		byte
		self.grade = 0x00								# 0x06		0x01		byte
		self.clearState = 0								# 0x07		0x01		byte
		self.scoreTotal = 0								# 0x08		0x04		LE int
		self.playCountTotal = 0							# 0x0C		0x04		LE int
		self.unknown = 0								# 0x10		0x04		LE int
		self.playerID = ""								# 0x14		0x08		string (null term)
		self.unknown2 = 0								# 0x1C		0x04		LE int
		self.nullPadding = 0							# 0x20		0x04		LE int


#-------------------------------------------------------------------------------------------
##
# Structure of a single personal score table entry of the F2 save file
#
class F2SaveStructPersonalScoreTableEntry:
	def __init__(self):									# offset	length		datatype
		self.songID = 0x00								# 0x00		0x04		LE hex
		self.difficulty = 0								# 0x04		0x01		byte
		self.mode = 0									# 0x05		0x01		byte
		self.grade = 0x00								# 0x06		0x01		byte
		self.clearAndUnlockedState = 0					# 0x07		0x01		byte
		self.scoreTotal = 0								# 0x08		0x04		LE int
		self.playCountTotal = 0							# 0x0C		0x04		LE int
		self.nullPadding = 0							# 0x10		0x04		LE int
		

#-------------------------------------------------------------------------------------------
##
# Structure representing the contents of a F2 save file
#
class F2SaveStruct:	
	def __init__(self):						
		self.profileHeader = F2SaveStructProfileHeader()			# startoffset: 0x00
		self.machineInfo = F2SaveStructMachineInfo()				# startoffset: 0x120
		self.machineScoreTable = list()								# startoffset: 0x2F0
		self.personalScoreTable = list()							# startoffset: 0x25970


#-------------------------------------------------------------------------------------------
## 
# Encrypt f2 profiles
# 
# @param savedata Buffer filled with decrypted savedata
# @param rankdata Buffer filled with decrypted rankdata
# @return Buffer with encrypted savedata
# @return Buffer with encrypted rankdata
#
def encryptProfile(savedata,rankdata):
	adlerseed = struct.unpack("<I",savedata[4:8])
	newsaveadler = zlib.adler32(savedata[4:],adlerseed[0])
	newrankadler = zlib.adler32(rankdata[4:],adlerseed[0])
	savedata[0:4] = struct.pack("<I",newsaveadler)
	rankdata[0:4] =  struct.pack("<I",newrankadler)
	
	
	seed = CONST_PROFILE_ENC_DEC_INIT_SEED
	for i in range(0,len(savedata)):
		smbuff = savedata[i] ^ (seed >> 8) & 0xFF
		savedata[i] = smbuff
		seed = (0x68993 * (smbuff + seed) + 0x4FDCF) & 0xFFFFFFFF
	
	seed = CONST_PROFILE_ENC_DEC_INIT_SEED	
	
	for i in range(0,len(rankdata)):
		smbuff = rankdata[i] ^ (seed >> 8) & 0xFF
		rankdata[i] = smbuff
		seed = (0x68993 * (smbuff + seed) + 0x4FDCF) & 0xFFFFFFFF	
	
	return savedata,rankdata
	

#-------------------------------------------------------------------------------------------
## 
# Decrypt f2 profiles
# 
# @param savedata Buffer filled with encrypted savedata
# @param rankdata Buffer filled with encrypted rankdata
# @return Buffer with decrypted savedata
# @return Buffer with decrypted rankdata
#
def decryptProfile(savedata,rankdata):
	seed = CONST_PROFILE_ENC_DEC_INIT_SEED
	for i in range(0,len(savedata)):
		smbuff = savedata[i]
		savedata[i] ^= (seed >> 8) & 0xFF
		seed = (0x68993 * (smbuff + seed) + 0x4FDCF) & 0xFFFFFFFF
	
	seed = CONST_PROFILE_ENC_DEC_INIT_SEED	
	for i in range(0,len(rankdata)):
		smbuff = rankdata[i]
		rankdata[i] ^= (seed >> 8) & 0xFF
		seed = (0x68993 * (smbuff + seed) + 0x4FDCF) & 0xFFFFFFFF
	
	return savedata,rankdata


#-------------------------------------------------------------------------------------------
## 
# Dump the contents of a buffered F2 save file to a structure
# 
# @param savedataBuffer Decrypted F2 save data in a buffer
# @return F2SaveStruct filled with data
#	
def dumpSaveStruct(savedataBuffer):
	structure = F2SaveStruct()

	# profile header
	structure.profileHeader.adler32 = 						int.from_bytes(savedataBuffer[0x00:0x04], "little")
	structure.profileHeader.adlerSeed = 					int.from_bytes(savedataBuffer[0x04:0x08], "little")
	structure.profileHeader.playerID = 					savedataBuffer[0x08:0x10].split(b'\x00')[0].decode("utf-8")
	structure.profileHeader.region = 						int.from_bytes(savedataBuffer[0x14:0x18], "little")
	structure.profileHeader.avatarID = 					int.from_bytes(savedataBuffer[0x18:0x1C], "little")
	structure.profileHeader.level = 						int.from_bytes(savedataBuffer[0x1C:0x20], "little")
	structure.profileHeader.totalCalories = 				struct.unpack('f', savedataBuffer[0x20:0x24])
	structure.profileHeader.totalV02 = 					struct.unpack('f', savedataBuffer[0x24:0x28])
	structure.profileHeader.numberRunningSteps = 			int.from_bytes(savedataBuffer[0x28:0x30], "little")
	structure.profileHeader.numberGamesPlayed = 			int.from_bytes(savedataBuffer[0x30:0x38], "little")
	structure.profileHeader.exp = 							int.from_bytes(savedataBuffer[0x38:0x40], "little")
	structure.profileHeader.arcadeScore = 					int.from_bytes(savedataBuffer[0x40:0x48], "little")
	structure.profileHeader.numberMissionsCompleted = 		int.from_bytes(savedataBuffer[0x48:0x4C], "little")
	structure.profileHeader.numberCoopMissionsCompleted = 	int.from_bytes(savedataBuffer[0x4C:0x50], "little")
	structure.profileHeader.battleCountWins = 				int.from_bytes(savedataBuffer[0x50:0x54], "little")
	structure.profileHeader.battleCountLoses = 			int.from_bytes(savedataBuffer[0x54:0x58], "little")
	structure.profileHeader.battleCountDraws = 			int.from_bytes(savedataBuffer[0x58:0x5C], "little")
	
	# TODO: remaining stuff from profile header
	structure.profileHeader.unlock_seed = int.from_bytes(savedataBuffer[0x78:0x7C], "little")
	if(structure.profileHeader.unlock_seed == -1):
		structure.profileHeader.unlock_seed = 0xEBADA1
	# machine data	
	structure.machineInfo.versionString = 						savedataBuffer[0x120:0x128].split(b'\x00')[0].decode("utf-8")
	structure.machineInfo.cpuString = 							savedataBuffer[0x128:0x1A8].split(b'\x00')[0].decode("utf-8")
	structure.machineInfo.motherboardString = 					savedataBuffer[0x1A8:0x228].split(b'\x00')[0].decode("utf-8")
	structure.machineInfo.gfxcardString = 						savedataBuffer[0x228:0x2A8].split(b'\x00')[0].decode("utf-8")
	structure.machineInfo.hddString = 							savedataBuffer[0x2A8:0x2C8].split(b'\x00')[0].decode("utf-8")
	structure.machineInfo.totalRamBytes = 						savedataBuffer[0x2C8:0x2CC].split(b'\x00')[0].decode("utf-8")
	structure.machineInfo.haspkeyID = 							int.from_bytes(savedataBuffer[0x2CC:0x2D0], "little")

	structure.machineInfo.machineStatsBeginnerModePlayCount = 	int.from_bytes(savedataBuffer[0x2D0:0x2D4], "little")
	structure.machineInfo.machineStatsManiacModePlayCount = 	int.from_bytes(savedataBuffer[0x2D4:0x2D8], "little")
	structure.machineInfo.machineStatsUnknown = 				int.from_bytes(savedataBuffer[0x2D8:0x2DC], "little")
	structure.machineInfo.machineStatsUnknown2 = 				int.from_bytes(savedataBuffer[0x2DC:0x2E0], "little")
	structure.machineInfo.machineStatsBattleModePlayCount = 	int.from_bytes(savedataBuffer[0x2E0:0x2E4], "little")
	structure.machineInfo.machineStatsMissionZonePlayCount = 	int.from_bytes(savedataBuffer[0x2E4:0x2E8], "little")
	structure.machineInfo.machineStatsSkillZonePlayCount = 	int.from_bytes(savedataBuffer[0x2E8:0x2EC], "little")
	structure.machineInfo.machineStatsCoopMissionPlayCount = 	int.from_bytes(savedataBuffer[0x2EC:0x2F0], "little")


	# read machine highscore table
	# start index	
	index = CONST_SAVE_FILE_MACHINE_TABLE_INDEX
	while True:
		# read until we get a null pattern, a bit hacky to detect the end of the table, 
		# but there is no length or size of the table anywhere
		if int.from_bytes(savedataBuffer[index + 0:index + 8], "little") == 0:
			break

		entry = F2SaveStructMachineScoreTableEntry()

		entry.songID = int.from_bytes(savedataBuffer[index + 0x00:index + 0x04], "little") 
		entry.difficulty = int.from_bytes(savedataBuffer[index + 0x04:index + 0x05], "little")
		entry.mode = int.from_bytes(savedataBuffer[index + 0x05:index + 0x06], "little")
		entry.grade = int.from_bytes(savedataBuffer[index + 0x06:index + 0x07], "little")
		entry.clearState = int.from_bytes(savedataBuffer[index + 0x07:index + 0x08], "little")
		entry.scoreTotal = int.from_bytes(savedataBuffer[index + 0x08:index + 0x0C], "little")
		entry.playCountTotal = int.from_bytes(savedataBuffer[index + 0x0C:index + 0x10], "little")
		entry.unknown = int.from_bytes(savedataBuffer[index + 0x10:index + 0x14], "little")
		entry.playerID = savedataBuffer[index + 0x14:index + 0x1C].split(b'\x00')[0].decode("utf-8")
		entry.unknown2 = int.from_bytes(savedataBuffer[index + 0x1C:index + 0x20], "little")
		entry.nullPadding = int.from_bytes(savedataBuffer[index + 0x20:index + 0x24], "little")

		# add entry and update index
		structure.machineScoreTable.append(entry)
		index += 0x24


	# read personal highscore table
	# start index
	index = CONST_SAVE_FILE_PERSONAL_TABLE_INDEX
	while True:
		# read until we get a null pattern, a bit hacky to detect the end of the table, 
		# but there is no length or size of the table anywhere
		if int.from_bytes(savedataBuffer[index + 0:index + 8], "little") == 0:
			break
	
		entry = F2SaveStructPersonalScoreTableEntry()

		entry.songID = int.from_bytes(savedataBuffer[index + 0x00:index + 0x04], "little") 
		entry.difficulty = int.from_bytes(savedataBuffer[index + 0x04:index + 0x05], "little")
		entry.mode = int.from_bytes(savedataBuffer[index + 0x05:index + 0x06], "little")
		entry.grade = int.from_bytes(savedataBuffer[index + 0x06:index + 0x07], "little")
		entry.clearAndUnlockState = int.from_bytes(savedataBuffer[index + 0x07:index + 0x08], "little")
		entry.scoreTotal = int.from_bytes(savedataBuffer[index + 0x08:index + 0x0C], "little")
		entry.playCountTotal = int.from_bytes(savedataBuffer[index + 0x0C:index + 0x10], "little")
		entry.nullPadding = int.from_bytes(savedataBuffer[index + 0x10:index + 0x14], "little")

		# add entry and update index
		structure.personalScoreTable.append(entry)
		index += 0x14
	

	return structure


#-------------------------------------------------------------------------------------------
## 
# Unlock all songs and stepcharts of a decrypted f2 save file
# 
def unlockAllSongsStepcharts(savefiledec):
	fp = open(savefiledec,"rb+")
	savedata = bytearray(fp.read())
	unlock(savedata, True, False, False, False)
	fp.write(savedata)
	fp.close()
	
	return 0


#-------------------------------------------------------------------------------------------
## 
# Unlock several things of the F2 save file
# 
# @param savebuf Buffer containing a decrypted F2 save file
# @param stepcharts True to unlock all stepcharts/songs
# @param modifier True to unlock all modifers/items
# @param missionsClear True to mark all missions cleared
# @param coopMissionsClear True to mark all coop missions cleared
#
def unlock(savebuf, stepcharts, modifier, missionsClear, coopMissionsClear):
	# go to personal highscore table index	
	
	if (stepcharts):
		index = CONST_SAVE_FILE_PERSONAL_TABLE_INDEX
		while True:
			# read until we get a null pattern, a bit hacky to detect the end of the table, 
			# but there is no length or size of the table anywhere
			if int.from_bytes(savebuf[index + 0:index + 8], "little") == 0:
				break
	
			# unlock data at index 0x07, one byte which also marks the clear state
			savebuf[index + 0x07:index + 0x08] |= 0x40

			# update index
			index += 0x14
	if (modifier):
		# TODO
		TODO = True
	if (missionsClear):
		# TODO
		TODO = True
	if (coopMissionsClear):
		# TODO
		TODO = True
	

	return 0


#-------------------------------------------------------------------------------------------
## 
# Unlock specified contents of a F2 save file
#
# @param what What to unlock: stepcharts, items, allMissionsClear, allCoopMissionsClear
# @param infilesavedec Decrypted F2 save file as input (gets modified)
# 
def unlockCmd(what, infilesavedec):
	if (what == "stepcharts"):
		unlockAllSongsStepcharts(infilesavedec)
	elif (what == "items"):
		# TODO
		TODO = True
	elif (what == "allMissionsClear"):
		# TODO
		TODO = True
	elif (what == "allCoopMissionsClear"):
		# TODO
		TODO = True
	else:
		print("Wrong argument specified: " + what)
		print("Nothing to unlock")
		return 1

	return 0


#-------------------------------------------------------------------------------------------
## 
# Dumps the contents of a F2 save file to a txt (formated)
# 
# @param infilesavedec Decrypted save file as input
# @param outfile File to write the output to (txt)
#
def dumpSaveToTxtCmd(infilesavedec, outfile):
	insave = open(infilesavedec,"rb")
	insavedata = bytearray(insave.read())
	insave.close()

	structure = dumpSaveStruct(insavedata)

	outsave = open(outfile, "wb")
	
	# write profile header data
	outsave.write((">>>>> Profile header data <<<<<\n").encode('utf-8'))
	outsave.write(("Adler32: 0x%X\n" % 							structure.profileHeader.adler32).encode('utf-8'))
	outsave.write(("Adler seed: 0x%X\n" % 						structure.profileHeader.adlerSeed).encode('utf-8'))
	outsave.write(("Player ID: " + 								structure.profileHeader.playerID + "\n").encode('utf-8'))
	outsave.write(("Region ID: 0x%X\n" % 						structure.profileHeader.region).encode('utf-8'))	
	outsave.write(("Avatar ID: 0x%X\n" % 						structure.profileHeader.avatarID).encode('utf-8'))
	outsave.write(("Level: %d\n" % 								structure.profileHeader.level).encode('utf-8'))
	outsave.write(("Total Calories: %f\n" % 					structure.profileHeader.totalCalories).encode('utf-8'))
	outsave.write(("Total V02: %f\n" % 							structure.profileHeader.totalV02).encode('utf-8'))
	outsave.write(("Number of running steps: %d\n" % 			structure.profileHeader.numberRunningSteps).encode('utf-8'))
	outsave.write(("Number of games played: %d\n" % 			structure.profileHeader.numberGamesPlayed).encode('utf-8'))
	outsave.write(("EXP: %d\n" % 								structure.profileHeader.exp).encode('utf-8'))
	outsave.write(("Arcade Score: %d\n" % 						structure.profileHeader.arcadeScore).encode('utf-8'))
	outsave.write(("Number of missions completed: %d\n" % 		structure.profileHeader.numberMissionsCompleted).encode('utf-8'))
	outsave.write(("Number of coop missions completed: %d\n" %	structure.profileHeader.numberCoopMissionsCompleted).encode('utf-8'))
	outsave.write(("Number of Battle mode wins: %d\n" %			structure.profileHeader.battleCountWins).encode('utf-8'))
	outsave.write(("Number of Battle mode loses: %d\n" %		structure.profileHeader.battleCountLoses).encode('utf-8'))
	outsave.write(("Number of Battle mode draws: %d\n" %		structure.profileHeader.battleCountDraws).encode('utf-8'))

	# TODO: remaining stuff from profile header

	outsave.write(("\n").encode('utf-8'))

	# write machine data
	outsave.write((">>>>> Machine data <<<<<\n").encode('utf-8'))
	outsave.write(("Version string: %s\n" % 			structure.machineInfo.versionString).encode('utf-8'))
	outsave.write(("CPU string: %s\n" % 				structure.machineInfo.cpuString).encode('utf-8'))
	outsave.write(("Motherboard string: %s\n" % 		structure.machineInfo.motherboardString).encode('utf-8'))
	outsave.write(("GFX card string: %s\n" % 			structure.machineInfo.gfxcardString).encode('utf-8'))
	outsave.write(("HDD string: %s\n" % 				structure.machineInfo.hddString).encode('utf-8'))
	outsave.write(("HASP key ID: 0x%X\n" % 				structure.machineInfo.haspkeyID).encode('utf-8'))

	outsave.write(("Beginner mode play count: %d\n" %	structure.machineInfo.machineStatsBeginnerModePlayCount).encode('utf-8'))
	outsave.write(("Maniac mode play count: %d\n" %		structure.machineInfo.machineStatsManiacModePlayCount).encode('utf-8'))
	outsave.write(("Unknown count: %d\n" %				structure.machineInfo.machineStatsUnknown).encode('utf-8'))
	outsave.write(("Unknown 2 count: %d\n" %			structure.machineInfo.machineStatsUnknown2).encode('utf-8'))
	outsave.write(("Battle mode play count: %d\n" %		structure.machineInfo.machineStatsBattleModePlayCount).encode('utf-8'))
	outsave.write(("Mission zone play count: %d\n" %	structure.machineInfo.machineStatsMissionZonePlayCount).encode('utf-8'))
	outsave.write(("Skill up zone play count: %d\n" %	structure.machineInfo.machineStatsSkillZonePlayCount).encode('utf-8'))
	outsave.write(("Coop mission play count: %d\n" %	structure.machineInfo.machineStatsCoopMissionPlayCount).encode('utf-8'))

	outsave.write(("\n").encode('utf-8'))

	# machine highscore table
	outsave.write((">>>>> Machine highscore table data <<<<<\n").encode('utf-8'))
	for entry in structure.machineScoreTable:
		outsave.write(("Song ID: %X\n" % 			entry.songID).encode('utf-8'))
		outsave.write(("Difficulty/Chart: %d\n" %	entry.difficulty).encode('utf-8'))		
		outsave.write(("Mode: 0x%X\n" %				entry.mode).encode('utf-8'))	
		outsave.write(("Grade: 0x%X\n" %			entry.grade).encode('utf-8'))
		outsave.write(("Clear state: 0x%X\n" %		entry.clearState).encode('utf-8'))
		outsave.write(("Score: %d\n" %				entry.scoreTotal).encode('utf-8'))
		outsave.write(("Play count total: %d\n" %	entry.playCountTotal).encode('utf-8'))
		outsave.write(("Unknown: %d\n" %			entry.unknown).encode('utf-8'))
		outsave.write(("Player ID: %s\n" %			entry.playerID).encode('utf-8'))
		outsave.write(("Unknown 2: %d\n" %			entry.unknown2).encode('utf-8'))
		outsave.write(("Null padding: %d\n" %		entry.nullPadding).encode('utf-8'))		
		outsave.write(("\n").encode('utf-8'))

	outsave.write(("\n").encode('utf-8'))

	# personal highscore table and unlocks
	outsave.write((">>>>> Personal highscore table and unlock data <<<<<\n").encode('utf-8'))
	for entry in structure.personalScoreTable:
		outsave.write(("Song ID: %X\n" % 					entry.songID).encode('utf-8'))
		outsave.write(("Difficulty/Chart: %d\n" %			entry.difficulty).encode('utf-8'))		
		outsave.write(("Mode: 0x%X\n" %						entry.mode).encode('utf-8'))	
		outsave.write(("Grade: 0x%X\n" %					entry.grade).encode('utf-8'))
		outsave.write(("Clear and unlock state: 0x%X\n" %	entry.clearAndUnlockState).encode('utf-8'))
		outsave.write(("Score: %d\n" %						entry.scoreTotal).encode('utf-8'))
		outsave.write(("Play count total: %d\n" %			entry.playCountTotal).encode('utf-8'))
		outsave.write(("Null padding: %d\n" %				entry.nullPadding).encode('utf-8'))		
		outsave.write(("\n").encode('utf-8'))

	outsave.close()

	return 0


#-------------------------------------------------------------------------------------------
##
# Encrypts/decrypts specified profile files
#
# @param mode Mode to set, either "enc" or "dec"
# @param infilesave Fiesta 2 save file input path
# @param infilerank Fiesta 2 rank file input path
# @param outfilesave Path for save output file
# @param outfilerank Path for rank output file
#
def encDecCmd(mode, infilesave, infilerank, outfilesave, outfilerank):
	insave = open(infilesave,"rb")
	insavedata = bytearray(insave.read())
	insave.close()

	inrank = open(infilerank,"rb")
	inrankdata = bytearray(inrank.read())
	inrank.close()

	if mode == "enc":
		outsavedata,outrankdata = encryptProfile(insavedata,inrankdata)
	elif mode == "dec":
		outsavedata,outrankdata = decryptProfile(insavedata,inrankdata)
	else:
		print ("Invalid mode:", mode,)
		print("Pick either enc or dec")
		return 1

	outsave = open(outfilesave,"wb")
	outsave.write(outsavedata)
	outsave.close()

	outrank = open(outfilerank,"wb")
	outrank.write(outrankdata)
	outrank.close()

	return 0


#-------------------------------------------------------------------------------------------
## 
# Print Usage of library in-built ready to use tools
#
def printUsage():
	print("Fiesta 2 profile tools library, usage:", sys.argv[0], "[--tool] <...>")
	print("Tools available:")
	print("\t --profile_dec <infile save enc> <infile rank enc> <outfile save dec> <outfile rank dec>")
	print("\t --profile_enc <infile save dec> <infile rank dec> <outfile save enc> <outfile rank enc>")
	print("\t --dump_save_txt <infile save dec> <outfile txt>")	
	print("\t --unlock [stepcharts|items|allMissionClear|allCoopMissionClear] <infile save dec>")


#-------------------------------------------------------------------------------------------
##
# Define main entry point of library is run on its own with some ready to use tools
#
if __name__ == "__main__":
	if (len(sys.argv) <= 2):
		printUsage()
		sys.exit(1)
	
	if (sys.argv[1] == "help" or sys.argv[1] == "h" or sys.argv[1] == "--help" or sys.argv[1] == "-h"):
		printUsage()
		sys.exit(1)

	if (sys.argv[1] == "--profile_dec" and len(sys.argv) == 6):
		sys.exit(encDecCmd("dec", sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5]))		
	elif (sys.argv[1] == "--profile_enc" and len(sys.argv) == 6):
		sys.exit(encDecCmd("enc", sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5]))	
	elif (sys.argv[1] == "--dump_save_txt" and len(sys.argv) == 4):
		sys.exit(dumpSaveToTxtCmd(sys.argv[2], sys.argv[3]))
	elif (sys.argv[1] == "--unlock" and len(sys.argv) == 4):
		sys.exit(unlockCmd(sys.argv[2], sys.argv[3]))
	else:
		printUsage()
		sys.exit(1)
