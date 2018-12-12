import numpy as np
import math
import sys

R = 28 #ball radius.
R1=(1-0.20)*R #ray for placing the internal points [mm]
R2=(1+0.20)*R #ray for placing the external points [mm]
nPoints=50


t = np.arange(0, 2*math.pi, 2*math.pi/nPoints)

x=np.zeros(nPoints)
z=np.cos(t)
y=np.sin(t)
p1=R1*np.vstack((x, y, z))
p2=R2*np.vstack((x, y, z))

points = np.hstack((p1, p2))

file = open("initial_ball_points_smiley_28mm_20percent.csv", "w+")

for a in range(0, 3):
    for b in range(0, nPoints*2):
        file.write("%.6f\n" % (points[a, b]))
