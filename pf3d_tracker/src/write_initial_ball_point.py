import numpy as np
import math
import sys

R = 57.0/2.0 #ball radius.
R1=(1-0.20)*R; #ray for placing the internal points [mm]
R2=(1+0.20)*R; #ray for placing the external points [mm]
nPoints=50

t = np.linspace(0.0, 2.0*math.pi, nPoints)

x=np.zeros(nPoints)
z=np.cos(t)
y=np.sin(t)
p1=R1*np.array([x, y, z])
p2=R2*np.array([x, y, z])

points = np.hstack((p1, p2))

file = open("initial_ball_points_smiley_57mm_20percent.csv", "w+")

for a in range(0, 3):
    for b in range(0, nPoints*2+1):
        file.write(str(points(a,b)+"\n")
