
import argparse
import struct

# 구조체 정의
class Mailbox_ExcpBarrier:
    def __init__(self, data):
        # 4바이트(32비트) 값을 비트 필드로 분해
        self.messageID = (data >> 24) & 0xFF       # 9비트
        self.barrierReqIdx = (data >> 16) & 0xFF        # 1비트
        self.rsv0 = (data >> 14) & 0x3      # 4비트
        self.rsvPSReqIndex = (data >> 10) & 0xF               # 2비트
        self.isBroadCast = (data >> 9) & 0x1      # 8비트
        self.psReqIndex = data & 0x1FF                 # 8비트

    def __str__(self):
        message_id_map = {
                          0: 'MAILBOX_RESERVED0_MSGID',
                          1: 'MAILBOX_RESERVED0_MSGID',
                          2: 'MAILBOX_RD_COMPLETION_MSG_ID',
                          3: 'THREE',
                          4: 'FOUR',
                          5: 'FIVE',
                          6: 'SIX',
                          7: 'SEVEN',
                          8: 'EIGHT',
                          9: 'MAILBOX_FIM_FW_LATENCY_MSG_ID',
                         15: 'MAILBOX_RLC_DPCA_MSG_ID',
                         23: 'MAILBOX_PS_EXCP_BARRIER_MSG_ID',
                         30: 'MAILBOX_PS_CVD_DIST_READ_MSG_ID',
                         38: 'MAILBOX_XOR_STORE_LOAD_XBID_OPBID_UPDATE_MSG_ID',
                         39: 'MAILBOX_XOR_STORE_LOAD_XBID_OPBID_SYNC_MSG_ID'
        }
        message_id_str = message_id_map.get(self.messageID, hex(self.messageID))

        return (f"psReqIndex: {hex(self.psReqIndex)}, isBroadCast: {hex(self.isBroadCast)}, "
                f"rsvPSReqIndex: {hex(self.rsvPSReqIndex)}, rsv0: {hex(self.rsv0)}, "
                f"barrierReqIdx: {hex(self.barrierReqIdx)}, messageID: {message_id_str}")
             #   f"barrierReqIdx: {hex(self.barrierReqIdx)}, messageID: {hex(self.messageID)}")

# 실행 인자를 처리하는 함수
def main(file_path):
    # 파일 읽기
    with open(file_path, 'r') as file:
        hex_values = file.read().split()

    # 16진수 값을 구조체로 변환하여 출력
    for hex_value in hex_values:
        data = int(hex_value, 16)  # 16진수 문자열을 정수로 변환
        excp_barrier = Mailbox_ExcpBarrier(data)
        print(excp_barrier)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process hex values and map to structure.')
    parser.add_argument('file_path', type=str, help='Path to the file containing hex values.')
    args = parser.parse_args()
    main(args.file_path)

