import numpy as np
import matplotlib.pyplot as plt

"""
from matplotlib import colors
from matplotlib.ticker import PercentFormatter

https://matplotlib.org/stable/gallery/statistics/hist.html

n_bins = 10

dist1 = [0, 1, 2, 3, 4, 5, 5, 5, 5, 4]
# N is the count in each bin, bins is the lower-limit of the bin
N, bins, patches = plt.hist(dist1, bins=n_bins)

# We'll color code by height, but you could use any scalar
fracs = N / N.max()

# we need to normalize the data to 0..1 for the full range of the colormap
norm = colors.Normalize(fracs.min(), fracs.max())

# Now, we'll loop through our objects and set the color of each accordingly
for thisfrac, thispatch in zip(fracs, patches):
    color = plt.cm.viridis(norm(thisfrac))
    thispatch.set_facecolor(color)

# We can also normalize our inputs by the total number of counts
plt.hist(dist1, bins=n_bins, density=True)

plt.show()
"""

import numpy as np
import matplotlib.pyplot as plt

input_file = open('temp', 'r')
lines = input_file.readlines()
input_file.close()

n_elems, htable_size, n_hashs = list(map(int, lines[0].split()))

fig = plt.figure()

axes = []
half_n_hashs = int((n_hashs + 2)/ 3)

n_cur_line = 1

for n_hash in range(n_hashs):
    axes.append(fig.add_subplot(half_n_hashs, 3, n_hash + 1))
    axes[n_hash].set_title(lines[n_cur_line], loc='left', pad = 1.01)

    # skip name and description
    n_cur_line += 2

    data = list(map(int, lines[n_cur_line].split()))
    n_cur_line += 1

    axes[n_hash].bar(np.arange(htable_size), data, width=1.0, color='#000000')

plt.subplots_adjust(hspace = 0.4)
plt.show()
