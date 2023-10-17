import time
import sys


class FPSCount:
    def __init__(self):
        self._start = time.time()
        self._frames = 0

    def fps(self) -> int:
        new_time = time.time()
        delta = new_time - self._start
        self._frames +=1
        if delta > 1 : # 1 second
            print("FPS: ", str(round(self._frames / delta)).zfill(2))
            sys.stdout.write("\033[F")
            self._frames = 0
            self._start = time.time()



class FixedTimeStep:
    dt = 1./60.;
    def __init__(self):
        self._prev_point = time.time();
        self._lag_accum  = 0;

    def sleep(self):
        """
        Sleep for a certain length of time to maintain the a regular
        framerate.
        """
        new_point = time.time();
        frame_time = new_point - self._prev_point;
        alpha =  frame_time
        self._lag_accum += FixedTimeStep.dt - frame_time
        self._lag_accum = max(-0.25, self._lag_accum)
        while self._lag_accum > 0:
            t1 = time.time()
            time.sleep(self._lag_accum)
            t2 = time.time()
            self._lag_accum -= t2 - t1
            alpha = t2 - t1
        self._prev_point =  time.time()
        return alpha


def main():
    fps_counter     = FPSCount();
    fixed_time_step = FixedTimeStep()
    while True:
        alpha = fixed_time_step.sleep();
        fps_counter.fps()
        print(f"step game for: {alpha:.3f} | fixed dt {FixedTimeStep.dt:.3f}")
        # hard work
        # time.sleep(0.06)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        exit(0)





