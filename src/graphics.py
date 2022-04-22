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
from matplotlib import colors

multiple_images = 0
one_image       = 1

def draw_bar(n_elems, htable_size, lines, n_cur_line):
    fig = plt.figure(figsize=(14, 7))

    axes = []
    
    data = list(map(int, lines[n_cur_line + 2].split()))
    lists_len = list(map(int, lines[n_cur_line + 3].split()))
    axes.append(fig.add_subplot(half_n_hashs, 3, n_hash + 1))

    dispersion  = round(float(np.var(lists_len)), 2)

    nonzero_lens = list([lists_len[i] for i in range(len(lists_len)) if lists_len[i] != 0])
    descr = 'дисперсия: ' + str(dispersion) + '\n'
    descr += 'average: '  + str(round(float(sum(nonzero_lens) / len(nonzero_lens)), 1))

    axes[0].set_title(lines[n_cur_line] + descr, loc='left', pad = 1.01)
    #axes[0].set_ylim([0, max(data)])
    # skip name and description
    n_cur_line += 4
    print(len(data))
    N, bins, patches = axes[0].hist(data, bins=int(int(len(data))/10), align='left', bottom=0.5)
    fracs = N / N.max()

    norm = colors.Normalize(fracs.min(), fracs.max())

    for thisfrac, thispatch in zip(fracs, patches):
        color = plt.cm.viridis(norm(thisfrac))
        thispatch.set_facecolor(color)

    return n_cur_line


def draw_multiple_bar(n_elems, htable_size, n_hashs, lines):
    fig = plt.figure(figsize=(14,7))

    axes = []
    half_n_hashs = int((n_hashs + 2)/ 3)

    n_cur_line = 2

    for n_hash in range(n_hashs):

        data = list(map(int, lines[n_cur_line + 2].split()))
        lists_len = list(map(int, lines[n_cur_line + 3].split()))
        axes.append(fig.add_subplot(half_n_hashs, 3, n_hash + 1))

        dispersion  = round(float(np.var(lists_len)), 2)

        nonzero_lens = list([lists_len[i] for i in range(len(lists_len)) if lists_len[i] != 0])
        
        descr = 'дисперсия: ' + str(dispersion) + '\n'
        descr += 'average: '  + str(round(float(sum(nonzero_lens) / len(nonzero_lens)), 1))

        axes[n_hash].set_title(lines[n_cur_line] + descr, loc='left', pad = 1.01)
        #axes[n_hash].set_ylim([0, max(data)])

        #axes[n_hash].set_ylim([0, max(data)])
        # skip name and description
        n_cur_line += 4
        print(len(data))
        N, bins, patches = axes[n_hash].hist(data, bins=int(int(len(data))/50), align='left', bottom=0.5)
        fracs = N / N.max()

        norm = colors.Normalize(fracs.min(), fracs.max())

        for thisfrac, thispatch in zip(fracs, patches):
            color = plt.cm.viridis(norm(thisfrac))
            thispatch.set_facecolor(color)

    plt.subplots_adjust(hspace = 0.4)

def main():
    input_file = open('temp', 'r')
    lines = input_file.readlines()
    input_file.close()

    draw_mode, is_show = list(map(int, lines[0].split()))
    n_elems, htable_size, n_hashs = list(map(int, lines[1].split()))

    if(draw_mode == one_image):
        draw_multiple_bar(n_elems, htable_size, n_hashs, lines)
        if(is_show):
            plt.show()
        plt.savefig('bars.png')

    elif(draw_mode == multiple_images):
        n_cur_line = 2
        for i in range(n_hashs):
            n_init_line = n_cur_line
            n_cur_line = draw_bar(n_elems, htable_size, lines, n_cur_line)
            if(is_show):
                plt.show()
        plt.savefig(str(n_init_line) + '.png')
    return 0

if __name__ == '__main__':
    main()
