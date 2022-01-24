# Script para visualizar el fractal generado por mandel.c
# Uso (secuencial):
# $ gcc -o mandel mandel.c
# $ ./mandel > mandel.txt
# $ python2 view.py mandel.txt
#
# Es necesario tener instalado Python v2.x y los paquetes numpy y matplotlib.
# Las maquinas de docencia no disponen de matplotlib, por lo que este script no funcionara.

import numpy as np
import matplotlib.pyplot as plt
import math
import sys

# Extract points from specified file
im = np.loadtxt( sys.argv[1] )

# Display
plt.imshow(im,cmap=plt.cm.flag)
plt.show()

