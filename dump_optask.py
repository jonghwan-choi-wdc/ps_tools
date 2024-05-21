import sys

def read_hex_values_from_file(filename):
    with open(filename, 'r') as file:
        data = file.read()
    
    # 16진수 값을 공백 기준으로 분리하여 리스트로 저장
    hex_values = data.split()
    return hex_values

def print_hex_values_in_chunks(hex_values, chunk_size=20):
    for i in range(0, len(hex_values), chunk_size):
        chunk = hex_values[i:i + chunk_size]
        print(' '.join(chunk))
        if chunk and chunk[0] == "00630209":
            print("OP_TYPE_UNCACHED_RD_TRANS_DECODE_TO_SRAM")
        if chunk and chunk[0] == "00200000":
            print("OP_TYPE_REQUEST_NOP")
        if chunk and chunk[0] == "00221200":
            print("OP_TYPE_NOP_LDPC_DECODE")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <filename>")
        sys.exit(1)

    filename = sys.argv[1]
    hex_values = read_hex_values_from_file(filename)
    print_hex_values_in_chunks(hex_values)

