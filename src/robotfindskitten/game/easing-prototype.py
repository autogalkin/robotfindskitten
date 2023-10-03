
from typing import Tuple
import math 

def easeInOutQuart(t):
    t *= 2
    if t < 1:
        return t * t * t * t / 2
    else:
        t -= 2
        return -(t * t * t * t - 2) / 2

def easeInQuad(t):
    return t * t

def easeInQuint(t):
    return t * t * t * t * t

def easeInExpo(t):
    return math.pow(2, 10 * (t - 1))

def easeOutExpo(t):
    return -math.pow(2, -10 * t) + 1

def easeInOutExpo(t):
    t *= 2
    if t < 1:
        return math.pow(2, 10 * (t - 1)) / 2
    else:
        t -= 1
        return -math.pow(2, -10 * t) - 1

def easeInOutCubic(t):
    t *= 2
    if t < 1:
        return t * t * t / 2
    else:
        t -= 2
        return (t * t * t + 2) / 2

def easeInOutSine(t):
    import math
    return -(math.cos(math.pi * t) - 1) / 2

def easeInSine(t):
    import math
    return -math.cos(t * math.pi / 2) + 1


width = 100
height = 20

buf = [' ']*width*height
for i in range(int(len(buf)/width)):
    buf[i*width-2] = '|'
    buf[i*width-1] = '\n'

def set(point: Tuple[int, int], v):
    try:
        if buf[int(point[1] * width +  point[0])] != '\n':
            buf[int(point[1] * width +  point[0])] = v
    except: pass

point = [1., 2.]
set(point, '-')

dt = 1./60.

total_iterations = int(5 / dt)
print(total_iterations)

start_x = point[0]
end_x = start_x + 60
changing_x = end_x - start_x

start_y = point[1]
end_y = start_y + 10
changing_y = end_y - start_y
for current_iteration in range(total_iterations):
    x = changing_x * easeOutExpo(current_iteration/total_iterations) + start_x
    y = changing_y * easeInSine(current_iteration/total_iterations) + start_y
    point[0] = round(x)
    point[1] = round(y)
    set(point, '-' if point[1] - start_y < 6 else '_')

print('_'*(width-2))
print(''.join(buf))




