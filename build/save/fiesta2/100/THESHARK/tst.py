import binascii
def scramble_buffer(data, seed=0x8993):
    scrambled_data = bytearray(len(data))
    for i, byte in enumerate(data):
        scrambled_data[i] = byte ^ ((seed >> 8) & 0xff)
        seed = seed * (scrambled_data[i] + seed) - 0x231
    return scrambled_data

def unscramble_buffer(scrambled_data):
    data = bytearray(len(scrambled_data))
    seed = 0xebada1
    for i in range(0,len(scrambled_data)):
        smbuff = data[i]
        data[i] ^= (seed >> 8) & 0xFF
        seed = (0x68993 * (smbuff + seed) + 0x4FDCF) & 0xFFFFFFFF;        
    return data

unscrambled_data = unscramble_buffer(b"\xE2\x60\x70\xF8\x9B\xC2\x73\x2B\x27\x7D\x28\x95\x90\x41\x22\x64")
print("Unscrambled data: ", binascii.hexlify(unscrambled_data))
print("Unscrambled data: ",unscrambled_data)
    
