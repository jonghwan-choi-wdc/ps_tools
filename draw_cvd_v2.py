import sys
import matplotlib.pyplot as plt

class Cvd:
    def __init__(self, file_path, max_cvd_dist_count=344, interval=0.025):
        self.file_path = file_path
        self.max_cvd_dist_count = max_cvd_dist_count
        self.interval = interval
        self.counts = []
        self.elements = []
        self.vts = []
        self.file_index = 0
        self.vba = None

    def process_file(self):
        try:
            with open(self.file_path, 'r') as file:
                self._process_lines(file)
        except FileNotFoundError:
            print(f"File not found: {self.file_path}")
            sys.exit(1)

    def _process_lines(self, file):
        count = 0
        for line in file:
            line = self._get_fw_line(line)

            if "zeroCnt" in line:
                self._process_zero_count_line(line, count)
                count += 1

                if count == self.max_cvd_dist_count:
                    self._process_element_counts()
                    self.plot_graph()
                    self._reset_data()
                    count = 0
            elif "PS: EH: Read CVD Dist LogErr" in line:
                self._process_log_error_line(line)

    def _get_fw_line(self, line):
        fw_index = line.find("FW")
        return line[fw_index:] if fw_index != -1 else line

    def _process_zero_count_line(self, line, count):
        parts = line.split(',')
        assert len(parts) >= 6, "Line does not have enough elements"

        element = parts[3].replace('"', '').strip()
        element = self._try_parse_element(element)

        self.elements.append(element)
        self.vts.append(count * self.interval)

    def _try_parse_element(self, element):
        if element.startswith('0x'):
            try:
                return int(element, 16)
            except ValueError:
                print(f"Invalid hex value: {element}")
                return None
        return element

    def _process_element_counts(self):
        self.counts = [0] * len(self.elements)
        for i in range(len(self.elements) - 1):
            self.counts[i] = self.elements[i] - self.elements[i + 1]

    def _process_log_error_line(self, line):
        parts = line.split(',')
        vba_parts = parts[3].split('|')
        self.vba = vba_parts[1]

    def _reset_data(self):
        self.counts = []
        self.elements = []
        self.vts = []

    # plot_graph 메소드는 동일하게 유지
    def plot_graph(self):
        print("Draw Graph VBA : {}".format(self.vba))
        vt_values = self.vts
        cnt_values = self.counts

        filtered_vt_values = [vt for vt, cnt in zip(vt_values, cnt_values) if cnt > 0]
        filtered_cnt_values = [cnt for cnt in cnt_values if cnt > 0]

        # Plotting the graph
#        plt.figure()
        plt.figure(figsize=(7.04, 5.28))
        plt.plot(filtered_vt_values, filtered_cnt_values, 'b.-') # Blue color, with dot markers and solid lines
        plt.scatter(filtered_vt_values, filtered_cnt_values) # Blue color, with dot markers and solid lines
        plt.yscale('log')  # Set the y-axis to a logarithmic scale

        plt.xlabel('vt (count)')
        plt.ylabel('cnt')

        if self.vba is None:
            self.vba = "0xFFFFFFFF"

        plt.title("VBA : " + self.vba)

        plt.grid(True)

        # Save the plot as an image file
        plt.savefig('graph.png')

        # 파일 이름에 호출 카운트를 포함합니다.
        filename = f'graph_{self.vba}_{self.file_index}.png'
        plt.savefig(filename)

        self.file_index += 1

        # Show the plot
        plt.show()

        # Clear the current plot
        plt.clf()
               
if __name__ == "__main__":
    # 메인 실행 부분은 동일하게 유지
    if len(sys.argv) < 2:
        print("Usage: python script.py filename.txt")
        sys.exit(1)

    file_path = sys.argv[1]
    cvd = Cvd(file_path)
    cvd.process_file()


