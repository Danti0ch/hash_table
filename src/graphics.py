import numpy as np
import matplotlib.pyplot as plt
from matplotlib import colors

multiple_images = 0
one_image       = 1

def draw_bar(n_elems, htable_size, lines, n_cur_line, y_lim):
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


def draw_multiple_bar(n_elems, htable_size, n_hashs_low, n_hashs_high, lines):
    fig = plt.figure(figsize=(14,7))

    axes = []
    half_n_hashs = int((n_hashs_low + n_hashs_high + 2)/ 3)

    n_cur_line = 2

    for n_hash in range(n_hashs_low + n_hashs_high):

        data = list(map(int, lines[n_cur_line + 2].split()))
        lists_len = list(map(int, lines[n_cur_line + 3].split()))
        axes.append(fig.add_subplot(half_n_hashs, 3, n_hash + 1))

        dispersion  = round(float(np.var(lists_len)), 2)

        nonzero_lens = list([lists_len[i] for i in range(len(lists_len)) if lists_len[i] != 0])
        
        descr = 'дисперсия: ' + str(dispersion) + '\n'
        descr += 'average: '  + str(round(float(sum(nonzero_lens) / len(nonzero_lens)), 1))

        axes[n_hash].set_title(lines[n_cur_line] + descr, loc='left', pad = 1.01)
        
        #if(max(data) < 10):
        #    axes[n_hash].bar([i for i in range(8)], lists_len[:8], color=['yellow'], width=0.03)
        #    n_cur_line += 4

    
        if(n_hash < n_hashs_low):
            axes[n_hash].bar([i for i in range(160)], lists_len[:160],  width=0.52)
            axes[n_hash].set_xlim([-1, 128])
            axes[n_hash].set_ylim([0, 7000])
            
        else:
            N, bins, patches = axes[n_hash].hist(data, bins=int(int(len(data))/50), align='left', bottom=0.5)
            axes[n_hash].set_xlim([-1, 8192])   
            axes[n_hash].set_ylim([0, 300])   
            
            # skip name and description

            fracs = N / N.max()

            norm = colors.Normalize(fracs.min(), fracs.max())

            #for thisfrac, thispatch in zip(fracs, patches):
            #    color = plt.cm.viridis(norm(thisfrac))
            #    thispatch.set_facecolor(color)
        n_cur_line+=4
            
        
    fig.tight_layout()
    plt.subplots_adjust(hspace = 0.4)

def main():
    input_file = open('temp', 'r')
    lines = input_file.readlines()
    input_file.close()

    draw_mode, is_show = list(map(int, lines[0].split()))
    n_elems, htable_size, n_hashs_low, n_hash_high = list(map(int, lines[1].split()))

    if(draw_mode == one_image):
        draw_multiple_bar(n_elems, htable_size, n_hashs_low, n_hash_high, lines)
        if(is_show):
            plt.show()
        plt.savefig('bars.png')

    elif(draw_mode == multiple_images):
        n_cur_line = 2
        for i in range(n_hashs_low):
            n_init_line = n_cur_line
            n_cur_line = draw_bar(n_elems, htable_size, lines, n_cur_line, 100)
            if(is_show):
                plt.show()
        for i in range(n_hashs_high):
            n_init_line = n_cur_line
            n_cur_line = draw_bar(n_elems, htable_size, lines, n_cur_line, 8000)
            if(is_show):
                plt.show()
        plt.savefig(str(n_init_line) + '.png')
    return 0

if __name__ == '__main__':
    main()
