import numpy as np
from parse import compile


class SimulatedTime(object):
    def __init__(self, logFile):
        f = open(logFile, 'r')
        lines = f.readlines()
        f.close()

        parser = compile(
            "|{}|  (STATUS) [F:DepositionCosmics] Total simulated time in CRY: {:g}{}")

        lines = [parser.parse(l.strip()) for l in lines]
        resultLines = [l for l in lines if l is not None]
        assert len(resultLines) == 1
        timestamp, time, unit = resultLines[0]
        if unit == 's':
            self.time = time
        elif unit == 'ms':
            self.time = time * 1e-3
        elif unit == 'us':
            self.time = time * 1e-6
        elif unit == 'ns':
            self.time = time * 1e-9


if __name__ == '__main__':
    st = SimulatedTime("../log.txt")
    print(st.time)
